<?php
use EchoDial\Deck\Deck;
use Keel\Core\Csrf;
use Keel\Core\Theme;

$members = $members ?? [];
$pendingInvites = $pendingInvites ?? [];
$brandHue = isset($organization['brand_hue']) ? (int) $organization['brand_hue'] : null;
?>
<!DOCTYPE html>
<html <?= Deck::htmlAttributes(lang: 'en') ?> <?= Deck::theme(hue: $brandHue, mode: Theme::serverPreference()) ?>>
<head>
<?php require __DIR__ . '/../partials/head.php'; ?>
</head>
<body>
    <span id="top" tabindex="-1"></span>

    <main class="container settings-page stack stack-6">
        <header class="bar">
            <div class="stack stack-2">
                <nav aria-label="Breadcrumb">
                    <ol class="breadcrumb">
                        <li><a href="/settings/organization">Organization settings</a></li>
                        <li><span aria-current="page">Members</span></li>
                    </ol>
                </nav>
                <h1 class="h2"><?= htmlspecialchars((string) ($organization['name'] ?? 'Organization')) ?></h1>
            </div>
            <?php $themeToggleClass = 'push'; require __DIR__ . '/../partials/theme-toggle.php'; ?>
        </header>

        <?php if (!empty($_GET['status']) && $_GET['status'] === 'invite_sent'): ?>
        <div class="alert alert-good">
            <?= Deck::icon('check-circle') ?>
            <p>Invite sent.</p>
        </div>
        <?php endif; ?>

        <?php if (!empty($_GET['error'])): ?>
        <div class="alert alert-bad">
            <?= Deck::icon('alert-circle') ?>
            <p><?= htmlspecialchars((string) $_GET['error']) ?></p>
        </div>
        <?php endif; ?>

        <div class="split">
            <section class="card" aria-labelledby="members-title">
                <div class="card-header">
                    <h2 id="members-title" class="card-title">Team members</h2>
                </div>
                <div class="table-wrap" tabindex="0" role="region" aria-labelledby="members-title">
                    <table class="table table-stack">
                        <thead>
                            <tr>
                                <th scope="col">Email</th>
                                <th scope="col">Role</th>
                                <th scope="col">Status</th>
                            </tr>
                        </thead>
                        <tbody>
                            <?php if ($members === []): ?>
                            <tr>
                                <td colspan="3" class="table-empty">
                                    <div class="empty">
                                        <span class="empty-art"><?= Deck::icon('users') ?></span>
                                        <p class="empty-title">No members yet</p>
                                        <p>Team members will appear here after they join the organization.</p>
                                    </div>
                                </td>
                            </tr>
                            <?php endif; ?>

                            <?php foreach ($members as $member): ?>
                            <tr>
                                <td data-label="Email" class="fw-medium"><?= htmlspecialchars((string) $member['email']) ?></td>
                                <td data-label="Role" class="uppercase"><?= htmlspecialchars((string) $member['role']) ?></td>
                                <td data-label="Status">
                                    <?php if ((int) ($member['is_super_admin'] ?? 0) === 1): ?>
                                    <span class="badge">Super admin</span>
                                    <?php else: ?>
                                    <span class="text-muted">Member</span>
                                    <?php endif; ?>
                                </td>
                            </tr>
                            <?php endforeach; ?>
                        </tbody>
                    </table>
                </div>
            </section>

            <div class="stack stack-6">
                <section class="card" aria-labelledby="invite-title">
                    <div class="card-header">
                        <h2 id="invite-title" class="card-title">Invite teammate</h2>
                    </div>
                    <form method="POST" action="/settings/members/invite" class="card-body">
                        <?= Csrf::field() ?>
                        <div class="field">
                            <label class="label" for="invite-email">Email</label>
                            <input id="invite-email" type="email" name="email" class="input" autocomplete="email" required>
                        </div>
                        <div class="field">
                            <label class="label" for="invite-role">Role</label>
                            <select id="invite-role" name="role" class="select">
                                <option value="member">Member</option>
                                <option value="admin">Admin</option>
                            </select>
                        </div>
                        <button type="submit" class="btn btn-primary btn-block">Send invite</button>
                    </form>
                </section>

                <section class="card" aria-labelledby="invites-title">
                    <div class="card-header">
                        <h2 id="invites-title" class="card-title">Pending invites</h2>
                    </div>
                    <div class="table-wrap" tabindex="0" role="region" aria-labelledby="invites-title">
                        <table class="table table-stack">
                            <thead>
                                <tr>
                                    <th scope="col">Email</th>
                                    <th scope="col">Role</th>
                                    <th scope="col">Expires</th>
                                </tr>
                            </thead>
                            <tbody>
                                <?php if ($pendingInvites === []): ?>
                                <tr>
                                    <td colspan="3" class="table-empty">
                                        <div class="empty">
                                            <span class="empty-art"><?= Deck::icon('mail') ?></span>
                                            <p class="empty-title">No pending invites</p>
                                            <p>Invites you send will appear here until accepted or expired.</p>
                                        </div>
                                    </td>
                                </tr>
                                <?php endif; ?>

                                <?php foreach ($pendingInvites as $invite): ?>
                                <tr>
                                    <td data-label="Email" class="fw-medium"><?= htmlspecialchars((string) $invite['email']) ?></td>
                                    <td data-label="Role" class="uppercase"><?= htmlspecialchars((string) $invite['role']) ?></td>
                                    <td data-label="Expires" class="nums"><?= htmlspecialchars((string) $invite['expires_at']) ?></td>
                                </tr>
                                <?php endforeach; ?>
                            </tbody>
                        </table>
                    </div>
                </section>
            </div>
        </div>
    </main>

    <?php require __DIR__ . '/../partials/back-to-top.php'; ?>
</body>
</html>
