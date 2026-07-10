<?php

namespace Keel\App\Controllers;

use Keel\App\Services\BillingService;
use Keel\Core\Controller;
use Keel\Core\Env;
use Keel\Core\Request;
use Keel\Core\Response;
use Stripe\Exception\SignatureVerificationException;
use Stripe\Webhook;

class StripeWebhookController extends Controller
{
    public function handle(Request $request): never
    {
        $payload = $request->rawBody();
        $signature = $request->headers['Stripe-Signature'] ?? $_SERVER['HTTP_STRIPE_SIGNATURE'] ?? '';
        $webhookSecret = trim((string) Env::get('STRIPE_WEBHOOK_SECRET', ''));

        if ($payload === '' || $signature === '' || $webhookSecret === '') {
            Response::json(['error' => 'Invalid webhook payload.'], 400);
        }

        try {
            $event = Webhook::constructEvent($payload, $signature, $webhookSecret);
        } catch (\UnexpectedValueException|SignatureVerificationException $exception) {
            Response::json(['error' => 'Webhook signature verification failed.'], 400);
        }

        (new BillingService())->syncSubscriptionFromWebhook($event);

        Response::json(['received' => true]);
    }
}