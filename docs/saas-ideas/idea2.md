# Idea 2: FeedTriage — catalog health scoring + AI feed fixes for Google Shopping, for agencies and DTC stores

**Lane:** E-commerce / DTC operators
**Verdict:** AMBER — the pain and the willingness to pay are proven and the MVP is genuinely 2 days, but the market is crowded and the moat is distribution and UX, not technology.

> **Evidence caveat, read first.** In this environment Reddit is fully blocked to my crawler (both fetch and search reject `reddit.com`), as are `apps.shopify.com`, `community.shopify.com`, G2, Capterra and Trustpilot page fetches. Everything below is sourced from web search results with real URLs. Where I could not open the page to confirm wording, I cite the **thread title verbatim** rather than invent body text, and mark inferred facts `UNVERIFIED:`. No quote here is fabricated.

## The Pain
Google Shopping and free listings are a top-3 revenue channel for most DTC stores, and a product that fails Merchant Center's data checks earns exactly zero impressions — silently. Disapprovals hit continuously: missing/invalid GTIN, missing brand or MPN, weak titles, wrong `google_product_category`, price/availability mismatch, image quality, and account-level "misrepresentation". The store owner sees a red number in Merchant Center and a policy-doc link, not a per-SKU fix list, and Google support is famously hard to reach. Agencies running Google Ads for 5-30 e-com clients hit this every single week and currently solve it with manual spreadsheet audits.

### Reddit evidence
Reddit is inaccessible to this session. Substituted with **Shopify Community** threads — same buyer, publicly verifiable URLs. Quotes are verbatim thread titles; dates are as reported by search and are `UNVERIFIED:` to the day.

- "Google Shopping Feed Products Not Showing or Getting Disapproved on Shopify" — Shopify Community, ~Mar 2026, https://community.shopify.com/t/google-shopping-feed-products-not-showing-or-getting-disapproved-on-shopify/599865
- "Repeated "Misrepresentation" rejection in Google Merchant Center – what am I missing?" — Shopify Community, ~Feb 2026, https://community.shopify.com/t/repeated-misrepresentation-rejection-in-google-merchant-center-what-am-i-missing/586038
- "Why does my Google Shopping feed keep getting disapproved? Here is what fixed it for me" — Shopify Community, recent, https://community.shopify.com/t/why-does-my-google-shopping-feed-keep-getting-disapproved-here-is-what-fixed-it-for-me/660659
- "Drop in Number of Active Items" — Shopify Community, recent; merchant reports recurring GMC active-item drops "for unclear reasons" with support hard to reach (search summary), https://community.shopify.com/t/drop-in-number-of-active-items/566100
- "Google Merchant Center - misrepresentation" — Shopify Community, ~Aug 2023 (older, flagged); handmade-sticker seller disapproved over missing MPN/GTIN, https://community.shopify.com/c/technical-q-a/google-merchant-center-misrepresentation/td-p/2156434

Market-side corroboration: Productsup, Channable, KlientBoost, HawkSEM and DataFeedWatch all publish 2025/2026 "fix your Merchant Center disapprovals" explainers — vendors only write that volume of content against high-frequency, high-search-volume pain.

## The Product
Point FeedTriage at a store — a `myshopify` domain (public `/products.json`, no auth), a product feed URL, or a CSV/XML upload. It parses every SKU, scores the catalog against Google's product data spec and the known disapproval reasons, and returns a ranked, per-SKU issue list in plain English with the exact fix. An LLM then rewrites the weak fields it is allowed to rewrite — title (Brand + Type + key attribute + size/colour), `product_type`, `google_product_category`, description — and FeedTriage publishes those as a **hosted supplemental feed URL** that the merchant registers in Merchant Center via scheduled fetch. No Google OAuth, no Shopify app install, no re-platforming: it sits on top of whatever feed app the store already runs.

