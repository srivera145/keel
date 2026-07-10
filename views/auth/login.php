<?php
$authMethod = $authMethod ?? 'both';
?>
<!DOCTYPE html>
<html lang="en"<?= \Keel\Core\Theme::htmlAttribute() ?>>
<head>
<?php require __DIR__ . '/../partials/head.php'; ?>
<style>
    :root {
        --keel-night: #07111f;
        --keel-line: rgba(148, 163, 184, 0.18);
        --keel-text: #e5eefb;
        --keel-muted: #97a9c9;
        --keel-accent: #ff6b3d;
        --keel-cyan: #69e6d8;
    }

    [data-theme="light"] {
        --keel-night: #f8fafc;
        --keel-line: rgba(15, 23, 42, 0.16);
        --keel-text: #0f172a;
        --keel-muted: #475569;
    }

    body {
        background:
            radial-gradient(circle at top left, rgba(105, 230, 216, 0.14), transparent 28%),
            radial-gradient(circle at top right, rgba(255, 107, 61, 0.15), transparent 32%),
            linear-gradient(180deg, #091221 0%, #07111f 42%, #0c1730 100%);
        color: var(--keel-text);
    }

    [data-theme="light"] body {
        background:
            radial-gradient(circle at top left, rgba(105, 230, 216, 0.12), transparent 30%),
            radial-gradient(circle at top right, rgba(255, 107, 61, 0.12), transparent 35%),
            linear-gradient(180deg, #f8fafc 0%, #eef2f7 100%);
    }

    .keel-grid {
        background-image:
            linear-gradient(rgba(148, 163, 184, 0.08) 1px, transparent 1px),
            linear-gradient(90deg, rgba(148, 163, 184, 0.08) 1px, transparent 1px);
        background-size: 44px 44px;
        mask-image: linear-gradient(180deg, rgba(0, 0, 0, 0.95), transparent 90%);
    }

    .keel-auth-card {
        background: linear-gradient(180deg, rgba(11, 24, 48, 0.96), rgba(7, 17, 31, 0.92));
        border: 1px solid var(--keel-line);
        box-shadow: 0 30px 90px rgba(0, 0, 0, 0.42);
    }

    [data-theme="light"] .keel-auth-card {
        background: linear-gradient(180deg, rgba(255, 255, 255, 0.98), rgba(248, 250, 252, 0.96));
        box-shadow: 0 22px 60px rgba(15, 23, 42, 0.14);
    }

    .keel-auth-content {
        padding: 1.75rem;
    }

    @media (min-width: 640px) {
        .keel-auth-content {
            padding: 2rem;
        }
    }

    .keel-auth-card::before {
        content: '';
        position: absolute;
        inset: 0;
        border-radius: inherit;
        border: 1px solid rgba(255, 255, 255, 0.04);
        pointer-events: none;
    }

    .keel-auth-input {
        background: rgba(11, 24, 48, 0.72);
        border: 1px solid var(--keel-line);
        color: var(--keel-text);
        transition: border-color 160ms ease, box-shadow 160ms ease;
    }

    .keel-auth-input::placeholder {
        color: var(--keel-muted);
    }

    [data-theme="light"] .keel-auth-input {
        background: #ffffff;
    }

    .keel-auth-input:focus {
        outline: none;
        border-color: rgba(105, 230, 216, 0.52);
        box-shadow: 0 0 0 3px rgba(105, 230, 216, 0.14);
    }

    .keel-tab-wrap {
        border-bottom: 1px solid var(--keel-line);
    }

    .keel-tab-active {
        color: #ffffff;
        border-bottom-color: var(--keel-accent);
    }

    [data-theme="light"] .keel-tab-active {
        color: var(--keel-text);
    }

    .keel-tab-inactive {
        border-bottom-color: transparent;
        color: var(--keel-muted);
    }

    .keel-auth-btn {
        background: var(--keel-accent);
        color: #08101e;
        box-shadow: 0 10px 30px rgba(255, 107, 61, 0.34);
    }

    .keel-auth-btn:hover {
        background: #ff835f;
    }

    .keel-tab-base {
        border-bottom-width: 2px;
    }
</style>
</head>
<body class="min-h-screen antialiased">
    <?php $csrfToken = \Keel\Core\Csrf::token(); ?>
    <div class="relative isolate min-h-screen overflow-hidden px-4 py-10 sm:px-6 lg:px-8">
        <div class="keel-grid absolute inset-0 -z-10 opacity-60"></div>
        <div class="mx-auto flex min-h-screen w-full max-w-sm items-center justify-center py-6">
            <section class="card relative w-full rounded-3xl p-0 keel-auth-card">
                <div class="keel-auth-content">
                <div class="mb-6 flex items-center gap-3">
                    <img src="/images/brand/keel-icon-light.png" alt="Keel icon" class="logo-dark-mode h-10 w-10 object-contain" loading="eager" decoding="async">
                    <img src="/images/brand/keel-icon.png" alt="Keel icon" class="logo-light-mode h-10 w-10 object-contain" loading="eager" decoding="async">
                    <div>
                        <h1 class="text-3xl font-semibold tracking-[-0.03em] text-[var(--keel-text)]">Sign in</h1>
                        <p class="mt-1 text-sm text-[var(--keel-muted)]">No password needed.</p>
                    </div>
                    <button type="button" class="theme-toggle-button ml-auto" data-theme-toggle>
                        <span data-theme-toggle-icon aria-hidden="true"></span>
                    </button>
                </div>

                <?php if (!empty($_GET['error']) && $_GET['error'] === 'invalid_invite'): ?>
                <div class="alert alert-error mb-4 border-red-500/40 bg-red-500/10 px-3 py-2 text-red-200">That invite link is invalid, expired, or already used.</div>
                <?php endif; ?>

                <?php if ($authMethod === 'both'): ?>
                <div class="mb-6 keel-tab-wrap">
                    <div class="grid grid-cols-2 gap-4" data-tabs data-tabs-active-class="keel-tab-active" data-tabs-inactive-class="keel-tab-inactive">
                        <button type="button" id="tab-otp" data-tab-target="panel-otp" aria-selected="true" class="keel-tab-base px-1 pb-2 text-sm font-semibold text-left transition keel-tab-active">Code</button>
                        <button type="button" id="tab-magic" data-tab-target="panel-magic" aria-selected="false" class="keel-tab-base px-1 pb-2 text-sm font-semibold text-left transition keel-tab-inactive">Magic link</button>
                    </div>
                </div>
                <?php endif; ?>

                <?php if ($authMethod === 'otp' || $authMethod === 'both'): ?>
                <div id="panel-otp" data-tab-panel="panel-otp" class="space-y-4">
                    <div id="otp-step-email">
                        <label class="form-label text-[var(--keel-muted)]">Email</label>
                        <input type="email" id="otp-email" class="form-input keel-auth-input" placeholder="you@example.com">
                        <button type="button" id="otp-send" class="btn btn-primary btn-md mt-3 w-full keel-auth-btn">Send code</button>
                    </div>
                    <div id="otp-step-code" class="hidden">
                        <label class="form-label text-[var(--keel-muted)]">Enter the 6-digit code</label>
                        <input type="text" id="otp-code" maxlength="6" inputmode="numeric" class="form-input text-center tracking-widest keel-auth-input" placeholder="000000">
                        <button type="button" id="otp-verify" class="btn btn-primary btn-md mt-3 w-full keel-auth-btn">Verify</button>
                    </div>
                </div>
                <?php endif; ?>

                <?php if ($authMethod === 'magic_link' || $authMethod === 'both'): ?>
                <div id="panel-magic" data-tab-panel="panel-magic" class="space-y-4 <?= $authMethod === 'both' ? 'hidden' : '' ?>">
                    <label class="form-label text-[var(--keel-muted)]">Email</label>
                    <input type="email" id="magic-email" class="form-input keel-auth-input" placeholder="you@example.com">
                    <button type="button" id="magic-send" class="btn btn-primary btn-md mt-3 w-full keel-auth-btn">Send magic link</button>
                    <p id="magic-sent" class="mt-3 hidden text-sm text-emerald-300">Check your email for the sign-in link.</p>
                </div>
                <?php endif; ?>

                <p id="auth-error" class="mt-4 hidden text-sm text-red-300"></p>
                </div>
            </section>
        </div>
    </div>

    <script>
        const csrfToken = document.querySelector('meta[name="csrf-token"]')?.getAttribute('content') || '<?= htmlspecialchars($csrfToken, ENT_QUOTES, 'UTF-8') ?>';

        const showError = (msg) => {
            const el = document.getElementById('auth-error');
            el.textContent = msg;
            el.classList.remove('hidden');
        };

        <?php if ($authMethod === 'otp' || $authMethod === 'both'): ?>
        document.getElementById('otp-send').addEventListener('click', async () => {
            const email = document.getElementById('otp-email').value;
            const res = await fetch('/auth/otp/request', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json', 'X-CSRF-Token': csrfToken },
                body: JSON.stringify({ email }),
            });
            const data = await res.json();
            if (data.success) {
                document.getElementById('otp-step-email').classList.add('hidden');
                document.getElementById('otp-step-code').classList.remove('hidden');
            } else {
                showError(data.message || 'Something went wrong.');
            }
        });

        document.getElementById('otp-verify').addEventListener('click', async () => {
            const email = document.getElementById('otp-email').value;
            const code = document.getElementById('otp-code').value;
            const res = await fetch('/auth/otp/verify', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json', 'X-CSRF-Token': csrfToken },
                body: JSON.stringify({ email, code }),
            });
            const data = await res.json();
            if (data.success) {
                window.location.href = data.redirect || '/dashboard';
            } else {
                showError(data.message || 'Invalid code.');
            }
        });
        <?php endif; ?>

        <?php if ($authMethod === 'magic_link' || $authMethod === 'both'): ?>
        document.getElementById('magic-send').addEventListener('click', async () => {
            const email = document.getElementById('magic-email').value;
            const res = await fetch('/auth/magic/request', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json', 'X-CSRF-Token': csrfToken },
                body: JSON.stringify({ email }),
            });
            const data = await res.json();
            if (data.success) {
                document.getElementById('magic-sent').classList.remove('hidden');
            } else {
                showError(data.message || 'Something went wrong.');
            }
        });
        <?php endif; ?>
    </script>
</body>
</html>
