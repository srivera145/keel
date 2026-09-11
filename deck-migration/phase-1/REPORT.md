# Deck migration: phase 1

**One view converted: `views/settings/members.php`.** The other 30 files that use Tailwind classes are untouched and still build through Vite. Stopped here for review.

This folder:

- `REPORT.md` — this file.
- `INVENTORY.md` — the Tailwind inventory, every number.
- `FINDINGS.md` — 15 Deck defects and awkward spots, with evidence.
- `changes.diff` — every changed source file (7 files, +274 / −62).
- `screenshots/before/`, `screenshots/after/` — captures from the real app at `keel.local`. The `deck-harness-*` captures are Deck alone, with no Keel CSS.

## Self-check

| Asked for | Result |
|---|---|
| Full inventory, in numbers | 31 files, 1,679 Tailwind class uses, 242 distinct classes, 107 `@apply` utilities, 0 `dark:` variants, 39 distinct arbitrary values, 25 raw palette classes. [INVENTORY.md](INVENTORY.md) |
| Can Vite be removed? What depends on it? | Not yet. It builds Tailwind for 30 files plus a 5.5 KB bundle of five vanilla modules; nothing else. [Vite](#vite) |
| `composer require` output showing the publish | [Below](#composer-require) — `Deck 0.1.3 published to public_html/deck (8 copied, 0 unchanged)` |
| Screenshots before and after at 375, 768, 1280 | `screenshots/before/members-{375,768,1280}-{light,dark}.png`, and the same names under `after/` |
| Tailwind removed vs Deck added | 65 Tailwind utilities + 24 Keel `@apply` classes removed. 58 Deck classes added: 41 component, 9 layout, 1 type, **7 utility**. [Counts](#class-counts) |
| Per-tenant theming | One view, two tenants, `--hue-brand:265` and `--hue-brand:175` inline on `<html>`, identical stylesheet URLs, no build. [Tenants](#per-tenant-theming) |
| Dark mode through Deck's mechanism | `light-dark()` tokens plus `data-theme`, switched by `[data-deck-theme]` / `Deck.theme()`. Kept on the server for signed-in users; no flash. [Dark mode](#dark-mode) |
| Every Tailwind construct with no Deck equivalent | 11 in this view. [Table](#tailwind-with-no-deck-equivalent) |
| Every Deck defect or awkwardness | 15: 3 high, 7 medium, 5 low. [FINDINGS.md](FINDINGS.md) |
| Works with JavaScript disabled | Yes. Only the theme switch needs JS, and it is hidden without it. [No JavaScript](#without-javascript) |
| Keel stays loadable | PHPUnit 16/16 (105 assertions). The Tailwind views (API tokens, dashboard, organization) render as before with the Tailwind bundle and no Deck. |

## Why this view

`settings/members.php` has everything the brief asked for:

- a form: email, select, submit
- two tables
- buttons and a navigation link
- success and error alerts
- a badge
- empty states
- the arbitrary two-column grid

It is also the one settings page whose controller already passes the organization row (`SELECT *`), so per-tenant theming needed no controller change.

`settings/api-tokens.php` exercises more of Deck (modal, copy button, date field, warning alert) and is the right next view. Its controller does not pass the organization, though — see [Before phase 2](#before-phase-2).

## What changed

| File | Change |
|---|---|
| `composer.json`, `composer.lock` | Requires `echodial/deck ^0.1.3`. Adds Deck's opt-in `post-install-cmd`, `post-update-cmd` and `deck-publish` scripts, with `extra.deck.publish-to: public_html/deck` and `auto-publish: true`. |
| `.gitignore` | Adds `/public_html/deck/` (Composer republishes it). |
| `views/partials/head.php` | A view that sets `$ui = 'deck'` gets `Deck::head()`, Keel's layer CSS and JS, and a pre-paint theme script. Every other view still gets `Vite::assets()`. |
| `views/settings/members.php` | Converted. |
| `public_html/css/keel.css` | New, 79 lines. Keel's rules in `app.base`, `app.components`, `app.pages`. No `!important`. |
| `public_html/js/keel.js` | New, 38 lines. Saves a Deck theme change to the server for signed-in users. |
| `database/migrations/010_add_brand_hue_to_organizations.sql` | New. Nullable `organizations.brand_hue`. |

No controller, model, route, middleware, auth, Stripe or queue code changed. The migration is the only change outside views and assets (decision 1).

### composer require

`extra.deck` and the scripts went into `composer.json` first, then:

```
./composer.json has been updated
Running composer update echodial/deck
Loading composer repositories with package information
Updating dependencies
Lock file operations: 1 install, 0 updates, 0 removals
  - Locking echodial/deck (v0.1.3)
Writing lock file
Installing dependencies from lock file (including require-dev)
Package operations: 1 install, 0 updates, 0 removals
  - Installing echodial/deck (v0.1.3): Extracting archive
Generating optimized autoload files
32 packages you are using are looking for funding.
Use the `composer fund` command to find out more!
> EchoDial\Deck\Installer::postInstall
Deck 0.1.3 published to public_html/deck (8 copied, 0 unchanged)
  Add to your layout:
    <link rel="stylesheet" href="/deck/deck.css">
    <script src="/deck/deck.js" defer></script>
No security vulnerability advisories found.
Using version ^0.1.3 for echodial/deck
```

`public_html/deck/` now holds `deck.css`, `deck.min.css`, `deck-icons.svg`, `deck.js`, `deck-extras.js`, `deck-adapters.js`, `deck.bundle.js` and `deck.bundle.min.js`.

Deck is published to `public_html/deck`, not the README's `public_html/assets/deck`. Vite's `emptyOutDir` wipes `public_html/assets/` on every build, and CI builds after `composer install` (FINDINGS 10).

## How Tailwind and Deck are kept apart

- **One pipeline per page.** `head.php` emits either `Deck::head()` or `Vite::assets()`, never both.
  - Checked in the rendered HTML: `/settings/members` loads `/deck/deck.min.css` and nothing from `/assets/`.
  - `/settings/api-tokens`, `/dashboard` and `/settings/organization` load the Tailwind bundle and nothing from `/deck/`.
- **Tailwind's scan doesn't pick up Deck.** Tailwind still scans `views/**/*.php`, so the Deck class names in the converted view could have added rules to its build. They didn't: the rebuilt CSS lost `alert-success` and `badge-neutral` (only members used them) and gained nothing (283 → 281 class selectors).
- **One theme preference.** Both pipelines read and write `users.theme_preference`, so a switch made on a Deck page carries to a Tailwind page and back.
- **Keel's layer loads last.** Keel's CSS for Deck views sits in the `app.*` layers and loads after `deck.css`.

## Class counts

| In `views/settings/members.php` | Before | After |
|---|---:|---:|
| Tailwind utilities | 65 (38 distinct) | 0 |
| Keel component classes built with `@apply` | 24 (15 distinct) | 0 |
| Deck components (`deck.components`) | — | 41 (25 distinct) |
| Deck layout primitives (`deck.layout`) | — | 9 (6 distinct) |
| Deck type (`deck.type`) | — | 1 (`h2`) |
| **Deck utilities** (`deck.utilities`) | — | **7 (5 distinct)** |
| Keel `app.*` classes | — | 4 (3 distinct) |
| All class tokens | 89 | 62 |

58 Deck classes against 65 Tailwind utilities looks close, so I checked where they went.

- **Component parts:** 41 of the 58 (`card` / `card-header` / `card-title`, `field` / `label` / `input`).
- **Layout primitives:** 9 of the 58 (`container`, `stack`, `bar`, `split`).
- **Utilities:** 7, all small typographic finishes — `fw-medium` ×2, `uppercase` ×2, `text-muted`, `nums`, `push`. No element carries more than one.

Utility count went 65 → 7. No part of the page needed a stack of Deck utilities. Three places needed a Keel class instead: `.settings-page`, `.table-empty` and `.theme-toggle`.

Keel's own components mapped like this:

| Keel (Tailwind `@apply`) | Deck |
|---|---|
| `btn btn-secondary btn-md w-full` | `btn btn-primary btn-block` |
| `card` (padded) | `card` › `card-header` › `card-title`, `card-body` |
| `alert alert-success mb-6 px-4 py-3` | `alert alert-good`, plus an icon |
| `alert alert-error mb-6 px-4 py-3` | `alert alert-bad`, plus an icon |
| `badge badge-neutral` | `badge` |
| `form-label`, `form-input` (input and select) | `field` › `label` + `input` / `select` |
| `table` | `table table-stack` (restacks below 40 rem) |
| `empty-state`, `-icon`, `-title`, `-text` | `empty`, `empty-art`, `empty-title`, `p` |
| page header: eyebrow `<p>` + right-hand link | `breadcrumb` |

## Per-tenant theming

Same view, same deploy, two tenants. Rendered HTML:

```
Acme Motors        <html lang="en" style="--hue-brand:265" data-theme="light">
Globex Logistics   <html lang="en" style="--hue-brand:175" data-theme="light">

both               <link rel="stylesheet" href="/deck/deck.min.css?v=1789159982">
                   <link rel="stylesheet" href="/css/keel.css?v=1789160469">
```

**How it works.** The hue is `organizations.brand_hue`, changed with an `UPDATE` and nothing else. The view passes it to `Deck::theme(hue: …, mode: Theme::serverPreference())` on `<html>`. When `brand_hue` is `NULL`, `keel.css` supplies Keel's own hue: `--hue-brand: 38`, from `#ff6b3d` = `oklch(70.5% 0.191 37.5)`. The main "after" screenshots show that default.

**Screenshots:** `after/tenant-acme-{1280,375}-{light,dark}.png` and `after/tenant-globex-{1280,375}-{light,dark}.png`.

**Limits found:**
- Retheming only works on `<html>` (FINDINGS 6).
- Hue 38 sits next to Deck's danger red (FINDINGS 7).
- A hue can't reproduce Keel's light orange (FINDINGS 8).

## Dark mode

**One mechanism is live on the page:** Deck's `light-dark()` tokens with `color-scheme`, switched by `data-theme` on `<html>`. Keel's `--color-*` swap lives in the Tailwind CSS, which this view no longer loads.

- **Server side:** the signed-in user's preference is written by `Deck::theme(mode: Theme::serverPreference())`.
- **The switch:** `[data-deck-theme]` calls `Deck.theme()`. `keel.js` sees the attribute change and saves it with `POST /settings/theme`, the same endpoint the Tailwind views use.
- **Before paint:** a script in `head.php` applies the saved theme before the first frame, and copies the server's value into Deck's storage key so `deck.js` restores the same one.

Measured with the switch:

| Step | `data-theme` | `localStorage['deck-theme']` | Server |
|---|---|---|---|
| Load, server says light | `light` | `light` | — |
| Press the switch | `dark` | `dark` | `POST /settings/theme` → 200 |
| New browser, empty storage | `dark` | `dark` | read from the server |
| Press again | `light` | `light` | `POST` → 200 |
| Reload | `light` | `light` | 0 requests |

- **Icon.** The moon becomes a sun and back. `aria-pressed` stays unset, because Deck sets no state (FINDINGS 5).
- **No flash.** The test user has no server preference and a saved dark choice, with `deck.js` held back 2.5 s. Keel's page is already dark 1.0 s in (`after/keel-theme-before-deckjs.png`). Deck alone, same test, is light at 1.0 s (`after/deck-harness-theme-flash-early.png`).
- **Dark mode now works on this view.** On Tailwind it did not (`before/members-1280-dark.png`).

## Without JavaScript

- **Layout.** `after/nojs-members-375-light.png`, `-1280-light.png` and `-1280-dark.png` show the same layout as with scripts on, with the server's theme applied.
- **Theme switch.** It is hidden: computed `display: none`, via a `<noscript>` style.
- **Invite form.** It posts with scripts off. Enter in the email field goes to `/settings/members?status=invite_sent`, and the success alert and the new invite row render (`after/nojs-invite-submitted-375.png`).
- **What needs JavaScript:** only the theme switch. Back-to-top is shown and hidden by a scroll-driven CSS animation; without that it is a plain link to `#top`.
- **Problems:** none. There were no console errors, failed requests or 4xx responses in any capture.
- **No horizontal overflow** at any width. The Tailwind page was 430 px wide at a 375 px viewport.

## Tailwind with no Deck equivalent

Every construct in this view without a direct Deck counterpart, and what was done.

| Tailwind in the view | Why there is no direct match | Resolution |
|---|---|---|
| `lg:grid-cols-[minmax(0,1.2fr)_minmax(320px,0.8fr)]` | Arbitrary value. Deck's `.split` rail is a fixed width. | `.split` with `--rail: 24rem`, set once in `.settings-page` |
| `max-w-5xl` (64 rem) | Deck containers step 40, 56, 76, 90 rem | `.container` with `--container: 64rem` in `.settings-page` |
| `py-10` (2.5 rem) | Utilities stop at 2 rem, and `.section` is about 6 rem | `padding-block: var(--space-10)` in `.settings-page` |
| `space-y-6`, `space-y-4` | Deck spaces with gap, not sibling margins | `.stack.stack-6`; the form takes `.card-body`'s gap |
| `mb-8`, `mb-6`, `mt-4` | Same | Removed. The page is one `.stack`. |
| `bg-gray-50`, `text-gray-900`, `text-gray-500`, `hover:text-gray-900`, `border-gray-200` | Deck has no raw palette | Removed in favour of Deck's surface and text tokens. These classes were what broke dark mode. |
| `text-3xl font-bold` title | Deck's `h1` is display size (46 px at 1280) | `<h1 class="h2">` |
| `mt-4 overflow-x-auto rounded-xl border border-gray-200` table wrapper inside a card | `.table-wrap` inside `.card` draws a second frame | `.table-wrap`, flattened inside cards by `keel.css` (FINDINGS 11) |
| `p-6` on the empty-state cell | `.dg-empty` exists only for the data grid | `.table-empty` in `keel.css` (FINDINGS 12) |
| `text-xs text-gray-500` | Deck tables are already small text; no smaller muted step | `.text-muted` (a size step smaller dropped) |
| JS-injected floating theme button and back-to-top button (`app.js`) | Not classes, but part of the view | `[data-deck-theme]` button in the header; Deck's `.back-to-top` link |

This view has no `space-x-*`, `divide-*` or `ring-*`. The other views hold 38 more distinct arbitrary values (INVENTORY.md §4), each of which needs the same kind of decision.

## Weight

Bytes a browser downloads for the view on a first visit, measured from the served files:

| | Tailwind view | Deck view |
|---|---|---|
| CSS | `app.css` 27,891 raw · 6,090 gzip | `deck.min.css` 172,836 · 32,516, plus `keel.css` 3,663 · 1,541 |
| JavaScript | `app.js` 5,549 · 2,103 | `deck.js` 54,299 · 14,753, plus `keel.js` 1,404 · 744 |
| Icon sprite | — | `deck-icons.svg` 110,032 · 33,523 |
| **Total** | **33,440 raw · 8,193 gzip · 7,145 Brotli** | **342,234 raw · 83,077 gzip · 68,593 Brotli** |

The Deck view downloads about ten times as much.

- **Why.** Tailwind's CSS is purged to what Keel's views use. Deck ships the whole framework and the whole 75-icon sprite, whatever the page uses.
- **What softens it.** Deck's cost is fixed and cached across views, so it won't grow as more views convert, and the Tailwind bundle disappears at the end.
- **Two cheap cuts:**
  - `deck.min.js` would save 4.8 KB gzip, but the PHP path can't emit it (FINDINGS 9).
  - A sprite trimmed to the icons Keel uses, via Deck's `icons.txt`, removes most of the 33.5 KB.

## Vite

**It can't be removed yet.**

**What it builds:** two things.
- Tailwind's CSS, which the other 30 files still need.
- A 5.5 KB bundle of five vanilla modules.

**What else depends on it:** nothing.
- No npm runtime packages.
- No TypeScript.
- No images or fonts through the pipeline.
- No test touches it.

**Recommendation:** remove it at the end of the migration, once the last view is off Tailwind and these five behaviours have a home.

| Behaviour in `app.js` | Used by | Replacement |
|---|---|---|
| Theme toggle and server save | every page | `[data-deck-theme]` + `keel.js`. Done for Deck views. |
| Back-to-top | every page | Deck `.back-to-top`. Done. |
| Copy buttons | API tokens | `deck-extras.js`, `data-deck-copy` |
| Revoke modal | API tokens | `<dialog class="modal">` and a few lines of `showModal()` in `keel.js` |
| Tabs | login | Deck styles `.tabs` but ships no behaviour. Either keep `tabs.js` as a static file, or use a `.segmented` radio group, which needs none. |

**Then delete:**
- `vite.config.js`, `postcss.config.js`, `tailwind.config.js`
- `package.json` and its lock file
- `src/Core/Vite.php`
- the CI build step

**Then update:** the README, CONTRIBUTING, the two docs pages, and the "Tailwind + Vite" copy in `welcome.php` and `billing/plans.php`.

**What you lose:** hot reload in development.

**What you gain:** `public_html/assets/` stops being wiped. That is where the three PWA icons that 404 today belong (INVENTORY.md §6).

## Decisions for you

1. **`organizations.brand_hue`** (migration 010) is a schema change. It is the only place a per-tenant hue can live; it is nullable, with no backfill.
2. **The page's single action changed emphasis.** Keel's dark-grey `btn-secondary` became Deck's `btn-primary` (brand fill). Deck reserves primary for the one important action on a screen, which this is, and it is what shows the tenant's hue.
3. **Header.** The "Organization members" eyebrow and the right-hand "Organization settings" link became one breadcrumb: *Organization settings / Members*.
4. **Theme switch.** The floating switch `app.js` injects became a button in the page header; its sun and moon follow the theme.
5. **Small content edits.**
   - The expiry cell lost its "Expires " prefix: the column says it, and at 375 px the stacked row read "Expires Expires 2026-…".
   - The empty-state letters (`m`, `i`) became Deck icons.
   - Labels gained `for` / `id`.
6. **Keel's default hue, 38,** renders brick rather than Keel's orange, and sits next to danger (FINDINGS 7, 8). Accept it, choose another hue, or wait for Deck.
7. **Screenshots are in this folder** (1.6 MB). Leave `deck-migration/` out of the commit if you don't want them in history.

## Before phase 2

- **Tenant for views without `$organization`.** API tokens, dashboard and billing views can't see the tenant today. The clean fix is to resolve it once in middleware and share it with every view. That is a middleware and `View` change, so it needs your go-ahead.
- **Brand vs danger.** The red/orange collision needs a decision before API tokens, where a red *Revoke* sits beside a hue-38 brand.
- **Next view.** Suggested: `settings/api-tokens.php`.
  - modal → `<dialog class="modal">`
  - copy button → `deck-extras.js`
  - `datetime-local` → Deck's date picker
  - amber notice → `.alert-warn`

## Not verified

- Firefox and Safari (only Chrome was tested).
- The first press of the theme switch on a dark OS.
- A real phone; 375 px was emulated.
- A screen reader pass.

## Local environment, not part of the change set

- **`.env`** created (gitignored), with `MULTI_TENANCY_ENABLED=true`, `MAIL_MAILER=log` and the local MariaDB.
- **Databases.**
  - `keel`: migrated and seeded with two demo tenants, *Acme Motors* and *Globex Logistics* (local only).
  - `keel_test`: had migration 010 applied by `composer test:all`.
- **Build output.** `vendor/` and `node_modules/` installed; `public_html/assets/` rebuilt.
- **No git repository,** so there are no commits. To keep the app loadable throughout, Deck was installed and publishing before any view referenced it, and the test suite and every screenshot ran after the last change.
