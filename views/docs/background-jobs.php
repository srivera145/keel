<div class="prose">
    <p>Keel includes a database-backed queue with <code>jobs</code> and <code>failed_jobs</code> tables.</p>

    <h2>Queue API</h2>
        <pre><code>Queue::push(
    SendOrganizationInviteJob::class,
    ['to_email' =&gt; $email, 'subject' =&gt; 'Invite'],
    'default',
    0
);</code></pre>
    <p><code>Queue::push()</code> writes to <code>jobs</code> with <code>available_at</code> using SQL <code>DATE_ADD(NOW(), INTERVAL ... SECOND)</code>.</p>

    <h2>Worker CLI</h2>
        <pre><code># one drain pass (cron style)
php database/queue-work.php --once

# long-running worker
php database/queue-work.php</code></pre>
    <ul>
        <li>Script is CLI-only and loads env before processing.</li>
        <li>Jobs are reserved with <code>FOR UPDATE</code> transaction locks.</li>
        <li>Failures are retried with backoff; at 5 attempts they move to <code>failed_jobs</code>.</li>
    </ul>

    <h2>Deployment patterns</h2>
    <ol>
        <li>Cron: run <code>php database/queue-work.php --once</code> every minute.</li>
        <li>Supervisor/systemd: keep <code>php database/queue-work.php</code> running continuously.</li>
    </ol>
</div>
