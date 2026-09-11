<?php

namespace Keel\App\Jobs;

use Keel\Core\Mailer;

class SendOrganizationInviteJob implements \Keel\App\Jobs\Job
{
    public function handle(array $data): void
    {
        $toEmail = trim((string) ($data['to_email'] ?? ''));
        $toName = trim((string) ($data['to_name'] ?? $toEmail));
        $subject = trim((string) ($data['subject'] ?? ''));
        $htmlBody = (string) ($data['html_body'] ?? '');

        if ($toEmail === '' || $subject === '' || $htmlBody === '') {
            throw new \RuntimeException('SendOrganizationInviteJob payload is missing required fields.');
        }

        $sent = Mailer::send($toEmail, $toName, $subject, $htmlBody);

        if (!$sent) {
            throw new \RuntimeException('Failed to send organization invite email.');
        }
    }
}
