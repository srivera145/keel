<?php

use Keel\Core\Env;
use Keel\Core\ErrorHandler;
use Keel\Core\Request;
use Keel\Core\Router;
use Keel\Core\Session;
use Keel\Core\View;

require __DIR__ . '/../vendor/autoload.php';

$basePath = dirname(__DIR__);

Env::load($basePath);

error_reporting(Env::get('APP_ENV') === 'production' ? 0 : E_ALL);
ini_set('display_errors', Env::get('APP_DEBUG', false) ? '1' : '0');

Session::start();
View::setPath($basePath . '/views');

header('X-Content-Type-Options: nosniff');
header('X-Frame-Options: DENY');
header('Referrer-Policy: strict-origin-when-cross-origin');

$router = new Router();
require $basePath . '/routes/web.php';

$request = new Request();

try {
	$router->dispatch($request);
} catch (\Throwable $e) {
	ErrorHandler::render(500, $e);
}
