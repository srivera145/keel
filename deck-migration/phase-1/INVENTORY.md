# Tailwind inventory: Keel before the Deck migration

Measured on the untouched tree, not estimated. Every `class="…"` attribute in `views/**/*.php` (including string literals inside `<?= … ?>` in an attribute), every `className =` / `classList.add|remove|toggle('…')` and every `data-tabs-*-class` in `resources/js/**/*.js` was tokenised. Each token was then handed to **Tailwind 3.4 itself, with Keel's `tailwind.config.js`**; a token counts as a Tailwind class only if Tailwind generated a rule for it. Tokens defined by Keel's own CSS (`resources/css/*.css`) or by `<style>` blocks inside views are counted separately.

| | |
|---|---:|
| Files containing Tailwind classes | **31** |
| Tailwind class instances in markup and JS | **1679** |
| Distinct Tailwind classes | **242** |
| `@apply` of Tailwind utilities in `resources/css/components.css` | **107** (58 distinct) |
| `dark:` variants | **0** |
| Arbitrary values (`[…]`) | **39** distinct, 190 uses |
| Raw palette classes (`gray-500`, `amber-50`, `white`…) | **25** distinct, 141 uses |

## 1. Files containing Tailwind classes

"Keel CSS" is Keel's own component classes (`.btn`, `.card`, `.table`…) — built from Tailwind with `@apply`, so they disappear with Tailwind too. "View CSS" is classes defined in a `<style>` block inside the view.

| File | Tailwind uses | Distinct | Keel CSS uses | View CSS uses |
|---|---:|---:|---:|---:|
| `views/welcome.php` | 454 | 154 | 4 | 74 |
| `views/auth/login.php` | 93 | 56 | 21 | 16 |
| `views/billing/plans.php` | 90 | 47 | 11 | 0 |
| `views/settings/api-tokens.php` | 89 | 60 | 30 | 0 |
| `views/docs/_layout.php` | 84 | 60 | 17 | 3 |
| `views/docs/installation.php` | 81 | 16 | 0 | 0 |
| `views/settings/members.php` | 65 | 38 | 24 | 0 |
| `views/docs/where-to-start-editing.php` | 53 | 22 | 0 | 0 |
| `views/super-admin/organizations.php` | 53 | 38 | 10 | 0 |
| `views/settings/organization.php` | 48 | 24 | 1 | 0 |
| `views/docs/mailing.php` | 45 | 17 | 0 | 0 |
| `views/docs/seo-discoverability.php` | 40 | 16 | 0 | 0 |
| `views/docs/multi-tenancy.php` | 35 | 17 | 0 | 0 |
| `views/docs/security.php` | 35 | 16 | 0 | 0 |
| `views/docs/background-jobs.php` | 34 | 17 | 0 | 0 |
| `views/errors/404.php` | 34 | 32 | 5 | 0 |
| `views/errors/500.php` | 34 | 32 | 5 | 0 |
| `views/billing/success.php` | 31 | 29 | 6 | 0 |
| `views/docs/api-tokens.php` | 31 | 16 | 0 | 0 |
| `views/docs/authentication.php` | 31 | 16 | 0 | 0 |
| `views/docs/billing.php` | 31 | 16 | 0 | 0 |
| `views/docs/theming.php` | 28 | 16 | 0 | 0 |
| `views/settings/activity.php` | 27 | 23 | 9 | 0 |
| `views/super-admin/activity.php` | 27 | 23 | 9 | 0 |
| `views/onboarding/organization.php` | 25 | 23 | 8 | 0 |
| `views/dashboard/index.php` | 22 | 19 | 1 | 0 |
| `views/billing/cancel.php` | 21 | 20 | 4 | 0 |
| `views/docs/index.php` | 18 | 18 | 1 | 0 |
| `views/docs/project-structure.php` | 17 | 16 | 0 | 0 |
| `resources/js/components/modal.js` | 2 | 1 | 2 | 0 |
| `resources/js/components/tabs.js` | 1 | 1 | 0 | 0 |
| **Total** | **1679** | **242** | **168** | **93** |

