<?php

namespace Keel\Core;

use PHPMailer\PHPMailer\Exception as PHPMailerException;
use PHPMailer\PHPMailer\PHPMailer;

class Mailer
{
    public static function send(string $toEmail, string $toName, string $subject, string $htmlBody): bool
    {
        $mailer = strtolower(trim((string) Env::get('MAIL_MAILER', 'smtp')));

        if ($mailer === 'log') {
            return self::sendViaLog($toEmail, $toName, $subject, $htmlBody);
        }

        if ($mailer !== 'smtp') {
            error_log('[Keel] Mail failed: Unsupported MAIL_MAILER value "' . $mailer . '". Expected smtp or log.');
            return false;
        }

        return self::sendViaSmtp($toEmail, $toName, $subject, $htmlBody);
    }

    private static function sendViaSmtp(string $toEmail, string $toName, string $subject, string $htmlBody): bool
    {
        $mail = new PHPMailer(true);

        try {
            $mail->isSMTP();
            $mail->Host = Env::get('MAIL_HOST');
            $mail->SMTPAuth = true;
            $mail->Username = Env::get('MAIL_USERNAME');
            $mail->Password = Env::get('MAIL_PASSWORD');
            $mail->SMTPSecure = Env::get('MAIL_ENCRYPTION', 'tls');
            $mail->Port = (int) Env::get('MAIL_PORT', 587);

            $mail->setFrom(Env::get('MAIL_FROM_ADDRESS'), Env::get('MAIL_FROM_NAME', 'App'));
            $mail->addAddress($toEmail, $toName);

            $mail->isHTML(true);
            $mail->Subject = $subject;
            $mail->Body = $htmlBody;
            $mail->AltBody = strip_tags($htmlBody);

            $mail->send();
            return true;
        } catch (PHPMailerException $e) {
            error_log('[Keel] Mail failed: ' . $mail->ErrorInfo);
            return false;
        }
    }

    private static function sendViaLog(string $toEmail, string $toName, string $subject, string $htmlBody): bool
    {
        $basePath = dirname(__DIR__, 2);
        $logDirectory = $basePath . '/storage/logs';
        $logPath = $logDirectory . '/mail.log';

        if (!is_dir($logDirectory) && !mkdir($logDirectory, 0775, true) && !is_dir($logDirectory)) {
            error_log('[Keel] Mail failed: Could not create log directory for MAIL_MAILER=log.');
            return false;
        }

        $timestamp = date('Y-m-d H:i:s');
        $textBody = trim(html_entity_decode(strip_tags($htmlBody), ENT_QUOTES | ENT_HTML5, 'UTF-8'));

        $entry = "[{$timestamp}] MAIL_MAILER=log\n"
            . "To: {$toName} <{$toEmail}>\n"
            . "Subject: {$subject}\n"
            . "\n"
            . "Text Body:\n"
            . ($textBody !== '' ? $textBody : '(empty)')
            . "\n\n"
            . "HTML Body:\n"
            . $htmlBody
            . "\n"
            . str_repeat('-', 80)
            . "\n";

        $written = @file_put_contents($logPath, $entry, FILE_APPEND | LOCK_EX);

        if ($written === false) {
            error_log('[Keel] Mail failed: Could not write to storage/logs/mail.log for MAIL_MAILER=log.');
            return false;
        }

        return true;
    }
}
