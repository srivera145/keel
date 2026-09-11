<?php

namespace Keel\App\Controllers;

use Keel\Core\Controller;
use Keel\Core\ErrorHandler;
use Keel\Core\Request;

class DocsController extends Controller
{
    public static function docsPages(): array
    {
        return [
            ['slug' => 'installation', 'title' => 'Installation', 'contentFile' => 'installation', 'group' => 'Getting Started'],
            ['slug' => 'project-structure', 'title' => 'Project Structure', 'contentFile' => 'project-structure', 'group' => 'Getting Started'],
            ['slug' => 'where-to-start-editing', 'title' => 'Where To Start Editing', 'contentFile' => 'where-to-start-editing', 'group' => 'Getting Started'],
            ['slug' => 'authentication', 'title' => 'Authentication', 'contentFile' => 'authentication', 'group' => 'Features'],
            ['slug' => 'mailing', 'title' => 'Mailing', 'contentFile' => 'mailing', 'group' => 'Features'],
            ['slug' => 'security', 'title' => 'Security', 'contentFile' => 'security', 'group' => 'Features'],
            ['slug' => 'billing', 'title' => 'Billing', 'contentFile' => 'billing', 'group' => 'Features'],
            ['slug' => 'multi-tenancy', 'title' => 'Multi-Tenancy', 'contentFile' => 'multi-tenancy', 'group' => 'Features'],
            ['slug' => 'background-jobs', 'title' => 'Background Jobs', 'contentFile' => 'background-jobs', 'group' => 'Features'],
            ['slug' => 'theming', 'title' => 'Theming', 'contentFile' => 'theming', 'group' => 'Features'],
            ['slug' => 'api-tokens', 'title' => 'API Tokens', 'contentFile' => 'api-tokens', 'group' => 'Features'],
            ['slug' => 'seo-discoverability', 'title' => 'SEO & Discoverability', 'contentFile' => 'seo-discoverability', 'group' => 'Features'],
        ];
    }

    public static function docsPagesBySlug(): array
    {
        $pagesBySlug = [];

        foreach (self::docsPages() as $page) {
            $slug = (string) ($page['slug'] ?? '');
            if ($slug === '') {
                continue;
            }

            $pagesBySlug[$slug] = $page;
        }

        return $pagesBySlug;
    }

    public static function docsNavGroups(bool $includeOverview = true): array
    {
        $groups = [];

        foreach (self::docsPages() as $page) {
            $groupName = (string) ($page['group'] ?? 'Docs');

            if (!isset($groups[$groupName])) {
                $groups[$groupName] = [
                    'title' => $groupName,
                    'links' => [],
                ];
            }

            $groups[$groupName]['links'][] = [
                'href' => '/docs/' . (string) $page['slug'],
                'label' => (string) $page['title'],
            ];
        }

        if ($includeOverview && isset($groups['Getting Started'])) {
            array_unshift($groups['Getting Started']['links'], ['href' => '/docs', 'label' => 'Overview']);
        }

        return array_values($groups);
    }

    public function index(Request $request): void
    {
        $this->view('docs._layout', [
            'title' => 'Docs',
            'pageTitle' => 'Documentation',
            'contentFile' => 'index',
            'currentPath' => '/docs',
            'navGroups' => self::docsNavGroups(),
            'docsGroups' => self::docsNavGroups(false),
            'repoHref' => $this->repoHref(),
            'hasSeedScript' => $this->hasSeedScript(),
            'hasDockerCompose' => $this->hasDockerCompose(),
        ]);
    }

    public function show(Request $request, string $slug): void
    {
        $pages = self::docsPagesBySlug();

        if (!isset($pages[$slug])) {
            ErrorHandler::render(404);
        }

        $page = $pages[$slug];

        $this->view('docs._layout', [
            'title' => 'Docs: ' . $page['title'],
            'pageTitle' => $page['title'],
            'contentFile' => $page['contentFile'],
            'currentPath' => '/docs/' . $slug,
            'navGroups' => self::docsNavGroups(),
            'repoHref' => $this->repoHref(),
            'hasSeedScript' => $this->hasSeedScript(),
            'hasDockerCompose' => $this->hasDockerCompose(),
        ]);
    }

    private function repoHref(): string
    {
        $repo = trim((string) \Keel\Core\Env::get('APP_REPO_URL', ''));

        return $repo !== '' ? $repo : 'https://github.com';
    }

    private function hasSeedScript(): bool
    {
        return file_exists(dirname(__DIR__, 3) . '/database/seed.php');
    }

    private function hasDockerCompose(): bool
    {
        return file_exists(dirname(__DIR__, 3) . '/docker-compose.yml');
    }
}
