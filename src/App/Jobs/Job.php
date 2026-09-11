<?php

namespace Keel\App\Jobs;

interface Job
{
    public function handle(array $data): void;
}
