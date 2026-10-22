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

### [Reporting](/docs/Li/plans/REPORTING_PLAN.md)

Report writer. Lithium manager 24 designs a JSON definition. Helium
`Reports.Render` composes an intermediate document and emits a
format. PDF is the default. The build order is the phase index.
A datasource may be a QueryRef plus parameters. Values print
through a mask. Phases 0–71. Phase 0 approved 2026-10-10. Next is
Phase 1.
CATCHUP stays the active plan until Andrew pulls this one in.

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
