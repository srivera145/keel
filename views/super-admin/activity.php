<?php
use EchoDial\Deck\Deck;
use Keel\Core\Theme;

$logs = $logs ?? [];
$currentPage = $currentPage ?? 1;
$totalPages = $totalPages ?? 1;
?>
<!DOCTYPE html>
<html <?= Deck::htmlAttributes(lang: 'en') ?> <?= Deck::theme(mode: Theme::serverPreference()) ?>>
<head>
<?php require __DIR__ . '/../partials/head.php'; ?>
</head>
<body>
    <span id="top" tabindex="-1"></span>

    <main class="container settings-page stack stack-6" style="--container: 80rem">
        <header class="bar">
            <div class="stack stack-2">
                <nav aria-label="Breadcrumb">
                    <ol class="breadcrumb">
                        <li><a href="/super-admin/organizations">Organizations</a></li>
                        <li><span aria-current="page">Platform activity</span></li>
                    </ol>
                </nav>
                <h1 class="h2">Platform Activity</h1>
            </div>
            <?php $themeToggleClass = 'push'; require __DIR__ . '/../partials/theme-toggle.php'; ?>
        </header>

        <section class="card" aria-labelledby="platform-activity-title">
            <h2 id="platform-activity-title" class="sr-only">Platform activity</h2>
            <div class="table-wrap" tabindex="0" role="region" aria-labelledby="platform-activity-title">
                <table class="table">
                    <thead>
                        <tr>
                            <th scope="col">When</th>
                            <th scope="col">Action</th>
                            <th scope="col">Org</th>
                            <th scope="col">Subject</th>
                            <th scope="col">Actor</th>
                            <th scope="col">Metadata</th>
                        </tr>
                    </thead>
                    <tbody>
                        <?php if ($logs === []): ?>
                        <tr>
                            <td colspan="6" class="table-empty">
                                <div class="empty">
                                    <span class="empty-art"><?= Deck::icon('clock') ?></span>
                                    <p class="empty-title">No activity yet</p>
                                    <p>Platform-level events will appear here after activity is recorded.</p>
                                </div>
                            </td>
                        </tr>
                        <?php endif; ?>

                        <?php foreach ($logs as $log): ?>
                        <?php $metadata = $log['metadata'] ? json_decode((string) $log['metadata'], true) : null; ?>
                        <tr>
                            <td class="nums"><?= htmlspecialchars((string) $log['created_at']) ?></td>
                            <td class="fw-medium"><?= htmlspecialchars((string) $log['action']) ?></td>
                            <td class="nums"><?= htmlspecialchars((string) ($log['organization_id'] ?? '-')) ?></td>
                            <td><?= htmlspecialchars((string) ($log['subject_type'] ?? '-')) ?>#<?= htmlspecialchars((string) ($log['subject_id'] ?? '-')) ?></td>
                            <td class="nums"><?= htmlspecialchars((string) ($log['user_id'] ?? '-')) ?></td>
                            <td><code><?= htmlspecialchars($metadata ? json_encode($metadata, JSON_UNESCAPED_SLASHES) : '-') ?></code></td>
                        </tr>
                        <?php endforeach; ?>
                    </tbody>
                </table>
            </div>
        </section>

        <nav class="bar wrap" aria-label="Pagination">
            <p class="text-sm text-muted">Page <?= (int) $currentPage ?> of <?= (int) $totalPages ?></p>
            <div class="pagination push">
                <?php if ($currentPage > 1): ?>
                <a href="/super-admin/activity?page=<?= (int) $currentPage - 1 ?>" rel="prev">Previous</a>
                <?php endif; ?>
                <?php if ($currentPage < $totalPages): ?>
                <a href="/super-admin/activity?page=<?= (int) $currentPage + 1 ?>" rel="next">Next</a>
                <?php endif; ?>
            </div>
        </nav>
    </main>

    <?php require __DIR__ . '/../partials/back-to-top.php'; ?>
</body>
</html>
