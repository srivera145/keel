<?php
/**
 * The public site header: the brand, the theme switch, and the three destinations.
 * Shared by the landing page and the docs shell.
 *
 * Set $navCurrent = 'docs' before requiring this to mark the current destination.
 * $repoHref and $repoLabel come from the view; both have sensible defaults.
 */
$repoHref = $repoHref ?? 'https://github.com';
$repoLabel = $repoLabel ?? 'GitHub';
?>
<header class="container">
    <nav class="navbar" aria-label="Primary">
        <a class="navbar-brand" href="/">
            <img class="brand-logo when-light" src="/images/brand/keel.png" alt="Keel" loading="eager" decoding="async">
            <img class="brand-logo when-dark" src="/images/brand/keel-light.png" alt="Keel" loading="eager" decoding="async">
        </a>
        <div class="cluster cluster-tight push">
            <?php require __DIR__ . '/theme-toggle.php'; ?>
            <a class="nav-link" href="/docs"<?= ($navCurrent ?? '') === 'docs' ? ' aria-current="page"' : '' ?>>Docs</a>
            <a class="nav-link" href="<?= htmlspecialchars($repoHref) ?>" target="_blank" rel="noreferrer"><?= htmlspecialchars($repoLabel) ?></a>
            <a class="btn btn-primary btn-sm" href="/login">Sign in</a>
        </div>
    </nav>
</header>
