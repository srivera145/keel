<?php

declare(strict_types=1);

namespace Tests\Feature;

use Keel\App\Controllers\DocsController;
use Tests\TestCase;

class PublicDiscoveryEndpointsFeatureTest extends TestCase
{
    public function testSitemapIncludesPublicPagesAndExpandedDocsButNotProtectedOrPatternRoutes(): void
    {
        $response = $this->get('/sitemap.xml');

        self::assertSame(200, $response->status);
        self::assertStringContainsString('application/xml', (string) $response->header('Content-Type'));

        $baseUrl = rtrim((string) ($_ENV['APP_URL'] ?? 'http://localhost'), '/');

        self::assertStringContainsString('<loc>' . $baseUrl . '/</loc>', $response->body);
        self::assertStringContainsString('<loc>' . $baseUrl . '/docs</loc>', $response->body);
        self::assertStringContainsString('<loc>' . $baseUrl . '/login</loc>', $response->body);

        foreach (DocsController::docsPages() as $page) {
            $slug = (string) ($page['slug'] ?? '');
            if ($slug === '') {
                continue;
            }

            self::assertStringContainsString('<loc>' . $baseUrl . '/docs/' . $slug . '</loc>', $response->body);
        }

        self::assertStringNotContainsString('/docs/{slug}', $response->body);
        self::assertStringNotContainsString('<loc>' . $baseUrl . '/dashboard</loc>', $response->body);
        self::assertStringNotContainsString('<loc>' . $baseUrl . '/settings/api-tokens</loc>', $response->body);
        self::assertStringNotContainsString('<loc>' . $baseUrl . '/api/v1/files</loc>', $response->body);
    }

    public function testRobotsTxtReflectsProtectedPrefixesFromRegisteredRoutes(): void
    {
        $response = $this->get('/robots.txt');

        self::assertSame(200, $response->status);
        self::assertStringContainsString('text/plain', (string) $response->header('Content-Type'));

        self::assertStringContainsString("User-agent: *\n", $response->body);
        self::assertStringContainsString("Allow: /\n", $response->body);
        self::assertStringContainsString("Disallow: /api/\n", $response->body);
        self::assertStringContainsString("Disallow: /billing/\n", $response->body);
        self::assertStringContainsString("Disallow: /dashboard\n", $response->body);
        self::assertStringContainsString("Disallow: /files/\n", $response->body);
        self::assertStringContainsString("Disallow: /settings/\n", $response->body);

        self::assertStringNotContainsString('Disallow: /docs', $response->body);
        self::assertStringNotContainsString('Disallow: /login', $response->body);

        $baseUrl = rtrim((string) ($_ENV['APP_URL'] ?? 'http://localhost'), '/');
        self::assertStringContainsString('Sitemap: ' . $baseUrl . '/sitemap.xml', $response->body);
    }

    public function testLlmsTxtUsesSharedDocsRegistryLinks(): void
    {
        $response = $this->get('/llms.txt');

        self::assertSame(200, $response->status);
        self::assertStringContainsString('text/plain', (string) $response->header('Content-Type'));
        self::assertStringContainsString('# Keel', $response->body);
        self::assertStringContainsString('## Docs', $response->body);

        $baseUrl = rtrim((string) ($_ENV['APP_URL'] ?? 'http://localhost'), '/');

        foreach (DocsController::docsPages() as $page) {
            $slug = (string) ($page['slug'] ?? '');
            $title = (string) ($page['title'] ?? '');

            if ($slug === '' || $title === '') {
                continue;
            }

            $expected = '- [' . $title . '](' . $baseUrl . '/docs/' . $slug . ')';
            self::assertStringContainsString($expected, $response->body);
        }
    }
}
