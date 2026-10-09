# Lithium TODO

Actionable incomplete work only. Active plan body:
[`/elements/003-lithium/CATCHUP.md`](/elements/003-lithium/CATCHUP.md).
Agent map: [`/elements/003-lithium/AGENTS.md`](/elements/003-lithium/AGENTS.md).

## How to use this file

- Sorted by **immediate ROI vs effort**.
- When CATCHUP finishes: archive per that plan’s Phase 38, update
  links, empty this file back to leftovers.

---

## P0 — Catchup (do not fork)

Work **only** via [`CATCHUP.md`](/elements/003-lithium/CATCHUP.md)
phases. Do not open parallel fix lists for lifecycle, XSS, login
partners, Scripting invoke, Course Manager, or chat/MCP.

| | |
| --- | --- |
| **Plan** | [CATCHUP.md](/elements/003-lithium/CATCHUP.md) |
| **Next** | Phase 0a — manager IDs in Helium Lookup 042 |
| **First deploy** | Band D — Course Manager (ID 34) |
| **Effort** | XL (gated phases 0, 0a, 0b, 1–38, 5a–5d) |
| **Done** | Phase 0 complete 2026-09-09 |
| **Remaining** | Phases 0a, 0b, 1–38, 5a–5d |
| **Superseded** | [LITHIUM_SPRINT.md](/docs/Li/plans/LITHIUM_SPRINT.md) |

## P2 — Report writer (draft, do not start)

Gated plan for manager 24. Phase 0 is not approved. HTML and CSV
come first. PDF and barcodes are the Lua ports (lua-pdfkit,
lua-zint). Images use Hydrogen's existing `image_scale` endpoint.
Author formulas are Lua stored in the definition. Do not pull this
ahead of CATCHUP unless Andrew says so.

| | |
| --- | --- |
| **Plan** | [REPORTING_PLAN.md](/docs/Li/plans/REPORTING_PLAN.md) |
| **Next** | Phase 0 — design lock |
| **Effort** | XL across Phases 0–26. Each phase is one sitting. |
| **Done** | 0% — plan only (2026-10-08) |

## Parallel (other repos)

| Item | Where |
|------|--------|
| Course Builder pipeline CB-0..35 | `/mnt/extra/Projects/500-Courses-Reception/COURSEBUILDER.md` |
| Keycloak real-IdP E2E | [`AUTH_FINALE.md`](/docs/H/plans/AUTH_FINALE.md) Phase 11 (OTP blocked) |
