<?php

declare(strict_types=1);

namespace Tests\Feature;

use Tests\TestCase;

class RateLimitingFeatureTest extends TestCase
{
    public function testThrottleBlocksAfterLimitIsExceeded(): void
    {
        $headers = [
            'X-CSRF-Token' => $this->csrfToken(),
            'Accept' => 'application/json',
        ];

        $blocked = false;

        for ($index = 1; $index <= 35; $index++) {
            $response = $this->postJson('/auth/otp/request', [
                'email' => 'ratelimit' . $index . '@example.test',
            ], $headers);

            if ($response->status === 429) {
                $blocked = true;
                break;
            }
        }

        self::assertTrue($blocked, 'Expected throttle middleware to block requests after exceeding the rate limit.');
    }
}
