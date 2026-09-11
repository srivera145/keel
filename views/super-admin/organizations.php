<?php
use EchoDial\Deck\Deck;
use Keel\Core\Theme;

$organizations = $organizations ?? [];
$selectedMembers = $selectedMembers ?? [];
?>
<!DOCTYPE html>
<html <?= Deck::htmlAttributes(lang: 'en') ?> <?= Deck::theme(mode: Theme::serverPreference()) ?>>
<head>
<?php require __DIR__ . '/../partials/head.php'; ?>
</head>
<body>
    <span id="top" tabindex="-1"></span>

    <main class="container settings-page stack stack-6" style="--container: 72rem; --rail: 26rem">
        <header class="bar">
            <div class="stack stack-2">
                <nav aria-label="Breadcrumb">
                    <ol class="breadcrumb">
                        <li><a href="/dashboard">Dashboard</a></li>
                        <li><span aria-current="page">Organizations</span></li>
                    </ol>
                </nav>
                <h1 class="h2">Organizations</h1>
            </div>
            <?php $themeToggleClass = 'push'; require __DIR__ . '/../partials/theme-toggle.php'; ?>
        </header>

        <div class="split">
            <section class="card" aria-labelledby="organizations-title">
                <div class="card-header">
                    <h2 id="organizations-title" class="card-title">All organizations</h2>
                </div>
                <div class="table-wrap" tabindex="0" role="region" aria-labelledby="organizations-title">
                    <table class="table table-stack">
                        <thead>
                            <tr>
                                <th scope="col">Organization</th>
                                <th scope="col">Slug</th>
                                <th scope="col" class="num">Members</th>
                            </tr>
                        </thead>
                        <tbody>
                            <?php if ($organizations === []): ?>
                            <tr>
                                <td colspan="3" class="table-empty">
                                    <div class="empty">
                                        <span class="empty-art"><?= Deck::icon('grid') ?></span>
                                        <p class="empty-title">No organizations yet</p>
                                        <p>Organizations will appear here once users complete onboarding.</p>
                                    </div>
                                </td>
                            </tr>
                            <?php endif; ?>

                            <?php foreach ($organizations as $organization): ?>
                            <tr>
                                <td data-label="Organization" class="fw-medium">
                                    <a href="/super-admin/organizations/<?= (int) $organization['id'] ?>"><?= htmlspecialchars((string) $organization['name']) ?></a>
                                    <?php if (isset($organization['brand_hue']) && $organization['brand_hue'] !== null): ?>
                                    <span class="hue-swatch" style="--hue: <?= (int) $organization['brand_hue'] ?>" role="img" aria-label="Brand hue <?= (int) $organization['brand_hue'] ?>"></span>
                                    <?php endif; ?>
                                </td>
                                <td data-label="Slug"><code><?= htmlspecialchars((string) $organization['slug']) ?></code></td>
                                <td data-label="Members" class="num"><?= htmlspecialchars((string) $organization['member_count']) ?></td>
                            </tr>
                            <?php endforeach; ?>
                        </tbody>
                    </table>
                </div>
            </section>

            <section class="card" aria-labelledby="selected-title">
                <?php if (empty($selectedOrganization)): ?>
                <div class="card-body">
                    <div class="empty">
                        <span class="empty-art"><?= Deck::icon('list') ?></span>
                        <h2 id="selected-title" class="empty-title">Select an organization</h2>
                        <p>Choose an organization from the table to inspect its member list.</p>
                    </div>
                </div>
                <?php else: ?>
                <div class="card-header">
                    <div class="stack stack-1">
                        <h2 id="selected-title" class="card-title"><?= htmlspecialchars((string) $selectedOrganization['name']) ?></h2>
                        <p class="text-sm text-muted">Slug <code><?= htmlspecialchars((string) $selectedOrganization['slug']) ?></code></p>
                    </div>
                </div>
                <div class="table-wrap" tabindex="0" role="region" aria-labelledby="selected-title">
                    <table class="table table-stack">
                        <thead>
                            <tr>
                                <th scope="col">Email</th>
                                <th scope="col">Role</th>
                            </tr>
                        </thead>
                        <tbody>
                            <?php if ($selectedMembers === []): ?>
                            <tr>
                                <td colspan="2" class="table-empty">
                                    <div class="empty">
                                        <span class="empty-art"><?= Deck::icon('users') ?></span>
                                        <p class="empty-title">No members yet</p>
                                        <p>Members will appear here once invited users accept.</p>
                                    </div>
                                </td>
                            </tr>
                            <?php endif; ?>

                            <?php foreach ($selectedMembers as $member): ?>
                            <tr>
                                <td data-label="Email" class="fw-medium"><?= htmlspecialchars((string) $member['email']) ?></td>
                                <td data-label="Role" class="uppercase"><?= htmlspecialchars((string) $member['role']) ?></td>
                            </tr>
                            <?php endforeach; ?>
                        </tbody>
                    </table>
                </div>
                <?php endif; ?>
            </section>
        </div>
    </main>

    <?php require __DIR__ . '/../partials/back-to-top.php'; ?>
</body>
</html>
