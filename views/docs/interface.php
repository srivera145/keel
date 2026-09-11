<?php
// Pinned to the Deck release this page was written against. Deck::VERSION reports what is
// actually installed, so an upgrade that outpaces this page surfaces here instead of going
// stale invisibly.
$documentedDeckVersion = '0.1.3';
$installedDeckVersion = \EchoDial\Deck\Deck::VERSION;
?>
<div class="prose">
    <p>Keel's interface is built entirely in <a href="https://get-deck.dev">Deck</a> — every view, form, table and error page. There is no npm, no bundler and no build step: <code>composer install</code> publishes a stylesheet, and you write HTML against it.</p>

    <p>This page is about building a page <strong>in Keel</strong>. It is not a class reference. Deck's own docs cover every component in more detail than is worth duplicating here; this page covers the wiring, then points you there.</p>

    <p class="text-sm text-muted">Written against Deck <?= htmlspecialchars($documentedDeckVersion) ?>. Installed here: <?= htmlspecialchars($installedDeckVersion) ?>.</p>

    <?php if ($installedDeckVersion !== $documentedDeckVersion): ?>
    <div class="alert alert-warn">
        <?= \EchoDial\Deck\Deck::icon('alert-triangle') ?>
        <p class="alert-body">Deck <?= htmlspecialchars($installedDeckVersion) ?> is installed but this page documents <?= htmlspecialchars($documentedDeckVersion) ?>. Deck is pre-1.0; check the class names below against the published stylesheet.</p>
    </div>
    <?php endif; ?>

    <h2>Where Deck lives</h2>
    <p>Deck is a Composer package. It is not vendored, not copied by hand, and not committed.</p>
    <pre><code>composer require echodial/deck</code></pre>
    <p>Keel's <code>composer.json</code> opts in to publishing:</p>
    <pre><code>"extra": {
    "deck": {
        "publish-to": "public_html/deck",
        "auto-publish": true
    }
}</code></pre>
    <ul>
        <li>Every <code>composer install</code> and <code>composer update</code> copies Deck's files into <code>public_html/deck/</code>.</li>
        <li>That directory is in <code>.gitignore</code>. It is build output, restored by Composer on a fresh clone.</li>
        <li><strong>Never edit anything in <code>public_html/deck/</code>.</strong> The next install overwrites it.</li>
        <li>To republish without a full install: <code>composer deck-publish</code>.</li>
    </ul>
    <p>Keel publishes to <code>public_html/deck</code> rather than Deck's default <code>public/assets/deck</code>, because Keel's web root is <code>public_html/</code>.</p>

    <h2>What the head partial emits</h2>
    <p>Every view includes <code>views/partials/head.php</code>, which configures Deck and calls <code>Deck::head()</code>:</p>
    <pre><code>\EchoDial\Deck\Deck::configure([
    'base' =&gt; '/deck',
    'root' =&gt; dirname(__DIR__, 2) . '/public_html',
    'extras' =&gt; $deckExtras ?? false,
]);</code></pre>
    <p><code>base</code> is the public URL prefix; <code>root</code> is the filesystem path it maps to, which Deck uses to fingerprint the files. <code>Deck::head()</code> then emits:</p>
    <pre><code>&lt;meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover"&gt;
&lt;link rel="stylesheet" href="/deck/deck.min.css?v=1789159982"&gt;
&lt;script src="/deck/deck.js?v=1789159982" defer&gt;&lt;/script&gt;
&lt;script&gt;/* points Deck at /deck/deck-icons.svg */&lt;/script&gt;</code></pre>
    <p>That is the whole stylesheet — one file, every component. Keel's own CSS and JS load immediately after it, and the order matters:</p>
    <pre><code>&lt;?= \EchoDial\Deck\Deck::head() ?&gt;
