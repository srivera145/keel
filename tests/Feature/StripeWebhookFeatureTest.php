<?php

declare(strict_types=1);

namespace Tests\Feature;

use Keel\Core\Database;
use Tests\TestCase;

class StripeWebhookFeatureTest extends TestCase
{
    public function testCheckoutWebhookWithValidSignatureCreatesSubscriptionRow(): void
    {
        $_ENV['STRIPE_WEBHOOK_SECRET'] = 'whsec_feature_test';
        $_SERVER['STRIPE_WEBHOOK_SECRET'] = 'whsec_feature_test';

        $user = $this->createUser([
            'email' => 'billing@example.test',
            'stripe_customer_id' => 'cus_feature_001',
        ]);

        $payload = [
            'id' => 'evt_checkout_completed',
            'object' => 'event',
            'type' => 'checkout.session.completed',
            'data' => [
                'object' => [
                    'id' => 'cs_test_123',
                    'object' => 'checkout.session',
                    'mode' => 'subscription',
                    'client_reference_id' => (string) $user['id'],
                    'metadata' => [
                        'user_id' => (string) $user['id'],
                        'plan' => 'pro_monthly',
                    ],
                    'subscription' => [
                        'id' => 'sub_feature_001',
                        'object' => 'subscription',
                        'customer' => 'cus_feature_001',
                        'status' => 'active',
                        'current_period_end' => time() + 3600,
                        'cancel_at_period_end' => false,
                        'metadata' => [
                            'plan' => 'pro_monthly',
                        ],
                        'items' => [
                            'object' => 'list',
                            'data' => [
                                [
                                    'price' => [
                                        'id' => 'price_pro_monthly',
                                    ],
                                ],
                            ],
                        ],
                    ],
                ],
            ],
        ];

        $rawPayload = (string) json_encode($payload);
        $timestamp = time();
        $signatureHash = hash_hmac('sha256', $timestamp . '.' . $rawPayload, 'whsec_feature_test');
        $signature = 't=' . $timestamp . ',v1=' . $signatureHash;

        $response = $this->postRawJson('/webhooks/stripe', $rawPayload, [
            'Stripe-Signature' => $signature,
        ]);

        self::assertSame(200, $response->status);
        self::assertTrue((bool) ($response->json()['received'] ?? false));

        $statement = Database::connection()->prepare('SELECT * FROM subscriptions WHERE stripe_subscription_id = ? LIMIT 1');
        $statement->execute(['sub_feature_001']);
        $subscription = $statement->fetch();

        self::assertIsArray($subscription);
        self::assertSame((int) $user['id'], (int) $subscription['user_id']);
        self::assertSame('active', (string) $subscription['status']);
    }

    public function testWebhookRejectsInvalidSignature(): void
    {
        $_ENV['STRIPE_WEBHOOK_SECRET'] = 'whsec_feature_test';
        $_SERVER['STRIPE_WEBHOOK_SECRET'] = 'whsec_feature_test';

        $payload = (string) json_encode([
            'id' => 'evt_invalid_signature',
            'object' => 'event',
            'type' => 'checkout.session.completed',
            'data' => ['object' => ['mode' => 'subscription']],
        ]);

        $response = $this->postRawJson('/webhooks/stripe', $payload, [
            'Stripe-Signature' => 't=' . time() . ',v1=invalid',
        ]);

        self::assertSame(400, $response->status);
        self::assertSame('Webhook signature verification failed.', (string) ($response->json()['error'] ?? ''));
    }
}
