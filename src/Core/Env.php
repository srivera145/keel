<?php

namespace Keel\Core;

use Dotenv\Dotenv;

class Env
{
    private static bool $loaded = false;

    public static function load(string $basePath): void
    {
        if (self::$loaded) {
            return;
        }

        $dotenv = Dotenv::createImmutable($basePath);
        $dotenv->safeLoad();
        self::$loaded = true;

        // Every timestamp the app writes is UTC; Database sets the MySQL session to UTC to match.
        date_default_timezone_set('UTC');
    }

    public static function get(string $key, mixed $default = null): mixed
    {
        // ?? skips nulls and getenv() returns a string or false, so $value is never null here.
        $value = $_ENV[$key] ?? $_SERVER[$key] ?? getenv($key);

        if ($value === false) {
            return $default;
        }

        return match (strtolower((string) $value)) {
            'true', '(true)' => true,
            'false', '(false)' => false,
            'null', '(null)' => null,
            'empty', '(empty)' => '',
            default => $value,
        };
    }
}
