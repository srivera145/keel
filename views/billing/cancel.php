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
                <span class="empty-art"><?= Deck::icon('x-circle', 'icon icon-lg') ?></span>
                <span class="badge">Stripe Checkout</span>
                <h1 class="empty-title">Checkout cancelled</h1>
                <p>No subscription changes were made locally. You can go back to billing whenever you want to try again.</p>

                <a href="/billing/upgrade" class="btn btn-primary">Return to billing</a>
            </div>
        </section>
    </main>
</body>
</html>
