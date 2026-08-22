# Idea 4: ClickGap — tells you *why* each page lost clicks, and writes the client the memo

**Lane:** Content, SEO & marketing ops
**Verdict:** AMBER — real recurring pain, proven willingness to pay, trivial COGS, but the category is well-served at exactly this price point and Semrush already ships the adjacent feature.

> ⚠️ **RESEARCH CONSTRAINT — READ FIRST.** This session's network blocks Reddit entirely (crawler-level *and* proxy-level), plus G2, Capterra, TrustPilot, ProductHunt, IndieHackers and HN. `WebFetch` was egress-blocked on every domain tried. **No Reddit thread could be opened, so this one-pager contains ZERO Reddit quotes rather than fabricated ones.** Every claim below is sourced to a real URL returned by web search, but marked `UNVERIFIED-FETCH` because the page itself could not be opened — wording is paraphrase from the search index, not verbatim. Treat all pricing as needing a 10-minute manual re-check before acting.

## The Pain
Since AI Overviews scaled, SEOs and content teams face "the Great Decoupling": impressions flat or rising while clicks collapse, so the old signal (rank) no longer predicts the outcome (traffic). The person who hurts is the freelance SEO consultant or small-agency owner who must, every single month, open Google Search Console for 5–30 client sites, work out *which* losses are AI-Overview cannibalization vs. genuine rank loss vs. falling demand, decide refresh/consolidate/delete for each page, and then explain it all to a client who just sees a red arrow. Detection is a solved commodity; the diagnosis and the client conversation are the unpaid labor. It recurs monthly, forever, and scales linearly with client count.

### Reddit evidence
**None available — Reddit was inaccessible from this environment. I will not invent quotes.** Substituted non-Reddit sourcing, all `UNVERIFIED-FETCH` paraphrase:

- Seer Interactive data: organic CTR fell ~61% (1.76% → 0.61%) on queries where an AI Overview appears — https://www.searchenginejournal.com/ai-overview-ctr-fell-61-but-clicks-didnt-collapse/572993/ (2026)
- Documented case: impressions +27.56% YoY while clicks −36.18% and average position *improved* 14% — the decoupling in one line — https://gobig.systems/impressions-up-clicks-down-ai-overviews/ (2026)
- "The hard part in 2026 is not detecting decay; it is deciding what to do about it, because the wrong action wastes budget and can worsen rankings." — https://www.digitalapplied.com/blog/content-refresh-prioritization-2026-seo-decision-matrix (2026)
- Agency reporting load: "Three hours per client per month across ten clients is thirty hours, close to a full working week, spent moving numbers around." — https://sourcetodocs.com/blog/client-reporting-tools-comparison/ (2026)
- Google Search Console gives no native way to isolate AI Overview performance — it is aggregated into web search — https://abmagency.com/extracting-ai-overview-impressions-from-google-search-console-a-comprehensive-guide/ (2026)

## The Product
Upload two Google Search Console CSV exports (this period, comparison period). ClickGap joins them per URL and per query and classifies every loss deterministically in PHP — no LLM in the math: *AIO cannibalization* (impressions flat/up, position flat, clicks down), *rank loss* (position down), *demand loss* (impressions down, position flat), *cannibalization* (2+ URLs on one query). It then ranks pages by recoverable-clicks, assigns an action (refresh / consolidate / restructure-for-citation / leave alone), and — the actual product — generates a plain-English, client-ready monthly memo explaining what happened and what you're doing about it. Core loop: **two CSVs in → diagnosis table + prioritized action queue + copy-paste client memo out.**

