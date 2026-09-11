<?php
use EchoDial\Deck\Deck;
use Keel\Core\Theme;
?>
<!DOCTYPE html>
<html <?= Deck::htmlAttributes(lang: 'en') ?> <?= Deck::theme(mode: Theme::serverPreference()) ?>>
<head>
<?php require __DIR__ . '/../partials/head.php'; ?>
</head>
<body>
    <?php $themeToggleClass = 'theme-toggle-floating'; require __DIR__ . '/../partials/theme-toggle.php'; ?>

    <main class="container stage">
        <section class="card">
            <div class="empty">
                <span class="empty-art"><?= Deck::icon('help', 'icon icon-lg') ?></span>
                <span class="badge">404</span>
                <h1 class="empty-title">Page not found</h1>
                <p>The page you requested does not exist or has moved.</p>

                <?php if (!empty($exception)): ?>
                <div class="alert alert-warn text-start w-full">
                    <?= Deck::icon('alert-triangle') ?>
                    <div>
                        <p class="alert-title"><?= htmlspecialchars($exception->getMessage()) ?></p>
                        <p class="alert-body mono"><?= htmlspecialchars($exception->getFile()) ?>:<?= htmlspecialchars((string) $exception->getLine()) ?></p>
                    </div>
                </div>
                <?php endif; ?>

                <a href="/" class="btn btn-primary">Back to home</a>
            </div>
        </section>
    </main>
</body>
</html>
