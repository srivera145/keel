<?php

namespace Keel\App\Middleware;

use Keel\App\Models\User;
use Keel\Core\Middleware;
use Keel\Core\Request;
use Keel\Core\Response;
use Keel\Core\Session;

class RequireOrgAdminMiddleware implements Middleware
{
    public function handle(Request $request, \Closure $next): mixed
    {
        $user = User::find((int) Session::get('user_id'));

        if (!$user || empty($user['organization_id'])) {
            if ($request->wantsJson()) {
                Response::json(['error' => 'Organization access is required.'], 403);
            }

            Response::redirect('/onboarding/organization');
        }

        if (!in_array((string) $user['role'], ['owner', 'admin'], true)) {
            if ($request->wantsJson()) {
                Response::json(['error' => 'Organization admin access is required.'], 403);
            }

            Response::redirect('/dashboard');
        }

        return $next($request);
    }
}