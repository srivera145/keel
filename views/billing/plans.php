<?php
$plans = $plans ?? [];
$currentSubscription = $currentSubscription ?? null;
$publishableKey = $publishableKey ?? '';
?>
<!DOCTYPE html>
<html lang="en"<?= \Keel\Core\Theme::htmlAttribute() ?>>
<head>
<?php require __DIR__ . '/../partials/head.php'; ?>
</head>
<body class="bg-gray-50 min-h-screen">
    <div class="max-w-5xl mx-auto px-4 py-10">
        <div class="flex items-center justify-between gap-4 mb-8">
            <div>
                <p class="text-sm text-gray-500 mb-2">Billing</p>
                <h1 class="text-3xl font-bold text-gray-900">Upgrade to Pro</h1>
            </div>
            <a href="/dashboard" class="text-sm text-gray-500 hover:text-gray-900">Back to dashboard</a>
        </div>

        <?php if (!empty($_GET['error']) && $_GET['error'] === 'invalid_plan'): ?>
        <div class="alert alert-error mb-6 rounded-xl px-4 py-3">That billing plan is not available.</div>
        <?php endif; ?>

        <?php if ($currentSubscription): ?>
        <div class="card mb-8">
            <p class="text-sm text-gray-500">Current subscription</p>
            <div class="mt-2 flex flex-col gap-3 sm:flex-row sm:items-center sm:justify-between">
                <div>
                    <p class="text-lg font-semibold text-gray-900"><?= htmlspecialchars(ucwords(str_replace('_', ' ', (string) $currentSubscription['plan']))) ?></p>
                    <p class="text-sm text-gray-500">Status: <?= htmlspecialchars((string) $currentSubscription['status']) ?></p>
                </div>
                <form method="POST" action="/billing/portal">
                    <?= \Keel\Core\Csrf::field() ?>
                    <button type="submit" class="btn btn-secondary btn-md">Manage in Stripe</button>
                </form>
            </div>
        </div>
        <?php endif; ?>

        <div class="grid gap-6 md:grid-cols-2">
            <div class="card flex flex-col">
                <p class="text-sm font-medium uppercase tracking-wide text-gray-500">Starter</p>
                <h2 class="mt-3 text-2xl font-bold text-gray-900">Free while you build</h2>
                <p class="mt-3 text-sm leading-6 text-gray-600">Use Keel locally, wire your app, and move to Stripe-hosted checkout when you are ready to turn billing on.</p>
                <ul class="mt-6 space-y-2 text-sm text-gray-600">
                    <li>OTP + magic link auth</li>
                    <li>Custom MVC and PDO</li>
                    <li>Tailwind + Vite asset pipeline</li>
                </ul>
                <div class="mt-8 text-sm text-gray-400">No checkout required for the starter kit itself.</div>
            </div>

            <div class="card flex flex-col">
                <p class="text-sm font-medium uppercase tracking-wide text-gray-500">Pro</p>
                <h2 class="mt-3 text-2xl font-bold text-gray-900">Monthly subscription</h2>
                <p class="mt-3 text-sm leading-6 text-gray-600">Hosted on Stripe Checkout. Keel stores the subscription state locally and leaves card collection entirely to Stripe.</p>
                <ul class="mt-6 space-y-2 text-sm text-gray-600">
                    <li>Stripe Checkout for signup</li>
                    <li>Stripe Billing Portal for self-serve management</li>
                    <li>Webhook-driven subscription sync</li>
                </ul>
                <form method="POST" action="/billing/checkout" class="mt-8">
                    <?= \Keel\Core\Csrf::field() ?>
                    <input type="hidden" name="plan" value="pro_monthly">
                    <button type="submit" class="btn btn-secondary btn-md w-full py-3 disabled:opacity-50" <?= empty($plans['pro_monthly']) ? 'disabled' : '' ?>>
                        <?= empty($plans['pro_monthly']) ? 'Set STRIPE_PRICE_PRO_MONTHLY in .env' : 'Subscribe with Stripe' ?>
                    </button>
                </form>
                <?php if ($publishableKey === ''): ?>
                <p class="mt-3 text-xs text-amber-600">Stripe keys are not configured yet. Add them in `.env` before testing checkout.</p>
                <?php endif; ?>
            </div>
        </div>
    </div>
</body>
</html>