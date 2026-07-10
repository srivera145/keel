<?php

namespace Keel\App\Controllers;

use Keel\Core\Controller;
use Keel\Core\Request;

class WelcomeController extends Controller
{
    public function index(Request $request): void
    {
        $this->view('welcome', ['title' => 'Keel - Open-Source PHP Starter Kit']);
    }
}