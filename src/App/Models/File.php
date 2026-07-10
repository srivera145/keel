<?php

namespace Keel\App\Models;

use Keel\Core\Database;

class File
{
    public static function create(array $attributes): int
    {
        $statement = Database::connection()->prepare(
            'INSERT INTO files (user_id, disk, path, original_name, mime_type, size_bytes, is_public)
             VALUES (:user_id, :disk, :path, :original_name, :mime_type, :size_bytes, :is_public)'
        );
        $statement->execute($attributes);

        return (int) Database::connection()->lastInsertId();
    }

    public static function find(int $id): ?array
    {
        $statement = Database::connection()->prepare('SELECT * FROM files WHERE id = ? LIMIT 1');
        $statement->execute([$id]);

        return $statement->fetch() ?: null;
    }

    public static function forUser(int $userId): array
    {
        $statement = Database::connection()->prepare(
            'SELECT id, path, original_name, mime_type, size_bytes, is_public, created_at
             FROM files
             WHERE user_id = ?
             ORDER BY created_at DESC, id DESC'
        );
        $statement->execute([$userId]);

        return $statement->fetchAll();
    }
}