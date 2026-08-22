# 5 SaaS Ideas — Cross-Idea QA & Ranking

Generated 2026-08-22. Five independent research chains, one per lane, each running find → validate → competitor recon → MVP spec → QA.

---

## ⚠️ Read this before you trust anything below

**The Reddit requirement was not met, and could not be.** Verified three independent ways:

1. `WebSearch` with `allowed_domains: ["reddit.com"]` returns a hard API error — *"The following domains are not accessible to our user agent: ['reddit.com']"*. This is Reddit's own crawler policy. It would fail identically on your machine.
2. This environment's egress proxy returns 403 on CONNECT for `reddit.com` — logged as `connect_rejected: gateway answered 403`.
3. Nearly every verification target is also egress-blocked: `g2.com`, `capterra.com`, `producthunt.com`, `news.ycombinator.com`, `apps.shopify.com`, `community.shopify.com`, `trustpilot.com`, and every vendor pricing page tried. Only `api.github.com` and the WebSearch index responded.

**Consequence:** zero verbatim Reddit quotes across all five ideas. Every agent flagged this and **none fabricated a citation** — which is the single most important quality signal in this whole exercise. Pricing and complaint data come from search-index snippets and are tagged `UNVERIFIED` in each file.

**Also:** subagents in this session had no Agent tool, so the requested 5×5 = 30-agent structure did not happen. Each master ran the five roles as sequential phases itself. You got 5 agents × 5 phases.

**What this set actually is:** a well-reasoned hypothesis set with explicit kill criteria — not validated research. Treat every idea as needing the verification pass in the last section before you write code.

---

## The five

| # | Product | Lane | Buyer | Entry price | Verdict |
|---|---|---|---|---|---|
| 5 | **QuestionnaireDesk** | B2B SaaS ops | Founder / AE / fractional CISO | $49/mo | AMBER |
| 2 | **FeedTriage** | E-commerce | Google Ads agency / DTC lead | $49/mo | AMBER |
| 4 | **ClickGap** | SEO / content | SEO consultant / agency | $29/mo | AMBER |
| 1 | **RoundTwo** | Agency ops | Agency owner / freelancer | $29/mo | AMBER |
| 3 | **Handled** | Local services | Home-services agency owner | $49/mo | AMBER |

**No GREENs, no REDs.** Correct outcome — without primary-source validation nothing earns a GREEN, and none of the five is bad enough to kill outright. Two candidates *were* killed during the run (local AI-visibility tracking, white-label AI-audit generators), both for the same reason: in 2026, anything whose core is "an LLM writes a document" is commoditised to near-zero.

---

## My ranking, and why

### 1. QuestionnaireDesk — best pain urgency + clearest proof of payment
A security questionnaire **blocks a deal that is already verbally won**. That is an acute, budget-unlocking purchase, not a chronic annoyance — and acute pain is what a solo founder with no brand needs, because it shortens the sales conversation to nothing. Proof of payment exists at two separate tiers ($9.6k–$25k/yr enterprise; $124–$250/mo indie), which is the strongest willingness-to-pay evidence in the set. The technical wedge — cell-perfect XLSX round-trip with merged cells and dropdown validation intact — is unglamorous enough that funded competitors won't prioritise it, and PhpSpreadsheet makes it directly in your wheelhouse. The free-tool funnel (upload → watch 10 rows fill → pay to unlock the download) is the best cold-start motion of the five: self-demonstrating and shareable.
**Against it:** entrant #8 at the low end. Verify ResponseHub's $124/mo and 1up's $250/mo first-hand — those two numbers decide whether the wedge exists.

### 2. FeedTriage — best cold outreach hook, zero approval gates
The GTM is the strongest thing here: you can generate a real audit from any Shopify store's **public** `/products.json` with no auth, no OAuth, no app review — then cold-email the owner "412 of your 1,180 products have no GTIN." That is a demo, not a pitch. The supplemental-feed-URL delivery mechanism sidesteps Merchant Center OAuth entirely, and programmatic SEO on Merchant Center error strings is a genuine zero-ad-spend channel. Google's automatic improvements explicitly *don't* touch titles/descriptions/GTINs, so the layer is durably unowned by the platform.
**Against it:** the weakest moat of the five. GoDataFeed already bundles AI titles + error monitoring at $39, and Simprosys anchors merchants at $5–18/mo. Squeezed from both sides — this is a distribution bet, not a product bet.