## Why This Makes Money
- **Buyer:** Freelance SEO consultant or owner of a 2–15 person agency managing 5–30 client sites. They sign their own cheques. Secondary: in-house content lead at a 20–200 person B2B company.
- **Pricing:** $29/mo solo (1 site, 4 reports/mo) · $99/mo consultant (10 sites) · $199/mo agency (30 sites + editable memo templates). Anchored just under SEOTesting's $125 team / $375 agency tiers.
- **Unit economics:** The math is deterministic and free. LLM is used only for the memo + rationale on the top ~25 pages: ~15K input / 3K output per report. At Claude Opus 5 ($5/$25 per MTok) that's **~$0.15/report**; on Haiku 4.5 ($1/$5) **~$0.03**. An agency running 30 sites monthly = **~$4.50/mo COGS against $199 → ~97% gross margin.** Even weekly runs stay above 90%. This is the deliberate structural advantage over the AI-visibility lane, where 50 prompts × 4 models × daily = ~6,000 LLM calls per customer per month and forces $300/mo pricing.
- **Willingness to pay proof:** SEOGets $29/mo, SEOTesting $50/$125/$375, RobotSpeed $99/mo, DashThis ~$44/mo, AgencyAnalytics from ~$59/mo — all selling GSC-derived reporting to this exact buyer today. (all `UNVERIFIED-FETCH`)
- **Path to first 10 customers:** (1) Hand-run the diagnosis manually on 20 prospects' public sites and send the memo free — the deliverable *is* the pitch. (2) SEO Slack communities: The SEO Community (~4,800 members, free), BigSEO Slack, Traffic Think Tank (paid, now Semrush-owned). (3) LinkedIn DMs to freelance SEO consultants — a densely listed, easily filtered population. (4) Publish the classification methodology openly; the method is the marketing.

## Competition & The Wedge

| Competitor | Pricing | Size signal | Top complaint |
|---|---|---|---|
| SEOGets | $29/mo Pro | Established indie GSC tool | No public API; dashboard-only, stops at "here's the decay" (`UNVERIFIED-FETCH`) |
| SEOTesting | $50 / $125 (5 sites) / $375 (20 sites) | Capterra + SoftwareAdvice listed | User wished for "much higher limits" on common functions (`UNVERIFIED-FETCH`) |
| RobotSpeed | $99/mo ($990/yr) | Shopify App Store listing | Auto-publishes AI refreshes — trust/quality risk, not a diagnosis tool |
| Semrush Content Audit | Bundled in Semrush (price `UNVERIFIED`) | Public company, category leader | **Already recommends update / consolidate / remove — the closest thing to this wedge** |
| AgencyAnalytics | from ~$59/mo annual | Large agency install base | "Every change to a report layout has to be made one dashboard at a time... for agencies managing 20+ clients that adds up" (`UNVERIFIED-FETCH`) |
| DashThis | ~$44/mo annual | Established reporting vendor | Presentation layer only — no diagnosis, no recommendation |
| HubSpot AEO | $50/mo ($45 annual), 25 prompts | HubSpot; acquired XFunnel 2025 | Proof the incumbent-eats-the-category risk is live in this lane |

**The wedge — and it is narrow.** Every competitor sells either *data* (SEOGets, SEOTesting, DashThis) or a *pre-AI-era decision framework* (Semrush Content Audit, which recommends update/consolidate/remove but was built when a click loss meant a rank loss). Nobody separates AI-Overview cannibalization from rank loss per page and then writes the client-facing explanation — which is the specific hour agencies are burning. Honestly: **this is a crowded lane with a competent incumbent one product-decision away from closing the gap.** It is not an empty market (good — willingness to pay is proven) but it is also not an open one. The defensible part is speed, price, and the memo; the diagnosis logic is a weekend of arithmetic anyone can copy.

## The 2-Day MVP
**Core loop:** Upload two GSC CSVs → per-URL diagnosis + prioritized action queue → generated client memo.
**Screens:** (1) Sites list · (2) New report — upload 2 CSVs + column mapping · (3) Report view — diagnosis table, filter by diagnosis type, sorted by recoverable clicks · (4) Client memo — editable, copy/print · (5) Account (Stripe Payment Link, not in-app billing).
**Data model:**
- `users` (id, email, password, plan, sites_limit)
- `sites` (id, user_id, domain, client_name)
- `reports` (id, site_id, period_a_start/end, period_b_start/end, status, memo_md, totals_json, created_at)
- `report_rows` (id, report_id, url, top_query, clicks_a/b, impressions_a/b, position_a/b, ctr_a/b, diagnosis enum, action enum, recoverable_clicks, priority_score, rationale)

