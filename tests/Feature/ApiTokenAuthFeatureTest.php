<?php

declare(strict_types=1);

namespace Tests\Feature;

use Keel\Core\Database;
use Tests\TestCase;

class ApiTokenAuthFeatureTest extends TestCase
{
    public function testValidTokenAuthenticatesApiRoute(): void
    {
        $user = $this->createUser(['email' => 'api-user@example.test']);
        $token = $this->createApiToken((int) $user['id']);

        $response = $this->getJson('/api/v1/files', [
            'Authorization' => 'Bearer ' . $token['token'],
        ]);

        self::assertSame(200, $response->status);
        self::assertIsArray($response->json()['data'] ?? null);
    }

    public function testInvalidOrExpiredTokenIsRejected(): void
    {
        $invalid = $this->getJson('/api/v1/files', [
            'Authorization' => 'Bearer not-a-real-token',
        ]);

        self::assertSame(401, $invalid->status);

        $user = $this->createUser(['email' => 'expired-user@example.test']);
        $expired = $this->createApiToken((int) $user['id'], date('Y-m-d H:i:s', strtotime('-1 hour')));

        $expiredResponse = $this->getJson('/api/v1/files', [
            'Authorization' => 'Bearer ' . $expired['token'],
        ]);

        self::assertSame(401, $expiredResponse->status);
    }

    public function testBearerTokenCannotCreateOrRevokeOtherTokens(): void
    {
        $user = $this->createUser(['email' => 'scoped-user@example.test']);
        $token = $this->createApiToken((int) $user['id']);

        $csrf = $this->csrfToken();

        $createResponse = $this->postJson('/settings/api-tokens', [
            'name' => 'Should Fail',
            'abilities' => '*',
        ], [
            'Authorization' => 'Bearer ' . $token['token'],
            'X-CSRF-Token' => $csrf,
            'Accept' => 'application/json',
        ]);

        self::assertSame(401, $createResponse->status);

        $revokeResponse = $this->postJson('/settings/api-tokens/1/revoke', [], [
            'Authorization' => 'Bearer ' . $token['token'],
            'X-CSRF-Token' => $csrf,
            'Accept' => 'application/json',
        ]);

        self::assertSame(401, $revokeResponse->status);

        $count = (int) Database::connection()->query('SELECT COUNT(*) FROM api_tokens')->fetchColumn();
        self::assertSame(1, $count);
    }
}
