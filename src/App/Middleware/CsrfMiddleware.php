<?php

namespace Keel\App\Middleware;

use Keel\Core\Csrf;
use Keel\Core\Middleware;
use Keel\Core\Request;
use Keel\Core\Response;

class CsrfMiddleware implements Middleware
{
    public function handle(Request $request, \Closure $next): mixed
    {
        if (!in_array($request->method, ['POST', 'PUT', 'DELETE'], true)) {
            return $next($request);
        }

        $token = (string) ($request->input('_csrf')
            ?? $request->headers['X-CSRF-Token']
            ?? $request->headers['x-csrf-token']
            ?? $_SERVER['HTTP_X_CSRF_TOKEN']
            ?? '');

        if (!Csrf::verify($token)) {
            Response::abort(419, 'Page expired');
        }

        return $next($request);
    }
}