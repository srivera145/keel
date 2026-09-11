<?php
use EchoDial\Deck\Deck;
use Keel\Core\Csrf;
use Keel\Core\Theme;

$authMethod = $authMethod ?? 'both';
?>
<!DOCTYPE html>
<html <?= Deck::htmlAttributes(lang: 'en') ?> <?= Deck::theme(mode: Theme::serverPreference()) ?>>
<head>
<?php require __DIR__ . '/../partials/head.php'; ?>
</head>
<body>
    <?php $csrfToken = Csrf::token(); ?>

    <main class="container stage" style="--stage-width: 26rem">
        <section class="card">
            <div class="card-body stack stack-6">
                <div class="bar">
                    <img class="brand-logo when-light" src="/images/brand/keel-icon.png" alt="Keel" width="40" height="40" loading="eager" decoding="async">
                    <img class="brand-logo when-dark" src="/images/brand/keel-icon-light.png" alt="Keel" width="40" height="40" loading="eager" decoding="async">
                    <div class="stack stack-0">
                        <h1 class="h4">Sign in</h1>
                        <p class="text-sm text-muted">No password needed.</p>
                    </div>
                    <?php $themeToggleClass = 'push'; require __DIR__ . '/../partials/theme-toggle.php'; ?>
                </div>

                <?php if (!empty($_GET['error']) && $_GET['error'] === 'invalid_invite'): ?>
                <div class="alert alert-bad">
                    <?= Deck::icon('alert-circle') ?>
                    <p>That invite link is invalid, expired, or already used.</p>
                </div>
                <?php endif; ?>

                <?php if ($authMethod === 'both'): ?>
                <div class="tabs" role="tablist" aria-label="Sign-in method">
                    <button type="button" class="tab" role="tab" id="tab-otp" aria-controls="panel-otp" aria-selected="true">Code</button>
                    <button type="button" class="tab" role="tab" id="tab-magic" aria-controls="panel-magic" aria-selected="false" tabindex="-1">Magic Link</button>
                </div>
                <?php endif; ?>

                <?php if ($authMethod === 'otp' || $authMethod === 'both'): ?>
                <div id="panel-otp" class="stack stack-4" role="tabpanel" aria-labelledby="tab-otp">
                    <div id="otp-step-email" class="stack stack-4">
                        <div class="field">
                            <label class="label" for="otp-email">Email</label>
                            <input type="email" id="otp-email" class="input" autocomplete="email" placeholder="you@example.com">
                        </div>
                        <button type="button" id="otp-send" class="btn btn-primary btn-block">Send Code</button>
                    </div>
                    <div id="otp-step-code" class="stack stack-4" hidden>
                        <div class="field">
                            <label class="label" for="otp-code">Enter the 6-digit code</label>
                            <input type="text" id="otp-code" class="input otp-input" maxlength="6" inputmode="numeric" autocomplete="one-time-code" placeholder="000000">
                        </div>
                        <button type="button" id="otp-verify" class="btn btn-primary btn-block">Verify</button>
                    </div>
                </div>
                <?php endif; ?>

                <?php if ($authMethod === 'magic_link' || $authMethod === 'both'): ?>
                <div id="panel-magic" class="stack stack-4" role="tabpanel" aria-labelledby="tab-magic" <?= $authMethod === 'both' ? 'hidden' : '' ?>>
                    <div class="field">
                        <label class="label" for="magic-email">Email</label>
                        <input type="email" id="magic-email" class="input" autocomplete="email" placeholder="you@example.com">
                    </div>
                    <button type="button" id="magic-send" class="btn btn-primary btn-block">Send Magic Link</button>
                    <div id="magic-sent" class="alert alert-good" hidden>
                        <?= Deck::icon('mail') ?>
                        <p>Check your email for the sign-in link.</p>
                    </div>
                </div>
                <?php endif; ?>

                <p id="auth-error" class="error" hidden></p>
            </div>
        </section>
    </main>

    <script>
        const csrfToken = document.querySelector('meta[name="csrf-token"]')?.getAttribute('content') || '<?= htmlspecialchars($csrfToken, ENT_QUOTES, 'UTF-8') ?>';

        const showError = (msg) => {
            const el = document.getElementById('auth-error');
            el.textContent = msg;
            el.hidden = false;
        };

        const post = async (url, body) => {
            const res = await fetch(url, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json', 'X-CSRF-Token': csrfToken },
                body: JSON.stringify(body),
            });
            return res.json();
        };

        <?php if ($authMethod === 'both'): ?>
        /* Deck styles .tab from aria-selected but ships no tab behaviour, so the roving
           tabindex and the arrow keys are ours. */
        const tabs = Array.from(document.querySelectorAll('[role="tab"]'));

        const selectTab = (tab) => {
            tabs.forEach((candidate) => {
                const selected = candidate === tab;
                candidate.setAttribute('aria-selected', selected ? 'true' : 'false');
                candidate.tabIndex = selected ? 0 : -1;
                document.getElementById(candidate.getAttribute('aria-controls')).hidden = !selected;
            });
        };

        tabs.forEach((tab, index) => {
            tab.addEventListener('click', () => selectTab(tab));
            tab.addEventListener('keydown', (event) => {
                const step = event.key === 'ArrowRight' ? 1 : event.key === 'ArrowLeft' ? -1 : 0;
                if (!step) {
                    return;
                }
                event.preventDefault();
                const next = tabs[(index + step + tabs.length) % tabs.length];
                next.focus();
                selectTab(next);
            });
        });
        <?php endif; ?>

        <?php if ($authMethod === 'otp' || $authMethod === 'both'): ?>
        document.getElementById('otp-send').addEventListener('click', async () => {
            const data = await post('/auth/otp/request', { email: document.getElementById('otp-email').value });
            if (data.success) {
                document.getElementById('otp-step-email').hidden = true;
                document.getElementById('otp-step-code').hidden = false;
                document.getElementById('otp-code').focus();
            } else {
                showError(data.message || 'Something went wrong.');
            }
        });

        document.getElementById('otp-verify').addEventListener('click', async () => {
            const data = await post('/auth/otp/verify', {
                email: document.getElementById('otp-email').value,
                code: document.getElementById('otp-code').value,
            });
            if (data.success) {
                window.location.href = data.redirect || '/dashboard';
            } else {
                showError(data.message || 'Invalid code.');
            }
        });
        <?php endif; ?>

        <?php if ($authMethod === 'magic_link' || $authMethod === 'both'): ?>
        document.getElementById('magic-send').addEventListener('click', async () => {
            const data = await post('/auth/magic/request', { email: document.getElementById('magic-email').value });
            if (data.success) {
                document.getElementById('magic-sent').hidden = false;
            } else {
                showError(data.message || 'Something went wrong.');
            }
        });
        <?php endif; ?>
    </script>
</body>
</html>
