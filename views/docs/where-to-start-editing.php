<div class="prose">
    <h2>Add a new page</h2>
    <p>The minimal workflow is controller + route + view.</p>

    <h3>1. Create a controller method</h3>
        <pre><code>&lt;?php

namespace Keel\App\Controllers;

use Keel\Core\Controller;
use Keel\Core\Request;

class ExampleController extends Controller
{
    public function index(Request $request): void
    {
        $this-&gt;view('example.index', ['title' =&gt; 'Example']);
    }
}</code></pre>

    <h3>2. Register the route in routes/web.php</h3>
        <pre><code>use Keel\App\Controllers\ExampleController;

$router-&gt;get('/example', [ExampleController::class, 'index']);</code></pre>

    <h3>3. Create the view</h3>
        <pre><code>&lt;!-- views/example/index.php --&gt;
&lt;!DOCTYPE html&gt;
&lt;html lang="en"&gt;
&lt;head&gt;
&lt;?php require __DIR__ . '/../partials/head.php'; ?&gt;
&lt;/head&gt;
&lt;body&gt;
    &lt;main class="container section"&gt;
        &lt;h1&gt;Example&lt;/h1&gt;
    &lt;/main&gt;
&lt;/body&gt;
&lt;/html&gt;</code></pre>

    <h2>Renaming Keel for a new project</h2>
    <ol>
        <li>Update <code>composer.json</code> package <code>name</code>.</li>
        <li>Update <code>package.json</code> package <code>name</code>.</li>
        <li>Update <code>.env</code>: <code>APP_NAME</code>, <code>APP_URL</code>, and <code>DB_DATABASE</code>.</li>
        <li>Swap brand assets in <code>public_html/images/brand/</code> and matching resource images.</li>
        <li>Review landing-page copy in <code>views/welcome.php</code>.</li>
    </ol>
</div>
