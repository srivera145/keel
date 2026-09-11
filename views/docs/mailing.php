<div class="space-y-6 text-sm leading-7 text-[var(--color-text-muted)]">
    <p>Keel mail delivery is handled by the Mailer class and supports two drivers: smtp and log.</p>

    <section>
        <h2 class="mb-2 text-lg font-semibold text-[var(--color-text-strong)]">Environment settings</h2>
        <pre class="overflow-x-auto rounded-xl bg-[var(--color-surface-muted)] p-4 text-xs text-[var(--color-text-strong)]"><code># smtp | log
MAIL_MAILER=smtp

MAIL_HOST=smtp.mailtrap.io
MAIL_PORT=587
MAIL_USERNAME=
MAIL_PASSWORD=
MAIL_ENCRYPTION=tls
MAIL_FROM_ADDRESS=hello@example.com
MAIL_FROM_NAME="Keel App"</code></pre>
        <p>Use MAIL_MAILER=log for local development without SMTP credentials.</p>
    </section>

    <section>
        <h2 class="mb-2 text-lg font-semibold text-[var(--color-text-strong)]">Driver behavior</h2>
        <ul class="list-disc space-y-1 pl-5">
            <li>smtp: sends real email through PHPMailer over SMTP.</li>
            <li>log: appends outgoing messages to storage/logs/mail.log.</li>
            <li>unsupported MAIL_MAILER value: returns false and writes an error log message.</li>
        </ul>
    </section>

    <section>
        <h2 class="mb-2 text-lg font-semibold text-[var(--color-text-strong)]">How to call the mailer</h2>
        <pre class="overflow-x-auto rounded-xl bg-[var(--color-surface-muted)] p-4 text-xs text-[var(--color-text-strong)]"><code>use Keel\Core\Mailer;

$sent = Mailer::send(
    $toEmail,
    $toName,
    'Subject line',
    '&lt;p&gt;HTML body&lt;/p&gt;'
);</code></pre>
        <p>Mailer::send returns true on success and false on failure.</p>
    </section>

    <section>
        <h2 class="mb-2 text-lg font-semibold text-[var(--color-text-strong)]">Where it is used today</h2>
        <ul class="list-disc space-y-1 pl-5">
            <li>OTP and magic-link sign-in emails use Mailer::send directly via OtpService and MagicLinkService.</li>
            <li>Organization invite emails are queued, then sent by SendOrganizationInviteJob through Mailer::send.</li>
        </ul>
    </section>

    <section>
        <h2 class="mb-2 text-lg font-semibold text-[var(--color-text-strong)]">Local testing flow</h2>
        <ol class="list-decimal space-y-1 pl-5">
            <li>Set MAIL_MAILER=log in .env.</li>
            <li>Request an OTP or magic link from the login page.</li>
            <li>Open storage/logs/mail.log and copy the code or URL from the newest entry.</li>
        </ol>
    </section>
</div>