`resources/js/app.js` and `resources/js/components/theme-toggle.js` contain no Tailwind classes (only Keel CSS classes). `resources/css/components.css` applies 107 Tailwind utilities (58 distinct), plus Keel's own `card` inside `.modal-panel`: `inline-flex` `items-center` `justify-center` `rounded-lg` `px-4` `py-2` `text-sm` `font-semibold` `transition` `px-3` `py-1.5` `text-xs` `px-5` `py-2.5` `text-base` `px-6` `py-3` `text-lg` `px-7` `py-3.5` `text-xl` `rounded-2xl` `border` `p-6` `rounded-full` `px-2` `py-1` `py-0.5` `text-[11px]` `mb-1` `block` `font-medium` `w-full` `min-w-full` `text-left` `border-t` `mt-6` `flex` `justify-between` `gap-2` `rounded` `rounded-xl` `border-dashed` `py-8` `text-center` `mx-auto` `h-10` `w-10` `mt-3` `mt-1` `mt-4` `fixed` `inset-0` `z-[120]` `hidden` `p-4` `max-w-md` `shadow-2xl`.

## 2. Variants in use

| Prefix | Uses |
|---|---:|
| `(no variant)` | 1564 |
| `sm:` | 46 |
| `lg:` | 29 |
| `hover:` | 22 |
| `focus:` | 10 |
| `md:` | 6 |
| `disabled:` | 1 |
| `xl:` | 1 |

Breakpoints in use are Tailwind's defaults (`sm` 640, `md` 768, `lg` 1024, `xl` 1280). `tailwind.config.js` changes none.

## 3. Distinct Tailwind classes, ranked by frequency

