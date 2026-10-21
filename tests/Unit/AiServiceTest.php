<?php

declare(strict_types=1);

namespace Tests\Unit;

use Keel\App\Services\AiService;
use PHPUnit\Framework\TestCase;

/** AiService against a fake transport: where requests go, streaming, and what is (not) logged. */
class AiServiceTest extends TestCase
{
    private const ENV_KEYS = ['ANTHROPIC_API_KEY', 'ANTHROPIC_BASE_URL', 'ANTHROPIC_MODEL', 'APP_DEBUG'];

    /** @var array<string, mixed> */
    private array $envBackup = [];

    /** @var list<array{url: string, headers: array<int, string>, body: array<string, mixed>}> */
    private array $sent = [];

    private string|false $errorLog;
    private string $logFile;

    protected function setUp(): void
    {
        parent::setUp();

        foreach (self::ENV_KEYS as $key) {
            $this->envBackup[$key] = $_ENV[$key] ?? null;
        }
        $_ENV['ANTHROPIC_API_KEY'] = 'test-key';
        $_ENV['ANTHROPIC_BASE_URL'] = '';
        $_ENV['ANTHROPIC_MODEL'] = 'claude-test-model';
        $_ENV['APP_DEBUG'] = 'false';

        $this->logFile = (string) tempnam(sys_get_temp_dir(), 'keel-ai-log-');
        $this->errorLog = ini_get('error_log');
        ini_set('error_log', $this->logFile);
    }

    protected function tearDown(): void
    {
        AiService::useTransport(null);
        ini_set('error_log', (string) $this->errorLog);
        @unlink($this->logFile);

        foreach ($this->envBackup as $key => $value) {
            if ($value === null) {
                unset($_ENV[$key]);
            } else {
                $_ENV[$key] = $value;
            }
        }

        parent::tearDown();
    }

    /**
     * A fake Anthropic. Without $chunks it answers once with $body; with them, it streams them to the callback and
     * stops when the callback returns false, as cURL does.
     *
     * @param list<string> $chunks
     */
    private function fake(int $status = 200, ?string $body = null, array $chunks = [], string $error = ''): void
    {
        $body ??= (string) json_encode(['content' => [['type' => 'text', 'text' => ' Hello there ']]]);

        AiService::useTransport(function (string $url, array $headers, string $requestBody, ?callable $onChunk) use ($status, $body, $chunks, $error): array {
            $this->sent[] = ['url' => $url, 'headers' => $headers, 'body' => (array) json_decode($requestBody, true)];

            if ($onChunk === null || $status !== 200) {
                return ['status' => $status, 'body' => $body, 'error' => $error];
            }

            foreach ($chunks as $chunk) {
                if (!$onChunk($chunk)) {
                    return ['status' => 200, 'body' => '', 'error' => 'Failure writing output to destination'];
                }
            }

            return ['status' => 200, 'body' => '', 'error' => $error];
        });
    }

    public function testCompleteSendsAMessagesRequestToAnthropic(): void
    {
        $this->fake();

        self::assertSame('Hello there', (new AiService())->complete('Say hello', ['maxTokens' => 64]));

        self::assertSame('https://api.anthropic.com/v1/messages', $this->sent[0]['url']);
        self::assertContains('x-api-key: test-key', $this->sent[0]['headers']);
        self::assertContains('anthropic-version: 2023-06-01', $this->sent[0]['headers']);
        self::assertSame([
            'model' => 'claude-test-model',
            'max_tokens' => 64,
            'messages' => [['role' => 'user', 'content' => 'Say hello']],
        ], $this->sent[0]['body']);
    }

    public function testAnthropicBaseUrlPointsRequestsElsewhere(): void
    {
        $_ENV['ANTHROPIC_BASE_URL'] = 'http://127.0.0.1:8099/';
        $this->fake();

        (new AiService())->complete('Hi');

        self::assertSame('http://127.0.0.1:8099/v1/messages', $this->sent[0]['url']);
    }

    public function testCompleteJsonStillDecodesTheAnswer(): void
    {
        $this->fake(200, (string) json_encode(['content' => [['type' => 'text', 'text' => '{"ok":true}']]]));

        self::assertSame(['ok' => true], (new AiService())->completeJson('Answer in JSON'));
    }

