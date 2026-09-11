<?php

namespace Keel\Core;

class Csrf
{
    private const SESSION_KEY = '_csrf_token';

    public static function token(): string
    {
        $token = Session::get(self::SESSION_KEY);

        if (is_string($token) && $token !== '') {
            return $token;
        }

        $token = bin2hex(random_bytes(32));
        Session::put(self::SESSION_KEY, $token);

        return $token;
    }

    public static function field(): string
    {
        return '<input type="hidden" name="_csrf" value="' . htmlspecialchars(self::token(), ENT_QUOTES, 'UTF-8') . '">';
    }

    public static function verify(string $token): bool
    {
        $sessionToken = Session::get(self::SESSION_KEY);

        return is_string($sessionToken)
            && $sessionToken !== ''
            && $token !== ''
            && hash_equals($sessionToken, $token);
    }
}