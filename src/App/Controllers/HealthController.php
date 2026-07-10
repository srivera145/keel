<?php

namespace Keel\App\Controllers;

use Keel\Core\Controller;
use Keel\Core\Database;
use Keel\Core\Request;

class HealthController extends Controller
{
    public function index(Request $request): void
    {
        try {
            Database::connection()->query('SELECT 1');
            $this->json(['status' => 'ok', 'database' => true], 200);
        } catch (\Throwable $exception) {
            $this->json(['status' => 'ok', 'database' => false], 503);
        }
    }
}