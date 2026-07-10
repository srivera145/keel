<!DOCTYPE html>
<html lang="en"<?= \Keel\Core\Theme::htmlAttribute() ?>>
<head>
<?php require __DIR__ . '/../partials/head.php'; ?>
</head>
<body class="bg-gray-50 min-h-screen flex items-center justify-center px-4">
    <div class="card w-full max-w-xl p-8 text-center">
        <p class="text-sm font-medium uppercase tracking-wide text-gray-500">500</p>
        <h1 class="mt-2 text-3xl font-bold text-gray-900">Something went wrong</h1>
        <p class="mt-4 text-sm leading-6 text-gray-600">An unexpected error interrupted the request. Please try again, or return to the homepage.</p>

        <?php if (!empty($exception)): ?>
        <div class="alert mt-6 rounded-xl border-amber-200 bg-amber-50 p-4 text-left text-amber-900">
            <p class="font-semibold"><?= htmlspecialchars($exception->getMessage()) ?></p>
            <p class="mt-2 text-xs"><?= htmlspecialchars($exception->getFile()) ?>:<?= htmlspecialchars((string) $exception->getLine()) ?></p>
        </div>
        <?php endif; ?>

        <div class="mt-8">
            <a href="/" class="btn btn-secondary btn-md">Back to home</a>
        </div>
    </div>
</body>
</html>