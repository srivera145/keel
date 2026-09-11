<?php
$hasSeedScript = $hasSeedScript ?? false;
$hasDockerCompose = $hasDockerCompose ?? false;
?>
<div class="prose">
    <p>Follow this sequence on a fresh clone.</p>

    <h2>1. Install dependencies</h2>
    <pre><code>composer install</code></pre>
    <p>That is the whole front-end setup too: Composer publishes Deck's stylesheet, icon sprite and scripts into <code>public_html/deck/</code>. There is no npm install and no build step.</p>

    <h2>2. Create your environment file</h2>
    <pre><code>cp .env.example .env</code></pre>
    <p>At minimum, set <code>APP_URL</code>, <code>DB_HOST</code>, <code>DB_PORT</code>, <code>DB_DATABASE</code>, <code>DB_USERNAME</code>, and <code>DB_PASSWORD</code>.</p>

    <h2>3. Run migrations</h2>
    <pre><code>php database/migrate.php</code></pre>
    <p>This command creates the database when needed and applies pending SQL files from <code>database/migrations/</code>.</p>

    <?php if ($hasSeedScript): ?>
    <h2>4. Optional dev/demo seed data</h2>
    <pre><code>php database/seed.php</code></pre>
    <p>Use this only for local/demo environments.</p>
    <?php endif; ?>

    <h2>Configure auth and feature toggles</h2>
    <pre><code>AUTH_METHOD=both
MAIL_MAILER=smtp
# or
MAIL_MAILER=log

MULTI_TENANCY_ENABLED=false

STRIPE_SECRET_KEY=
STRIPE_PUBLISHABLE_KEY=
STRIPE_WEBHOOK_SECRET=
STRIPE_PRICE_PRO_MONTHLY=</code></pre>
    <p><code>AUTH_METHOD</code> supports <code>otp</code>, <code>magic_link</code>, and <code>both</code>.</p>

    <h2>XAMPP + keel.local setup</h2>
    <ol>
        <li>Place the project at <code>C:\xampp\htdocs\keel</code>.</li>
        <li>Add <code>127.0.0.1 keel.local</code> to your hosts file.</li>
        <li>Add a vhost pointing <code>DocumentRoot</code> to <code>C:/xampp/htdocs/keel/public_html</code>.</li>
        <li>Ensure Apache loads <code>httpd-vhosts.conf</code> and restart Apache.</li>
        <li>Set <code>APP_URL=http://keel.local</code>.</li>
    </ol>

    <?php if ($hasDockerCompose): ?>
    <h2>Optional Docker Compose path</h2>
    <pre><code>docker compose up --build
docker compose exec app composer install
docker compose exec app php database/migrate.php</code></pre>
    <p>Then open <code>http://localhost:8080</code>.</p>
    <?php endif; ?>
</div>