| # | Class | Uses | Files |
|---:|---|---:|---:|
| 1 | `text-sm` | 85 | 29 |
| 2 | `font-semibold` | 80 | 22 |
| 3 | `text-[var(--color-text-strong)]` | 76 | 14 |
| 4 | `text-lg` | 53 | 18 |
| 5 | `mb-2` | 48 | 15 |
| 6 | `text-xs` | 44 | 21 |
| 7 | `overflow-x-auto` | 37 | 18 |
| 8 | `text-gray-900` | 37 | 13 |
| 9 | `rounded-xl` | 36 | 19 |
| 10 | `px-4` | 35 | 16 |
| 11 | `flex` | 34 | 16 |
| 12 | `p-4` | 34 | 17 |
| 13 | `text-gray-500` | 33 | 13 |
| 14 | `items-center` | 30 | 16 |
| 15 | `bg-[var(--color-surface-muted)]` | 28 | 12 |
| 16 | `font-medium` | 25 | 11 |
| 17 | `border` | 21 | 5 |
| 18 | `uppercase` | 21 | 7 |
| 19 | `pl-5` | 20 | 13 |
| 20 | `space-y-1` | 20 | 13 |
| 21 | `min-h-screen` | 18 | 16 |
| 22 | `mx-auto` | 18 | 12 |
| 23 | `leading-7` | 17 | 14 |
| 24 | `text-3xl` | 17 | 14 |
| 25 | `gap-4` | 16 | 10 |
| 26 | `hidden` | 16 | 5 |
| 27 | `mt-3` | 16 | 5 |
| 28 | `text-[var(--color-text-muted)]` | 16 | 14 |
| 29 | `text-base` | 16 | 4 |
| 30 | `font-bold` | 15 | 13 |
| 31 | `list-disc` | 15 | 11 |
| 32 | `mt-2` | 15 | 8 |
| 33 | `mt-4` | 15 | 10 |
| 34 | `text-[var(--keel-muted)]` | 15 | 2 |
| 35 | `rounded-full` | 14 | 2 |
| 36 | `space-y-6` | 14 | 14 |
| 37 | `w-full` | 14 | 9 |
| 38 | `bg-gray-50` | 13 | 13 |
| 39 | `grid` | 13 | 9 |
| 40 | `justify-between` | 13 | 10 |
| 41 | `py-3` | 13 | 4 |
| 42 | `flex-col` | 11 | 4 |
| 43 | `gap-3` | 11 | 6 |
| 44 | `text-gray-600` | 11 | 8 |
| 45 | `justify-center` | 10 | 7 |
| 46 | `p-6` | 10 | 7 |
| 47 | `sm:px-6` | 10 | 3 |
| 48 | `lg:px-8` | 9 | 3 |
| 49 | `max-w-7xl` | 9 | 3 |
| 50 | `mb-6` | 9 | 6 |
| 51 | `mb-8` | 9 | 8 |
| 52 | `mt-6` | 9 | 5 |
| 53 | `py-10` | 9 | 9 |
| 54 | `py-2` | 9 | 5 |
| 55 | `tracking-wide` | 9 | 6 |
| 56 | `transition` | 9 | 2 |
| 57 | `hover:text-gray-900` | 8 | 8 |
| 58 | `leading-6` | 8 | 7 |
| 59 | `inline-flex` | 7 | 2 |
| 60 | `rounded-2xl` | 7 | 3 |
| 61 | `sm:flex-row` | 7 | 4 |
| 62 | `space-y-4` | 7 | 6 |
| 63 | `table` | 7 | 5 |
| 64 | `gap-6` | 6 | 6 |
| 65 | `h-10` | 6 | 3 |
| 66 | `leading-8` | 6 | 1 |
| 67 | `mt-1` | 6 | 3 |
| 68 | `mt-8` | 6 | 5 |
| 69 | `object-contain` | 6 | 3 |
| 70 | `px-3` | 6 | 5 |
| 71 | `rounded-3xl` | 6 | 3 |
| 72 | `list-decimal` | 5 | 5 |
| 73 | `max-w-xl` | 5 | 5 |
| 74 | `p-8` | 5 | 5 |
| 75 | `px-5` | 5 | 1 |
| 76 | `text-center` | 5 | 5 |
| 77 | `text-left` | 5 | 4 |
| 78 | `text-xl` | 5 | 3 |
| 79 | `tracking-[0.28em]` | 5 | 1 |
| 80 | `bg-white` | 4 | 2 |
| 81 | `border-gray-200` | 4 | 3 |
| 82 | `gap-2` | 4 | 3 |
| 83 | `hover:border-[var(--keel-cyan)]` | 4 | 1 |
| 84 | `mt-5` | 4 | 3 |
| 85 | `p-0` | 4 | 4 |
| 86 | `px-6` | 4 | 1 |
| 87 | `py-8` | 4 | 1 |
| 88 | `relative` | 4 | 2 |
| 89 | `sm:justify-between` | 4 | 3 |
| 90 | `sm:text-4xl` | 4 | 1 |
| 91 | `text-2xl` | 4 | 3 |
| 92 | `text-amber-900` | 4 | 3 |
| 93 | `tracking-[-0.04em]` | 4 | 1 |
| 94 | `antialiased` | 3 | 3 |
| 95 | `bg-[var(--keel-accent)]` | 3 | 1 |
| 96 | `bg-amber-50` | 3 | 3 |
| 97 | `border-amber-200` | 3 | 3 |
| 98 | `h-3` | 3 | 1 |
| 99 | `max-w-4xl` | 3 | 3 |
| 100 | `max-w-5xl` | 3 | 3 |
| 101 | `md:grid-cols-2` | 3 | 3 |
| 102 | `overflow-hidden` | 3 | 2 |
| 103 | `pb-20` | 3 | 1 |
| 104 | `py-4` | 3 | 1 |
| 105 | `rounded-[2rem]` | 3 | 1 |
| 106 | `sm:items-center` | 3 | 2 |
| 107 | `text-slate-950` | 3 | 1 |
| 108 | `tracking-[0.2em]` | 3 | 1 |
| 109 | `w-3` | 3 | 1 |
| 110 | `-z-10` | 2 | 2 |
| 111 | `absolute` | 2 | 2 |
| 112 | `border-amber-300` | 2 | 1 |
| 113 | `border-b` | 2 | 1 |
| 114 | `border-gray-300` | 2 | 2 |
| 115 | `border-t` | 2 | 1 |
| 116 | `flex-wrap` | 2 | 1 |
| 117 | `font-mono` | 2 | 2 |
| 118 | `gap-5` | 2 | 2 |
| 119 | `hover:bg-[#ff835f]` | 2 | 1 |
| 120 | `hover:bg-gray-100` | 2 | 2 |
| 121 | `inset-0` | 2 | 2 |
| 122 | `isolate` | 2 | 2 |
| 123 | `lg:grid-cols-[minmax(0,1.2fr)_minmax(320px,0.8fr)]` | 2 | 2 |
| 124 | `lg:items-center` | 2 | 1 |
| 125 | `lg:pt-12` | 2 | 2 |
| 126 | `lg:px-10` | 2 | 1 |
| 127 | `max-w-2xl` | 2 | 1 |
| 128 | `max-w-3xl` | 2 | 2 |
| 129 | `max-w-6xl` | 2 | 2 |
| 130 | `mb-3` | 2 | 1 |
| 131 | `mb-4` | 2 | 2 |
| 132 | `md:hidden` | 2 | 1 |
| 133 | `mt-10` | 2 | 1 |
| 134 | `opacity-60` | 2 | 2 |
| 135 | `p-5` | 2 | 2 |
| 136 | `pb-2` | 2 | 1 |
| 137 | `pb-6` | 2 | 2 |
| 138 | `pt-8` | 2 | 2 |
| 139 | `px-1` | 2 | 1 |
| 140 | `py-6` | 2 | 2 |
| 141 | `rounded-[1.75rem]` | 2 | 1 |
| 142 | `sm:items-start` | 2 | 2 |
| 143 | `sm:p-7` | 2 | 2 |
| 144 | `sm:px-8` | 2 | 1 |
| 145 | `sm:py-10` | 2 | 1 |
| 146 | `space-y-2` | 2 | 1 |
| 147 | `space-y-5` | 2 | 2 |
| 148 | `text-gray-700` | 2 | 2 |
| 149 | `text-right` | 2 | 1 |
| 150 | `tracking-[0.24em]` | 2 | 1 |
| 151 | `underline` | 2 | 2 |
| 152 | `w-10` | 2 | 1 |
| 153 | `bg-[#69e6d8]` | 1 | 1 |
| 154 | `bg-[#ff6b3d]` | 1 | 1 |
| 155 | `bg-[#ffd166]` | 1 | 1 |
| 156 | `bg-[var(--color-brand)]` | 1 | 1 |
| 157 | `bg-[var(--keel-accent-soft)]` | 1 | 1 |
| 158 | `bg-red-500/10` | 1 | 1 |
| 159 | `block` | 1 | 1 |
| 160 | `border-[var(--keel-line)]` | 1 | 1 |
| 161 | `border-red-500/40` | 1 | 1 |
| 162 | `disabled:opacity-50` | 1 | 1 |
| 163 | `focus:bg-white` | 1 | 1 |
| 164 | `focus:fixed` | 1 | 1 |
| 165 | `focus:left-4` | 1 | 1 |
| 166 | `focus:not-sr-only` | 1 | 1 |
| 167 | `focus:px-4` | 1 | 1 |
| 168 | `focus:py-2` | 1 | 1 |
| 169 | `focus:rounded-md` | 1 | 1 |
| 170 | `focus:text-slate-950` | 1 | 1 |
| 171 | `focus:top-4` | 1 | 1 |
| 172 | `focus:z-[100]` | 1 | 1 |
| 173 | `gap-10` | 1 | 1 |
| 174 | `gap-12` | 1 | 1 |
| 175 | `grid-cols-2` | 1 | 1 |
| 176 | `h-11` | 1 | 1 |
| 177 | `hover:bg-[var(--color-surface-muted)]` | 1 | 1 |
| 178 | `hover:bg-amber-100` | 1 | 1 |
| 179 | `hover:text-[var(--color-text-strong)]` | 1 | 1 |
| 180 | `hover:text-[var(--keel-cyan)]` | 1 | 1 |
| 181 | `hover:text-[var(--keel-text)]` | 1 | 1 |
| 182 | `hover:underline` | 1 | 1 |
| 183 | `justify-end` | 1 | 1 |
| 184 | `lg:block` | 1 | 1 |
| 185 | `lg:flex-row` | 1 | 1 |
| 186 | `lg:grid-cols-[280px_minmax(0,1fr)]` | 1 | 1 |
| 187 | `lg:grid-cols-[minmax(0,0.8fr)_minmax(0,1.2fr)]` | 1 | 1 |
| 188 | `lg:grid-cols-[minmax(0,1.05fr)_minmax(420px,0.95fr)]` | 1 | 1 |
| 189 | `lg:grid-cols-[minmax(0,1.1fr)_minmax(320px,0.9fr)]` | 1 | 1 |
| 190 | `lg:grid-cols-3` | 1 | 1 |
| 191 | `lg:hidden` | 1 | 1 |
| 192 | `lg:items-start` | 1 | 1 |
| 193 | `lg:justify-between` | 1 | 1 |
| 194 | `lg:pb-28` | 1 | 1 |
| 195 | `lg:text-7xl` | 1 | 1 |
| 196 | `max-w-md` | 1 | 1 |
| 197 | `max-w-sm` | 1 | 1 |
| 198 | `mb-5` | 1 | 1 |
| 199 | `md:flex` | 1 | 1 |
| 200 | `ml-auto` | 1 | 1 |
| 201 | `mt-12` | 1 | 1 |
| 202 | `pb-4` | 1 | 1 |
| 203 | `pt-4` | 1 | 1 |
| 204 | `pt-5` | 1 | 1 |
| 205 | `py-1.5` | 1 | 1 |
| 206 | `py-20` | 1 | 1 |
| 207 | `rounded` | 1 | 1 |
| 208 | `rounded-[1.5rem]` | 1 | 1 |
| 209 | `rounded-lg` | 1 | 1 |
| 210 | `sm:gap-6` | 1 | 1 |
| 211 | `sm:grid-cols-2` | 1 | 1 |
| 212 | `sm:grid-cols-3` | 1 | 1 |
| 213 | `sm:items-end` | 1 | 1 |
| 214 | `sm:justify-center` | 1 | 1 |
| 215 | `sm:min-w-[19rem]` | 1 | 1 |
| 216 | `sm:p-5` | 1 | 1 |
| 217 | `sm:pt-10` | 1 | 1 |
| 218 | `sm:text-6xl` | 1 | 1 |
| 219 | `sm:text-xl` | 1 | 1 |
| 220 | `space-y-1.5` | 1 | 1 |
| 221 | `space-y-3` | 1 | 1 |
| 222 | `sr-only` | 1 | 1 |
| 223 | `sticky` | 1 | 1 |
| 224 | `text-[var(--color-brand-contrast)]` | 1 | 1 |
| 225 | `text-[var(--color-brand)]` | 1 | 1 |
| 226 | `text-[var(--keel-accent)]` | 1 | 1 |
| 227 | `text-[var(--keel-text)]` | 1 | 1 |
| 228 | `text-5xl` | 1 | 1 |
| 229 | `text-amber-600` | 1 | 1 |
| 230 | `text-emerald-300` | 1 | 1 |
| 231 | `text-gray-400` | 1 | 1 |
| 232 | `text-red-200` | 1 | 1 |
| 233 | `text-red-300` | 1 | 1 |
| 234 | `top-6` | 1 | 1 |
| 235 | `tracking-[-0.03em]` | 1 | 1 |
| 236 | `tracking-[-0.05em]` | 1 | 1 |
| 237 | `tracking-[0.22em]` | 1 | 1 |
| 238 | `tracking-[0.3em]` | 1 | 1 |
| 239 | `tracking-tight` | 1 | 1 |
| 240 | `tracking-widest` | 1 | 1 |
| 241 | `w-11` | 1 | 1 |
| 242 | `xl:grid-cols-3` | 1 | 1 |

