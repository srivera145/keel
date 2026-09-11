<?php

namespace Keel\App\Controllers;

use Keel\Core\Controller;
use Keel\Core\Env;
use Keel\Core\Request;
use Keel\Core\Response;

class LlmsTxtController extends Controller
{
    public function index(Request $request): never
    {
        $baseUrl = $this->baseUrl();
        $repoUrl = trim((string) Env::get('APP_REPO_URL', ''));

        $lines = [
            '# Keel',
            '',
            "> Open-source PHP 8.2 starter kit for SaaS applications with custom MVC, passwordless auth, billing, and docs. Not affiliated with other projects named Keel.",
            '',
            '## Docs',
            '',
        ];

        foreach (DocsController::docsPages() as $page) {
            $slug = trim((string) ($page['slug'] ?? ''));
            $title = trim((string) ($page['title'] ?? ''));

            if ($slug === '' || $title === '') {
                continue;
            }

            $lines[] = '- [' . $title . '](' . $baseUrl . '/docs/' . $slug . ')';
        }

        $lines[] = '';
        $lines[] = '## Project';
        $lines[] = '';

        if ($repoUrl !== '') {
            $lines[] = '- [GitHub repository](' . $repoUrl . ')';
        }

        Response::raw(implode("\n", $lines) . "\n", 200, ['Content-Type' => 'text/plain; charset=UTF-8']);
    }

    private function baseUrl(): string
    {
        $baseUrl = trim((string) Env::get('APP_URL', ''));

        return $baseUrl !== '' ? rtrim($baseUrl, '/') : 'http://localhost';
    }
}
