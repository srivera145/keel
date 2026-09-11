<?php

namespace Keel\App\Services;

use Keel\App\Models\User;
use Keel\Core\Activity;
use Keel\Core\Database;
use Keel\Core\Env;
use Stripe\Checkout\Session;
use Stripe\BillingPortal\Session as BillingPortalSession;
use Stripe\Checkout\Session as CheckoutSession;
use Stripe\Event;
use Stripe\StripeObject;
use Stripe\StripeClient;
use Stripe\Subscription;

class BillingService
{
    private ?StripeClient $stripe = null;

    public function createCheckoutSession(array $user, string $priceId): CheckoutSession
    {
        if ($priceId === '') {
            throw new \RuntimeException('Stripe price ID is required.');
        }

        $customerId = $this->ensureCustomerId($user);
        $plan = $this->resolvePlanFromPriceId($priceId);

        return $this->stripe()->checkout->sessions->create([
            'mode' => 'subscription',
            'customer' => $customerId,
            'client_reference_id' => (string) $user['id'],
            'success_url' => $this->appUrl('/billing/success?session_id={CHECKOUT_SESSION_ID}'),
            'cancel_url' => $this->appUrl('/billing/cancel'),
            'line_items' => [[
                'price' => $priceId,
                'quantity' => 1,
            ]],
            'metadata' => [
                'user_id' => (string) $user['id'],
                'plan' => $plan,
            ],
            'subscription_data' => [
                'metadata' => [
                    'user_id' => (string) $user['id'],
                    'plan' => $plan,
                ],
            ],
        ]);
    }

    public function createPortalSession(array $user): BillingPortalSession
    {
        $customerId = trim((string) ($user['stripe_customer_id'] ?? ''));

        if ($customerId === '') {
            throw new \RuntimeException('No Stripe customer exists for this user yet.');
        }

        return $this->stripe()->billingPortal->sessions->create([
            'customer' => $customerId,
            'return_url' => $this->appUrl('/billing/upgrade'),
        ]);
    }

    public function syncSubscriptionFromWebhook(Event $event): void
    {
        $type = $event->type;

        if ($type === 'checkout.session.completed') {
            /** @var CheckoutSession $session */
            $session = $event->data->object;

            if (($session->mode ?? null) !== 'subscription' || empty($session->subscription)) {
                return;
            }

            $subscription = $this->resolveCheckoutSubscription($session);
            if ($subscription === null) {
                return;
            }

            $this->syncStripeSubscription($subscription, [
                'user_id' => (string) ($session->metadata->user_id ?? $session->client_reference_id ?? ''),
                'plan' => (string) ($session->metadata->plan ?? ''),
                'activity_action' => 'subscription.created',
            ]);
            return;
        }

        if ($type === 'customer.subscription.updated' || $type === 'customer.subscription.deleted') {
            /** @var Subscription $subscription */
            $subscription = $event->data->object;
            $this->syncStripeSubscription($subscription, [
                'activity_action' => $type === 'customer.subscription.deleted' ? 'subscription.canceled' : 'subscription.updated',
            ]);
            return;
        }

        if ($type === 'invoice.payment_failed') {
            $invoice = $event->data->object;
            $subscriptionId = (string) ($invoice->subscription ?? '');

            if ($subscriptionId === '') {
                return;
            }

            $subscription = $this->stripe()->subscriptions->retrieve($subscriptionId, []);
            $this->syncStripeSubscription($subscription, [
                'activity_action' => 'subscription.updated',
            ]);
        }
    }

    public function currentSubscriptionForUser(int $userId): ?array
    {
        $statement = Database::connection()->prepare(
            'SELECT * FROM subscriptions WHERE user_id = ? ORDER BY updated_at DESC, id DESC LIMIT 1'
        );
        $statement->execute([$userId]);

        return $statement->fetch() ?: null;
    }

    public function hasActiveSubscription(int $userId): bool
    {
        $statement = Database::connection()->prepare(
            "SELECT 1 FROM subscriptions WHERE user_id = ? AND status IN ('active', 'trialing') ORDER BY id DESC LIMIT 1"
        );
        $statement->execute([$userId]);

        return (bool) $statement->fetchColumn();
    }

    private function ensureCustomerId(array $user): string
    {
        $existingCustomerId = trim((string) ($user['stripe_customer_id'] ?? ''));

        if ($existingCustomerId !== '') {
            return $existingCustomerId;
        }

        $customer = $this->stripe()->customers->create([
            'email' => (string) $user['email'],
            'name' => (string) ($user['name'] ?? $user['email']),
            'metadata' => [
                'user_id' => (string) $user['id'],
            ],
        ]);

        $statement = Database::connection()->prepare('UPDATE users SET stripe_customer_id = ? WHERE id = ?');
        $statement->execute([$customer->id, $user['id']]);

        return $customer->id;
    }

