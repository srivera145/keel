<?php

namespace Keel\App\Controllers;

use Keel\Core\Controller;
use Keel\Core\Env;
use Keel\Core\Request;

class ManifestController extends Controller
{
    public function index(Request $request): never
    {
        $appName = trim((string) Env::get('APP_NAME', 'Keel'));
        if ($appName === '') {
            $appName = 'Keel';
        }

        $manifest = [
            'name' => $appName,
            'short_name' => $this->shortName($appName),
            'start_url' => '/',
            'display' => 'standalone',
            'background_color' => '#07111f',
            'theme_color' => '#07111f',
            'description' => $appName . ' - built on Keel',
            'icons' => [
                [
                    'src' => '/assets/brand/icon-192.png',
                    'sizes' => '192x192',
                    'type' => 'image/png',
                ],
                [
                    'src' => '/assets/brand/icon-512.png',
                    'sizes' => '512x512',
                    'type' => 'image/png',
                ],
                [
                    'src' => '/assets/brand/icon-maskable-512.png',
                    'sizes' => '512x512',
                    'type' => 'image/png',
                    'purpose' => 'maskable',
                ],
            ],
        ];

        header('Content-Type: application/manifest+json; charset=UTF-8');
        echo json_encode($manifest, JSON_UNESCAPED_SLASHES | JSON_UNESCAPED_UNICODE);
        exit;
    }

    private function shortName(string $name): string
    {
        $maxLength = 12;

        if (function_exists('mb_strlen') && function_exists('mb_substr')) {
            return mb_strlen($name) > $maxLength ? mb_substr($name, 0, $maxLength) : $name;
        }

        return strlen($name) > $maxLength ? substr($name, 0, $maxLength) : $name;
    }
}
