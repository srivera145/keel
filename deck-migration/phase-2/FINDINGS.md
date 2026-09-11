# Deck findings from Keel, phase 2

Found while converting the remaining 28 views and removing Tailwind. Numbering continues from
[phase 1](../phase-1/FINDINGS.md), which recorded findings 1–15 against the members view.

Deck 0.1.3 from Packagist (`v0.1.3`, commit `a416e82`). Headless Chrome on Windows 11. **Still not
checked in Firefox or Safari.**

- **Measured** — reproduced in a converted view or against Deck's own files.
- **Source** — read from Deck's code, not reproduced separately.
- **Severity** — *High*: silently breaks layout or behaviour. *Medium*: works, but needs Keel-side
  code or undercuts something Deck advertises. *Low*: ergonomics or docs.

| # | Finding | Severity | Evidence |
|---:|---|---|---|
| 16 | `.modal` documents `showModal()` but ships no wiring for it | Medium | Measured |
| 17 | `.table-stack` is all-or-nothing, and unusable on a long table | Medium | Measured |
| 18 | `.split-rail-start` does change what markup order means | Medium | Measured |
| 19 | `.navbar` has no mobile menu without `deck-extras.js` | Medium | Measured |
| 20 | `.copy` only carries a single-line value | Low | Measured |
| 21 | `.grid`'s column floors strand the last card | Low | Measured |
| 22 | `.stepper`'s note is one line, and its vertical mode lays out in a row | Low | Measured |
| 23 | `.sidebar` is always a column; a horizontal rail needs app CSS | Low | Measured |

---

## 16. `.modal` documents `showModal()` but ships no wiring for it

**Medium · measured**

Deck's `.modal` comment says to open it with `showModal()`, "never `show()` and never the open
attribute". Nothing in `deck.js` or `deck-extras.js` does that: `showModal` appears nowhere in
either file. Drawers get `data-deck-drawer` in the extras; modals get nothing.

Every consumer therefore writes the same opener. Keel's is in `public_html/js/keel.js`:
a form carrying `data-confirm="<dialog id>"` opens the dialog first, and a button carrying
`data-confirm-submit="<form id>"` posts it. With scripts off the form posts straight through, so
the action still works and only the confirmation step is lost.

**Suggested fix:** wire `data-deck-modal="#id"` in `deck.js` the way `data-deck-drawer` already is.

## 17. `.table-stack` is all-or-nothing, and unusable on a long table

**Medium · measured**

`.table-stack` restacks every row into one labelled line per column below 40rem. That reads well for
a short table and collapses on a long one.

**Evidence:** the organization activity log, 40 rows by 6 columns with a JSON metadata cell, at
375px:

| | Page height |
|---|---:|
| `.table` with `.table-stack` | 11,481px |
| `.table` alone, scrolling inside `.table-wrap` | 3,836px |

Deck offers nothing between the two for `.table` — no way to keep two columns and drop the rest.
`.dg-cards` exists for the data grid, which is a heavier component than a log table needs.

**Keel's choice:** `.table-stack` on the short tables (members, API tokens, organizations), and
plain `.table` inside the scroll container on both activity logs.

**Suggested fix:** a column-priority opt-in, e.g. `.table-stack` honouring `data-priority` on `th`.

## 18. `.split-rail-start` does change what markup order means

**Medium · measured**

The source comment says it "puts the rail on the starting edge without touching markup order, so the
main column stays first for a screen reader and for the keyboard."

It only swaps the two column widths:

```css
.split            { grid-template-columns: minmax(0, 1fr) var(--rail, 20rem); }
.split-rail-start { grid-template-columns: var(--rail, 20rem) minmax(0, 1fr); }
```

So the **first** child lands in the rail column either way. To get the rail on the left it has to be
first in the DOM — exactly the markup change the comment says is unnecessary. Keel's docs shell puts
the sidebar first and renders correctly; a reader following the comment would put the article first
and get a 16rem article beside a wide sidebar.

**Suggested fix:** either use `order` so the comment becomes true, or correct the comment.

