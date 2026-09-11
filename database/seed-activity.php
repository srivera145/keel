<?php

declare(strict_types=1);

use Keel\Core\Database;
use Keel\Core\Env;

require dirname(__DIR__) . '/vendor/autoload.php';

if (PHP_SAPI !== 'cli') {
    fwrite(STDERR, "This script must be run from the command line.\n");
    exit(1);
}

Env::load(dirname(__DIR__));

$appEnv = strtolower((string) Env::get('APP_ENV', 'production'));
if (!in_array($appEnv, ['local', 'dev', 'development', 'testing'], true)) {
    fwrite(STDERR, "Refusing to seed activity data outside local/dev/testing environments.\n");
    exit(1);
}

$count = max(1, min(500, parseIntArg('--count', 30)));
$email = parseStringArg('--email', 'activity-seed@example.com');
$orgId = parseOptionalIntArg('--org-id');
$append = in_array('--append', $argv, true);

$pdo = Database::connection();

$userId = ensureUser($pdo, $email, $orgId);

if (!$append) {
    $cleanup = $pdo->prepare('DELETE FROM activity_log WHERE user_id = ? AND metadata LIKE ?');
    $cleanup->execute([$userId, '%"seed":"activity_cli"%']);
}

$actions = [
    ['user.login', null],
    ['file.uploaded', 'File'],
    ['file.viewed', 'File'],
    ['organization.invite_sent', 'OrganizationInvite'],
    ['organization.invite_accepted', 'Organization'],
    ['subscription.updated', 'Subscription'],
    ['subscription.canceled', 'Subscription'],
];

$insert = $pdo->prepare(
    'INSERT INTO activity_log (user_id, organization_id, action, subject_type, subject_id, metadata, ip_address, created_at)
     VALUES (:user_id, :organization_id, :action, :subject_type, :subject_id, :metadata, :ip_address, :created_at)'
);

for ($i = 0; $i < $count; $i++) {
    [$action, $subjectType] = $actions[$i % count($actions)];
    $subjectId = $subjectType === null ? null : 1000 + $i;
    $metadata = json_encode([
        'seed' => 'activity_cli',
        'index' => $i,
        'note' => 'Synthetic activity row for local smoke/testing',
    ], JSON_UNESCAPED_SLASHES);

    $insert->execute([
        'user_id' => $userId,
        'organization_id' => $orgId,
        'action' => $action,
        'subject_type' => $subjectType,
        'subject_id' => $subjectId,
        'metadata' => $metadata,
        'ip_address' => '127.0.0.1',
        'created_at' => date('Y-m-d H:i:s', strtotime("-{$i} minutes")),
    ]);
}

fwrite(STDOUT, "Seeded {$count} activity rows for user #{$userId} ({$email}).\n");
fwrite(STDOUT, 'Mode: ' . ($append ? 'append' : 'replace previous seeded rows') . "\n");

function ensureUser(PDO $pdo, string $email, ?int $orgId): int
{
    $upsert = $pdo->prepare('INSERT INTO users (email, organization_id, created_at) VALUES (?, ?, NOW()) ON DUPLICATE KEY UPDATE organization_id = COALESCE(VALUES(organization_id), organization_id)');
    $upsert->execute([$email, $orgId]);

    $statement = $pdo->prepare('SELECT id FROM users WHERE email = ? LIMIT 1');
    $statement->execute([$email]);

    return (int) $statement->fetchColumn();
}

function parseIntArg(string $name, int $default): int
{
    global $argv;

    foreach ($argv as $arg) {
        if (str_starts_with($arg, $name . '=')) {
            return (int) substr($arg, strlen($name) + 1);
        }
    }

    return $default;
}

function parseOptionalIntArg(string $name): ?int
{
    global $argv;

    foreach ($argv as $arg) {
        if (str_starts_with($arg, $name . '=')) {
            $value = trim((string) substr($arg, strlen($name) + 1));
            if ($value === '') {
                return null;
            }

            return (int) $value;
        }
    }

    return null;
}

function parseStringArg(string $name, string $default): string
{
    global $argv;

    foreach ($argv as $arg) {
        if (str_starts_with($arg, $name . '=')) {
            $value = trim((string) substr($arg, strlen($name) + 1));
            return $value === '' ? $default : $value;
        }
    }

    return $default;
}
