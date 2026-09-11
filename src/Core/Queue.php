<?php

namespace Keel\Core;

class Queue
{
    public static function push(string $jobClass, array $data, string $queue = 'default', int $delaySeconds = 0): void
    {
        $delaySeconds = max(0, $delaySeconds);
        $payload = json_encode($data, JSON_THROW_ON_ERROR);

        $statement = Database::connection()->prepare(
            'INSERT INTO jobs (queue, job_class, payload, available_at)
             VALUES (:queue, :job_class, :payload, DATE_ADD(NOW(), INTERVAL :delay_seconds SECOND))'
        );

        $statement->bindValue(':queue', $queue);
        $statement->bindValue(':job_class', $jobClass);
        $statement->bindValue(':payload', $payload);
        $statement->bindValue(':delay_seconds', $delaySeconds, \PDO::PARAM_INT);

        $statement->execute();
    }
}
