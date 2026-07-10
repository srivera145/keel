<?php
$organizations = $organizations ?? [];
$selectedMembers = $selectedMembers ?? [];
?>
<!DOCTYPE html>
<html lang="en"<?= \Keel\Core\Theme::htmlAttribute() ?>>
<head>
<?php require __DIR__ . '/../partials/head.php'; ?>
</head>
<body class="bg-gray-50 min-h-screen">
    <div class="mx-auto max-w-6xl px-4 py-10">
        <div class="mb-8 flex items-center justify-between gap-4">
            <div>
                <p class="text-sm text-gray-500">Platform operator</p>
                <h1 class="text-3xl font-bold text-gray-900">Organizations</h1>
            </div>
            <a href="/dashboard" class="text-sm text-gray-500 hover:text-gray-900">Back to dashboard</a>
        </div>

        <div class="grid gap-6 lg:grid-cols-[minmax(0,1.1fr)_minmax(320px,0.9fr)]">
            <div class="card overflow-x-auto p-0">
                <table class="table">
                    <thead>
                        <tr>
                            <th>Organization</th>
                            <th>Slug</th>
                            <th>Members</th>
                        </tr>
                    </thead>
                    <tbody>
                        <?php if ($organizations === []): ?>
                        <tr>
                            <td colspan="3" class="p-6">
                                <div class="empty-state">
                                    <span class="empty-state-icon" aria-hidden="true">o</span>
                                    <p class="empty-state-title">No organizations yet</p>
                                    <p class="empty-state-text">Organizations will appear here once users complete onboarding.</p>
                                </div>
                            </td>
                        </tr>
                        <?php endif; ?>

                        <?php foreach ($organizations as $organization): ?>
                        <tr>
                            <td><a class="font-medium text-gray-900 underline" href="/super-admin/organizations/<?= (int) $organization['id'] ?>"><?= htmlspecialchars((string) $organization['name']) ?></a></td>
                            <td><?= htmlspecialchars((string) $organization['slug']) ?></td>
                            <td><?= htmlspecialchars((string) $organization['member_count']) ?></td>
                        </tr>
                        <?php endforeach; ?>
                    </tbody>
                </table>
            </div>

            <div class="card">
                <?php if (empty($selectedOrganization)): ?>
                <h2 class="text-lg font-semibold text-gray-900">Select an organization</h2>
                <p class="mt-3 text-sm leading-6 text-gray-600">Choose an organization from the table to inspect its member list.</p>
                <?php else: ?>
                <h2 class="text-lg font-semibold text-gray-900"><?= htmlspecialchars((string) $selectedOrganization['name']) ?></h2>
                <p class="mt-1 text-sm text-gray-500">Slug: <?= htmlspecialchars((string) $selectedOrganization['slug']) ?></p>
                <div class="mt-5 overflow-x-auto rounded-xl border border-gray-200">
                    <table class="table">
                        <thead>
                            <tr>
                                <th>Email</th>
                                <th>Role</th>
                            </tr>
                        </thead>
                        <tbody>
                            <?php if ($selectedMembers === []): ?>
                            <tr>
                                <td colspan="2" class="p-6">
                                    <div class="empty-state">
                                        <span class="empty-state-icon" aria-hidden="true">m</span>
                                        <p class="empty-state-title">No members yet</p>
                                        <p class="empty-state-text">Members will appear here once invited users accept.</p>
                                    </div>
                                </td>
                            </tr>
                            <?php endif; ?>
                            <?php foreach ($selectedMembers as $member): ?>
                            <tr>
                                <td class="font-medium text-gray-900"><?= htmlspecialchars((string) $member['email']) ?></td>
                                <td class="uppercase tracking-wide"><?= htmlspecialchars((string) $member['role']) ?></td>
                            </tr>
                            <?php endforeach; ?>
                        </tbody>
                    </table>
                </div>
                <?php endif; ?>
            </div>
        </div>
    </div>
</body>
</html>