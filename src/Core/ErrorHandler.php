<?php

namespace Keel\Core;

class ErrorHandler
{
    /**
     * The 404 or 500 response. It goes out through Response::raw rather than exit, so under the test harness
     * (capture mode) an error page is a response the test can inspect instead of the end of the PHPUnit run.
     */
    public static function render(int $status, ?\Throwable $e = null): never
    {
        if ($e !== null) {
            error_log('[Keel] Uncaught exception: ' . $e::class . ': ' . $e->getMessage() . ' in ' . $e->getFile() . ':' . $e->getLine());
        }

        $status = $status === 404 ? 404 : 500;

        // API clients (routes under /api) get JSON in the API's error shape, never an HTML page or exception details.
        $path = (string) (parse_url((string) ($_SERVER['REQUEST_URI'] ?? '/'), PHP_URL_PATH) ?: '/');
        if ($path === '/api' || str_starts_with($path, '/api/')) {
            Response::raw(
                (string) json_encode(['error' => $status === 404 ? 'Not found.' : 'Server error.']),
                $status,
                ['Content-Type' => 'application/json']
            );
        }

        $debug = (bool) Env::get('APP_DEBUG', false);
        $exception = $debug ? $e : null;
        $title = $status === 404 ? 'Page Not Found' : 'Server Error';
        $template = $status === 404 ? 'errors.404' : 'errors.500';

        $level = ob_get_level();
        ob_start();

        try {
            View::render($template, [
                'title' => $title,
                'exception' => $exception,
            ]);
        } catch (\Throwable $viewException) {
            // Drop whatever the failed view printed (and any buffers it left open) before the plain fallback.
            while (ob_get_level() > $level + 1) {
                ob_end_clean();
            }
            ob_clean();
            echo $status === 404 ? 'Page not found' : 'Server error';
        }

        Response::raw((string) ob_get_clean(), $status, ['Content-Type' => 'text/html; charset=UTF-8']);
    }
}
