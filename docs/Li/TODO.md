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
| **Next** | Phase 9 — Invoke panel, in progress. Resume is the manual `Api.Echo` gate. Do not rebuild `scripting-invoke.js` |
| **First deploy** | Band D — Course Manager (ID 34) |
| **Effort** | XL (gated phases 0, 0a, 0b, 0c, 1–38, 5a–5d) |
| **Done** | Phases 0, 0a, 0b, 0c, 1, 2, 3, 4, 5, 7, and 8 complete (2026-10-16). Phase 9 code landed the same day. 5a–5d and 6 deferred. 1389–1393 reported applied. 1394 not in that report |
| **Remaining** | Finish the Phase 9 manual lines, then Phases 10–38. Apply `acuranzo_1394.lua` before Scripting uses Lookup 059 key 13. Band B (5a–5d, 6) when Andrew pulls it back |
| **Superseded** | [LITHIUM_SPRINT.md](/docs/Li/plans/LITHIUM_SPRINT.md) |

## P2 — Report writer (do not start)

Gated plan for manager 24. Phase 0 approved 2026-10-10. Next is
Phase 1, contracts and fixtures. PDF is the default export. The
build order is the phase index. Author formulas are Lua stored in
the definition. Do not pull this ahead of CATCHUP unless Andrew
says so.

| | |
| --- | --- |
| **Plan** | [REPORTING_PLAN.md](/docs/Li/plans/REPORTING_PLAN.md) |
| **Next** | Phase 1 — contracts and fixtures |
| **Effort** | XL across Phases 0–71. Each phase is one behavior and one gate. |
| **Done** | Phase 0 of 72 (2026-10-10) |

## Parallel (other repos)

| Item | Where |
|------|--------|
| Course Builder pipeline CB-0..35 | `/mnt/extra/Projects/500-Courses-Reception/COURSEBUILDER.md` |
| Keycloak real-IdP E2E | [`AUTH_FINALE.md`](/docs/H/plans/AUTH_FINALE.md) Phase 11 (OTP blocked) |
