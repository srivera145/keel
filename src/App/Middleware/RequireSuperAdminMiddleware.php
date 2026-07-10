<?php

namespace Keel\App\Middleware;

use Keel\App\Models\User;
use Keel\Core\Middleware;
use Keel\Core\Request;
use Keel\Core\Response;
use Keel\Core\Session;

class RequireSuperAdminMiddleware implements Middleware
{
    public function handle(Request $request, \Closure $next): mixed
    {
        $user = User::find((int) Session::get('user_id'));

        if (!$user || (int) ($user['is_super_admin'] ?? 0) !== 1) {
            if ($request->wantsJson()) {
                Response::json(['error' => 'Super admin access is required.'], 403);
            }

            Response::redirect('/dashboard');
        }

        return $next($request);
    }
}