<?php

namespace Keel\Core;

class Auth
{
    private static ?int $resolvedUserId = null;

    public static function setUserId(?int $userId): void
    {
        self::$resolvedUserId = $userId;
    }

    public static function id(): ?int
    {
        if (self::$resolvedUserId !== null) {
            return self::$resolvedUserId;
        }

        $sessionUserId = Session::get('user_id');

        return $sessionUserId ? (int) $sessionUserId : null;
    }

    public static function check(): bool
    {
        return self::id() !== null;
    }
}
