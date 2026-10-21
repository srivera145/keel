<?php

declare(strict_types=1);

namespace Tests\Unit;

use Keel\Core\Env;
use PHPUnit\Framework\TestCase;

class EnvTest extends TestCase
{
    private mixed $originalMissing;
    private mixed $originalBool;

    protected function setUp(): void
    {
        parent::setUp();
        $this->originalMissing = $_ENV['__TEST_MISSING__'] ?? null;
        $this->originalBool = $_ENV['__TEST_BOOL__'] ?? null;
    }

    protected function tearDown(): void
    {
        if ($this->originalMissing === null) {
            unset($_ENV['__TEST_MISSING__']);
        } else {
            $_ENV['__TEST_MISSING__'] = $this->originalMissing;
        }

        if ($this->originalBool === null) {
            unset($_ENV['__TEST_BOOL__']);
        } else {
            $_ENV['__TEST_BOOL__'] = $this->originalBool;
        }

        parent::tearDown();
    }

    public function testGetReturnsDefaultWhenKeyMissing(): void
    {
        unset($_ENV['__TEST_MISSING__']);

        self::assertSame('fallback', Env::get('__TEST_MISSING__', 'fallback'));
    }

    public function testLoadSetsPhpsClockToUtc(): void
    {
        $loaded = new \ReflectionProperty(Env::class, 'loaded');
        $wasLoaded = $loaded->getValue();
        $zone = date_default_timezone_get();
        $emptyDirectory = sys_get_temp_dir() . '/keel-env-test-' . bin2hex(random_bytes(4));
        mkdir($emptyDirectory);

        try {
            date_default_timezone_set('America/Denver');
            $loaded->setValue(null, false);

            Env::load($emptyDirectory);

            self::assertSame('UTC', date_default_timezone_get());
        } finally {
            $loaded->setValue(null, $wasLoaded);
            date_default_timezone_set($zone);
            rmdir($emptyDirectory);
        }
    }

    public function testGetCastsStringBooleansToNativeBooleans(): void
    {
        $_ENV['__TEST_BOOL__'] = 'true';
        self::assertTrue(Env::get('__TEST_BOOL__'));

        $_ENV['__TEST_BOOL__'] = 'false';
        self::assertFalse(Env::get('__TEST_BOOL__'));
    }
}