<?php

declare(strict_types=1);

namespace Tests\Feature;

use Keel\Core\Database;
use Tests\TestCase;

/**
 * The database connection runs in UTC, the clock PHP writes timestamps with. Without it, NOW() is the server's local
 * time, and every expiry PHP writes (sign-in codes, links, invites) is off by the server's UTC offset.
 */
class DatabaseTimeZoneFeatureTest extends TestCase
{
    public function testTheSessionTimeZoneIsUtc(): void
    {
        $row = Database::connection()
            ->query('SELECT @@session.time_zone AS session_time_zone, TIMESTAMPDIFF(SECOND, UTC_TIMESTAMP(), NOW()) AS offset_seconds')
            ->fetch();

        self::assertSame('+00:00', $row['session_time_zone']);
        self::assertSame(0, (int) $row['offset_seconds']);
    }

    public function testNowAgreesWithPhpsClock(): void
    {
        self::assertSame('UTC', date_default_timezone_get());

        $now = (string) Database::connection()->query('SELECT NOW()')->fetchColumn();

        self::assertLessThanOrEqual(5, abs(strtotime($now) - time()), "NOW() is {$now}, PHP says " . date('Y-m-d H:i:s'));
    }

    public function testANewConnectionIsUtcToo(): void
    {
        Database::resetConnection();

        self::assertSame('+00:00', Database::connection()->query('SELECT @@session.time_zone')->fetchColumn());
    }
}