&lt;link rel="stylesheet" href="/css/keel.css"&gt;
&lt;script src="/js/keel.js" defer&gt;&lt;/script&gt;</code></pre>
    <p>You do not call any of this yourself. Requiring the head partial is enough.</p>

    <h2>The PHP helpers</h2>
    <p>Deck ships four helpers Keel uses. All are static on <code>EchoDial\Deck\Deck</code>.</p>
    <ul>
        <li><code>Deck::head()</code> — the tags above. Called once, in the head partial.</li>
        <li><code>Deck::htmlAttributes(lang: 'en')</code> — writes <code>lang</code> and <code>dir</code> on <code>&lt;html&gt;</code>.</li>
        <li><code>Deck::theme(hue: null, mode: null, chroma: null)</code> — writes the theme attributes on <code>&lt;html&gt;</code>. See <a href="/docs/theming">Theming</a>.</li>
        <li><code>Deck::icon(name, class, label)</code> — renders an SVG sprite reference.</li>
    </ul>
    <p>Every Keel view opens the same way:</p>
    <pre><code>&lt;html &lt;?= Deck::htmlAttributes(lang: 'en') ?&gt; &lt;?= Deck::theme(mode: Theme::serverPreference()) ?&gt;&gt;</code></pre>
    <p>Icons come from a sprite of 75 symbols, each with a smaller <code>-sm</code> variant:</p>
    <pre><code>&lt;?= Deck::icon('trash') ?&gt;
&lt;?= Deck::icon('copy', 'icon icon-sm') ?&gt;
&lt;?= Deck::icon('shield', 'icon', 'Security') ?&gt;</code></pre>
    <p>Without a third argument the icon is <code>aria-hidden</code>, which is what you want beside a text label. Pass a label only when the icon is the only content. Names include <code>check</code>, <code>x</code>, <code>plus</code>, <code>trash</code>, <code>edit</code>, <code>copy</code>, <code>user</code>, <code>users</code>, <code>mail</code>, <code>lock</code>, <code>shield</code>, <code>clock</code>, <code>credit-card</code>, <code>alert-circle</code>, <code>alert-triangle</code>, <code>check-circle</code>, <code>settings</code> and <code>sparkle</code>.</p>

    <h2>Your CSS goes in the app layers</h2>
    <p>Deck declares its cascade layers in a fixed order and leaves four at the end for the application:</p>
    <pre><code>@layer
  deck.reset, deck.tokens, deck.type, deck.layout, deck.components,
  deck.mobile, deck.motion, deck.effects, deck.utilities, deck.rtl, deck.print,

  app.base,       /* your resets and element styles */
  app.components, /* your components and deck overrides */
  app.pages,      /* per-page tweaks */
  app.overrides;  /* the escape hatch */</code></pre>
    <p>A rule in any <code>app.*</code> layer beats any Deck rule regardless of specificity, because later layers win. That is the whole mechanism: <strong>you never need <code>!important</code>, and never need to out-specify Deck.</strong></p>
    <p>A real example. Deck's <code>.table-wrap</code> brings its own border and radius, so inside a <code>.card</code> it draws a second frame one pixel in from the first. Keel flattens it in <code>public_html/css/keel.css</code>:</p>
    <pre><code>@layer app.components {
  .card &gt; .table-wrap {
    border: 0;
    border-radius: 0;
  }
}</code></pre>
    <p>A single class selector overriding a Deck component, with no <code>!important</code>. It works because <code>app.components</code> is declared after <code>deck.components</code>.</p>
    <p>This only holds if <code>keel.css</code> loads <em>after</em> <code>deck.css</code>, which the head partial guarantees. Loaded first, the <code>@layer</code> blocks in <code>keel.css</code> would declare the order themselves, and Deck would win everything.</p>
    <p>Which layer to use:</p>
    <ul>
        <li><code>app.base</code> — element defaults and tokens. Keel sets <code>--hue-brand</code> and a <code>[hidden]</code> rule here.</li>
        <li><code>app.components</code> — reusable pieces, and corrections to Deck components.</li>
        <li><code>app.pages</code> — one page or one screen. Keel's <code>.settings-page</code> and <code>.stage</code> live here.</li>
        <li><code>app.overrides</code> — last resort. Empty in Keel.</li>
    </ul>

    <h2>A complete view</h2>
    <p>A projects list: a header, a table, an empty state, and a form beside it. Controller, route, view.</p>

    <h3>1. The controller</h3>
    <pre><code>&lt;?php

namespace Keel\App\Controllers;

