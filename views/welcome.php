<?php

use Keel\Core\Env;

$repoUrl = trim((string) Env::get('APP_REPO_URL', ''));
$repoHref = $repoUrl !== '' ? $repoUrl : 'https://github.com';
$repoLabel = $repoUrl !== '' ? 'GitHub' : 'GitHub (set APP_REPO_URL before publishing)';
$hasLicense = file_exists(dirname(__DIR__) . '/LICENSE');
$metaDescription = 'Keel is an open-source PHP 8.2 starter kit for SaaS apps with passwordless auth, Stripe billing, multi-tenancy, and docs.';

$softwareSchema = [
    '@context' => 'https://schema.org',
    '@type' => 'SoftwareSourceCode',
    'name' => 'Keel',
    'description' => 'Open-source PHP 8.2 starter kit for building SaaS applications with passwordless auth, Stripe billing, multi-tenancy, and docs.',
    'disambiguatingDescription' => 'An open-source PHP 8.2 starter kit for building SaaS applications. Not affiliated with the Kubernetes deployment automation tool named Keel or the London operations platform startup also named Keel.',
    'programmingLanguage' => 'PHP',
    'license' => 'https://opensource.org/licenses/MIT',
    'author' => [
        '@type' => 'Person',
        'name' => 'Santos Rivera',
    ],
];

if ($repoUrl !== '') {
    $softwareSchema['codeRepository'] = $repoUrl;
}

$faqItems = [
    [
        'question' => 'What is Keel?',
        'answer' => 'Keel is an open-source PHP 8.2 starter kit for building SaaS applications, with passwordless authentication, Stripe billing, optional multi-tenancy, and a plain-PHP MVC structure.',
    ],
    [
        'question' => 'Is Keel free, and what license does it use?',
        'answer' => 'Yes. Keel is open source under the MIT License.',
    ],
    [
        'question' => 'Does Keel require Laravel or another framework?',
        'answer' => 'No. Keel uses a custom lightweight MVC foundation and does not require Laravel or another PHP framework.',
    ],
    [
        'question' => 'Is this related to the Kubernetes tool or the London startup also called Keel?',
        'answer' => 'No. This Keel project is unrelated. It is an independent open-source PHP starter kit for SaaS applications.',
    ],
    [
        'question' => 'How are public discovery files maintained?',
        'answer' => 'Public routes are marked in route registration with the sitemap option, and sitemap.xml, robots.txt, and llms.txt are generated from that route metadata and shared docs registry at request time.',
    ],
];

$faqSchema = [
    '@context' => 'https://schema.org',
    '@type' => 'FAQPage',
    'mainEntity' => array_map(static function (array $item): array {
        return [
            '@type' => 'Question',
            'name' => $item['question'],
            'acceptedAnswer' => [
                '@type' => 'Answer',
                'text' => $item['answer'],
            ],
        ];
    }, $faqItems),
];

$heroCommands = [
    'composer install',
    'npm install',
    'cp .env.example .env',
    'php database/migrate.php',
    'npm run dev',
];