## 4. Arbitrary values

| Class | Uses | Files |
|---|---:|---|
| `text-[var(--color-text-strong)]` | 76 | docs/api-tokens.php, docs/authentication.php, docs/background-jobs.php, docs/billing.php, docs/index.php, docs/installation.php, docs/mailing.php, docs/multi-tenancy.php, docs/project-structure.php, docs/security.php, docs/seo-discoverability.php, docs/theming.php, docs/where-to-start-editing.php, docs/_layout.php |
| `bg-[var(--color-surface-muted)]` | 28 | docs/api-tokens.php, docs/authentication.php, docs/background-jobs.php, docs/billing.php, docs/installation.php, docs/mailing.php, docs/multi-tenancy.php, docs/project-structure.php, docs/security.php, docs/seo-discoverability.php, docs/theming.php, docs/where-to-start-editing.php |
| `text-[var(--color-text-muted)]` | 16 | docs/api-tokens.php, docs/authentication.php, docs/background-jobs.php, docs/billing.php, docs/index.php, docs/installation.php, docs/mailing.php, docs/multi-tenancy.php, docs/project-structure.php, docs/security.php, docs/seo-discoverability.php, docs/theming.php, docs/where-to-start-editing.php, docs/_layout.php |
| `text-[var(--keel-muted)]` | 15 | auth/login.php, welcome.php |
| `tracking-[0.28em]` | 5 | welcome.php |
| `hover:border-[var(--keel-cyan)]` | 4 | welcome.php |
| `tracking-[-0.04em]` | 4 | welcome.php |
| `bg-[var(--keel-accent)]` | 3 | welcome.php |
| `tracking-[0.2em]` | 3 | welcome.php |
| `rounded-[2rem]` | 3 | welcome.php |
| `lg:grid-cols-[minmax(0,1.2fr)_minmax(320px,0.8fr)]` | 2 | settings/api-tokens.php, settings/members.php |
| `hover:bg-[#ff835f]` | 2 | welcome.php |
| `tracking-[0.24em]` | 2 | welcome.php |
| `rounded-[1.75rem]` | 2 | welcome.php |
| `tracking-[-0.03em]` | 1 | auth/login.php |
| `text-[var(--keel-text)]` | 1 | auth/login.php |
| `text-[var(--color-brand)]` | 1 | docs/index.php |
| `lg:grid-cols-[280px_minmax(0,1fr)]` | 1 | docs/_layout.php |
| `bg-[var(--color-brand)]` | 1 | docs/_layout.php |
| `text-[var(--color-brand-contrast)]` | 1 | docs/_layout.php |
| `hover:bg-[var(--color-surface-muted)]` | 1 | docs/_layout.php |
| `hover:text-[var(--color-text-strong)]` | 1 | docs/_layout.php |
| `lg:grid-cols-[minmax(0,1.1fr)_minmax(320px,0.9fr)]` | 1 | super-admin/organizations.php |
| `focus:z-[100]` | 1 | welcome.php |
| `tracking-[0.3em]` | 1 | welcome.php |
| `lg:grid-cols-[minmax(0,1.05fr)_minmax(420px,0.95fr)]` | 1 | welcome.php |
| `border-[var(--keel-line)]` | 1 | welcome.php |
| `tracking-[-0.05em]` | 1 | welcome.php |
| `hover:text-[var(--keel-cyan)]` | 1 | welcome.php |
| `bg-[#ff6b3d]` | 1 | welcome.php |
| `bg-[#ffd166]` | 1 | welcome.php |
| `bg-[#69e6d8]` | 1 | welcome.php |
| `hover:text-[var(--keel-text)]` | 1 | welcome.php |
| `rounded-[1.5rem]` | 1 | welcome.php |
| `bg-[var(--keel-accent-soft)]` | 1 | welcome.php |
| `text-[var(--keel-accent)]` | 1 | welcome.php |
| `lg:grid-cols-[minmax(0,0.8fr)_minmax(0,1.2fr)]` | 1 | welcome.php |
| `tracking-[0.22em]` | 1 | welcome.php |
| `sm:min-w-[19rem]` | 1 | welcome.php |

