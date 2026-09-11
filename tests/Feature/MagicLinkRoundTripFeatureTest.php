<?php

declare(strict_types=1);

namespace Tests\Feature;

use Tests\TestCase;

class MagicLinkRoundTripFeatureTest extends TestCase
{
    public function testMagicLinkRoundTripAuthenticatesAndRedirects(): void
    {
        $email = 'magic_user@example.test';

        $requestResponse = $this->postJson('/auth/magic/request', ['email' => $email], [
            'X-CSRF-Token' => $this->csrfToken(),
        ]);

        self::assertSame(200, $requestResponse->status);
        self::assertTrue((bool) ($requestResponse->json()['success'] ?? false));

        $mailLog = $this->latestMailLog();
        self::assertNotSame('', $mailLog);

        preg_match('/\/auth\/magic\?token=([a-f0-9]{64})&(?:amp;)?email=([^\s<"\']+)/i', $mailLog, $matches);
        $token = $matches[1] ?? '';
        $encodedEmail = $matches[2] ?? '';

        self::assertSame(64, strlen($token));
        self::assertNotSame('', $encodedEmail);

        $response = $this->get('/auth/magic?token=' . $token . '&email=' . $encodedEmail);

        self::assertSame(302, $response->status);
        self::assertSame('/dashboard', $response->header('Location'));
        self::assertTrue(isset($_SESSION['user_id']) && (int) $_SESSION['user_id'] > 0);
    }
}
