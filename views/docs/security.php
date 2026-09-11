<div class="space-y-6 text-sm leading-7 text-[var(--color-text-muted)]">
    <section>
        <h2 class="mb-2 text-lg font-semibold text-[var(--color-text-strong)]">CSRF protection</h2>
        <p>State-changing requests are guarded by <code>CsrfMiddleware</code> for <code>POST</code>, <code>PUT</code>, and <code>DELETE</code>.</p>
        <pre class="overflow-x-auto rounded-xl bg-[var(--color-surface-muted)] p-4 text-xs text-[var(--color-text-strong)]"><code>$token = (string) ($request-&gt;input('_csrf')
    ?? $request-&gt;headers['X-CSRF-Token']
    ?? $request-&gt;headers['x-csrf-token']
    ?? $_SERVER['HTTP_X_CSRF_TOKEN']
    ?? '');</code></pre>
        <p>Use <code>\Keel\Core\Csrf::field()</code> for forms or the <code>csrf-token</code> meta tag for fetch requests.</p>
    </section>

    <section>
        <h2 class="mb-2 text-lg font-semibold text-[var(--color-text-strong)]">Rate limiting</h2>
        <p><code>ThrottleMiddleware</code> applies an IP + route key and uses the <code>rate_limits</code> table.</p>
        <pre class="overflow-x-auto rounded-xl bg-[var(--color-surface-muted)] p-4 text-xs text-[var(--color-text-strong)]"><code>$key = strtolower($ipAddress . '|' . $request-&gt;uri);
RateLimiter::attempt($key, 30, 1);</code></pre>
        <p>On overflow, JSON requests get a <code>429</code> JSON response; web requests abort with <code>429</code>.</p>
    </section>

    <section>
        <h2 class="mb-2 text-lg font-semibold text-[var(--color-text-strong)]">Error handling</h2>
        <p>Unknown routes render <code>404</code> via <code>ErrorHandler::render(404)</code>; uncaught exceptions render <code>500</code>.</p>
        <p>In production, detailed exception output is hidden unless <code>APP_DEBUG=true</code>.</p>
    </section>

    <section>
        <h2 class="mb-2 text-lg font-semibold text-[var(--color-text-strong)]">Baseline security headers</h2>
        <ul class="list-disc space-y-1 pl-5">
            <li><code>X-Content-Type-Options: nosniff</code></li>
            <li><code>X-Frame-Options: DENY</code></li>
            <li><code>Referrer-Policy: strict-origin-when-cross-origin</code></li>
        </ul>
    </section>
</div>
