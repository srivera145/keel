<?php

use EchoDial\Deck\Deck;
use Keel\Core\Theme;

$pageTitle = $pageTitle ?? 'Documentation';
$contentFile = $contentFile ?? 'index';
$currentPath = $currentPath ?? '/docs';
$navGroups = $navGroups ?? [];
$repoHref = $repoHref ?? 'https://github.com';
$hasSeedScript = $hasSeedScript ?? false;
$hasDockerCompose = $hasDockerCompose ?? false;

// head.php owns the <title>; the previous layout emitted a second one after it.
$title = $pageTitle . ' - Keel';
$navCurrent = 'docs';
?>
<!DOCTYPE html>
<html <?= Deck::htmlAttributes(lang: 'en') ?> <?= Deck::theme(mode: Theme::serverPreference()) ?>>
<head>
<?php require __DIR__ . '/../partials/head.php'; ?>
</head>
<body class="g-mesh">
    <span id="top" tabindex="-1"></span>
    <a class="skip-link" href="#main-content">Skip to content</a>

    <?php require __DIR__ . '/../partials/site-nav.php'; ?>

    <main id="main-content" class="container section">
        <div class="split split-rail-start" style="--rail: 16rem">
            <nav class="docs-sections card" aria-label="Documentation">
                <div class="sidebar">
                    <?php foreach ($navGroups as $group): ?>
                    <span class="sidebar-group"><?= htmlspecialchars((string) ($group['title'] ?? 'Section')) ?></span>
                    <?php foreach (($group['links'] ?? []) as $link): ?>
                    <?php $href = (string) ($link['href'] ?? '/docs'); ?>
                    <a class="sidebar-link" href="<?= htmlspecialchars($href) ?>"<?= $href === $currentPath ? ' aria-current="page"' : '' ?>>
                        <?= htmlspecialchars((string) ($link['label'] ?? 'Untitled')) ?>
                    </a>
                    <?php endforeach; ?>
                    <?php endforeach; ?>
                </div>
            </nav>

            <article class="card">
                <div class="card-body stack stack-4">
                    <h1 class="h2"><?= htmlspecialchars($pageTitle) ?></h1>
                    <?php require __DIR__ . '/' . $contentFile . '.php'; ?>
                </div>
            </article>
        </div>
    </main>

    <?php require __DIR__ . '/../partials/back-to-top.php'; ?>
</body>
</html>