    public function testMessagesSendsThePayloadAsGivenAndHandsBackErrorResponses(): void
    {
        $error = (string) json_encode(['type' => 'error', 'error' => ['type' => 'overloaded_error', 'message' => 'Overloaded']]);
        $this->fake(529, $error);
        $payload = [
            'model' => 'claude-test-model',
            'max_tokens' => 16,
            'system' => 'Be brief.',
            'messages' => [['role' => 'user', 'content' => 'Hi']],
            'stream' => true,
        ];

        $result = (new AiService())->messages($payload);

        self::assertSame(['status' => 529, 'body' => $error], $result);
        unset($payload['stream']);
        self::assertSame($payload, $this->sent[0]['body']);
        self::assertContains('Accept: application/json', $this->sent[0]['headers']);
    }

    public function testMessagesThrowsWhenAnthropicCannotBeReached(): void
    {
        $this->fake(0, '', [], 'Could not resolve host');

        $this->expectException(\RuntimeException::class);
        $this->expectExceptionMessage('Anthropic request failed or timed out.');

        (new AiService())->messages(['model' => 'm', 'max_tokens' => 1, 'messages' => []]);
    }

    public function testStreamHandsOverEachChunkAsItArrives(): void
    {
        $events = [
            "event: message_start\ndata: {\"type\":\"message_start\"}\n\n",
            "event: content_block_delta\ndata: {\"type\":\"content_block_delta\",\"delta\":{\"type\":\"text_delta\",\"text\":\"Hi\"}}\n\n",
            "event: message_stop\ndata: {\"type\":\"message_stop\"}\n\n",
        ];
        $this->fake(200, null, $events);
        $received = [];

        $result = (new AiService())->stream(['model' => 'm', 'max_tokens' => 8, 'messages' => []], static function (string $chunk) use (&$received): bool {
            $received[] = $chunk;

            return true;
        });

        self::assertSame($events, $received);
        self::assertSame(['status' => 200, 'body' => '', 'aborted' => false], $result);
        self::assertTrue($this->sent[0]['body']['stream']);
        self::assertContains('Accept: text/event-stream', $this->sent[0]['headers']);
    }

    public function testStreamStopsWhenTheCallbackSaysTheClientIsGone(): void
    {
        $this->fake(200, null, ['one', 'two', 'three', 'four']);
        $received = [];

        $result = (new AiService())->stream(['model' => 'm', 'max_tokens' => 8, 'messages' => []], static function (string $chunk) use (&$received): bool {
            $received[] = $chunk;

            return count($received) < 2;
        });

        self::assertSame(['one', 'two'], $received, 'nothing is read after the client goes');
        self::assertTrue($result['aborted']);
    }

    public function testStreamReturnsAnErrorResponseInsteadOfStreamingIt(): void
    {
        $this->fake(429, '{"type":"error","error":{"type":"rate_limit_error"}}');
        $called = false;

        $result = (new AiService())->stream(['model' => 'm', 'max_tokens' => 8, 'messages' => []], static function () use (&$called): bool {
            $called = true;

            return true;
        });

        self::assertFalse($called);
        self::assertSame(429, $result['status']);
        self::assertStringContainsString('rate_limit_error', $result['body']);
    }

    public function testFailuresAreLoggedWithoutTheBodyEvenWithDebug(): void
    {
        $_ENV['APP_DEBUG'] = 'true';
        $this->fake(400, (string) json_encode(['type' => 'error', 'error' => ['message' => 'echoes PRIVATE-PROMPT-TEXT']]));

        try {
            (new AiService())->complete('PRIVATE-PROMPT-TEXT');
            self::fail('An error response did not throw.');
        } catch (\RuntimeException $exception) {
            self::assertSame('Anthropic request failed with status 400.', $exception->getMessage());
        }

        $log = (string) file_get_contents($this->logFile);
        self::assertStringContainsString('"status":400', $log);
        self::assertStringNotContainsString('PRIVATE-PROMPT-TEXT', $log);
    }

    public function testAMissingApiKeyIsReported(): void
    {
        $_ENV['ANTHROPIC_API_KEY'] = '';
        $this->fake();

        $this->expectExceptionMessage('ANTHROPIC_API_KEY is not configured.');

        (new AiService())->complete('Hi');
    }
}
