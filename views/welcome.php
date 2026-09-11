<?php

use EchoDial\Deck\Deck;
use Keel\Core\Env;
use Keel\Core\Theme;

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
    'cp .env.example .env',
    'php database/migrate.php',
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
            'Deck CSS with no build step: one stylesheet, published by Composer, re-themed from a single hue.',
            'A hand-authored component layer (buttons, cards, tables, modals) is built on re-themeable design tokens.',
            'Light/dark theme toggle includes persisted user preference support.',
            'Lightweight PWA support is included via a manifest route plus app icons/favicons.',
            'Self-maintaining sitemap.xml, robots.txt, and llms.txt: mark a route public once where it is registered, and it stays reflected everywhere with nothing to regenerate.',
            'PHPUnit scaffold + GitHub Actions CI included for testable pull requests.',
            'Docker Compose is available as an alternative for contributors not using XAMPP.',
        ],
    ],
];

// One icon per capability group, from Deck's sprite.
$featureIcons = [
    'Auth & Security' => 'shield',
    'Billing' => 'credit-card',
    'Data & Files' => 'folder',
    'AI' => 'sparkle',
    'Teams' => 'users',
    'Automation & Data' => 'refresh',
    'Developer Experience' => 'settings',
];

$quickstartSteps = [
    [
        'title' => 'Install dependencies',
        'lines' => ['composer install'],
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

$deckExtras = true; // the copy button on the install block
?>
<!DOCTYPE html>
<html <?= Deck::htmlAttributes(lang: 'en') ?> <?= Deck::theme(mode: Theme::serverPreference()) ?>>
<head>
<?php require __DIR__ . '/partials/head.php'; ?>
<script type="application/ld+json">
<?= json_encode($softwareSchema, JSON_UNESCAPED_SLASHES | JSON_UNESCAPED_UNICODE | JSON_PRETTY_PRINT) ?>
</script>
<script type="application/ld+json">
<?= json_encode($faqSchema, JSON_UNESCAPED_SLASHES | JSON_UNESCAPED_UNICODE | JSON_PRETTY_PRINT) ?>
</script>
</head>
<body class="g-mesh">
    <span id="top" tabindex="-1"></span>
    <a class="skip-link" href="#main-content">Skip to content</a>

    <?php require __DIR__ . '/partials/site-nav.php'; ?>

    <main id="main-content">
        <section class="container section">
            <div class="grid grid-wide items-start">
                <div class="stack stack-6">
                    <div class="stack stack-4">
                        <span class="badge badge-brand"><?= htmlspecialchars('PHP 8.2 starter kit for shipping real products') ?></span>
                        <h1><?= htmlspecialchars('Keel is an open-source PHP 8.2 starter kit for building SaaS applications.') ?></h1>
                        <p class="lede">
                            It ships with passwordless authentication, Stripe billing, files + AI helpers, optional multi-tenancy, queue/audit automation, and developer tooling while keeping the codebase explicit and maintainable.
                        </p>
                    </div>
                    <div class="cluster">
                        <a class="btn btn-primary btn-lg" href="#quickstart">Get Started</a>
                        <a class="btn btn-lg" href="<?= htmlspecialchars($repoHref) ?>" target="_blank" rel="noreferrer">View on GitHub</a>
                    </div>
                    <dl class="grid" style="--min: 9rem">
                        <div class="stat">
                            <dt class="stat-label">Auth</dt>
                            <dd class="fw-semi">OTP + magic link</dd>
                        </div>
                        <div class="stat">
                            <dt class="stat-label">Data</dt>
                            <dd class="fw-semi">PDO + MySQL</dd>
                        </div>
                        <div class="stat">
                            <dt class="stat-label">Assets</dt>
                            <dd class="fw-semi"><a href="https://get-deck.dev" target="_blank" rel="noreferrer">Deck</a>, no build step</dd>
                        </div>
                    </dl>
                </div>

                <div class="card">
                    <div class="card-header">
                        <span class="text-xs uppercase text-muted">Quick install</span>
                        <button type="button" class="btn btn-sm btn-ghost push" data-deck-copy="#hero-install">
                            <?= Deck::icon('copy', 'icon icon-sm') ?>
                            Copy
                        </button>
                    </div>
                    <pre id="hero-install"><code><?php foreach ($heroCommands as $command): ?>$ <?= htmlspecialchars($command) . "\n" ?><?php endforeach; ?></code></pre>
                    <div class="card-body">
                        <p class="text-sm text-muted">
                            The commands above are the same ones documented in the setup instructions. No generated CLI, no hidden scaffolding step.
                        </p>
                    </div>
                </div>
            </div>
        </section>

        <section class="container section stack stack-8">
            <div class="bar wrap">
                <div class="stack stack-2">
                    <p class="text-sm uppercase text-brand">Features</p>
                    <h2>Everything on this page exists in the repository today.</h2>
                </div>
                <p class="lede push">Keel groups practical capabilities into clear building blocks so you can ship product features quickly without giving up ownership of the stack.</p>
            </div>

            <div class="grid" style="--min: 20rem">
                <?php foreach ($featureGroups as $featureGroup): ?>
                <article class="card">
                    <div class="card-body">
                        <span class="icon-tile"><?= Deck::icon($featureIcons[$featureGroup['group']] ?? 'check-circle') ?></span>
                        <h3 class="card-title"><?= htmlspecialchars($featureGroup['group']) ?></h3>
                        <ul class="stack stack-2 text-sm text-muted">
                            <?php foreach ($featureGroup['items'] as $item): ?>
                            <li><?= htmlspecialchars($item) ?></li>
                            <?php endforeach; ?>
                        </ul>
                    </div>
                </article>
                <?php endforeach; ?>
            </div>
        </section>

        <section id="quickstart" class="container section stack stack-8">
            <div class="stack stack-2">
                <p class="text-sm uppercase text-brand">Quickstart</p>
                <h2>Get from clone to running app with the documented flow.</h2>
                <p class="lede">These steps mirror the README setup flow: install dependencies, create your environment file, run <code>php database/migrate.php</code>, then set auth and optional feature flags.</p>
            </div>

            <ol class="stack stack-4">
                <?php foreach ($quickstartSteps as $index => $step): ?>
                <li class="card">
                    <div class="card-body">
                        <div class="bar wrap">
                            <span class="badge badge-solid">Step <?= $index + 1 ?></span>
                            <h3 class="card-title"><?= htmlspecialchars($step['title']) ?></h3>
                        </div>
                        <pre><code><?php foreach ($step['lines'] as $line): ?><?= htmlspecialchars($line) . "\n" ?><?php endforeach; ?></code></pre>
                    </div>
                </li>
                <?php endforeach; ?>
            </ol>
        </section>

        <section class="container section">
            <div class="card">
                <div class="card-body stack stack-4">
                    <div class="stack stack-2">
                        <p class="text-sm uppercase text-brand">Why Keel</p>
                        <h2>Convention over configuration, without hidden magic.</h2>
                    </div>
                    <div class="grid" style="--min: 16rem">
                        <p class="text-muted">Keel is opinionated where repetition wastes time: routing, controller structure, auth flow, mailer wiring, and asset handling already have a sensible shape.</p>
                        <p class="text-muted">It stays honest by keeping the machinery small. When you open the code six months later, the path from request to response is still obvious and local.</p>
                        <p class="text-muted">That balance is the point: enough structure to move quickly, not so much abstraction that the foundation starts steering the product.</p>
                    </div>
                    <p class="text-muted">That applies to the interface too. Every screen in Keel — this page, the docs, the sign-in flow, the billing and admin tables — is built in <a href="https://get-deck.dev" target="_blank" rel="noreferrer">Deck</a>, a CSS framework installed by Composer. There is no npm, no bundler and no build step, and a tenant's brand colour is one inline style on the <code>&lt;html&gt;</code> tag.</p>
                </div>
            </div>
        </section>

        <section class="container section stack stack-6">
            <div class="stack stack-2">
                <p class="text-sm uppercase text-brand">FAQ</p>
                <h2>Quick answers for developers and crawlers.</h2>
            </div>
            <div class="accordion">
                <?php foreach ($faqItems as $faq): ?>
                <details name="keel-faq">
                    <summary><?= htmlspecialchars($faq['question']) ?></summary>
                    <div class="accordion-body"><?= htmlspecialchars($faq['answer']) ?></div>
                </details>
                <?php endforeach; ?>
            </div>
        </section>
    </main>

    <footer class="footer">
        <div class="container cluster cluster-between">
            <p>Built by Santos Rivera.</p>
            <div class="cluster cluster-tight">
                <a href="<?= htmlspecialchars($repoHref) ?>" target="_blank" rel="noreferrer"><?= htmlspecialchars($repoLabel) ?></a>
                <span><?= $hasLicense ? 'MIT licensed' : 'License: ' . (Env::get('APP_LICENSE', 'TODO') === 'TODO' ? 'TODO' : htmlspecialchars((string) Env::get('APP_LICENSE'))) ?></span>
            </div>
        </div>
    </footer>

    <?php require __DIR__ . '/partials/back-to-top.php'; ?>
</body>
</html>