use Keel\Core\Controller;
use Keel\Core\Request;

class ProjectController extends Controller
{
    public function index(Request $request): void
    {
        $this-&gt;view('projects.index', [
            'title' =&gt; 'Projects',
            'projects' =&gt; [
                ['name' =&gt; 'Onboarding revamp', 'status' =&gt; 'active'],
                ['name' =&gt; 'Billing migration', 'status' =&gt; 'paused'],
            ],
        ]);
    }
}</code></pre>

    <h3>2. The route</h3>
    <pre><code>use Keel\App\Controllers\ProjectController;

$router-&gt;get('/projects', [ProjectController::class, 'index']);</code></pre>

    <h3>3. The view</h3>
    <p><code>views/projects/index.php</code>:</p>
    <pre><code>&lt;?php

use EchoDial\Deck\Deck;
use Keel\Core\Csrf;
use Keel\Core\Theme;

$projects = $projects ?? [];
?&gt;
&lt;!DOCTYPE html&gt;
&lt;html &lt;?= Deck::htmlAttributes(lang: 'en') ?&gt; &lt;?= Deck::theme(mode: Theme::serverPreference()) ?&gt;&gt;
&lt;head&gt;
&lt;?php require __DIR__ . '/../partials/head.php'; ?&gt;
&lt;/head&gt;
&lt;body&gt;
    &lt;span id="top" tabindex="-1"&gt;&lt;/span&gt;

    &lt;main class="container settings-page stack stack-6"&gt;
        &lt;header class="bar"&gt;
            &lt;div class="stack stack-2"&gt;
                &lt;nav aria-label="Breadcrumb"&gt;
                    &lt;ol class="breadcrumb"&gt;
                        &lt;li&gt;&lt;a href="/dashboard"&gt;Dashboard&lt;/a&gt;&lt;/li&gt;
                        &lt;li&gt;&lt;span aria-current="page"&gt;Projects&lt;/span&gt;&lt;/li&gt;
                    &lt;/ol&gt;
                &lt;/nav&gt;
                &lt;h1 class="h2"&gt;Projects&lt;/h1&gt;
            &lt;/div&gt;
            &lt;?php $themeToggleClass = 'push'; require __DIR__ . '/../partials/theme-toggle.php'; ?&gt;
        &lt;/header&gt;

        &lt;div class="split"&gt;
            &lt;section class="card" aria-labelledby="list-title"&gt;
                &lt;div class="card-header"&gt;
                    &lt;h2 id="list-title" class="card-title"&gt;All projects&lt;/h2&gt;
                &lt;/div&gt;
                &lt;div class="table-wrap"&gt;
                    &lt;table class="table table-stack"&gt;
                        &lt;thead&gt;
                            &lt;tr&gt;
                                &lt;th scope="col"&gt;Name&lt;/th&gt;
                                &lt;th scope="col"&gt;Status&lt;/th&gt;
                            &lt;/tr&gt;
                        &lt;/thead&gt;
                        &lt;tbody&gt;
                            &lt;?php if ($projects === []): ?&gt;
                            &lt;tr&gt;
                                &lt;td colspan="2" class="table-empty"&gt;
                                    &lt;div class="empty"&gt;
                                        &lt;span class="empty-art"&gt;&lt;?= Deck::icon('folder') ?&gt;&lt;/span&gt;
                                        &lt;p class="empty-title"&gt;No projects yet&lt;/p&gt;
                                        &lt;p&gt;Create one with the form beside this table.&lt;/p&gt;
                                    &lt;/div&gt;
                                &lt;/td&gt;
                            &lt;/tr&gt;
                            &lt;?php endif; ?&gt;

                            &lt;?php foreach ($projects as $project): ?&gt;
                            &lt;tr&gt;
                                &lt;td data-label="Name" class="fw-medium"&gt;&lt;?= htmlspecialchars((string) $project['name']) ?&gt;&lt;/td&gt;
                                &lt;td data-label="Status"&gt;
                                    &lt;span class="badge &lt;?= $project['status'] === 'active' ? 'badge-good' : 'badge-warn' ?&gt;"&gt;
                                        &lt;?= htmlspecialchars((string) $project['status']) ?&gt;
                                    &lt;/span&gt;
                                &lt;/td&gt;
                            &lt;/tr&gt;
                            &lt;?php endforeach; ?&gt;
                        &lt;/tbody&gt;
                    &lt;/table&gt;
                &lt;/div&gt;
            &lt;/section&gt;

            &lt;section class="card" aria-labelledby="new-title"&gt;
                &lt;div class="card-header"&gt;
                    &lt;h2 id="new-title" class="card-title"&gt;New project&lt;/h2&gt;
                &lt;/div&gt;
                &lt;form method="POST" action="/projects" class="card-body"&gt;
                    &lt;?= Csrf::field() ?&gt;
                    &lt;div class="field"&gt;
                        &lt;label class="label" for="name"&gt;Name&lt;/label&gt;
                        &lt;input class="input" type="text" id="name" name="name" required&gt;
                    &lt;/div&gt;
                    &lt;div class="field"&gt;
                        &lt;label class="label" for="status"&gt;Status&lt;/label&gt;
                        &lt;select class="select" id="status" name="status"&gt;
                            &lt;option value="active"&gt;Active&lt;/option&gt;
                            &lt;option value="paused"&gt;Paused&lt;/option&gt;
                        &lt;/select&gt;
                    &lt;/div&gt;
                    &lt;button type="submit" class="btn btn-primary btn-block"&gt;Create project&lt;/button&gt;
                &lt;/form&gt;
            &lt;/section&gt;
        &lt;/div&gt;
    &lt;/main&gt;

    &lt;?php require __DIR__ . '/../partials/back-to-top.php'; ?&gt;