    private function syncStripeSubscription(Subscription $subscription, array $context = []): void
    {
        $customerId = (string) ($subscription->customer ?? '');
        $user = $this->findUserForSubscription($customerId, $context);

        if (!$user) {
            throw new \RuntimeException('Unable to resolve user for Stripe subscription sync.');
        }

        $priceId = (string) ($subscription->items->data[0]->price->id ?? '');
        $plan = trim((string) ($subscription->metadata->plan ?? $context['plan'] ?? ''));

        if ($plan === '') {
            $plan = $this->resolvePlanFromPriceId($priceId);
        }

        if ($customerId !== '' && trim((string) ($user['stripe_customer_id'] ?? '')) === '') {
            $statement = Database::connection()->prepare('UPDATE users SET stripe_customer_id = ? WHERE id = ?');
            $statement->execute([$customerId, $user['id']]);
        }

        $statement = Database::connection()->prepare(
            'INSERT INTO subscriptions (user_id, stripe_subscription_id, stripe_price_id, plan, status, current_period_end, cancel_at_period_end)
             VALUES (:user_id, :stripe_subscription_id, :stripe_price_id, :plan, :status, :current_period_end, :cancel_at_period_end)
             ON DUPLICATE KEY UPDATE
                user_id = VALUES(user_id),
                stripe_price_id = VALUES(stripe_price_id),
                plan = VALUES(plan),
                status = VALUES(status),
                current_period_end = VALUES(current_period_end),
                cancel_at_period_end = VALUES(cancel_at_period_end),
                updated_at = CURRENT_TIMESTAMP'
        );

        $statement->execute([
            'user_id' => $user['id'],
            'stripe_subscription_id' => (string) $subscription->id,
            'stripe_price_id' => $priceId,
            'plan' => $plan,
            'status' => (string) $subscription->status,
            'current_period_end' => $this->formatTimestamp($subscription->current_period_end ?? null),
            'cancel_at_period_end' => !empty($subscription->cancel_at_period_end) ? 1 : 0,
        ]);

        $action = (string) ($context['activity_action'] ?? 'subscription.updated');
        $localId = $this->localSubscriptionId((string) $subscription->id);

        Activity::log($action, 'Subscription', $localId, [
            'stripe_subscription_id' => (string) $subscription->id,
            'status' => (string) $subscription->status,
            'plan' => $plan,
        ]);
    }

    private function findUserForSubscription(string $customerId, array $context = []): ?array
    {
        if ($customerId !== '') {
            $statement = Database::connection()->prepare('SELECT * FROM users WHERE stripe_customer_id = ? LIMIT 1');
            $statement->execute([$customerId]);
            $user = $statement->fetch();

            if ($user) {
                return $user;
            }
        }

        $userId = (int) ($context['user_id'] ?? 0);

        if ($userId > 0) {
            return User::find($userId);
        }

        return null;
    }

    private function resolvePlanFromPriceId(string $priceId): string
    {
        if ($priceId === '') {
            return 'unknown';
        }

        $sources = [$_ENV, $_SERVER];

        foreach ($sources as $source) {
            foreach ($source as $key => $value) {
                if (!is_string($key) || !str_starts_with($key, 'STRIPE_PRICE_')) {
                    continue;
                }

                if ((string) $value === $priceId) {
                    return strtolower(substr($key, strlen('STRIPE_PRICE_')));
                }
            }
        }

        return $priceId;
    }

    private function stripe(): StripeClient
    {
        if ($this->stripe !== null) {
            return $this->stripe;
        }

        $secretKey = trim((string) Env::get('STRIPE_SECRET_KEY', ''));

        if ($secretKey === '') {
            throw new \RuntimeException('STRIPE_SECRET_KEY is not configured.');
        }

        $this->stripe = new StripeClient($secretKey);

        return $this->stripe;
    }

    private function appUrl(string $path): string
    {
        return rtrim((string) Env::get('APP_URL', ''), '/') . $path;
    }

    private function formatTimestamp(?int $timestamp): ?string
    {
        if (!$timestamp) {
            return null;
        }

        return gmdate('Y-m-d H:i:s', $timestamp);
    }

    private function localSubscriptionId(string $stripeSubscriptionId): ?int
    {
        if ($stripeSubscriptionId === '') {
            return null;
        }

        $statement = Database::connection()->prepare('SELECT id FROM subscriptions WHERE stripe_subscription_id = ? LIMIT 1');
        $statement->execute([$stripeSubscriptionId]);
        $id = $statement->fetchColumn();

        return $id !== false ? (int) $id : null;
    }

    private function resolveCheckoutSubscription(Session $session): ?Subscription
    {
        $reference = $session->subscription;

        if ($reference instanceof Subscription) {
            return $reference;
        }

        if ($reference instanceof StripeObject) {
            return Subscription::constructFrom($reference->toArray());
        }

        if (is_array($reference)) {
            return Subscription::constructFrom($reference);
        }

        $subscriptionId = trim((string) $reference);
        if ($subscriptionId === '') {
            return null;
        }

        return $this->stripe()->subscriptions->retrieve($subscriptionId, []);
    }
}