<?php

namespace Keel\App\Services;

use Keel\Core\Database;
use Keel\Core\Env;
use Keel\Core\Mailer;

class OtpService
{
    private const TOKEN_TYPE = 'otp';
    private const EXPIRY_MINUTES = 10;
    private const RATE_LIMIT_WINDOW_MINUTES = 15;
    private const RATE_LIMIT_MAX = 5;

    public function requestCode(string $email): array
    {
        $user = $this->findOrCreateUser($email);

        if ($this->tooManyRequests($user['id'])) {
            return ['success' => false, 'message' => 'Too many requests. Please try again later.'];
        }

        $code = str_pad((string) random_int(0, 999999), 6, '0', STR_PAD_LEFT);
        $hash = password_hash($code, PASSWORD_DEFAULT);
        $expiresAt = date('Y-m-d H:i:s', strtotime('+' . self::EXPIRY_MINUTES . ' minutes'));

        $stmt = Database::connection()->prepare(
            'INSERT INTO auth_tokens (user_id, type, token_hash, expires_at, created_at) VALUES (?, ?, ?, ?, NOW())'
        );
        $stmt->execute([$user['id'], self::TOKEN_TYPE, $hash, $expiresAt]);

        $sent = Mailer::send(
            $email,
            $user['name'] ?? $email,
            'Your verification code',
            $this->emailTemplate($code)
        );

        return ['success' => $sent, 'message' => $sent ? 'Code sent.' : 'Failed to send email.'];
    }

    public function verifyCode(string $email, string $code): array
    {
        $user = $this->findUserByEmail($email);
        if (!$user) {
            return ['success' => false, 'message' => 'Invalid code.'];
        }

        $stmt = Database::connection()->prepare(
            'SELECT * FROM auth_tokens WHERE user_id = ? AND type = ? AND used_at IS NULL AND expires_at > NOW() ORDER BY id DESC LIMIT 5'
        );
        $stmt->execute([$user['id'], self::TOKEN_TYPE]);

        foreach ($stmt->fetchAll() as $token) {
            if (password_verify($code, $token['token_hash'])) {
                $update = Database::connection()->prepare('UPDATE auth_tokens SET used_at = NOW() WHERE id = ?');
                $update->execute([$token['id']]);

                return ['success' => true, 'user' => $user];
            }
        }

        return ['success' => false, 'message' => 'Invalid or expired code.'];
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

    private function emailTemplate(string $code): string
    {
        $appName = Env::get('APP_NAME', 'App');
        return <<<HTML
        <div style="font-family: Arial, sans-serif; max-width: 480px; margin: 0 auto; padding: 32px;">
            <h2 style="color: #111827;">{$appName} verification code</h2>
            <p style="color: #4b5563; font-size: 15px;">Use this code to sign in. It expires in 10 minutes.</p>
            <div style="font-size: 32px; font-weight: 700; letter-spacing: 6px; background: #f3f4f6; padding: 16px 24px; border-radius: 8px; text-align: center; margin: 24px 0;">{$code}</div>
            <p style="color: #9ca3af; font-size: 13px;">If you didn't request this, you can safely ignore this email.</p>
        </div>
        HTML;
    }
}
