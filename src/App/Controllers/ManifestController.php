<?php

namespace Keel\App\Controllers;

use Keel\Core\Controller;
use Keel\Core\Env;
use Keel\Core\Request;
use Keel\Core\Response;

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
            // Icons that ship in public_html (the old /assets/brand/... paths did not exist).
            'icons' => [
                [
                    'src' => '/apple-touch-icon.png',
                    'sizes' => '180x180',
                    'type' => 'image/png',
                ],
                [
                    'src' => '/images/brand/keel-icon.png',
                    'sizes' => '1024x1024',
                    'type' => 'image/png',
                ],
            ],
        ];

        Response::raw(
            (string) json_encode($manifest, JSON_UNESCAPED_SLASHES | JSON_UNESCAPED_UNICODE),
            200,
            ['Content-Type' => 'application/manifest+json; charset=UTF-8']
        );
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
