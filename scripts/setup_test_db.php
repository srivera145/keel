<?php

declare(strict_types=1);

use Dotenv\Dotenv;
use Keel\Core\Migrations;

require dirname(__DIR__) . '/vendor/autoload.php';

if (PHP_SAPI !== 'cli') {
    fwrite(STDERR, "This script must be run from the command line.\n");
    exit(1);
}

$basePath = dirname(__DIR__);
Dotenv::createImmutable($basePath, '.env.testing')->safeLoad();

$pretend = in_array('--pretend', $argv, true);

$host = envValue('DB_HOST', '127.0.0.1');
$port = envValue('DB_PORT', '3306');
$user = envValue('DB_USERNAME', 'root');
$pass = envValue('DB_PASSWORD', '');
$charset = envValue('DB_CHARSET', 'utf8mb4');
$testDatabase = envValue('DB_DATABASE_TEST', envValue('DB_DATABASE', ''));

if ($testDatabase === '') {
    fwrite(STDERR, "DB_DATABASE_TEST must be set in .env.testing before bootstrapping feature tests.\n");
    exit(1);
}

if (!preg_match('/(test|ci)/i', $testDatabase)) {
    fwrite(STDERR, "Refusing to run: DB_DATABASE_TEST must include 'test' or 'ci' to avoid hitting a development database.\n");
    exit(1);
}

try {
    $pdo = connectForMigrations($host, $port, $testDatabase, $user, $pass, $charset);
    $pdo->exec("SET time_zone = '+00:00'");

    // One statement at a time, so a failure anywhere in a file is reported and the file is not recorded.
    $applied = Migrations::apply($pdo, $basePath . '/database/migrations', static function (string $line): void {
        fwrite(STDOUT, $line . "\n");
    }, $pretend);

    if ($applied === []) {
        fwrite(STDOUT, "Test database '{$testDatabase}' is up to date.\n");
    } elseif ($pretend) {
        fwrite(STDOUT, 'Pending migrations: ' . count($applied) . "\n");
    } else {
        fwrite(STDOUT, "Feature-test database '{$testDatabase}' is ready.\n");
    }
} catch (Throwable $exception) {
    fwrite(STDERR, $exception->getMessage() . "\n");
    exit(1);
}

function envValue(string $key, string $default = ''): string
{
    // As in Env::get: ?? skips nulls and getenv() returns a string or false, so $value is never null here.
    $value = $_ENV[$key] ?? $_SERVER[$key] ?? getenv($key);

    if ($value === false) {
        return $default;
    }

    return (string) $value;
}

function connectForMigrations(string $host, string $port, string $database, string $user, string $pass, string $charset): PDO
{
    $databaseDsn = "mysql:host={$host};port={$port};dbname={$database};charset={$charset}";
    $serverDsn = "mysql:host={$host};port={$port};charset={$charset}";

    try {
        return createPdo($databaseDsn, $user, $pass);
    } catch (PDOException $exception) {
        if (!str_contains($exception->getMessage(), 'Unknown database')) {
            throw new RuntimeException('Could not connect to the configured test database.');
        }
    }

    $serverConnection = createPdo($serverDsn, $user, $pass);
    $safeDatabase = str_replace('`', '``', $database);
    $serverConnection->exec("CREATE DATABASE IF NOT EXISTS `{$safeDatabase}` CHARACTER SET {$charset} COLLATE {$charset}_unicode_ci");

    return createPdo($databaseDsn, $user, $pass);
}

function createPdo(string $dsn, string $user, string $pass): PDO
{
    return new PDO($dsn, $user, $pass, [
        PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION,
        PDO::ATTR_DEFAULT_FETCH_MODE => PDO::FETCH_ASSOC,
        PDO::ATTR_EMULATE_PREPARES => false,
    ]);
}
