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
                <span class="empty-art"><?= Deck::icon('check-circle', 'icon icon-lg') ?></span>
                <span class="badge badge-good">Stripe Checkout</span>
                <h1 class="empty-title">Checkout complete</h1>
                <p>Your subscription is confirmed by Stripe webhook events, not by this redirect alone. If your account does not reflect the change immediately, refresh in a moment and Stripe will catch up.</p>

                <div class="cluster cluster-center">
                    <a href="/billing/upgrade" class="btn btn-primary">Back to billing</a>
                    <a href="/dashboard" class="btn">Go to dashboard</a>
                </div>
            </div>
        </section>
    </main>
</body>
</html>
