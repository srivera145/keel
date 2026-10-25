<?php

declare(strict_types=1);

namespace Tests\Feature;

use Keel\Core\Database;
use Keel\Core\Migrations;
use PDO;
use Tests\TestCase;

/**
 * Migrations run one statement at a time, so a failure anywhere in a file is reported as itself and the file is not
 * recorded as applied. Runs against the test database with throwaway files and tables, removed afterwards.
 */
class MigrationRunnerFeatureTest extends TestCase
{
    private const TABLES = ['zz_runner_a', 'zz_runner_b', 'zz_runner_c'];

    private string $directory;

    protected function setUp(): void
    {
        parent::setUp();

        $this->directory = sys_get_temp_dir() . '/keel-migrations-' . bin2hex(random_bytes(4));
        mkdir($this->directory);
        $this->dropScratch();
    }

    protected function tearDown(): void
    {
        $this->dropScratch();

        foreach (glob($this->directory . '/*.sql') ?: [] as $file) {
            unlink($file);
        }
        rmdir($this->directory);

        parent::tearDown();
    }

    public function testAFailingLaterStatementIsReportedAndTheFileIsNotRecorded(): void
    {
        $this->write('900_runner_ok.sql', "CREATE TABLE zz_runner_a (id INT);\nINSERT INTO zz_runner_a (id) VALUES (1);\n");
        // With the whole file in one exec(), the SELECT's result set hid the INSERT's error: exec() returned, and the
        // next query failed with "Cannot execute queries while other unbuffered queries are active".
        $this->write('901_runner_bad.sql', "CREATE TABLE zz_runner_b (id INT);\nSELECT 1;\nINSERT INTO zz_runner_missing (id) VALUES (1);\nCREATE TABLE zz_runner_c (id INT);\n");

        try {
            Migrations::apply(Database::connection(), $this->directory);
            self::fail('The failing migration did not throw.');
        } catch (\RuntimeException $exception) {
            self::assertStringContainsString('901_runner_bad.sql failed at statement 3 of 4', $exception->getMessage());
            self::assertStringContainsString('zz_runner_missing', $exception->getMessage());
        }

        self::assertSame(['900_runner_ok.sql'], $this->recorded());
        self::assertSame(['zz_runner_a', 'zz_runner_b'], $this->scratchTables(), 'statements after the failure did not run');
    }

    public function testAppliedFilesAreSkippedAndAFixedFileAppliesOnTheNextRun(): void
    {
        $this->write('900_runner_ok.sql', 'CREATE TABLE zz_runner_a (id INT);');
        $this->write('901_runner_bad.sql', "CREATE TABLE zz_runner_b (id INT);\nINSERT INTO zz_runner_missing (id) VALUES (1);");

        try {
            Migrations::apply(Database::connection(), $this->directory);
        } catch (\RuntimeException) {
        }

        $this->write('901_runner_bad.sql', "CREATE TABLE IF NOT EXISTS zz_runner_b (id INT);\nCREATE TABLE zz_runner_c (id INT);");

        self::assertSame(['901_runner_bad.sql'], Migrations::apply(Database::connection(), $this->directory));
        self::assertSame(['900_runner_ok.sql', '901_runner_bad.sql'], $this->recorded());
        self::assertSame([], Migrations::apply(Database::connection(), $this->directory));
    }

    public function testStatementsThatReturnRowsDoNotBlockTheNextOne(): void
    {
        // The "add the column only if it is missing" idiom: EXECUTE runs a SELECT when the column exists.
        $this->write('900_runner_idiom.sql', "CREATE TABLE zz_runner_a (id INT);\n"
            . "SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'zz_runner_a' AND column_name = 'id') = 0, 'ALTER TABLE zz_runner_a ADD COLUMN id INT', 'SELECT 1');\n"
            . "PREPARE stmt FROM @sql;\nEXECUTE stmt;\nDEALLOCATE PREPARE stmt;\n"
            . "SELECT 2;\n"
            . "CREATE TABLE zz_runner_b (id INT);\n");

        self::assertSame(['900_runner_idiom.sql'], Migrations::apply(Database::connection(), $this->directory));
        self::assertSame(['zz_runner_a', 'zz_runner_b'], $this->scratchTables());
    }

    public function testPretendListsPendingFilesWithoutRunningThem(): void
    {
        $this->write('900_runner_ok.sql', 'CREATE TABLE zz_runner_a (id INT);');
        $lines = [];

        $pending = Migrations::apply(Database::connection(), $this->directory, static function (string $line) use (&$lines): void {
            $lines[] = $line;
        }, true);

        self::assertSame(['900_runner_ok.sql'], $pending);
        self::assertSame(['Would apply 900_runner_ok.sql'], $lines);
        self::assertSame([], $this->recorded());
        self::assertSame([], $this->scratchTables());
    }

    public function testTheConnectionsAttributesAreRestored(): void
    {
        $this->write('900_runner_bad.sql', 'INSERT INTO zz_runner_missing (id) VALUES (1);');
        $connection = Database::connection();
        $connection->setAttribute(PDO::ATTR_EMULATE_PREPARES, false);

        try {
            Migrations::apply($connection, $this->directory);
        } catch (\RuntimeException) {
        }

        self::assertFalse((bool) $connection->getAttribute(PDO::ATTR_EMULATE_PREPARES));
        self::assertSame(PDO::ERRMODE_EXCEPTION, $connection->getAttribute(PDO::ATTR_ERRMODE));
    }

    private function write(string $name, string $sql): void
    {
        file_put_contents($this->directory . '/' . $name, $sql);
    }

    /** @return list<string> */
    private function recorded(): array
    {
        $names = Database::connection()
            ->query("SELECT name FROM migrations WHERE name LIKE '9%runner%' ORDER BY name")
            ->fetchAll(PDO::FETCH_COLUMN);

        return array_map('strval', $names);
    }

    /** @return list<string> */
    private function scratchTables(): array
    {
        $tables = Database::connection()
            ->query("SELECT table_name FROM information_schema.tables WHERE table_schema = DATABASE() AND table_name LIKE 'zz\\_runner\\_%' ORDER BY table_name")
            ->fetchAll(PDO::FETCH_COLUMN);

        return array_map('strval', $tables);
    }

    private function dropScratch(): void
    {
        $connection = Database::connection();
        $connection->exec('DROP TABLE IF EXISTS ' . implode(', ', self::TABLES));
        $connection->exec("DELETE FROM migrations WHERE name LIKE '9%runner%'");
    }
}
