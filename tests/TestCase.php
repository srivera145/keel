<?php

declare(strict_types=1);

namespace Tests;

use Dotenv\Dotenv;
use Keel\Core\Auth;
use Keel\Core\CapturedResponseException;
use Keel\Core\Csrf;
use Keel\Core\Database;
use Keel\Core\Request;
use Keel\Core\Response;
use Keel\Core\Router;
use Keel\Core\Session;
use Keel\Core\View;
use PDO;
use PHPUnit\Framework\TestCase as PhpUnitTestCase;
use Tests\Support\TestResponse;

abstract class TestCase extends PhpUnitTestCase
{
    protected static string $basePath;

    protected static bool $booted = false;

    private array $serverBackup = [];
    private array $getBackup = [];
    private array $postBackup = [];

    public static function setUpBeforeClass(): void
    {
        parent::setUpBeforeClass();

        self::$basePath = dirname(__DIR__);

        if (!self::$booted) {
            self::loadTestingEnvironment();
            self::prepareTestingDatabase();
            self::applyPendingMigrations();
            self::$booted = true;
        }

        View::setPath(self::$basePath . '/views');
        Session::start();
    }

    protected function setUp(): void
    {
        parent::setUp();

        $this->serverBackup = $_SERVER;
        $this->getBackup = $_GET;
        $this->postBackup = $_POST;

        Response::setCaptureMode(true);
        Database::resetConnection();
        Session::start();
        $_SESSION = [];
        Auth::setUserId(null);

        $this->truncateApplicationTables();
        $this->clearMailLog();

        $_ENV['MULTI_TENANCY_ENABLED'] = 'false';
        $_SERVER['MULTI_TENANCY_ENABLED'] = 'false';
        $_ENV['AUTH_METHOD'] = 'both';
        $_SERVER['AUTH_METHOD'] = 'both';
        $_ENV['MAIL_MAILER'] = 'log';
        $_SERVER['MAIL_MAILER'] = 'log';
        $_ENV['STRIPE_WEBHOOK_SECRET'] = 'whsec_feature_test';
        $_SERVER['STRIPE_WEBHOOK_SECRET'] = 'whsec_feature_test';
    }

    protected function tearDown(): void
    {
        Response::setCaptureMode(false);
        Auth::setUserId(null);

        $_SERVER = $this->serverBackup;
        $_GET = $this->getBackup;
        $_POST = $this->postBackup;

        parent::tearDown();
    }

    protected function enableMultiTenancy(): void
    {
        $_ENV['MULTI_TENANCY_ENABLED'] = 'true';
        $_SERVER['MULTI_TENANCY_ENABLED'] = 'true';
    }

    protected function get(string $uri, array $headers = []): TestResponse
    {
        return $this->dispatch('GET', $uri, [], $headers, false, null);
    }

    protected function getJson(string $uri, array $headers = []): TestResponse
    {
        $headers['Accept'] = 'application/json';

        return $this->dispatch('GET', $uri, [], $headers, false, null);
    }

    protected function post(string $uri, array $data = [], array $headers = []): TestResponse
    {
        return $this->dispatch('POST', $uri, $data, $headers, false, null);
    }

    protected function postJson(string $uri, array $data = [], array $headers = []): TestResponse
    {
        $headers['Content-Type'] = 'application/json';
        $headers['Accept'] = $headers['Accept'] ?? 'application/json';
        $rawBody = (string) json_encode($data);

        return $this->dispatch('POST', $uri, [], $headers, true, $rawBody);
    }

    protected function postRawJson(string $uri, string $rawBody, array $headers = []): TestResponse
    {
        $headers['Content-Type'] = 'application/json';
        $headers['Accept'] = $headers['Accept'] ?? 'application/json';

        return $this->dispatch('POST', $uri, [], $headers, true, $rawBody);
    }

    protected function actingAs(array $overrides = []): array
    {
        $user = $this->createUser($overrides);

        Session::put('user_id', (int) $user['id']);
        Session::put('user_email', (string) $user['email']);
        Session::put('organization_id', $user['organization_id'] ?? null);
        Auth::setUserId(null);

        return $user;
    }

