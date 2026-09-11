<!DOCTYPE html>
<html lang="en"<?= \Keel\Core\Theme::htmlAttribute() ?>>
<head>
<?php require __DIR__ . '/../partials/head.php'; ?>
</head>
<body class="bg-gray-50 min-h-screen flex items-center justify-center px-4">
    <div class="card w-full max-w-md p-8">
        <p class="text-sm text-gray-500">Organization setup</p>
        <h1 class="mt-2 text-2xl font-bold text-gray-900">What is your organization called?</h1>
        <p class="mt-3 text-sm leading-6 text-gray-600">Create the team space above your account. You will be the owner.</p>

        <?php if (!empty($_GET['error']) && $_GET['error'] === 'missing_name'): ?>
        <div class="alert alert-error mt-4 px-3 py-2">Enter an organization name.</div>
        <?php endif; ?>

        <form method="POST" action="/onboarding/organization" class="mt-6 space-y-4">
            <?= \Keel\Core\Csrf::field() ?>
            <div>
                <label for="organization-name" class="form-label">Organization name</label>
                <input id="organization-name" name="name" type="text" class="form-input" placeholder="Acme Motors" required>
            </div>
            <button type="submit" class="btn btn-secondary btn-md w-full">Create organization</button>
        </form>
    </div>
</body>
</html>