## 5. Raw palette classes

These bypass Keel's own design tokens (`--color-*`), which is why the app views ignore `data-theme`: `bg-gray-50` and `text-gray-900` do not change in dark mode while `.card` does.

| Class | Uses | Files |
|---|---:|---|
| `text-gray-900` | 37 | billing/cancel.php, billing/plans.php, billing/success.php, dashboard/index.php, errors/404.php, errors/500.php, onboarding/organization.php, settings/activity.php, settings/api-tokens.php, settings/members.php, settings/organization.php, super-admin/activity.php, super-admin/organizations.php |
| `text-gray-500` | 33 | billing/cancel.php, billing/plans.php, billing/success.php, dashboard/index.php, errors/404.php, errors/500.php, onboarding/organization.php, settings/activity.php, settings/api-tokens.php, settings/members.php, settings/organization.php, super-admin/activity.php, super-admin/organizations.php |
| `bg-gray-50` | 13 | billing/cancel.php, billing/plans.php, billing/success.php, dashboard/index.php, errors/404.php, errors/500.php, onboarding/organization.php, settings/activity.php, settings/api-tokens.php, settings/members.php, settings/organization.php, super-admin/activity.php, super-admin/organizations.php |
| `text-gray-600` | 11 | billing/cancel.php, billing/plans.php, billing/success.php, errors/404.php, errors/500.php, onboarding/organization.php, settings/api-tokens.php, super-admin/organizations.php |
| `hover:text-gray-900` | 8 | billing/plans.php, dashboard/index.php, settings/activity.php, settings/api-tokens.php, settings/members.php, settings/organization.php, super-admin/activity.php, super-admin/organizations.php |
| `bg-white` | 4 | billing/success.php, settings/api-tokens.php |
| `text-amber-900` | 4 | errors/404.php, errors/500.php, settings/api-tokens.php |
| `border-gray-200` | 4 | settings/api-tokens.php, settings/members.php, super-admin/organizations.php |
| `border-amber-200` | 3 | errors/404.php, errors/500.php, settings/api-tokens.php |
| `bg-amber-50` | 3 | errors/404.php, errors/500.php, settings/api-tokens.php |
| `text-slate-950` | 3 | welcome.php |
| `border-gray-300` | 2 | billing/success.php, settings/api-tokens.php |
| `text-gray-700` | 2 | billing/success.php, settings/api-tokens.php |
| `hover:bg-gray-100` | 2 | billing/success.php, settings/api-tokens.php |
| `border-amber-300` | 2 | settings/api-tokens.php |
| `border-red-500/40` | 1 | auth/login.php |
| `bg-red-500/10` | 1 | auth/login.php |
| `text-red-200` | 1 | auth/login.php |
| `text-emerald-300` | 1 | auth/login.php |
| `text-red-300` | 1 | auth/login.php |
| `text-gray-400` | 1 | billing/plans.php |
| `text-amber-600` | 1 | billing/plans.php |
| `hover:bg-amber-100` | 1 | settings/api-tokens.php |
| `focus:bg-white` | 1 | welcome.php |
| `focus:text-slate-950` | 1 | welcome.php |

