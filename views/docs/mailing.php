<div class="prose">
    <p>Keel mail delivery is handled by the Mailer class and supports two drivers: smtp and log.</p>

    <h2>Environment settings</h2>
        <pre><code># smtp | log
MAIL_MAILER=smtp

MAIL_HOST=smtp.mailtrap.io
MAIL_PORT=587
MAIL_USERNAME=
MAIL_PASSWORD=
MAIL_ENCRYPTION=tls
MAIL_FROM_ADDRESS=hello@example.com
MAIL_FROM_NAME="Keel App"</code></pre>
    <p>Use MAIL_MAILER=log for local development without SMTP credentials.</p>

    <h2>Driver behavior</h2>
    <ul>
        <li>smtp: sends real email through PHPMailer over SMTP.</li>
        <li>log: appends outgoing messages to storage/logs/mail.log.</li>
        <li>unsupported MAIL_MAILER value: returns false and writes an error log message.</li>
    </ul>

    <h2>How to call the mailer</h2>
        <pre><code>use Keel\Core\Mailer;

$sent = Mailer::send(
    $toEmail,
    $toName,
    'Subject line',
    '&lt;p&gt;HTML body&lt;/p&gt;'
);</code></pre>
    <p>Mailer::send returns true on success and false on failure.</p>

    <h2>Where it is used today</h2>
    <ul>
        <li>OTP and magic-link sign-in emails use Mailer::send directly via OtpService and MagicLinkService.</li>
        <li>Organization invite emails are queued, then sent by SendOrganizationInviteJob through Mailer::send.</li>
    </ul>

    <h2>Local testing flow</h2>
    <ol>
        <li>Set MAIL_MAILER=log in .env.</li>
        <li>Request an OTP or magic link from the login page.</li>
        <li>Open storage/logs/mail.log and copy the code or URL from the newest entry.</li>
    </ol>
</div>
