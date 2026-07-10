<div class="space-y-6 text-sm leading-7 text-[var(--color-text-muted)]">
    <p>Keel keeps framework internals separate from app code while keeping views plain PHP.</p>

    <pre class="overflow-x-auto rounded-xl bg-[var(--color-surface-muted)] p-4 text-xs text-[var(--color-text-strong)]"><code>public_html/
  index.php
  assets/
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
  css/
  js/
database/
  migrations/
  migrate.php
  queue-work.php
tests/
storage/
  logs/
  app/</code></pre>

    <section>
        <h2 class="mb-2 text-lg font-semibold text-[var(--color-text-strong)]">What belongs where</h2>
        <ul class="list-disc space-y-1 pl-5">
            <li><code>src/Core/</code>: framework internals like <code>Router</code>, <code>Request</code>, <code>Response</code>, <code>Database</code>, <code>View</code>, <code>ErrorHandler</code>.</li>
            <li><code>src/App/Controllers/</code>: HTTP endpoints and request orchestration.</li>
            <li><code>src/App/Services/</code>: business logic, external APIs, and workflows.</li>
            <li><code>src/App/Middleware/</code>: route guards and request checks.</li>
            <li><code>routes/web.php</code>: route registration.</li>
            <li><code>views/</code>: plain PHP templates (no template engine dependency).</li>
            <li><code>resources/</code>: Tailwind and JavaScript source files compiled by Vite.</li>
            <li><code>database/migrations/</code>: SQL migrations applied by <code>php database/migrate.php</code>.</li>
            <li><code>database/queue-work.php</code>: queue worker CLI entrypoint.</li>
        </ul>
    </section>
</div>