**Integrations:** Anthropic PHP SDK only (`claude-opus-5` for the memo, `claude-haiku-4-5` for per-row rationale) — ~$0.15/report. **No OAuth at v1, deliberately:** the GSC scope is a Google *sensitive scope* requiring verification with a demo video, published privacy policy and domain verification, and apps left in Testing status get Search-Console refresh tokens that **expire after 7 days** — an instant 2-day-MVP killer. CSV is not a compromise here, it is the correct v1.
**Explicitly NOT in v1:** GSC OAuth · scheduled/automatic monitoring · SERP API or rank tracking · any AI-visibility/prompt tracking · GA4 · white-label PDF · team seats · multi-user · internal-link suggestions · auto-publishing refreshed content · API.

## Risks & Kill Criteria
- **Google ships it natively.** GSC has already begun surfacing AI Mode traffic data. If Google exposes per-page AI-Overview attribution, the core diagnosis becomes a free platform feature. → Watch the Search Console changelog; this single event kills the product.
- **Incumbent closes the gap.** Semrush Content Audit already does update/consolidate/delete; SEOGets sits at $29 with decay views. → If either ships decoupling-aware diagnosis, the wedge is gone. Check changelogs monthly.
- **The Lorelight failure mode: insight that doesn't change behavior.** Lorelight shut down 31 Oct 2025 with working tech and paying signups because customers "signed up, explored the data, got excited and then churned." → Measure month-2 return rate. Below 50% and this is a report, not a tool. Kill.
- **CSV friction kills activation.** → If under 40% of signups complete an upload in week 1, OAuth becomes mandatory, which adds Google review and blows the 2-day premise entirely.
- **Cold-start distribution.** No audience, no list, no ad budget, in a lane where every competitor has years of content. → If 30 days of manual free audits + Slack + LinkedIn yields fewer than 5 trials, the binding constraint is distribution, not product — and no amount of building fixes it.

## QA Notes
**Verified:** Anthropic model pricing and IDs (from the loaded `claude-api` skill — Opus 5 $5/$25, Haiku 4.5 $1/$5 per MTok) — the COGS model is the strongest part of this thesis and survives easily; the deterministic-math-plus-thin-LLM-layer design genuinely avoids the margin trap that forces AI-visibility tools to $300/mo.
**Assumed / unverified:** *All* competitor pricing (search-index paraphrase, no page fetched). *All* complaint quotes (same). The claim that no competitor does AIO-vs-rank-loss diagnosis — I could not audit Semrush's current Content Audit feature set, and that is the single load-bearing assumption of the wedge.
**Weakest link:** the wedge is one feature wide, in a category with a $29 indie incumbent *and* a public-company incumbent already shipping the adjacent decision layer. This is a competent small business, not a breakout — plausibly $2–5K MRR, hard to see $50K.
**Also note:** the master brief's warning was correct and confirmed — AI-visibility tracking is a graveyard (Lorelight dead, XFunnel absorbed by HubSpot now selling at $50/mo, Scrunch absorbed by Sitecore). This idea deliberately avoids that lane.
**Confidence: LOW-MEDIUM.** The pain and the buyer are real and well-evidenced. The competitive gap is not adequately verified because the tools needed to verify it were blocked. **Recommended next step before writing code: 30 minutes manually checking seogets.com/pricing, seotesting.com/pricing and Semrush's Content Audit feature list, plus 10 Reddit/Slack conversations with actual consultants.** If Semrush already diagnoses decoupling, this drops to RED.
