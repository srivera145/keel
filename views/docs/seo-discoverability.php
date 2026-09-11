<div class="prose">
    <p>
    Keel exposes three discoverability endpoints at runtime: <code>/sitemap.xml</code>, <code>/robots.txt</code>, and <code>/llms.txt</code>.
    They are generated from route registration and the shared docs registry, so there is no separate static file to maintain.
    </p>

    <h2>Mark a public page for sitemap inclusion</h2>
    <p>
        <code>Router::get()</code> supports an optional third argument for route options. Use <code>['sitemap' =&gt; true]</code>
        when registering a real public content page.
    </p>
        <pre><code>public function get(string $uri, callable|array $action, array $options = []): void
{
    $this-&gt;addRoute('GET', $uri, $action, $options);
}</code></pre>
    <p>Example route registration from <code>routes/web.php</code>:</p>
        <pre><code>$router-&gt;get('/', [WelcomeController::class, 'index'], ['sitemap' =&gt; true]);
$router-&gt;get('/docs', [DocsController::class, 'index'], ['sitemap' =&gt; true]);
$router-&gt;get('/login', [AuthController::class, 'showLogin'], ['sitemap' =&gt; true]);</code></pre>
    <p>
        Once flagged, the page is returned by <code>Router::publicPages()</code>, and appears in <code>/sitemap.xml</code>
        automatically with no extra file edits.
    </p>

    <h2>Runtime endpoints</h2>
        <pre><code>$router-&gt;get('/sitemap.xml', [SitemapController::class, 'index']);
$router-&gt;get('/robots.txt', [RobotsController::class, 'index']);
$router-&gt;get('/llms.txt', [LlmsTxtController::class, 'index']);</code></pre>
    <ul>
        <li><code>SitemapController</code> builds XML from <code>APP_URL</code> + <code>Router::publicPages()</code> + expanded docs slugs.</li>
        <li><code>RobotsController</code> builds <code>Disallow</code> prefixes from registered protected routes/middleware.</li>
        <li><code>LlmsTxtController</code> outputs docs links from the same shared docs registry used by the docs sidebar.</li>
    </ul>

    <h2>Important behavior notes</h2>
    <ul>
        <li>Only explicitly flagged GET routes are sitemap-eligible.</li>
        <li>Parameterized docs routes are expanded to real slugs instead of emitting <code>/docs/{slug}</code>.</li>
        <li>Protected app routes (dashboard/settings/api/admin) are excluded from public discovery output.</li>
    </ul>
</div>