### 3. ClickGap — best margins, cleanest build, most incumbent risk
97% gross margin because the diagnosis is deterministic arithmetic and the LLM only writes the memo (~$0.15/report against $199/mo). Two CSV uploads sidestep Google's sensitive-scope OAuth review — which matters more than it sounds, because GSC testing-mode refresh tokens expire in 7 days and would otherwise be a hard MVP blocker. The AI-Overview-vs-rank-loss diagnostic split is a genuinely current problem.
**Against it:** the wedge is one feature wide and Semrush Content Audit is one product decision from closing it. Highest "incumbent kills you" risk in the set.

### 4. RoundTwo — widest open gap, softest core assumption
The wedge is real and well-argued: every incumbent asks the *client* to change behaviour (install a widget, log into a hub), and the category's own marketing admits that fails. RoundTwo inverts it — the client changes nothing. No competitor knows what the contract said, so none produces a *priced* change order. The Ignition survey (273 agency managers, 57% losing $1k–$5k/month to unbilled scope) is the most credible single datum in the whole set.
**Against it:** the entire thesis rests on agencies being *willing* to bill for out-of-scope work — and the same survey says only 1% currently do. That could be a latent need, or it could be that they'd rather eat it than risk the relationship. Coin flip, and the agent's own kill criterion (<20% of rounds containing a billed item) admits it.

### 5. Handled — good buyer, narrow market, rented data
Correctly dodges the lane's fatal trap by selling to home-services *agencies* rather than to plumbers, who don't buy software from websites. No approval gates — CallRail's v3 API has self-serve keys and the design never touches telephony, so no A2P 10DLC registration.
**Against it:** two compounding problems. It rents its core data from CallRail, who could gate or close the feature. And the social dynamic is untested — you're asking an agency to pay for a report that points at their own client's staff. The agent found no buyer-voice evidence either way, which given the Reddit blackout means genuinely none.

---

## Cross-cutting patterns the individual agents couldn't see

**Three of five converged on the agency buyer** (ClickGap, Handled, FeedTriage) — independently, from different lanes. That is a real signal, not a coincidence: agencies are reachable on LinkedIn, have discretionary tool budget, buy portfolio-wide so one sale is worth more, and don't need procurement. If you build any one of these, the distribution channel you learn transfers to the other two.

**All five proposed the same GTM: a free manual audit as the pitch.** Legitimately the right cold-start motion for a founder with no audience — the deliverable *is* the demo. But the convergence means the agents anchored on it, so treat it as one validated pattern rather than five independent confirmations.

**Every idea deliberately routes around an OAuth/approval gate** — CSV upload, XLSX round-trip, public JSON, supplemental feed URL, self-serve API key. This was the constraint I gave them and it held. None of these five has a multi-week review sitting between you and your first sale.

**The two killed candidates share a lesson worth keeping:** "LLM writes a document" is not a business in 2026. The three strongest ideas here all accrue value from something else — an integration (Handled), accumulated data history (ClickGap, QuestionnaireDesk's answer library), or a file-format capability (QuestionnaireDesk).

---

## Verify these before writing a line of code

The blackout means the following are load-bearing assumptions, not facts. This is ~90 minutes of manual work in a browser and it will kill at least one of these ideas cheaply.

**Universal (do this first):** read 10–15 Reddit threads per lane in the subs each file names. You're checking one thing: do practitioners describe this pain in their own words, unprompted? If not, the idea dies there.

| # | Verify | Why it's load-bearing |
|---|---|---|
| 5 | ResponseHub at $124/mo, 1up at $250/mo | If either is cheaper or ships XLSX round-trip, the wedge is gone |
| 5 | That no incumbent returns the buyer's own workbook filled | This *is* the differentiation |
| 2 | GoDataFeed's $39 tier — does it already do AI titles + error monitoring? | If yes, you're undercut before launch |
| 2 | That a supplemental feed URL still accepts scheduled fetch | The whole no-OAuth design depends on it |
| 4 | Semrush Content Audit — does it already split AI-Overview loss from rank loss? | If yes, ClickGap is RED |
| 1 | Do BugHerd / Atarim / Marker.io already ingest pasted email? | Single most important unknown in that file |
| 1 | Ask 5 agency owners: "would you send a client a priced out-of-scope list?" | The entire thesis |
| 3 | CallRail v3 API — self-serve key, call recording access, rate limits | No API, no product |
| 3 | Offer 20 agencies a free manual CSV audit | Agent's own bar: under 2 yeses, don't build |

---

## Files

- `idea1.md` — RoundTwo (agency ops)
- `idea2.md` — FeedTriage (e-commerce)
- `idea3.md` — Handled (local services)
- `idea4.md` — ClickGap (SEO/content)
- `idea5.md` — QuestionnaireDesk (B2B SaaS ops)
