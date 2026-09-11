<?php

namespace Keel\App\Services;

use Keel\Core\Database;
use Keel\Core\Env;
use Keel\Core\Mailer;

class MagicLinkService
{
    private const TOKEN_TYPE = 'magic_link';
    private const EXPIRY_MINUTES = 15;
    private const RATE_LIMIT_WINDOW_MINUTES = 15;
    private const RATE_LIMIT_MAX = 5;

    public function sendLink(string $email): array
    {
        $user = $this->findOrCreateUser($email);

        if ($this->tooManyRequests($user['id'])) {
            return ['success' => false, 'message' => 'Too many requests. Please try again later.'];
        }

        $rawToken = bin2hex(random_bytes(32));
        $hash = hash('sha256', $rawToken);
        $expiresAt = date('Y-m-d H:i:s', strtotime('+' . self::EXPIRY_MINUTES . ' minutes'));

        $stmt = Database::connection()->prepare(
            'INSERT INTO auth_tokens (user_id, type, token_hash, expires_at, created_at) VALUES (?, ?, ?, ?, NOW())'
        );
        $stmt->execute([$user['id'], self::TOKEN_TYPE, $hash, $expiresAt]);

        $url = rtrim(Env::get('APP_URL'), '/') . '/auth/magic?token=' . $rawToken . '&email=' . urlencode($email);

        $sent = Mailer::send(
            $email,
            $user['name'] ?? $email,
            'Your sign-in link',
            $this->emailTemplate($url)
        );

        return ['success' => $sent, 'message' => $sent ? 'Link sent.' : 'Failed to send email.'];
    }

    public function verifyToken(string $email, string $rawToken): array
    {
        $user = $this->findUserByEmail($email);
        if (!$user) {
            return ['success' => false, 'message' => 'Invalid link.'];
        }

        $hash = hash('sha256', $rawToken);

        $stmt = Database::connection()->prepare(
            'SELECT * FROM auth_tokens WHERE user_id = ? AND type = ? AND token_hash = ? AND used_at IS NULL AND expires_at > NOW() LIMIT 1'
        );
        $stmt->execute([$user['id'], self::TOKEN_TYPE, $hash]);
        $token = $stmt->fetch();

        if (!$token) {
            return ['success' => false, 'message' => 'Invalid or expired link.'];
        }

        $update = Database::connection()->prepare('UPDATE auth_tokens SET used_at = NOW() WHERE id = ?');
        $update->execute([$token['id']]);

        return ['success' => true, 'user' => $user];
    }

    private function tooManyRequests(int $userId): bool
    {
        $windowStart = date('Y-m-d H:i:s', strtotime('-' . self::RATE_LIMIT_WINDOW_MINUTES . ' minutes'));

        $stmt = Database::connection()->prepare(
            'SELECT COUNT(*) as cnt FROM auth_tokens WHERE user_id = ? AND type = ? AND created_at > ?'
        );
        $stmt->execute([$userId, self::TOKEN_TYPE, $windowStart]);

        return (int) $stmt->fetch()['cnt'] >= self::RATE_LIMIT_MAX;
    }

    private function findOrCreateUser(string $email): array
    {
        $user = $this->findUserByEmail($email);
        if ($user) {
            return $user;
        }

        $stmt = Database::connection()->prepare('INSERT INTO users (email, created_at) VALUES (?, NOW())');
        $stmt->execute([$email]);

        return $this->findUserByEmail($email);
    }

    private function findUserByEmail(string $email): ?array
    {
        $stmt = Database::connection()->prepare('SELECT * FROM users WHERE email = ? LIMIT 1');
        $stmt->execute([$email]);
        return $stmt->fetch() ?: null;
    }

    private function emailTemplate(string $url): string
    {
        $appName = Env::get('APP_NAME', 'App');
        return <<<HTML
        <div style="font-family: Arial, sans-serif; max-width: 480px; margin: 0 auto; padding: 32px;">
            <h2 style="color: #111827;">Sign in to {$appName}</h2>
            <p style="color: #4b5563; font-size: 15px;">Click the button below to sign in. This link expires in 15 minutes.</p>
            <a href="{$url}" style="display: inline-block; background: #111827; color: #ffffff; text-decoration: none; padding: 12px 24px; border-radius: 8px; font-weight: 600; margin: 16px 0;">Sign in</a>
            <p style="color: #9ca3af; font-size: 13px;">If you didn't request this, you can safely ignore this email.</p>
        </div>
        HTML;
    }
}
