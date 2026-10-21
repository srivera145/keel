<?php

declare(strict_types=1);

namespace Tests\Feature;

use Keel\Core\CapturedResponseException;
use Keel\Core\ErrorHandler;
use Tests\Support\TestResponse;
use Tests\TestCase;

/**
 * Error pages go out as responses (ErrorHandler used to exit, which ended the PHPUnit run that hit one), as HTML for
 * pages and as JSON under /api, and never show exception details unless APP_DEBUG is on.
 */
class ErrorPagesFeatureTest extends TestCase
{
    private string|false $errorLog;
    private string $logFile;
    private mixed $appDebug;

    protected function setUp(): void
    {
        parent::setUp();

        $this->appDebug = $_ENV['APP_DEBUG'] ?? null;

        // ErrorHandler logs the exception; keep it out of the test output.
        $this->logFile = (string) tempnam(sys_get_temp_dir(), 'keel-error-log-');
        $this->errorLog = ini_get('error_log');
        ini_set('error_log', $this->logFile);
    }

    protected function tearDown(): void
    {
        ini_set('error_log', (string) $this->errorLog);
        @unlink($this->logFile);
        if ($this->appDebug === null) {
            unset($_ENV['APP_DEBUG']);
        } else {
            $_ENV['APP_DEBUG'] = $this->appDebug;
        }

        parent::tearDown();
    }

    public function testAnUnknownPageIsAnHtml404(): void
    {
        $response = $this->get('/no-such-page');

        self::assertSame(404, $response->status);
        self::assertStringContainsString('text/html', (string) $response->header('Content-Type'));
        self::assertStringContainsString('Page not found', $response->body);
    }

    public function testAnUnknownApiEndpointIsAJson404(): void
    {
        $response = $this->get('/api/v1/no-such-endpoint');

        self::assertSame(404, $response->status);
        self::assertSame('application/json', $response->header('Content-Type'));
        self::assertSame(['error' => 'Not found.'], $response->json());
    }

    public function testAServerErrorHidesTheExceptionWithoutDebug(): void
    {
        $_ENV['APP_DEBUG'] = 'false';
        $response = $this->render(500, '/dashboard', new \RuntimeException('secret detail'));

        self::assertSame(500, $response->status);
        self::assertStringContainsString('Something went wrong', $response->body);
        self::assertStringNotContainsString('secret detail', $response->body);
        self::assertStringContainsString('RuntimeException: secret detail', (string) file_get_contents($this->logFile));
    }

    public function testAServerErrorShowsTheExceptionWithDebug(): void
    {
        $_ENV['APP_DEBUG'] = 'true';
        $response = $this->render(500, '/dashboard', new \RuntimeException('debug detail'));

        self::assertSame(500, $response->status);
        self::assertStringContainsString('debug detail', $response->body);
    }

    public function testAnApiServerErrorIsJsonWithoutDetailsEvenWithDebug(): void
    {
        $_ENV['APP_DEBUG'] = 'true';
        $response = $this->render(500, '/api/v1/files', new \RuntimeException('secret detail'));

        self::assertSame(500, $response->status);
        self::assertSame(['error' => 'Server error.'], $response->json());
    }

    private function render(int $status, string $uri, ?\Throwable $exception): TestResponse
    {
        $_SERVER['REQUEST_URI'] = $uri;

        try {
            ErrorHandler::render($status, $exception);
        } catch (CapturedResponseException $captured) {
            return new TestResponse($captured->status, $captured->headers, $captured->body);
        }
    }
}