    protected function createUser(array $overrides = []): array
    {
        $email = $overrides['email'] ?? 'user_' . bin2hex(random_bytes(4)) . '@example.test';
        $name = $overrides['name'] ?? null;
        $organizationId = $overrides['organization_id'] ?? null;
        $role = $overrides['role'] ?? 'owner';
        $isSuperAdmin = (int) ($overrides['is_super_admin'] ?? 0);
        $stripeCustomerId = $overrides['stripe_customer_id'] ?? null;
        $themePreference = $overrides['theme_preference'] ?? null;

        $statement = Database::connection()->prepare(
            'INSERT INTO users (name, email, organization_id, role, is_super_admin, stripe_customer_id, theme_preference, created_at)
             VALUES (:name, :email, :organization_id, :role, :is_super_admin, :stripe_customer_id, :theme_preference, NOW())'
        );

        $statement->bindValue(':name', $name);
        $statement->bindValue(':email', $email);
        $statement->bindValue(':organization_id', $organizationId, $organizationId === null ? PDO::PARAM_NULL : PDO::PARAM_INT);
        $statement->bindValue(':role', $role);
        $statement->bindValue(':is_super_admin', $isSuperAdmin, PDO::PARAM_INT);
        $statement->bindValue(':stripe_customer_id', $stripeCustomerId);
        $statement->bindValue(':theme_preference', $themePreference);
        $statement->execute();

        $id = (int) Database::connection()->lastInsertId();

        $select = Database::connection()->prepare('SELECT * FROM users WHERE id = ? LIMIT 1');
        $select->execute([$id]);

        return $select->fetch() ?: [];
    }

    protected function createOrganization(string $name = 'Test Org'): array
    {
        $slug = strtolower(preg_replace('/[^a-z0-9]+/i', '-', $name)) . '-' . bin2hex(random_bytes(2));

        $statement = Database::connection()->prepare('INSERT INTO organizations (name, slug) VALUES (?, ?)');
        $statement->execute([$name, $slug]);

        $id = (int) Database::connection()->lastInsertId();
        $select = Database::connection()->prepare('SELECT * FROM organizations WHERE id = ? LIMIT 1');
        $select->execute([$id]);

        return $select->fetch() ?: [];
    }

    protected function createApiToken(int $userId, ?string $expiresAt = null): array
    {
        $rawToken = bin2hex(random_bytes(32));
        $hash = hash('sha256', $rawToken);

        $statement = Database::connection()->prepare(
            'INSERT INTO api_tokens (user_id, name, token_hash, abilities, expires_at, created_at)
             VALUES (?, ?, ?, ?, ?, NOW())'
        );
        $statement->execute([$userId, 'Feature Test Token', $hash, '*', $expiresAt]);

        return [
            'token' => $rawToken,
            'id' => (int) Database::connection()->lastInsertId(),
        ];
    }

    protected function csrfToken(): string
    {
        return Csrf::token();
    }

    protected function latestMailLog(): string
    {
        $path = self::$basePath . '/storage/logs/mail.log';

        return file_exists($path) ? (string) file_get_contents($path) : '';
    }

    protected function dispatch(
        string $method,
        string $uri,
        array $data,
        array $headers,
        bool $isJson,
        ?string $rawBody
    ): TestResponse {
        $serverBefore = $_SERVER;
        $getBefore = $_GET;
        $postBefore = $_POST;

        Database::resetConnection();

        $_SERVER['REQUEST_METHOD'] = strtoupper($method);
        $_SERVER['REQUEST_URI'] = $uri;
        $_SERVER['SERVER_PROTOCOL'] = 'HTTP/1.1';
        $_SERVER['REMOTE_ADDR'] = $_SERVER['REMOTE_ADDR'] ?? '127.0.0.1';

        [$path, $query] = array_pad(explode('?', $uri, 2), 2, '');
        $_SERVER['REQUEST_URI'] = $path . ($query !== '' ? '?' . $query : '');

        $_GET = [];
        if ($query !== '') {
            parse_str($query, $_GET);
        }

        $_POST = [];
        unset($_SERVER['CONTENT_TYPE'], $_SERVER['KEEL_RAW_BODY']);

        if ($isJson) {
            $_SERVER['CONTENT_TYPE'] = 'application/json';
            $_SERVER['KEEL_RAW_BODY'] = $rawBody ?? '{}';
        } elseif (in_array(strtoupper($method), ['POST', 'PUT', 'DELETE'], true)) {
            $_POST = $data;
        }

        foreach ($headers as $name => $value) {
            $serverKey = strtoupper(str_replace('-', '_', (string) $name));
            if ($serverKey === 'CONTENT_TYPE') {
                $_SERVER['CONTENT_TYPE'] = (string) $value;
                continue;
            }

            $_SERVER['HTTP_' . $serverKey] = (string) $value;
        }

        http_response_code(200);

        ob_start();

        try {
            $router = new Router();
            $routerFile = self::$basePath . '/routes/web.php';
            require $routerFile;

            $request = new Request();
            $result = $router->dispatch($request);
            $buffer = (string) ob_get_clean();

            if (is_string($result) || is_numeric($result)) {
                $buffer .= (string) $result;
            }

            return new TestResponse(http_response_code() ?: 200, [], $buffer);
        } catch (CapturedResponseException $capturedResponse) {
            $buffer = (string) ob_get_clean();
            $body = $capturedResponse->body !== '' ? $capturedResponse->body : $buffer;

            return new TestResponse($capturedResponse->status, $capturedResponse->headers, $body);
        } finally {
            $_SERVER = $serverBefore;
            $_GET = $getBefore;
            $_POST = $postBefore;
        }
    }

