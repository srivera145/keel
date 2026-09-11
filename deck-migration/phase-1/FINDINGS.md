# Deck findings from Keel, phase 1

Every place Deck lacked something the Tailwind view had, or where a Deck component was awkward, found while converting `views/settings/members.php`.

Deck 0.1.3 from Packagist (`v0.1.3`, commit `a416e82`). Tested in headless Chrome on Windows 11. **Not yet checked in Firefox or Safari.**

- **Measured** — reproduced against `deck.css`/`deck.js` alone, with no Keel CSS on the page. These are the `screenshots/after/deck-harness-*` captures.
- **Source** — read from Deck's code, not reproduced separately.
- **Severity** — *High*: silently breaks layout or behaviour a consumer relies on. *Medium*: works, but needs Keel-side code or undercuts something Deck advertises. *Low*: ergonomics or docs.

| # | Finding | Severity | Evidence |
|---:|---|---|---|
| 1 | `.stack-N` lays out nothing without `.stack`; docs and demo say otherwise | High | Measured |
| 2 | The `hidden` attribute does not hide Deck components | High | Measured |
| 3 | A returning visitor's saved theme paints wrong first | High | Measured |
| 4 | The server's theme loses to an old local choice | Medium | Source |
| 5 | Changing the theme fires no event; the switch has no state | Medium | Measured |
| 6 | Retheming works on `<html>` only; a subtree override does nothing | Medium | Measured |
| 7 | A brand in the red–orange band collides with danger | Medium | Measured |
| 8 | One hue cannot carry a light, saturated brand | Medium | Measured |
| 9 | The Composer/PHP path can't serve minified core JavaScript | Medium | Source, sizes measured |
| 10 | The README's Helm publish path is wiped by Vite | Medium | Measured in Keel |
| 11 | A table inside a card draws a double frame | Low | Measured |
| 12 | `.table-stack` mishandles full-width cells and the last row | Low | Measured |
| 13 | App-screen sizing needs three knobs Deck doesn't name | Low | Measured in Keel |
| 14 | JavaScript-only controls have no no-JS convention | Low | Measured |
| 15 | Smaller PHP-helper rough edges | Low | Source |

---

## 1. `.stack-N` lays out nothing without `.stack`; docs and demo say otherwise

**High · measured**

First render: the page header sat directly on the cards and the two rail cards touched. `.stack-6` only sets `--gap`. `display: flex` and the `gap` that reads it live on `.stack`.

**Evidence**

- `deck-harness-stack-modifier.png`: `class="stack-6"` computes `display: block` with 0 px between children. `class="stack stack-6"` computes `display: flex` with 24 px.
- `API.md` describes `.stack-4` as *"Same as .stack with no step class."*
- Deck's demo pages use `stack-N` without `.stack` on **1,571 lines**, and together with `.stack` on **none**. Example: `public_html/index.php:321` `<section class="container hero stack-6">`.
- Some of those uses sit on an element that reads `--gap` (`.grid`, `.cluster`, `.split`, `.center`) and do work. On a plain block they do nothing.

**Keel's workaround:** `class="stack stack-6"`, in three places.

**Suggested fix:** add the `.stack-*` names to the `.stack` rule, or correct `API.md` and the demo. Related: Deck FINDINGS 22.

## 2. The `hidden` attribute does not hide Deck components

**High · measured**

**Evidence:** `deck-harness-hidden-attribute.png`.
- `<button class="btn" hidden>` computes `display: inline-flex`.
- `.card[hidden]` and `.stack[hidden]` compute `flex`.
- Only an unclassed `<p hidden>` is hidden.

`02-reset.css` has no `[hidden]` rule, and any author-layer `display` beats the browser's own stylesheet.

**Why it matters:** `el.hidden = true` is the standard progressive-enhancement move, and it silently fails on anything with a Deck class.

**Keel's workaround:** `@layer app.base { [hidden] { display: none; } }` in `public_html/css/keel.css`.

**Suggested fix:** `[hidden]:not([hidden="until-found"]) { display: none !important; }` in `deck.reset`.

## 3. A returning visitor's saved theme paints wrong first

**High · measured**

**Evidence:** a harness page with `localStorage['deck-theme'] = 'dark'` and `deck.js` held back 2.5 s.
- At 1.0 s, `<html>` has no `data-theme` and the body is `oklch(0.978 0.012 232)`, which is light.
- After load it turns dark.
- See `deck-harness-theme-flash-early.png` and `-late.png`.

`deck.js` restores the saved theme inside `DOMContentLoaded`. `Deck::head()` loads `deck.js` with `defer` and emits no pre-paint script, so the first frame is always the OS theme.

**Keel's workaround:** an inline script in `views/partials/head.php` applies the theme before first paint. The same test on Keel's page is already dark at 1.0 s (`keel-theme-before-deckjs.png`).

