<?php

namespace Keel\Core;

/**
 * Reads Vite's manifest.json to output <script>/<link> tags for built assets.
 * Falls back to the dev server automatically when public_html/hot exists
 * (created by `npm run dev`, removed when the dev server stops).
 */
class Vite
{
    public static function assets(array $entries): string
    {
        $basePath = dirname(__DIR__, 2);
        $hotFile = $basePath . '/public_html/hot';

        if (file_exists($hotFile)) {
            $devServerUrl = rtrim(trim(file_get_contents($hotFile)), '/');
            $tags = '<script type="module" src="' . $devServerUrl . '/@vite/client"></script>' . "\n";
            foreach ($entries as $entry) {
                $tags .= '<script type="module" src="' . $devServerUrl . '/' . $entry . '"></script>' . "\n";
            }
            return $tags;
        }

        $manifestPath = $basePath . '/public_html/assets/.vite/manifest.json';

        if (!file_exists($manifestPath)) {
            return '<!-- Vite manifest not found. Run `npm run build`. -->';
        }

        $manifest = json_decode(file_get_contents($manifestPath), true);
        $tags = '';

        foreach ($entries as $entry) {
            if (!isset($manifest[$entry])) {
                continue;
            }

            $chunk = $manifest[$entry];
            $tags .= '<script type="module" src="/assets/' . $chunk['file'] . '"></script>' . "\n";

            foreach ($chunk['css'] ?? [] as $cssFile) {
                $tags .= '<link rel="stylesheet" href="/assets/' . $cssFile . '">' . "\n";
            }
        }

        return $tags;
    }
}