$featureGroups = [
    [
        'group' => 'Auth & Security',
        'items' => [
            'OTP + Magic Link flows with no passwords, configurable per project via AUTH_METHOD.',
            'Mail delivery is built in via PHPMailer: use SMTP for real sends, or MAIL_MAILER=log for zero-config local development (codes and magic links in storage/logs/mail.log).',
            'CSRF protection, rate limiting, and dedicated 404/500 error pages ship out of the box.',
        ],
    ],
    [
        'group' => 'Billing',
        'items' => [
            'Stripe Checkout + Billing Portal with subscription state synced by webhooks.',
            'Keel never handles raw card data directly.',
        ],
    ],
    [
        'group' => 'Data & Files',
        'items' => [
            'PDO + MySQL with explicit queries and no ORM abstraction layer.',
            'File uploads validate real MIME types server-side, with local disk today and a swappable storage abstraction for later.',
        ],
    ],
    [
        'group' => 'AI',
        'items' => [
            'A thin Claude API wrapper supports text, image-assisted, and JSON-structured completions.',
        ],
    ],
    [
        'group' => 'Teams',
        'items' => [
            'Opt-in multi-tenancy with organizations, roles, invites, and org-level access control.',
            'Includes both org-admin settings and a platform super-admin area.',
        ],
    ],
    [
        'group' => 'Automation & Data',
        'items' => [
            'A database-backed background job queue runs from database/queue-work.php, with retries and failed-job capture.',
            'Built-in activity logging records auth, billing, file, and organization events for audit trails.',
            'API tokens support programmatic Bearer access alongside normal session auth.',
        ],
    ],
    [
        'group' => 'Developer Experience',
        'items' => [
            'Tailwind + Vite with automatic dev/build asset switching in shared templates.',
            'A hand-authored component layer (buttons, cards, tables, modals) is built on re-themeable design tokens.',
            'Light/dark theme toggle includes persisted user preference support.',
            'Lightweight PWA support is included via a manifest route plus app icons/favicons.',
            'Self-maintaining sitemap.xml, robots.txt, and llms.txt: mark a route public once where it is registered, and it stays reflected everywhere with nothing to regenerate.',
            'PHPUnit scaffold + GitHub Actions CI included for testable pull requests.',
            'Docker Compose is available as an alternative for contributors not using XAMPP.',
        ],
    ],
];

