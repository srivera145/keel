<div class="space-y-6 text-sm leading-7 text-[var(--color-text-muted)]">
    <p>Keel uses Stripe-hosted Checkout and Billing Portal flows with webhook sync.</p>

    <section>
        <h2 class="mb-2 text-lg font-semibold text-[var(--color-text-strong)]">Required environment keys</h2>
        <pre class="overflow-x-auto rounded-xl bg-[var(--color-surface-muted)] p-4 text-xs text-[var(--color-text-strong)]"><code>STRIPE_SECRET_KEY=
STRIPE_PUBLISHABLE_KEY=
STRIPE_WEBHOOK_SECRET=
STRIPE_PRICE_PRO_MONTHLY=</code></pre>
    </section>

    <section>
        <h2 class="mb-2 text-lg font-semibold text-[var(--color-text-strong)]">Routes</h2>
        <pre class="overflow-x-auto rounded-xl bg-[var(--color-surface-muted)] p-4 text-xs text-[var(--color-text-strong)]"><code>$router-&gt;get('/billing/upgrade', [BillingController::class, 'showPlans']);
$router-&gt;post('/billing/checkout', [BillingController::class, 'checkout']);
$router-&gt;post('/billing/portal', [BillingController::class, 'portal']);
$router-&gt;post('/webhooks/stripe', [StripeWebhookController::class, 'handle']);</code></pre>
    </section>

    <section>
        <h2 class="mb-2 text-lg font-semibold text-[var(--color-text-strong)]">Operational notes</h2>
        <ul class="list-disc space-y-1 pl-5">
            <li>Checkout sessions are created in <code>BillingService::createCheckoutSession()</code>.</li>
            <li>Portal sessions are created in <code>BillingService::createPortalSession()</code>.</li>
            <li>Webhook signatures are verified against <code>STRIPE_WEBHOOK_SECRET</code> before sync logic runs.</li>
            <li>Billing state is mirrored into the local <code>subscriptions</code> table.</li>
            <li>Raw card data never passes through Keel application code.</li>
        </ul>
    </section>
</div>