**Suggested fix:** have `Deck::head()` emit a four-line inline restore ahead of the stylesheet.

## 4. The server's theme loses to an old local choice

**Medium · source**

The README's pattern is `<html <?= Deck::theme(hue: …, mode: $user->theme) ?>>`, which writes `data-theme` server-side. On `DOMContentLoaded`, `deck.js` overwrites it with whatever `localStorage['deck-theme']` holds (`src/js/deck.js`, lines 1314–1317).

**Example:** a user who switched to light on their phone, but still has `dark` stored on their laptop, gets dark on the laptop, whatever the server says.

**Keel's workaround:** the head script copies a signed-in user's server preference into `deck-theme` before `deck.js` runs.

**Suggested fix:** skip the restore when `<html>` already carries a server-set `data-theme`, or mark it (`data-theme-source="server"`).

## 5. Changing the theme fires no event; the switch has no state

**Medium · measured**

- `Deck.theme()` sets the attribute and localStorage, then returns. Nothing is dispatched.
- `[data-deck-theme]` sets no `aria-pressed` (it was `null` before and after a press in the toggle check) and never changes its icon.
- Deck's own `public_html/php-helper.php` demo shows a moon in both themes.

**Keel's workaround**
- `public_html/js/keel.js` watches `data-theme` with a `MutationObserver` so it can `POST /settings/theme`.
- Five rules in `keel.css` (`.theme-toggle .icon-to-light` / `.icon-to-dark`) swap sun and moon.

**Suggested fix:** dispatch `deck:theme` with `{ theme }` on `document.documentElement`, and set `aria-pressed` on every `[data-deck-theme]`.

## 6. Retheming works on `<html>` only; a subtree override does nothing

**Medium · measured**

**Evidence:** `deck-harness-hue-38-and-subtree.png`. On a hue-38 page, a card with `style="--hue-brand:175"` still renders:
- `btn-primary` as `oklch(0.516 0.118 38)`
- `.g-brand` as a `38 → 72` gradient

`--brand-50` … `--brand-950`, `--brand` and `--focus` are computed once on `:root` and inherit as finished colours, so changing the hue lower down changes nothing.

**Why it matters for Keel:** per-tenant theming works on a tenant's own pages. But these can never show a second hue on the same page:
- a super-admin list of organisations
- a tenant switcher
- a "preview your brand colour" setting

**Suggested fix:** redeclare the brand ramp under a scoping hook (`:root, .theme-scope`), or document that `--hue-brand` must be set on `<html>`.

## 7. A brand in the red–orange band collides with danger

**Medium · measured**

Keel's `#ff6b3d` is hue 37.5, so `--hue-brand: 38`. Deck's `--hue-bad` is 24 and `--hue-accent` is 42.

**Evidence:** `deck-harness-hue-38-and-subtree.png`.
- `btn-primary` is `oklch(0.516 0.118 38)` and `btn-danger` is `oklch(0.52 0.165 24)`: the same lightness, 14° apart.
- `badge-brand` next to `badge-bad`, and `alert-info` next to `alert-bad`, are hard to tell apart.
- `btn-accent` looks more like Keel's real brand than `btn-primary` does.

Orange and red brands are common, and nothing in `Deck::theme()` or the docs warns about it.

**Keel's workaround:** none yet. This view has no destructive action, but API tokens (red *Revoke* buttons) will need a decision first.

**Suggested fix:** document the hue bands to avoid, or step `--hue-bad` and `--hue-accent` away when `--hue-brand` comes within about 25°.

## 8. One hue cannot carry a light, saturated brand

**Medium · measured**

Keel's brand is `oklch(70.5% 0.191 37.5)` with dark text on it. Deck fills `btn-primary` with `--brand-600`, which is fixed at 51.6% lightness and 0.118 chroma, with light text. Keel's orange therefore renders as brick (`after/members-1280-light.png`).

`--chroma-brand` raises chroma but not lightness. The fixed lightness is a deliberate contrast decision, but "retheme from one number" gives a tenant their hue, not their colour.

**Keel's workaround:** kept to the hue only, as the brief asked.

**Suggested fix:** document it. Consider a `--brand-fill` override paired with `contrast-color()` for the text on it.

## 9. The Composer/PHP path can't serve minified core JavaScript

**Medium · source, sizes measured**

- `Deck::js()` emits unminified `deck.js` (54,299 bytes) unless `bundle => true`.
- The bundle (`deck.bundle.min.js`, 59,728 bytes) also carries the extras and adapters.
- `Installer::DEFAULT_FILES` publishes no `deck.min.js` (32,556 bytes), `deck-extras.min.js` or `deck-adapters.min.js`.

The README's 8.9 KB (Brotli) core is therefore unreachable through `Deck::head()`. The CSS side already prefers `deck.min.css`.

