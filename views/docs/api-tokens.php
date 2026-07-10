<div class="space-y-6 text-sm leading-7 text-[var(--color-text-muted)]">
    <p>API tokens provide Bearer-based API access without replacing session auth for web pages.</p>

    <section>
        <h2 class="mb-2 text-lg font-semibold text-[var(--color-text-strong)]">UI routes</h2>
        <pre class="overflow-x-auto rounded-xl bg-[var(--color-surface-muted)] p-4 text-xs text-[var(--color-text-strong)]"><code>$router-&gt;get('/settings/api-tokens', [ApiTokenController::class, 'index']);
$router-&gt;post('/settings/api-tokens', [ApiTokenController::class, 'store']);
$router-&gt;post('/settings/api-tokens/{id}/revoke', [ApiTokenController::class, 'destroy']);</code></pre>
    </section>

    <section>
        <h2 class="mb-2 text-lg font-semibold text-[var(--color-text-strong)]">Token storage and validation</h2>
        <ul class="list-disc space-y-1 pl-5">
            <li><code>ApiTokenService</code> creates a random 64-hex token and stores a SHA-256 hash in <code>api_tokens.token_hash</code>.</li>
            <li>The raw token is shown once at creation time.</li>
            <li><code>ApiAuthMiddleware</code> expects <code>Authorization: Bearer &lt;token&gt;</code>.</li>
            <li>Middleware checks expiration, updates <code>last_used_at</code>, and authenticates the user context.</li>
        </ul>
    </section>

    <section>
        <h2 class="mb-2 text-lg font-semibold text-[var(--color-text-strong)]">Protected API routes</h2>
        <pre class="overflow-x-auto rounded-xl bg-[var(--color-surface-muted)] p-4 text-xs text-[var(--color-text-strong)]"><code>$router-&gt;group([
    'prefix' =&gt; '/api/v1',
    'middleware' =&gt; [ThrottleMiddleware::class, ApiAuthMiddleware::class],
], function ($router) {
    $router-&gt;get('/files', [ApiFileController::class, 'index']);
});</code></pre>
    </section>
</div>
