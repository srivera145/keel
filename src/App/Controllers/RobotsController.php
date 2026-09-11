<?php

namespace Keel\App\Controllers;

use Keel\Core\Controller;
use Keel\Core\Env;
use Keel\Core\Request;
use Keel\Core\Response;
use Keel\Core\Router;

class RobotsController extends Controller
{
    private const PROTECTED_MIDDLEWARE = [
        \Keel\App\Middleware\AuthMiddleware::class,
        \Keel\App\Middleware\RequireOrganizationMiddleware::class,
        \Keel\App\Middleware\RequireOrgAdminMiddleware::class,
        \Keel\App\Middleware\RequireSuperAdminMiddleware::class,
        \Keel\App\Middleware\ApiAuthMiddleware::class,
    ];

    public function index(Request $request): never
    {
        $disallowPrefixes = $this->disallowPrefixes();
        $baseUrl = $this->baseUrl();

        $lines = [
            'User-agent: *',
            'Allow: /',
        ];

        foreach ($disallowPrefixes as $prefix) {
            $lines[] = 'Disallow: ' . $prefix;
        }

        $lines[] = '';
        $lines[] = 'Sitemap: ' . $baseUrl . '/sitemap.xml';

        Response::raw(implode("\n", $lines) . "\n", 200, ['Content-Type' => 'text/plain; charset=UTF-8']);
    }

    private function disallowPrefixes(): array
    {
        $router = Router::current();
        $prefixes = [];

        foreach (($router?->registeredRoutes() ?? []) as $route) {
            $middleware = (array) ($route['middleware'] ?? []);
            $isProtected = count(array_intersect($middleware, self::PROTECTED_MIDDLEWARE)) > 0;

            if (!$isProtected) {
                continue;
            }

            $prefix = $this->prefixForUri((string) ($route['uri'] ?? '/'));
            if ($prefix === '/') {
                continue;
            }

            $prefixes[$prefix] = true;
        }

        $prefixes = array_keys($prefixes);
        sort($prefixes, SORT_STRING);

        return $prefixes;
    }

    private function prefixForUri(string $uri): string
    {
        $segments = array_values(array_filter(explode('/', trim($uri, '/')), static fn (string $segment): bool => $segment !== ''));

        if ($segments === []) {
            return '/';
        }

        if (count($segments) === 1) {
            return $segments[0] === 'dashboard'
                ? '/dashboard'
                : '/' . $segments[0] . '/';
        }

        return '/' . $segments[0] . '/';
    }

    private function baseUrl(): string
    {
        $baseUrl = trim((string) Env::get('APP_URL', ''));

        return $baseUrl !== '' ? rtrim($baseUrl, '/') : 'http://localhost';
    }
}
