<?php

namespace Keel\App\Services;

use Keel\Core\Env;

/**
 * Claude through Anthropic's Messages API, over cURL.
 *
 * complete(), completeWithImage() and completeJson() send one prompt and return the text. messages() and stream()
 * send a whole Messages API request as given, for a page or proxy that builds its own: stream() hands each piece of
 * the server-sent event stream to a callback as it arrives, and stops reading (and so stops paying for tokens) as
 * soon as the callback returns false. To relay a stream to a browser and stop when it goes away:
 *
 *     ignore_user_abort(true);
 *     header('Content-Type: text/event-stream');
 *     header('Cache-Control: no-store');
 *     $ai->stream($payload, static function (string $chunk): bool {
 *         echo $chunk;
 *         flush();
 *         return connection_aborted() === 0;
 *     });
 *
 * Nothing here logs a request or response body (prompts and answers are user data): failures are logged with the
 * HTTP status and the cURL error only. ANTHROPIC_BASE_URL points it at another host (a gateway, a local fake), and
 * useTransport() replaces cURL in tests.
 */
class AiService
{
    private const DEFAULT_BASE_URL = 'https://api.anthropic.com';
    private const VERSION = '2023-06-01';

    /** @var (callable(string, array<int, string>, string, ?callable): array{status: int, body: string, error: string})|null */
    private static $transport = null;

    /**
     * For tests: replace cURL. The callable gets the URL, the request headers, the JSON body and (when streaming)
     * the chunk callback, and returns the status, the body (what it did not stream) and a transport error ('' for
     * none). Pass null to go back to cURL.
     */
    public static function useTransport(?callable $transport): void
    {
        self::$transport = $transport;
    }

    public function complete(string $prompt, array $options = []): string
    {
        $response = $this->request([
            'model' => $this->model(),
            'max_tokens' => $this->maxTokens($options),
            'messages' => [[
                'role' => 'user',
                'content' => $prompt,
            ]],
        ]);

        return $this->extractText($response);
    }

    public function completeWithImage(string $prompt, string $imagePath, array $options = []): string
    {
        if (!is_file($imagePath)) {
            throw new \RuntimeException('Image file not found.');
        }

        $mimeType = (string) (new \finfo(FILEINFO_MIME_TYPE))->file($imagePath);
        if (!in_array($mimeType, ['image/jpeg', 'image/png'], true)) {
            throw new \RuntimeException('Only JPEG and PNG images are supported for multimodal requests.');
        }

        $contents = file_get_contents($imagePath);
        if ($contents === false) {
            throw new \RuntimeException('Failed to read image file.');
        }

        $response = $this->request([
            'model' => $this->model(),
            'max_tokens' => $this->maxTokens($options),
            'messages' => [[
                'role' => 'user',
                'content' => [
                    [
                        'type' => 'image',
                        'source' => [
                            'type' => 'base64',
                            'media_type' => $mimeType,
                            'data' => base64_encode($contents),
                        ],
                    ],
                    [
                        'type' => 'text',
                        'text' => $prompt,
                    ],
                ],
            ]],
        ]);

        return $this->extractText($response);
    }

    public function completeJson(string $prompt, array $options = []): array
    {
        $jsonPrompt = $prompt . "\n\nReturn only valid JSON. Do not include markdown fences or commentary.";
        $text = $this->complete($jsonPrompt, $options);
        $decoded = json_decode($text, true);

        if (!is_array($decoded) || json_last_error() !== JSON_ERROR_NONE) {
            throw new \RuntimeException('Anthropic returned malformed JSON: ' . json_last_error_msg());
        }

        return $decoded;
    }

    /**
     * One Messages API request, sent as given (any `stream` key is dropped). The status and raw body come back
     * whatever the status, for the caller to read or pass on; only a transport failure throws.
     *
     * @return array{status: int, body: string}
     */
    public function messages(array $payload, int $timeoutSeconds = 120): array
    {
        unset($payload['stream']);
        $result = $this->send($payload, null, $timeoutSeconds);

        if ($result['error'] !== '') {
            $this->logFailure('Anthropic request failed', ['status' => $result['status'], 'curl_error' => $result['error']]);
            throw new \RuntimeException('Anthropic request failed or timed out.');
        }

        return ['status' => $result['status'], 'body' => $result['body']];
    }

