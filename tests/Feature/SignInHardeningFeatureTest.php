<?php

declare(strict_types=1);

namespace Tests\Feature;

use Keel\Core\Database;
use Tests\TestCase;

/** OTP codes and magic links: single use, expiry, wrong-guess limit, and limits per email and per IP address. */
class SignInHardeningFeatureTest extends TestCase
{
    private function requestCode(string $email, string $ip = '203.0.113.10'): array
    {
        $_SERVER['REMOTE_ADDR'] = $ip;

        return $this->postJson('/auth/otp/request', ['email' => $email], ['X-CSRF-Token' => $this->csrfToken()])->json();
    }

    private function verify(string $email, string $code, string $ip = '203.0.113.10'): array
    {
        $_SERVER['REMOTE_ADDR'] = $ip;

        return $this->postJson('/auth/otp/verify', ['email' => $email, 'code' => $code], ['X-CSRF-Token' => $this->csrfToken()])->json();
    }

    private function requestLink(string $email, string $ip = '203.0.113.10'): array
    {
        $_SERVER['REMOTE_ADDR'] = $ip;

        return $this->postJson('/auth/magic/request', ['email' => $email], ['X-CSRF-Token' => $this->csrfToken()])->json();
    }

    private function lastCodeFromMail(): string
    {
        preg_match_all('/\b(\d{6})\b/', $this->latestMailLog(), $matches);

        return (string) end($matches[1]);
    }

    private function lastLinkFromMail(): string
    {
        preg_match_all('#(/auth/magic\?token=[a-f0-9]{64}&(?:amp;)?email=[^\s"<\']+)#', $this->latestMailLog(), $matches);

        return html_entity_decode((string) end($matches[1]));
    }

    public function testFiveWrongCodesBurnTheCode(): void
    {
        self::assertTrue($this->requestCode('guess@example.test')['success']);
        $code = $this->lastCodeFromMail();
        $wrong = $code === '000000' ? '111111' : '000000';

        for ($i = 1; $i <= 4; $i++) {
            self::assertSame('Invalid or expired code.', $this->verify('guess@example.test', $wrong)['message'], "guess {$i}");
        }
        self::assertSame('Too many wrong codes. Request a new code.', $this->verify('guess@example.test', $wrong)['message']);

        // The right code no longer works: a new one must be requested.
        self::assertFalse($this->verify('guess@example.test', $code)['success']);

        self::assertTrue($this->requestCode('guess@example.test')['success']);
        self::assertTrue($this->verify('guess@example.test', $this->lastCodeFromMail())['success']);
    }

    public function testMalformedCodesCountAsWrongGuesses(): void
    {
        $this->requestCode('malformed@example.test');
        $code = $this->lastCodeFromMail();

        foreach (['abc', '12345', $code . '0', 'abcdef', '12-456'] as $guess) {
            self::assertFalse($this->verify('malformed@example.test', $guess)['success'], $guess);
        }

        self::assertFalse($this->verify('malformed@example.test', $code)['success']);
    }

    public function testCodesAreSingleUse(): void
    {
        $this->requestCode('once@example.test');
        $code = $this->lastCodeFromMail();

        self::assertTrue($this->verify('once@example.test', $code)['success']);
        self::assertFalse($this->verify('once@example.test', $code)['success']);
    }

    /**
     * Expiries are written with PHP's clock (UTC) and compared with NOW(). The connection's session time zone is
     * UTC, so a code expires on time whatever the database server's own time zone is (at UTC-6 a code used to
     * stay valid for six hours).
     */
    public function testCodesExpireOnTimeWhateverTheServerTimeZone(): void
    {
        $this->requestCode('late@example.test');
        $code = $this->lastCodeFromMail();

        // A minute past its expiry by the UTC clock PHP wrote it with.
        $statement = Database::connection()->prepare("UPDATE auth_tokens SET expires_at = ? WHERE type = 'otp'");
        $statement->execute([date('Y-m-d H:i:s', time() - 60)]);

        self::assertFalse($this->verify('late@example.test', $code)['success']);
    }

    public function testFiveCodesPerEmailPerFifteenMinutes(): void
    {
        for ($i = 0; $i < 5; $i++) {
            self::assertTrue($this->requestCode('many@example.test', '203.0.113.' . (20 + $i))['success'], "request {$i}");
        }

        self::assertFalse($this->requestCode('many@example.test', '203.0.113.99')['success']);
    }

    public function testTenCodeRequestsPerIpPerFifteenMinutes(): void
    {
        for ($i = 0; $i < 10; $i++) {
            self::assertTrue($this->requestCode("ip{$i}@example.test", '198.51.100.7')['success'], "request {$i}");
        }

        $blocked = $this->requestCode('ip-next@example.test', '198.51.100.7');
        self::assertFalse($blocked['success']);
        self::assertSame('Too many requests. Please try again later.', $blocked['message']);

        // The limit was checked before the account was created.
        $statement = Database::connection()->prepare('SELECT COUNT(*) FROM users WHERE email = ?');
        $statement->execute(['ip-next@example.test']);
        self::assertSame(0, (int) $statement->fetchColumn());

        self::assertTrue($this->requestCode('ip-next@example.test', '198.51.100.8')['success'], 'another IP is not affected');
    }

    public function testTwentyVerificationAttemptsPerIpPerFifteenMinutes(): void
    {
        for ($i = 0; $i < 20; $i++) {
            self::assertFalse($this->verify("nobody{$i}@example.test", '123456', '192.0.2.50')['success']);
        }

        self::assertSame('Too many attempts. Please try again later.', $this->verify('nobody@example.test', '123456', '192.0.2.50')['message']);
        self::assertSame('Invalid code.', $this->verify('nobody@example.test', '123456', '192.0.2.51')['message'], 'another IP is not affected');
    }

    public function testMagicLinkIsSingleUse(): void
    {
        self::assertTrue($this->requestLink('link@example.test')['success']);
        $link = $this->lastLinkFromMail();

        self::assertSame('/dashboard', $this->get($link)->header('Location'));

        $_SESSION = [];
        self::assertSame('/login?error=invalid_link', $this->get($link)->header('Location'), 'a used link does not sign in again');
        self::assertArrayNotHasKey('user_id', $_SESSION);
    }

    public function testMagicLinkExpires(): void
    {
        $this->requestLink('expired@example.test');
        $link = $this->lastLinkFromMail();

        $statement = Database::connection()->prepare("UPDATE auth_tokens SET expires_at = ? WHERE type = 'magic_link'");
        $statement->execute([date('Y-m-d H:i:s', time() - 60)]);

        self::assertSame('/login?error=invalid_link', $this->get($link)->header('Location'));
    }

    public function testTenMagicLinkRequestsPerIpPerFifteenMinutes(): void
    {
        for ($i = 0; $i < 10; $i++) {
            self::assertTrue($this->requestLink("link{$i}@example.test", '198.51.100.20')['success'], "request {$i}");
        }

        $blocked = $this->requestLink('link-next@example.test', '198.51.100.20');
        self::assertFalse($blocked['success']);
        self::assertSame('Too many requests. Please try again later.', $blocked['message']);

        self::assertTrue($this->requestLink('link-next@example.test', '198.51.100.21')['success'], 'another IP is not affected');
    }
}