&lt;/body&gt;
&lt;/html&gt;</code></pre>

    <p>What each piece is doing:</p>
    <ul>
        <li><code>container</code> centres and bounds the page; <code>settings-page</code> is Keel's own width and padding, from <code>app.pages</code>.</li>
        <li><code>stack stack-6</code> spaces children with <code>gap</code>, not margins. The number is the gap step.</li>
        <li><code>bar</code> is a flex row with space between; <code>push</code> pushes an item to the far end.</li>
        <li><code>breadcrumb</code> on an <code>&lt;ol&gt;</code> draws the trail and its separators. Mark the current page with <code>aria-current="page"</code> rather than a link.</li>
        <li><code>&lt;h1 class="h2"&gt;</code> is deliberate, not a typo. The heading levels are the document outline; <code>h1</code>–<code>h6</code> are also type-scale classes you can put on any element. Deck's <code>h1</code> is display-sized, which suits a landing page and overpowers an app screen, so app pages keep the <code>&lt;h1&gt;</code> element and take the <code>h2</code> size.</li>
        <li><code>split</code> is a main column plus a rail. <code>--rail</code> sets the rail width.</li>
        <li><code>card</code> › <code>card-header</code> › <code>card-title</code>, then <code>card-body</code>. A form can be the card body directly — <code>card-body</code> supplies the gap between fields.</li>
        <li><code>field</code> › <code>label</code> + <code>input</code>/<code>select</code> is the form triple. Always pair <code>for</code> and <code>id</code>.</li>
        <li><code>table-wrap</code> scrolls a wide table; <code>table-stack</code> restacks each row into labelled lines below 40rem. <strong>Every <code>&lt;td&gt;</code> needs a <code>data-label</code></strong> — that attribute is where the stacked label comes from, and a cell without one stacks with a blank label. Leave <code>table-stack</code> off long tables; it makes them very tall.</li>
        <li><code>empty</code> › <code>empty-art</code> + <code>empty-title</code> is the empty state. <code>table-empty</code> is Keel's, so the cell padding does not double up.</li>
        <li><code>badge-good</code> and <code>badge-warn</code> are semantic tones, not colours. So are <code>alert-good</code>, <code>alert-warn</code> and <code>alert-bad</code>.</li>
        <li><code>btn btn-primary btn-block</code> — one primary button per screen. <code>btn-block</code> makes it full width.</li>
    </ul>
    <p>No widths, no colours, no spacing values in the markup. If you find yourself stacking utilities to build something, there is usually a component for it.</p>
    <p>This view uses <code>split</code> because it is a main column plus a rail. The other layout primitive you will reach for constantly is <code>grid</code>, which wraps equal-width cards without a column count:</p>
    <pre><code>&lt;div class="grid" style="--min: 20rem"&gt;
    &lt;article class="card"&gt;...&lt;/article&gt;
    &lt;article class="card"&gt;...&lt;/article&gt;