$quickstartSteps = [
    [
        'title' => 'Install dependencies',
        'lines' => ['composer install', 'npm install'],
    ],
    [
        'title' => 'Create your environment file',
        'lines' => ['cp .env.example .env'],
    ],
    [
        'title' => 'Run migrations',
        'lines' => ['php database/migrate.php'],
    ],
    [
        'title' => 'Set core auth + mail toggles',
        'lines' => ['AUTH_METHOD=both', 'MAIL_MAILER=smtp', 'MAIL_MAILER=log'],
    ],
    [
        'title' => 'Enable optional billing + org features',
        'lines' => [
            'MULTI_TENANCY_ENABLED=false',
            'STRIPE_SECRET_KEY=',
            'STRIPE_PUBLISHABLE_KEY=',
            'STRIPE_WEBHOOK_SECRET=',
            'STRIPE_PRICE_PRO_MONTHLY=',
        ],
    ],
];
?>
<!DOCTYPE html>
<html lang="en"<?= \Keel\Core\Theme::htmlAttribute() ?>>
<head>
<?php require __DIR__ . '/partials/head.php'; ?>
<style>
    :root {
        --keel-night: #07111f;
        --keel-panel: rgba(7, 17, 31, 0.78);
        --keel-panel-strong: #0b1830;
        --keel-line: rgba(148, 163, 184, 0.18);
        --keel-text: #e5eefb;
        --keel-muted: #97a9c9;
        --keel-accent: #ff6b3d;
        --keel-accent-soft: rgba(255, 107, 61, 0.16);
        --keel-cyan: #69e6d8;
    }

    [data-theme="light"] {
        --keel-night: #f8fafc;
        --keel-panel: rgba(255, 255, 255, 0.94);
        --keel-panel-strong: #ffffff;
        --keel-line: rgba(15, 23, 42, 0.14);
        --keel-text: #0f172a;
        --keel-muted: #475569;
        --keel-accent-soft: rgba(255, 107, 61, 0.14);
    }

    html {
        scroll-behavior: smooth;
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

    .keel-shell {
        background: linear-gradient(180deg, rgba(11, 24, 48, 0.96), rgba(7, 17, 31, 0.92));
        border: 1px solid rgba(148, 163, 184, 0.18);
        box-shadow: 0 22px 80px rgba(0, 0, 0, 0.34);
    }

    [data-theme="light"] .keel-shell {
        background: linear-gradient(180deg, rgba(255, 255, 255, 0.98), rgba(248, 250, 252, 0.96));
        border-color: rgba(15, 23, 42, 0.12);
        box-shadow: 0 20px 48px rgba(15, 23, 42, 0.12);
    }

    .keel-card {
        background: rgba(11, 24, 48, 0.72);
        border: 1px solid var(--keel-line);
        backdrop-filter: blur(12px);
    }

    [data-theme="light"] .keel-card {
        background: rgba(255, 255, 255, 0.9);
    }

    .keel-text-strong {
        color: #f8fafc;
    }

    [data-theme="light"] .keel-text-strong {
        color: #0f172a;
    }

    .keel-text-soft {
        color: #94a3b8;
    }

    [data-theme="light"] .keel-text-soft {
        color: #64748b;
    }

    .keel-nav-link {
        color: #e2e8f0;
    }

    .keel-nav-link:hover {
        color: #ffffff;
    }

    [data-theme="light"] .keel-nav-link {
        color: #334155;
    }

    [data-theme="light"] .keel-nav-link:hover {
        color: #0f172a;
    }

    .keel-subtle-border {
        border-color: rgba(148, 163, 184, 0.18);
    }

    [data-theme="light"] .keel-subtle-border {
        border-color: rgba(15, 23, 42, 0.14);
    }

    .keel-subtle-bg {
        background-color: rgba(255, 255, 255, 0.05);
    }

    [data-theme="light"] .keel-subtle-bg {
        background-color: rgba(15, 23, 42, 0.04);
    }

    .keel-code-surface {
        background-color: #050b16;
        color: #e2e8f0;
    }

    .keel-code-meta {
        color: #64748b;
    }

    .keel-cyan-label {
        color: var(--keel-cyan);
    }

    [data-theme="light"] .keel-cyan-label {
        color: #0f766e;
    }

    .keel-wordmark {
        letter-spacing: 0.24em;
    }

    .keel-ring:focus-visible {
        outline: 2px solid var(--keel-cyan);
        outline-offset: 3px;
    }
    @media (prefers-reduced-motion: reduce) {
        html {
            scroll-behavior: auto;
        }
    }
</style>
<script type="application/ld+json">
<?= json_encode($softwareSchema, JSON_UNESCAPED_SLASHES | JSON_UNESCAPED_UNICODE | JSON_PRETTY_PRINT) ?>
</script>
<script type="application/ld+json">
<?= json_encode($faqSchema, JSON_UNESCAPED_SLASHES | JSON_UNESCAPED_UNICODE | JSON_PRETTY_PRINT) ?>
</script>
</head>
<body class="min-h-screen antialiased">
    <a href="#main-content" class="keel-ring sr-only focus:not-sr-only focus:fixed focus:left-4 focus:top-4 focus:z-[100] focus:rounded-md focus:bg-white focus:px-4 focus:py-2 focus:text-slate-950">Skip to content</a>

    <div class="relative isolate overflow-hidden">
        <div class="keel-grid absolute inset-0 -z-10 opacity-60"></div>

        <header class="mx-auto max-w-7xl px-4 pb-6 pt-5 sm:px-6 lg:px-8">
            <nav class="keel-card rounded-full px-4 py-3 sm:px-6" aria-label="Primary">
                <div class="flex items-center justify-between gap-4">
                    <a href="/" class="keel-ring keel-text-strong inline-flex items-center gap-3 rounded-full text-sm font-semibold uppercase tracking-[0.3em]">
                        <img src="/images/brand/keel-light.png" alt="Keel logo" class="logo-dark-mode h-10 object-contain" loading="eager" decoding="async">
                        <img src="/images/brand/keel.png" alt="Keel logo" class="logo-light-mode h-10 object-contain" loading="eager" decoding="async">
                    </a>

                    <button type="button" id="mobile-menu-button" class="keel-ring keel-nav-link keel-subtle-border inline-flex items-center justify-center rounded-full border px-3 py-2 text-sm md:hidden" aria-expanded="false" aria-controls="mobile-menu">
                        Menu
                    </button>

                    <div class="hidden items-center gap-3 md:flex">
                        <button type="button" class="theme-toggle-button keel-ring" data-theme-toggle>
                            <span data-theme-toggle-icon aria-hidden="true"></span>
                        </button>
                        <a href="/docs" class="keel-ring keel-nav-link keel-subtle-border rounded-full border px-4 py-2 text-sm transition hover:border-[var(--keel-cyan)]">Docs</a>
                        <a href="<?= htmlspecialchars($repoHref) ?>" class="keel-ring keel-nav-link keel-subtle-border rounded-full border px-4 py-2 text-sm transition hover:border-[var(--keel-cyan)]" target="_blank" rel="noreferrer"><?= htmlspecialchars($repoLabel) ?></a>
                        <a href="/login" class="keel-ring rounded-full bg-[var(--keel-accent)] px-4 py-2 text-sm font-semibold text-slate-950 transition hover:bg-[#ff835f]">Sign in</a>
                    </div>
                </div>

                <div id="mobile-menu" class="keel-subtle-border mt-4 hidden border-t pt-4 md:hidden">
                    <div class="flex flex-col gap-3">
                        <button type="button" class="theme-toggle-button keel-ring" data-theme-toggle>
                            <span data-theme-toggle-icon aria-hidden="true"></span>
                        </button>
                        <a href="/docs" class="keel-ring keel-nav-link keel-subtle-border rounded-2xl border px-4 py-3 text-sm">Docs</a>
                        <a href="<?= htmlspecialchars($repoHref) ?>" class="keel-ring keel-nav-link keel-subtle-border rounded-2xl border px-4 py-3 text-sm" target="_blank" rel="noreferrer"><?= htmlspecialchars($repoLabel) ?></a>
                        <a href="/login" class="keel-ring rounded-2xl bg-[var(--keel-accent)] px-4 py-3 text-sm font-semibold text-slate-950">Sign in</a>
                    </div>
                </div>
            </nav>
        </header>

        <main id="main-content">
            <section class="mx-auto max-w-7xl px-4 pb-20 pt-8 sm:px-6 lg:px-8 lg:pb-28 lg:pt-12">
                <div class="grid gap-12 lg:grid-cols-[minmax(0,1.05fr)_minmax(420px,0.95fr)] lg:items-center">
                    <div class="max-w-3xl">
                        <p class="keel-subtle-bg keel-cyan-label mb-6 inline-flex items-center gap-2 rounded-full border border-[var(--keel-line)] px-4 py-2 text-xs font-medium uppercase tracking-[0.28em]">
                            <?= htmlspecialchars('PHP 8.2 starter kit for shipping real products') ?>
                        </p>
                        <h1 class="keel-text-strong max-w-4xl text-5xl font-semibold tracking-[-0.05em] sm:text-6xl lg:text-7xl">
                            <?= htmlspecialchars('Keel is an open-source PHP 8.2 starter kit for building SaaS applications.') ?>
                        </h1>
                        <p class="mt-6 max-w-2xl text-lg leading-8 text-[var(--keel-muted)] sm:text-xl">
                            It ships with passwordless authentication, Stripe billing, files + AI helpers, optional multi-tenancy, queue/audit automation, and developer tooling while keeping the codebase explicit and maintainable.
                        </p>
                        <div class="mt-10 flex flex-col gap-4 sm:flex-row sm:items-center">
                            <a href="#quickstart" class="keel-ring inline-flex items-center justify-center rounded-full bg-[var(--keel-accent)] px-6 py-3 text-sm font-semibold text-slate-950 transition hover:bg-[#ff835f]">Get Started</a>
                            <a href="<?= htmlspecialchars($repoHref) ?>" class="keel-ring keel-text-strong keel-subtle-border inline-flex items-center justify-center rounded-full border px-6 py-3 text-sm font-semibold transition hover:border-[var(--keel-cyan)] hover:text-[var(--keel-cyan)]" target="_blank" rel="noreferrer">View on GitHub</a>
                        </div>
                        <dl class="mt-12 grid gap-4 text-sm text-[var(--keel-muted)] sm:grid-cols-3">
                            <div class="keel-subtle-bg keel-subtle-border rounded-3xl border px-5 py-4">
                                <dt class="keel-text-soft text-xs uppercase tracking-[0.2em]">Auth</dt>
                                <dd class="keel-text-strong mt-2 text-base font-medium">OTP + magic link</dd>
                            </div>
                            <div class="keel-subtle-bg keel-subtle-border rounded-3xl border px-5 py-4">
                                <dt class="keel-text-soft text-xs uppercase tracking-[0.2em]">Data</dt>
                                <dd class="keel-text-strong mt-2 text-base font-medium">PDO + MySQL</dd>
                            </div>
                            <div class="keel-subtle-bg keel-subtle-border rounded-3xl border px-5 py-4">
                                <dt class="keel-text-soft text-xs uppercase tracking-[0.2em]">Assets</dt>
                                <dd class="keel-text-strong mt-2 text-base font-medium">Tailwind + Vite</dd>
                            </div>
                        </dl>
                    </div>

                    <div class="keel-shell relative rounded-[2rem] p-5 sm:p-7">
                        <div class="keel-subtle-border flex items-center justify-between gap-4 border-b pb-4">
                            <div class="flex items-center gap-2" aria-hidden="true">
                                <span class="h-3 w-3 rounded-full bg-[#ff6b3d]"></span>
                                <span class="h-3 w-3 rounded-full bg-[#ffd166]"></span>
                                <span class="h-3 w-3 rounded-full bg-[#69e6d8]"></span>
                            </div>
                            <button type="button" class="keel-ring keel-text-soft keel-subtle-border rounded-full border px-3 py-1.5 text-xs font-medium uppercase tracking-[0.24em] transition hover:border-[var(--keel-cyan)] hover:text-[var(--keel-text)]" data-copy-target="hero-install">Copy</button>
                        </div>
                        <div class="keel-code-surface keel-subtle-border mt-6 overflow-hidden rounded-[1.5rem] border">
                            <div class="keel-code-meta keel-subtle-border flex items-center justify-between border-b px-5 py-3 text-xs uppercase tracking-[0.24em]">
                                <span>Quick install</span>
                                <span>README-backed</span>
                            </div>
                            <pre id="hero-install" class="overflow-x-auto px-5 py-6 text-sm leading-7"><code><?php foreach ($heroCommands as $command): ?>$ <?= htmlspecialchars($command) . "\n" ?><?php endforeach; ?></code></pre>
                        </div>
                        <p class="mt-5 text-sm leading-7 text-[var(--keel-muted)]">
                            The commands above are the same ones documented in the setup instructions. No generated CLI, no hidden scaffolding step.
                        </p>
                    </div>
                </div>
            </section>

            <section class="mx-auto max-w-7xl px-4 py-8 sm:px-6 lg:px-8">
                <div class="flex flex-col gap-4 sm:flex-row sm:items-end sm:justify-between">
                    <div>
                        <p class="keel-cyan-label text-sm font-medium uppercase tracking-[0.28em]">Features</p>
                        <h2 class="keel-text-strong mt-3 text-3xl font-semibold tracking-[-0.04em] sm:text-4xl">Everything on this page exists in the repository today.</h2>
                    </div>
                        <p class="max-w-2xl text-sm leading-7 text-[var(--keel-muted)]">Keel groups practical capabilities into clear building blocks so you can ship product features quickly without giving up ownership of the stack.</p>
                </div>

                <div class="mt-10 grid gap-5 md:grid-cols-2 xl:grid-cols-3">
                    <?php foreach ($featureGroups as $featureGroup): ?>
                    <article class="keel-card rounded-[1.75rem] p-6">
                        <div class="mb-5 inline-flex h-11 w-11 items-center justify-center rounded-2xl bg-[var(--keel-accent-soft)] text-sm font-semibold text-[var(--keel-accent)]">0<?= (string) (array_search($featureGroup, $featureGroups, true) + 1) ?></div>
                        <h3 class="keel-text-strong text-xl font-semibold"><?= htmlspecialchars($featureGroup['group']) ?></h3>
                        <ul class="mt-3 space-y-3 text-sm leading-7 text-[var(--keel-muted)]">
                            <?php foreach ($featureGroup['items'] as $item): ?>
                            <li><?= htmlspecialchars($item) ?></li>
                            <?php endforeach; ?>
                        </ul>
                    </article>
                    <?php endforeach; ?>
                </div>
            </section>

            <section id="quickstart" class="mx-auto max-w-7xl px-4 py-20 sm:px-6 lg:px-8">
                <div class="grid gap-10 lg:grid-cols-[minmax(0,0.8fr)_minmax(0,1.2fr)] lg:items-start">
                    <div>
                        <p class="keel-cyan-label text-sm font-medium uppercase tracking-[0.28em]">Quickstart</p>
                        <h2 class="keel-text-strong mt-3 text-3xl font-semibold tracking-[-0.04em] sm:text-4xl">Get from clone to running app with the documented flow.</h2>
                        <p class="mt-5 max-w-xl text-base leading-8 text-[var(--keel-muted)]">These steps mirror the README setup flow: install dependencies, create your environment file, run <code>php database/migrate.php</code>, then set auth and optional feature flags.</p>
                    </div>

                    <ol class="space-y-4">
                        <?php foreach ($quickstartSteps as $index => $step): ?>
                        <li class="keel-card rounded-[1.75rem] p-6">
                            <div class="flex flex-col gap-4 sm:flex-row sm:items-start sm:justify-between">
                                <div>
                                    <p class="keel-text-soft text-xs uppercase tracking-[0.22em]">Step <?= $index + 1 ?></p>
                                    <h3 class="keel-text-strong mt-2 text-xl font-semibold"><?= htmlspecialchars($step['title']) ?></h3>
                                </div>
                                <div class="keel-code-surface keel-subtle-border rounded-2xl border px-4 py-3 text-left font-mono text-sm sm:min-w-[19rem]">
                                    <?php foreach ($step['lines'] as $line): ?>
                                    <div><?= htmlspecialchars($line) ?></div>
                                    <?php endforeach; ?>
                                </div>
                            </div>
                        </li>
                        <?php endforeach; ?>
                    </ol>
                </div>
            </section>

            <section class="mx-auto max-w-7xl px-4 pb-20 sm:px-6 lg:px-8">
                <div class="keel-card rounded-[2rem] px-6 py-8 sm:px-8 sm:py-10 lg:px-10">
                    <p class="keel-cyan-label text-sm font-medium uppercase tracking-[0.28em]">Why Keel</p>
                    <h2 class="keel-text-strong mt-3 text-3xl font-semibold tracking-[-0.04em] sm:text-4xl">Convention over configuration, without hidden magic.</h2>
                    <div class="mt-6 grid gap-6 lg:grid-cols-3">
                        <p class="text-base leading-8 text-[var(--keel-muted)]">Keel is opinionated where repetition wastes time: routing, controller structure, auth flow, mailer wiring, and asset handling already have a sensible shape.</p>
                        <p class="text-base leading-8 text-[var(--keel-muted)]">It stays honest by keeping the machinery small. When you open the code six months later, the path from request to response is still obvious and local.</p>
                        <p class="text-base leading-8 text-[var(--keel-muted)]">That balance is the point: enough structure to move quickly, not so much abstraction that the foundation starts steering the product.</p>
                    </div>
                </div>
            </section>

            <section class="mx-auto max-w-7xl px-4 pb-20 sm:px-6 lg:px-8">
                <div class="keel-card rounded-[2rem] px-6 py-8 sm:px-8 sm:py-10 lg:px-10">
                    <p class="keel-cyan-label text-sm font-medium uppercase tracking-[0.28em]">FAQ</p>
                    <h2 class="keel-text-strong mt-3 text-3xl font-semibold tracking-[-0.04em] sm:text-4xl">Quick answers for developers and crawlers.</h2>
                    <div class="mt-6 space-y-6">
                        <?php foreach ($faqItems as $faq): ?>
                        <article>
                            <h3 class="keel-text-strong text-xl font-semibold"><?= htmlspecialchars($faq['question']) ?></h3>
                            <p class="mt-2 text-base leading-8 text-[var(--keel-muted)]"><?= htmlspecialchars($faq['answer']) ?></p>
                        </article>
                        <?php endforeach; ?>
                    </div>
                </div>
            </section>
        </main>

        <footer class="keel-subtle-border border-t">
            <div class="mx-auto flex max-w-7xl flex-col gap-4 px-4 py-8 text-sm text-[var(--keel-muted)] sm:px-6 lg:flex-row lg:items-center lg:justify-between lg:px-8">
                <p>Built by Santos Rivera.</p>
                <div class="flex flex-col gap-3 sm:flex-row sm:items-center sm:gap-6">
                    <a href="<?= htmlspecialchars($repoHref) ?>" class="keel-ring keel-nav-link transition" target="_blank" rel="noreferrer"><?= htmlspecialchars($repoLabel) ?></a>
                    <p><?= $hasLicense ? 'MIT licensed' : 'License: ' . (Env::get('APP_LICENSE', 'TODO') === 'TODO' ? 'TODO' : htmlspecialchars((string) Env::get('APP_LICENSE'))) ?></p>
                </div>
            </div>
        </footer>
    </div>

    <script>
        const copyTextToClipboard = async (text) => {
            if (navigator.clipboard && window.isSecureContext) {
                await navigator.clipboard.writeText(text);
                return;
            }

            const textarea = document.createElement('textarea');
            textarea.value = text;
            textarea.setAttribute('readonly', '');
            textarea.style.position = 'fixed';
            textarea.style.left = '-9999px';
            document.body.appendChild(textarea);
            textarea.focus();
            textarea.select();

            const copied = document.execCommand('copy');
            document.body.removeChild(textarea);

            if (!copied) {
                throw new Error('Clipboard copy command failed.');
            }
        };

        const mobileMenuButton = document.getElementById('mobile-menu-button');
        const mobileMenu = document.getElementById('mobile-menu');

        if (mobileMenuButton && mobileMenu) {
            mobileMenuButton.addEventListener('click', () => {
                const isOpen = mobileMenuButton.getAttribute('aria-expanded') === 'true';
                mobileMenuButton.setAttribute('aria-expanded', isOpen ? 'false' : 'true');
                mobileMenu.classList.toggle('hidden', isOpen);
            });
        }

        document.querySelectorAll('[data-copy-target]').forEach((button) => {
            button.addEventListener('click', async () => {
                const targetId = button.getAttribute('data-copy-target');
                const target = targetId ? document.getElementById(targetId) : null;

                if (!target) {
                    return;
                }

                const originalText = button.textContent;

                try {
                    await copyTextToClipboard(target.innerText.trim());
                    button.textContent = 'Copied';
                } catch (error) {
                    button.textContent = 'Copy failed';
                }

                window.setTimeout(() => {
                    button.textContent = originalText;
                }, 1500);
            });
        });
    </script>
</body>
</html>