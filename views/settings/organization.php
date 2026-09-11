<?php
use EchoDial\Deck\Deck;
use Keel\Core\Theme;

$brandHue = isset($organization['brand_hue']) ? (int) $organization['brand_hue'] : null;
?>
<!DOCTYPE html>
<html <?= Deck::htmlAttributes(lang: 'en') ?> <?= Deck::theme(hue: $brandHue, mode: Theme::serverPreference()) ?>>
<head>
<?php require __DIR__ . '/../partials/head.php'; ?>
</head>
<body>
    <span id="top" tabindex="-1"></span>

    <main class="container settings-page stack stack-6">
        <header class="bar">
            <div class="stack stack-2">
                <nav aria-label="Breadcrumb">
                    <ol class="breadcrumb">
                        <li><a href="/dashboard">Dashboard</a></li>
                        <li><span aria-current="page">Organization settings</span></li>
                    </ol>
                </nav>
                <h1 class="h2"><?= htmlspecialchars((string) ($organization['name'] ?? 'Organization')) ?></h1>
            </div>
            <?php $themeToggleClass = 'push'; require __DIR__ . '/../partials/theme-toggle.php'; ?>
        </header>

        <section class="card">
            <div class="card-body">
                <dl class="grid" style="--min: 14rem">
                    <div class="stat">
                        <dt class="stat-label">Organization name</dt>
                        <dd class="fw-semi"><?= htmlspecialchars((string) ($organization['name'] ?? '')) ?></dd>
                    </div>
                    <div class="stat">
                        <dt class="stat-label">Slug</dt>
                        <dd class="fw-semi mono"><?= htmlspecialchars((string) ($organization['slug'] ?? '')) ?></dd>
                    </div>
                    <div class="stat">
                        <dt class="stat-label">Your role</dt>
                        <dd><span class="badge badge-brand"><?= htmlspecialchars((string) ($user['role'] ?? '')) ?></span></dd>
                    </div>
                    <div class="stat">
                        <dt class="stat-label">Members</dt>
                        <dd><a href="/settings/members">Manage members</a></dd>
                    </div>
                </dl>
            </div>
        </section>
    </main>

    <?php require __DIR__ . '/../partials/back-to-top.php'; ?>
</body>
</html>
