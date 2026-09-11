<div class="prose">
    <h2>One number</h2>
    <p>Deck derives every brand colour from <code>--hue-brand</code>, a number from 0 to 360. Keel sets its own in <code>public_html/css/keel.css</code>:</p>
    <pre><code>@layer app.base {
  :root { --hue-brand: 38; }
}</code></pre>
    <p>38 is Keel's orange, <code>#ff6b3d</code>, expressed as a hue. Change it and the buttons, links, focus rings, badges and charts all follow, with no rebuild and no second stylesheet.</p>

    <h2>Per-tenant branding</h2>
    <p>An organization carries <code>brand_hue</code>. The view hands it to Deck on the <code>&lt;html&gt;</code> tag, where an inline style beats every layer:</p>
    <pre><code>&lt;html &lt;?= Deck::htmlAttributes(lang: 'en') ?&gt; &lt;?= Deck::theme(hue: $brandHue, mode: Theme::serverPreference()) ?&gt;&gt;</code></pre>
    <p>Two tenants on the same deploy get different palettes from that one attribute. It has to be on <code>&lt;html&gt;</code>: Deck computes its brand ramp once on <code>:root</code>, so setting <code>--hue-brand</code> on a section or a card changes nothing below it.</p>

    <h2>Keel's own CSS</h2>
    <p>Deck ships in cascade layers and leaves four of them empty for the application: <code>app.base</code>, <code>app.components</code>, <code>app.pages</code> and <code>app.overrides</code>. Everything Keel adds lives in <code>public_html/css/keel.css</code> inside those layers, so it beats Deck with no <code>!important</code> and no specificity games.</p>
    <ul>
        <li><code>app.base</code>: the brand hue, and a <code>[hidden]</code> rule Deck's reset leaves out.</li>
        <li><code>app.components</code>: <code>.when-light</code> and <code>.when-dark</code>, <code>.table-empty</code>, <code>.brand-logo</code>, <code>.hue-swatch</code>, <code>.otp-input</code>.</li>
        <li><code>app.pages</code>: <code>.settings-page</code>, <code>.stage</code>, and the docs rail.</li>
    </ul>
    <p>Never edit <code>public_html/deck/</code>. Composer republishes it on every install. <a href="/docs/interface">Building the Interface</a> covers the layer order and why a rule in <code>app.*</code> beats Deck without <code>!important</code>.</p>

    <h2>Light and dark</h2>
    <ul>
        <li>Deck's colours are <code>light-dark()</code> pairs driven by <code>color-scheme</code>. <code>data-theme</code> on <code>&lt;html&gt;</code> forces one or the other.</li>
        <li><code>views/partials/theme-toggle.php</code> renders Deck's <code>[data-deck-theme]</code> button. The moon and sun swap through <code>.when-light</code> and <code>.when-dark</code>, because Deck exposes no state of its own on the button.</li>
        <li><code>public_html/js/keel.js</code> watches the attribute and posts to <code>/settings/theme</code>, so a signed-in user's choice follows them to another device.</li>
        <li>A short script in <code>views/partials/head.php</code> applies the saved theme before the first paint, which <code>deck.js</code> alone does not do.</li>
    </ul>

    <h2>Reference</h2>
    <ul>
        <li><a href="/docs/interface">Building the Interface</a> — the helpers, the layers, and a complete view.</li>
        <li><a href="https://get-deck.dev/docs/guides/theming.php">Deck: theming</a> — the token ramp in full.</li>
        <li><a href="https://get-deck.dev/docs/explain/why-one-hue.php">Deck: why one hue</a> — the reasoning behind <code>--hue-brand</code>.</li>
        <li><a href="https://get-deck.dev/docs/guides/dark-mode.php">Deck: dark mode</a> — <code>light-dark()</code> and <code>color-scheme</code>.</li>
    </ul>
</div>
