<?php

declare(strict_types=1);

use Keel\Core\Env;
use Keel\Core\Migrations;

require dirname(__DIR__) . '/vendor/autoload.php';

if (PHP_SAPI !== 'cli') {
    fwrite(STDERR, "This script must be run from the command line.\n");
    exit(1);
}

$basePath = dirname(__DIR__);
Env::load($basePath);

$pretend = in_array('--pretend', $argv, true);

try {
    $pdo = connectForMigrations();
    $pdo->exec("SET time_zone = '+00:00'");

    // One statement at a time, so a failure anywhere in a file is reported and the file is not recorded.
    $applied = Migrations::apply($pdo, $basePath . '/database/migrations', static function (string $line): void {
        fwrite(STDOUT, $line . "\n");
    }, $pretend);

    if ($applied === []) {
        fwrite(STDOUT, "No pending migrations.\n");
    } elseif ($pretend) {
        fwrite(STDOUT, 'Pending migrations: ' . count($applied) . "\n");
    }
} catch (\Throwable $exception) {
    fwrite(STDERR, $exception->getMessage() . "\n");
    exit(1);
}

function connectForMigrations(): \PDO
{
    $host = (string) Env::get('DB_HOST', '127.0.0.1');
    $port = (string) Env::get('DB_PORT', '3306');
    $name = (string) Env::get('DB_DATABASE');
    $user = (string) Env::get('DB_USERNAME');
    $pass = (string) Env::get('DB_PASSWORD');
    $charset = (string) Env::get('DB_CHARSET', 'utf8mb4');

    if ($name === '') {
        throw new \RuntimeException('DB_DATABASE must be set before running migrations.');
    }

    $databaseDsn = "mysql:host={$host};port={$port};dbname={$name};charset={$charset}";
    $serverDsn = "mysql:host={$host};port={$port};charset={$charset}";

    try {
        return createPdo($databaseDsn, $user, $pass);
    } catch (\PDOException $exception) {
        if (!str_contains($exception->getMessage(), 'Unknown database')) {
            throw new \RuntimeException('Could not connect to the configured database.');
        }
    }

    try {
        $serverConnection = createPdo($serverDsn, $user, $pass);
    } catch (\PDOException $exception) {
        throw new \RuntimeException('Could not connect to MySQL server to create the database.');
    }

    $databaseName = str_replace('`', '``', $name);
    $serverConnection->exec("CREATE DATABASE IF NOT EXISTS `{$databaseName}` CHARACTER SET {$charset} COLLATE {$charset}_unicode_ci");

    return createPdo($databaseDsn, $user, $pass);
}

function createPdo(string $dsn, string $user, string $pass): \PDO
{
    return new \PDO($dsn, $user, $pass, [
        \PDO::ATTR_ERRMODE => \PDO::ERRMODE_EXCEPTION,
        \PDO::ATTR_DEFAULT_FETCH_MODE => \PDO::FETCH_ASSOC,
        \PDO::ATTR_EMULATE_PREPARES => false,
    ]);
}
