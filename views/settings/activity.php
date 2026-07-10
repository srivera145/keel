<?php
$logs = $logs ?? [];
$currentPage = $currentPage ?? 1;
$totalPages = $totalPages ?? 1;
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
                <p class="text-sm text-gray-500">Organization activity</p>
                <h1 class="text-3xl font-bold text-gray-900">Activity Log</h1>
            </div>
            <a href="/settings/organization" class="text-sm text-gray-500 hover:text-gray-900">Organization settings</a>
        </div>

        <div class="card overflow-x-auto p-0">
            <table class="table">
                <thead>
                    <tr>
                        <th>When</th>
                        <th>Action</th>
                        <th>Subject</th>
                        <th>Actor</th>
                        <th>IP</th>
                        <th>Metadata</th>
                    </tr>
                </thead>
                <tbody>
                    <?php if ($logs === []): ?>
                    <tr>
                        <td colspan="6" class="p-6">
                            <div class="empty-state">
                                <span class="empty-state-icon" aria-hidden="true">i</span>
                                <p class="empty-state-title">No activity yet</p>
                                <p class="empty-state-text">Actions taken by organization members will appear here.</p>
                            </div>
                        </td>
                    </tr>
                    <?php endif; ?>

                    <?php foreach ($logs as $log): ?>
                    <?php $metadata = $log['metadata'] ? json_decode((string) $log['metadata'], true) : null; ?>
                    <tr>
                        <td><?= htmlspecialchars((string) $log['created_at']) ?></td>
                        <td class="font-medium text-gray-900"><?= htmlspecialchars((string) $log['action']) ?></td>
                        <td><?= htmlspecialchars((string) ($log['subject_type'] ?? '-')) ?>#<?= htmlspecialchars((string) ($log['subject_id'] ?? '-')) ?></td>
                        <td><?= htmlspecialchars((string) ($log['user_id'] ?? '-')) ?></td>
                        <td><?= htmlspecialchars((string) ($log['ip_address'] ?? '-')) ?></td>
                        <td class="text-xs text-gray-500"><?= htmlspecialchars($metadata ? json_encode($metadata, JSON_UNESCAPED_SLASHES) : '-') ?></td>
                    </tr>
                    <?php endforeach; ?>
                </tbody>
            </table>
        </div>

        <div class="pagination">
            <span>Page <?= (int) $currentPage ?> of <?= (int) $totalPages ?></span>
            <div class="pagination-links">
                <?php if ($currentPage > 1): ?>
                <a href="/settings/activity?page=<?= (int) $currentPage - 1 ?>" class="pagination-link">Previous</a>
                <?php endif; ?>
                <?php if ($currentPage < $totalPages): ?>
                <a href="/settings/activity?page=<?= (int) $currentPage + 1 ?>" class="pagination-link">Next</a>
                <?php endif; ?>
            </div>
        </div>
    </div>
</body>
</html>
