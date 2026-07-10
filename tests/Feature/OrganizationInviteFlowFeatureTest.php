<?php

declare(strict_types=1);

namespace Tests\Feature;

use Keel\Core\Database;
use Tests\TestCase;

class OrganizationInviteFlowFeatureTest extends TestCase
{
    public function testInviteFlowAttachesAcceptedUserToOrganizationWithRole(): void
    {
        $this->enableMultiTenancy();

        $organization = $this->createOrganization('Feature Test Org');
        $owner = $this->actingAs([
            'email' => 'owner@example.test',
            'organization_id' => (int) $organization['id'],
            'role' => 'owner',
        ]);

        self::assertSame((int) $organization['id'], (int) $owner['organization_id']);

        $inviteEmail = 'invitee@example.test';
        $sendInviteResponse = $this->post('/settings/members/invite', [
            '_csrf' => $this->csrfToken(),
            'email' => $inviteEmail,
            'role' => 'member',
        ]);

        self::assertSame(302, $sendInviteResponse->status);
        self::assertSame('/settings/members?status=invite_sent', $sendInviteResponse->header('Location'));

        $inviteRecord = Database::connection()->query('SELECT * FROM organization_invites ORDER BY id DESC LIMIT 1')->fetch();
        self::assertIsArray($inviteRecord);
        self::assertSame($inviteEmail, (string) $inviteRecord['email']);
        self::assertSame('member', (string) $inviteRecord['role']);

        $job = Database::connection()->query('SELECT payload FROM jobs ORDER BY id DESC LIMIT 1')->fetch();
        self::assertIsArray($job);

        $payload = json_decode((string) $job['payload'], true);
        self::assertIsArray($payload);

        $htmlBody = html_entity_decode((string) ($payload['html_body'] ?? ''), ENT_QUOTES | ENT_HTML5, 'UTF-8');
        preg_match('/https?:\/\/[^\"\']+\/invite\/accept\?[^\"\']+/i', $htmlBody, $urlMatch);
        $inviteUrl = $urlMatch[0] ?? '';

        self::assertNotSame('', $inviteUrl);

        $query = (string) parse_url($inviteUrl, PHP_URL_QUERY);
        parse_str($query, $inviteQuery);

        $inviteEmail = (string) ($inviteQuery['email'] ?? '');
        $token = (string) ($inviteQuery['token'] ?? '');

        self::assertSame('invitee@example.test', $inviteEmail);
        self::assertSame(64, strlen($token));
        self::assertSame(hash('sha256', $token), (string) $inviteRecord['token_hash']);

        $acceptResponse = $this->get('/invite/accept?email=' . urlencode($inviteEmail) . '&token=' . $token);

        self::assertSame(302, $acceptResponse->status);
        self::assertSame('/dashboard', $acceptResponse->header('Location'));

        $userStatement = Database::connection()->prepare('SELECT * FROM users WHERE email = ? LIMIT 1');
        $userStatement->execute([$inviteEmail]);
        $acceptedUser = $userStatement->fetch();

        self::assertIsArray($acceptedUser);
        self::assertSame((int) $organization['id'], (int) $acceptedUser['organization_id']);
        self::assertSame('member', (string) $acceptedUser['role']);

        $acceptedInvite = Database::connection()->query('SELECT accepted_at FROM organization_invites ORDER BY id DESC LIMIT 1')->fetch();
        self::assertIsArray($acceptedInvite);
        self::assertNotNull($acceptedInvite['accepted_at']);
    }
}
