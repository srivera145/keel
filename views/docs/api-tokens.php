<div class="prose">
    <p>API tokens provide Bearer-based API access without replacing session auth for web pages.</p>

    <h2>UI routes</h2>
        <pre><code>$router-&gt;get('/settings/api-tokens', [ApiTokenController::class, 'index']);
$router-&gt;post('/settings/api-tokens', [ApiTokenController::class, 'store']);
$router-&gt;post('/settings/api-tokens/{id}/revoke', [ApiTokenController::class, 'destroy']);</code></pre>

    <h2>Token storage and validation</h2>
    <ul>
        <li><code>ApiTokenService</code> creates a random 64-hex token and stores a SHA-256 hash in <code>api_tokens.token_hash</code>.</li>
        <li>The raw token is shown once at creation time.</li>
        <li><code>ApiAuthMiddleware</code> expects <code>Authorization: Bearer &lt;token&gt;</code>.</li>
        <li>Middleware checks expiration, updates <code>last_used_at</code>, and authenticates the user context.</li>
    </ul>

    <h2>Protected API routes</h2>
        <pre><code>$router-&gt;group([
    'prefix' =&gt; '/api/v1',
    'middleware' =&gt; [ThrottleMiddleware::class, ApiAuthMiddleware::class],
], function ($router) {
    $router-&gt;get('/files', [ApiFileController::class, 'index']);
});</code></pre>
</div>