## 6. What Vite builds

One entry, `resources/js/app.js` (`vite.config.js` → `build.rollupOptions.input`). A production build emits exactly three files into `public_html/assets/`, which it empties first (`emptyOutDir: true`):

| Output | Size | Contents |
|---|---:|---|
| `assets/app-[hash].css` | 27.9 KB (6.1 KB gzip) | Tailwind base/components/utilities, `components.css` (Keel's `@apply` components), `app.css` (Keel's `--color-*` tokens, light overrides, logo swap, theme toggle, back-to-top) |
| `assets/app-[hash].js` | 5.5 KB (2.1 KB gzip) | Vanilla ES modules, no npm runtime dependencies: `theme-toggle.js` (toggle + `POST /settings/theme`), `tabs.js` (login page), `modal.js` (API token revoke), copy buttons (`data-copy-source`, API tokens), back-to-top (every page) |
| `.vite/manifest.json` | 0.2 KB | Read by `src/Core/Vite.php` |

No images, fonts or other assets go through Vite: brand images are static files in `public_html/images/`. In development `npm run dev` writes `public_html/hot` and `Vite::assets()` points the page at the dev server for HMR.

Not bundled by Vite: inline `<script>` blocks in `views/auth/login.php` (OTP and magic link requests), `views/welcome.php` (mobile menu, copy button) and `views/docs/_layout.php` (sidebar toggle), and the theme bootstrap in `views/partials/head.php`.

