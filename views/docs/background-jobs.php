<div class="space-y-6 text-sm leading-7 text-[var(--color-text-muted)]">
    <p>Keel includes a database-backed queue with <code>jobs</code> and <code>failed_jobs</code> tables.</p>

    <section>
        <h2 class="mb-2 text-lg font-semibold text-[var(--color-text-strong)]">Queue API</h2>
        <pre class="overflow-x-auto rounded-xl bg-[var(--color-surface-muted)] p-4 text-xs text-[var(--color-text-strong)]"><code>Queue::push(
    SendOrganizationInviteJob::class,
    ['to_email' =&gt; $email, 'subject' =&gt; 'Invite'],
    'default',
    0
);</code></pre>
        <p><code>Queue::push()</code> writes to <code>jobs</code> with <code>available_at</code> using SQL <code>DATE_ADD(NOW(), INTERVAL ... SECOND)</code>.</p>
    </section>

    <section>
        <h2 class="mb-2 text-lg font-semibold text-[var(--color-text-strong)]">Worker CLI</h2>
        <pre class="overflow-x-auto rounded-xl bg-[var(--color-surface-muted)] p-4 text-xs text-[var(--color-text-strong)]"><code># one drain pass (cron style)
php database/queue-work.php --once

# long-running worker
php database/queue-work.php</code></pre>
        <ul class="list-disc space-y-1 pl-5">
            <li>Script is CLI-only and loads env before processing.</li>
            <li>Jobs are reserved with <code>FOR UPDATE</code> transaction locks.</li>
            <li>Failures are retried with backoff; at 5 attempts they move to <code>failed_jobs</code>.</li>
        </ul>
    </section>

    <section>
        <h2 class="mb-2 text-lg font-semibold text-[var(--color-text-strong)]">Deployment patterns</h2>
        <ol class="list-decimal space-y-1 pl-5">
            <li>Cron: run <code>php database/queue-work.php --once</code> every minute.</li>
            <li>Supervisor/systemd: keep <code>php database/queue-work.php</code> running continuously.</li>
        </ol>
    </section>
</div>