**Keel's choice:** ships `deck.js` unminified. Loading only what the view needs outweighed minifying.

**Suggested fix:** publish the three `.min.js` files and prefer them in `js()`, the way `stylesheet()` does for CSS.

## 10. The README's Helm publish path is wiped by Vite

**Medium · measured in Keel**

The README says: *"On cPanel hosting and Helm sites … use `public_html/assets/deck`."*

Keel's Vite build has `outDir: 'public_html/assets'` with `emptyOutDir: true`, and CI runs `composer install` before `npm run build`. Every build would delete Deck. `public_html/assets` is a common bundler output folder.

**Keel's workaround:** `extra.deck.publish-to` is `public_html/deck`.

**Suggested fix:** one sentence in the README — publish outside any bundler's output directory.

## 11. A table inside a card draws a double frame

**Low · measured**

`deck-harness-card-table-wrap.png`: in `.card > .card-header + .table-wrap`, the wrap's own border and radius sit one pixel inside the card's. Deck's demo never nests them, yet a table in a card is the default layout for SaaS settings screens.

**Keel's workaround:** `.card > .table-wrap { border: 0; border-radius: 0; }`.

**Suggested fix:** ship that rule, or add a `.table-wrap-flush` variant.

## 12. `.table-stack` mishandles full-width cells and the last row

**Low · measured**

**Evidence:** `deck-harness-table-stack-colspan-375.png`, at 375 px.
- A `td[colspan]` holding an empty state becomes a `display: flex` label/value row with no label, so a 278 px empty state is pushed to one side of a 331 px cell.
- The last stacked row keeps its 12 px bottom margin, leaving a strip inside the wrap.
- Inside a card, the stacked rows' borders sit flush against the card edge.

`.dg-empty` exists for the data grid; `.table` has no equivalent.

**Keel's workaround:**
- `.table-empty` rules
- `tbody tr:last-child { margin-block-end: 0; }`
- a 0.75 rem inset on `.card > .table-wrap:has(> .table-stack)`

**Suggested fix:** return `.table-stack td[colspan]` to `display: block` without `::before`, zero the last row's margin, and add a `.table-empty` for `.table`.

## 13. App-screen sizing needs three knobs Deck doesn't name

**Low · measured in Keel**

- **Width.** Container steps are 40, 56, 76 and 90 rem. The common app width (Tailwind `max-w-5xl`, 64 rem) falls between them.
- **Page padding.** `.section` (about 6 rem at 1280 px) is the only page-padding primitive, and the demo uses it on app-like pages. A settings screen wants about 2.5 rem, one step beyond the largest utility (`-8`, 2 rem).
- **Title size.** `h1` is `--text-3xl`, 46 px at 1280 px: a display size for a screen title. Keel uses `<h1 class="h2">`.
- **Columns.** `.split` rails are a fixed width, so proportional columns like the Tailwind view's `1.2fr / 0.8fr` aren't available.

**Keel's workaround:** `.settings-page { --container: 64rem; --rail: 24rem; padding-block: var(--space-10); }` in `app.pages`.

## 14. JavaScript-only controls have no no-JS convention

**Low · measured**

With scripts off, `[data-deck-theme]` renders a button that does nothing, and Deck has no `no-js`/`js-only` hook to hide it. Related: Deck FINDINGS 57.

**Keel's workaround:** `<noscript><style>[data-deck-theme] { display: none; }</style></noscript>`. It computes `display: none` in every no-JS capture.

## 15. Smaller PHP-helper rough edges

**Low · source**

- **`extras` defaults to on.** `Deck::configure()` defaults `extras => true`, so every page loads `deck-extras.js` unless told otherwise. The option's comment lists *"datepicker, combobox, grid, toasts, QR"*, four of which live in `deck.js`.
- **Output depends on where it runs.** `asset()` and `stylesheet()` guess the web root from `$_SERVER['DOCUMENT_ROOT']`, which is unset from the CLI (PHPUnit, a queue worker rendering a view). There, the same template links `deck.css` (311 KB) with `?v=0.1.3` instead of `deck.min.css?v=<mtime>`. Keel passes `root` explicitly.
- **The installer's suggested tags point at unminified files.** After publishing it suggests `deck.css` and `deck.js`, while `Deck::head()` would emit `deck.min.css`.
- **Duplicate `data-theme`.** `htmlAttributes()` and `theme()` both emit `data-theme` when `configure(['theme' => …])` is set and `mode:` is also passed.
- **Stray space.** When `theme()` returns `''`, the tag reads `<html lang="en" >`.

---

## Already known to Deck, confirmed here

- **FINDINGS 24.** `.table-wrap` can't be scrolled from the keyboard. Keel adds `tabindex="0" role="region" aria-labelledby` to both wraps.
- **FINDINGS 71 (fixed).** The first press of the theme switch works on a light OS. Not re-checked on a dark OS.
