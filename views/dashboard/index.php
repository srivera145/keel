<?php
use EchoDial\Deck\Deck;
use Keel\Core\Csrf;
use Keel\Core\Theme;
?>
<!DOCTYPE html>
<html <?= Deck::htmlAttributes(lang: 'en') ?> <?= Deck::theme(mode: Theme::serverPreference()) ?>>
<head>
<?php require __DIR__ . '/../partials/head.php'; ?>
</head>
<body>
    <span id="top" tabindex="-1"></span>

    <main class="container settings-page stack stack-6" style="--container: 48rem">
        <header class="bar">
            <h1 class="h2">Dashboard</h1>
            <?php $themeToggleClass = 'push'; require __DIR__ . '/../partials/theme-toggle.php'; ?>
            <form method="POST" action="/logout">
                <?= Csrf::field() ?>
                <button type="submit" class="btn btn-sm">Sign out</button>
            </form>
        </header>

        <section class="card">
            <div class="card-body">
                <div class="stat">
                    <dt class="stat-label">Signed in as</dt>
                    <dd class="fw-semi"><?= htmlspecialchars($user['email'] ?? '') ?></dd>
                </div>
            </div>
        </section>
    </main>
</body>
</html>