    private static function loadTestingEnvironment(): void
    {
        if (file_exists(self::$basePath . '/.env.testing')) {
            Dotenv::createImmutable(self::$basePath, '.env.testing')->safeLoad();
        }

        $testDatabase = trim((string) ($_ENV['DB_DATABASE_TEST'] ?? ''));
        if ($testDatabase === '') {
            throw new \RuntimeException('DB_DATABASE_TEST must be configured for feature tests.');
        }

        if (!preg_match('/(test|ci)/i', $testDatabase)) {
            throw new \RuntimeException('DB_DATABASE_TEST must clearly target a non-development database (expected name to include test/ci).');
        }

        $_ENV['APP_ENV'] = 'testing';
        $_SERVER['APP_ENV'] = 'testing';
        $_ENV['DB_DATABASE'] = $testDatabase;
        $_SERVER['DB_DATABASE'] = $testDatabase;
        $_ENV['MAIL_MAILER'] = $_ENV['MAIL_MAILER'] ?? 'log';
        $_SERVER['MAIL_MAILER'] = (string) $_ENV['MAIL_MAILER'];
        $_ENV['APP_URL'] = $_ENV['APP_URL'] ?? 'http://localhost';
        $_SERVER['APP_URL'] = (string) $_ENV['APP_URL'];
    }

    private static function prepareTestingDatabase(): void
    {
        $host = (string) ($_ENV['DB_HOST'] ?? '127.0.0.1');
        $port = (string) ($_ENV['DB_PORT'] ?? '3306');
        $username = (string) ($_ENV['DB_USERNAME'] ?? 'root');
        $password = (string) ($_ENV['DB_PASSWORD'] ?? '');
        $charset = (string) ($_ENV['DB_CHARSET'] ?? 'utf8mb4');
        $database = (string) $_ENV['DB_DATABASE'];

        $serverDsn = "mysql:host={$host};port={$port};charset={$charset}";
        $serverPdo = new PDO($serverDsn, $username, $password, [
            PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION,
            PDO::ATTR_DEFAULT_FETCH_MODE => PDO::FETCH_ASSOC,
            PDO::ATTR_EMULATE_PREPARES => false,
        ]);

        $safeDatabase = str_replace('`', '``', $database);
        $serverPdo->exec("CREATE DATABASE IF NOT EXISTS `{$safeDatabase}` CHARACTER SET {$charset} COLLATE {$charset}_unicode_ci");

        Database::resetConnection();
        $activeDatabase = (string) Database::connection()->query('SELECT DATABASE()')->fetchColumn();

        if ($activeDatabase !== $database) {
            throw new \RuntimeException('Feature tests are not connected to the configured test database.');
        }
    }

    private static function applyPendingMigrations(): void
    {
        $connection = Database::connection();
        $connection->exec(
            'CREATE TABLE IF NOT EXISTS migrations (
                id INT AUTO_INCREMENT PRIMARY KEY,
                name VARCHAR(255) NOT NULL UNIQUE,
                migrated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
            ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4'
        );

        $applied = [];
        $statement = $connection->query('SELECT name FROM migrations');
        foreach ($statement->fetchAll(PDO::FETCH_COLUMN) as $migrationName) {
            $applied[(string) $migrationName] = true;
        }

        $files = glob(self::$basePath . '/database/migrations/*.sql') ?: [];
        sort($files, SORT_NATURAL);

        foreach ($files as $file) {
            $name = basename($file);
            if (isset($applied[$name])) {
                continue;
            }

            $sql = trim((string) file_get_contents($file));
            if ($sql === '') {
                continue;
            }

            $connection->exec($sql);

            $insert = $connection->prepare('INSERT INTO migrations (name) VALUES (?)');
            $insert->execute([$name]);
        }
    }

    private function truncateApplicationTables(): void
    {
        $tables = [
            'activity_log',
            'api_tokens',
            'auth_tokens',
            'failed_jobs',
            'files',
            'jobs',
            'organization_invites',
            'organizations',
            'rate_limits',
            'subscriptions',
            'users',
        ];

        $connection = Database::connection();
        $connection->exec('SET FOREIGN_KEY_CHECKS=0');

        foreach ($tables as $table) {
            $connection->exec('TRUNCATE TABLE ' . $table);
        }

        $connection->exec('SET FOREIGN_KEY_CHECKS=1');
    }

    private function clearMailLog(): void
    {
        $path = self::$basePath . '/storage/logs/mail.log';

        if (file_exists($path)) {
            file_put_contents($path, '');
        }
    }
}
