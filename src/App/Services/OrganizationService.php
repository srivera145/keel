<?php

namespace Keel\App\Services;

use Keel\App\Jobs\SendOrganizationInviteJob;
use Keel\App\Models\User;
use Keel\Core\Activity;
use Keel\Core\Database;
use Keel\Core\Env;
use Keel\Core\Queue;

class OrganizationService
{
    private const INVITE_EXPIRY_DAYS = 7;
    private const VALID_ROLES = ['owner', 'admin', 'member'];

    public function create(string $name, int $ownerUserId): array
    {
        $name = trim($name);

        if ($name === '') {
            throw new \RuntimeException('Organization name is required.');
        }

        $slug = $this->uniqueSlug($name);
        $connection = Database::connection();
        $connection->beginTransaction();

        try {
            $statement = $connection->prepare('INSERT INTO organizations (name, slug) VALUES (?, ?)');
            $statement->execute([$name, $slug]);
            $organizationId = (int) $connection->lastInsertId();

            $updateUser = $connection->prepare('UPDATE users SET organization_id = ?, role = ? WHERE id = ?');
            $updateUser->execute([$organizationId, 'owner', $ownerUserId]);

            $connection->commit();
        } catch (\Throwable $exception) {
            if ($connection->inTransaction()) {
                $connection->rollBack();
            }

            throw $exception;
        }

        return $this->organization($organizationId) ?? [];
    }

    public function invite(int $organizationId, string $email, string $role, int $invitedBy): array
    {
        $email = strtolower(trim($email));
        $role = $this->normalizeRole($role);

        if (!filter_var($email, FILTER_VALIDATE_EMAIL)) {
            throw new \RuntimeException('Enter a valid email address.');
        }

        $organization = $this->organization($organizationId);
        if (!$organization) {
            throw new \RuntimeException('Organization not found.');
        }

        $rawToken = bin2hex(random_bytes(32));
        $hash = hash('sha256', $rawToken);
        $expiresAt = date('Y-m-d H:i:s', strtotime('+' . self::INVITE_EXPIRY_DAYS . ' days'));

        $connection = Database::connection();
        $connection->beginTransaction();

        try {
            $statement = $connection->prepare(
                'INSERT INTO organization_invites (organization_id, email, role, token_hash, invited_by, expires_at)
                 VALUES (?, ?, ?, ?, ?, ?)'
            );
            $statement->execute([$organizationId, $email, $role, $hash, $invitedBy, $expiresAt]);
            $inviteId = (int) $connection->lastInsertId();

            $inviteUrl = rtrim((string) Env::get('APP_URL', ''), '/') . '/invite/accept?email=' . urlencode($email) . '&token=' . urlencode($rawToken);
            Queue::push(SendOrganizationInviteJob::class, [
                'to_email' => $email,
                'to_name' => $email,
                'subject' => 'You have been invited to join ' . $organization['name'],
                'html_body' => $this->inviteTemplate($organization['name'], $role, $inviteUrl),
            ]);

            Activity::log('organization.invite_sent', 'OrganizationInvite', $inviteId, [
                'email' => $email,
                'role' => $role,
            ]);

            $connection->commit();
        } catch (\Throwable $exception) {
            if ($connection->inTransaction()) {
                $connection->rollBack();
            }

            throw $exception;
        }

        return [
            'success' => true,
            'email' => $email,
            'role' => $role,
            'expires_at' => $expiresAt,
        ];
    }

