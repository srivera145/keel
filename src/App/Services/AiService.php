<?php

namespace Keel\App\Services;

use Keel\Core\Env;

class AiService
{
    private const API_URL = 'https://api.anthropic.com/v1/messages';

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

    private function request(array $payload): array
    {
        $apiKey = trim((string) Env::get('ANTHROPIC_API_KEY', ''));
        if ($apiKey === '') {
            throw new \RuntimeException('ANTHROPIC_API_KEY is not configured.');
        }

        $body = json_encode($payload, JSON_UNESCAPED_SLASHES);
        if ($body === false) {
            throw new \RuntimeException('Failed to encode Anthropic request payload.');
        }

        $handle = curl_init(self::API_URL);
        curl_setopt_array($handle, [
            CURLOPT_POST => true,
            CURLOPT_RETURNTRANSFER => true,
            CURLOPT_HTTPHEADER => [
                'Content-Type: application/json',
                'x-api-key: ' . $apiKey,
                'anthropic-version: 2023-06-01',
            ],
            CURLOPT_POSTFIELDS => $body,
            CURLOPT_TIMEOUT => 30,
        ]);

        $rawResponse = curl_exec($handle);
        $status = (int) curl_getinfo($handle, CURLINFO_HTTP_CODE);
        $curlError = curl_error($handle);
        curl_close($handle);

        if ($rawResponse === false || $curlError !== '') {
            $this->logFailure('Anthropic request failed', ['status' => $status, 'curl_error' => $curlError]);
            throw new \RuntimeException('Anthropic request failed or timed out.');
        }

        $decoded = json_decode($rawResponse, true);

        if ($status !== 200) {
            $this->logFailure('Anthropic returned an error response', [
                'status' => $status,
                'body' => $decoded ?? $rawResponse,
            ]);
            throw new \RuntimeException('Anthropic request failed with status ' . $status . '.');
        }

        if (!is_array($decoded)) {
            throw new \RuntimeException('Anthropic returned an unreadable response.');
        }

        return $decoded;
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

    private function logFailure(string $message, array $context): void
    {
        if ((bool) Env::get('APP_DEBUG', false)) {
            error_log('[Keel] ' . $message . ': ' . json_encode($context));
            return;
        }

        $sanitized = $context;
        unset($sanitized['body']);
        error_log('[Keel] ' . $message . ': ' . json_encode($sanitized));
    }
}