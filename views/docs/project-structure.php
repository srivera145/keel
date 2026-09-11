<div class="prose">
    <p>Keel keeps framework internals separate from app code while keeping views plain PHP.</p>

    <pre><code>public_html/
  index.php
  css/keel.css
  js/keel.js
  deck/
  uploads/
src/
  Core/
  App/
    Controllers/
    Middleware/
    Models/
    Services/
routes/
  web.php
views/
  partials/
  auth/
  dashboard/
resources/
  images/
database/
  migrations/
  migrate.php
  queue-work.php
tests/
storage/
  logs/
  app/</code></pre>

    <h2>What belongs where</h2>
    <ul>
        <li><code>src/Core/</code>: framework internals like <code>Router</code>, <code>Request</code>, <code>Response</code>, <code>Database</code>, <code>View</code>, <code>ErrorHandler</code>.</li>
        <li><code>src/App/Controllers/</code>: HTTP endpoints and request orchestration.</li>
        <li><code>src/App/Services/</code>: business logic, external APIs, and workflows.</li>
        <li><code>src/App/Middleware/</code>: route guards and request checks.</li>
        <li><code>routes/web.php</code>: route registration.</li>
        <li><code>views/</code>: plain PHP templates (no template engine dependency).</li>
        <li><code>public_html/deck/</code>: Deck's stylesheet, icon sprite and scripts, published by Composer. Never edit these by hand.</li>
        <li><code>public_html/css/keel.css</code> and <code>public_html/js/keel.js</code>: Keel's own styles and glue, served exactly as written. There is no build step.</li>
        <li><code>resources/images/</code>: brand source images.</li>
        <li><code>database/migrations/</code>: SQL migrations applied by <code>php database/migrate.php</code>.</li>
        <li><code>database/queue-work.php</code>: queue worker CLI entrypoint.</li>
    </ul>
</div>
