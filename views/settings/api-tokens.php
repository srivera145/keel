<?php
use EchoDial\Deck\Deck;
use Keel\Core\Csrf;
use Keel\Core\Theme;

$tokens = $tokens ?? [];
$newToken = $newToken ?? null;
$error = $error ?? null;
// deck-extras.js carries the copy button, and only a freshly created token needs it.
$deckExtras = is_string($newToken) && $newToken !== '';
?>
<!DOCTYPE html>
<html <?= Deck::htmlAttributes(lang: 'en') ?> <?= Deck::theme(mode: Theme::serverPreference()) ?>>
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
                        <li><a href="/dashboard">Dashboard</a></li>
                        <li><span aria-current="page">API tokens</span></li>
                    </ol>
                </nav>
                <h1 class="h2">API Tokens</h1>
            </div>
            <?php $themeToggleClass = 'push'; require __DIR__ . '/../partials/theme-toggle.php'; ?>
        </header>

        <?php if (is_string($newToken) && $newToken !== ''): ?>
        <div class="alert alert-warn">
            <?= Deck::icon('alert-triangle') ?>
            <div class="stack stack-2 grow">
                <p class="alert-title">Copy this token now. You will not be able to see it again.</p>
                <div class="copy">
                    <code class="copy-value"><?= htmlspecialchars($newToken) ?></code>
                    <button type="button" class="copy-btn" data-deck-copy>
                        <span class="copy-idle"><?= Deck::icon('copy', 'icon icon-sm') ?> Copy</span>
                        <span class="copy-done"><?= Deck::icon('check', 'icon icon-sm') ?> Copied</span>
                    </button>
                </div>
            </div>
        </div>
        <?php endif; ?>

        <?php if (is_string($error) && $error !== ''): ?>
        <div class="alert alert-bad">
            <?= Deck::icon('alert-circle') ?>
            <p><?= htmlspecialchars($error) ?></p>
        </div>
        <?php endif; ?>

        <div class="split">
            <section class="card" aria-labelledby="tokens-title">
                <div class="card-header">
                    <h2 id="tokens-title" class="card-title">Issued tokens</h2>
                </div>
                <div class="table-wrap" tabindex="0" role="region" aria-labelledby="tokens-title">
                    <table class="table table-stack">
                        <thead>
                            <tr>
                                <th scope="col">Name</th>
                                <th scope="col">Abilities</th>
                                <th scope="col">Last used</th>
                                <th scope="col">Expires</th>
                                <th scope="col" class="text-end">Actions</th>
                            </tr>
                        </thead>
                        <tbody>
                            <?php if ($tokens === []): ?>
                            <tr>
                                <td colspan="5" class="table-empty">
                                    <div class="empty">
                                        <span class="empty-art"><?= Deck::icon('lock') ?></span>
                                        <p class="empty-title">No API tokens yet</p>
                                        <p>Create a token to authenticate scripts or integrations.</p>
                                    </div>
                                </td>
                            </tr>
                            <?php endif; ?>

                            <?php foreach ($tokens as $token): ?>
                            <tr>
                                <td data-label="Name" class="fw-medium"><?= htmlspecialchars((string) $token['name']) ?></td>
                                <td data-label="Abilities"><code><?= htmlspecialchars((string) $token['abilities']) ?></code></td>
                                <td data-label="Last used" class="nums"><?= htmlspecialchars((string) ($token['last_used_at'] ?? 'Never')) ?></td>
                                <td data-label="Expires" class="nums"><?= htmlspecialchars((string) ($token['expires_at'] ?? 'Never')) ?></td>
                                <td data-label="Actions" class="text-end">
                                    <form id="revoke-form-<?= (int) $token['id'] ?>" method="POST" action="/settings/api-tokens/<?= (int) $token['id'] ?>/revoke" data-confirm="revoke-token-<?= (int) $token['id'] ?>">
                                        <?= Csrf::field() ?>
                                        <button type="submit" class="btn btn-danger btn-sm">Revoke</button>
                                    </form>
                                </td>
                            </tr>
                            <?php endforeach; ?>
                        </tbody>
                    </table>
                </div>
            </section>

            <section class="card" aria-labelledby="create-title">
                <div class="card-header">
                    <h2 id="create-title" class="card-title">Create token</h2>
                </div>
                <form method="POST" action="/settings/api-tokens" class="card-body">
                    <?= Csrf::field() ?>
                    <div class="field">
                        <label class="label" for="token-name">Name</label>
                        <input type="text" id="token-name" name="name" class="input" placeholder="CI integration" required>
                    </div>
                    <div class="field">
                        <label class="label" for="token-abilities">Abilities</label>
                        <input type="text" id="token-abilities" name="abilities" value="*" class="input" placeholder="read,write">
                        <p class="help">Comma-separated, or <code>*</code> for everything.</p>
                    </div>
                    <div class="field">
                        <label class="label" for="token-expires">Expires at <span class="optional">optional</span></label>
                        <input type="datetime-local" id="token-expires" name="expires_at" class="input">
                    </div>
                    <button type="submit" class="btn btn-primary btn-block">Create token</button>
                </form>
            </section>
        </div>
    </main>

    <?php foreach ($tokens as $token): ?>
    <dialog class="modal" id="revoke-token-<?= (int) $token['id'] ?>" aria-labelledby="revoke-title-<?= (int) $token['id'] ?>">
        <div class="modal-header">
            <h2 class="modal-title" id="revoke-title-<?= (int) $token['id'] ?>">Revoke token?</h2>
        </div>
        <div class="modal-body">
            <p>This will immediately disable <strong><?= htmlspecialchars((string) $token['name']) ?></strong>. Existing clients using it will fail authentication.</p>
        </div>
        <div class="modal-footer">
            <button type="button" class="btn" data-modal-close>Cancel</button>
            <button type="button" class="btn btn-danger" data-confirm-submit="revoke-form-<?= (int) $token['id'] ?>">Revoke token</button>
        </div>
    </dialog>
    <?php endforeach; ?>

    <?php require __DIR__ . '/../partials/back-to-top.php'; ?>
</body>
</html>
