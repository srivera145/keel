<?php

namespace Keel\Core;

class RateLimiter
{
    public static function attempt(string $key, int $maxAttempts, int $decayMinutes): bool
    {
        $normalizedKey = trim($key);
        $maxAttempts = max(1, $maxAttempts);
        $decayMinutes = max(1, $decayMinutes);

        if ($normalizedKey === '') {
            return true;
        }

        $connection = Database::connection();
        $connection->beginTransaction();

        try {
            $select = $connection->prepare('SELECT id, attempts, expires_at < NOW() AS expired FROM rate_limits WHERE `key` = ? LIMIT 1 FOR UPDATE');
            $select->execute([$normalizedKey]);
            $row = $select->fetch();

            if (!$row) {
                $insert = $connection->prepare(
                    'INSERT INTO rate_limits (`key`, attempts, expires_at) VALUES (?, 1, DATE_ADD(NOW(), INTERVAL ' . $decayMinutes . ' MINUTE))'
                );
                $insert->execute([$normalizedKey]);
                $connection->commit();

                return true;
            }

            if ((int) $row['expired'] === 1) {
                $reset = $connection->prepare(
                    'UPDATE rate_limits SET attempts = 1, expires_at = DATE_ADD(NOW(), INTERVAL ' . $decayMinutes . ' MINUTE) WHERE id = ?'
                );
                $reset->execute([$row['id']]);
                $connection->commit();

                return true;
            }

            if ((int) $row['attempts'] >= $maxAttempts) {
                $connection->commit();

                return false;
            }

            $update = $connection->prepare('UPDATE rate_limits SET attempts = attempts + 1 WHERE id = ?');
            $update->execute([$row['id']]);
            $connection->commit();

            return true;
        } catch (\Throwable $exception) {
            if ($connection->inTransaction()) {
                $connection->rollBack();
            }

            throw $exception;
        }
    }
}