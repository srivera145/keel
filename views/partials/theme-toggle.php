<?php
/**
 * The theme switch for Deck views.
 *
 * Deck's [data-deck-theme] flips the theme and saves it; public_html/js/keel.js
 * copies the choice to the server for signed-in users. Deck exposes no state on
 * the button, so the icon that matches the theme in effect is the visible one.
 *
 * Set $themeToggleClass before requiring this to place it: 'push' in a .bar,
 * 'theme-toggle-floating' on a page with no header to hang it on.
 */
?>
<button type="button" class="btn btn-icon btn-ghost <?= htmlspecialchars($themeToggleClass ?? '', ENT_QUOTES, 'UTF-8') ?>" data-deck-theme aria-label="Switch between light and dark theme">
    <?= \EchoDial\Deck\Deck::icon('moon', 'icon when-light') ?>
    <?= \EchoDial\Deck\Deck::icon('sun', 'icon when-dark') ?>
</button>
<?php unset($themeToggleClass); ?>
