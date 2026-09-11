# Contributing to Keel

Thanks for contributing.

## Local setup

Use the setup in [README.md](README.md), especially:

1. Install Composer and npm dependencies.
2. Configure `.env`.
3. Follow the XAMPP `keel.local` virtual-host steps.
4. Run migrations.
5. Start the Vite pipeline.

Docker is available as an optional contributor path in the README, but XAMPP + `keel.local` is still the primary workflow used by this repository.

## Project conventions

Keel keeps application code intentionally simple and explicit.

1. Keep controllers thin in `src/App/Controllers/`.
2. Put business logic in `src/App/Services/`.
3. Keep views as plain PHP files in `views/` with no templating engine.
4. Follow PSR-4 under the `Keel\` namespace as configured in `composer.json`.

## UI component classes

Use shared component classes from `resources/css/components.css` instead of re-deriving long utility strings in views.

Re-theme cloned projects by editing the `:root` token block in `resources/css/app.css`. Component classes and Tailwind semantic utilities resolve through those CSS variables, so color changes do not require editing views or rebuilding config.

1. Buttons:
`btn` base with variants `btn-primary`, `btn-secondary`, `btn-danger`.

Size scale (compose with variants):
`btn-sm`, `btn-md`, `btn-lg`, `btn-xl`, `btn-2xl`.

Use `btn-sm` for compact table actions, `btn-md` for default form and card actions, `btn-lg` and above for prominent marketing CTAs.
2. Cards:
`card` for standard panel containers.
3. Badges:
`badge` base, `badge-neutral` and `badge-success` for status/role pills.

Badge sizes:
`badge-sm` for dense rows and `badge-lg` for more prominent status chips.
4. Alerts:
`alert` base, `alert-error`, and `alert-success` for feedback messages.
5. Forms:
`form-label` and `form-input` for common label/input treatment.
6. Tables and pagination:
`table` for list/table structure, plus `pagination`, `pagination-links`, and `pagination-link` for paged views.
7. Empty states:
`empty-state`, `empty-state-icon`, `empty-state-title`, and `empty-state-text` for intentional zero-row/table states.
8. Shared behavior helpers:
Use `data-tabs` + `data-tab-target` + `data-tab-panel` for reusable tabs, and `data-modal-open` + `data-modal-close` with `modal`/`modal-panel` for confirmation dialogs.

If a screen needs a visual treatment these classes do not cover, compose from these first and only add local utilities for the difference.

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
