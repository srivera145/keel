# Deck migration: phase 2

**All 28 remaining views converted, and Tailwind removed.** Keel now has no npm, no bundler and no build step: `composer install` publishes Deck, and everything else is served as written.

This folder:

- `REPORT.md` — this file.
- `FINDINGS.md` — 8 new Deck defects (16–23), plus 4 phase-1 findings confirmed again.
- `screenshots/A|B|C|D/{before,after}/` — 370 captures at 375, 768 and 1280, light and dark.
- `screenshots/regression/after/` — 14 captures taken *after* the deletions, including both tenants.

Phase 1 is in [`../phase-1/`](../phase-1/): the inventory, the first converted view, findings 1–15.

## Self-check

| Asked for | Result |
|---|---|
| Per-view Tailwind-before vs Deck-after counts | All 30 files. [Class counts](#class-counts) |
| Screenshots at 375 / 768 / 1280 | 370 batch captures + 14 post-removal. Every batch, light and dark. |
| Every view loads with the `$ui` switch in mixed state | Route sweep after each batch: **28 routes, 0 problems**, no PHP notices. |
| Dark mode per view | Every view on Deck's `light-dark()` + `data-theme`. The Tailwind `--color-*` swap is gone. [Dark mode](#dark-mode) |
| Per-tenant theming, once per batch | Demonstrated each batch, and again after removal: `--hue-brand:265` vs `175`. [Theming](#per-tenant-theming) |
| JS-disabled confirmation | Every batch. Only the theme switch needs JS, and it hides itself. [No JavaScript](#without-javascript) |
| New Deck defects | 8 new (16–23), 4 phase-1 findings re-confirmed. [FINDINGS.md](FINDINGS.md) |
| **Batch E:** zero Tailwind references | `grep -rli tailwind` → one file, Deck's own. [Removal](#removing-tailwind) |
| **Batch E:** the Vite decision, with reasoning | Removed. It only built Tailwind. [Vite](#the-vite-decision) |
| Keel stays loadable throughout | PHPUnit **16/16 (105 assertions)** at every checkpoint, including after the deletions. |
| No application logic touched | No controller, model, route, middleware, auth, Stripe or queue file changed in either phase. |

## The batches

| | Views | Notable |
|---|---|---|
| **A** | 6 — `welcome`, `auth/login`, `errors/404`, `errors/500`, `billing/success`, `billing/cancel` | The landing page, the biggest file in the project. Login's tabs needed hand-written roving tabindex (Deck styles `.tabs`, ships no behaviour). |
| **B** | 3 — `settings/api-tokens`, `settings/organization`, `settings/activity` | `<dialog class="modal">` + `data-confirm` wiring (finding 16). `.table-stack` removed from the 40-row log (finding 17). |
| **C** | 3 — `billing/plans`, `super-admin/organizations`, `super-admin/activity` | The per-tenant hue swatch, which Deck can't do natively (finding 6). |
| **D** | 16 — `dashboard`, `onboarding/organization`, `docs/_layout`, `docs/index`, 12 docs prose pages | Each prose page collapsed to a single `prose` class. Three shared partials extracted. |
| **E** | — | Default flipped to Deck, switch removed, Tailwind and Vite deleted. |

Three partials were extracted rather than repeating markup: `partials/theme-toggle.php` (a `$themeToggleClass` placement hook), `partials/back-to-top.php`, and `partials/site-nav.php` (shared by the landing page and the docs shell).

## Class counts

Measured by `final-counts.mjs`, which has no npm dependencies — phase 1's classifier asked Tailwind's own compiler which names it owned, and Tailwind is gone. Deck's vocabulary and layers now come from `vendor/echodial/deck/API.md`; Keel's own classes from the selectors in `keel.css`. "Tailwind before" is phase 1's measurement of the pre-Deck tree, unchanged.

| | Before | After |
|---|---:|---:|
| Tailwind utilities | 1,676 | 0 |
| Keel component classes (`@apply`) | 260 | 0 |
| Deck components + layout | — | 481 |
| **Deck utilities** | — | **118** |
| Keel `app.*` classes | — | 37 |
| **All class tokens** | **1,936** | **638** |

**Utilities went 1,676 → 118.** Total class tokens fell to a third. Nine `app.*` classes carry everything Deck has no answer for: `settings-page`, `stage`, `docs-sections`, `table-empty`, `hue-swatch`, `otp-input`, `brand-logo`, `theme-toggle-floating`, `when-light`/`when-dark`.

Per view, worst first:

| View | TW before | Keel before | Deck comp | Deck util | `app.*` | After |
|---|---:|---:|---:|---:|---:|---:|
| `welcome.php` | 454 | 78 | 83 | 32 | 0 | 115 |
| `auth/login.php` | 93 | 38 | 43 | 3 | 6 | 52 |
| `billing/plans.php` | 90 | 11 | 39 | 23 | 1 | 64 |
| `settings/api-tokens.php` | 89 | 30 | 59 | 7 | 2 | 68 |
| `docs/_layout.php` | 84 | 20 | 13 | 2 | 1 | 16 |
| `docs/installation.php` | 81 | 0 | 1 | 0 | 0 | 1 |
| `settings/members.php` (phase 1) | 65 | 24 | 47 | 7 | 3 | 57 |
| `super-admin/organizations.php` | 53 | 10 | 35 | 6 | 4 | 45 |
| `docs/where-to-start-editing.php` | 53 | 0 | 3 | 0 | 0 | 3 |
| `settings/organization.php` | 48 | 1 | 21 | 4 | 1 | 26 |
| `errors/404.php`, `errors/500.php` | 34 | 5 | 12 | 3 | 2 | 17 |
| `billing/success.php` | 31 | 6 | 12 | 0 | 2 | 14 |
| `settings/activity.php`, `super-admin/activity.php` | 27 | 9 | 16 | 10 | 2 | 28 |
| `onboarding/organization.php` | 25 | 8 | 18 | 5 | 2 | 25 |
| `dashboard/index.php` | 22 | 1 | 11 | 2 | 1 | 14 |
| `billing/cancel.php` | 21 | 4 | 8 | 0 | 2 | 10 |
| `docs/index.php` | 18 | 1 | 8 | 0 | 0 | 8 |
| 11 other docs prose pages | 17–45 | 0 | 1 | 0 | 0 | **1** each |

**Two measurement caveats, both mine, not Deck's:**

1. The counter reads literal `class="…"` attributes, so it misses classes passed through PHP. `$themeToggleClass` supplies `push` on 9 pages and `theme-toggle-floating` on 5. Those 14 tokens are added to the totals above; the raw script prints 624.
2. It also extracts PHP string literals *inside* class attributes, so that conditional class names get counted. That pulls in two non-classes: `'status'` (an array key in `plans.php`) and `'UTF-8'` (an `htmlspecialchars` argument in `theme-toggle.php`). Artifacts, not unknown classes.

### Where the ratio is tightest

The brief's test: if the Deck count approaches the Tailwind count, the conversion went utility-for-utility.

- **The activity pages pass on inspection, not on ratio** (36 → 28). The page is small to begin with and is mostly one table — which in Tailwind carried classes on every cell and in Deck carries none at all (`<table class="table">`). The 10 utilities are one-per-element finishes: three `nums` on the timestamp, ID and IP columns, two `push` (the theme switch and the pager), then `fw-medium`, `text-sm`, `text-muted`, `sr-only` and `wrap`. Nothing stacks.
- **`billing/plans.php` is the one view I'd call a partial failure** (101 → 64, 23 utilities). The pair `text-sm text-muted` repeats seven times on plan copy, and a `w-full` sits on a form whose button already has `btn-block`. That is utility-first authoring surviving the conversion. The fix is one `app.components` class for plan copy — I did not apply it after the batch was screenshotted and swept. Say the word and it's a five-line change plus a re-capture.

## Per-tenant theming

Survives removal. Re-measured after Tailwind and Vite were deleted:

```
Acme Motors        <html lang="en" style="--hue-brand:265" data-theme="light">
Globex Logistics   <html lang="en" style="--hue-brand:175" data-theme="light">
both               pipeline: deck
```

The hue is still one `UPDATE` to `organizations.brand_hue` and nothing else — no rebuild, because there is nothing left to rebuild. `NULL` falls back to Keel's `--hue-brand: 38` in `app.base`. Acme was restored to `NULL` afterwards.

The known limit is unchanged (finding 6): the hue only works at `:root`, so the super-admin list of all tenants can't preview each one. `.hue-swatch` rebuilds Deck's `--brand-600` by hand — the one place Keel repeats a Deck value instead of reading it.

## Dark mode

One mechanism, everywhere: Deck's `light-dark()` tokens with `color-scheme`, switched by `data-theme` on `<html>`. Keel's old `--color-*` swap died with `app.css`.

The pre-paint script in `head.php` applies the saved theme before the first frame and seeds Deck's storage key, so there is no flash. Signed-in users' choices persist through `POST /settings/theme`, driven by a `MutationObserver` in `keel.js` (finding 5: Deck exposes no theme-change event).

## Without JavaScript

Confirmed each batch, at 375 and 1280:

- **Layout is identical**, with the server's theme applied.
- **The theme switch hides itself** via `<noscript><style>[data-deck-theme] { display: none; }</style></noscript>`.
- **Forms post normally.** The invite form, the login OTP flow and token revocation all work; revocation simply loses its confirmation step, because `data-confirm` only intercepts when JS is present.
- **Back-to-top** is a plain `#top` link; its show/hide is a CSS scroll-driven animation, not script.
- **Login tabs** degrade to both panels visible — the `hidden` attribute is only applied by script.

One caveat about method, not about Keel: Playwright's `waitForNavigation` and `scrollIntoViewIfNeeded` both hang with `javaScriptEnabled: false`, because its stability check waits on `requestAnimationFrame`. Forms were submitted with `page.press('#field', 'Enter')` instead. That is a Playwright limitation, not a defect in Deck or Keel.

## Removing Tailwind

`grep -rli tailwind`, excluding `.git` and `vendor/`:

```
./public_html/deck/deck.css
```

One match, and it is **Deck's own published file** — the string is Deck's marketing line, "The parts Tailwind makes you build yourself every time." That file is gitignored and rewritten by Composer on every install. Keel's own tree is clean. (`deck-migration/*.md` also matches, necessarily: these reports discuss Tailwind by name.)

`npm` survives in exactly two places, both sentences saying there isn't any: the README's "No npm, no bundler, no build step" and the installation doc's "There is no `npm install` and no build step."

**Deleted** — verified absent:

`tailwind.config.js` · `postcss.config.js` · `vite.config.js` · `package.json` · `package-lock.json` · `src/Core/Vite.php` · `resources/css/` · `resources/js/` · `public_html/assets/` · `node_modules/`

**Kept:** `resources/images/brand/` (source images, never part of the pipeline).

**Rewritten:** `README.md`, `CONTRIBUTING.md`, `.github/workflows/ci.yml` (Node and npm steps removed), `.gitignore`, and three docs pages — `theming.php`, `project-structure.php`, `installation.php`. `public_html/css/keel.css` was rewritten to remove Tailwind from its comments.

No source file references `Vite::` or the `$ui` switch any more.

## The Vite decision

**Removed.**

Phase 1 measured what it actually built: Tailwind's CSS, plus a 5.5 KB bundle of five vanilla ES modules. No TypeScript, no npm runtime packages, no images or fonts through the pipeline, no test touching it.

All five behaviours now have a home:

| Behaviour | Now |
|---|---|
| Theme toggle + server save | `[data-deck-theme]` + `keel.js` |
| Back-to-top | Deck's `.back-to-top`, shown by CSS |
| Copy buttons | Deck's `data-deck-copy` |
| Revoke modal | `<dialog class="modal">` + `data-confirm` in `keel.js` |
| Login tabs | Deck's `.tabs` + roving tabindex in `keel.js` |

With Tailwind gone, Vite had nothing left to build — keeping it would have meant a bundler, a `package.json` and a CI Node step to produce nothing. That is the case you described: it only built Tailwind, so removing it makes Keel genuinely build-stepless, and the README now says so.

**What you lose:** hot reload in development. **What you gain:** `public_html/assets/` stops being wiped by `emptyOutDir` (finding 10) — which is why Deck publishes to `public_html/deck/` — and CI drops a whole toolchain.

`public_html/js/keel.js` and `public_html/css/keel.css` were checked before being touched, as you asked: **both are hand-written**, not generated. Nothing emitted them; they are Keel's own source, edited in place.

## New Deck findings

Eight new, all measured — [FINDINGS.md](FINDINGS.md) has the evidence:

| # | Finding | Severity |
|---:|---|---|
| 16 | `.modal` documents `showModal()` but ships no wiring for it | Medium |
| 17 | `.table-stack` is all-or-nothing, unusable on a long table (11,481px → 3,836px) | Medium |
| 18 | `.split-rail-start` *does* change what markup order means — the comment is wrong | Medium |
| 19 | `.navbar` has no mobile menu without the 32 KB extras bundle | Medium |
| 20 | `.copy` only carries a single-line value | Low |
| 21 | `.grid`'s column floors strand the last card | Low |
| 22 | `.stepper`'s note is one line, and its vertical mode lays out in a row | Low |
| 23 | `.sidebar` is always a column; a horizontal rail needs app CSS | Low |

Findings 2, 5, 6 and 7 from phase 1 were each hit again across the remaining views. Finding 2 is the one that would bite any adopter hardest: **Deck's reset has no `[hidden]` rule**, so any Deck class that sets `display` overrides the attribute and the element stays on screen. Keel's one-line fix in `app.base` is load-bearing for the login tabs and the OTP step.

## Not verified

- Firefox and Safari. Chrome only, both phases.
- A real phone; 375px was emulated.
- A screen-reader pass.
- Stripe Checkout against live keys — `billing/plans.php` renders the unconfigured state.
- The docs prose pages are visually thinner than their Tailwind versions by design (one `prose` class). Worth your eye on whether that reads as intended.

## Decisions for you

1. **`billing/plans.php`'s repeated `text-sm text-muted`** — the one spot that stayed utility-first. Fix offered above.
2. **Keel's default hue, 38,** still renders brick rather than Keel's orange, and still sits close to Deck's danger red (findings 7, 8). Unchanged from phase 1 and still open.
3. **`deck-migration/`** is 1.6 MB of screenshots and reports. A `.git` directory appeared during phase 2; nothing was committed. Leave this folder out of the commit if you don't want it in history.
4. **`database/migrations/010_add_brand_hue_to_organizations.sql`** remains the only change outside views and assets across both phases.
