<meta charset="UTF-8">
<?php
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

// deck.js carries the theme switch and the back-to-top fallback, which every view uses. A
// view that needs the copy button, drawer or carousel sets $deckExtras = true before
// requiring this partial. Composer publishes these files into public_html/deck
// (see extra.deck in composer.json); Deck::head() emits the tags, including the viewport
// meta its mobile-first layout assumes.
\EchoDial\Deck\Deck::configure([
	'base' => '/deck',
	'root' => dirname(__DIR__, 2) . '/public_html',
	'extras' => $deckExtras ?? false,
]);
$publicAsset = static fn (string $path): string => $path . '?v=' . (int) @filemtime(dirname(__DIR__, 2) . '/public_html' . $path);
?>
<script>
	/* Apply the saved theme before first paint. deck.js restores it too, but not until
	   DOMContentLoaded, after a returning dark-mode visitor has already seen a light frame.
	   A signed-in user's server preference wins and is copied into Deck's storage key so
	   deck.js restores the same value; a guest's older keel-theme choice carries over once. */
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
<?= \EchoDial\Deck\Deck::head() ?>
<link rel="stylesheet" href="<?= htmlspecialchars($publicAsset('/css/keel.css'), ENT_QUOTES, 'UTF-8') ?>">
<script src="<?= htmlspecialchars($publicAsset('/js/keel.js'), ENT_QUOTES, 'UTF-8') ?>" defer></script>
<noscript><style>[data-deck-theme] { display: none; }</style></noscript>
