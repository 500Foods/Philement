# Lithium TODO

Actionable incomplete work only. Open implementation:
[`/docs/Li/plans/REPORTING_PLAN.md`](/docs/Li/plans/REPORTING_PLAN.md).
Agent map: [`/elements/003-lithium/AGENTS.md`](/elements/003-lithium/AGENTS.md).

## Current work (2026-10-31)

The open Lithium implementation is the report writer. Start at
[REPORTING_PLAN.md](/docs/Li/plans/REPORTING_PLAN.md) Resume, then
the milestone file it names. The Sitting section there is the
operating model. Andrew starts each phase with `cdli && grok`.
Phases 1–5 are complete. Phase 6 is in progress. Andrew reported
drag, Undo, and New. The export sentence of 6.4 is still open.
Phase 7 is in progress. The first dots did not show and have
been repainted. Redo sits beside Undo. Do not start Phase 8. The
contract note is
[LITHIUM-RPT.md](/docs/Li/LITHIUM-RPT.md). One inch is 96 CSS pixels.
The phase body is
[01-designer.md](/docs/Li/plans/reporting/01-designer.md).

CATCHUP stays parked. Phase 19 stays in progress until he names
Course Builder. Do not mark it complete. Do not start Phase 20.
The P0 table below is the catchup ledger. Its "Next: Phase 11"
line is stale. Do not treat it as the next sitting.

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
| **Next** | Phase 11. Discuss it and wait. Phase 10 closed 2026-10-22. The meta tab was declined |
| **First deploy** | Band D — Course Manager (ID 34) |
| **Effort** | XL (gated phases 0, 0a, 0b, 0c, 1–38, 5a–5d) |
| **Done** | Phases 0, 0a, 0b, 0c, 1, 2, 3, 4, 5, 7, 8, 9, and 10 complete (Phase 10 closed 2026-10-22). 5a–5d and 6 deferred. 1389–1393 reported applied. 1395 and 1396 applied. 500 Courses Lookup 059 key 13 is `scripts/script-manager` |
| **Remaining** | Phases 11–38. Band B (5a–5d, 6) when Andrew pulls it back |
| **Superseded** | [LITHIUM_SPRINT.md](/docs/Li/plans/LITHIUM_SPRINT.md) |

## P2 — Report writer

Gated plan for manager 24. Phase 0 approved 2026-10-10. The phase
bodies were split into milestone files on 2026-10-30. Next is
Phase 1, contracts and fixtures, in
[01-designer.md](/docs/Li/plans/reporting/01-designer.md). PDF is
the default export. The locks stay in the parent plan. One phase
per conversation.

| | |
| --- | --- |
| **Plan** | [REPORTING_PLAN.md](/docs/Li/plans/REPORTING_PLAN.md) |
| **Phases** | [reporting/](/docs/Li/plans/reporting/) |
| **Next** | Phase 1 — contracts and fixtures, in `01-designer.md` |
| **Effort** | XL across Phases 0–71. Each phase is one behavior and one gate. |
| **Done** | Phase 0 of 72 (2026-10-10). File split 2026-10-30. |

## Parallel (other repos)

| Item | Where |
|------|--------|
| Course Builder pipeline CB-0..35 | `/mnt/extra/Projects/500-Courses-Reception/COURSEBUILDER.md` |
| Keycloak real-IdP E2E | [`AUTH_FINALE.md`](/docs/H/plans/AUTH_FINALE.md) Phase 11 (OTP blocked) |
