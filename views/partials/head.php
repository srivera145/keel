<meta charset="UTF-8">
<?php
// Views move to Deck one at a time by setting $ui = 'deck' before requiring this partial.
// A page gets Deck or the Vite/Tailwind bundle, never both, so the two stylesheets never meet.
$ui = $ui ?? 'tailwind';

$serverTheme = \Keel\Core\Theme::serverPreference();
$isAuthenticated = \Keel\Core\Theme::isAuthenticated();

$appUrl = rtrim((string) \Keel\Core\Env::get('APP_URL', ''), '/');
$requestUri = (string) ($_SERVER['REQUEST_URI'] ?? '/');
$requestPath = (string) (parse_url($requestUri, PHP_URL_PATH) ?? '/');
$requestPath = $requestPath !== '' ? $requestPath : '/';
$canonicalUrl = $appUrl !== ''
	? $appUrl . ($requestPath === '/' ? '/' : $requestPath)
	: ($requestPath === '' ? '/' : $requestPath);

$defaultTitle = 'Keel - Open-Source PHP Starter Kit';
$resolvedTitle = trim((string) ($title ?? $defaultTitle));
if ($resolvedTitle === '' || strtolower($resolvedTitle) === 'keel') {
	$resolvedTitle = $defaultTitle;
}

$defaultDescription = 'Keel is an open-source PHP 8.2 starter kit for SaaS apps with passwordless auth, Stripe billing, multi-tenancy, and docs.';
$resolvedDescription = trim((string) ($metaDescription ?? $defaultDescription));
$siteName = trim((string) \Keel\Core\Env::get('APP_NAME', 'Keel'));
$siteName = $siteName !== '' ? $siteName : 'Keel';
$socialImage = $appUrl !== '' ? $appUrl . '/images/brand/keel.png' : '/images/brand/keel.png';
?>
<?php if ($ui === 'deck'): ?>
<script>
	/* Apply the saved theme before first paint. deck.js restores it too, but not until
	   DOMContentLoaded, after a returning dark-mode visitor has already seen a light frame.
	   A signed-in user's server preference wins and is copied into Deck's storage key so
	   deck.js restores the same value; a guest's old keel-theme choice carries over once. */
	(function () {
		var serverTheme = <?= json_encode($serverTheme, JSON_UNESCAPED_SLASHES) ?>;

		try {
			if (serverTheme) {
				localStorage.setItem('deck-theme', serverTheme);
			} else if (!localStorage.getItem('deck-theme') && localStorage.getItem('keel-theme')) {
				localStorage.setItem('deck-theme', localStorage.getItem('keel-theme'));
			}

			var theme = serverTheme || localStorage.getItem('deck-theme');
			if (theme === 'light' || theme === 'dark') {
				document.documentElement.setAttribute('data-theme', theme);
			}
		} catch (error) {
			// No storage access: the server attribute or the OS preference decides.
		}
	})();
</script>
<?php else: ?>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<script>
	(function () {
		var serverTheme = <?= json_encode($serverTheme, JSON_UNESCAPED_SLASHES) ?>;
		var isAuthenticated = <?= $isAuthenticated ? 'true' : 'false' ?>;
		var theme = serverTheme;

		if (!theme && !isAuthenticated) {
			try {
				var storedTheme = localStorage.getItem('keel-theme');
				if (storedTheme === 'light' || storedTheme === 'dark') {
					theme = storedTheme;
				}
			} catch (error) {
				// Ignore storage access errors and continue to system/default resolution.
			}
		}

		if (!theme && window.matchMedia) {
			try {
				theme = window.matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light';
			} catch (error) {
				theme = null;
			}
		}

		if (!theme) {
			theme = 'dark';
		}

		document.documentElement.setAttribute('data-theme', theme);
	})();
</script>
<?php endif; ?>
<meta name="csrf-token" content="<?= htmlspecialchars(\Keel\Core\Csrf::token(), ENT_QUOTES, 'UTF-8') ?>">
<meta name="keel-authenticated" content="<?= $isAuthenticated ? '1' : '0' ?>">
<link rel="manifest" href="/manifest.webmanifest">
<meta name="theme-color" content="#07111f">
<meta name="apple-mobile-web-app-capable" content="yes">
<meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
<meta name="apple-mobile-web-app-title" content="<?= htmlspecialchars(\Keel\Core\Env::get('APP_NAME', 'Keel')) ?>">
<meta name="description" content="<?= htmlspecialchars($resolvedDescription, ENT_QUOTES, 'UTF-8') ?>">
<link rel="canonical" href="<?= htmlspecialchars($canonicalUrl, ENT_QUOTES, 'UTF-8') ?>">

<meta property="og:title" content="<?= htmlspecialchars($resolvedTitle, ENT_QUOTES, 'UTF-8') ?>">
<meta property="og:description" content="<?= htmlspecialchars($resolvedDescription, ENT_QUOTES, 'UTF-8') ?>">
<meta property="og:type" content="website">
<meta property="og:url" content="<?= htmlspecialchars($canonicalUrl, ENT_QUOTES, 'UTF-8') ?>">
<meta property="og:image" content="<?= htmlspecialchars($socialImage, ENT_QUOTES, 'UTF-8') ?>">
<meta property="og:site_name" content="<?= htmlspecialchars($siteName, ENT_QUOTES, 'UTF-8') ?>">

<meta name="twitter:card" content="summary_large_image">
<meta name="twitter:title" content="<?= htmlspecialchars($resolvedTitle, ENT_QUOTES, 'UTF-8') ?>">
<meta name="twitter:description" content="<?= htmlspecialchars($resolvedDescription, ENT_QUOTES, 'UTF-8') ?>">
<meta name="twitter:image" content="<?= htmlspecialchars($socialImage, ENT_QUOTES, 'UTF-8') ?>">
<!-- Favicons are generated from the raster keel-icon.png source; swap in a vector source later for crisper scaling. -->
<link rel="icon" type="image/png" sizes="16x16" href="/favicon-16x16.png">
<link rel="icon" type="image/png" sizes="32x32" href="/favicon-32x32.png">
<link rel="icon" type="image/x-icon" href="/favicon.ico">
<link rel="apple-touch-icon" sizes="180x180" href="/apple-touch-icon.png">
<title><?= htmlspecialchars($resolvedTitle, ENT_QUOTES, 'UTF-8') ?></title>
<?php if ($ui === 'deck'): ?>
<?php
// deck.js alone: the theme switch and back-to-top fallback are all a converted view uses so far.
// Composer publishes these files into public_html/deck (see extra.deck in composer.json).
\EchoDial\Deck\Deck::configure([
	'base' => '/deck',
	'root' => dirname(__DIR__, 2) . '/public_html',
	'extras' => false,
]);
$publicAsset = static fn (string $path): string => $path . '?v=' . (int) @filemtime(dirname(__DIR__, 2) . '/public_html' . $path);
?>
<?= \EchoDial\Deck\Deck::head() ?>
<link rel="stylesheet" href="<?= htmlspecialchars($publicAsset('/css/keel.css'), ENT_QUOTES, 'UTF-8') ?>">
<script src="<?= htmlspecialchars($publicAsset('/js/keel.js'), ENT_QUOTES, 'UTF-8') ?>" defer></script>
<noscript><style>[data-deck-theme] { display: none; }</style></noscript>
<?php else: ?>
<?= \Keel\Core\Vite::assets(['resources/js/app.js']) ?>
<?php endif; ?>
