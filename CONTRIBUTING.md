# Contributing to Keel

Thanks for contributing.

## Local setup

Use the setup in [README.md](README.md), especially:

1. Install Composer dependencies.
2. Configure `.env`.
3. Follow the XAMPP `keel.local` virtual-host steps.
4. Run migrations.

There is no front-end build step. `composer install` publishes Deck's stylesheet, icon sprite and scripts into `public_html/deck/`.

Docker is available as an optional contributor path in the README, but XAMPP + `keel.local` is still the primary workflow used by this repository.

## Project conventions

Keel keeps application code intentionally simple and explicit.

1. Keep controllers thin in `src/App/Controllers/`.
2. Put business logic in `src/App/Services/`.
3. Keep views as plain PHP files in `views/` with no templating engine.
4. Follow PSR-4 under the `Keel\` namespace as configured in `composer.json`.

## UI components

Views are built from [Deck](https://get-deck.dev)'s components. Reach for a component before composing utilities: if a view ends up with a stack of them, the component either exists already or belongs in Keel's own layer.

1. Buttons:
`btn` base with variants `btn-primary`, `btn-danger`, `btn-ghost`, `btn-outline`, `btn-soft`. Sizes are `btn-sm` and `btn-lg`; `btn-block` fills its container. Use exactly one `btn-primary` per screen.
2. Surfaces:
`card` with `card-header`, `card-body`, `card-footer`, and `card-title` for the heading.
3. Badges and alerts:
`badge` with `badge-good`, `badge-warn`, `badge-bad`, `badge-brand`. `alert` with `alert-good`, `alert-warn`, `alert-bad`, `alert-info`, plus `alert-title` and `alert-body`.
4. Forms:
`field` wrapping a `label` and an `input`, `select` or `textarea`. `help` for a hint, `error` for a validation message.
5. Tables:
`table-wrap` around `table`. Add `table-stack` for a short table that should restack on a phone; leave it off a dense log, where stacking is unreadable. `table-empty` is Keel's own class for a full-width empty-state row.
6. Layout:
`container`, `stack` with a step (`stack stack-6`), `cluster`, `bar` with `push`, `split` for content plus a rail, and `grid` with `--min` when the default column floor strands a card.
7. Empty states:
`empty` with `empty-art` and `empty-title`.

### Keel's own CSS

Anything Deck does not cover goes in `public_html/css/keel.css`, inside the `app.base`, `app.components` and `app.pages` layers Deck reserves. A rule in those layers beats Deck without `!important`. Re-theme a cloned project by changing `--hue-brand` in `app.base`; see the Theming page in the docs.

Never edit `public_html/deck/` — Composer republishes it on every install.

### Shared partials and behaviour

- `views/partials/head.php` emits Deck's tags, Keel's CSS and JS, and the pre-paint theme script.
- `views/partials/site-nav.php`, `views/partials/theme-toggle.php` and `views/partials/back-to-top.php` are shared chrome.
- `data-deck-theme` on a button flips the theme; `public_html/js/keel.js` saves it for signed-in users.
- `data-confirm="<dialog id>"` on a form asks for confirmation through Deck's `modal` before posting, and posts straight through when JavaScript is off.
- Set `$deckExtras = true` in a view that needs `deck-extras.js` (the copy button, drawer or carousel).

## Running tests before a PR

### Test database setup (required for feature tests)

Feature tests run against a dedicated test database configured via `.env.testing` and `DB_DATABASE_TEST`.

1. Create a separate local database (example: `keel_test`).
2. Copy or edit `.env.testing` so `DB_DATABASE_TEST` points to that database.
3. Keep your development database in `.env` unchanged.
4. Run PHPUnit; the feature harness applies pending SQL migrations to `DB_DATABASE_TEST` automatically.

One-command bootstrap for local feature tests:

```powershell
composer test:db
```

Then run the feature suite:

```powershell
composer test:feature
```

Run all tests (unit + feature) with DB bootstrap:

```powershell
composer test:all
```

Do not point `DB_DATABASE_TEST` to your regular development database.

Run tests locally before you open a pull request:

```bash
./vendor/bin/phpunit
```

On Windows PowerShell:

```powershell
vendor\bin\phpunit
```

Run only feature tests:

```powershell
vendor\bin\phpunit --testsuite Feature
```

## Pull request expectations

1. One feature or fix per PR.
2. Include a clear summary of what changed.
3. Explain why the change is needed and any tradeoffs.
4. Mention tests you ran.
