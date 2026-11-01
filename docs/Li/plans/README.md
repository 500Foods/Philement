# Lithium plans

Working and completed implementation plans for Lithium.

- **Active backlog:** [`/docs/Li/TODO.md`](/docs/Li/TODO.md)
- **This folder:** open plans
- **Completed:** `complete/` (add `_COMPLETE` suffix when a plan finishes)

When a plan finishes: move it into `complete/`, update links, drop it from
`TODO.md`.

How a phase runs: [`/docs/PLANNING.md`](/docs/PLANNING.md).

## Active plans

### [CATCHUP](/elements/003-lithium/CATCHUP.md)

Gated Phases 0, 0a, 0b, 0c, 1–38, 5a–5d. Pause point 2026-10-22:
Phases 0 through 5, 7, 8, 9, and 10 are complete. Phases 5a–5d and 6 stay
deferred. Andrew confirmed `Api.Echo` on `https://lithium.500courses.com`.
Phase 10 closed on 2026-10-22. He declined the optional meta tab.
Next is Phase 11. Discuss it and wait for approval.
First official deployment is
**Course Manager** (ID 34) for 500 Courses.
Agent map:
[`/elements/003-lithium/AGENTS.md`](/elements/003-lithium/AGENTS.md).

Course Builder **pipeline** (Lua / Helium / human gates CB-0..35) is not
duplicated here:

[`/mnt/extra/Projects/500-Courses-Reception/COURSEBUILDER.md`](/mnt/extra/Projects/500-Courses-Reception/COURSEBUILDER.md)

### [Record marks](/docs/Li/plans/RECORD_MARKS.md)

Flags, notes, unseen badges, and watches for Lithium grids.
Query Manager is the proof, then both inboxes, then a live
badge, then Scripting, then the other grids. Proposed
2026-10-28. Not approved. Phase 1 has not started. Tickets
and Web Push are out of this plan.

### [Reporting](/docs/Li/plans/REPORTING_PLAN.md)

Report writer. Lithium manager 24 designs a JSON definition. Helium
`Reports.Render` composes an intermediate document and emits a
format. PDF is the default. The build order is the phase index.
A datasource may be a QueryRef plus parameters. Values print
through a mask. Phases 0–71. Phase 0 approved 2026-10-10. The
locks and the index stay in this file. Phase bodies are in
[`/docs/Li/plans/reporting/`](/docs/Li/plans/reporting/). Phases 1–5
are complete. Phases 6 and 7 closed 2026-10-31. Andrew
confirmed drag, Undo, Redo, export, import, and the dots.
Phase 8 is complete (2026-10-31). Andrew accepted the
rulers, the corner cycle, and the unit name. Right and
bottom rulers stay later. Phase 9 is complete
(2026-10-31). Andrew said the view scale works pretty
well. Phase 10 is complete (2026-10-31). Andrew said
it works pretty well. Phase 10-UI is in progress
(2026-10-31). The shell has the report list, Report,
Editor, and JSON pages, and the three toolbars. The
wheel zooms with Ctrl or Cmd. The 500 Courses check
is waiting. Phase 11 has not started.
The body is in
[`01-designer.md`](/docs/Li/plans/reporting/01-designer.md).
The
contract note is
[`/docs/Li/LITHIUM-RPT.md`](/docs/Li/LITHIUM-RPT.md). One inch is
96 CSS pixels.
The Sitting section in the parent is the operating model: one
phase per `cdli && grok` chat, Lithium tests with the work,
Andrew applies migrations and checks 500 Courses, then the
session writes the close-out and stops.

### Superseded

[`LITHIUM_SPRINT.md`](/docs/Li/plans/LITHIUM_SPRINT.md) — authored
2026-08-22, 0% done. Do not implement from it. Work items live in
CATCHUP.

## Related Hydrogen plans

- [`/docs/H/plans/AUTH_FINALE.md`](/docs/H/plans/AUTH_FINALE.md) — remaining OIDC / Keycloak / auth (Hydrogen)
- [`/docs/H/plans/NOTIFICATIONS_PLAN.md`](/docs/H/plans/NOTIFICATIONS_PLAN.md) — Web Push (Hydrogen; Lithium consumes in CATCHUP Phase 34)
- [`/docs/H/plans/complete/LUA_CLIENT_COMPLETE.md`](/docs/H/plans/complete/LUA_CLIENT_COMPLETE.md) — `/api/conduit/script`
- [`/docs/H/plans/complete/CHAT_FINALE_COMPLETE.md`](/docs/H/plans/complete/CHAT_FINALE_COMPLETE.md) — chat WS/REST
- [`/docs/H/plans/complete/MCP_COMPLETE.md`](/docs/H/plans/complete/MCP_COMPLETE.md) — MCP
