<?php

namespace Keel\App\Middleware;

use Keel\Core\Auth;
use Keel\Core\Database;
use Keel\Core\Middleware;
use Keel\Core\Request;
use Keel\Core\Response;

class ApiAuthMiddleware implements Middleware
{
    public function handle(Request $request, \Closure $next): mixed
    {
        $header = $request->headers['Authorization']
            ?? $request->headers['authorization']
            ?? $_SERVER['HTTP_AUTHORIZATION']
            ?? '';

        if (!is_string($header) || !preg_match('/^Bearer\s+(.+)$/i', trim($header), $matches)) {
            Response::json(['error' => 'Unauthenticated.'], 401);
        }

        $presentedToken = trim((string) ($matches[1] ?? ''));
        if ($presentedToken === '') {
            Response::json(['error' => 'Unauthenticated.'], 401);
        }

        $tokenHash = hash('sha256', $presentedToken);

        $statement = Database::connection()->prepare(
            'SELECT id, user_id, expires_at FROM api_tokens WHERE token_hash = ? LIMIT 1'
        );
        $statement->execute([$tokenHash]);
        $token = $statement->fetch();

        if (!$token) {
            Response::json(['error' => 'Unauthenticated.'], 401);
        }

        if (!empty($token['expires_at']) && strtotime((string) $token['expires_at']) <= time()) {
            Response::json(['error' => 'Token expired.'], 401);
        }

        $touch = Database::connection()->prepare('UPDATE api_tokens SET last_used_at = NOW() WHERE id = ?');
        $touch->execute([(int) $token['id']]);

        Auth::setUserId((int) $token['user_id']);

        return $next($request);
    }
}
