<?php
/**
 * Deck's back-to-top link. It shows and hides with a scroll-driven CSS
 * animation, so it needs no JavaScript; deck.js only fills in for browsers
 * without animation-timeline. Pair it with <span id="top" tabindex="-1"></span>
 * as the first element in <body>, which is where it sends focus.
 */
?>
<a class="back-to-top" href="#top" aria-label="Back to top"><?= \EchoDial\Deck\Deck::icon('chevron-up') ?></a>
