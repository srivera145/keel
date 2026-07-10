<?php

namespace Keel\Core;

class View
{
    private static string $viewsPath;

    public static function setPath(string $path): void
    {
        self::$viewsPath = rtrim($path, '/');
    }

    public static function render(string $template, array $data = []): void
    {
        extract($data, EXTR_SKIP);
        $file = self::$viewsPath . '/' . str_replace('.', '/', $template) . '.php';

        if (!file_exists($file)) {
            throw new \RuntimeException("View not found: {$template}");
        }

        require $file;
    }
}
