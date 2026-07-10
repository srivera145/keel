<?php

namespace Keel\App\Middleware;

use Keel\App\Models\User;
use Keel\Core\Env;
use Keel\Core\Middleware;
use Keel\Core\Request;
use Keel\Core\Response;
use Keel\Core\Session;

class RequireOrganizationMiddleware implements Middleware
{
    public function handle(Request $request, \Closure $next): mixed
    {
        if (!(bool) Env::get('MULTI_TENANCY_ENABLED', false)) {
            return $next($request);
        }

        $user = User::find((int) Session::get('user_id'));
        if (!$user) {
            if ($request->wantsJson()) {
                Response::json(['error' => 'Unauthenticated.'], 401);
            }

            Response::redirect('/login');
        }

        if (empty($user['organization_id'])) {
            if ($request->wantsJson()) {
                Response::json(['error' => 'Organization onboarding is required.'], 403);
            }

            Response::redirect('/onboarding/organization');
        }

        return $next($request);
    }
}