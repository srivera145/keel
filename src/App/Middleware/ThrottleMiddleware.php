<?php

namespace Keel\App\Middleware;

use Keel\Core\Middleware;
use Keel\Core\RateLimiter;
use Keel\Core\Request;
use Keel\Core\Response;

class ThrottleMiddleware implements Middleware
{
    private const MAX_ATTEMPTS = 30;
    private const DECAY_MINUTES = 1;

    public function handle(Request $request, \Closure $next): mixed
    {
        $ipAddress = $this->clientIp();
        $key = strtolower($ipAddress . '|' . $request->uri);

        if (!RateLimiter::attempt($key, self::MAX_ATTEMPTS, self::DECAY_MINUTES)) {
            if ($request->wantsJson()) {
                Response::json(['error' => 'Too many requests. Please try again later.'], 429);
            }

            Response::abort(429, 'Too many requests');
        }

        return $next($request);
    }

    private function clientIp(): string
    {
        return trim((string) ($_SERVER['REMOTE_ADDR'] ?? 'unknown')) ?: 'unknown';
    }
}