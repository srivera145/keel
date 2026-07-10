<div class="space-y-6 text-sm leading-7 text-[var(--color-text-muted)]">
    <section>
        <h2 class="mb-2 text-lg font-semibold text-[var(--color-text-strong)]">Design tokens</h2>
        <p>Core tokens live in <code>resources/css/app.css</code> under <code>:root</code> and <code>[data-theme="light"]</code>.</p>
        <pre class="overflow-x-auto rounded-xl bg-[var(--color-surface-muted)] p-4 text-xs text-[var(--color-text-strong)]"><code>--color-brand
--color-brand-hover
--color-brand-contrast
--color-surface
--color-surface-hover
--color-surface-muted
--color-text
--color-text-muted
--color-text-strong
--color-card-bg
--color-card-border
--color-input-border</code></pre>
    </section>

    <section>
        <h2 class="mb-2 text-lg font-semibold text-[var(--color-text-strong)]">Component layer</h2>
        <p><code>resources/css/components.css</code> maps components to tokenized styles:</p>
        <ul class="list-disc space-y-1 pl-5">
            <li><code>.btn</code>, <code>.btn-primary</code>, <code>.btn-secondary</code>, <code>.btn-danger</code></li>
            <li><code>.card</code>, <code>.badge</code>, <code>.alert</code>, <code>.form-input</code></li>
            <li><code>.table</code>, <code>.pagination</code>, <code>.modal</code>, <code>.empty-state</code></li>
        </ul>
    </section>

    <section>
        <h2 class="mb-2 text-lg font-semibold text-[var(--color-text-strong)]">Theme toggle and persistence</h2>
        <ul class="list-disc space-y-1 pl-5">
            <li>Use <code>data-theme-toggle</code> buttons with <code>data-theme-toggle-icon</code> spans.</li>
            <li><code>resources/js/components/theme-toggle.js</code> updates <code>data-theme</code> on <code>&lt;html&gt;</code>.</li>
            <li>Unauthenticated users persist preference in <code>localStorage</code> key <code>keel-theme</code>.</li>
            <li>Authenticated users are persisted server-side via <code>POST /settings/theme</code>.</li>
            <li><code>Theme::htmlAttribute()</code> emits the server preference into the initial HTML tag.</li>
        </ul>
    </section>
</div>