**Everything that depends on Vite:** `src/Core/Vite.php`; `views/partials/head.php`; `vite.config.js`, `postcss.config.js`, `tailwind.config.js`, `package.json`/`package-lock.json`; the `Build frontend assets` step in `.github/workflows/ci.yml`; `/public_html/hot` and `/public_html/assets/*` in `.gitignore`; README (setup step 9, stack list, directory tree), CONTRIBUTING (step 5), `views/docs/project-structure.php` and `views/docs/installation.php`; and marketing copy that names "Tailwind + Vite" in `views/welcome.php` (×2) and `views/billing/plans.php`. PHPUnit does not depend on it (no test asserts on asset tags).

One pre-existing defect is tied to it: `ManifestController` advertises `/assets/brand/icon-192.png`, `icon-512.png` and `icon-maskable-512.png`. The icons live in `resources/images/brand/`, nothing copies them, and `emptyOutDir` would delete them from `public_html/assets/` on every build anyway, so all three return 404 today.

## 7. `tailwind.config.js` customisations

- **Plugins:** none.
- **Fonts, spacing, breakpoints, shadows:** none customised; Tailwind defaults.
- **`darkMode`:** not set (Tailwind 3 default `media`), and irrelevant: no `dark:` class exists.
- **Colours** — `theme.extend.colors`, every one a CSS variable from `resources/css/app.css`: `brand` (DEFAULT, hover, contrast), `surface` (DEFAULT, hover, muted), `text` (DEFAULT, muted, strong), `danger` (DEFAULT, hover, soft, border, text), `success` (DEFAULT, soft, border, text), `card` (DEFAULT, border), `input.border`, `table` (head, row-alt, row-hover), `empty` (bg, border, icon, icon-text), `modal.backdrop`. **None of these named colours is used in any view** — no `bg-brand`, no `text-muted`. Views reach the tokens through Keel's `.btn`/`.card`/… classes, through arbitrary values such as `text-[var(--color-text-muted)]`, or bypass them with raw palette classes (section 5).
- **Border radius:** `md`, `lg`, `xl` remapped to `--radius-md` (0.5rem), `--radius-lg` (0.75rem), `--radius-xl` (1rem) — the same values as Tailwind's defaults.
- **The brand colour:** `--color-brand: #ff6b3d` (hover `#ff835f`, text on it `#08101e`), identical in both themes. In OKLCH that is `oklch(70.5% 0.191 37.5)`, so the closest Deck `--hue-brand` is **38**. `views/auth/login.php`, `views/welcome.php` and `views/docs/_layout.php` repeat `#ff6b3d`/`#ff835f` as literals rather than using the token.

