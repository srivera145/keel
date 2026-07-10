<?php
$members = $members ?? [];
$pendingInvites = $pendingInvites ?? [];
?>
<!DOCTYPE html>
<html lang="en"<?= \Keel\Core\Theme::htmlAttribute() ?>>
<head>
<?php require __DIR__ . '/../partials/head.php'; ?>
</head>
<body class="bg-gray-50 min-h-screen">
    <div class="mx-auto max-w-5xl px-4 py-10">
        <div class="mb-8 flex items-center justify-between gap-4">
            <div>
                <p class="text-sm text-gray-500">Organization members</p>
                <h1 class="text-3xl font-bold text-gray-900"><?= htmlspecialchars((string) ($organization['name'] ?? 'Organization')) ?></h1>
            </div>
            <a href="/settings/organization" class="text-sm text-gray-500 hover:text-gray-900">Organization settings</a>
        </div>

        <?php if (!empty($_GET['status']) && $_GET['status'] === 'invite_sent'): ?>
        <div class="alert alert-success mb-6 px-4 py-3">Invite sent.</div>
        <?php endif; ?>

        <?php if (!empty($_GET['error'])): ?>
        <div class="alert alert-error mb-6 px-4 py-3"><?= htmlspecialchars((string) $_GET['error']) ?></div>
        <?php endif; ?>

        <div class="grid gap-6 lg:grid-cols-[minmax(0,1.2fr)_minmax(320px,0.8fr)]">
            <div class="card">
                <h2 class="text-lg font-semibold text-gray-900">Team members</h2>
                <div class="mt-4 overflow-x-auto rounded-xl border border-gray-200">
                    <table class="table">
                        <thead>
                            <tr>
                                <th>Email</th>
                                <th>Role</th>
                                <th>Status</th>
                            </tr>
                        </thead>
                        <tbody>
                            <?php if ($members === []): ?>
                            <tr>
                                <td colspan="3" class="p-6">
                                    <div class="empty-state">
                                        <span class="empty-state-icon" aria-hidden="true">m</span>
                                        <p class="empty-state-title">No members yet</p>
                                        <p class="empty-state-text">Team members will appear here after they join the organization.</p>
                                    </div>
                                </td>
                            </tr>
                            <?php endif; ?>

                            <?php foreach ($members as $member): ?>
                            <tr>
                                <td class="font-medium text-gray-900"><?= htmlspecialchars((string) $member['email']) ?></td>
                                <td class="uppercase tracking-wide"><?= htmlspecialchars((string) $member['role']) ?></td>
                                <td>
                                    <?php if ((int) ($member['is_super_admin'] ?? 0) === 1): ?>
                                    <span class="badge badge-neutral">Super admin</span>
                                    <?php else: ?>
                                    <span class="text-xs text-gray-500">Member</span>
                                    <?php endif; ?>
                                </td>
                            </tr>
                            <?php endforeach; ?>
                        </tbody>
                    </table>
                </div>
            </div>

            <div class="space-y-6">
                <div class="card">
                    <h2 class="text-lg font-semibold text-gray-900">Invite teammate</h2>
                    <form method="POST" action="/settings/members/invite" class="mt-4 space-y-4">
                        <?= \Keel\Core\Csrf::field() ?>
                        <div>
                            <label class="form-label">Email</label>
                            <input type="email" name="email" class="form-input" required>
                        </div>
                        <div>
                            <label class="form-label">Role</label>
                            <select name="role" class="form-input">
                                <option value="member">Member</option>
                                <option value="admin">Admin</option>
                            </select>
                        </div>
                        <button type="submit" class="btn btn-secondary btn-md w-full">Send invite</button>
                    </form>
                </div>

                <div class="card">
                    <h2 class="text-lg font-semibold text-gray-900">Pending invites</h2>
                    <div class="mt-4 overflow-x-auto rounded-xl border border-gray-200">
                        <table class="table">
                            <thead>
                                <tr>
                                    <th>Email</th>
                                    <th>Role</th>
                                    <th>Expires</th>
                                </tr>
                            </thead>
                            <tbody>
                                <?php if ($pendingInvites === []): ?>
                                <tr>
                                    <td colspan="3" class="p-6">
                                        <div class="empty-state">
                                            <span class="empty-state-icon" aria-hidden="true">i</span>
                                            <p class="empty-state-title">No pending invites</p>
                                            <p class="empty-state-text">Invites you send will appear here until accepted or expired.</p>
                                        </div>
                                    </td>
                                </tr>
                                <?php endif; ?>

                                <?php foreach ($pendingInvites as $invite): ?>
                                <tr>
                                    <td class="font-medium text-gray-900"><?= htmlspecialchars((string) $invite['email']) ?></td>
                                    <td class="uppercase tracking-wide"><?= htmlspecialchars((string) $invite['role']) ?></td>
                                    <td>Expires <?= htmlspecialchars((string) $invite['expires_at']) ?></td>
                                </tr>
                                <?php endforeach; ?>
                            </tbody>
                        </table>
                    </div>
                </div>
            </div>
        </div>
    </div>
</body>
</html>