<!DOCTYPE html>
<html lang="en"<?= \Keel\Core\Theme::htmlAttribute() ?>>
<head>
<?php require __DIR__ . '/../partials/head.php'; ?>
</head>
<body class="bg-gray-50 min-h-screen flex items-center justify-center px-4">
    <div class="card w-full max-w-xl p-8 text-center">
        <p class="text-sm text-gray-500">Stripe Checkout</p>
        <h1 class="mt-2 text-3xl font-bold text-gray-900">Checkout complete</h1>
        <p class="mt-4 text-sm leading-6 text-gray-600">Your subscription is confirmed by Stripe webhook events, not by this redirect alone. If your account does not reflect the change immediately, refresh in a moment and Stripe will catch up.</p>
        <div class="mt-8 flex flex-col gap-3 sm:flex-row sm:justify-center">
            <a href="/billing/upgrade" class="btn btn-secondary btn-md">Back to billing</a>
            <a href="/dashboard" class="btn btn-md border border-gray-300 bg-white text-gray-700 hover:bg-gray-100">Go to dashboard</a>
        </div>
    </div>
</body>
</html>