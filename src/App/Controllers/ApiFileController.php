<?php

namespace Keel\App\Controllers;

use Keel\App\Models\File;
use Keel\Core\Auth;
use Keel\Core\Controller;
use Keel\Core\Request;

class ApiFileController extends Controller
{
    public function index(Request $request): never
    {
        $userId = Auth::id();
        if ($userId === null) {
            $this->json(['error' => 'Unauthenticated.'], 401);
        }

        $this->json([
            'data' => File::forUser($userId),
        ]);
    }
}
