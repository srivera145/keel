<div class="prose">
    <p>Keel uses Stripe-hosted Checkout and Billing Portal flows with webhook sync.</p>

    <h2>Required environment keys</h2>
        <pre><code>STRIPE_SECRET_KEY=
STRIPE_PUBLISHABLE_KEY=
STRIPE_WEBHOOK_SECRET=
STRIPE_PRICE_PRO_MONTHLY=</code></pre>

    <h2>Routes</h2>
        <pre><code>$router-&gt;get('/billing/upgrade', [BillingController::class, 'showPlans']);
$router-&gt;post('/billing/checkout', [BillingController::class, 'checkout']);
$router-&gt;post('/billing/portal', [BillingController::class, 'portal']);
$router-&gt;post('/webhooks/stripe', [StripeWebhookController::class, 'handle']);</code></pre>

    <h2>Operational notes</h2>
    <ul>
        <li>Checkout sessions are created in <code>BillingService::createCheckoutSession()</code>.</li>
        <li>Portal sessions are created in <code>BillingService::createPortalSession()</code>.</li>
        <li>Webhook signatures are verified against <code>STRIPE_WEBHOOK_SECRET</code> before sync logic runs.</li>
        <li>Billing state is mirrored into the local <code>subscriptions</code> table.</li>
        <li>Raw card data never passes through Keel application code.</li>
    </ul>
</div>
