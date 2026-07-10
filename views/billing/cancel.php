<!DOCTYPE html>
<html lang="en"<?= \Keel\Core\Theme::htmlAttribute() ?>>
<head>
<?php require __DIR__ . '/../partials/head.php'; ?>
</head>
<body class="bg-gray-50 min-h-screen flex items-center justify-center px-4">
    <div class="card w-full max-w-xl p-8 text-center">
        <p class="text-sm text-gray-500">Stripe Checkout</p>
        <h1 class="mt-2 text-3xl font-bold text-gray-900">Checkout cancelled</h1>
        <p class="mt-4 text-sm leading-6 text-gray-600">No subscription changes were made locally. You can go back to billing whenever you want to try again.</p>
        <div class="mt-8">
            <a href="/billing/upgrade" class="btn btn-secondary btn-md">Return to billing</a>
        </div>
    </div>
</body>
</html>