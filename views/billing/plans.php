<?php
use EchoDial\Deck\Deck;
use Keel\Core\Csrf;
use Keel\Core\Theme;

$plans = $plans ?? [];
$currentSubscription = $currentSubscription ?? null;
$publishableKey = $publishableKey ?? '';

// Stripe's status vocabulary, mapped to Deck's badge tones.
$statusTone = [
    'active' => 'badge-good',
    'trialing' => 'badge-good',
    'past_due' => 'badge-warn',
    'unpaid' => 'badge-warn',
    'incomplete' => 'badge-warn',
    'canceled' => 'badge-bad',
];
?>
<!DOCTYPE html>
<html <?= Deck::htmlAttributes(lang: 'en') ?> <?= Deck::theme(mode: Theme::serverPreference()) ?>>
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
                        <li><span aria-current="page">Billing</span></li>
                    </ol>
                </nav>
                <h1 class="h2">Upgrade to Pro</h1>
            </div>
            <?php $themeToggleClass = 'push'; require __DIR__ . '/../partials/theme-toggle.php'; ?>
        </header>

        <?php if (!empty($_GET['error']) && $_GET['error'] === 'invalid_plan'): ?>
        <div class="alert alert-bad">
            <?= Deck::icon('alert-circle') ?>
            <p>That billing plan is not available.</p>
        </div>
        <?php endif; ?>

        <?php if ($currentSubscription): ?>
        <section class="card" aria-labelledby="current-title">
            <div class="card-header">
                <h2 id="current-title" class="card-title">Current subscription</h2>
            </div>
            <div class="card-body">
                <div class="bar wrap">
                    <div class="stack stack-1">
                        <p class="fw-semi"><?= htmlspecialchars(ucwords(str_replace('_', ' ', (string) $currentSubscription['plan']))) ?></p>
                        <p class="text-sm text-muted">
                            Status
                            <span class="badge <?= $statusTone[(string) $currentSubscription['status']] ?? '' ?>"><?= htmlspecialchars((string) $currentSubscription['status']) ?></span>
                        </p>
                    </div>
                    <form method="POST" action="/billing/portal" class="push">
                        <?= Csrf::field() ?>
                        <button type="submit" class="btn">Manage in Stripe</button>
                    </form>
                </div>
            </div>
        </section>
        <?php endif; ?>

        <div class="grid grid-wide">
            <section class="card" aria-labelledby="starter-title">
                <div class="card-body">
                    <p class="text-sm uppercase text-muted">Starter</p>
                    <h2 id="starter-title" class="h4">Free while you build</h2>
                    <p class="text-sm text-muted">Use Keel locally, wire your app, and move to Stripe-hosted checkout when you are ready to turn billing on.</p>
                    <ul class="stack stack-2 text-sm text-muted">
                        <li>OTP + magic link auth</li>
                        <li>Custom MVC and PDO</li>
                        <li>Deck CSS, no build step</li>
                    </ul>
                </div>
                <div class="card-footer">
                    <p class="text-sm text-muted">No checkout required for the starter kit itself.</p>
                </div>
            </section>

            <section class="card" aria-labelledby="pro-title">
                <div class="card-body">
                    <p class="text-sm uppercase text-brand">Pro</p>
                    <h2 id="pro-title" class="h4">Monthly subscription</h2>
                    <p class="text-sm text-muted">Hosted on Stripe Checkout. Keel stores the subscription state locally and leaves card collection entirely to Stripe.</p>
                    <ul class="stack stack-2 text-sm text-muted">
                        <li>Stripe Checkout for signup</li>
                        <li>Stripe Billing Portal for self-serve management</li>
                        <li>Webhook-driven subscription sync</li>
                    </ul>
                    <?php if ($publishableKey === ''): ?>
                    <div class="alert alert-warn">
                        <?= Deck::icon('alert-triangle') ?>
                        <p class="alert-body">Stripe keys are not configured yet. Add them in <code>.env</code> before testing checkout.</p>
                    </div>
                    <?php endif; ?>
                </div>
                <div class="card-footer">
                    <form method="POST" action="/billing/checkout" class="w-full">
                        <?= Csrf::field() ?>
                        <input type="hidden" name="plan" value="pro_monthly">
                        <button type="submit" class="btn btn-primary btn-block" <?= empty($plans['pro_monthly']) ? 'disabled' : '' ?>>
                            <?= empty($plans['pro_monthly']) ? 'Set STRIPE_PRICE_PRO_MONTHLY in .env' : 'Subscribe with Stripe' ?>
                        </button>
                    </form>
                </div>
            </section>
        </div>
    </main>

    <?php require __DIR__ . '/../partials/back-to-top.php'; ?>
</body>
</html>