    public function acceptInvite(string $email, string $rawToken): array
    {
        $email = strtolower(trim($email));
        $hash = hash('sha256', $rawToken);

        $statement = Database::connection()->prepare(
            'SELECT * FROM organization_invites
             WHERE email = ? AND token_hash = ? AND accepted_at IS NULL AND expires_at > NOW()
             ORDER BY id DESC LIMIT 1'
        );
        $statement->execute([$email, $hash]);
        $invite = $statement->fetch();

        if (!$invite) {
            throw new \RuntimeException('This invite is invalid or has expired.');
        }

        $connection = Database::connection();
        $connection->beginTransaction();

        try {
            $user = User::findByEmail($email);

            if ($user && !empty($user['organization_id']) && (int) $user['organization_id'] !== (int) $invite['organization_id']) {
                throw new \RuntimeException('This email already belongs to another organization.');
            }

            if (!$user) {
                $insertUser = $connection->prepare('INSERT INTO users (email, organization_id, role, created_at) VALUES (?, ?, ?, NOW())');
                $insertUser->execute([$email, $invite['organization_id'], $invite['role']]);
                $user = User::find((int) $connection->lastInsertId());
            } else {
                $updateUser = $connection->prepare('UPDATE users SET organization_id = ?, role = ? WHERE id = ?');
                $updateUser->execute([$invite['organization_id'], $invite['role'], $user['id']]);
                $user = User::find((int) $user['id']);
            }

            $updateInvite = $connection->prepare('UPDATE organization_invites SET accepted_at = NOW() WHERE id = ?');
            $updateInvite->execute([$invite['id']]);

            Activity::log('organization.invite_accepted', 'Organization', (int) $invite['organization_id'], [
                'email' => $email,
            ]);

            $connection->commit();
        } catch (\Throwable $exception) {
            if ($connection->inTransaction()) {
                $connection->rollBack();
            }

            throw $exception;
        }

        return [
            'success' => true,
            'user' => $user,
            'organization' => $this->organization((int) $invite['organization_id']),
        ];
    }

    public function members(int $organizationId): array
    {
        $statement = Database::connection()->prepare(
            'SELECT id, email, role, is_super_admin, created_at FROM users WHERE organization_id = ? ORDER BY created_at ASC, id ASC'
        );
        $statement->execute([$organizationId]);

        return $statement->fetchAll();
    }

    public function pendingInvites(int $organizationId): array
    {
        $statement = Database::connection()->prepare(
            'SELECT id, email, role, expires_at, created_at FROM organization_invites
             WHERE organization_id = ? AND accepted_at IS NULL AND expires_at > NOW()
             ORDER BY created_at DESC'
        );
        $statement->execute([$organizationId]);

        return $statement->fetchAll();
    }

    public function organization(int $organizationId): ?array
    {
        $statement = Database::connection()->prepare('SELECT * FROM organizations WHERE id = ? LIMIT 1');
        $statement->execute([$organizationId]);

        return $statement->fetch() ?: null;
    }

    public function allOrganizations(): array
    {
        $statement = Database::connection()->query(
            'SELECT o.*, COUNT(u.id) AS member_count
             FROM organizations o
             LEFT JOIN users u ON u.organization_id = o.id
             GROUP BY o.id
             ORDER BY o.created_at DESC, o.id DESC'
        );

        return $statement->fetchAll();
    }

    private function uniqueSlug(string $name): string
    {
        $base = trim((string) preg_replace('/[^a-z0-9]+/i', '-', strtolower($name)), '-');
        $base = $base !== '' ? $base : 'organization';
        $slug = $base;
        $suffix = 2;

        while ($this->slugExists($slug)) {
            $slug = $base . '-' . $suffix;
            $suffix++;
        }

        return $slug;
    }

    private function slugExists(string $slug): bool
    {
        $statement = Database::connection()->prepare('SELECT 1 FROM organizations WHERE slug = ? LIMIT 1');
        $statement->execute([$slug]);

        return (bool) $statement->fetchColumn();
    }

    private function normalizeRole(string $role): string
    {
        $role = strtolower(trim($role));

        if (!in_array($role, self::VALID_ROLES, true)) {
            throw new \RuntimeException('That organization role is not allowed.');
        }

        return $role;
    }

    private function inviteTemplate(string $organizationName, string $role, string $inviteUrl): string
    {
        $appName = (string) Env::get('APP_NAME', 'Keel App');

        return <<<HTML
        <div style="font-family: Arial, sans-serif; max-width: 480px; margin: 0 auto; padding: 32px;">
            <h2 style="color: #111827;">Join {$organizationName} on {$appName}</h2>
            <p style="color: #4b5563; font-size: 15px;">You have been invited as an {$role}. This invite expires in 7 days.</p>
            <a href="{$inviteUrl}" style="display: inline-block; background: #111827; color: #ffffff; text-decoration: none; padding: 12px 24px; border-radius: 8px; font-weight: 600; margin: 16px 0;">Accept invite</a>
            <p style="color: #9ca3af; font-size: 13px;">If you were not expecting this invite, you can safely ignore this email.</p>
        </div>
        HTML;
    }
}