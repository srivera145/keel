<div class="prose">
    <p>Keel keeps framework internals separate from app code while keeping views plain PHP.</p>

    <pre><code>public_html/
  index.php
  css/keel.css       yours
  js/keel.js         yours
  deck/              Deck's, published by Composer, git-ignored
    deck.css
    deck.min.css
    deck.js
    deck-extras.js
    deck-icons.svg
  uploads/
src/
  Core/
  App/
    Controllers/
    Middleware/
    Models/
    Services/
routes/
  web.php
views/
  partials/
  auth/
  dashboard/
resources/
  images/
database/
  migrations/
  migrate.php
  queue-work.php
tests/
storage/
  logs/
  app/</code></pre>

    <h2>What belongs where</h2>
    <ul>
        <li><code>src/Core/</code>: framework internals like <code>Router</code>, <code>Request</code>, <code>Response</code>, <code>Database</code>, <code>View</code>, <code>ErrorHandler</code>.</li>
        <li><code>src/App/Controllers/</code>: HTTP endpoints and request orchestration.</li>
        <li><code>src/App/Services/</code>: business logic, external APIs, and workflows.</li>
        <li><code>src/App/Middleware/</code>: route guards and request checks.</li>
        <li><code>routes/web.php</code>: route registration.</li>
        <li><code>views/</code>: plain PHP templates (no template engine dependency).</li>
        <li><code>public_html/deck/</code>: Deck's published files. Not yours — see below.</li>
        <li><code>public_html/css/keel.css</code> and <code>public_html/js/keel.js</code>: Keel's own styles and glue, served exactly as written. There is no build step.</li>
        <li><code>resources/images/</code>: brand source images.</li>
        <li><code>database/migrations/</code>: SQL migrations applied by <code>php database/migrate.php</code>.</li>
        <li><code>database/queue-work.php</code>: queue worker CLI entrypoint.</li>
    </ul>

    <h2>Deck's files versus yours</h2>
    <p>Two directories under <code>public_html/</code> hold CSS. The difference matters, because one of them is overwritten without warning.</p>
    <table class="table table-stack">
        <thead>
            <tr>
                <th scope="col">&nbsp;</th>
                <th scope="col"><code>public_html/deck/</code></th>
                <th scope="col"><code>public_html/css/keel.css</code></th>
            </tr>
        </thead>
        <tbody>
            <tr>
                <td data-label="Whose">Whose</td>
                <td data-label="public_html/deck/">Deck's</td>
                <td data-label="public_html/css/keel.css">Yours</td>
            </tr>
            <tr>
                <td data-label="Whose">How it gets there</td>
                <td data-label="public_html/deck/"><code>composer install</code></td>
                <td data-label="public_html/css/keel.css">You write it</td>
            </tr>
            <tr>
                <td data-label="Whose">In git</td>
                <td data-label="public_html/deck/">No, git-ignored</td>
                <td data-label="public_html/css/keel.css">Yes</td>
            </tr>
            <tr>
                <td data-label="Whose">Safe to edit</td>
                <td data-label="public_html/deck/">Never</td>
                <td data-label="public_html/css/keel.css">Always</td>
            </tr>
        </tbody>
    </table>
    <p><strong>Never edit anything in <code>public_html/deck/</code>.</strong> It is build output: Composer republishes the whole directory on every <code>install</code> and <code>update</code>, so an edit there survives until the next install and then vanishes — including on a teammate's machine, and in CI, where it never existed at all. The directory is git-ignored for that reason, and a fresh clone has no <code>public_html/deck/</code> until Composer runs.</p>
    <p>Everything you want to change goes in <code>public_html/css/keel.css</code> instead. Deck reserves four cascade layers for it — <code>app.base</code>, <code>app.components</code>, <code>app.pages</code> and <code>app.overrides</code> — which are declared after all of Deck's, so a rule there beats a Deck rule without <code>!important</code> and without extra specificity. That is the supported way to override Deck, and it survives upgrades. See <a href="/docs/interface">Building the Interface</a>.</p>
</div>
