<?php

namespace Keel\Core;

class Theme
{
    public const DARK = 'dark';
    public const LIGHT = 'light';

    public static function normalize(mixed $value): ?string
    {
        if (!is_string($value)) {
            return null;
        }

        $normalized = strtolower(trim($value));

        if ($normalized === self::DARK || $normalized === self::LIGHT) {
            return $normalized;
        }

        return null;
    }

    public static function serverPreference(): ?string
    {
        return self::normalize(Session::get('theme_preference'));
    }

    public static function htmlAttribute(): string
    {
        $theme = self::serverPreference();

        if ($theme === null) {
            return '';
        }

        return ' data-theme="' . htmlspecialchars($theme, ENT_QUOTES, 'UTF-8') . '"';
    }

    public static function isAuthenticated(): bool
    {
        return Session::has('user_id');
    }
}
