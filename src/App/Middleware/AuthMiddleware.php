<?php

namespace Keel\App\Middleware;

use Keel\App\Models\User;
use Keel\Core\Middleware;
use Keel\Core\Request;
use Keel\Core\Response;
use Keel\Core\Session;
use Keel\Core\Theme;

class AuthMiddleware implements Middleware
{
    public function handle(Request $request, \Closure $next): mixed
    {
        if (!Session::has('user_id')) {
            if ($request->wantsJson()) {
                Response::json(['error' => 'Unauthenticated.'], 401);
            }
            Response::redirect('/login');
        }

        $user = User::find((int) Session::get('user_id'));

        if ($user === null) {
            Session::destroy();

            if ($request->wantsJson()) {
                Response::json(['error' => 'Unauthenticated.'], 401);
            }

            Response::redirect('/login');
        }

        Session::put('theme_preference', Theme::normalize($user['theme_preference'] ?? null));

        return $next($request);
    }
}
