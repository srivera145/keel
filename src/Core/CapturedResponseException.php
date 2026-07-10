<?php

namespace Keel\Core;

class CapturedResponseException extends \RuntimeException
{
    public function __construct(
        public readonly int $status,
        public readonly array $headers,
        public readonly string $body
    ) {
        parent::__construct('Captured HTTP response for test harness.');
    }
}
