<?php

namespace Keel\App\Controllers;

use Keel\App\Models\User;
use Keel\Core\Auth;
use Keel\Core\Controller;
use Keel\Core\Request;

class DashboardController extends Controller
{
    public function index(Request $request): void
    {
        $user = User::find((int) Auth::id());
        $this->view('dashboard.index', ['user' => $user]);
    }
}
