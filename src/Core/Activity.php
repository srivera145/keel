<?php

namespace Keel\Core;

class Activity
{
    public static function log(
        string $action,
        ?string $subjectType = null,
        ?int $subjectId = null,
        array $metadata = []
    ): void {
        try {
            $userId = Auth::id();
            $organizationId = null;

            if ((bool) Env::get('MULTI_TENANCY_ENABLED', false) && Session::has('organization_id')) {
                $organizationId = Session::get('organization_id');
            }

            $ipAddress = $_SERVER['REMOTE_ADDR'] ?? null;
            $metadataJson = $metadata === [] ? null : json_encode($metadata, JSON_THROW_ON_ERROR);

            $statement = Database::connection()->prepare(
                'INSERT INTO activity_log (user_id, organization_id, action, subject_type, subject_id, metadata, ip_address)
                 VALUES (:user_id, :organization_id, :action, :subject_type, :subject_id, :metadata, :ip_address)'
            );

            $statement->bindValue(':user_id', self::nullableInt($userId), self::nullableIntType($userId));
            $statement->bindValue(':organization_id', self::nullableInt($organizationId), self::nullableIntType($organizationId));
            $statement->bindValue(':action', $action);
            $statement->bindValue(':subject_type', $subjectType);
            $statement->bindValue(':subject_id', self::nullableInt($subjectId), self::nullableIntType($subjectId));
            $statement->bindValue(':metadata', $metadataJson);
            $statement->bindValue(':ip_address', $ipAddress);

            $statement->execute();
        } catch (\Throwable $exception) {
            error_log('[Keel] Activity log failed: ' . $exception->getMessage());
        }
    }

    private static function nullableInt(mixed $value): ?int
    {
        if ($value === null || $value === '') {
            return null;
        }

        return (int) $value;
    }

    private static function nullableIntType(mixed $value): int
    {
        return ($value === null || $value === '') ? \PDO::PARAM_NULL : \PDO::PARAM_INT;
    }
}