## Why This Makes Money
- **Buyer:** primary = Google Ads / PMax agency or freelancer managing 5-30 e-com clients (owner or head of paid signs, tool budget is theirs). Secondary = in-house e-com/growth lead at a $1M-$20M DTC brand.
- **Pricing:** Free audit (1 store, top 25 issues, no feed output) as the lead magnet → **Solo $49/mo** (1 store, 10k SKUs, supplemental feed) → **Store $99/mo** (3 stores, 50k SKUs, weekly re-scan + email alerts) → **Agency $249/mo** (25 stores, white-label PDF audit). The agency tier is the real business; $49 exists to convert inbound audits.
- **Willingness to pay proof:** GoDataFeed LITE $39 / PLUS $99 / PRO $199 per month (godatafeed.com/pricing, via search); DataFeedWatch approx. $64 / $84 / $239 (Capterra/GetApp, via search; direct fetch blocked); Channable from approx. $49-74 Core tiers; Shoptimised from **£119.99/mo** (shoptimised.com/pricing, via search); Feedonomics and Productsup are enterprise-priced. Simprosys ($4.99-$17.99/mo) holds **4.9 stars across ~6,600 Shopify App Store reviews** — proof tens of thousands of merchants already pay *something* monthly for feed tooling.
- **Path to first 10 customers:** (1) Generate free audits for 200 DTC Shopify stores from public `/products.json` and email each owner the three biggest fixes with real SKU counts — a cold email containing "412 of your 1,180 products have no GTIN" is not spam, it is a demo. (2) LinkedIn outreach to Google Ads / PMax freelancers offering a free white-label audit for one client. (3) PPC communities (r/PPC, PPCChat, Google Ads Discords, MeasureSlack) with the free audit tool. (4) Programmatic SEO: one page per Merchant Center error string ("Missing value [gtin]", "Invalid value [gtin]", "Image too small", "Mismatched value (price)") with an inline checker — high-intent, high-volume, zero ad spend. Note Shoptimised already runs a free-feed-audit lead magnet, so this channel is validated but contested.

## Competition & The Wedge
| Competitor | Pricing | Size signal | Top complaint |
|---|---|---|---|
| DataFeedWatch (Cart.com) | ~$64 / $84 / $239 mo | Category leader, 2,000+ channels, acquired by Cart.com | "can be quite expensive… a limitation for small businesses"; UI "complex", "confusing for new users"; support ghosting reports (G2/Capterra/Trustpilot via search) |
| GoDataFeed | $39 / $99 / $199 mo + $5/1k extra SKUs, $29-99 per extra channel | Established US player | Metered add-ons stack fast; channel-based pricing punishes multi-channel stores |
| Channable | from ~$49-74/mo Core tiers, +$35 add-ons | VC-backed NL platform, marketplaces + campaign automation | Priced per item × project × channel — cost scales unpredictably with catalog |
| Shoptimised | from £119.99/mo | UK agency-focused, sells performance-based pricing | Entry price locks out sub-£1M merchants |
| Simprosys Google Shopping Feed | $4.99-$17.99/mo | ~6,600 reviews, 4.9★, Shopify App Store | "functionality is good but the support is terrible when things are not working" (search-surfaced review text) |
| Feedonomics (BigCommerce) | Enterprise / quote | Acquired by BigCommerce — `UNVERIFIED:` reported ~$100M, 2021 | Enterprise-only; no self-serve |
| Feedops / Feedon.ai / MagicFeed | Various, mostly <$100 | Small recent AI-feed entrants | Newness; thin track record |

**The wedge:** three specific gaps. (1) **Google will not close it for you** — Merchant Center's automatic improvements update price, availability, condition and images, but *explicitly do not update titles, descriptions, GTINs, custom labels or other feed attributes* (support.google.com/merchants/answer/15623993, via search), which is exactly the layer FeedTriage owns. (2) **Incumbents sell feed *plumbing*, priced per channel and per SKU, and assume you already know what to fix** — DataFeedWatch's own users call the UI "complex" and "confusing for new users"; nobody sells a cheap, opinionated *diagnosis* with a per-SKU fix list. (3) **Nobody serves the agency multi-store audit workflow self-serve** — feed audits today are a manual consulting deliverable (FeedArmy sells them as a service; Shoptimised gives one away to book a call), not a $249/mo product. FeedTriage is deliberately *complementary*: it layers on top of Simprosys or Shopify's free Google & YouTube channel rather than asking anyone to rip out their feed app.

## The 2-Day MVP
**Core loop:** paste a store domain or feed URL → get a scored, per-SKU issue list → accept AI-rewritten titles/types/categories → copy a hosted supplemental feed URL into Merchant Center.

**Screens:** (1) New scan — domain / feed URL / file upload. (2) Scan report — score, issue breakdown by severity, affected-SKU counts, filterable product table. (3) Product detail — original vs AI-rewritten field, accept/reject. (4) Bulk fix run — pick issue type + template, run across N SKUs. (5) Supplemental feed — the public token URL, download CSV, and copy-paste GMC setup steps. (6) Plans/billing.