## 19. `.navbar` has no mobile menu without `deck-extras.js`

**Medium · measured**

`.navbar-links` is `display: none` below 48rem, and the only menu Deck offers for that width is
`.drawer`, which lives in `deck-extras.js` (32 KB) and needs JavaScript. A marketing page with four
destinations either ships the extras bundle or loses its navigation on a phone.

**Keel's workaround:** no `.navbar-links` at all. The landing page and docs shell put the
destinations in a `.cluster`, which wraps to a second row on a phone and needs no JavaScript. It
replaced the Tailwind version's JS menu button outright.

**Suggested fix:** document the wrapping-cluster pattern as the no-JS navbar, or let `.navbar-links`
wrap instead of disappearing.

## 20. `.copy` only carries a single-line value

**Low · measured**

`.copy` sets `white-space: nowrap` with a horizontal scroll on its `code` child, so it is built for
one line: an API key, an order number. The landing page's install block is five lines, and putting
it in `.copy` would have collapsed it onto one.

The behaviour is available without the component — `data-deck-copy="#id"` reads any element's
`textContent` — so the button works, but it sits outside `.copy` and gets none of its styling.

**Keel's choice:** `.copy` for the API token (single line), a plain `.btn` with
`data-deck-copy="#hero-install"` beside the `<pre>` on the landing page.

**Suggested fix:** a `.copy-block` variant that wraps a `<pre>`.

## 21. `.grid`'s column floors strand the last card

**Low · measured**

`.grid-wide` sets a 24rem column floor. In a 76rem container with the default gap that fits two
columns, not three, so a three-item row became two-plus-one in three places on the landing page
(feature cards, hero stats, the "Why Keel" paragraphs).

The fix is Deck's own knob, `--min`, set per grid — but the named variants (`grid-tight`,
`grid-wide`) read as if they cover the common cases, and the one that matches "three cards across"
is unnamed.

**Keel's workaround:** `style="--min: 20rem"` and friends on three grids.

## 22. `.stepper`'s note is one line, and its vertical mode lays out in a row

**Low · measured**

`.step-note` is `--text-xs` in `--text-faint`: a caption. A quickstart step with a five-line code
block does not fit it. In `.stepper-vertical` and `.stepper-auto` below 40rem, `.step` becomes
`flex-direction: row`, so the marker, the label and the note sit side by side rather than stacked —
Deck's own demo markup does the same.

**Keel's choice:** the quickstart stayed an ordered list of cards, with the step number as a
`.badge`. The sequence is real, so the numbering carries information; the stepper's shape did not
fit the content.

## 23. `.sidebar` is always a column; a horizontal rail needs app CSS

**Low · measured**

`.sidebar` is `flex-direction: column` at every width. Deck's `.scroller` is the row equivalent, but
the two do not compose: `.sidebar.scroller` inherits the column direction from `.sidebar`, which
wins on source order.

The docs rail needs to be a column beside the article and a scrollable row above it on a phone, so
Keel writes that media query itself in `app.pages`. Deck's own docs site solves this somewhere; the
framework does not ship the answer.

---

## Phase 1 findings confirmed again

- **2 (the `hidden` attribute).** Hit twice more: the login page's tab panels and the OTP step both
  use `hidden`, and both need Keel's `[hidden]` rule in `app.base` to work at all.
- **5 (no theme state).** Every converted view uses the shared toggle partial, so the five-rule icon
  swap and the `MutationObserver` persistence are paid once rather than per view — but they are
  still Keel's code, for a control Deck ships.
- **6 (hue only on `<html>`).** The super-admin organizations list is the case phase 1 predicted: it
  shows several tenants at once and cannot preview each brand. Keel draws the swatch by rebuilding
  Deck's `--brand-600` by hand in `.hue-swatch` — the one place Keel repeats a Deck value.
- **7 (brand beside danger).** The API tokens page puts red `btn-danger` Revoke buttons beside the
  hue-38 brand. They are distinguishable, but only just; at a glance the page reads as two reds.
