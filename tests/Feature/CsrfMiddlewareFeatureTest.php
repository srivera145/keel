<?php

declare(strict_types=1);

namespace Tests\Feature;

use Tests\TestCase;

class CsrfMiddlewareFeatureTest extends TestCase
{
    public function testCsrfRejectsMissingTokenAndAllowsValidToken(): void
    {
        $withoutToken = $this->postJson('/auth/otp/request', [
            'email' => 'csrf_missing@example.test',
        ]);

        self::assertSame(419, $withoutToken->status);

        $withToken = $this->postJson('/auth/otp/request', [
            'email' => 'csrf_valid@example.test',
        ], [
            'X-CSRF-Token' => $this->csrfToken(),
        ]);

        self::assertSame(200, $withToken->status);
        self::assertTrue((bool) ($withToken->json()['success'] ?? false));
    }
}