**Data model:**
- `users` (id, email, password, plan, stripe_customer_id)
- `stores` (id, user_id, name, domain, source_type[shopify_json|feed_url|upload], feed_url, last_scanned_at, score)
- `scans` (id, store_id, started_at, finished_at, product_count, issue_count, score)
- `products` (id, store_id, offer_id, title, description, brand, gtin, mpn, product_type, google_product_category, price, currency, availability, image_url, link, condition, raw_json)
- `issues` (id, store_id, product_id, code, severity, attribute, message, fix_hint)
- `fixes` (id, product_id, attribute, old_value, new_value, source[rule|llm], status[draft|published], tokens_used)
- `supplemental_feeds` (id, store_id, public_token, format[csv|xml], last_fetched_at, fetch_count)

**Integrations:** Shopify public `/products.json` (no auth, no app review — 250/page pagination; some stores disable it, and Shopify returns 430 on rate limit, so throttle). Generic ingest: Google Shopping RSS 2.0 XML, CSV/TSV, published Google Sheets CSV. Anthropic or OpenAI API for rewrites. Stripe Checkout. Output: public token URL consumed by **Google Merchant Center supplemental feed via scheduled fetch** — verified to support overriding `title` and other attributes, keyed on `id`, fetchable daily, with **no OAuth and no approval gate**. Deliberately avoided: Google Content API for Shopping (the `content` scope needs OAuth verification — weeks), Shopify Admin API + App Store listing (review queue + revenue cut), Google Ads API.

**Explicitly NOT in v1:** scheduled monitoring and alert emails; multi-user/team accounts; white-label PDF export; image analysis or image hosting; any non-Google channel (Meta, Bing, TikTok, marketplaces); visual rule builder; A/B title testing; Google Ads performance data; multi-country/multi-currency feeds; write-back to Shopify. **Honest minimum:** scan → issues → LLM fixes → supplemental feed → Stripe is 2 days. Weekly re-scan + alerts and the agency multi-store dashboard are day 3-4, and the agency tier does not really sell without them.

## Risks & Kill Criteria
- **GoDataFeed already advertises "AI titles & descriptions, error monitoring" at $39/mo** → if more than a third of the first 20 demo calls say "my feed tool already does this", the wedge is dead; reposition to pure multi-store agency auditing or kill.
- **Simprosys anchors Shopify merchants at $4.99-$17.99/mo** → a solo merchant may refuse $49. If merchant trial→paid is under 10% while agency trials convert, drop the Solo tier and sell only to agencies.
- **Google extends automatic improvements to titles/descriptions** → today they explicitly do not. Watch Merchant Center release notes; if titles become auto-optimised by default, the core value evaporates.
- **Supplemental feeds cannot fix policy, image or misrepresentation issues** — only data attributes. If the first 50 scans show most real disapprovals are policy/account-level, the product diagnoses but cannot fix, and churn follows. Measure issue mix in week 1.
- **Cold-outreach channel fails** → if 200 personalised audit emails yield under a 2% reply rate, the no-ad-budget thesis breaks; fall back to programmatic SEO on error strings, which is slower (3-6 months) than this founder may want.

## QA Notes
**Verified:** competitor existence and approximate pricing for GoDataFeed, DataFeedWatch, Channable, Shoptimised, Simprosys (all via search results with real URLs; direct page fetches were blocked by the network proxy, so treat exact figures as ±10%). Verified as real and technically load-bearing: Shopify's public `/products.json` endpoint still works in 2026; Merchant Center supplemental feeds support scheduled URL fetch and can override `title`; Google's automatic improvements explicitly exclude titles, descriptions and GTINs. Real thread URLs on community.shopify.com confirm recurring 2026 disapproval pain.

**Assumed / unverified:** every date on the community threads (search-reported, not confirmed on-page); Feedonomics' acquisition price; that agencies will pay $249/mo self-serve for auditing (no direct evidence — the closest proof is that agencies sell audits as a manual service, which cuts both ways: they may prefer to keep billing for the labour).

**Weakest link:** the moat. Nothing here is hard to copy, an incumbent could ship the diagnostic view in a sprint, and the real differentiator is a cold-outreach motion (free audit from public data) that a competitor can also run. This is a distribution bet, not a technology bet.

**Confidence:** medium. GREEN on "does the pain exist and do people pay for adjacent tools" — that is well evidenced. AMBER on "can a solo dev take enough of it", because the $5-20 Shopify app tier and the $39-99 AI-feed tier squeeze the price band from both sides. Realistic ceiling: $3-8k MRR from 30-60 accounts within 12 months, weighted to agencies. Not a venture outcome; a plausible solo-founder income.
