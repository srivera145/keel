<?php

declare(strict_types=1);

use Keel\App\Jobs\Job;
use Keel\Core\Database;
use Keel\Core\Env;

require dirname(__DIR__) . '/vendor/autoload.php';

if (PHP_SAPI !== 'cli') {
    fwrite(STDERR, "This script must be run from the command line.\n");
    exit(1);
}

$basePath = dirname(__DIR__);
Env::load($basePath);

$once = in_array('--once', $argv, true);
$queueName = 'default';

try {
    if ($once) {
        processAvailableJobs($queueName);
        exit(0);
    }

    while (true) {
        $processed = processOneJob($queueName);

        if (!$processed) {
            sleep(2);
        }
    }
} catch (\Throwable $exception) {
    fwrite(STDERR, $exception->getMessage() . "\n");
    exit(1);
}

function processAvailableJobs(string $queueName): void
{
    while (processOneJob($queueName)) {
        // Keep draining currently available jobs for cron usage.
    }
}

function processOneJob(string $queueName): bool
{
    $jobRow = reserveNextJob($queueName);

    if ($jobRow === null) {
        return false;
    }

    try {
        runJob($jobRow);

        $deleteStatement = Database::connection()->prepare('DELETE FROM jobs WHERE id = :id');
        $deleteStatement->execute(['id' => $jobRow['id']]);

        return true;
    } catch (\Throwable $exception) {
        releaseOrFailJob($jobRow, $exception);
        return true;
    }
}

function reserveNextJob(string $queueName): ?array
{
    $connection = Database::connection();
    $connection->beginTransaction();

    try {
        $selectStatement = $connection->prepare(
            'SELECT id, queue, job_class, payload, attempts
             FROM jobs
             WHERE queue = :queue
               AND available_at <= NOW()
               AND reserved_at IS NULL
             ORDER BY id ASC
             LIMIT 1
             FOR UPDATE'
        );
        $selectStatement->execute(['queue' => $queueName]);
        $jobRow = $selectStatement->fetch();

        if (!$jobRow) {
            $connection->commit();
            return null;
        }

        $reserveStatement = $connection->prepare('UPDATE jobs SET reserved_at = NOW() WHERE id = :id');
        $reserveStatement->execute(['id' => $jobRow['id']]);

        $connection->commit();

        return $jobRow;
    } catch (\Throwable $exception) {
        if ($connection->inTransaction()) {
            $connection->rollBack();
        }

        throw $exception;
    }
}

function runJob(array $jobRow): void
{
    $payload = json_decode((string) $jobRow['payload'], true);

    if (!is_array($payload)) {
        throw new \RuntimeException('Job payload must decode to an array.');
    }

    $jobClass = (string) $jobRow['job_class'];

    if (!class_exists($jobClass)) {
        throw new \RuntimeException("Job class {$jobClass} does not exist.");
    }

    $job = new $jobClass();

    if (!$job instanceof Job) {
        throw new \RuntimeException("Job class {$jobClass} must implement " . Job::class . '.');
    }

    $job->handle($payload);
}

function releaseOrFailJob(array $jobRow, \Throwable $exception): void
{
    $connection = Database::connection();
    $connection->beginTransaction();

    try {
        $lockStatement = $connection->prepare(
            'SELECT id, queue, job_class, payload, attempts
             FROM jobs
             WHERE id = :id
             FOR UPDATE'
        );
        $lockStatement->execute(['id' => $jobRow['id']]);
        $current = $lockStatement->fetch();

        if (!$current) {
            $connection->commit();
            return;
        }

        $nextAttempts = (int) $current['attempts'] + 1;

        if ($nextAttempts >= 5) {
            $failedStatement = $connection->prepare(
                'INSERT INTO failed_jobs (queue, job_class, payload, exception)
                 VALUES (:queue, :job_class, :payload, :exception)'
            );
            $failedStatement->execute([
                'queue' => $current['queue'],
                'job_class' => $current['job_class'],
                'payload' => $current['payload'],
                'exception' => formatException($exception),
            ]);

            $deleteStatement = $connection->prepare('DELETE FROM jobs WHERE id = :id');
            $deleteStatement->execute(['id' => $current['id']]);
        } else {
            $backoffSeconds = $nextAttempts * 30;

            $retryStatement = $connection->prepare(
                'UPDATE jobs
                 SET attempts = :attempts,
                     reserved_at = NULL,
                     available_at = DATE_ADD(NOW(), INTERVAL :backoff_seconds SECOND)
                 WHERE id = :id'
            );
            $retryStatement->bindValue(':attempts', $nextAttempts, \PDO::PARAM_INT);
            $retryStatement->bindValue(':backoff_seconds', $backoffSeconds, \PDO::PARAM_INT);
            $retryStatement->bindValue(':id', (int) $current['id'], \PDO::PARAM_INT);
            $retryStatement->execute();
        }

        $connection->commit();
    } catch (\Throwable $innerException) {
        if ($connection->inTransaction()) {
            $connection->rollBack();
        }

        throw $innerException;
    }
}

function formatException(\Throwable $exception): string
{
    $message = $exception::class . ': ' . $exception->getMessage() . "\n" . $exception->getTraceAsString();

    return substr($message, 0, 65535);
}
