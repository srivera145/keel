<!DOCTYPE html>
<html lang="en"<?= \Keel\Core\Theme::htmlAttribute() ?>>
<head>
<?php require __DIR__ . '/../partials/head.php'; ?>
</head>
<body class="bg-gray-50 min-h-screen">
    <div class="mx-auto max-w-4xl px-4 py-10">
        <div class="mb-8 flex items-center justify-between gap-4">
            <div>
                <p class="text-sm text-gray-500">Organization settings</p>
                <h1 class="text-3xl font-bold text-gray-900"><?= htmlspecialchars((string) ($organization['name'] ?? 'Organization')) ?></h1>
            </div>
            <a href="/dashboard" class="text-sm text-gray-500 hover:text-gray-900">Back to dashboard</a>
        </div>

        <div class="card">
            <dl class="grid gap-6 sm:grid-cols-2">
                <div>
                    <dt class="text-sm text-gray-500">Organization name</dt>
                    <dd class="mt-1 text-base font-medium text-gray-900"><?= htmlspecialchars((string) ($organization['name'] ?? '')) ?></dd>
                </div>
                <div>
                    <dt class="text-sm text-gray-500">Slug</dt>
                    <dd class="mt-1 text-base font-medium text-gray-900"><?= htmlspecialchars((string) ($organization['slug'] ?? '')) ?></dd>
                </div>
                <div>
                    <dt class="text-sm text-gray-500">Your role</dt>
                    <dd class="mt-1 text-base font-medium text-gray-900"><?= htmlspecialchars((string) ($user['role'] ?? '')) ?></dd>
                </div>
                <div>
                    <dt class="text-sm text-gray-500">Members</dt>
                    <dd class="mt-1 text-base font-medium text-gray-900"><a href="/settings/members" class="text-gray-900 underline">Manage members</a></dd>
                </div>
            </dl>
        </div>
    </div>
</body>
</html>