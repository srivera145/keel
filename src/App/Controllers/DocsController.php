<?php

namespace Keel\App\Controllers;

use Keel\Core\Controller;
use Keel\Core\ErrorHandler;
use Keel\Core\Request;

class DocsController extends Controller
{
    public function index(Request $request): void
    {
        $this->view('docs._layout', [
            'title' => 'Docs',
            'pageTitle' => 'Documentation',
            'contentFile' => 'index',
            'currentPath' => '/docs',
            'navGroups' => $this->navGroups(),
            'repoHref' => $this->repoHref(),
            'hasSeedScript' => $this->hasSeedScript(),
            'hasDockerCompose' => $this->hasDockerCompose(),
        ]);
    }

    public function show(Request $request, string $slug): void
    {
        $pages = $this->pages();

        if (!isset($pages[$slug])) {
            ErrorHandler::render(404);
        }

        $page = $pages[$slug];

        $this->view('docs._layout', [
            'title' => 'Docs: ' . $page['title'],
            'pageTitle' => $page['title'],
            'contentFile' => $page['contentFile'],
            'currentPath' => '/docs/' . $slug,
            'navGroups' => $this->navGroups(),
            'repoHref' => $this->repoHref(),
            'hasSeedScript' => $this->hasSeedScript(),
            'hasDockerCompose' => $this->hasDockerCompose(),
        ]);
    }

    private function pages(): array
    {
        return [
            'installation' => ['title' => 'Installation', 'contentFile' => 'installation'],
            'project-structure' => ['title' => 'Project Structure', 'contentFile' => 'project-structure'],
            'where-to-start-editing' => ['title' => 'Where To Start Editing', 'contentFile' => 'where-to-start-editing'],
            'authentication' => ['title' => 'Authentication', 'contentFile' => 'authentication'],
            'mailing' => ['title' => 'Mailing', 'contentFile' => 'mailing'],
            'security' => ['title' => 'Security', 'contentFile' => 'security'],
            'billing' => ['title' => 'Billing', 'contentFile' => 'billing'],
            'multi-tenancy' => ['title' => 'Multi-Tenancy', 'contentFile' => 'multi-tenancy'],
            'background-jobs' => ['title' => 'Background Jobs', 'contentFile' => 'background-jobs'],
            'theming' => ['title' => 'Theming', 'contentFile' => 'theming'],
            'api-tokens' => ['title' => 'API Tokens', 'contentFile' => 'api-tokens'],
        ];
    }

    private function navGroups(): array
    {
        return [
            [
                'title' => 'Getting Started',
                'links' => [
                    ['href' => '/docs', 'label' => 'Overview'],
                    ['href' => '/docs/installation', 'label' => 'Installation'],
                    ['href' => '/docs/project-structure', 'label' => 'Project Structure'],
                    ['href' => '/docs/where-to-start-editing', 'label' => 'Where To Start Editing'],
                ],
            ],
            [
                'title' => 'Features',
                'links' => [
                    ['href' => '/docs/authentication', 'label' => 'Authentication'],
                    ['href' => '/docs/mailing', 'label' => 'Mailing'],
                    ['href' => '/docs/security', 'label' => 'Security'],
                    ['href' => '/docs/billing', 'label' => 'Billing'],
                    ['href' => '/docs/multi-tenancy', 'label' => 'Multi-Tenancy'],
                    ['href' => '/docs/background-jobs', 'label' => 'Background Jobs'],
                    ['href' => '/docs/theming', 'label' => 'Theming'],
                    ['href' => '/docs/api-tokens', 'label' => 'API Tokens'],
                ],
            ],
        ];
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
