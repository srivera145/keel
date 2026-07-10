<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<?php
$serverTheme = \Keel\Core\Theme::serverPreference();
$isAuthenticated = \Keel\Core\Theme::isAuthenticated();
?>
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
<meta name="csrf-token" content="<?= htmlspecialchars(\Keel\Core\Csrf::token(), ENT_QUOTES, 'UTF-8') ?>">
<meta name="keel-authenticated" content="<?= $isAuthenticated ? '1' : '0' ?>">
<link rel="manifest" href="/manifest.webmanifest">
<meta name="theme-color" content="#07111f">
<meta name="apple-mobile-web-app-capable" content="yes">
<meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
<meta name="apple-mobile-web-app-title" content="<?= htmlspecialchars(\Keel\Core\Env::get('APP_NAME', 'Keel')) ?>">
<!-- Favicons are generated from the raster keel-icon.png source; swap in a vector source later for crisper scaling. -->
<link rel="icon" type="image/png" sizes="16x16" href="/favicon-16x16.png">
<link rel="icon" type="image/png" sizes="32x32" href="/favicon-32x32.png">
<link rel="icon" type="image/x-icon" href="/favicon.ico">
<link rel="apple-touch-icon" sizes="180x180" href="/apple-touch-icon.png">
<title><?= htmlspecialchars($title ?? \Keel\Core\Env::get('APP_NAME', 'Keel App')) ?></title>
<?= \Keel\Core\Vite::assets(['resources/js/app.js']) ?>
