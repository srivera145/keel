<?php
use EchoDial\Deck\Deck;
use Keel\Core\Csrf;
use Keel\Core\Theme;
?>
<!DOCTYPE html>
<html <?= Deck::htmlAttributes(lang: 'en') ?> <?= Deck::theme(mode: Theme::serverPreference()) ?>>
<head>
<?php require __DIR__ . '/../partials/head.php'; ?>
</head>
<body>
    <?php $themeToggleClass = 'theme-toggle-floating'; require __DIR__ . '/../partials/theme-toggle.php'; ?>

    <main class="container stage" style="--stage-width: 30rem">
        <section class="card">
            <div class="card-body stack stack-4">
                <div class="stack stack-2">
                    <p class="text-sm uppercase text-muted">Organization setup</p>
                    <h1 class="h3">What is your organization called?</h1>
                    <p class="text-sm text-muted">Create the team space above your account. You will be the owner.</p>
                </div>

                <?php if (!empty($_GET['error']) && $_GET['error'] === 'missing_name'): ?>
                <div class="alert alert-bad">
                    <?= Deck::icon('alert-circle') ?>
                    <p>Enter an organization name.</p>
                </div>
                <?php endif; ?>

                <form method="POST" action="/onboarding/organization" class="stack stack-4">
                    <?= Csrf::field() ?>
                    <div class="field">
                        <label class="label" for="organization-name">Organization name</label>
                        <input id="organization-name" name="name" type="text" class="input" placeholder="Acme Motors" required>
                    </div>
                    <button type="submit" class="btn btn-primary btn-block">Create organization</button>
                </form>
            </div>
        </section>
    </main>
</body>
</html>