    /**
     * A streaming Messages API request (`stream: true` is set). Each chunk of a 200 response's event stream goes
     * to $onChunk as it arrives: raw bytes, so an event can be split across two chunks. Returning false stops the
     * transfer, and `aborted` comes back true. A non-200 response is not streamed: its body comes back instead.
     *
     * @param callable(string): bool $onChunk
     * @return array{status: int, body: string, aborted: bool}
     */
    public function stream(array $payload, callable $onChunk, int $timeoutSeconds = 300): array
    {
        $payload['stream'] = true;
        $aborted = false;

        $result = $this->send($payload, static function (string $chunk) use ($onChunk, &$aborted): bool {
            if ($onChunk($chunk) === false) {
                $aborted = true;

                return false;
            }

            return true;
        }, $timeoutSeconds);

        if ($result['error'] !== '' && !$aborted) {
            $this->logFailure('Anthropic stream failed', ['status' => $result['status'], 'curl_error' => $result['error']]);
            throw new \RuntimeException('Anthropic request failed or timed out.');
        }

        return ['status' => $result['status'], 'body' => $result['body'], 'aborted' => $aborted];
    }

    private function request(array $payload): array
    {
        $result = $this->send($payload, null, 30);

        if ($result['error'] !== '') {
            $this->logFailure('Anthropic request failed', ['status' => $result['status'], 'curl_error' => $result['error']]);
            throw new \RuntimeException('Anthropic request failed or timed out.');
        }

        if ($result['status'] !== 200) {
            $this->logFailure('Anthropic returned an error response', ['status' => $result['status']]);
            throw new \RuntimeException('Anthropic request failed with status ' . $result['status'] . '.');
        }

        $decoded = json_decode($result['body'], true);
        if (!is_array($decoded)) {
            throw new \RuntimeException('Anthropic returned an unreadable response.');
        }

        return $decoded;
    }

    /**
     * @param (callable(string): bool)|null $onChunk
     * @return array{status: int, body: string, error: string}
     */
    private function send(array $payload, ?callable $onChunk, int $timeoutSeconds): array
    {
        $apiKey = trim((string) Env::get('ANTHROPIC_API_KEY', ''));
        if ($apiKey === '') {
            throw new \RuntimeException('ANTHROPIC_API_KEY is not configured.');
        }

        $body = json_encode($payload, JSON_UNESCAPED_SLASHES);
        if ($body === false) {
            throw new \RuntimeException('Failed to encode Anthropic request payload.');
        }

        $baseUrl = trim((string) Env::get('ANTHROPIC_BASE_URL', ''));
        $url = rtrim($baseUrl !== '' ? $baseUrl : self::DEFAULT_BASE_URL, '/') . '/v1/messages';
        $headers = [
            'Content-Type: application/json',
            'Accept: ' . ($onChunk !== null ? 'text/event-stream' : 'application/json'),
            'x-api-key: ' . $apiKey,
            'anthropic-version: ' . self::VERSION,
        ];

        if (self::$transport !== null) {
            return (self::$transport)($url, $headers, $body, $onChunk);
        }

        $handle = curl_init($url);
        $status = 0;
        $buffered = '';
        $options = [
            CURLOPT_POST => true,
            CURLOPT_HTTPHEADER => $headers,
            CURLOPT_POSTFIELDS => $body,
            CURLOPT_CONNECTTIMEOUT => 10,
            CURLOPT_TIMEOUT => $timeoutSeconds,
            CURLOPT_RETURNTRANSFER => $onChunk === null,
        ];

        if ($onChunk !== null) {
            // Called with each piece of the body as it arrives. Returning less than its length aborts the transfer.
            $options[CURLOPT_WRITEFUNCTION] = static function ($curl, string $data) use ($onChunk, &$status, &$buffered): int {
                $status = $status ?: (int) curl_getinfo($curl, CURLINFO_RESPONSE_CODE);
                if ($status !== 200) {
                    $buffered .= $data;

                    return strlen($data);
                }

                return $onChunk($data) ? strlen($data) : 0;
            };
        }

        curl_setopt_array($handle, $options);

        $raw = curl_exec($handle);
        $status = (int) curl_getinfo($handle, CURLINFO_HTTP_CODE);
        $error = curl_errno($handle) !== 0 ? curl_error($handle) : '';
        curl_close($handle);

        return [
            'status' => $status,
            'body' => $onChunk === null ? (is_string($raw) ? $raw : '') : $buffered,
            'error' => $error,
        ];
    }

    private function extractText(array $response): string
    {
        $parts = $response['content'] ?? [];

        foreach ($parts as $part) {
            if (($part['type'] ?? null) === 'text' && isset($part['text'])) {
                return trim((string) $part['text']);
            }
        }

        throw new \RuntimeException('Anthropic response did not contain any text content.');
    }

    private function model(): string
    {
        // Check this default against Anthropic's current model lineup before shipping.
        return trim((string) Env::get('ANTHROPIC_MODEL', 'claude-sonnet-4-5'));
    }

    private function maxTokens(array $options): int
    {
        return max(1, (int) ($options['maxTokens'] ?? 1024));
    }

    /** Status and transport errors only: a request or response body is never logged, APP_DEBUG or not. */
    private function logFailure(string $message, array $context): void
    {
        error_log('[Keel] ' . $message . ': ' . json_encode($context));
    }
}
