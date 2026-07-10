<?php
$pageTitle = $pageTitle ?? 'Documentation';
$contentFile = $contentFile ?? 'index';
$currentPath = $currentPath ?? '/docs';
$navGroups = $navGroups ?? [];
$repoHref = $repoHref ?? 'https://github.com';
$hasSeedScript = $hasSeedScript ?? false;
$hasDockerCompose = $hasDockerCompose ?? false;
?>
<!DOCTYPE html>
<html lang="en"<?= \Keel\Core\Theme::htmlAttribute() ?>>
<head>
<?php require __DIR__ . '/../partials/head.php'; ?>
<title><?= htmlspecialchars(($title ?? 'Docs') . ' - Keel') ?></title>
<style>
    :root {
        --keel-night: #07111f;
        --keel-line: rgba(148, 163, 184, 0.18);
    }

    [data-theme="light"] {
        --keel-night: #f8fafc;
        --keel-line: rgba(15, 23, 42, 0.14);
    }

    body {
        background:
            radial-gradient(circle at top left, rgba(105, 230, 216, 0.14), transparent 28%),
            radial-gradient(circle at top right, rgba(255, 107, 61, 0.15), transparent 32%),
            linear-gradient(180deg, #091221 0%, #07111f 42%, #0c1730 100%);
        color: var(--color-text);
    }

    [data-theme="light"] body {
        background:
            radial-gradient(circle at top left, rgba(105, 230, 216, 0.12), transparent 30%),
            radial-gradient(circle at top right, rgba(255, 107, 61, 0.12), transparent 35%),
            linear-gradient(180deg, #f8fafc 0%, #eef2f7 100%);
    }

    .docs-nav {
        background: linear-gradient(180deg, rgba(11, 24, 48, 0.96), rgba(7, 17, 31, 0.92));
        border: 1px solid var(--keel-line);
        box-shadow: 0 22px 80px rgba(0, 0, 0, 0.34);
    }

    [data-theme="light"] .docs-nav {
        background: linear-gradient(180deg, rgba(255, 255, 255, 0.98), rgba(248, 250, 252, 0.96));
        box-shadow: 0 18px 40px rgba(15, 23, 42, 0.12);
    }

    .docs-panel {
        backdrop-filter: blur(12px);
        border-color: var(--keel-line);
    }
</style>
</head>
<body class="min-h-screen antialiased">
    <div class="mx-auto max-w-7xl px-4 pb-6 pt-8 sm:px-6 sm:pt-10 lg:px-8 lg:pt-12">
        <header class="docs-nav mb-6 rounded-3xl p-4 sm:p-5">
            <div class="flex flex-wrap items-center justify-between gap-3">
                <a href="/" class="inline-flex items-center gap-3 rounded-full">
                    <img src="/images/brand/keel-light.png" alt="Keel logo" class="logo-dark-mode h-10 object-contain" loading="eager" decoding="async">
                    <img src="/images/brand/keel.png" alt="Keel logo" class="logo-light-mode h-10 object-contain" loading="eager" decoding="async">
                </a>
                <div class="flex flex-wrap items-center gap-2">
                    <button type="button" class="theme-toggle-button" data-theme-toggle>
                        <span data-theme-toggle-icon aria-hidden="true"></span>
                    </button>
                    <a href="/docs" class="btn btn-secondary btn-sm">Docs</a>
                    <a href="<?= htmlspecialchars($repoHref) ?>" class="btn btn-secondary btn-sm" target="_blank" rel="noreferrer">GitHub</a>
                    <a href="/login" class="btn btn-primary btn-sm">Sign in</a>
                </div>
            </div>
        </header>

        <div class="mb-4 flex items-center justify-between gap-3 lg:hidden">
            <h1 class="text-xl font-semibold text-[var(--color-text-strong)]"><?= htmlspecialchars($pageTitle) ?></h1>
            <button type="button" id="docs-sidebar-toggle" class="btn btn-secondary btn-sm" aria-expanded="false" aria-controls="docs-sidebar">Sections</button>
        </div>

        <div class="grid gap-5 lg:grid-cols-[280px_minmax(0,1fr)]">
            <aside id="docs-sidebar" class="hidden lg:block">
                <div class="card docs-panel sticky top-6 rounded-2xl p-4">
                    <p class="mb-3 text-xs font-semibold uppercase tracking-wide text-[var(--color-text-muted)]">Documentation</p>
                    <nav aria-label="Docs navigation" class="space-y-4">
                        <?php foreach ($navGroups as $group): ?>
                        <section>
                            <h2 class="mb-2 text-xs font-semibold uppercase tracking-wide text-[var(--color-text-muted)]"><?= htmlspecialchars((string) ($group['title'] ?? 'Section')) ?></h2>
                            <ul class="space-y-1.5">
                                <?php foreach (($group['links'] ?? []) as $link): ?>
                                <?php
                                $href = (string) ($link['href'] ?? '/docs');
                                $isActive = $href === $currentPath;
                                ?>
                                <li>
                                    <a href="<?= htmlspecialchars($href) ?>" class="block rounded-lg px-3 py-2 text-sm <?= $isActive ? 'bg-[var(--color-brand)] text-[var(--color-brand-contrast)]' : 'text-[var(--color-text-muted)] hover:bg-[var(--color-surface-muted)] hover:text-[var(--color-text-strong)]' ?>">
                                        <?= htmlspecialchars((string) ($link['label'] ?? 'Untitled')) ?>
                                    </a>
                                </li>
                                <?php endforeach; ?>
                            </ul>
                        </section>
                        <?php endforeach; ?>
                    </nav>
                </div>
            </aside>

            <main class="space-y-5">
                <article class="card docs-panel rounded-3xl p-5 sm:p-7">
                    <h1 class="mb-3 text-3xl font-semibold tracking-tight text-[var(--color-text-strong)]"><?= htmlspecialchars($pageTitle) ?></h1>
                    <?php require __DIR__ . '/' . $contentFile . '.php'; ?>
                </article>
            </main>
        </div>
    </div>

    <script>
        const sidebarToggle = document.getElementById('docs-sidebar-toggle');
        const sidebar = document.getElementById('docs-sidebar');

        if (sidebarToggle && sidebar) {
            sidebarToggle.addEventListener('click', () => {
                const expanded = sidebarToggle.getAttribute('aria-expanded') === 'true';
                sidebarToggle.setAttribute('aria-expanded', expanded ? 'false' : 'true');
                sidebar.classList.toggle('hidden', expanded);
            });
        }
    </script>
</body>
</html>
