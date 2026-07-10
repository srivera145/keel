<?php

namespace Keel\App\Controllers;

use Keel\Core\Controller;
use Keel\Core\Env;
use Keel\Core\Request;
use Keel\Core\Response;
use Keel\Core\Router;

class SitemapController extends Controller
{
    public function index(Request $request): never
    {
        $baseUrl = $this->baseUrl();
        $paths = $this->publicPaths();

        $xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        $xml .= "<urlset xmlns=\"http://www.sitemaps.org/schemas/sitemap/0.9\">\n";

        foreach ($paths as $path) {
            $xml .= "  <url>\n";
            $xml .= '    <loc>' . htmlspecialchars($this->absoluteUrl($baseUrl, $path), ENT_QUOTES | ENT_XML1, 'UTF-8') . "</loc>\n";

            $lastModified = $this->lastModifiedForPath($path);
            if ($lastModified !== null) {
                $xml .= '    <lastmod>' . htmlspecialchars($lastModified, ENT_QUOTES | ENT_XML1, 'UTF-8') . "</lastmod>\n";
            }

            $xml .= "  </url>\n";
        }

        $xml .= "</urlset>\n";

        Response::raw($xml, 200, ['Content-Type' => 'application/xml; charset=UTF-8']);
    }

    private function publicPaths(): array
    {
        $paths = [];
        $router = Router::current();

        foreach (($router?->publicPages() ?? []) as $page) {
            $uri = (string) ($page['uri'] ?? '');

            if ($uri === '' || str_contains($uri, '{')) {
                continue;
            }

            $paths[$uri] = true;
        }

        foreach (DocsController::docsPages() as $docPage) {
            $slug = trim((string) ($docPage['slug'] ?? ''));
            if ($slug === '') {
                continue;
            }

            $paths['/docs/' . $slug] = true;
        }

        $paths = array_keys($paths);
        sort($paths, SORT_STRING);

        return $paths;
    }

    private function lastModifiedForPath(string $path): ?string
    {
        $basePath = dirname(__DIR__, 3);
        $file = null;

        if ($path === '/') {
            $file = $basePath . '/views/welcome.php';
        } elseif ($path === '/docs') {
            $file = $basePath . '/views/docs/index.php';
        } elseif (str_starts_with($path, '/docs/')) {
            $slug = substr($path, strlen('/docs/'));
            $file = $basePath . '/views/docs/' . $slug . '.php';
        } elseif ($path === '/login') {
            $file = $basePath . '/views/auth/login.php';
        }

        if ($file === null || !is_file($file)) {
            return null;
        }

        $timestamp = filemtime($file);
        if ($timestamp === false) {
            return null;
        }

        return gmdate('Y-m-d\TH:i:s\Z', $timestamp);
    }

    private function baseUrl(): string
    {
        $baseUrl = trim((string) Env::get('APP_URL', ''));

        return $baseUrl !== '' ? rtrim($baseUrl, '/') : 'http://localhost';
    }

    private function absoluteUrl(string $baseUrl, string $path): string
    {
        if ($path === '/') {
            return $baseUrl . '/';
        }

        return $baseUrl . '/' . ltrim($path, '/');
    }
}
