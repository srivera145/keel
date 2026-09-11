<div class="prose">
    <p>Keel supports passwordless OTP, magic-link, or both.</p>

    <h2>Environment toggle</h2>
        <pre><code># otp | magic_link | both
AUTH_METHOD=both</code></pre>

    <h2>Routes</h2>
        <pre><code>$router-&gt;post('/auth/otp/request', [AuthController::class, 'requestOtp']);
$router-&gt;post('/auth/otp/verify', [AuthController::class, 'verifyOtp']);
$router-&gt;post('/auth/magic/request', [AuthController::class, 'requestMagicLink']);
$router-&gt;get('/auth/magic', [AuthController::class, 'verifyMagicLink']);</code></pre>

    <h2>Current behavior</h2>
    <ul>
        <li>OTP: 6-digit code, hashed with <code>password_hash()</code>, 10-minute expiry.</li>
        <li>Magic link: 32-byte token, SHA-256 hash at rest, 15-minute expiry.</li>
        <li>Both flows send mail through the shared Mailer (see the Mailing docs page for SMTP/log setup).</li>
        <li>Both flows rate-limit token issuance to 5 requests per 15 minutes per user.</li>
        <li>Successful sign-in regenerates the session and sets <code>user_id</code>, <code>user_email</code>, and org/theme session values.</li>
        <li>If multi-tenancy is enabled and no org is assigned, login redirects to <code>/onboarding/organization</code>; otherwise <code>/dashboard</code>.</li>
    </ul>
</div>
