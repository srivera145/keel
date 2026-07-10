<?php

namespace Keel\App\Middleware;

use Keel\App\Services\BillingService;
use Keel\Core\Middleware;
use Keel\Core\Request;
use Keel\Core\Response;
use Keel\Core\Session;

class SubscriptionMiddleware implements Middleware
{
    public function handle(Request $request, \Closure $next): mixed
    {
        $userId = (int) Session::get('user_id');
        $hasAccess = $userId > 0 && (new BillingService())->hasActiveSubscription($userId);

        if (!$hasAccess) {
            if ($request->wantsJson()) {
                Response::json(['error' => 'An active subscription is required.'], 402);
            }

            Response::redirect('/billing/upgrade');
        }

        return $next($request);
    }
}