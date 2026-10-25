<?php

namespace Keel\Core;

use PDO;

/**
 * Applies the .sql files in database/migrations in natural order, recording each in the migrations table. Used by
 * database/migrate.php, scripts/setup_test_db.php and the test base class.
 *
 * A file runs one statement at a time. A whole file in one PDO::exec() does not reliably report a failure further
 * down: once a statement returns a result set (a SELECT, or EXECUTE of a prepared SELECT in the "add the column
 * only if it is missing" idiom), exec() returns without an exception, a later statement's error is lost, and the
 * next query fails with "Cannot execute queries while other unbuffered queries are active" instead of the real
 * error. Here the failing statement throws with its own error and its number in the file, and the file is not
 * recorded as applied. Each statement goes through query() over the text protocol and its result is closed, so a
 * statement that returns rows does not block the next one.
 *
 * Statements are split on semicolons outside quotes, backticks and comments. DELIMITER blocks (stored routines and
 * triggers with semicolons in their bodies) are not supported.
 */
class Migrations
{
    /**
     * @param callable(string): void|null $log receives one progress line at a time
     * @return list<string> the migrations applied (or, when pretending, the ones pending)
     */
    public static function apply(PDO $pdo, string $directory, ?callable $log = null, bool $pretend = false): array
    {
        $log ??= static function (string $line): void {
        };

        $pdo->exec(
            'CREATE TABLE IF NOT EXISTS migrations (
                id INT AUTO_INCREMENT PRIMARY KEY,
                name VARCHAR(255) NOT NULL UNIQUE,
                migrated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
            ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4'
        );

        $applied = [];
        foreach ($pdo->query('SELECT name FROM migrations')->fetchAll(PDO::FETCH_COLUMN) as $name) {
            $applied[(string) $name] = true;
        }

        $files = glob(rtrim($directory, '/\\') . '/*.sql') ?: [];
        sort($files, SORT_NATURAL);

        $ran = [];
        foreach ($files as $file) {
            $name = basename($file);
            if (isset($applied[$name])) {
                continue;
            }

            $statements = self::statements((string) file_get_contents($file));
            if ($statements === []) {
                $log("Skipping empty migration {$name}");
                continue;
            }

            $ran[] = $name;
            if ($pretend) {
                $log("Would apply {$name}");
                continue;
            }

            $log("Applying {$name}...");
            self::run($pdo, $name, $statements);
            $pdo->prepare('INSERT INTO migrations (name) VALUES (?)')->execute([$name]);
            $log("Applied {$name}");
        }

        return $ran;
    }

    /** @param list<string> $statements */
    private static function run(PDO $pdo, string $name, array $statements): void
    {
        $errorMode = $pdo->getAttribute(PDO::ATTR_ERRMODE);
        $emulate = $pdo->getAttribute(PDO::ATTR_EMULATE_PREPARES);
        $pdo->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
        // The text protocol takes every statement a migration may hold, PREPARE and EXECUTE included; server-side
        // prepares do not.
        $pdo->setAttribute(PDO::ATTR_EMULATE_PREPARES, true);

        try {
            foreach ($statements as $index => $sql) {
                try {
                    $pdo->query($sql)->closeCursor();
                } catch (\PDOException $exception) {
                    throw new \RuntimeException(
                        "Migration {$name} failed at statement " . ($index + 1) . ' of ' . count($statements) . ': ' . $exception->getMessage(),
                        0,
                        $exception
                    );
                }
            }
        } finally {
            $pdo->setAttribute(PDO::ATTR_EMULATE_PREPARES, $emulate);
            $pdo->setAttribute(PDO::ATTR_ERRMODE, $errorMode);
        }
    }

    /**
     * Splits SQL into statements on semicolons that are not inside a quoted string, a backticked name or a
     * comment. Line comments (-- and #) are dropped; block comments are kept, so MySQL's executable comments
     * (/*! ... *\/) still run. A statement that is only whitespace and comments is skipped.
     *
     * @return list<string>
     */
    public static function statements(string $sql): array
    {
        $statements = [];
        $current = '';
        $hasCode = false;
        $length = strlen($sql);

        for ($i = 0; $i < $length; $i++) {
            $char = $sql[$i];
            $next = $i + 1 < $length ? $sql[$i + 1] : '';

            // Line comments: "-- " (a dash pair followed by whitespace or the end) and "#".
            if ($char === '#' || ($char === '-' && $next === '-' && ($i + 2 >= $length || ctype_space($sql[$i + 2])))) {
                $end = strpos($sql, "\n", $i);
                $i = $end === false ? $length : $end - 1;
                continue;
            }

            // Block comments, kept verbatim.
            if ($char === '/' && $next === '*') {
                $end = strpos($sql, '*/', $i + 2);
                $end = $end === false ? $length : $end + 2;
                $comment = substr($sql, $i, $end - $i);
                $current .= $comment;
                $hasCode = $hasCode || str_starts_with($comment, '/*!');
                $i = $end - 1;
                continue;
            }

            // Quoted strings and names: copied through to the matching quote. A backslash escapes the next
            // character inside '...' and "...", and a doubled quote is a literal quote.
            if ($char === "'" || $char === '"' || $char === '`') {
                $start = $i;
                $i++;
                while ($i < $length) {
                    if ($sql[$i] === '\\' && $char !== '`') {
                        $i += 2;
                        continue;
                    }
                    if ($sql[$i] === $char) {
                        if ($i + 1 < $length && $sql[$i + 1] === $char) {
                            $i += 2;
                            continue;
                        }
                        break;
                    }
                    $i++;
                }
                $current .= substr($sql, $start, min($i, $length - 1) - $start + 1);
                $hasCode = true;
                continue;
            }

            if ($char === ';') {
                if ($hasCode) {
                    $statements[] = trim($current);
                }
                $current = '';
                $hasCode = false;
                continue;
            }

            $current .= $char;
            $hasCode = $hasCode || !ctype_space($char);
        }

        if ($hasCode) {
            $statements[] = trim($current);
        }

        return $statements;
    }
}
