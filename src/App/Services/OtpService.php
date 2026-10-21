<?php

namespace Keel\App\Services;

use Keel\Core\Database;
use Keel\Core\Env;
use Keel\Core\Mailer;
use Keel\Core\RateLimiter;

/**
 * Six-digit sign-in codes. Each code lasts 10 minutes and works once. Limits:
 *   - requests: 5 codes per email and 10 per IP address per 15 minutes;
 *   - guesses: a wrong code counts against every outstanding code for the user, and a code with 5 wrong guesses
 *     against it is burned (a new one must be requested); an IP address gets 20 verification attempts per
 *     15 minutes across all accounts.
 * Without the guess limit a code could be guessed by trying all million of them within its 10 minutes.
 */
class OtpService
{
    private const TOKEN_TYPE = 'otp';
    private const EXPIRY_MINUTES = 10;
    private const RATE_LIMIT_WINDOW_MINUTES = 15;
    private const RATE_LIMIT_MAX = 5;
    private const IP_REQUESTS_MAX = 10;
    private const IP_VERIFY_MAX = 20;
    private const MAX_WRONG_ATTEMPTS = 5;

    public function requestCode(string $email, string $ip = 'unknown'): array
    {
        if (!RateLimiter::attempt('otp:request:ip:' . $ip, self::IP_REQUESTS_MAX, self::RATE_LIMIT_WINDOW_MINUTES)) {
            return ['success' => false, 'message' => 'Too many requests. Please try again later.'];
        }

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

    public function verifyCode(string $email, string $code, string $ip = 'unknown'): array
    {
        if (!RateLimiter::attempt('otp:verify:ip:' . $ip, self::IP_VERIFY_MAX, self::RATE_LIMIT_WINDOW_MINUTES)) {
            return ['success' => false, 'message' => 'Too many attempts. Please try again later.'];
        }

        $user = $this->findUserByEmail($email);
        if (!$user) {
            return ['success' => false, 'message' => 'Invalid code.'];
        }

        $connection = Database::connection();
        $stmt = $connection->prepare(
            'SELECT * FROM auth_tokens WHERE user_id = ? AND type = ? AND used_at IS NULL AND expires_at > NOW() ORDER BY id DESC LIMIT 5'
        );
        $stmt->execute([$user['id'], self::TOKEN_TYPE]);
        $tokens = $stmt->fetchAll();

        if (preg_match('/^\d{6}$/D', $code) === 1) {
            foreach ($tokens as $token) {
                if (password_verify($code, $token['token_hash'])) {
                    // Single use, however many requests carry the code at once: only one of them marks it used.
                    $update = $connection->prepare('UPDATE auth_tokens SET used_at = NOW() WHERE id = ? AND used_at IS NULL');
                    $update->execute([$token['id']]);

                    if ($update->rowCount() !== 1) {
                        return ['success' => false, 'message' => 'Invalid or expired code.'];
                    }

                    return ['success' => true, 'user' => $user];
                }
            }
        }

        if ($tokens !== []) {
            // A wrong guess counts against every outstanding code; at the limit they are all burned.
            $connection->prepare('UPDATE auth_tokens SET attempts = attempts + 1 WHERE user_id = ? AND type = ? AND used_at IS NULL')
                ->execute([$user['id'], self::TOKEN_TYPE]);
            $burn = $connection->prepare('UPDATE auth_tokens SET used_at = NOW() WHERE user_id = ? AND type = ? AND used_at IS NULL AND attempts >= ?');
            $burn->execute([$user['id'], self::TOKEN_TYPE, self::MAX_WRONG_ATTEMPTS]);

            if ($burn->rowCount() > 0) {
                return ['success' => false, 'message' => 'Too many wrong codes. Request a new code.'];
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
