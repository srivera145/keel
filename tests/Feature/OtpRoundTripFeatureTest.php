<?php

declare(strict_types=1);

namespace Tests\Feature;

use Tests\TestCase;

class OtpRoundTripFeatureTest extends TestCase
{
    public function testOtpRoundTripEstablishesSessionAndRedirectsToDashboard(): void
    {
        $email = 'otp_user@example.test';

        $requestResponse = $this->postJson('/auth/otp/request', ['email' => $email], [
            'X-CSRF-Token' => $this->csrfToken(),
        ]);

        self::assertSame(200, $requestResponse->status);
        self::assertTrue((bool) ($requestResponse->json()['success'] ?? false));

        $mailLog = $this->latestMailLog();
        self::assertNotSame('', $mailLog);

        preg_match('/\b(\d{6})\b/', $mailLog, $matches);
        $code = $matches[1] ?? '';

        self::assertMatchesRegularExpression('/^\d{6}$/', $code);

        $verifyResponse = $this->postJson('/auth/otp/verify', [
            'email' => $email,
            'code' => $code,
        ], [
            'X-CSRF-Token' => $this->csrfToken(),
        ]);

        self::assertSame(200, $verifyResponse->status);
        self::assertSame('/dashboard', (string) ($verifyResponse->json()['redirect'] ?? ''));
        self::assertTrue(isset($_SESSION['user_id']) && (int) $_SESSION['user_id'] > 0);
    }
}
