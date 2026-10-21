<?php

declare(strict_types=1);

namespace Tests\Unit;

use Keel\Core\Migrations;
use PHPUnit\Framework\TestCase;

/** How a migration file is split into the statements the runner executes one at a time. */
class MigrationsTest extends TestCase
{
    public function testSplitsOnSemicolonsBetweenStatements(): void
    {
        $sql = "CREATE TABLE a (id INT);\n\nALTER TABLE a ADD COLUMN b INT;\nINSERT INTO a (id) VALUES (1)";

        self::assertSame([
            'CREATE TABLE a (id INT)',
            'ALTER TABLE a ADD COLUMN b INT',
            'INSERT INTO a (id) VALUES (1)',
        ], Migrations::statements($sql));
    }

    public function testKeepsSemicolonsInsideStringsAndNames(): void
    {
        $sql = "INSERT INTO t (a, b) VALUES ('x;y', \"p;q\");\n"
            . "INSERT INTO t (a) VALUES ('it''s; fine');\n"
            . "INSERT INTO t (a) VALUES ('back\\'slash; too');\n"
            . "CREATE TABLE `odd;name` (id INT);";

        self::assertSame([
            "INSERT INTO t (a, b) VALUES ('x;y', \"p;q\")",
            "INSERT INTO t (a) VALUES ('it''s; fine')",
            "INSERT INTO t (a) VALUES ('back\\'slash; too')",
            'CREATE TABLE `odd;name` (id INT)',
        ], Migrations::statements($sql));
    }

    public function testDropsLineCommentsAndSkipsCommentOnlyStatements(): void
    {
        $sql = "-- A header comment; with a semicolon.\n"
            . "CREATE TABLE a (id INT); -- trailing; comment\n"
            . "# hash comment;\n"
            . "/* a block comment; alone */;\n"
            . "\n-- nothing after this\n";

        self::assertSame(['CREATE TABLE a (id INT)'], Migrations::statements($sql));
    }

    public function testKeepsBlockAndExecutableComments(): void
    {
        $sql = "/*!40101 SET NAMES utf8mb4 */;\nSELECT 1 /* inline; note */ + 1;";

        self::assertSame([
            '/*!40101 SET NAMES utf8mb4 */',
            'SELECT 1 /* inline; note */ + 1',
        ], Migrations::statements($sql));
    }

    public function testADoubleDashWithoutASpaceIsNotAComment(): void
    {
        self::assertSame(['SELECT 1--1'], Migrations::statements('SELECT 1--1;'));
    }

    public function testEmptyInputHasNoStatements(): void
    {
        self::assertSame([], Migrations::statements(''));
        self::assertSame([], Migrations::statements("  \n;\n ; "));
    }

    public function testEveryKeelMigrationSplitsIntoStatements(): void
    {
        $files = glob(dirname(__DIR__, 2) . '/database/migrations/*.sql') ?: [];
        self::assertNotEmpty($files);

        foreach ($files as $file) {
            $statements = Migrations::statements((string) file_get_contents($file));
            self::assertNotEmpty($statements, basename($file));

            foreach ($statements as $statement) {
                self::assertMatchesRegularExpression('/^(CREATE|ALTER|INSERT|UPDATE|DROP)\b/i', $statement, basename($file));
            }
        }
    }
}