&lt;/div&gt;</code></pre>
    <p><code>--min</code> is the narrowest a column may get before the grid drops to fewer columns; set it per grid. The named variants <code>grid-tight</code> and <code>grid-wide</code> are presets for the same knob, and <code>grid-wide</code>'s 24rem floor fits only two columns in a 76rem container — so when three cards across is what you want, set <code>--min</code> yourself. Keel's landing page does exactly that in three places.</p>
    <p>The spacing steps behind <code>stack-2</code>, <code>stack-6</code> and the <code>--space-*</code> tokens are listed in <a href="https://get-deck.dev/docs/reference/spacing.php">Deck's spacing reference</a>.</p>

    <h2>JavaScript</h2>
    <p><code>deck.js</code> loads on every page and drives:</p>
    <ul>
        <li><code>[data-deck-theme]</code> — the theme switch.</li>
        <li><code>[data-deck-btt]</code> — back to top.</li>
        <li>The combobox, date picker, sortable data grid, and icon sprite injection.</li>
    </ul>
    <p><code>deck-extras.js</code> is a second, larger bundle and is <strong>off by default</strong>. Set <code>$deckExtras = true</code> before requiring the head partial to load it:</p>
    <pre><code>$deckExtras = true; // this page uses the copy button</code></pre>
    <p>It drives <code>data-deck-copy</code>, <code>data-deck-drawer</code>, <code>data-deck-carousel</code>, <code>data-deck-mega</code> and <code>data-deck-qr</code>. Only two Keel views ask for it: the landing page, and the API tokens page when a new token is on screen.</p>
    <p>Two things Deck styles but does not wire, which Keel supplies in <code>public_html/js/keel.js</code>:</p>
    <ul>
        <li><strong>Modals.</strong> <code>.modal</code> is styled; nothing calls <code>showModal()</code>. Keel opens a <code>&lt;dialog class="modal"&gt;</code> from a form carrying <code>data-confirm</code>, and posts it from a button carrying <code>data-confirm-submit</code>. With scripts off, the form posts straight through.</li>
        <li><strong>Tabs.</strong> <code>.tabs</code> is styled; the roving tabindex and arrow keys are Keel's, on the login page.</li>
    </ul>
    <p>Keel's JS also persists a theme change to the server. Everything else on the page works with JavaScript off.</p>

    <h2>Where to look things up</h2>
    <ul>
        <li><a href="https://get-deck.dev/docs">Deck docs</a> — the full reference.</li>
        <li><a href="https://get-deck.dev/docs/reference/classes.php">Class reference</a> — every public class, by layer.</li>
        <li><a href="https://get-deck.dev/docs/reference/php.php">PHP helper</a> — <code>Deck::head()</code>, <code>icon()</code>, <code>theme()</code>, <code>htmlAttributes()</code>.</li>
        <li><a href="https://get-deck.dev/docs/guides/layers.php">Cascade layers</a> — why <code>app.*</code> beats <code>deck.*</code>.</li>
        <li><a href="https://get-deck.dev/docs/guides/layout.php">Layout</a> — <code>container</code>, <code>stack</code>, <code>grid</code>, <code>split</code>, <code>bar</code>, <code>cluster</code>.</li>
        <li><a href="https://get-deck.dev/docs/components/card.php">Components</a> — one page each.</li>
        <li><a href="https://get-deck.dev/docs/reference/javascript.php">JavaScript API</a> — <code>Deck.toast()</code>, <code>Deck.copy()</code>, <code>Deck.theme()</code>.</li>
    </ul>
    <p>The fastest local reference is the stylesheet itself. Every component in <code>public_html/deck/deck.css</code> carries a comment above it explaining what it is for and how it is meant to be used.</p>
</div>
