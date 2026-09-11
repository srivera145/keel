<?php
$hasSeedScript = $hasSeedScript ?? false;
$hasDockerCompose = $hasDockerCompose ?? false;
?>
<div class="space-y-6 text-sm leading-7 text-[var(--color-text-muted)]">
    <p>Follow this sequence on a fresh clone.</p>

    <section>
        <h2 class="mb-2 text-lg font-semibold text-[var(--color-text-strong)]">1. Install dependencies</h2>
        <pre class="overflow-x-auto rounded-xl bg-[var(--color-surface-muted)] p-4 text-xs text-[var(--color-text-strong)]"><code>composer install
npm install</code></pre>
    </section>

    <section>
        <h2 class="mb-2 text-lg font-semibold text-[var(--color-text-strong)]">2. Create your environment file</h2>
        <pre class="overflow-x-auto rounded-xl bg-[var(--color-surface-muted)] p-4 text-xs text-[var(--color-text-strong)]"><code>cp .env.example .env</code></pre>
        <p>At minimum, set <code>APP_URL</code>, <code>DB_HOST</code>, <code>DB_PORT</code>, <code>DB_DATABASE</code>, <code>DB_USERNAME</code>, and <code>DB_PASSWORD</code>.</p>
    </section>

    <section>
        <h2 class="mb-2 text-lg font-semibold text-[var(--color-text-strong)]">3. Run migrations</h2>
        <pre class="overflow-x-auto rounded-xl bg-[var(--color-surface-muted)] p-4 text-xs text-[var(--color-text-strong)]"><code>php database/migrate.php</code></pre>
        <p>This command creates the database when needed and applies pending SQL files from <code>database/migrations/</code>.</p>
    </section>

    <?php if ($hasSeedScript): ?>
    <section>
        <h2 class="mb-2 text-lg font-semibold text-[var(--color-text-strong)]">4. Optional dev/demo seed data</h2>
        <pre class="overflow-x-auto rounded-xl bg-[var(--color-surface-muted)] p-4 text-xs text-[var(--color-text-strong)]"><code>php database/seed.php</code></pre>
        <p>Use this only for local/demo environments.</p>
    </section>
    <?php endif; ?>

    <section>
        <h2 class="mb-2 text-lg font-semibold text-[var(--color-text-strong)]">Configure auth and feature toggles</h2>
        <pre class="overflow-x-auto rounded-xl bg-[var(--color-surface-muted)] p-4 text-xs text-[var(--color-text-strong)]"><code>AUTH_METHOD=both
MAIL_MAILER=smtp
# or
MAIL_MAILER=log

MULTI_TENANCY_ENABLED=false

STRIPE_SECRET_KEY=
STRIPE_PUBLISHABLE_KEY=
STRIPE_WEBHOOK_SECRET=
STRIPE_PRICE_PRO_MONTHLY=</code></pre>
        <p><code>AUTH_METHOD</code> supports <code>otp</code>, <code>magic_link</code>, and <code>both</code>.</p>
    </section>

    <section>
        <h2 class="mb-2 text-lg font-semibold text-[var(--color-text-strong)]">Start the frontend pipeline</h2>
        <pre class="overflow-x-auto rounded-xl bg-[var(--color-surface-muted)] p-4 text-xs text-[var(--color-text-strong)]"><code>npm run dev</code></pre>
    </section>

    <section>
        <h2 class="mb-2 text-lg font-semibold text-[var(--color-text-strong)]">XAMPP + keel.local setup</h2>
        <ol class="list-decimal space-y-1 pl-5">
            <li>Place the project at <code>C:\xampp\htdocs\keel</code>.</li>
            <li>Add <code>127.0.0.1 keel.local</code> to your hosts file.</li>
            <li>Add a vhost pointing <code>DocumentRoot</code> to <code>C:/xampp/htdocs/keel/public_html</code>.</li>
            <li>Ensure Apache loads <code>httpd-vhosts.conf</code> and restart Apache.</li>
            <li>Set <code>APP_URL=http://keel.local</code>.</li>
        </ol>
    </section>

    <?php if ($hasDockerCompose): ?>
    <section>
        <h2 class="mb-2 text-lg font-semibold text-[var(--color-text-strong)]">Optional Docker Compose path</h2>
        <pre class="overflow-x-auto rounded-xl bg-[var(--color-surface-muted)] p-4 text-xs text-[var(--color-text-strong)]"><code>docker compose up --build
docker compose exec app composer install
docker compose exec app npm install
docker compose exec app php database/migrate.php</code></pre>
        <p>Then open <code>http://localhost:8080</code>.</p>
    </section>
    <?php endif; ?>
</div>
