<?php

namespace Keel\App\Controllers;

use Keel\App\Models\User;
use Keel\App\Services\BillingService;
use Keel\Core\Auth;
use Keel\Core\Controller;
use Keel\Core\Env;
use Keel\Core\Request;

class BillingController extends Controller
{
    public function showPlans(Request $request): void
    {
        $user = $this->currentUser();
        $billing = new BillingService();

        $this->view('billing.plans', [
            'title' => 'Billing',
            'plans' => $this->plans(),
            'currentSubscription' => $billing->currentSubscriptionForUser((int) $user['id']),
            'publishableKey' => Env::get('STRIPE_PUBLISHABLE_KEY', ''),
        ]);
    }

    public function checkout(Request $request): void
    {
        $plan = trim((string) $request->input('plan'));
        $priceId = $this->plans()[$plan] ?? '';

        if ($priceId === '') {
            $this->redirect('/billing/upgrade?error=invalid_plan');
        }

        $session = (new BillingService())->createCheckoutSession($this->currentUser(), $priceId);
        $this->redirect((string) $session->url);
    }

    public function success(Request $request): void
    {
        $this->view('billing.success', ['title' => 'Billing Success']);
    }

    public function cancel(Request $request): void
    {
        $this->view('billing.cancel', ['title' => 'Billing Cancelled']);
    }

    public function portal(Request $request): void
    {
        $session = (new BillingService())->createPortalSession($this->currentUser());
        $this->redirect((string) $session->url);
    }

    private function plans(): array
    {
        return [
            'pro_monthly' => trim((string) Env::get('STRIPE_PRICE_PRO_MONTHLY', '')),
        ];
    }

    private function currentUser(): array
    {
        $user = User::find((int) Auth::id());

        if (!$user) {
            throw new \RuntimeException('Authenticated user could not be loaded.');
        }

        return $user;
    }
}