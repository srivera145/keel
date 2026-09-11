<?php

namespace Keel\App\Services;

use Keel\Core\Database;

class ApiTokenService
{
    public function create(int $userId, string $name, string $abilities = '*', ?\DateTimeImmutable $expiresAt = null): array
    {
        $rawToken = bin2hex(random_bytes(32));
        $tokenHash = hash('sha256', $rawToken);

        $statement = Database::connection()->prepare(
            'INSERT INTO api_tokens (user_id, name, token_hash, abilities, expires_at)
             VALUES (:user_id, :name, :token_hash, :abilities, :expires_at)'
        );
        $statement->bindValue(':user_id', $userId, \PDO::PARAM_INT);
        $statement->bindValue(':name', $name);
        $statement->bindValue(':token_hash', $tokenHash);
        $statement->bindValue(':abilities', $abilities);
        if ($expiresAt === null) {
            $statement->bindValue(':expires_at', null, \PDO::PARAM_NULL);
        } else {
            $statement->bindValue(':expires_at', $expiresAt->format('Y-m-d H:i:s'));
        }
        $statement->execute();

        $recordId = (int) Database::connection()->lastInsertId();
        $record = $this->findOwnedById($recordId, $userId);

        return [
            'token' => $rawToken,
            'record' => $record,
        ];
    }

    public function revoke(int $tokenId, int $userId): bool
    {
        $statement = Database::connection()->prepare('DELETE FROM api_tokens WHERE id = ? AND user_id = ?');
        $statement->execute([$tokenId, $userId]);

        return $statement->rowCount() > 0;
    }

    // "list" cannot be used as a method name in PHP, so listTokens is used instead.
    public function listTokens(int $userId): array
    {
        $statement = Database::connection()->prepare(
            'SELECT id, name, abilities, last_used_at, expires_at, created_at
             FROM api_tokens
             WHERE user_id = ?
             ORDER BY id DESC'
        );
        $statement->execute([$userId]);

        return $statement->fetchAll();
    }

    private function findOwnedById(int $id, int $userId): ?array
    {
        $statement = Database::connection()->prepare(
            'SELECT id, user_id, name, abilities, last_used_at, expires_at, created_at
             FROM api_tokens WHERE id = ? AND user_id = ? LIMIT 1'
        );
        $statement->execute([$id, $userId]);

        return $statement->fetch() ?: null;
    }
}