## 8. Dark mode

Not Tailwind's mechanism. Keel swaps CSS custom properties on `data-theme` on `<html>`:

- `:root` in `resources/css/app.css` holds the **dark** values; `[data-theme="light"]` overrides them. Views with their own `<style>` repeat the pattern (`welcome.php`, `auth/login.php`, `docs/_layout.php`).
- `Theme::htmlAttribute()` writes the signed-in user's `users.theme_preference` (migration 009, refreshed into the session by `AuthMiddleware`) onto `<html>` server-side.
- An inline script in `views/partials/head.php` resolves the theme before paint: server preference, else `localStorage['keel-theme']` (guests only), else `prefers-color-scheme`, else dark.
- `theme-toggle.js` flips the attribute, stores `keel-theme`, and for signed-in users `POST`s `/settings/theme`. Pages with no toggle get a floating one injected.

The token swap only reaches what uses the tokens. The app views (dashboard, settings, billing, super-admin, onboarding, errors) set `bg-gray-50` and `text-gray-900` directly, so in dark mode their body stays light, headings inside dark cards turn near-invisible, and inputs stay white — see `screenshots/before/members-1280-dark.png`.

## 9. Per-tenant branding

None exists. `organizations` has `id`, `name`, `slug` and timestamps; no colour, hue or logo column. The brand colour is one hard-coded value in `resources/css/app.css` (plus literals in three views) and changing it means a rebuild. Phase 1 adds `organizations.brand_hue` (migration 010, nullable).
