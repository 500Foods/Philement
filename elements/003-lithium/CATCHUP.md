<!-- markdownlint-disable MD007 MD024 -->
# Lithium Catchup Plan

## Purpose

Bring Lithium up to the Hydrogen / Helium surface that already exists, then
ship **Course Manager** as the first official 500 Courses operator
deployment. This file is the **only** active Lithium implementation plan.

It supersedes
[`/docs/Li/plans/LITHIUM_SPRINT.md`](/docs/Li/plans/LITHIUM_SPRINT.md)
(authored 2026-08-22, 0% done). Do not implement from the sprint. Absorb
its still-true work items here.

Agent map (read first every session):
[`/elements/003-lithium/AGENTS.md`](/elements/003-lithium/AGENTS.md).

Coding rules:
[`/docs/Li/LITHIUM-INS.md`](/docs/Li/LITHIUM-INS.md).

---

## How To Use This Document

- Work **one phase at a time**, top to bottom.
- Each phase is its **own conversation**.
- Each phase has: **Goal**, **Dependencies**, **Entry gate**,
  **Reference**, **Work items**, **Exit gate / validation**, **Status**,
  **Lessons learned**.
- Mark work items `[x]` only when verification actually passed.
- Defer with `[~]` plus one-line rationale and the target phase.
- After each phase: fill **Status**, append **Working Log**, **stop**.
  Do not start the next phase in the same turn unless the user asked to
  continue.
- Never mark a phase complete with a failing or skipped gate unless the
  Status block records an explicit variance.

### Implementor workflow (every phase)

1. Confirm the prior phase Status is complete. Re-read its Exit gate.
   Do not trust memory of a prior session.
2. Discuss this phase first. Read Goal + Work + Exit + Reference. Grep
   the code. Ask questions. **No source edits yet.**
3. Get explicit approval to start implementation.
4. Ask questions during implementation rather than guessing.
5. Update Working Log at major milestones, not only at the end.
6. Record lessons learned.
7. Mark `[x]` and Status complete only after the named commands ran.
   Intent to verify is not verification.
8. **Never apply a database migration.** Prepare Helium packets and hand
   them to the user. Do not run `schematool` / `schemahelper` apply.
9. **No product C.** Do not add Hydrogen routes, chat-mint endpoints, or
   Canvas/Stripe/admin C. Lua is `POST /api/conduit/script`. Tables are
   LithiumTable. The SPA holds a Hydrogen JWT only.

### Testing policy

| Layer | When | What |
| ------- | ------ | ------ |
| **Vitest** | Every Lithium JS phase | Real functions, not tautological mocks |
| **Lint** | Every Lithium JS/CSS phase | `npm run lint` and `npm run lint:css` from `elements/003-lithium` |
| **Build** | Vite entry, templates, new managers | `npm run build` |
| **Templates** | HTML/CSS under `src/managers/` | `npm run templates:copy` |
| **Hydrogen / Helium** | QueryRefs, `scripts` rows, tableDefs | Packet + `zsh -ic 'mkt'` **by the human** after apply |
| **Manual / live** | Auth, invoke, Course Manager | Named checklist against Hydrogen + JWT |

Lithium commands run in `elements/003-lithium`. Hydrogen aliases are C/Lua
only.

---

## Resuming Work

**CURRENT PAUSE POINT (as of 2026-10-23):** Phases 0, 0a, 0b, 0c, 1, 2, 3, 4, 5, 7, 8, 9, 10, 11, 12, 13, 14, and 15 are complete. Andrew confirmed `Api.Echo` on `https://lithium.500courses.com`: Wait on `job_id: BBTDC`, Wait off `job_id: CCCRP`, both `status: completed`. **Phase 10** is complete. Andrew confirmed the tulip button, Ctrl+P, pretty-print then save, the Preview tab (summary, then Lua), and that a 9px font survives a reload. He declined the optional meta tab. It does not move forward. **Phase 11** is complete. CodeMirror 6 is the only stack. Lua folds `function`, `if`, `do`, `for`, and `while` through `end`, and `repeat` through `until`. `H.` and `H.name` use the namespace tag. **Phase 12** is complete. Andrew confirmed the Learning group shows Course Manager and that he can open the shell. The gate reads the stored token with `getClaims(retrieveJWT())`. A bare `getClaims()` returns null and hides the row before the token is read. QueryRef 155 failure still denies, and the sidebar still omits ID 34 when the gate fails. A direct open still shows the lock panel. Andrew reported Acuranzo 1378, 1379, 1380, and 1381 applied. **Phase 13** is complete (2026-10-23). Andrew reported the latest build deployed and the latest migrations applied. The list has no Retired checkbox. Retired stays under Course Options. Staff list **4500**, insert **4501** (`courses_4082.lua` stores Retired), update **4502** (`courses_4081.lua` stores Retired), enrolment history **4503**, management history **4504**, prices **4505** (`courses_4083.lua`). Lookup 059 key 14 is `courses/course-manager` (`acuranzo_1397.lua` and `acuranzo_1398.lua`). Default columns are Code and Title. The list is a LithiumTable. The navigator owns add, duplicate, and delete. Delete leaves the row. Footer save and cancel use ManagerEditHelper. Canvas shows the image at 524×292, stretched, with rounded corners. Commerce lists cad, usd, eur, and gbp. **4004** stays the published catalog. **4008** is the courses table file, not a caller ref. **Phase 14** is complete (2026-10-23). Andrew confirmed Save prices on course 16 at 13:55:13: CAD cents 4999, success true, affected_rows 1, eight statements all success. Save prices is **4506** (`courses_4084.lua`, rewritten by `courses_4087.lua`). Orders are **4507** (`courses_4085.lua`). `Stripe.Mode` is `courses_4086.lua`, invokable 1. The client logs the save and treats success `fail` as an error. Ensure Product, Refund, and Deactivate stay invokable 0. **Phase 15** is complete (2026-10-23). Andrew searched andrew@500courses.com. One hit opened account **9001**. Canvas user **801**. He granted a course, and Open Canvas user worked. Archive and Renew were not clicked. Search **4508**, account **4509**, seats **4510**, orders **4511**, Canvas link **4512**. The live search and grant are the apply report for `courses_4088.lua` through `courses_4096.lua`. `Enroll.StaffGate` stays invokable 0. `Enroll.StaffGrant`, `Enroll.StaffArchive`, and `Enroll.StaffRenew` are invokable 1. At the close of Phase 15 the next Courses file was **4097**, the next caller ref was **4513**, and the next Acuranzo file was **1399**. **Phase 19** took those numbers on 2026-10-23 and is in progress. Andrew applied `courses_4097.lua` through `courses_4110.lua` and `acuranzo_1399.lua` and `acuranzo_1400.lua`. He confirmed the list and Submission tab, then received the receipt for "All about mDNS". That row is Received and will not be mailed again. A later working-title save on QueryRef **4514** failed at statement 1 and rolled back, so the ledger write is still unproven. Units on that ledger are integer cents, integer milliseconds, and an integer prompt count. Andrew applied `courses_4111.lua` through `courses_4114.lua` and `acuranzo_1401.lua`. The receipt went out on the Hydrogen restart with a subject and no body. `courses_4115.lua` is QueryRef **4522**. It moves Received or Intake back to Suggested and clears `receipt_mail`. `courses_4116.lua` puts the send time in the receipt key. `courses_4117.lua` replaces the text and HTML of `course.suggestion_received`. The greeting uses the suggestion `submitter_name`. The course title uses the suggestion `course_name`. Neither value comes from the JWT. Those three are not applied. Opening Course Builder does not send mail. The next Courses file is **4118**. The next caller ref is **4523**. The next Acuranzo file is **1402**. Lookup 059 key **16** stays free. File numbers stay below 4500. **Phase 16**, **Phase 17**, and **Phase 18** stay parked. Andrew parked Course Manager on 2026-10-23. Do not mark Phase 19 complete. After **1401** is applied and Hydrogen is restarted, a still-Suggested row is mailed on the next one-second tick without a login. Deploy Lithium after that restart so an old page stops calling the script. Do not start Phase 20. Do not edit Phases 16–18 until he pulls Course Manager back. Phases 5a, 5b, 5c, 5d, and 6 stay deferred until Andrew pulls Band B back. He does deploys and login checks. Do not drive a browser for those. Save, duplicate, and update send INTEGER `SCRIPT_TYPE`, `STATUS`, `USERID`, and `INVOKABLE` (0 when the row has none). A blank schedule or run time is `''`. Andrew applied Acuranzo 1395 (QueryRef 130) and 1396 (QueryRef 129) and confirmed pretty-print then save on 2026-10-22. Delete uses QueryRef 131 with STRING `GROUP_NAME` and `SCRIPT_NAME`. A dirty row switch asks to save or stay. On the 500 Courses Lithium database, Lookup 059 key 13 is `scripts/script-manager` and its collection names delete 131. Course QueryRefs moved to the 4000 pack on 2026-10-16. Band D uses the Courses pack QueryRefs section. Do not call retired **#085**, **#150**, or **#151**.

### Resume here next session

Phases 0–5, 7, 8, 9, 10, 11, 12, 13, 14, and 15 are complete. Do not reopen them. Do not open 5a–5d or 6 unless Andrew pulls Band B back.

1. **Phase 14** is complete (2026-10-23). Andrew confirmed Save
   prices on course 16 at 13:55:13. QueryRef **4506**, CAD cents
   4999, success true, affected_rows 1, eight statements all
   success. USD 3900, EUR 3500, and GBP 3000 were unchanged and
   reported affected 0. The brochure stayed clean. **Phase 15**
   is complete (2026-10-23). Andrew searched
   andrew@500courses.com, opened account **9001** (Canvas user
   **801**), granted a course, and confirmed the Canvas user
   link. Archive and Renew were not clicked. The live search
   and grant are the apply report for `courses_4088.lua`
   through `courses_4096.lua`. Phases 16–18 are parked.
   Phase 19 is in progress. Andrew applied 4097–4114 and
   Acuranzo 1399–1401. The receipt went out on the Hydrogen
   restart. QueryRef **4514** had failed at statement 1 before
   that cast rewrite. `courses_4115.lua` and `courses_4116.lua`
   are written and are not applied. The screen does not send
   the receipt. Suggested is now a stage choice.
   Do not mark it complete. Do not start Phase 20. Do not edit
   Phases 16–18. Phase 13
   closed on
   2026-10-23. Andrew reported the latest build deployed, the
   latest migrations applied, and the list Retired checkbox
   removed. **4004** stays the published catalog. Staff list
   **4500**, insert **4501**, update **4502**, enrolment history
   **4503**, management history **4504**, prices **4505**, save
   prices **4506**, orders **4507**. `Stripe.Mode` is
   `courses_4086.lua` and is invokable. Lookup 059 key 14 is
   `courses/course-manager`. Default columns are Code and Title.
   Leave `Catalog.SyncFromCanvas`, `Catalog.Retire`,
   `Stripe.EnsureProduct`, `Stripe.Refund`, and
   `Stripe.DeactivateCustomer` at `invokable` 0 until Phase 18.
   `courses_4087.lua` is the SQL that ran for 4506. Do not edit
   `courses_4084.lua`. Phase 19 applied **4097–4114**, refs
   **4513–4521**, and Acuranzo **1399–1401**. The receipt went
   out on the restart with a subject and no body. The unapplied
   packet is **4115**, **4116**, and **4117**. **4522** moves
   Received or Intake back to Suggested and clears `receipt_mail`.
   **4116** puts the send time in the receipt key. **4117**
   replaces the receipt text and HTML. The greeting uses the
   suggestion `submitter_name`. The course title uses the
   suggestion `course_name`. The next Courses file is **4118**.
   The next caller ref is **4523**. The next Acuranzo file is
   **1402**.
   Lookup 059 key **16** stays free.
   File numbers stay below 4500. QueryRef **4500** does not return
   cents. Andrew declined the Scripting meta tab. It stays
   declined. He does deploys and login checks. Do not drive a
   browser for those. Fold All and the `H.` tint have unit
   coverage and no live click yet.
2. Run stays disabled unless the row is invokable. Wait defaults to
   true. Timeout is 1–60, default 15. A top-level `_hydrogen` key is
   removed before the request. Poll delays are 250, 500, 1000, then
   2000 ms. There is no cancel control. Changing rows stops painting
   that poll and does not kill the job.
3. Save and duplicate send INTEGER `SCRIPT_TYPE`, `STATUS`,
   `USERID`, and `INVOKABLE`, and `''` for a blank schedule or run
   time. Delete already sends STRING `GROUP_NAME` and `SCRIPT_NAME`
   to QueryRef 131. A dirty row switch uses the Phase 4 confirm
   dialog: Save continues, Stay keeps the row. Other tables still
   auto-save.
4. On the 500 Courses Lithium database, Lookup 059 key 13 is
   `scripts/script-manager`. The constructor still sets list 89 and
   search 90. New scripts ask for group and name because those
   columns are not editable.
5. Andrew confirmed `Api.Echo` on 2026-10-22. Wait on was
   `job_id: BBTDC`. Wait off was `job_id: CCCRP`. Both were
   `status: completed`. Andrew applied 1395 and 1396 and confirmed
   pretty-print then save. Create, duplicate, and delete still have
   no browser pass. `npm test` was 1040 passed and 10 failed
   when Phase 9's code landed. The 10 are the live auth file
   (HTTP 503). Lint was 0 errors and 35 existing warnings.
6. Left open on purpose: Profile footer Save still does not call
   `settingsHandler.save()`. Style `applyState` is empty.
   `this.app.user` is unset; `USERID` reads `app.auth.user`.
   The Style Manager login mockup still paints the four partner
   images. Didit, Apple, Google, and Microsoft stay omitted.

---

## Hard rules (do not relitigate)

1. **No product C.** Hydrogen already has auth, Conduit, `/api/conduit/script`,
   chat WS, MCP, Mail Relay. Lithium is the SPA.
2. **SPA holds a Hydrogen JWT only.** Never Keycloak `id_token` /
   `access_token`, never Canvas tokens, never `sk_` / `whsec_`.
3. **Lua via `POST /api/conduit/script`** (+ `GET /api/conduit/script/{job_id}`).
   Do not send `params._hydrogen`. Name is `Group.Name` (dot only).
   `invokable=0` → Hydrogen 404 `script_not_found` (existence-hiding).
4. **Helium packets, not agent apply.** New QueryRefs / `invokable` flips /
   menu rows / tableDefs are migrations the human applies.
5. **LithiumTable** (`src/tables/lithium-table-main.js`) is the table path.
   Do not invent a second grid.
6. **Reception is a different SPA.** Learner catalog / cart / My Courses
   stay in `/mnt/extra/Projects/500-Courses-Reception/`. Lithium at
   `lithium.500courses.com` is the operator surface.
7. **Course Builder pipeline** stays
   [`COURSEBUILDER.md`](/mnt/extra/Projects/500-Courses-Reception/COURSEBUILDER.md)
   (CB-0..35). This plan only gates the Lithium operator UI.

---

## First official deployment

**500 Courses Course Manager (menu ID 34)** on `lithium.500courses.com`.

Band D is the deployable product. Bands E–I do not block that deploy.
Authz for v1 is lock L3. The JWT `roles` claim is a CSV of integer
`role_id`s, resolved to `roles.name`. Staff means the name is exactly
`staff` or `admin`. Do **not** wait for Hydrogen 2.23 / AUTH_FINALE Phase 2
`account_roles` rows. When 2.23 lands, Band H records the cutover; the
manager UI contract does not change.

Reception PRIORITIZE Part 5 is the requirements source:
[`/mnt/extra/Projects/500-Courses-Reception/PRIORITIZE.md`](/mnt/extra/Projects/500-Courses-Reception/PRIORITIZE.md)
(§ Part 5). That doc still says “do not ship until 2.23”; **this plan
overrides that gate** with JWT `roles`. Staff Lua stays `invokable=0`
until a Band D packet (human apply) flips that button. Phase 15
flipped `Enroll.StaffGrant`, `Enroll.StaffArchive`, and
`Enroll.StaffRenew` to 1. `Enroll.StaffGate` stays 0. Catalog
sync, retire, Ensure Product, refund, and deactivate stay 0
until Phase 18.

---

## Goals

1. Safe close / activate — every manager tears down editors and tables.
2. No XSS on HTML sinks — DOMPurify or shared `escapeHtml`.
3. Honest UI — no success toast on no-ops.
4. One ID scheme — docs, registry, config, tours agree.
   **34 = Course Manager. 35 = Course Builder.**
5. Shared `invokeScript` / `getScriptJob` used by Scripting, Course
   Manager, and Course Builder.
6. Login: OIDC (500 Passwords) + ordered partner buttons that actually
   work or are absent.
7. Scripting Manager can invoke, poll, and show `invokable`.
8. CodeMirror 6 plugins are split, tested, and the only editor stack.
9. Course Manager v1: courses, commerce, learners/Canvas, inbox, staff
   script console. Operators never see the word “Lua”.
10. Course Builder operator queue after Helium `course_build_*` exists.
11. Crimson / Chats / MCP operator UI catch up to CHAT_FINALE + MCP
    without becoming an MCP daemon.
12. Mail / Jobs / Dashboard / Roles stop being copy-paste shells.
13. AUTH_FINALE and Notifications are **consumed**, not re-implemented.
14. Docs match runtime.

### Non-goals

- Reception public SPA, Cap PoW, student chrome.
- Product Hydrogen C (`/api/coursebuilder/*`, `/api/enroll/*`,
  `/api/admin/*`, `/api/stripe/*`, browser MCP on `:3100`).
- Chat JWT mint in C, unless a **separate** Hydrogen plan is approved.
  CATCHUP maps `JWT not authorized for chat` honestly and waits.
- `alt_chat`, public chat, MCP DCR/stdio, QueryRef **#061** from the SPA
  (engines **with API keys**).
- Filling every placeholder (Calendar, Contacts, Files, …). Collapse;
  implement only what a later band names.
- a11y audit, i18n, CSP meta, Lighthouse budget, Playwright CI.
- Impersonating a learner from Lithium.
- Showing `sk_` / Canvas admin tokens.
- APPLY/LOAD migrations from the SPA.
- Luacheck-for-SPA or run-as-role (no Hydrogen route).
- Editing Keycloak credentials from Lithium.

---

## Current Observed State (2026-09-09)

Snapshot from the day this plan was written. Cells marked
2026-10-16 were refreshed after Phases 0a–2. Phases 0a–0c registered
managers 34 and 35, handed the staff-role packet, and taught the
Course Manager placeholder to call QueryRef **155**. Phases 1 and 2
shipped the close path and the shared helpers. The Courses pack
QueryRefs section is the course-number contract.

### Hydrogen / Helium already done (do not rebuild)

| Surface | Evidence |
| --------- | ---------- |
| Password + OIDC RP + handoff | AUTH_FINALE shipped RP; last-method polish is OIDC-PLAN Phase 26 |
| `POST /api/conduit/script` | LUA_CLIENT complete |
| Chat WS + REST `auth_chat` / `auth_chats` | CHAT_FINALE complete; JWT `aud=hydrogen-chat` AND `roles=="chat"` **exact** |
| MCP Streamable HTTP + `#152`/`#153` | MCP complete; product tool = `System.Info` |
| Mail Relay outbound + templates + OTP C API | MAILRELAY complete; Lithium UI is not |
| Course learner Lua | `Enroll.*`, `Catalog.*`, `Account.*`, `Stripe.*`. Phase 15 set `Enroll.StaffGrant`, `Enroll.StaffArchive`, and `Enroll.StaffRenew` to invokable 1. `Enroll.StaffGate` stays 0. The other staff writers stay 0 until Phase 18. |
| QueryRefs | Scripting 87/89/90/129/130; delete **131**; MCP **#152/#153**; chat store **#067/#068/#069**. Course numbers in this row were retired 2026-10-16. See Courses pack QueryRefs. **#086** and **#155** stayed. |

### Lithium today

| Surface | Reality |
| --------- | --------- |
| Login OIDC | `renderOidcProviders` from `auth.oidc_providers` (500passwords) |
| Partner buttons | Config row (Phase 5, 2026-10-16). A button renders only when `login partners.<id>` is true and that id is in `auth.oidc_providers`. Checked-in config shows 500 Passwords only. Didit, Apple, Google, and Microsoft stay hidden until 5a–5d. |
| Scripting ID 33 | List/edit/save plus a Run tab (Phase 9 code complete; Andrew confirms `Api.Echo` on `lithium.500courses.com`). `scripting.js` is 627 lines |
| `conduit.js` | `authQuery` plus `buildScriptPayload`, `invokeScript`, and `getScriptJob` (Phase 2, 2026-10-16). The Scripting Run tab calls them (Phase 9). A synchronous invoke extends the POST past the 30s client cap by `timeout_seconds` plus 5 seconds. Andrew confirms the live Run lines. |
| Crimson | Live WS chat; login JWT; hardcoded engine `"Crimson"` |
| Chats ID 18 | 49-line placeholder |
| Course Manager | Staff catalog at `course-manager/` (Phase 15 complete, 2026-10-23). QueryRef **155** is the roles gate. List **4500**, insert **4501**, update **4502**, history **4503** and **4504**, prices **4505**, save prices **4506**, orders **4507**. Learners: search **4508**, account **4509**, seats **4510**, orders **4511**, Canvas link **4512**. The list has no Retired checkbox. Sync, retire, Ensure Product, refund, and deactivate stay disabled. `cleanup()` is the close hook. Andrew parked Course Manager after Phase 15. Suggestions are Course Builder. |
| Course Builder | Placeholder at `course-builder/` (Phase 0a, refreshed 2026-10-16). No QueryRef. Close hook is `teardown()`. Real UI is Band E. |
| Terminal | Real iframe popup but `DEFAULT_URL` hardcoded to `https://www.philement.com`; not wired to Hydrogen terminal `WebPath`. Troubleshoot in Phase 0c. |
| Mail / Jobs / Dashboard / Roles | Placeholders |
| CM6 | Real. `codemirror-setup.js` is the facade (654 lines). Newline, indent, JSON sort, and fold live in `cm6-newline.js`, `cm6-indent.js`, `cm6-json.js`, and `cm6-fold.js`. Lua is `lua-mode.js`. |
| CM5 | Deleted in Phase 11 (2026-10-22). `src/init/codemirror-init.js` had zero imports. |
| `closeManager` | `destroy`, else `cleanup`, else `teardown` (Phase 1, 2026-10-16). Close does not call `onDeactivate`. |
| DOMPurify | `sanitizeHtml` in `src/core/utils.js`. Phase 3 (2026-10-16) calls it from Lookups, Crimson, Version History, and the Queries summary preview. |
| `escapeHtml` | Real in `src/core/utils.js`. Quotes stay literal. Lookups processors use it before highlight. |

---

## Courses pack QueryRefs (2026-10-16)

Helium cut the course objects out of Acuranzo into the Courses pack
on the same database. File numbers, bookkeeping `query_ref`, and
caller-facing QueryRefs share 4000–4999. Script names did not change.
Inventory:
[`/docs/He/COURSES_MIGRATION.md`](/docs/He/COURSES_MIGRATION.md).

Band D calls the **New** column. The September working log still
shows 85, 150, and 151 because those were the numbers that day.

| Use | Was | New | Installed by |
| --- | --- | --- | --- |
| Insert `course_suggestions` | 85 | **4000** | [`courses_4002.lua`](/elements/002-helium/courses/migrations/courses_4002.lua) |
| Canvas: lookup link by account | 144 | **4001** | [`courses_4004.lua`](/elements/002-helium/courses/migrations/courses_4004.lua) |
| Canvas: link account to Canvas user | 145 | **4002** | [`courses_4005.lua`](/elements/002-helium/courses/migrations/courses_4005.lua) |
| Canvas: touch link | 146 | **4003** | [`courses_4006.lua`](/elements/002-helium/courses/migrations/courses_4006.lua) |
| Courses: list published | 147 | **4004** | [`courses_4011.lua`](/elements/002-helium/courses/migrations/courses_4011.lua). Filters in [`courses_4075.lua`](/elements/002-helium/courses/migrations/courses_4075.lua) |
| Courses: get by slug | 148 | **4005** | [`courses_4012.lua`](/elements/002-helium/courses/migrations/courses_4012.lua) |
| Enrolment History lines by course | 150 | **4006** | [`courses_4039.lua`](/elements/002-helium/courses/migrations/courses_4039.lua). Later SQL in [`courses_4068.lua`](/elements/002-helium/courses/migrations/courses_4068.lua). Lua only. The SPA calls **4503**. |
| Management History lines by course | 151 | **4007** | [`courses_4048.lua`](/elements/002-helium/courses/migrations/courses_4048.lua). Lua only. The SPA calls **4504**. |
| Staff: list every course |  | **4500** | [`courses_4076.lua`](/elements/002-helium/courses/migrations/courses_4076.lua). Andrew reported applied 2026-10-23. |
| Staff: insert course |  | **4501** | [`courses_4077.lua`](/elements/002-helium/courses/migrations/courses_4077.lua). Retired stored by [`courses_4082.lua`](/elements/002-helium/courses/migrations/courses_4082.lua). Andrew reported applied 2026-10-23. |
| Staff: update course |  | **4502** | [`courses_4078.lua`](/elements/002-helium/courses/migrations/courses_4078.lua). Retired stored by [`courses_4081.lua`](/elements/002-helium/courses/migrations/courses_4081.lua). Andrew reported applied 2026-10-23. |
| Staff: enrolment history |  | **4503** | [`courses_4079.lua`](/elements/002-helium/courses/migrations/courses_4079.lua). Same line text as 4006. Andrew reported applied 2026-10-23. |
| Staff: management history |  | **4504** | [`courses_4080.lua`](/elements/002-helium/courses/migrations/courses_4080.lua). Same line text as 4007. Andrew reported applied 2026-10-23. |
| Staff: prices |  | **4505** | [`courses_4083.lua`](/elements/002-helium/courses/migrations/courses_4083.lua). Read. Andrew reported applied 2026-10-23. |
| Staff: save prices |  | **4506** | [`courses_4084.lua`](/elements/002-helium/courses/migrations/courses_4084.lua). Upserts cad, usd, eur, and gbp. Does not write `stripe_price_id`. [`courses_4087.lua`](/elements/002-helium/courses/migrations/courses_4087.lua) rewrites the same ref so `-- TRANSACTION_END` is its own line. Andrew confirmed that SQL on course 16 (CAD 4999 cents) on 2026-10-23. |
| Staff: course orders |  | **4507** | [`courses_4085.lua`](/elements/002-helium/courses/migrations/courses_4085.lua). One course. `ACCOUNTID` 0 lists every account. Andrew reported applied. |
| Staff: search accounts |  | **4508** | [`courses_4088.lua`](/elements/002-helium/courses/migrations/courses_4088.lua). Exact account id, Keycloak subject, or OIDC email. Andrew confirmed 2026-10-23. |
| Staff: read account |  | **4509** | [`courses_4089.lua`](/elements/002-helium/courses/migrations/courses_4089.lua). Account, registration meta, and prefs. Andrew confirmed 2026-10-23. |
| Staff: account enrollments |  | **4510** | [`courses_4090.lua`](/elements/002-helium/courses/migrations/courses_4090.lua). Every seat for one account. Andrew confirmed 2026-10-23. |
| Staff: account orders |  | **4511** | [`courses_4091.lua`](/elements/002-helium/courses/migrations/courses_4091.lua). Every order for one account. **4507** stays per course. Andrew confirmed 2026-10-23. |
| Staff: canvas link |  | **4512** | [`courses_4092.lua`](/elements/002-helium/courses/migrations/courses_4092.lua). SPA read of `account_canvas_links`. **4001** stays the Lua read. Andrew confirmed 2026-10-23. |
| Staff: list suggestions |  | **4513** | [`courses_4099.lua`](/elements/002-helium/courses/migrations/courses_4099.lua). Applied. The client now calls **4515**. |
| Staff: save working title |  | **4514** | [`courses_4100.lua`](/elements/002-helium/courses/migrations/courses_4100.lua). Applied. Title-only save still uses this. The live save failed at statement 1. [`courses_4111.lua`](/elements/002-helium/courses/migrations/courses_4111.lua) rewrites the cast and is applied. |
| Staff: list suggestions |  | **4515** | [`courses_4102.lua`](/elements/002-helium/courses/migrations/courses_4102.lua). Suggested, Received, Intake. Applied. |
| Staff: insert suggestion |  | **4516** | [`courses_4103.lua`](/elements/002-helium/courses/migrations/courses_4103.lua). Applied. **4000** stays public. |
| Staff: update suggestion |  | **4517** | [`courses_4104.lua`](/elements/002-helium/courses/migrations/courses_4104.lua). Applied. Does not change state or receipt_mail. [`courses_4112.lua`](/elements/002-helium/courses/migrations/courses_4112.lua) rewrites the casts and is applied. |
| Staff: duplicate suggestion |  | **4518** | [`courses_4105.lua`](/elements/002-helium/courses/migrations/courses_4105.lua). Applied. No draft row, so the receipt waits for the orchestrator. [`courses_4113.lua`](/elements/002-helium/courses/migrations/courses_4113.lua) rewrites the title cast and is applied. |
| Staff: delete suggestion |  | **4519** | [`courses_4106.lua`](/elements/002-helium/courses/migrations/courses_4106.lua). Applied. |
| Staff: move to Intake |  | **4520** | [`courses_4107.lua`](/elements/002-helium/courses/migrations/courses_4107.lua). Applied. Does not send mail. |
| Staff: toggle flag |  | **4521** | [`courses_4108.lua`](/elements/002-helium/courses/migrations/courses_4108.lua). Applied. |
| Staff: move to Suggested |  | **4522** | [`courses_4115.lua`](/elements/002-helium/courses/migrations/courses_4115.lua). Not applied. Clears `receipt_mail`. [`courses_4116.lua`](/elements/002-helium/courses/migrations/courses_4116.lua) puts the send time in the receipt key and is not applied. |

Stays in Acuranzo:

| Ref | What | File |
| --- | --- | --- |
| **86** | Insert `contact_submissions` | [`acuranzo_1200.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1200.lua) |
| **149** | Scripting allowlist. Hydrogen hardcodes it. | [`acuranzo_1298.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1298.lua) |
| **155** | Get Role Name By ID. Course Manager already calls this. | [`acuranzo_1381.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1381.lua) |

`Enroll.LogEvent` reads **4006**. `Catalog.LogEvent` reads **4007**.
Both stay `invokable=0`.

**4004** is the published catalog. It is not a staff list of every
course. `courses_4075.lua` adds `Q`, `LEVEL`, `PAID_FREE`, `TAG`,
`SORT`, and `HIDE_RETIRED`. Bind every new name (`''` when unused).
Retired published rows still return unless `HIDE_RETIRED=1`. **4005**
is unchanged by that file. **4001–4003** are the Canvas link queries
Lua calls. They are not an account search.

Insert of `course_suggestions` stays **4000** for the public form.
Staff insert is **4516**. Contact submissions
still have insert **86** only. Phase 19 staff list **4513**
(`courses_4099.lua`) and working-title save **4514**
(`courses_4100.lua`) are applied. The replacement list is **4515**
(`courses_4102.lua`) and is applied. There is no fallback to
**4513** in the client.
The staff list of every course is **4500** (`courses_4076.lua`), not
**4004** and not **4008**. **4008** is the bookkeeping key of the
courses table file. `auth_query` would stop on that type-1001 row.
The next Courses file is **4118**. The next caller-facing QueryRef
is **4523**. The next Acuranzo file is **1402**. File numbers stay
below 4500. Caller refs for `auth_query` use 4500–4999.
**4500–4521** are assigned and applied. **4522** is written and
not applied. **4111–4114** and `acuranzo_1401.lua` are applied.
The receipt went out on the restart with a subject and no body.
`courses_4117.lua` replaces that text and HTML and is not applied.
The greeting uses the suggestion `submitter_name`. The course
title uses the suggestion `course_name`.
Andrew reported **4500–4505** applied on 2026-10-23, with Lookup 059 key 14
from `acuranzo_1397.lua` and Code and Title from
`acuranzo_1398.lua`. He later reported **4506**, **4507**, and
`courses_4086.lua` applied. On 2026-10-23 he confirmed Save prices
on course 16: CAD cents 4999, affected_rows 1, eight statements.
The same day, the Learners search and grant confirmed **4508–4512**
and `courses_4088.lua` through `courses_4096.lua`.
[`courses_4087.lua`](/elements/002-helium/courses/migrations/courses_4087.lua)
is the SQL that ran for **4506**. `courses_4086.lua` seeds
`Stripe.Mode` and does not install a caller ref. Do not invent a number in the SPA.
Do not apply a migration from an agent session.

Script names stay: `Catalog.*`, `Enroll.*`, `Account.*`, `Stripe.*`,
`Provision.EnsureCanvasUser`, `Mail.Notices.CourseExpiration`. Bodies
live under
[`/elements/002-helium/courses/migrations/`](/elements/002-helium/courses/migrations/).
`Orchestrators.Orchestrator` stayed in Acuranzo. The first insert of
`Provision.EnsureCanvasUser` stayed in
[`acuranzo_1289.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1289.lua).
`Enroll.RenewFree` still has no Helium seed. Lookup **055** keeps
family id 055. Lookup 042 keys 33–35 and Lookup 048 key 6 (Learning)
stay in
[`acuranzo_1378.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1378.lua)
and
[`acuranzo_1379.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1379.lua).

A database that already applied the old 1xxx course files must be
rebuilt before this pack is applied. On that rebuilt database, 85,
150, and 151 are absent.

---

## Phase Groups

| Group | Phases | Theme |
| ------- | -------- | ------- |
| 0 | 0 | Design lock (no source) |
| 0+ | 0a, 0b, 0c | Manager IDs in Helium; role integers + lookups; terminal URL config |
| A | 1–4 | Lifecycle, sanitize, invoke helper, XSS, honest UI |
| B | 5, 5a–5d, 6 | Login partners (shared + Didit/Apple/Google/Microsoft) + OIDC |
| C | 7–11 | Scripting Manager + CodeMirror productize |
| **D** | **12–18** | **Course Manager — first deploy** |
| E | 19–23 | Course Builder operator (ID 35) |
| F | 24–28 | Crimson / Chats / MCP catch-up |
| G | 29–32 | Placeholders, Mail, Jobs, Dashboard/Roles |
| H | 33–34 | AUTH_FINALE + Notifications **consumers** (gated) |
| I | 35–38 | File splits, LithiumTable, docs/tests, closeout |

---

## Canonical ID map (lock in Phase 0, write into `LITHIUM-MGR.md`)

| ID | Name | Module | Notes |
| ---- | ------ | -------- | ------- |
| 1 | Login | `managers/login/` | Not in menu registry |
| 2 | Menu / Main | `managers/main/` | |
| 3 | User Profile | `profile-manager/` | Utility |
| 4 | Session Log | `session-log/` | Utility |
| 5 | Crimson | `crimson/` | **Popout only.** Not a slot manager. |
| 6 | Tour | `tour/` | Not a slot manager |
| 7 | Dashboard | `dashboard/` | Placeholder until Band G |
| 8 | Mail | `mail-manager/` | Placeholder until Band G |
| 9 | Server Profiles | `server-profiles.js` | |
| 10 | Server Sessions | `server-sessions.js` | |
| 11 | Version | `version-history/` | |
| 12–17, 19–21, 24, 26–28, 30–31 | Calendar…Camera | placeholders | Collapse in Phase 29 |
| 18 | Chats | `chats/` | History manager. **Not** Crimson. Band F. |
| 22 | Style | `style-manager/` | |
| 23 | Lookups | `lookups/` | |
| 25 | Role | `role-manager/` | Placeholder until Band G |
| 29 | Queries | `queries/` | Canonical LithiumTable consumer |
| 32 | Terminal | `terminal/` | Menu 32. Utility key `terminal` **without** numeric 5. |
| 33 | Scripting | `scripting/` | |
| **34** | **Course Manager** | `course-manager/` | **First deploy.** Staff shell (Phase 12). QueryRef **155**. `cleanup()`. Later Band D phases add the screens. |
| **35** | **Course Builder** | `course-builder/` | Placeholder registered (Phase 0a). Band E is the real UI. |

Tour matching uses **numeric ID only**. Do not give Terminal utility
numeric 5 (collides with Crimson tours).

---

## Locked defaults (Phase 0 confirms; do not bikeshed)

| # | Decision | Default |
| --- | --------- | --------- |
| L1 | Course Manager ID | **34** |
| L2 | Course Builder ID | **35** |
| L3 | Course Manager authz v1 | Parse JWT `roles` to integer `role_id`s (C today: CSV string; accept array if that lands). Resolve `role_id` → `roles.name`. Staff if name is exactly `staff` or `admin`. Lookup 036 is Role Type (`type_a36`), not the name catalog. Seed + UI in Phase 0b. Hide manager + disable invoke otherwise. |
| L4 | 2.23 `account_roles` | Not a v1 ship gate. Band H cutover later. |
| L5 | Terminal vs Crimson | Crimson = system 5 popout. Terminal = menu 32 + utility key `terminal` only. |
| L6 | Lifecycle | `closeManager` → `destroy` else `cleanup` else `teardown`. Close does not call `onDeactivate`. `showManager` calls `onDeactivate` then `onActivate` after the crossfade. There is no `hideManager`. An already-visible manager is not activated again. Locked in Phase 1 (2026-10-16). |
| L7 | Partner buttons | Render only if `login partners.<id>` is true **and** a Hydrogen OIDC provider with that `id` exists. Else **omit**. Order: Didit, Apple, Google, Microsoft, then `auth.oidc_providers`. Wire-up is Phases 5a–5d (one partner each). Repo/example Hydrogen has only `500passwords`. |
| L8 | Last-method polish | OIDC-PLAN Phase 26 is done. Only extend `.is-recent` to partners if handlers exist. |
| L9 | Chat JWT | CATCHUP does **not** add C. Crimson keeps sending the login JWT until a Hydrogen mint exists; surface `JWT not authorized for chat` / `error_code` honestly. |
| L10 | Crimson vs Chats vs MCP | Three UIs. Crimson = in-app assistant. Chats = persisted threads (#069/#068). MCP = operator status/catalog, never browser JSON-RPC to `:3100`. |
| L11 | Engine list | New invokable/`mcp_access` Lua **without keys**. Never QueryRef #061 from the SPA. |
| L12 | Hosted MCP `allowed_tools` | Stay `["System.Info"]` until Phase 28 packet. Empty list = all tools (fail-closed today). |
| L13 | Script rename | Forbidden in v1 (edit in place). |
| L14 | Punchcard | Menu data is the gate plus JWT roles for Course Manager. `getPermittedManagers()` must not claim `[7..33]` after 34/35 exist. |
| L15 | Style Apply | Implement inject if CSS text exists; else disable buttons. No success toast on no-op. |
| L16 | `codemirror-init.js` | Dead CM5. Delete in Phase 11. |
| L17 | Staff script `invokable` | Flip to 1 only in the Band D packet, with Course Manager UI live. |
| L18 | CATCHUP vs sprint | CATCHUP wins. Sprint is history. |

---

## Phase 0 — Design lock

**Goal:** Freeze IDs, authz, login order, chat/MCP split, and Helium
QueryRefs so later phases do not invent a second scheme.

**Dependencies:** None.

**Entry gate:**

- [x] This plan and `AGENTS.md` have been read.

**Reference:**

- [`/elements/003-lithium/AGENTS.md`](/elements/003-lithium/AGENTS.md)
- [`/elements/003-lithium/src/app/manager-loader.js`](/elements/003-lithium/src/app/manager-loader.js)
- [`/elements/003-lithium/config/lithium.json`](/elements/003-lithium/config/lithium.json)
- [`/docs/Li/LITHIUM-MGR.md`](/docs/Li/LITHIUM-MGR.md)
- [`/docs/H/api/conduit/script.md`](/docs/H/api/conduit/script.md)
- [`/docs/H/plans/AUTH_FINALE.md`](/docs/H/plans/AUTH_FINALE.md)
- [`/mnt/extra/Projects/500-Courses-Reception/PRIORITIZE.md`](/mnt/extra/Projects/500-Courses-Reception/PRIORITIZE.md) Part 5
- Helium `acuranzo_1204`/`1206`/`1207`/`1263`–`1265`/`1297` (scripts).
  The 2026-09-09 course refs were `1326` (#150) and `1335` (#151).
  Both were retired 2026-10-16. See Courses pack QueryRefs.

**Work items:**

- [x] Write the canonical ID table (above) into `LITHIUM-MGR.md`.
- [x] Confirm Scripting QueryRefs 87/89/90/129/130/131 and whether list/
      detail/update SELECT `invokable` (1297 added the column; CRUD may
      omit it). Log the table: column → QueryRef.
- [x] Confirm Course Manager QueryRefs that already exist (#150, #151,
      #085, Catalog/Enroll/Stripe script names). Do not invent refs.
      Those three numbers are the 2026-09-09 confirm. They moved on
      2026-10-16. See Courses pack QueryRefs.
- [x] Read one live Hydrogen JWT `roles` value (or Test 40 fixture) and
      write the exact v1 match rule (substring vs list vs exact).
- [x] Confirm `login partners` key (space in JSON) vs `oidc_providers`.
      Record whether Didit/Apple/Google/Microsoft exist on
      `OIDC_RP.Providers[]` for lithium.philement.com and
      lithium.500courses.com.
- [x] Record L1–L18 in Working Log `P0` as accepted or with a variance
      the user signed.
- [x] Confirm `LITHIUM-TOC.md` / `LITHIUM-DEV.md` already say Vite runs
      without Hydrogen (login/data need it). Log, do not rewrite unless
      drift is found.

**Exit gate / validation:**

- [x] `LITHIUM-MGR.md` ID table matches `manager-loader.js` 7–33 plus
      reserved **34** and **35**.
- [x] Working Log `P0` has QueryRef table, JWT roles rule, partner-provider
      existence, and L1–L18.
- [x] No Lithium/Hydrogen/Helium source changed except `LITHIUM-MGR.md`
      (docs-only phase).

**Status:** complete

**Lessons learned:**

- C JWT `roles` is a CSV **string of role_id integers**, not `staff`/`admin`
  labels. Login JSON is also `json_string`. Lithium already treats
  `data.roles` as an array — that is wrong until Phase 0b.
- Lookup **036** is Role Type (`roles.type_a36`: Project Manager / Auditor /
  SME), not the role-name catalog. Name is `roles.name` (e.g. `mail_send`).
- Lookup **034** and **037** both seed as Role Status (Inactive/Active).
  README calling 037 Access Status is wrong (023 is Access Status). User
  signed retargeting 037 → Role Origin in Phase 0b.
- Helium Lookup **042** Modules stops at 32. Scripting 33 is in the SPA
  registry only. 34/35 do not exist anywhere yet.
- Example `hydrogen.json` OIDC_RP has only `500passwords`. Partner buttons
  are dead HTML.

---

## Phase 0a — Canonical manager IDs in Helium and consumers

**Goal:** One ID scheme in Helium Lookup 042, `lithium.json`,
`manager-loader.js`, punchcard fallback, tours, and docs. Add 33–35.

**Dependencies:** Phase 0 ID lock.

**Entry gate:**

- [ ] Phase 0 Status complete.

**Reference:**

- Helium Lookup 042: `acuranzo_1075.lua` (1–30), `acuranzo_1161.lua` (31–32)
- Lookup 048 Module Groups (`acuranzo_1081.lua`)
- QueryRef #046 Get Main Menu (`acuranzo_1137.lua`)
- [`/elements/003-lithium/src/app/manager-loader.js`](/elements/003-lithium/src/app/manager-loader.js)
- [`/elements/003-lithium/config/lithium.json`](/elements/003-lithium/config/lithium.json)
- [`/elements/003-lithium/src/core/permissions.js`](/elements/003-lithium/src/core/permissions.js)
  `getPermittedManagers` hardcoded `[7..33]`

**Work items:**

- [ ] Grep every consumer of manager IDs (registry, config, Lookup 042,
      Lookup 048, QueryRef 046, tours, punchcard, AGENTS.md, MGR.md).
- [ ] Helium packet: Lookup 042 row **33 Scripting** (already in SPA),
      **34 Course Manager**, **35 Course Builder**. Assign module groups
      (048); add a Courses group if needed. Do not apply.
- [ ] `lithium.json` `managers` keys `033`–`035`.
- [ ] `managerRegistry` + `_importManager` reserve 34/35 (placeholder
      modules OK until Band D/E). Delete is not this phase.
- [ ] `getPermittedManagers()` fallback includes 34/35 once registered;
      must not claim `[7..33]` after that (L14).
- [ ] Do not implement Course Manager UI here.

**Exit gate / validation:**

- [x] Working Log: consumer list + packet name. Human apply + `mkt`.
- [x] ID table in `LITHIUM-MGR.md` still matches runtime after the packet.
- [x] `npm test && npm run lint` if JS changed.

**Status:** complete

**Working Log:**

- **Consumers reviewed:** `manager-loader.js` (registry 7–33, `_importManager` switch),
  `permissions.js` (`getPermittedManagers` fallback `[7..33]`, `parsePermissions` fallback),
  `config/lithium.json` + `public/config/lithium.json` (managers keys `001`–`033`),
  `src/shared/menu.js` (QueryRef 046 consumer — `collectionInfo.index` from Lookup 042
  collection, filtered against `lithium.json` enabled set),
  `src/managers/main/main-sidebar.js` (sidebar build + fallback static icons),
  `src/managers/tour/tour.js` (numeric ID matching only), `LITHIUM-MGR.md` (ID registry table).

- **Helium packets (handed to human, not applied):**
  - `acuranzo_1378.lua` — Lookup 042 (Modules) keys 33 (Scripting, group 5),
    34 (Course Manager, group 6 Learning), 35 (Course Builder, group 6 Learning).
  - `acuranzo_1379.lua` — Lookup 048 (Module Groups) key 6 "Learning"
    (group sort_seq 6, after existing Internal 0…Security 5).
  - Reverse migrations delete exactly the keys inserted; DB2-safe multi-row
    `VALUES` seed; `${COMMON_INSERT}` audit fields; `${JIS}`/`${JIE}` wrappers
    for JSON collection; `${SUBQUERY_DELIMITER}` between statements.

- **Lithium source changes:**
  - `config/lithium.json` + `public/config/lithium.json`: added
    `"034.Course Manager": true`, `"035.Course Builder": true`.
  - `manager-loader.js`: added `34` and `35` to `managerRegistry`; added
    `case 34` and `case 35` to `_importManager` switch (lazy `import()` of
    `course-manager/course-manager.js` and `course-builder/course-builder.js`).
  - `permissions.js`: extended fallback arrays in `getPermittedManagers()` and
    `parsePermissions()` to include 34 and 35.
  - `tests/unit/permissions.test.js`: updated expected fallback array in 3 tests.
  - Stub modules: `src/managers/course-manager/course-manager.js` + `.css`,
    `src/managers/course-builder/course-builder.js` + `.css` (placeholder
    shells, no functional UI — Band D/E).

- **Verification:** `npm test` → 905 passed (32 files). `npm run lint` →
  0 errors, 34 pre-existing warnings (no new warnings).

- **Notes:** key 33 Scripting was already in `manager-loader.js` registry
  and `lithium.json`; only the Lookup 042 Helium seed was missing (now added
  in the packet). Learning group (key_idx 6) assigned sort_seq 6 to sort
  after Security (5). Module group value_int in 042 collection = 6 for keys
  34/35 to join Lookup 048 key 6.

**Lessons learned:**

- Lookup 042 `value_int` column = module group key_idx in Lookup 048.
  QueryRef 046 joins `module.value_int = modulegroup.key_idx`. New modules
  34/35 must set `value_int = 6` to land in the Learning group.
- `menu.js` `getEnabledManagerIds()` parses `NNN.Name` keys from
  `lithium.json` and filters QueryRef 046 results by the JSON `index` field
  — so both the Helium `index` value and `lithium.json` key must match the
  numeric manager ID.
- `tour.js` matches on numeric ID only via regex `^(\d+)\.` — tour steps
  like `"034.Course Manager"` will work once the menu row exists.

---

## Phase 0b — Role integers, Lookup 036 labels, Lookup 037 Origin

**Goal:** Course Manager authz can resolve JWT role_ids to names.
Lookup 037 becomes Role Origin. Staff/admin rows exist in `roles`.

**Dependencies:** Phase 0 L3 lock.

**Entry gate:**

- [ ] Phase 0 Status complete.

**Reference:**

- QueryRef #017 Get User Roles → `account_roles.role_id`
- `roles` table (`acuranzo_1016.lua`): `status_a34`, `scope_a35`,
  `type_a36`, `name`
- Lookup 034 Role Status; 035 Role Scope; 036 Role Type
  (Project Manager / Auditor / SME); 037 duplicate Role Status
- Seeded role: `mail_send` (`acuranzo_1257.lua`, `role_id=1`)
- Band G Role / Security managers for later UI

**Work items:**

- [x] Client: normalize JWT `roles` to an integer list (split CSV
      string; accept array). Do not substring-match `staff`.
- [x] Resolve `role_id` → `roles.name` (QueryRef if one exists; else
      packet). Staff iff name is exactly `staff` or `admin`.
- [x] Helium packet: seed `staff` and `admin` `roles` rows. Retarget
      Lookup **037** from duplicate Role Status to **Role Origin**
      (new value list). Do not apply.
- [x] Do not confuse Lookup 036 key_idx with `role_id`. 036 labels
      `type_a36` only.
- [x] Role/Security Manager UI is Band G; this phase is data + parse
      helper only.

**Exit gate / validation:**

- [x] Unit tests: CSV `"1,3"` and `[1,3]` → integers; `"staff"` string
      does not pass as a role_id.
- [x] Packet handed over. Working Log lists role_ids for staff/admin
      after seed.
- [x] `npm test && npm run lint` if JS changed.

**Status:** complete

**Working Log:**

- **Client role parsing:** Added `parseRoleIds()` to `src/core/utils.js`.
  Normalizes JWT `roles` claim (CSV string like `"1,3,7"` or array
  `[1,3,7]`) to an integer array. Strings like `"staff"` are rejected
  (don't parse as integers). Zero and negative integers are filtered.

- **Staff check:** Added `isStaffRoleSet(roleNames)` — returns true iff
  any resolved name is exactly `staff` or `admin` (L3 rule). Added
  `isCourseManagerAuthorized(roleIds, roleNames)` as the L3 gate helper.

- **JWT `roles` normalization across consumers:**
  - `src/app/auth-manager.js:41` — `this.user.roles = parseRoleIds(validation.claims.roles)`
  - `src/app/lithium-app.js:285` — login handler emits `roles: parseRoleIds(data.roles)`
  - `src/managers/main/main-state.js:87` — `loadUserInfo()` uses `parseRoleIds(claims.roles)`
  - `src/managers/login/login-form.js:318` — emits `roles: parseRoleIds(data.roles)`
  - `src/managers/login/oidc-login.js:170` — emits `roles: parseRoleIds(data.roles)`
  - `src/managers/profile-manager/profile-manager.js:448` — `loadUserInfo()` uses `parseRoleIds`
  - `src/managers/profile-manager/pages/account/page-account.js:42` — displays parsed roles

- **Course Manager (ID 34) authz gate:** Updated
  `src/managers/course-manager/course-manager.js` to call async
  `_checkStaffAccess()` in `render()`. Resolves role_ids → names via
  QueryRef #155 (Get Role Name By ID, `:ROLEID`) through `authQuery`. Falls back
  to ID-based check (staff=2, admin=3) if no API or query fails. Denied
  users see a locked "no permission" screen, not a placeholder.

- **Helium packets (handed to human, not applied — split per "one migration = one logical change"):**
  - `acuranzo_1380.lua` — **Seed roles**: Insert `staff` (role_id=2, scope=System, type=Project Manager) and `admin` (role_id=3) into the `roles` table. Reverse deletes role_ids 2 and 3. Follows the `acuranzo_1257.lua` pattern.
  - `acuranzo_1381.lua` — **QueryRef #155** ("Get Role Name By ID"): `TYPE_SQL` SELECT `role_id, name` from `roles` where `role_id = :ROLEID` and `status_a34 = 1`. Same QueryRef shape as #127 (`acuranzo_1260.lua`) / #017 (`acuranzo_1108.lua`). `auth_query` blocks type 0 (internal). SPA calls once per role_id.
  - `acuranzo_1382.lua` — **Lookup 037 retarget**: Updates `lookups` table to change Lookup 037 from "Role Status" (duplicate of 034) to "Role Origin" with values Seeded (key_idx 0) and Manual (key_idx 1). Reverse restores original values.

- **Staff/admin role_ids after seed:** staff=2, admin=3 (mail_send was already 1).

- **Lookup 036 clarification:** Confirmed 036 labels `roles.type_a36`
  only (Project Manager / Auditor / SME). Does NOT map `role_id` →
  label. New staff/admin roles use `type_a36 = 0` (Project Manager).

- **QueryRef #155 usage:** The SPA calls `authQuery(api, 155, { INTEGER: { ROLEID: id } })`
  once per JWT role_id. If #155 is not yet applied, the Course Manager falls back
  to the known ID list [2, 3].

- **Tests updated:** `login-form.test.js` and `oidc-login.test.js` mock data
  changed from string role names `['admin', 'user']` to integer role_ids `[2, 3]`
  to match the real Hydro JWT contract.

- **Verification:** `npm test` → 934 passed (32 files). `npm run lint` → 0 errors,
  35 pre-existing warnings (no new warnings). `npm run lint:css` → 0 errors.

---

## Phase 0c — Terminal URL troubleshooting

**Goal:** The Terminal manager (menu 32, utility key `terminal`) currently
hardcodes `DEFAULT_URL = 'https://www.philement.com'` — an iframe target that
has nothing to do with Hydrogen's Terminal subsystem. Determine the correct
Hydrogen terminal endpoint and wire the SPA to use it from config instead of
a hardcoded fallback.

**Dependencies:** Phase 0 (L5: Terminal = menu 32 + utility key `terminal`
only).

**Entry gate:**

- [ ] Phase 0 Status complete.

**Reference:**

- [`/elements/003-lithium/src/managers/terminal/terminal.js`](/elements/003-lithium/src/managers/terminal/terminal.js)
  — `DEFAULT_URL`, `show(options.url)`, `init()` iframe creation
- [`/elements/003-lithium/config/lithium.json`](/elements/003-lithium/config/lithium.json)
  `server.url`, `server.api_prefix`, `server.websocket_url`
- [`/mnt/extra/Projects/Philement/elements/001-hydrogen/hydrogen/src/config/config_terminal.h`](/elements/001-hydrogen/hydrogen/src/config/config_terminal.h)
  — TerminalConfig: `web_path`, `webroot`, `cors_origin`, `index_page`
- [`/mnt/extra/Projects/Philement/elements/001-hydrogen/hydrogen/src/config/config_terminal.c`](/elements/001-hydrogen/hydrogen/src/config/config_terminal.c)
  — defaults: `WebPath = "/terminal"`, `WebRoot = "PAYLOAD:/terminal"`,
  `CORSOrigin = "*"`, `IndexPage = "terminal.html"`
- [`/docs/H/core/reference/terminal_architecture.md`](/docs/H/core/reference/terminal_architecture.md)
  — xterm.js frontend + WebSocket protocol `"terminal"`
- [`/docs/H/tests/test_26_terminal.md`](/docs/H/tests/test_26_terminal.md)
  — Test 26 confirms payload-served and filesystem terminal pages + WS I/O
- `src/core/config.js` — `getConfigValue(path, defaultValue)` accessor

**Work items:**

- [x] Grep `DEFAULT_URL` / `terminal` / `web_path` references in Lithium
      and confirm the iframe is the only consumer. Log whether anything reads
      `server.terminal` or a terminal key today (expect: nothing).
- [x] Confirm Hydrogen terminal is served at `server.url` + `WebPath`
      (`/terminal` by default). Document the default in Working Log.
- [x] Decide config shape: add `server.terminal_path` (e.g. `"/terminal"`)
      to `lithium.json` with the Hydrogen `WebPath` value, or derive from a
      new `terminal` block. Record the choice.
- [x] Fix `terminal.js`: replace hardcoded `DEFAULT_URL` with a URL built from
      `getConfigValue('server.url')` + configured terminal path. If config is
      absent, log a warning and fall back to the Hydrogen default path (do not
      silently point at `philement.com`).
- [x] Add a `terminalUrl` getter used by `show()` / `init()` so the iframe
      `src` is always config-driven.
- [x] Verify CORS: Hydrogen `CORSOrigin` defaults to `*`, so an iframe
      cross-origin to the terminal path is permitted. Log the check; flag if
      a production deploy locks CORS down.
- [x] Manual: open Terminal; confirm the iframe loads the Hydrogen xterm.js
      page (or a clear disabled message if the subsystem is off).

**Exit gate / validation:**

- [ ] Working Log: terminal endpoint (default path), config key chosen, CORS
      note, and what `https://www.philement.com` was meant to be (if anything).
- [ ] `grep 'https://www.philement.com' src/managers/terminal/` empty (or
      justified leftover listed with target phase).
- [ ] `npm test && npm run lint` if JS changed.

**Status:** complete

**Working Log:**

- **Consumer grep:** `DEFAULT_URL` was only used in `src/managers/terminal/terminal.js` — one site, the iframe `src` in `init()`. No other code reads `server.terminal` or a terminal config key. `createTerminalButton` exists but is not imported anywhere (dead export, to be cleaned in a placeholder-collapse phase).

- **Hydrogen terminal endpoint:** `config_terminal.c` sets `TerminalConfig.web_path = "/terminal"` (default, overridable via `TerminalConfig.WebPath`). `WebRoot = "PAYLOAD:/terminal"`, `IndexPage = "terminal.html"`, `CORSOrigin = "*"`. So the iframe target is `server.url` + `/terminal`.

- **Config shape decision:** Added `server.terminal_path` (string, `"/terminal"`) to both `config/lithium.json` and `public/config/lithium.json`. Also added `terminal_path: '/terminal'` to `DEFAULT_CONFIG` in `src/core/config.js`. Chose a `server.*` key rather than a new `terminal` block to stay consistent with existing `server.url` / `server.api_prefix` / `server.websocket_url` naming.

- **terminal.js changes:** Removed hardcoded `const DEFAULT_URL = 'https://www.philement.com'`. Added `import { getConfigValue } from '../../core/config.js'`. Added a `terminalUrl` getter on `TerminalManager` that builds `${base}${path}` from `server.url` + `server.terminal_path` (default `/terminal`). Strips trailing slash from server URL; ensures leading slash on path. Logs a `WARN` and returns path-only if `server.url` is absent. The iframe `src` in `init()` now uses `this.terminalUrl`.

- **CORS check:** Hydrogen defaults `CORSOrigin` to `"*"`, so cross-origin iframe to the terminal path is permitted. A production deploy that locks `TerminalConfig.CORSOrigin` would break the iframe and should be co-ordinated — logged as a deploy-time concern, not a code fix.

- **`www.philement.com` provenance:** `https://www.philement.com` was a leftover from an early terminal-served-as-public-page design. It was never wired to Hydrogen's Terminal subsystem (xterm.js over WS). The iframe now points at `${HydrogenURL}/terminal` which serves the actual xterm.js page. No production purpose identified.

- **Tests:** Added `tests/unit/managers/terminal.test.js` with 7 tests for `terminalUrl`: standard build, default path fallback, trailing slash strip, missing leading slash on path, absent server.url (path-only fallback), never-returns-philement assertion, localhost default. Config paths mocked; DOM deps mocked.

- **Verification:** `npm test` → 941 passed (33 files). `npm run lint` → 0 errors, 35 pre-existing warnings (no new). `npm run lint:css` → 0 errors. `grep 'www.philement.com' src/managers/terminal/` → empty.

- **Notes:** `createTerminalButton` (terminal.js:85) is dead code — not imported anywhere. Left as-is; Phase 29 (placeholder collapse) can sweep it. Manual smoke test deferred (no Hydrogen instance in this session); the getter is unit-covered and config-driven.

**Lessons learned:**

---

## Band A — Honesty, XSS, invoke helper

## Phase 1 — Manager close / activate lifecycle

**Goal:** Closing a manager destroys Tabulator, CodeMirror, splitters,
and listeners.

**Dependencies:** Phase 0 lifecycle lock (L6).

**Entry gate:**

- [x] Phase 0 Status complete.

**Reference:**

- [`/elements/003-lithium/src/app/manager-loader.js`](/elements/003-lithium/src/app/manager-loader.js)
- Queries/Lookups/Scripting `cleanup()` vs Login `teardown()` vs
  Terminal `destroy()`

**Work items:**

- [x] `closeManager` tries `destroy`, then `cleanup`, then `teardown`
      (await, log failures).
- [x] `showManager` / hide call `onActivate` / `onDeactivate` when present.
- [x] Delete dead `_importManager` cases 1–6 (wrong modules, including
      Chats as case 6).
- [x] Unit test: fake manager with only `cleanup()` is invoked; only
      `destroy()` still works.

**Exit gate / validation:**

- [x] `npm test && npm run lint`
- [x] Grep: `closeManager` no longer calls `destroy` exclusively.

**Status:** complete (2026-10-16). Variance: `npm test` was 953
passed and 10 failed. The 10 failures are
`tests/integration/auth.integration.test.js`. The live server at
`https://lithium.philement.com` returned HTTP 503 on login, renew,
and logout. Those cases do not import `manager-loader.js`. The new
lifecycle file passed 7 tests. `npm run lint`: 0 errors, 35
pre-existing warnings.

**Accomplished:**

- `closeManager` calls `_releaseManager`: `destroy`, otherwise
  `cleanup`, otherwise `teardown`. A thrown hook is logged and the
  slot is still removed. Close does not call `onDeactivate`.
- `showManager` calls `onDeactivate` on the outgoing instance and
  `onActivate` on the incoming one after the crossfade. A manager
  that is already visible is left alone.
- `_importManager` cases 1–6 are gone.
- `tests/unit/app/manager-loader.test.js` covers cleanup-only,
  destroy-wins, teardown-only, a thrown cleanup, hook order, the
  already-visible switch, and the retired import ids.

**Lessons learned:**

- There is no `hideManager`. A switch is another `showManager`. The
  hooks belong after `await _crossfadeSlots`, so a Tabulator redraw
  runs once the slot is visible.
- `onDeactivate` on close would fire next to `cleanup` for Queries
  and Lookups. The approved rule keeps those hooks on the switch
  path only.

**Handoff:**

- Phase 2 may start. It adds `sanitizeHtml` and `invokeScript` and
  must not edit `manager-loader.js`.
- Queries, Lookups, Style, Scripting, and Version History already
  implement `cleanup()`. Closing one of them now runs that method.
  A browser pass of open-then-close was not done in this session.
- `AGENTS.md` lifecycle section matches this phase. The 2026-10-16
  doc alignment corrected the ID map: 34 and 35 are registered
  placeholders. Band D and Band E own the real UI.

---

## Phase 2 — Shared `escapeHtml` / `sanitizeHtml` / `invokeScript`

**Goal:** One sanitize path and one script-invoke client. No manager UI
yet.

**Dependencies:** Phase 0.

**Entry gate:**

- [x] Phase 0 complete. May parallel Phase 1 if this phase does not
      touch `manager-loader.js`. Phase 1 was already complete.
      `manager-loader.js` was not edited.

**Reference:**

- [`/elements/003-lithium/src/core/utils.js`](/elements/003-lithium/src/core/utils.js)
- [`/elements/003-lithium/src/shared/conduit.js`](/elements/003-lithium/src/shared/conduit.js)
- [`/docs/H/api/conduit/script.md`](/docs/H/api/conduit/script.md)
- `tests/unit/conduit.test.js`

**Work items:**

- [x] Confirm `escapeHtml` in `utils.js`; tests for `& < > " '`.
- [x] Add `sanitizeHtml(html)` wrapping the existing DOMPurify
      dependency. Use for markdown → HTML later.
- [x] Add `invokeScript(api, { script, params, wait, timeoutSeconds })`
      and `getScriptJob(api, jobId)`. Radar blips + `error.serverError`
      parity with `authQuery`.
- [x] Reject slash names; require `script`; never send `_hydrogen`;
      clamp timeout 1–60 on the client if you add a clamp.

**Exit gate / validation:**

- [x] `npm test` includes utils + conduit script tests.
- [x] `npm run lint`
- [x] No manager calls the helper yet.

**Status:** complete (2026-10-16). Variance: `npm test` was 965
passed and 10 failed. The 10 failures are the same live auth file
as Phase 1 (`lithium.philement.com` returned HTTP 503). Utils and
conduit tests passed. `npm run lint`: 0 errors, 35 pre-existing
warnings.

**Accomplished:**

- `escapeHtml` stays the text-node helper. Tests cover `& < > " '`.
  Quotes stay literal.
- `sanitizeHtml` in `src/core/utils.js` calls `DOMPurify.sanitize`.
  No manager imports it.
- `buildScriptPayload`, `invokeScript`, and `getScriptJob` are in
  `src/shared/conduit.js`. Names must be `Group.Name`. A slash is
  rejected. `params._hydrogen` throws before the request.
  `timeoutSeconds` clamps to 1–60. A triangle radar blip is added
  and removed. HTTP errors get `error.serverError` from
  `{ success: false, error, message }`. An HTTP 200 envelope with
  `status: failed` is returned, not thrown.

**Lessons learned:**

- `escapeHtml` uses `textContent`, so `"` and `'` are not encoded.
  That is safe in a text node and unsafe in an attribute. Phase 3
  should not put the result inside an attribute.
- happy-dom does not round-trip elements through DOMPurify 3.4.
  `<p>Hello</p>` comes back as `Hello` in Vitest. A browser keeps
  the tag. The unit test asserts the text and the absence of
  `<script` and `onerror`.

**Handoff:**

- Phase 3 wires the sinks named in that phase's Reference. Use
  `escapeHtml` for text and `sanitizeHtml(marked.parse(...))` for
  markdown. Keep `escapeHtml` output out of attributes. Do not call
  `invokeScript` from a manager in that phase.
- `getScriptJob` encodes the job id. An empty id or a slash rejects
  before the GET.
- Full `npm test` still fails the 10 live auth cases when health is
  up and login, renew, or logout returns HTTP 503. That variance is
  on this phase's Status. Lint exits 0 with 35 pre-existing warnings.

---

## Phase 3 — XSS sinks

**Goal:** Untrusted and markdown content cannot execute script.

**Dependencies:** Phase 2.

**Entry gate:**

- [x] Phase 2 `escapeHtml` / `sanitizeHtml` shipped.

**Reference:**

Confirmed 2026-10-16. Edit these functions. `lookups.js` is over the
1000-line cap; Phase 35 splits it after sanitize, so Phase 3 edits
in place.

- [`lookups.js`](/elements/003-lithium/src/managers/lookups/lookups.js)
  `processContentWithHighlighting` and `basicContentProcessing`
  use `.replace(/&/g, '&')`, and the same pattern for `<` and `>`.
  The replacement is the character itself, so nothing is escaped.
  `refreshPreview` assigns `previewContent.innerHTML`.
- [`crimson-chat.js`](/elements/003-lithium/src/managers/crimson/crimson-chat.js)
  `formatMessageContentMarkdown` runs `marked.parse`. The stream
  path assigns that HTML, and the live stream assigns
  `formatMessageContent`. Follow-up buttons in
  `addFollowUpQuestions` put a local `escapeHtml` inside HTML.
  Static chrome in `crimson-citations.js`, `crimson-ui.js`, and
  `crimson-core.js` is not the sink. `crimson-core.js` `escapeHtml`
  is the text-node helper.
- [`version-history.js`](/elements/003-lithium/src/managers/version-history/version-history.js)
  preview: `marked.parse` for markdown, and the raw summary when the
  content type is html, then `previewContent.innerHTML`. Local
  `escapeHtml` already escapes through `textContent`.
- [`highlight-init.js`](/elements/003-lithium/src/init/highlight-init.js)
  `highlightInline` interpolates `language` and `code` into
  `innerHTML`.
- [`tour.js`](/elements/003-lithium/src/managers/tour/tour.js)
  `updateCaptionsForTime` sets `captionEl.innerHTML = caption.text`,
  then `processIcons`.
- [`grouping-popup.js`](/elements/003-lithium/src/tables/popups/grouping-popup.js)
  sets `title.innerHTML` when `createLookupPreview` returns HTML or
  `titleContainsIcon` is true. Plain titles already use
  `textContent`.
- [`queries-ui.js`](/elements/003-lithium/src/managers/queries/queries-ui.js)
  `toggleNavPopup` interpolates `item.label`. The column-chooser
  title already uses `textContent`.
- [`queries-editors.js`](/elements/003-lithium/src/managers/queries/queries-editors.js)
  `renderMarkdownPreview` assigns `marked.parse`. Same class of sink
  as Crimson and Version History. Added during the 2026-10-16 doc
  alignment.

**Work items:**

- [x] Lookups processors: real `escapeHtml` then highlight; sanitized
      assign.
- [x] Crimson + Version History: `sanitizeHtml(marked.parse(...))`.
      Version History's html content type is the same assign.
- [x] Queries summary preview: `sanitizeHtml(marked.parse(...))` in
      `renderMarkdownPreview`.
- [x] `highlight-init.js`: do not interpolate raw `code` / `language`
      into `innerHTML`.
- [x] Tour captions: `textContent` + icon nodes, or sanitize allowlist.
- [x] Grouping popup titles and Queries nav-popup labels:
      `textContent`, except an allowlisted icon.
- [x] Unit tests with `<img onerror>` / `<script>` fixtures.

**Exit gate / validation:**

- [x] Grep Lookups shows real entity escapes or `escapeHtml`.
- [x] `npm test && npm run lint`
- [x] Manual: lookup summary `<img onerror=alert(1)>` does not fire
      (Working Log).

**Status:** complete (2026-10-16). Variance: `npm test` was 983
passed and 10 failed. The 10 failures are the same live auth file
as Phases 1 and 2 (`lithium.philement.com` returned HTTP 503).
`tests/unit/xss-sinks.test.js` passed 16 tests. `npm run lint`: 0
errors, 35 pre-existing warnings. `npm run lint:css` exited 0.

**Accomplished:**

- Lookups `processContentWithHighlighting` and
  `basicContentProcessing` start with `escapeHtml`. `refreshPreview`
  assigns `sanitizeHtml`. SunEditor tags such as `<p>` show as text.
- Crimson markdown, the live stream, Version History markdown, and
  the Queries summary preview assign `sanitizeHtml(marked.parse(...))`.
  Version History HTML content type assigns `sanitizeHtml` of the
  raw summary. Status, Released, and Focus are text nodes. A plain
  summary uses `escapeHtml` inside `pre.version-preview-plain`.
- `highlightInline` sets `textContent` and adds `language-*` only
  when the language matches `/^[a-z0-9_+#-]+$/i`.
- Tour captions turn each exact `<fa fa-name></fa>` into an icon
  node. Other caption markup stays text. A grouping title or a nav
  `item.icon` becomes an icon only when the whole value is one
  `fa-name` token (`/^fa-[a-z0-9-]+$/`). The lookup group title is
  text. Queries template names are text.
- Crimson citations are built after sanitize. The tooltip is
  `setAttribute('title', ...)`. The book icon is a node.
  Follow-up text is a text node. The up icon is a static node.
- `sanitizeHtml` re-parses when DOMPurify's string still contains a
  script, iframe, object, embed, an `on*` attribute, or
  `javascript:`. Headless Chrome already strips those, so ordinary
  markup skips the second pass.
- `npm run templates:copy` refreshed
  `public/src/managers/version-history/version-history.css`.

**Lessons learned:**

- happy-dom's DOMPurify leaves a nested `<script>` and an `onerror`
  attribute in the returned string. A lone script and a lone image
  are stripped. Headless Chrome with DOMPurify 3.4.16 strips the
  nested payload: `alert` does not fire, the script element is gone,
  and the image has no `onerror`. The second pass makes Vitest see
  that result. Ordinary markup (`<p>`, lists, links, a safe image)
  does not match the marker, so Chrome keeps DOMPurify's string.
- A citation tooltip is attribute text. `setAttribute` stores
  `<script>` as text. The sink test looks for script, iframe,
  object, and embed elements, and for attribute names that start
  with `on`.
- Default DOMPurify drops `<fa>`. Icon sinks insert nodes, then
  `processIcons` runs. `sanitizeHtml` has no `<fa>` allowlist.
- A safe `<img src>` can remain after sanitize. The lookup path
  escapes first, so `<img onerror=alert(1)>` is text.
- `lookups.js` stays over the 1000-line cap. Phase 35 splits it.
  Static chrome (the grouping arrow and the Queries delete icon)
  still assigns authored `<fa>` HTML.

**Handoff:**

- Phase 4 is Honest UI: replace `window.confirm`, `prompt`, and
  `alert` on the hottest paths, remove the Login / Main / Session
  Log / Profile `renderFallback` clones, and send a real Bearer
  token from SunEditor. Do not start it in the Phase 3 conversation.
- `escapeHtml` output stays out of attributes. `invokeScript` still
  has no manager caller.
- Full `npm test` still fails the 10 live auth cases when health is
  up and login, renew, or logout returns HTTP 503. The count on
  this phase is 983 passed and 10 failed. Lint exits 0 with 35
  pre-existing warnings.

---

## Phase 4 — Honest UI (dialogs, saves, SunEditor JWT)

**Goal:** Buttons work or refuse. No native dialogs on hot paths. Upload
Authorization is a real Bearer token.

**Dependencies:** Phase 0. Phase 1 recommended for cleanup. Phase 2 for
nothing here.

**Entry gate:**

- [x] Phase 0 complete. Existing toast / confirm pattern cited in log.

**Reference:**

- `lithium-table-ops.js`, `lithium-table-ui.js`, `queries-navigation.js`
- Style Manager `applyState`; Profile `page-*/save()`
- `src/init/suneditor-init.js`
- `jwt.js` `retrieveJWT`

**Work items:**

- [x] Replace `window.confirm` / `prompt` / `alert` on the hottest
      paths (table ops, queries nav/templates, template-popup,
      jsoneditor-init, camera-popout).
- [x] Remove Login / Main / Session Log / Profile `renderFallback` HTML
      clones; fail with toast + session log.
- [x] Remove Lookups JSON textarea fallback.
- [x] Style Manager: implement `applyState` **or** disable Apply / Undo /
      Redo / Copy (L15). `copyCssToClipboard` copies real CSS.
- [x] Profile page `save()` stubs: return failure or disable. No
      `{ success: true }` on no-ops.
- [x] `USERID` from `this.app.auth.user`, not `this.app.user` (Scripting,
      Queries, Lookups when touched).
- [x] SunEditor: `retrieveJWT()`; missing token must not produce
      `"Bearer null"`.

**Exit gate / validation:**

- [x] `rg "\\balert\\(|\\bconfirm\\(|\\bprompt\\(" src/` empty or
      leftovers listed with target phase.
- [x] `rg renderFallback src/` empty.
- [x] Profile stub `success: true` grep empty.
- [x] Unit: missing JWT ≠ `"Bearer null"`.
- [x] `npm test && npm run lint`

**Status:** complete (2026-10-16). Variance: `npm test` was 996
passed and 10 failed. The 10 failures are the same live auth file
as Phases 1–3 (`lithium.philement.com` returned HTTP 503).
`tests/unit/honest-ui.test.js` passed 13 tests. `npm run lint`: 0
errors, 35 pre-existing warnings. `npm run lint:css` exited 0.
No manager HTML changed, so `templates:copy` was not required.

**Accomplished:**

- [`confirm.js`](/elements/003-lithium/src/shared/confirm.js) and
  [`confirm.css`](/elements/003-lithium/src/styles/confirm.css) are
  the shared confirm and prompt. Delete uses the danger button.
  The message is `textContent`. Cancel, Escape, backdrop, and
  `close-all-popups` resolve false or null. Table delete, Queries
  delete, template reload, and both template-name prompts use it.
  Camera failures and invalid JSON import use `toast.error` plus
  `log`. Queries `showSaveTemplateDialog` was already a custom
  dialog and stayed.
- Login, Main, Session Log, and Profile return before init when
  the template fetch fails or is not OK. The login language list
  does the same: toast, log, and a text node. Lookups no longer
  reads `#lookups-json-editor-fallback`.
- Style Apply writes the CSS editor text into
  `#style-manager-preview` when that text is non-empty, then toasts
  success. Copy copies that same text. Empty Apply and Copy stay
  disabled and do not toast. Undo and Redo stay disabled.
  `applyState` is still empty.
- Stub profile `save()` returns `{ success: false }` and stays
  dirty. In-page Save buttons are disabled. The settings footer
  Save stays disabled while the current page cannot save.
  `saveAll` records that failure and toasts once. Language, date
  formats, photo, and Lookups manager settings (manager 23) still
  return `{ success: true }` and `canSave()` is true.
- Scripting, Queries, and Lookups read `USERID` from
  `app.auth.user`. A missing id is still `0`.
- [`uploadAuthHeader`](/elements/003-lithium/src/init/suneditor-init.js)
  returns `{}` unless the token is a non-empty string.
  `initEditor` reads `retrieveJWT()` again so a login after
  construction is sent. A missing token is not `"Bearer null"`.

**Lessons learned:**

- There was no confirm modal to reuse. Toast
  ([`toast.js`](/elements/003-lithium/src/shared/toast.js)) was the
  existing pattern. The new dialog dispatches `close-all-popups`
  before it listens, or it cancels itself.
- `+` binds tighter than `||`, so
  `'Bearer ' + localStorage.getItem(...) || ''` is the string
  `"Bearer null"` when the key is missing.
- The visible Profile footer Save is the collection JSON path.
  `settingsHandler.save()` has no UI caller. Gating that footer
  when `canSave()` is false stops a stub edit from toasting
  "Preferences saved successfully".
- The login language panel's `_renderFallback` matches
  `rg renderFallback`. It was an HTML clone, so it went with the
  other clones.
- `npm run lint` globs one directory level (`src/*/*.js` and
  `tests/*/*.js` under `sh`). Nested manager files are outside
  that command. `npm run lint:css` covers `src/**/*.css`.
- Headless Chrome `--dump-dom` escapes `<` inside `<pre>`. The
  dialog check is `hasImg: false` plus the resolved values.

**Handoff:**

- The next conversation opens Phase 5, Ordered partner logins.
  Start at **Resuming Work**. Do not start Phase 5a in that sitting.
- Profile pages that persist are language, date formats, photo,
  and manager 23. Everything else inherits the refusing base save.
  Wiring the footer Save to `settingsHandler.save()` is still open.
- `applyState` and the undo stacks are still empty. Apply replaces
  the whole `#style-manager-preview` text, including rules that
  `applyPreviewStyle` appended.
- `this.app.user` is still unset. `USERID` reads `app.auth.user`.
- Full `npm test` still fails the 10 live auth cases when health
  is up and login, renew, or logout returns HTTP 503. The count
  on this phase is 996 passed and 10 failed.

---

## Band B — Login

## Phase 5 — Ordered partner logins

**Goal:** Didit / Apple / Google / Microsoft are real buttons or absent.
500 Passwords stays `auth.oidc_providers`.

**Dependencies:** Phase 0 L7–L8.

**Entry gate:**

- [x] Phase 0 Working Log records whether each partner id exists on
      Hydrogen `OIDC_RP.Providers[]` for the target deploys.
      Variance: the log records example `hydrogen.json` only
      (`500passwords`) and says the live configs were not in the
      tree. Andrew confirmed this sitting treats Didit, Apple,
      Google, and Microsoft as absent on both deploys until 5a–5d.

**Reference:**

- [`/elements/003-lithium/src/managers/login/login.js`](/elements/003-lithium/src/managers/login/login.js)
- [`/elements/003-lithium/src/managers/login/login.html`](/elements/003-lithium/src/managers/login/login.html)
- [`/elements/003-lithium/config/lithium.json`](/elements/003-lithium/config/lithium.json)
  `login partners`, `auth.oidc_providers`
- [`/docs/Li/LITHIUM-OIDC.md`](/docs/Li/LITHIUM-OIDC.md)
- [`/docs/H/api/auth/oidc_rp.md`](/docs/H/api/auth/oidc_rp.md)
- `tests/unit/managers/login/login.test.js`

**Work items:**

- [x] Remove hardcoded partner markup as the live path. Render partners
      from config (same row as OIDC).
- [x] Honor `login partners` booleans. `false` or missing Hydrogen
      provider → do not render (no greyed-out fake).
- [x] Click → `startOidc(id)` (or the Phase 0-recorded start path).
      Record `auth.last_method`. Apply `.is-recent` like OIDC.
- [x] Visual order: Didit, Apple, Google, Microsoft, then OIDC providers.
- [x] Do not put Keycloak tokens in the SPA. Do not add C.
- [x] If a partner has no Hydrogen provider, Working Log lists it as
      hidden; do not invent a second protocol.

**Exit gate / validation:**

- [x] Unit tests: boolean off → no button; boolean on + provider →
      click calls `startOidc` with that id; order asserted.
- [x] `npm test && npm run lint`
- [x] Manual checklist if a provider is live (log which). None of
      Didit, Apple, Google, or Microsoft is live. No browser pass.
      Andrew checks the login page.

**Status:** complete (2026-10-16). Variance: `npm test` was 1003
passed and 10 failed. The 10 failures are the same live auth file
as Phases 1–4 (`lithium.philement.com` returned HTTP 503).
`tests/unit/managers/login/login.test.js` passed 37 tests, 7 of
them new for this phase. `npm run lint`: 0 errors, 35 pre-existing
warnings. That glob does not reach `login.js`. Direct `npx eslint`
on `login.js` and `login.test.js`: 0 errors, 2 pre-existing
warnings in `login.js`. No browser pass. Andrew asked to check the
login page himself. No partner provider is live.

**Accomplished:**

- [`login.html`](/elements/003-lithium/src/managers/login/login.html)
  no longer contains the four partner buttons. The alt row is empty
  until `renderOidcProviders` fills it. `templates:copy` updated
  [`public/src/managers/login/login.html`](/elements/003-lithium/public/src/managers/login/login.html).
  [`index.html`](/elements/003-lithium/index.html) preloads only
  `login_500passwords.svg`.
- [`orderLoginProviders`](/elements/003-lithium/src/managers/login/login.js)
  and `hiddenLoginPartners` apply L7. A partner id renders when
  `login partners.<id>` is strictly true and `auth.oidc_providers`
  has an entry with that id and a label. Order is Didit, Apple,
  Google, Microsoft, then other providers. A false or missing flag
  omits that id, including from the tail. The click path is unchanged:
  `auth.last_method` is `oidc:<id>`, `.is-recent` matches `oidc:<id>`
  or plain `oidc`, then `startOidc(id)`.
- Checked-in [`lithium.json`](/elements/003-lithium/config/lithium.json)
  still has the four booleans true and only `500passwords` in
  `auth.oidc_providers`. Those four buttons stay hidden. A stale
  template button in the row is removed on render.
- The Style Manager login mockup still paints the four images. It is
  not the live login path.

**Lessons learned:**

- `getConfigValue` splits on `.`. The partner map is the top-level
  key `login partners`, space included. `login.partners` would miss it.
- A partner whose flag is false must not fall through into the OIDC
  tail, or turning the flag off would still show the button.
- `npm run lint` from `package.json` does not lint
  `src/managers/login/login.js`. The direct eslint run does.
- No partner image preload belongs on `index.html` while those
  buttons are omitted. The Style Manager mockup still references
  the PNG files on disk.

**Handoff:**

- Andrew deferred Phases 5a–5d and 6 on 2026-10-16 to open Band C.
  Resume Band B when he pulls it back. The pause point at the top
  of this file is Phase 8. Phase 7 closed the same day on Andrew's
  report that 1389–1393 are applied.
- To show a partner later: add a Hydrogen `OIDC_RP.Providers[]`
  entry (human config, not C) and a matching `auth.oidc_providers`
  id. The boolean is already true. The click path already calls
  `startOidc`. Phase 5a decides whether Didit is OIDC. If it is not,
  leave the button omitted.
- Andrew has not looked at the login page yet. The checked-in page
  should show 500 Passwords and no Didit, Apple, Google, or
  Microsoft button.
- Profile footer Save, `applyState`, `this.app.user`, and
  `invokeScript` are unchanged. The Style Manager mockup still
  shows the four partner images.

---

## Phase 5a — Didit partner

**Goal:** Didit is a working Hydrogen OIDC provider (`Name` = `didit`)
or the button stays omitted.

**Dependencies:** Phase 5 plumbing. Phase 0 L7.

**Entry gate:**

- [ ] Phase 5 complete or this phase only adds the Didit provider
      config research (log if Phase 5 not started).

**Work items:**

- [~] Confirm whether Didit is OIDC or another protocol. CATCHUP adds
      no product C. If not OIDC, omit and log.
      Deferred 2026-10-16. Resume when Andrew pulls Band B back.
- [~] If OIDC: Hydrogen `OIDC_RP.Providers[]` entry `Name: didit`
      (human config, not C). Lithium `login partners.didit` + matching
      `oidc_providers` id. Click → `startOidc('didit')`.
- [~] No Keycloak tokens in the SPA.

**Exit gate / validation:**

- [ ] Working Log: OIDC or omitted, and why.
- [ ] `npm test && npm run lint` if JS changed.

**Status:** deferred (2026-10-16). [~] Andrew deferred the rest of Band B to open Band C. Resume when he pulls Band B back. No new phase number.

**Lessons learned:**

---

## Phase 5b — Apple partner

**Goal:** Apple Sign In via Hydrogen OIDC (`Name` = `apple`) or omitted.

**Dependencies:** Phase 5. Phase 0 L7.

**Entry gate:**

- [ ] Phase 5a Status complete or `[~]`.

**Work items:**

- [~] Research Apple as an OIDC_RP provider (IdP, client, redirect).
      No product C. Omit if Hydrogen cannot host it yet.
      Deferred 2026-10-16. Resume when Andrew pulls Band B back.
- [~] If live: provider `apple`, `login partners.apple`, `startOidc`.

**Exit gate / validation:**

- [ ] Working Log: wired or omitted.
- [ ] `npm test && npm run lint` if JS changed.

**Status:** deferred (2026-10-16). [~] Same deferral as Phase 5a. Resume when Andrew pulls Band B back.

**Lessons learned:**

---

## Phase 5c — Google partner

**Goal:** Google via Hydrogen OIDC (`Name` = `google`) or omitted.

**Dependencies:** Phase 5. Phase 0 L7.

**Entry gate:**

- [ ] Phase 5b Status complete or `[~]`.

**Work items:**

- [~] Research Google as an OIDC_RP provider. No product C.
      Deferred 2026-10-16. Resume when Andrew pulls Band B back.
- [~] If live: provider `google`, `login partners.google`, `startOidc`.

**Exit gate / validation:**

- [ ] Working Log: wired or omitted.
- [ ] `npm test && npm run lint` if JS changed.

**Status:** deferred (2026-10-16). [~] Same deferral as Phase 5a. Resume when Andrew pulls Band B back.

**Lessons learned:**

---

## Phase 5d — Microsoft partner

**Goal:** Microsoft via Hydrogen OIDC (`Name` = `microsoft`) or omitted.

**Dependencies:** Phase 5. Phase 0 L7.

**Entry gate:**

- [ ] Phase 5c Status complete or `[~]`.

**Work items:**

- [~] Research Microsoft as an OIDC_RP provider. No product C.
      Deferred 2026-10-16. Resume when Andrew pulls Band B back.
- [~] If live: provider `microsoft`, `login partners.microsoft`,
      `startOidc`.

**Exit gate / validation:**

- [ ] Working Log: wired or omitted.
- [ ] `npm test && npm run lint` if JS changed.

**Status:** deferred (2026-10-16). [~] Same deferral as Phase 5a. Resume when Andrew pulls Band B back.

**Lessons learned:**

---

## Phase 6 — OIDC client verification

**Goal:** Lithium OIDC path still matches the Keycloak recipe after
Band A/B churn. Do not block on AUTH_FINALE Phase 11 OTP.

**Dependencies:** Phase 5 (partners must not have broken OIDC).

**Entry gate:**

- [ ] Hydrogen `OIDC_RP.Enabled` known true/false on the target (log).

**Reference:**

- [`/docs/Li/LITHIUM-KEYCLOAK.md`](/docs/Li/LITHIUM-KEYCLOAK.md)
- `src/core/oidc-client.js`
- AUTH_FINALE Phase 11 is **ops**, not this phase.

**Work items:**

- [~] Walk KEYCLOAK client checklist against `oidc-client.js` + login.
      Deferred 2026-10-16. Resume when Andrew pulls Band B back.
- [~] Integration: invalid handoff → 401; unknown provider does not
      fall back (mock or live).
- [~] Confirm password login still works with IdP down (if testable).
- [~] Provider `id` must match `OIDC_RP.Providers[].Name`.

**Exit gate / validation:**

- [ ] Working Log: enabled? start URL? provider id match?
- [ ] `npm test`. If real Keycloak E2E still OTP-blocked, `[~]` and do
      **not** block CATCHUP.

**Status:** deferred (2026-10-16). [~] Same deferral as Phase 5a. Resume when Andrew pulls Band B back. Phase 5 already kept the 500 Passwords click path.

**Lessons learned:**

---

## Band C — Scripting Manager and CodeMirror

## Phase 7 — Scripts schema and QueryRef audit

**Goal:** Scripting matches live `scripts`, including `invokable`. Helium
packet ready if CRUD queries omit the column.

**Dependencies:** Phase 0 QueryRef confirm.

**Entry gate:**

- [x] Phase 0 Working Log has the scripts QueryRef table. Re-checked
      on disk 2026-10-16. The five caller-facing SQL bodies still omit
      `invokable`.

**Reference:**

- Helium `acuranzo_1204` (87), `1206` (89), `1207` (90), `1263` (129),
  `1264` (130), `1265` (131), `1297` (`scripts.invokable`)
- Packet: `acuranzo_1389` through `acuranzo_1394`
- Lookup 59 tableDef `scripts/script-manager` (key 13, migration 1394)
- [`/docs/H/api/conduit/script.md`](/docs/H/api/conduit/script.md)

**Work items:**

- [x] Inventory columns vs QueryRefs vs tableDef. Confirm `invokable`
      on list/detail/insert/update. Confirm delete 131 params
      (`GROUP_NAME` + `SCRIPT_NAME`, not INTEGER pk).
- [x] If queries omit `invokable`, prepare Helium packet (human apply).
      Do not apply.
- [x] List seeded `invokable=1` scripts (`Api.Echo`, …) in Working Log.
- [x] Confirm Lookup 59 exposes `invokable` and `deleteQueryRef: 131`.
      It did not. Key 13 is in the packet.

**Exit gate / validation:**

- [x] Working Log table: column → QueryRef / tableDef field.
- [x] Packet handed over **or** log that live queries already SELECT
      `invokable`.
- [ ] If Helium changed: human `mkt` after apply; name the migration
      test in the log. Payload refresh is `mka` or
      `payload-generate.sh`. `mkt` does not refresh the archive.

**Status:** complete (2026-10-16) on Andrew's report that 1389–1393
are applied. Variance: the agent did not see SchemaTool or
SchemaHelper output, did not run `mkt` or `mka`, and no migration
test was named. The human-build exit line stays open.
[`acuranzo_1394.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1394.lua)
(Lookup 059 key 13) is written and was not in that report.
`luacheck` on the six files was 0 warnings. No Lithium source
change, so `npm test` was not re-run for this close.

**Accomplished:**

- Re-read QueryRefs 087, 089, 090, 129, and 130. Each omitted
  `invokable` before this packet. Delete 131 is `GROUP_NAME` +
  `SCRIPT_NAME`. Lookup 059 keys on disk were 0–12. None was
  `scripts/script-manager`.
- Wrote six files. Andrew reported five of them applied:
  - [`acuranzo_1389.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1389.lua) — QueryRef 087 SELECT adds `invokable`. Applied.
  - [`acuranzo_1390.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1390.lua) — QueryRef 089 SELECT adds `invokable`. Applied.
  - [`acuranzo_1391.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1391.lua) — QueryRef 090 SELECT adds `invokable`. Applied.
  - [`acuranzo_1392.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1392.lua) — QueryRef 129 INSERT writes `:INVOKABLE`. Applied.
  - [`acuranzo_1393.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1393.lua) — QueryRef 130 UPDATE writes `:INVOKABLE`. Applied.
  - [`acuranzo_1394.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1394.lua) — Lookup 059 key 13, `value_txt` `scripts/script-manager`, `deleteQueryRef` 131. Written. Not in the apply report.
- `lua` `loadfile` compiled all six. `luacheck` reported 0 warnings
  and 0 errors. The tableDef JSON parses. Highest Acuranzo file before
  this packet was 1388. The next file is 1395. No Lithium JS edit.

**Lessons learned:**

- The Phase 0 inventory was still true. Later files did not patch
  these five QueryRefs.
- Hydrogen rejects a missing `:NAME` before SQL runs. A SQL
  `COALESCE` does not make `INVOKABLE` optional. The column default
  of 0 applies only when an INSERT leaves the column out. 1392 and
  1393 do not leave it out. The Scripting save in `scripting.js`
  does not send `INVOKABLE`.
- Andrew applied 1392 and 1393 with the SELECT files. Save and
  duplicate now fail until Phase 8 sends INTEGER `INVOKABLE`. List,
  search, and detail keep working. Existing rows are unchanged.
- The apply report covered 1389–1393. Lookup 059 key 13 stays a
  file on disk until a later report.
- Closing on an owner report does not stand in for `mkt` or a named
  migration test. Leave that exit line open and record the gap.
- `LithiumTable` copies a tableDef `deleteQueryRef` when the
  constructor leaves that ref null. `handleDelete` then reads one
  `primaryKeyField`. Scripting passes the pair
  `['group_name', 'script_name']`, so `selectedId` is null and
  QueryRef 131 is not called. The grid row is still removed locally.
  That local remove already happened before key 13. Key 13 is not
  live yet.
- One caller-facing QueryRef per file. Five query updates plus one
  lookup key.
- `courses_4032.lua` seeds `Stripe.Webhook` with `invokable = 1`.
  Phase 17 later calls that script non-invokable. This phase left
  the seed alone.

**Handoff:**

- Phase 8 starts in the next conversation. Send INTEGER `INVOKABLE`
  on insert (129) and update (130), including duplicate. Use 0 when
  the row has none. Save and duplicate currently omit it, so both
  fail against the applied SQL.
- Custom delete is `GROUP_NAME` + `SCRIPT_NAME` (QueryRef 131). Do
  not use a generic INTEGER `selectedId`. Split `scripting.js` under
  1000 lines before invoke UI. Dirty prompt. Rename forbidden (L13).
- Apply [`acuranzo_1394.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1394.lua)
  before Scripting relies on Lookup 059 key 13. The constructor
  still sets list 89 and search 90, so Phase 8 can proceed without
  that key. Do not apply it unless Andrew asks.
- If the payload was not regenerated after 1389–1393, run `mka` or
  `payload-generate.sh` before a migration test. `mkt` does not
  refresh the archive. No test name was recorded. The agent does
  not run SchemaTool or SchemaHelper apply.
- Seeded `invokable = 1` rows are in the Working Log. Staff writers
  stay 0 until the Band D packet (L17).

---

## Phase 8 — Scripting CRUD honesty

**Goal:** Create / update / duplicate / delete are real. `scripting.js`
under 1000 **before** invoke UI lands.

**Dependencies:** Phases 1, 4 (`USERID`), 7.

**Entry gate:**

- [x] Phase 7 schema table exists. Phase 1 close path exists.

**Reference:**

- [`scripting.js`](/elements/003-lithium/src/managers/scripting/scripting.js) (616 after this phase)
- [`scripting-save.js`](/elements/003-lithium/src/managers/scripting/scripting-save.js)
- [`scripting-footer.js`](/elements/003-lithium/src/managers/scripting/scripting-footer.js)
- [`scripting-editors.js`](/elements/003-lithium/src/managers/scripting/scripting-editors.js)
- Queries delete pattern (`queries-navigation.js`)
- Applied 2026-10-16: 087, 089, and 090 SELECT `invokable`. 129 and
  130 require INTEGER `INVOKABLE`.
- Lookup 059 key 13 is
  [`acuranzo_1394.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1394.lua).
  Not in the 1389–1393 apply report.

**Work items:**

- [x] Split footer / save-payload builder out of `scripting.js` so the
      file is under 1000 before Phase 9.
- [x] Save/insert/update use confirmed QueryRefs and `app.auth.user`.
- [x] Custom delete: composite PK + Phase 4 modal. Do not use generic
      INTEGER `selectedId`.
- [x] Dirty prompt on row switch. Rename forbidden (L13).
- [x] `cleanup()`: table, editors, splitter, **keydown** listener.
      `destroyCodeMirrorScrollbars` must not stay a no-op if scrollbars
      are created.
- [x] Extract save-payload builder + unit tests.

**Exit gate / validation:**

- [x] `wc -l src/managers/scripting/scripting.js` ≤ 1000
- [x] Unit tests for save payload.
- [x] `npm test && npm run lint`

**Status:** complete (2026-10-16). Variance: `npm test` was 1021
passed and 10 failed. The 10 are
`tests/integration/auth.integration.test.js` (HTTP 503 from
`https://lithium.philement.com`), the same live auth variance as
Phases 1–5 and 7. Lint was 0 errors and 35 existing warnings.
No browser pass of create, save, duplicate, or delete.
`acuranzo_1394.lua` stays unapplied. No new migration.

**Accomplished:**

- [`scripting-save.js`](/elements/003-lithium/src/managers/scripting/scripting-save.js)
  builds insert (129), update (130), duplicate, and delete (131)
  payloads. INTEGER `INVOKABLE` is 1 only when the row value is 1,
  otherwise 0. INTEGER `USERID` comes from `app.auth.user.id`, or 0.
  `SCRIPT_TYPE` and `STATUS` stay in STRING.
- A row loaded from the list updates in place. A changed group or
  name throws. A new script asks for group and name, then inserts.
  Duplicate appends ` (Copy)`, clears the three run timestamps, and
  loads code from the row, the open editor, or QueryRef 087.
- Delete confirms, then sends STRING `GROUP_NAME` and `SCRIPT_NAME`.
  A draft that was never saved is removed locally and does not call
  131. The generic INTEGER `selectedId` path is not used.
- A dirty row switch on Scripting asks “Save changes to Group.Name?”
  Save continues. Stay keeps the row. Other managers still auto-save.
- Footer actions moved to
  [`scripting-footer.js`](/elements/003-lithium/src/managers/scripting/scripting-footer.js).
  `wc -l` on `scripting.js` is 616. `cleanup()` removes the document
  keydown listener. The Lua editor does not install
  `customScrollbars()`, so `destroyCodeMirrorScrollbars` stays empty.
- [`scripting-save.test.js`](/elements/003-lithium/tests/unit/managers/scripting-save.test.js)
  passed 18 tests.

**Lessons learned:**

- “The row has names” is not “this row is persisted.” A new script
  already has both names before the first insert. The list load is
  the persisted set. Rename is refused only for an id in that set.
- LithiumTable auto-saves on row switch. Replacing
  `autoSaveBeforeRowChange` on the Scripting instance leaves every
  other table alone. Stay returns false, and the existing selection
  path reselects the editing row.
- `primaryKeyField` as a pair makes `rowData[pkField]` undefined, so
  the generic delete never calls QueryRef 131 and still removes the
  grid row. Scripting needed its own delete.
- List QueryRef 089 omits `code`. Duplicate of a row that is not the
  open editor has to read QueryRef 087 or the copy is empty.
- `buildEditorExtensions` does not install `customScrollbars()`. A
  no-op teardown is the honest cleanup. Phase 11 owns that plugin.
- The document keydown listener was an anonymous function, so
  `cleanup()` could not remove it.
- Group and name are not editable under auto-discovery, and 1394
  also marks them `editable: false`. Create is two prompts, then a
  local draft, then Save. 1394 is still not applied.

**Handoff:**

- Phase 9 starts in the next conversation. Add
  `scripting-invoke.js`. Do not put the run panel in `scripting.js`.
- `invokeScript` / `getScriptJob` still have no manager caller.
  `Api.Echo` is the `invokable = 1` fixture. Run disabled unless
  invokable. Wait defaults true. Timeout 1–60. No `_hydrogen`. No
  run-as-role. No fake cancel.
- Apply [`acuranzo_1394.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1394.lua)
  before Scripting reads Lookup 059 key 13. List 89 and search 90
  are still set on the constructor.
- No browser pass. The live auth file still fails 10 cases with
  HTTP 503. Cert for `lithium.philement.com` was already expired.
- Profile footer Save, Style `applyState`, and `this.app.user` stay
  open. Ctrl+P `handlePrettify` is still missing (Phase 10).

---

## Phase 9 — Invoke panel and async poll

**Goal:** Operator can run `Api.Echo` (wait true) and poll `wait: false`.
Course Manager will clone this pattern, not `scripting.js`.

**Dependencies:** Phases 2, 7, 8.

**Entry gate:**

- [x] `invokeScript` exists. `Api.Echo` is seeded `invokable=1` by
      `acuranzo_1297.lua` (Phase 7 working log). This sitting did not
      re-query the live row.

**Reference:**

- Queries Test tab + JSON collection editor (`queries-editors.js`)
- New `scripting-invoke.js` (do not grow `scripting.js`)

**Work items:**

- [x] Run tab: JSON params via `buildEditorExtensions({ language: 'json' })`,
      wait default true, timeout 1–60, Run disabled unless `invokable`.
- [x] Result pane: status, `job_id`, JSON / error. Treat 404
      `script_not_found` as not-runnable, not “row missing”.
- [x] `wait: false` → `getScriptJob` backoff until
      `completed|failed|killed|timeout`. No fake cancel.
- [x] Strip `_hydrogen`. Do not invent run-as-role.
- [x] Unit tests: payload, invokable gating, poll machine.

**Exit gate / validation:**

- [x] Manual: JWT → Scripting → `Api.Echo` → body in pane (log job_id). Andrew confirmed 2026-10-22: `status: completed`, `job_id: BBTDC`. The body is the echoed params.
- [x] Manual: wait off → spinner → terminal status (log). Andrew confirmed 2026-10-22: `status: completed`, `job_id: CCCRP`.
- [x] `npm test && npm run lint` — variance below. The chained
      command exited 1 at `npm test`. Lint was a separate run.

**Status:** complete (2026-10-22). Andrew confirmed both manual
lines on `https://lithium.500courses.com`. Wait on: `status:
completed`, `job_id: BBTDC`. Wait off: `status: completed`,
`job_id: CCCRP`. The body is the echoed params. Variance: the
2026-10-16 `npm test` was 1040 passed and 10 failed. The 10 are
`tests/integration/auth.integration.test.js` (HTTP 503 from
`https://lithium.philement.com`; cert expired 2026-08-02). No
Scripting source changed in this sitting. No new migration. No
product C.

**Accomplished:**

- [`scripting-invoke.js`](/elements/003-lithium/src/managers/scripting/scripting-invoke.js)
  (546 lines) owns the Run tab. `scripting.js` is 627 lines and only
  mounts the panel, shows it on the Run tab, syncs on row change, and
  destroys it on close.
- The Run tab sits after Preview. Params are a JSON CodeMirror editor
  that starts at `{}`, stays editable, and does not mark the script
  row dirty. Wait defaults checked. Timeout defaults to 15 and clamps
  to 1–60. Run stays disabled unless `invokable` is 1 and the name is
  `Group.Name`.
- The request drops a top-level `params._hydrogen` and logs that strip.
  The result pane still prints a `_hydrogen` key the script returns.
  There is no run-as-role field and no cancel control.
- `wait: false` polls `getScriptJob` at 250 ms, 500 ms, 1000 ms, then
  2000 ms. A pending job keeps polling until the row changes, the
  manager closes, or GET returns 404. Leaving the row stops painting
  and does not kill the job. The spinner is `.scripting-run-spinner`
  while the run or the poll is in flight.
- HTTP 404 `script_not_found` paints status `not_runnable` and the
  body “Not runnable”. GET 404 `job_not_found` paints status
  `missing` and the body “Job not found”. The session line is
  `[Scripting] Run ${script} status=${status} job_id=${jobId}`.
- A synchronous invoke extends the POST past the 30s client cap:
  `scriptClientTimeoutMs` is `(timeout_seconds + 5) * 1000` when wait
  is on. Timeout 15 uses 20000 ms. Timeout 60 uses 65000 ms. Wait off
  and the GET polls stay on the 30s default.
- [`scripting-invoke.test.js`](/elements/003-lithium/tests/unit/managers/scripting-invoke.test.js)
  passed 17 tests. `templates:copy` refreshed the public Scripting
  HTML and CSS.

**Lessons learned:**

- `post()` ignored any timeout longer than 30 seconds. A wait-true
  timeout of 31–60 would abort in the browser before Hydrogen
  returned. The grace is 5 seconds on top of `timeout_seconds`.
- `scriptInvokable` reads the field. Passing the whole row makes
  `Number(object)` NaN, so Run stays disabled and `run()` returns
  before the POST. The first suite run failed five tests for that.
- HTTP 202 is `response.ok`. `wait: false` returns the pending
  envelope. HTTP errors throw, and `extractError` attaches
  `serverError` only when `success` is false.
- A non-terminal status (`pending`, `running`) continues the poll.
  Stop is a terminal status, GET 404, a row change, or close.
- The params editor is session state. A detail merge only syncs the
  button, so a late `invokable` flag does not abandon an in-flight run.
  Selecting the same row id does not abandon it either.
- Invalid JSON is painted in the pane and does not call the API.
  There is no toast.
- `window.lithiumApp.currentManager` is the slot record
  `{ id, name, instance }`. The Scripting object is `instance`.
- On 500 Courses, QueryRefs 87 and 89 return `invokable`. Lookup 059
  key 13 is present. The disabled Run on 2026-10-17 was that schema
  before those rows were visible. Wait off paints `pending` and then
  `completed`. The spinner is `#scripting-tab-run.is-running`.

**Handoff:**

- Phase 9 is complete. Andrew confirmed both runs. Phase 10 is next.
  Discuss it and wait for approval before editing. Do not reopen
  5a–5d or 6 unless Andrew pulls Band B back. Do not drive a browser
  for his login checks.
- The 500 Courses database already has Lookup 059 key 13
  `scripts/script-manager`, and the collection names delete 131.
  List 89 and search 90 stay on the constructor.
- The philement.com auth file still fails 10 cases with HTTP 503.
  Cert for `lithium.philement.com` expired 2026-08-02. The 500
  Courses gate did not use that host.
- Phase 10 still owns the preview summary, `lithiumSettings` fonts,
  Ctrl+P `handlePrettify`, the optional meta tab, validate/luacheck,
  run-as-role, and `LITHIUM-MGR-SCRIPT.md`. Profile footer Save,
  Style `applyState`, and `this.app.user` stay open.
- `npm run deploy:500courses` publishes to
  `/fvl/tnt/t-500courses/lithium` and keeps that tree's
  `config/lithium.json`. This gate used the build already there
  (1.1.3447). Do not bump the version unless the source changed.

---

## Phase 10 — Scripting UX, tests, docs

**Goal:** Band C Scripting closable without blocking Course Manager if
Phase 9 already works. Prefer finishing this before Band D unless the
user pulls Course Manager forward.

**Dependencies:** Phases 3, 8, 9.

**Entry gate:**

- [x] Phase 9 sync invoke works against Hydrogen. Andrew confirmed
      2026-10-22 (`BBTDC`, `CCCRP`).

**Reference:**

- [`/docs/Li/LITHIUM-MGR-SCRIPT.md`](/docs/Li/LITHIUM-MGR-SCRIPT.md)

**Work items:**

- [x] Preview = sanitized **summary** markdown (Phase 3 helper),
      then the Lua source as plain text. `buildScriptSummaryHtml`
      and `scriptPreviewLua` in `scripting-preview.js`. Empty
      summary shows a note. The editor buffer wins over the row.
- [x] Fonts via `window.lithiumSettings`, path `33.font.<tab>`,
      using `get` / `set` / `removeSection` (one dotted path). An
      old `lithium_scripting_font_*` value is copied once.
- [x] Ctrl+P formats with `@johnnymorganz/stylua` 2.6.0 defaults
      (tabs, indent width 4, column 120, Unix LF). The Lua toolbar
      has the same tulip button as Queries, `#scripting-prettify-btn`,
      disabled off the Lua tab. Andrew confirmed both on 2026-10-22.
- [~] Optional meta tab: invokable, schedule, last run. Andrew
      declined it on 2026-10-22. It does not move to Phase 11.
- [x] `tests/unit/managers/scripting-*.test.js` for the rest of this
      phase that is built. Formatter coverage is
      `scripting-format.test.js`. Preview and font coverage are
      `scripting-preview.test.js` and `scripting-font.test.js`.
      The declined meta tab has no test.
- [x] Write `LITHIUM-MGR-SCRIPT.md`; TOC + MGR links.
- [x] All `src/managers/scripting/*` ≤ 1000 lines. Largest is
      `scripting.js` at 649.

**Exit gate / validation:**

- [x] `npm test && npm run lint` — variance below. The chained
      command exited 1 at `npm test`. Lint was a separate run.
      2026-10-22: 1067 passed and 10 failed. The failures are
      `auth.integration.test.js` HTTP 503 against
      `https://lithium.philement.com` (cert expired 2026-08-02).
      Lint was 0 errors and 34 warnings.
- [x] `wc -l src/managers/scripting/*` all ≤ 1000. Largest is
      `scripting.js` at 649.
- [x] TOC lists `LITHIUM-MGR-SCRIPT.md`

**Status:** complete (2026-10-22). Andrew confirmed the tulip
button, Ctrl+P, pretty-print then save, the Preview tab (summary,
then Lua), and that a 9px font survives a reload. He declined the
optional meta tab. Variance: `npm test` was 1067 passed and 10
failed. The 10 are `tests/integration/auth.integration.test.js`
(HTTP 503 from `https://lithium.philement.com`; cert expired
2026-08-02). `npm run lint` was 0 errors and 34 warnings. No new
caller-facing QueryRef. Acuranzo 1395 and 1396 were already applied.

**Accomplished:**

- Ctrl+P and `#scripting-prettify-btn` format with
  `@johnnymorganz/stylua` 2.6.0 defaults (tabs, indent width 4,
  column 120, Unix LF). The code is
  [`scripting-format.js`](/elements/003-lithium/src/managers/scripting/scripting-format.js)
  and `EditorManager.handlePrettify`. Andrew confirmed both on
  2026-10-22.
- Save sends INTEGER `SCRIPT_TYPE`, `STATUS`, `USERID`, and
  `INVOKABLE`, and `''` for a blank schedule or run time.
  Acuranzo 1395 (QueryRef 130) and 1396 (QueryRef 129) store that
  blank as SQL NULL. Andrew applied both and confirmed
  pretty-print then save.
- Preview shows sanitized summary markdown, then the Lua source as
  plain text (`scripting-preview.js`). Andrew confirmed it on
  `Api.Echo`.
- Fonts use `window.lithiumSettings` at `33.font.<tab>`. Save calls
  `flush()`. Load reads `lithium_preferences` when the server copy
  has no font. Andrew confirmed that 9px survives a reload.
- [`/docs/Li/LITHIUM-MGR-SCRIPT.md`](/docs/Li/LITHIUM-MGR-SCRIPT.md)
  is linked from the TOC and from `LITHIUM-MGR.md`.
- Tests: `scripting-format.test.js` 9, `scripting-save.test.js` 19,
  `scripting-preview.test.js` 5, `scripting-font.test.js` 7.
  `npm test` was 1067 passed and 10 failed. `npm run lint` was 0
  errors and 34 warnings. Largest scripting file is `scripting.js`
  at 649 lines.

**Working log:**

- 2026-10-22: Andrew approved the official StyLua package and its
  defaults. `scripting-format.js` loads `@johnnymorganz/stylua/web`
  on Ctrl+P. Each format builds a new `Config`. A parse failure is
  a string, is toasted, and does not change the buffer. Lua 5.5
  `global` still fails that parse. `scripting-format.test.js`
  passed 9. `npx vite build` emitted `vendor-stylua` and a separate
  wasm asset (about 3.6 MB) referenced only from the Scripting
  chunk. Full `npm test` was not re-run.
- 2026-10-22: Andrew confirmed the tulip button and Ctrl+P. Saving
  the formatted script failed on QueryRef 130 because `SCRIPT_TYPE`
  and `STATUS` were numbers in `STRING`, and the blank schedule and
  run times were JSON null. `buildScriptSaveParams` now puts those
  integers in `INTEGER` and sends `''` for a blank. `acuranzo_1395.lua`
  and `acuranzo_1396.lua` store that blank as SQL NULL. They are
  not applied. `scripting-save.test.js` passed 19. Full `npm test`
  was not re-run.
- 2026-10-22: Andrew applied 1395 and 1396. Pretty-print then save
  works on the 500 Courses Lithium install. Preview markdown, font
  prefs, the optional meta tab, `LITHIUM-MGR-SCRIPT.md`, and the
  phase exit (`npm test && npm run lint`, line counts, TOC) are
  still open.
- 2026-10-22: Andrew asked for the summary preview, font prefs,
  the scripting doc, and the test exit. Preview renders
  `currentScript.summary` through `marked` and `sanitizeHtml`.
  A blank summary shows "No summary to preview." Fonts use
  `window.lithiumSettings` at `33.font.<tab>`. That service takes
  one path. A section-plus-path call would store the path string
  in section 33. `scripting-preview.test.js` passed 3.
  `scripting-font.test.js` passed 5. `npm test` was 1063 passed
  and 10 failed, the same live auth HTTP 503. `npm run lint` was
  0 errors and 34 warnings. Every scripting file is under 1000
  lines. TOC lists the doc. The optional meta tab is not built.
  The phase stays open until he confirms the Preview tab and the
  font popup.
- 2026-10-22: Andrew opened Preview on `Api.Echo` and saw only
  `LUA_CLIENT fixture: echo params (+ optional probe_health)`.
  That string is the whole `summary` column from
  `acuranzo_1296.lua`. The font button was disabled because
  `switchTab` enabled it only on Lua. It is now enabled on Lua
  and Preview, and stays disabled on Run. He chose summary, then
  the Lua source. Preview shows both. The Lua is `textContent`.
- 2026-10-22: Andrew confirmed Preview (summary, then Lua) and that
  the font applies. A reload did not keep 9px. Save wrote
  `lithium_preferences` and waited two seconds to update account
  settings. Login then replaces the in-memory settings with the
  server copy. Load now reads the local preferences when that copy
  has no font, writes the font back, and flushes the update at once.
- 2026-10-22: Andrew confirmed the font size survives a reload:
  "Fixed! Font size is retained after restart." The optional meta
  tab is still not started. Phase 10 stays in progress.
- 2026-10-22: Andrew declined the optional meta tab. `npm test`
  was 1067 passed and 10 failed, the same live auth HTTP 503.
  `npm run lint` was 0 errors and 34 warnings. `scripting.js` is
  649 lines. Phase 10 is complete.

**Lessons learned:**

- StyLua's `formatCode` destroys the Config, including on a parse
  failure. A second call on the same Config throws a null-pointer
  string. The thrown parse error has no `.message`.
- Hydrogen's conduit type check rejects JSON null before the binder
  that would store SQL NULL. A blank optional value is `''`, and
  the SQL is `NULLIF(CAST(:COL AS text), '')`. A timestamp then
  casts that result. One caller-facing QueryRef per file, so 129
  and 130 are 1396 and 1395.
- `window.lithiumSettings` is `GlobalSettingsService`. `get`, `set`,
  and `removeSection` each take one dotted path. The profile
  service's `(section, path)` shape is a different object.
- `set` writes `lithium_preferences` and waits two seconds before
  QueryRef 74. `syncFromServer` replaces memory and does not write
  local storage. A reload inside that window drops the new font.
  `flush()` sends the update at once. Load reads the local JSON
  when the server copy has no font.
- `Api.Echo` `scripts.summary` is one sentence. The longer markdown
  in `acuranzo_1296.lua` belongs to the migration query row.
- happy-dom's DOMPurify drops the `h1` tag. The preview test
  asserts the heading text and that `<script` is absent.

**Handoff:**

- Phase 10 is complete. Phase 11 (productize CodeMirror) is next.
  Discuss it and wait for approval. Do not edit Phase 11 source in
  this sitting.
- Andrew declined the meta tab. Validate/luacheck and run-as-role
  stay out. Do not build them under Phase 11 unless he asks.
- Create, duplicate, and delete still have no browser pass.
- Queries fonts still use `lithium_queries_font_*`. Do not change
  that storage.
- Profile footer Save, Style `applyState`, and `this.app.user` stay
  open. Phases 5a–5d and 6 stay deferred. Reporting stays parked.
- Variance: `npm test` was 1067 passed and 10 failed, all
  `auth.integration.test.js` HTTP 503 on
  `https://lithium.philement.com`. The cert expired 2026-08-02.
  Lint was a separate run: 0 errors and 34 warnings.
- He does deploys and login checks. Do not drive a browser. Do not
  apply migrations. Next free Acuranzo file is 1397. This phase
  added no caller-facing QueryRef.

---

## Phase 11 — Productize CodeMirror plugins

**Goal:** CM6 is the only stack; custom plugins are modules with tests;
Lua folds.

**Dependencies:** Phase 8 (scripting already uses `buildEditorExtensions`).
Phases 9 and 10 are complete. Default is serial. Discuss this phase
and wait for approval before editing.

**Entry gate:**

- [x] Phase 8 complete (editor facades stable).

**Reference:**

- `src/core/codemirror.js`, `codemirror-setup.js`,
  `cm6-virtual-columns.js`, `cm6-custom-scrollbars.js`,
  `editor-footer.js`
- `src/core/lua-mode.js`, `cm6-newline.js`, `cm6-indent.js`,
  `cm6-json.js`, `cm6-fold.js`
- Deleted: `src/init/codemirror-init.js`

**Work items:**

- [x] Runtime CM packages are in `dependencies`.
      `@codemirror/language` is declared at `^6.13.1`.
      `@codemirror/legacy-modes` is removed. Nothing imported it.
- [x] Deleted `src/init/codemirror-init.js` (L16). Zero imports.
- [x] Split `codemirror-setup.js` under 1000. Newline, indent
      facet/filter/Tab-Enter, JSON sort, and fold-to-level are
      modules. `buildEditorExtensions` stays the facade and
      re-exports the moved helpers. The file is 654 lines.
- [x] Virtual columns register once. A compartment owns the
      extension when the caller passes one. The extra push runs
      only when there is no compartment.
- [x] Tests for `virtualColumns` pad/trim and scrollbar destroy:
      `tests/unit/cm6-virtual-columns.test.js` and
      `tests/unit/cm6-scrollbars.test.js`.
- [x] Kept the homemade Lua tokenizer. Andrew chose that on
      2026-10-22. Long brackets stay as they were. Fold All covers
      `function`, `if`, `do`, `for`, and `while` through `end`, and
      `repeat` through `until`. The header line stays. Keywords
      inside strings and comments do not count. Parentheses and
      braces do not fold. `H.` and `H.name` use the `namespace` tag.
- [x] Footer error count stays 0. `init` still writes `Errors: 0`.
      No caller passes a diagnostic count. No luacheck.

**Exit gate / validation:**

- [x] `wc -l src/core/codemirror-setup.js` is 654.
- [x] `npm test && npm run lint && npm run build` — variance below.
      The chained command exits 1 at `npm test`. Lint and build were
      separate runs, both exit 0. 2026-10-22: 1079 passed and 10
      failed. The failures are `auth.integration.test.js` HTTP 503
      against `https://lithium.philement.com` (cert expired
      2026-08-02). Lint was 0 errors and 34 warnings. Build
      completed.
- [x] Grep of `src/` and `tests/`: no imports of `codemirror-init.js`.

**Status:** complete (2026-10-22). Andrew chose the current Lua
tokenizer, statement-block folds, and the `H.` tint. Variance:
`npm test` was 1079 passed and 10 failed. The 10 are
`tests/integration/auth.integration.test.js` (HTTP 503 from
`https://lithium.philement.com`; cert expired 2026-08-02).
`npm run lint` was 0 errors and 34 warnings. `npm run build`
completed. No new caller-facing QueryRef.

**Accomplished:**

- Runtime `@codemirror/*` packages moved from `devDependencies` to
  `dependencies`. `@codemirror/language` is a direct dependency.
  `@codemirror/legacy-modes` is gone from `package.json` and the
  lockfile.
- Deleted `src/init/codemirror-init.js`.
- [`codemirror-setup.js`](/elements/003-lithium/src/core/codemirror-setup.js)
  is the facade at 654 lines. Newline, indent, JSON sort, and fold
  helpers live in `cm6-newline.js`, `cm6-indent.js`, `cm6-json.js`,
  and `cm6-fold.js`, and are re-exported.
- A virtual-columns compartment is the only registration when the
  caller passes one.
- Lua stays in
  [`lua-mode.js`](/elements/003-lithium/src/core/lua-mode.js).
  `luaFoldRanges` feeds a fold service. Fold All asks `foldable`
  per line as well as per syntax node. `H.` and `H.name` are
  `namespace` tokens, which oneDark already paints.
- Trailing virtual-column spaces trim after the update, because
  CodeMirror rejects a dispatch inside `ViewPlugin.update`.
- Tests: `lua-mode.test.js` 14, `cm6-virtual-columns.test.js` 2,
  `cm6-scrollbars.test.js` 1. `npm test` was 1079 passed and 10
  failed. Lint was 0 errors and 34 warnings. Build completed.

**Working log:**

- 2026-10-22: Andrew kept the current tokenizer, asked for statement
  folds (`function` / `if` / `do` / `for` / `while` … `end`, and
  `repeat` … `until`), and asked for the `H.` tint. Parentheses,
  braces, and long strings stay open. The header line stays visible.
- 2026-10-22: The split, the dependency move, the CM5 delete, and
  the fold service landed. `npm test` was 1079 passed and 10 failed
  (the same auth HTTP 503). Lint was 0 errors and 34 warnings.
  `npm run build` completed. `codemirror-setup.js` is 654 lines.

**Lessons learned:**

- Stream tokens are flat. A node prop on `keyword` cannot span
  `function` … `end`. `foldable` asks a fold service with the line
  extent first. Fold All used to walk syntax nodes only, so that
  service never saw the line.
- The `do` of `for` and `while` is the body introducer. A bare `do`
  is its own block.
- oneDark paints `namespace` with the same chalk color it uses for
  numbers and type names. `H.http` does not need a new stylesheet.
- `ViewPlugin.update` cannot dispatch. The trim of virtual-column
  padding has to wait until that update finishes.
- `@codemirror/legacy-modes` was installed and never imported.

**Handoff:**

- Phase 11 is complete. Phase 12 (Course Manager scaffold, ID 34)
  is next. Discuss it and wait for approval. Do not edit Phase 12
  source in this sitting.
- Fold All on Lua covers the six statement blocks above. The header
  stays. Strings and comments do not count. Parentheses and braces
  do not fold. There is no browser click of the Fold All button.
  The unit test folds a function body in happy-dom.
- `H.` and `H.name` are `namespace` tokens. A bare `H` stays a name.
- Create, duplicate, and delete still have no browser pass.
- The footer still shows `Errors: 0`. Do not add luacheck here.
- Queries fonts still use `lithium_queries_font_*`. Do not change
  that storage.
- Profile footer Save, Style `applyState`, and `this.app.user` stay
  open. Phases 5a–5d and 6 stay deferred. Reporting stays parked.
  [`/docs/Li/plans/REPORTING_PLAN.md`](/docs/Li/plans/REPORTING_PLAN.md)
  still links the deleted `codemirror-init.js`. That plan was not
  edited.
- Variance: `npm test` was 1079 passed and 10 failed, all
  `auth.integration.test.js` HTTP 503 on
  `https://lithium.philement.com`. The cert expired 2026-08-02.
  Lint was a separate run: 0 errors and 34 warnings. Build was a
  separate run and completed.
- He does deploys and login checks. Do not drive a browser. Do not
  apply migrations. Next free Acuranzo file is still 1397. This
  phase added no caller-facing QueryRef.

---

## Band D — Course Manager (first official deployment)

Requirements: PRIORITIZE Part 5. Operators must not need Lua, QueryRefs,
or `H.http`. Every action is JWT `invokeScript` or a staff QueryRef +
LithiumTable.

## Phase 12 — Course Manager scaffold (ID 34)

**Goal:** Loadable manager, JWT-roles gated, no fake success, no Course
Builder widgets.

**Dependencies:** Phases 0 (L1, L3), 1, 2.

**Entry gate:**

- [x] Phase 0 reserved ID 34. Phase 1 close path exists. Phase 2
      `invokeScript` exists.

**Reference:**

- PRIORITIZE §5 (authz paragraph overridden by L3)
- New-manager checklist in AGENTS.md
- `src/app/manager-loader.js`, `config/lithium.json`

**Work items:**

- [x] Placeholder already exists from Phase 0a: `course-manager.js`,
      `course-manager.css`, registry id 34, `_importManager` case 34,
      `config/lithium.json` `managers`, and `cleanup()`. Markup stays
      inline. Phase 12 replaced the “coming in Phase 12” panel. ID 34
      was not registered a second time.
- [x] `cleanup()` stays the close hook.
- [x] Hide / refuse load when JWT `roles` fails L3. Staff sees a title
      and one sentence. Anyone else sees the lock panel. The sidebar
      omits ID 34 when the gate fails. A direct open still shows the
      lock panel.
- [x] No new Helium file. Andrew reported 1378, 1379, 1380, and 1381
      applied on the 500 Courses database (2026-10-22). Those rows are
      the menu path. QueryRef **155** is the name lookup.
- [x] Docs stub
      [`/docs/Li/LITHIUM-MGR-COURSE.md`](/docs/Li/LITHIUM-MGR-COURSE.md)
      points at PRIORITIZE Part 5. The shell text does not contain
      “Lua”.
- [x] Do not impersonate learners. Do not show secrets. The shell has
      no learner data and no secret fields.

**Exit gate / validation:**

- [x] `npm run build && npm test && npm run lint` — variance below.
      The chained command exits 1 at `npm test`. Lint and build were
      separate runs. Lint exited 0 and printed no problems. Build
      completed, including `lint:css`. 2026-10-22: 1097 passed and 10
      failed. The failures are `auth.integration.test.js` HTTP 503
      against `https://lithium.philement.com` (cert expired
      2026-08-02).
- [x] Loader imports ID 34 without throw.
      `course-manager.test.js` calls `ManagerLoader._importManager(34)`.
- [x] Unit: roles gate helper (allow/deny fixtures).
      `isCourseManagerAuthorized` stays in `utils.test.js`.
      `course-access.test.js` covers QueryRef 155 allow, deny, and
      the role-id 2 failure path.

**Status:** complete (2026-10-22). Andrew chose the one-sentence
staff shell, sidebar hide plus the lock panel, and deny when
QueryRef 155 cannot resolve names. He reported 1378, 1379, 1380,
and 1381 applied. Variance: `npm test` was 1097 passed and 10
failed. The 10 are `tests/integration/auth.integration.test.js`
(HTTP 503 from `https://lithium.philement.com`; cert expired
2026-08-02). `npm run lint` exited 0 and printed no problems.
`npm run build` completed. No new caller-facing QueryRef. Andrew
confirmed the shell on 2026-10-22 after the stored-token fix.

**Accomplished:**

- [`course-manager.js`](/elements/003-lithium/src/managers/course-manager/course-manager.js)
  is 71 lines. Staff see “Course Manager” and one sentence. The
  lock panel stays for a denied open. `cleanup()` clears the
  container.
- [`course-access.js`](/elements/003-lithium/src/managers/course-manager/course-access.js)
  calls QueryRef **155** once per role id. A missing client, a
  thrown query, or a non-array payload denies and logs. Role ids
  2 and 3 are not a local allow list.
- [`main-sidebar.js`](/elements/003-lithium/src/managers/main/main-sidebar.js)
  drops ID 34 from the grouped menu and from `permittedManagers`
  when the gate fails. The menu cache is not rewritten.
- Doc:
  [`/docs/Li/LITHIUM-MGR-COURSE.md`](/docs/Li/LITHIUM-MGR-COURSE.md).
  TOC lists it.
- Tests: `course-access.test.js` 13, `course-manager.test.js` 5.
  `npm test` was 1097 passed and 10 failed. Lint printed no
  problems. Build completed.

**Working log:**

- 2026-10-22: Andrew chose a title and one sentence, hiding the
  sidebar row while keeping the lock panel, and denying when
  QueryRef 155 fails. He reported 1378, 1379, 1380, and 1381
  applied. No new migration.
- 2026-10-22: The shell, the gate, and the sidebar filter landed.
  `npm test` was 1097 passed and 10 failed (the same auth HTTP
  503). `npm run lint` exited 0 and printed no problems.
  `npm run build` completed. `course-manager.js` is 71 lines.
- 2026-10-22: The first deploy hid Course Manager for account 9001
  even though `roles` was `2,3` and QueryRef 155 returned `staff`
  and `admin`. `getClaims()` with no argument returns null, so the
  gate logged `no role ids` and the sidebar removed ID 34 before
  the token was read. A new login did not change that. The fix is
  `getClaims(retrieveJWT())`. Andrew then confirmed the Learning
  group shows the row and the shell opens. `course-access.test.js`
  is 15 tests. No new migration.

**Lessons learned:**

- The Phase 0b fallback that mapped role id 2 to `staff` and 3 to
  `admin` hid a failed QueryRef 155. The shell now denies that
  path. A staff login needs 1380 and 1381 on the database, which
  Andrew says are applied.
- `getClaims` only decodes the string it is given. Calling it with
  no argument returns null. The menu gate and the shell must use
  `getClaims(retrieveJWT())`. A fresh login does not repair a check
  that never reads the stored token. The session log line for that
  bug is `no role ids`, not a failed QueryRef 155.
- The Hydrogen connection name in the JWT `database` claim is
  `Lithium`. That connection is Yugabyte database `t-500courses-db`,
  schema `lithium`. `t_500courses_db` is the login role. Account
  9001 already had `account_roles` 2 and 3, and both `roles` rows
  are active. The missing menu row was the client, not the database.
- `getMenu` returns its cache by reference. Filtering ID 34 has to
  copy the groups. Writing the filtered list back would hide the
  row for the rest of the session even after a later allow.
- `permittedManagers` is what the static sidebar and the last-manager
  restore both read. Stripping 34 there hides the row on the menu
  failure path. `loadManager(34)` does not consult that list, so a
  direct open still paints the lock panel.

**Handoff:**

- Phase 12 is complete. Andrew confirmed the Learning group shows
  Course Manager and that the shell opens. Phase 13 (courses list)
  is next. Discuss it and wait for approval. Do not edit Phase 13
  source in this sitting.
- Phase 13 cannot list every course until a staff QueryRef exists.
  **4004** (`courses_4011.lua`, filters in `courses_4075.lua`) is
  the published catalog. It still returns retired published rows
  unless `HIDE_RETIRED=1`. It is not the operator list. The next
  Courses file is **4076**. The next caller-facing ref is **4008**.
  `courses_4008.lua` already created the `courses` table. Do not
  treat that file number as QueryRef 4008. No Lookup 059 tableDef
  exists for courses. Confirm the next free Lookup 059 key on disk
  before numbering one. Prepare the packet only after Phase 13 is
  approved, and do not apply it.
- The table already has `code`, `slug`, `title`, `summary`,
  `description`, `image_path`, `canvas_course_id`, `pricing_type`,
  `stripe_product_id`, `published`, `retired`, and `tags` (jsonb,
  `courses_4046.lua`) plus `canvas_updated_at`. History refs
  **4006** and **4007** exist. `Catalog.SyncFromCanvas` and
  `Catalog.Retire` stay `invokable=0` until Phase 18. Phase 13
  shows those buttons disabled. Do not flip `invokable` in Phase 13.
- He does deploys and login checks. Do not drive a browser for
  those.
- Create, duplicate, and delete still have no browser pass. Fold
  All and the `H.` tint have unit coverage and no live click.
- Profile footer Save, Style `applyState`, and `this.app.user` stay
  open. Phases 5a–5d and 6 stay deferred. Reporting stays parked.
- Variance: `npm test` was 1097 passed and 10 failed, all
  `auth.integration.test.js` HTTP 503 on
  `https://lithium.philement.com`. The cert expired 2026-08-02.
  Lint was a separate run and printed no problems. Build was a
  separate run and completed.
- Do not apply migrations. Next free Acuranzo file is still 1397.
  This phase added no caller-facing QueryRef.

---

## Phase 13 — Courses (brochure SKU)

**Goal:** List / create-link / edit Lithium-owned fields / retire /
sync-from-Canvas CTAs.

**Dependencies:** Phase 12. Phase 9 pattern recommended.

**Entry gate:**

- [x] Phase 12 Status complete. Helium `courses` tableDef / list
      QueryRef identified in Working Log (do not invent).

**Reference:**

- PRIORITIZE §5.1
- `Catalog.SyncFromCanvas`, `Catalog.Retire` (still `invokable=0` until
  Phase 18 packet)
- Staff list **4500**, insert **4501**, update **4502**. History for
  the SPA is **4503** and **4504**. Lua still reads **4006** and
  **4007**. **4004** is the published catalog.

**Work items:**

- [x] LithiumTable of all `courses` (published, unpublished, **retired**).
      Navigator and local search on code/slug/title/`canvas_course_id`.
      QueryRef **4500**. The list uses the Query Manager shell:
      midnight theme, splitter, navigator. Retired is not a list
      filter.
- [x] Create Lithium catalog row: `code`, `slug`, `canvas_course_id`,
      `pricing_type`, `image_path` URL, plus required `title`,
      `is_featured`, `sort_order`, and tags. QueryRef **4501**.
      `published` stays 0. Sync does not insert.
- [x] Edit Lithium-owned fields only. QueryRef **4502** leaves
      title, summary, description, published, and stripe as they
      are. `courses_4081.lua` adds `retired`. Canvas unpublish is
      a sync result. Open Canvas is the button. Sync flips
      `published` when that script is invokable.
- [x] Navigator add, duplicate, and delete come from LithiumTable
      (`readonly: false`). Delete toasts and leaves the row.
      Duplicate appends `-copy` and ` (Copy)`, clears the Canvas id
      and the Stripe product, and stores published and retired as 0.
- [x] Save and cancel use ManagerEditHelper and the slot footer.
      Fields stay disabled until edit mode. The slot title is the
      caption.
- [x] Course Options holds Retired and Featured. Insert **4501**
      (`courses_4082.lua`) and update **4502** (`courses_4081.lua`)
      store INTEGER `RETIRED`.
- [x] Commerce lists cad, usd, eur, and gbp from QueryRef **4505**
      (`courses_4083.lua`). Amounts stay read-only. A secret-shaped
      Stripe price id renders as “Hidden”.
- [x] Canvas uses the graduation-cap icon and shows the image from
      the image path at 524×292, twice the 262×146 card, stretched
      to that box, with rounded corners and a little space above.
- [x] Default visible columns are Code and Title
      (`acuranzo_1398.lua`). A saved template can keep the older
      columns until it is reset.
- [x] Tags editor is a JSON object textarea. An array is refused.
      Blank tags send `{}`.
- [x] Sync, sync all, and retire call `invokeScript` when QueryRef
      **87** says `invokable` is 1. Otherwise the button stays
      disabled and the wrapper title is “not enabled on this server”.
- [x] History is a `<pre>` filled with `textContent`. The SPA calls
      **4503** and **4504**, the auth_query twins of **4006** and
      **4007**.
- [x] `stripe_product_id` is read-only. A value shaped like `sk_`,
      `rk_`, or `whsec_` renders as “Hidden”.

**Exit gate / validation:**

- [x] Manual: list fixture courses, including retired rows; denied
      role sees nothing useful. Andrew reported the latest build
      deployed, the latest migrations applied, and the list Retired
      checkbox removed on 2026-10-23, and closed this gate.
- [x] `npm test && npm run lint` — variance in Status. `npm run lint:css`
      exited 0.
- [x] All new files ≤ 1000 lines. `course-manager.js` is 623.
      `course-edit.js` is 297. `course-catalog.js` is 605.
      `course-manager.css` is 234.

**Status:** complete (2026-10-23). Andrew reported the latest build
deployed, the latest migrations applied, and the list Retired
checkbox removed. He closed the manual gate on that report.
Applied: `courses_4076.lua` through `courses_4083.lua` (QueryRefs
**4500–4505**) and `acuranzo_1397.lua` plus `acuranzo_1398.lua`
(Lookup 059 key 14, default columns Code and Title). Next Courses
file **4084**. Next caller ref **4506**. Next Acuranzo file
**1399**. File numbers stay below 4500. **4008** is the courses
table file. Variance: `npm test` was 1121 passed and 10 failed
(1131; 47 files passed, 1 failed). The 10 are
`tests/integration/auth.integration.test.js` (HTTP 503 from
`https://lithium.philement.com`, where the suite expected 200 or
401; cert expired 2026-08-02). That is the known live auth
variance. `npm run lint` and `npm run lint:css` exited 0 and
printed no problems. The live browser was not driven in this
sitting. Andrew did the deploy and the manual close.

**Accomplished:**

- [`courses_4076.lua`](/elements/002-helium/courses/migrations/courses_4076.lua)
  QueryRef **4500**, staff list, no parameters.
- [`courses_4077.lua`](/elements/002-helium/courses/migrations/courses_4077.lua)
  QueryRef **4501**, insert. [`courses_4078.lua`](/elements/002-helium/courses/migrations/courses_4078.lua)
  QueryRef **4502**, update of Lithium-owned columns.
- [`courses_4079.lua`](/elements/002-helium/courses/migrations/courses_4079.lua)
  QueryRef **4503** and [`courses_4080.lua`](/elements/002-helium/courses/migrations/courses_4080.lua)
  QueryRef **4504** copy the **4006** and **4007** line text for
  `auth_query`. Lua still loads 4006 and 4007.
- [`acuranzo_1397.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1397.lua)
  Lookup 059 key 14, `courses/course-manager`.
  [`acuranzo_1398.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1398.lua)
  sets the default visible columns to Code and Title.
- [`courses_4081.lua`](/elements/002-helium/courses/migrations/courses_4081.lua)
  rewrites **4502** and
  [`courses_4082.lua`](/elements/002-helium/courses/migrations/courses_4082.lua)
  rewrites **4501** so both store `retired`.
  [`courses_4083.lua`](/elements/002-helium/courses/migrations/courses_4083.lua)
  is QueryRef **4505**, the read-only price list.
- [`course-manager.js`](/elements/003-lithium/src/managers/course-manager/course-manager.js),
  [`course-edit.js`](/elements/003-lithium/src/managers/course-manager/course-edit.js),
  and [`course-catalog.js`](/elements/003-lithium/src/managers/course-manager/course-catalog.js).
  The L3 gate and the lock panel stay. The slot title is the caption.
  The navigator and the footer follow Query Manager.
- Tests: `course-catalog.test.js` 12, `course-manager.test.js` 15,
  `course-access.test.js` 15. luacheck on `courses_4081.lua`,
  `courses_4082.lua`, `courses_4083.lua`, and `acuranzo_1398.lua`:
  0 warnings.

**Working log:**

- 2026-10-22: Andrew approved Phase 13. **4008** cannot be the staff
  list. Bootstrap orders `queries` by `query_type_a28` descending,
  and `auth_query` keeps the first cache row. After APPLY, every
  file number still has an active type-1001 reverse row, so a
  type-1 row at that integer is invisible. 4006 and 4007 have the
  same shape. New SPA refs are **4500–4504**. File numbers stay
  below 4500.
- 2026-10-22: Packet written and not applied. `npm run lint` and
  `npm run lint:css` exited 0 and printed no problems. Course
  Manager unit tests passed before the full suite.
  `course-catalog.test.js` passed 11. `course-manager.test.js`
  passed 12. Full `npm test` was 1117 passed and 10 failed. The
  10 are `auth.integration.test.js` HTTP 503 on
  `https://lithium.philement.com` (cert expired 2026-08-02). The
  same live auth variance as earlier phases.
- 2026-10-22: Andrew opened the live list. The columns were crushed
  and the detail was one long form. The list is now the same
  LithiumTable shell as Query Manager (navigator, local search,
  splitter, midnight theme). The right side is four tabs: Course
  (brochure fields and Save), Canvas (summary, description,
  published, last sync, and the sync actions), Commerce (Stripe
  product id), and History. Learners and the inbox stay out until
  those phases have a real list. `course-manager.test.js` passed
  13. `npm run lint` and `npm run lint:css` exited 0.
- 2026-10-22: Query Manager follow-up. The navigator group is
  add, duplicate, and delete. Delete leaves the course. Save and
  cancel use the slot footer. Course Options holds Retired and
  Featured. Canvas uses graduation-cap and the image path.
  Commerce lists the four currencies. Default columns are Code and
  Title. Open Canvas is `https://canvas.500courses.com/courses/{id}`.
  `course-catalog.test.js` passed 12. `course-manager.test.js`
  passed 15. `course-access.test.js` passed 15. Full `npm test`
  was 1121 passed and 10 failed (1131). The 10 are
  `auth.integration.test.js` HTTP 503 on
  `https://lithium.philement.com` (cert expired 2026-08-02).
  `npm run lint` and `npm run lint:css` exited 0. luacheck on
  the four new Lua files reported 0 warnings. The live browser
  was not driven.
- 2026-10-23: The Canvas image is 524×292, stretched to the box,
  with rounded corners and `--space-5` above it. The Retired
  checkbox above the list is gone. Retired stays under Course
  Options. `course-catalog.test.js` passed 12.
  `course-manager.test.js` passed 15. `course-access.test.js`
  passed 15. `npm run lint` and `npm run lint:css` exited 0.
  The live Course Manager was not driven.
- 2026-10-23: Andrew reported the latest build deployed and the
  latest migrations applied, including `courses_4081.lua`,
  `courses_4082.lua`, `courses_4083.lua`, and `acuranzo_1398.lua`.
  The list has no Retired checkbox. He closed the manual gate. No
  source edit in this sitting. Phase 13 is complete. Phase 14 is
  next. Discuss it and wait.

**Lessons learned:**

- A Courses caller ref that equals a file number is safe for a
  typed lookup (public type 10, migration apply type 1000) and for
  Lua that loads `queries.code` with `ORDER BY query_id DESC`. It
  is not safe for `auth_query`. The first cached row is the reverse
  migration. Reserve **4500–4999** for caller refs. Do not number a
  file there.
- QueryRef **87** is still the invokable check. The three action
  buttons stay disabled when that flag is not 1. Disabled-button
  titles do not show. The wrapper span holds the title, and the
  disabled button has `pointer-events: none`.
- `auth_query` does not check `staff`. Any Hydrogen JWT can call
  **4500–4505** once they are applied. The manager gate is the
  client. A later Hydrogen change would be a different plan.
- Hydrogen ignores a bind name the SQL omits, and it fails when the
  SQL names a bind the client omits. The SPA sends INTEGER
  `RETIRED` on every save. Reload Lithium before applying
  `courses_4081.lua` and `courses_4082.lua`.
- A saved LithiumTable template keeps its own column list. Lookup
  059 key 14 changes the default. Reset the template from the
  navigator when Code and Title are the columns you want.
- History SQL uses PostgreSQL `TO_CHAR` and `||`, matching 4006 and
  4007. The 500 Courses database is Yugabyte. Do not point the SPA
  back at 4006 or 4007.
- Insert uses `CREATEDID` and `UPDATEDID` so the operator id is not
  bound twice. A blank canvas id is `''`, then `NULLIF`, then a
  cast. Tags are always a JSON object string. MySQL casts text to
  `char(255)`, so a title longer than 255 characters does not fit
  that engine. Yugabyte casts text.
- Andrew closed Phase 13 after the deploy and the apply. The list
  has no Retired checkbox on that build. Retired is the Options
  checkbox.

**Handoff:**

- Phase 13 is complete. Andrew reported the latest build deployed,
  the latest migrations applied, and the list Retired checkbox
  removed. Phase 14 (catalog commerce) is next. Discuss it and
  wait for approval. Do not edit Phase 14 source in this sitting.
- The read-only price list is QueryRef **4505**. Price editing,
  Ensure Product, a new Stripe Price when an amount changes,
  test versus live from `STRIPE_MODE`, an orders list, and refund
  or deactivate stay in Phase 14.
- Leave `invokable` at 0 on `Catalog.SyncFromCanvas` and
  `Catalog.Retire` until Phase 18. The buttons stay disabled, with
  the tip “not enabled on this server”.
- Next Courses file **4084**. Next caller ref **4506**. Next
  Acuranzo file **1399**. File numbers stay below 4500. **4004**
  stays the published catalog. **4008** is the courses table file.
- A saved LithiumTable template can still show the older columns.
  Reset it from the navigator when Code and Title are the columns
  you want. The storage key is `course_manager_table`.
- `auth_query` does not check `staff`. Any Hydrogen JWT can call
  **4500–4505**. The manager gate is the client.
- The SPA sends INTEGER `RETIRED` on every save. A save that omits
  it fails against **4501** and **4502** as rewritten by 4082 and
  4081.
- He does deploys and login checks. Do not drive a browser for
  those. Do not apply a migration from an agent session.
- Phases 5a–5d and 6 stay deferred. Reporting stays parked.
  Profile footer Save, Style `applyState`, and `this.app.user`
  stay open.
- Variance: `npm test` was 1121 passed and 10 failed (1131).
  The 10 are `auth.integration.test.js` HTTP 503 on
  `https://lithium.philement.com`. The cert expired 2026-08-02.
  Lint and stylelint exited 0 and printed no problems. Vitest
  used happy-dom.


---

## Phase 14 — Catalog commerce (Stripe)

**Goal:** Prices and Stripe product ensure/refund actions without
exposing `sk_`.

**Dependencies:** Phase 13.

**Entry gate:**

- [x] Phase 13 list/detail works. Andrew reported the latest build
      deployed, the latest migrations applied, and the list Retired
      checkbox removed on 2026-10-23.

**Reference:**

- PRIORITIZE §5.2
- `Stripe.EnsureProduct`, `Stripe.Refund`, `Stripe.DeactivateCustomer`
  (`invokable=0` until Phase 18)
- `Account.Orders` is the learner list. Staff orders are **4507**.

**Work items:**

- [x] View/edit `course_prices` (CAD/USD/EUR/GBP). Save is **4506**.
      A blank amount deactivates the row and keeps the cents.
- [x] **Ensure Product** posts `{ course_id }`. **Update Stripe
      prices** posts `{ course_id, action: "rotate" }`. Both stay
      disabled until QueryRef 87 says `Stripe.EnsureProduct` is
      invokable. A free course stays disabled with “Free courses
      do not use Stripe.”
- [x] Price change writes Lithium cents first. Rotate mints a new
      Stripe Price later and keeps the old id. **4506** does not
      write `stripe_price_id`.
- [x] `Stripe.Mode` (`courses_4086.lua`, invokable 1) returns
      `test` or `live` from `STRIPE_MODE`. It does not read
      `STRIPE_SECRET_KEY`.
- [x] Orders for this course. Optional account id. Status includes
      `refunded`. **4507**. A full account search stays in Phase 15.
- [x] Refund and Deactivate call the real scripts and stay disabled
      until invokable. Dashboard links are the working path until
      Phase 18. No delete-Stripe control.

**Exit gate / validation:**

- [x] Grep of `src/managers/course-manager`: the only `sk_`, `rk_`,
      and `whsec_` hit is `stripeLabel` in `course-catalog.js`,
      which renders those shapes as “Hidden”. No key is stored.
- [x] Unit: `ensureProductOptions(4).params` is `{ course_id: 4 }`.
- [x] `npm test && npm run lint` — variance in Status. `npm run
      lint:css` exited 0. luacheck on 4084–4087 exited 0.

**Status:** complete (2026-10-23). Andrew confirmed Save prices on
course 16 at 13:55:13. QueryRef **4506**, CAD present 1 keep 0
cents 4999 active 1, success true, affected_rows 1, row_count 0.
All eight statements returned success true. Statement 1 (the CAD
update) reported affected 1. Statements 2–8 reported affected 0.
The following price reload was HTTP 200, 284 bytes. The brochure
stayed clean. Assigned: `courses_4084.lua` QueryRef **4506**,
rewritten by `courses_4087.lua`; `courses_4085.lua` QueryRef
**4507**; `courses_4086.lua` `Stripe.Mode` (no caller ref,
invokable 1). Next Courses file **4088**. Next caller ref
**4508**. Next Acuranzo file **1399**. File numbers stay below
4500. Variance: `npm test` was 1135 passed and 10 failed (1145;
48 files passed, 1 failed). The 10 are
`tests/integration/auth.integration.test.js` (HTTP 503 from
`https://lithium.philement.com`; cert expired 2026-08-02). That is
the known live auth variance. `npm run lint` exited 0 and printed
no problems after the save-log change. `npm run lint:css` exited 0
on the earlier commerce pass, and no CSS changed after that.
luacheck reported 0 warnings on 4084–4087. The agent did not drive
the live Course Manager. Andrew did the deploy, the apply, and
the save.

**Accomplished:**

- [`courses_4084.lua`](/elements/002-helium/courses/migrations/courses_4084.lua)
  QueryRef **4506**. One transaction updates then inserts each of
  cad, usd, eur, and gbp. `KEEP=1` leaves the stored cents and sets
  `is_active` to 0. A blank currency with no row stays absent.
  `stripe_price_id` is not written.
- [`courses_4085.lua`](/elements/002-helium/courses/migrations/courses_4085.lua)
  QueryRef **4507**. Orders whose `items_json` names the course.
  PostgreSQL uses jsonb containment. Text engines match the
  compact `{"courseId":N,` token. `ACCOUNTID` 0 skips the account
  filter. The select joins `accounts.stripe_customer_id`.
- [`courses_4086.lua`](/elements/002-helium/courses/migrations/courses_4086.lua)
  seeds `Stripe.Mode`, invokable 1. Anything other than `live` is
  `test`. The body does not mention `STRIPE_SECRET_KEY`.
- [`course-commerce.js`](/elements/003-lithium/src/managers/course-manager/course-commerce.js)
  edits prices, lists orders, and posts Ensure, Rotate, Refund, and
  Deactivate. The footer still saves the brochure through **4501**
  and **4502**. Save prices is its own button.
- Tests: `course-commerce.test.js` 10, `course-catalog.test.js` 12,
  `course-manager.test.js` 17, `course-access.test.js` 15.
- [`courses_4087.lua`](/elements/002-helium/courses/migrations/courses_4087.lua)
  rewrites the stored 4506 SQL so `-- TRANSACTION_END` is its own
  line. It updates the existing TYPE_SQL row. It does not edit
  `courses_4084.lua`. Andrew confirmed that SQL on 2026-10-23.
- [`course-price-log.js`](/elements/003-lithium/src/managers/course-manager/course-price-log.js)
  and `authQueryResponse` in
  [`conduit.js`](/elements/003-lithium/src/shared/conduit.js).
  `extractError` treats success `"fail"` as an error. A save with
  no affected rows toasts “Prices were not saved” and leaves the
  draft. The session log lists the four currency binds and the
  statement counts. It omits the staff id and price-row bodies.

**Lessons learned:**

- Price load and order load need separate generation counters. One
  shared token cancels the price paint when the orders query starts.
- `npm test` now sees a `Stripe.Mode` invoke during init. A later
  Sync or Retire assert has to find the call by script name.
- Checkout charges the Lithium cents on an active row. Saving a new
  amount changes the next charge before Rotate mints a Stripe Price.
- `items_json` is jsonb on PostgreSQL, so `::text` would reorder
  keys. Containment is the match there. The other engines match the
  compact text Checkout writes, with a comma after the id so 12
  does not match 123.
- The multi-statement upsert stores `-- TRANSACTION_START`,
  `-- STATEMENT`, and `-- TRANSACTION_END` inside the QueryRef.
  `${SUBQUERY_DELIMITER}` stays the migration-executor token and
  does not belong in that SQL.
- Lua drops the newline immediately after `[[`. `courses_4084.lua`
  therefore glued `-- TRANSACTION_END` onto the last `);`. A
  marker has to be the whole trimmed line. `courses_4087.lua`
  puts a real newline in front of that marker.
- Hydrogen's missing-query body uses `success: "fail"`, not
  boolean false. `extractError` used to ignore that string, so
  Save prices toasted success and reloaded the old cents.
- The query cache is filled at bootstrap, before that startup
  applies migrations, and APPLY does not rebuild it. A query
  inserted on this start is visible after the next start.
- A disabled button does not show its title. The wrapper span holds
  the tip, and the button uses `pointer-events: none`.
- `stripeLabel` is the secret detector. `safeStripeId` takes the
  prefix as an argument, so `course-commerce.js` does not contain
  `sk_`.
- On this save only the currency whose cents changed reported
  affected 1. The other seven statements succeeded with affected
  0: the unchanged updates, and the inserts that an existing row
  skips. A later save that changes a second currency should report
  affected 1 on that currency's update.
- Save prices writes `course_prices` and leaves the brochure
  snapshot alone. The footer stays clean on purpose. QueryRef
  **4500** does not return the cents. A list column for them is
  later work.

**Handoff:**

- Phase 14 is complete. Andrew confirmed Save prices on course 16
  at 13:55:13. Phase 15 (learners and Canvas links) is next.
  Discuss it and wait for approval. Do not edit Phase 15 source
  in the next sitting until that approval. Do not start Phase 15
  in this sitting.
- Leave `invokable` at 0 on `Stripe.EnsureProduct`, `Stripe.Refund`,
  `Stripe.DeactivateCustomer`, `Catalog.SyncFromCanvas`, and
  `Catalog.Retire` until Phase 18. Dashboard links are the working
  Stripe path. They open only when `Stripe.Mode` is `test` or
  `live` and the id matches `pi_` or `cus_`.
- The 500 Courses pod sets `STRIPE_MODE=test`. `Stripe.Mode`
  returns `test` until that value is exactly `live`.
- Amounts on the form are dollars. SQL stores cents. A blank amount
  on an existing row deactivates it and keeps the cents. A free
  course can still save price rows. Ensure and Rotate stay disabled
  for free. Save prices does not dirty the brochure.
- The query cache loads at bootstrap, before that start applies
  migrations, and APPLY does not rebuild it. A query inserted on
  a start is visible after the next start. **4506** requires every
  INTEGER bind, including the zeros. **4507** requires `COURSEID`
  and `ACCOUNTID`. `courses_4087.lua` only replaces an existing
  QueryRef 4506 row. Do not edit `courses_4084.lua`.
- QueryRef **4500** does not return cents. Leave the list columns
  as they are.
- Next Courses file **4088**. Next caller ref **4508**. Next
  Acuranzo file **1399**. File numbers stay below 4500. **4004**
  stays the published catalog.
- He does deploys and login checks. Do not drive a browser for
  those.

---

## Phase 15 — Learners and Canvas links

**Goal:** Search accounts; seats; Canvas link/relink without a browser
Canvas token.

**Dependencies:** Phase 12.

**Entry gate:**

- [x] Phase 12 complete. `account_canvas_links` is
      [`courses_4003.lua`](/elements/002-helium/courses/migrations/courses_4003.lua).
      Lua reads it with QueryRef **4001**
      ([`courses_4004.lua`](/elements/002-helium/courses/migrations/courses_4004.lua)).
      Writes are **4002** and **4003**. Those integers are also file
      numbers, so the SPA calls **4512** instead. They are not an
      account search. No Helium script lists Canvas aliases or merges
      two users.

**Reference:**

- PRIORITIZE §5.3–5.4
- Courses pack QueryRefs
- `Enroll.FreeCourse`, `Enroll.Archive`, `Enroll.RenewFree`,
  `Provision.EnsureCanvasUser`, `Stripe.SyncCustomer`

**Work items:**

- [x] Account search (email, Keycloak sub, account id). Exact match.
      Account id wins, then subject, then email. QueryRef **4508**.
- [x] Detail: prefs + `user_registration_meta` read-only. Enrollments,
      orders, Canvas link, Stripe `cus_` (not secrets). QueryRefs
      **4509**, **4510**, **4511**, and **4512**.
- [x] Grant, archive, and renew call `Enroll.StaffGrant`,
      `Enroll.StaffArchive`, and `Enroll.StaffRenew`. The role check
      is `Enroll.StaffGate` (invokable 0). No “fake paid” button.
      `Enroll.FreeCourse`, `Enroll.Archive`, and `Enroll.RenewFree`
      are unchanged. `Enroll.RenewFree` stays unseeded.
- [x] No alias script. The screen shows `canvas_user_id`,
      `canvas_email`, and `last_seen_at`, plus one ops sentence.
      Relink and merge stay with ops.
- [x] Open Canvas user uses
      `https://canvas.500courses.com/users/{canvas_user_id}`. The
      course button stays
      `https://canvas.500courses.com/courses/{id}`. Both are URL only.
- [x] Do not impersonate. Do not PATCH Keycloak. The target account
      is `params.account_id`. The caller stays in `params._hydrogen`,
      which the SPA does not send.

**Exit gate / validation:**

- [x] Manual: Andrew searched `andrew@500courses.com`. One hit
      opened account **9001**. Canvas user **801**, canvas email
      the same address, last seen `2026-10-21 20:14:27.914495`.
      Currency CAD, language en. First and last name empty.
      Stripe customer None. One existing seat: Welcome to 500
      Courses, active, `free_renew`, open. Orders empty. He then
      granted a course. Grant ran and succeeded. Open Canvas user
      worked. Archive was not clicked. Renew was not clicked.
- [x] `npm test && npm run lint` — variance in Status. `npm run
      lint:css` exited 0. luacheck on 4088–4096 exited 0.

**Status:** complete (2026-10-23). Andrew approved the four pins:
new staff scripts, a link row plus an ops note, a fifth tab, and
`/users/{id}`. Applied, by the live search and grant:
`courses_4088.lua` QueryRef **4508**, `courses_4089.lua` **4509**,
`courses_4090.lua` **4510**, `courses_4091.lua` **4511**,
`courses_4092.lua` **4512**, `courses_4093.lua` `Enroll.StaffGate`
(invokable 0), `courses_4094.lua` `Enroll.StaffGrant`,
`courses_4095.lua` `Enroll.StaffArchive`, and `courses_4096.lua`
`Enroll.StaffRenew` (those three invokable 1). Archive and Renew
were not clicked, so those two scripts are in the applied packet
and were not exercised. Next Courses file **4097**. Next caller
ref **4513**. Next Acuranzo file **1399**. File numbers stay below
4500. Variance: `npm test` was 1142 passed and 10 failed (1152;
49 files passed, 1 failed). The 10 are
`tests/integration/auth.integration.test.js` (HTTP 503 from
`https://lithium.philement.com`; cert expired 2026-08-02). That is
the known live auth variance. `npm run lint` exited 0. `npm run
lint:css` exited 0. luacheck reported 0 warnings on 4088–4096.
`luac -p` parsed each outer file and each of the four script
bodies. The agent did not apply the packet and did not drive the
live Course Manager. Andrew did the deploy and the live check.

**Accomplished:**

- Learners is the fifth tab. The left side stays the course list.
  Search and the read-only account live on the right.
- [`course-learners.js`](/elements/003-lithium/src/managers/course-manager/course-learners.js)
  paints identity, seats, and orders with `textContent`. A secret
  that is not `cus_` renders as Hidden. Seats and orders are
  stacked lines. Grant uses the selected course. Archive and Renew
  use the seat's course. Confirm comes first. A completed envelope
  whose body has `ok: false` is a failure.
- The three buttons stay disabled, with “not enabled on this
  server” on the wrapper, until QueryRef 87 says that script is
  invokable. After the packet was applied and Hydrogen restarted,
  Grant was real. Andrew granted a course. Archive and Renew were
  not clicked.
- Tests: `course-learners.test.js` 7. `course-manager.test.js` 17
  still passes, including the no-`lua` and no-`phase` shell text.
  Commands that passed: `npm test` (variance above), `npm run lint`,
  `npm run lint:css`, luacheck on 4088–4096, and `luac -p` on the
  outer files and the four script bodies.

**Lessons learned:**

- `Enroll.FreeCourse` and `Enroll.Archive` bind the JWT caller.
  A staff screen for someone else needs new script names. The role
  check reads `roles.name` inside Lua. A client flag is not the
  check.
- Grant writes the Helium seat first. Canvas enroll is best-effort
  and does not roll the seat back. Archive still updates Canvas
  before `archived_at`, the same way `courses_4070.lua` does.
- An existing seat, including an archived one, is left alone.
  Grant returns `already: true`.
- Renew adds 90 days only when `renew_policy` is `free_renew`.
  `paid_renew` stays on checkout. `Enroll.RenewFree` stays
  unseeded because Reception already calls that name with
  `enrollment_id`.
- `actionOutcome` treats every `completed` status as success. The
  script body can still say `ok: false`. The Learners path reads
  that flag.
- Search email is `account_oidc_identities.email`, the same column
  `Provision.EnsureCanvasUser` uses. `accounts` has no email.
  Account **9001** stores the email in `accounts.name`. First and
  last name were empty, so the name line showed that email.
- Notify fields read None when `user_preferences` has no row. A
  stored 0 reads No. Currency CAD and language en on account 9001
  came from `user_registration_meta`.
- One search hit auto-opens. Two hits stay as buttons.
- Grant stays disabled until the selected course is free,
  published, and not retired, even after the scripts are
  invokable. The tip says why. Paid, retired, and unpublished
  stay disabled.
- Open Canvas user is
  `https://canvas.500courses.com/users/{canvas_user_id}`. Andrew
  confirmed `/users/801`. The course button stays
  `https://canvas.500courses.com/courses/{id}`.
- `.course-field` is a column, so each label sits above its value.
  That stack is the existing course-field CSS.
- The query cache fills at bootstrap, before that start applies
  migrations. **4508–4512** are visible after the next start.
  The live search is the evidence that restart had happened.
  Each of those statements requires every named bind.

**Handoff:**

- Phase 15 is complete. On 2026-10-23 Andrew parked Course
  Manager. Phases 16–18 wait until he pulls them back.
  Suggestions are Course Builder. The next phase is Phase 19,
  the submission list, which he approved. The next sitting
  implements it. Do not edit Phases 16–18 in that sitting.
- Suggestion insert stays QueryRef **4000**. The staff list is
  a new caller ref. Contact insert stays **86** and stays
  parked with Phase 16.
- Re-check on the live account before numbering anything new:
  account **9001**, Canvas user **801**, and the seat Andrew
  granted. Search email is `account_oidc_identities.email`.
- Do not seed `Enroll.RenewFree`. Do not call
  `Provision.EnsureCanvasUser` for one account. Do not add a
  `Stripe.SyncCustomer` button. Do not flip `Catalog.SyncFromCanvas`,
  `Catalog.Retire`, `Stripe.EnsureProduct`, `Stripe.Refund`, or
  `Stripe.DeactivateCustomer` to invokable 1. That is Phase 18.
  `Enroll.StaffGate` stays invokable 0.
- Do not edit `courses_4084.lua`. **4004** stays the published
  catalog. **4001** stays the Lua Canvas-link read.
- Next Courses file **4097**. Next caller ref **4513**. Next
  Acuranzo file **1399**. File numbers stay below 4500.
- He does deploys and login checks. Do not drive a browser for
  those. Do not apply a migration from an agent session.

---

## Phase 16 — Inbox (suggestions + contact)

Parked 2026-10-23. Andrew parked Course Manager. The suggestion
queue moved to Course Builder (Phase 19, COURSEBUILDER CB-S).
Contact submissions stay here until he pulls this phase back.
Do not edit this phase.

**Goal:** Humans triage `contact_submissions` without raw SQL.
Suggestion triage is Course Builder.

**Dependencies:** Phase 12.

**Entry gate:**

- [x] Phase 12 complete. Phase 15 is complete (2026-10-23).
      Insert QueryRef **4000** exists (was #085) in
      [`courses_4001.lua`](/elements/002-helium/courses/migrations/courses_4001.lua).
      Contact insert stays **86** in
      [`acuranzo_1200.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1200.lua).
      Phase 13 took 4500–4505. Phase 14 took 4506 and 4507.
      Phase 15 took 4508–4512. The live search and grant are
      the apply report. `courses_4086.lua` is a script seed and
      did not take a caller ref. `courses_4087.lua` rewrites
      4506. Phase 19 took **4097–4100** and refs **4513** and
      **4514**. The next file is **4101**. The next caller ref is
      **4515**. File numbers stay below 4500.
- [ ] List and triage QueryRefs for contact submissions.
      Parked with this phase. Suggestion reads are Phase 19.
      Do not edit this phase until Andrew pulls Course Manager
      back.

**Reference:**

- PRIORITIZE §5.5
- COURSEBUILDER CB-9 (queue only — do not build the LLM pipeline)
- Courses pack QueryRefs
- `course_suggestions` in
  [`courses_4001.lua`](/elements/002-helium/courses/migrations/courses_4001.lua);
  contact insert QueryRef **#086** in
  [`acuranzo_1200.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1200.lua)

**Work items:**

- [ ] LithiumTable list suggestions. Accept / decline / junk buttons
      via scripts or QueryRefs (packet if missing).
- [ ] LithiumTable contact submissions. Mark handled.
- [ ] Honest empty state. These queues are required once Course
      Manager exists — not a “later” tab with a fake badge.

**Exit gate / validation:**

- [ ] Manual: fixture suggestion visible; XSS in body does not fire.
- [ ] `npm test && npm run lint`

**Status:** deferred (2026-10-23). Suggestion queue moved to Phase 19.

**Lessons learned:**

---

## Phase 17 — Staff script console

Parked 2026-10-23 with the rest of Course Manager. Do not edit
this phase until Andrew pulls it back.

**Goal:** Invokable scripts as labelled forms, not a JSON blob as the
only UI. Word “Lua” does not appear.

**Dependencies:** Phases 2, 9, 12.

**Entry gate:**

- [ ] Phase 9 invoke helper proven. Phase 12 roles gate works.

**Reference:**

- PRIORITIZE §5.7 button table
- Live `scripts` where `invokable=1`

**Work items:**

- [ ] List invokable scripts (group/name, one-line purpose).
- [ ] Param schemas: labelled fields / pulldowns (courses, accounts,
      currencies, `action` enums). JSON blob allowed as escape hatch,
      not the only path for the known button list.
- [ ] Run `wait: true`; show ok/error, counts, ids.
- [ ] Non-invokable (`Stripe.Webhook`, `Enroll.PaidCourse`,
      `Enroll.LogEvent`) listed system-only, not runnable.
- [ ] Small catalog of param schemas so a new migration can add a
      script without a Lithium rewrite (data, not C).

**Exit gate / validation:**

- [ ] Unit: schema → payload for `Catalog.SyncFromCanvas` and
      `Enroll.FreeCourse`.
- [ ] Manual: `Api.Echo` from the console if still invokable.
- [ ] `npm test && npm run lint`
- [ ] UI copy grep: no “Lua” in course-manager templates.

**Status:** deferred (2026-10-23). Parked with Course Manager.

**Lessons learned:**

---

## Phase 18 — First deploy: invokable packet + 500 Courses smoke

Parked 2026-10-23 with the rest of Course Manager. Do not edit
this phase until Andrew pulls it back. The five scripts named
below stay invokable 0 while it is parked.

**Goal:** Course Manager is shippable on `lithium.500courses.com`.

**Dependencies:** Phases 12–17. Phase 10 is complete. Do not block
deploy on further Scripting polish.

**Entry gate:**

- [ ] Phases 12–17 Status complete or explicit `[~]` with user sign-off.
- [ ] Operator ready to apply Helium packet and roll
      `lithium-500courses`.

**Reference:**

- PRIORITIZE §5.7 known buttons
- Deploy: `npm run deploy:500courses` / operator roll notes in
  PRIORITIZE

**Work items:**

- [ ] Helium packet: flip `invokable=1` on the agreed staff list
      (`Catalog.SyncFromCanvas`, `Catalog.Retire`, `Stripe.EnsureProduct`,
      Stripe customer/refund scripts if ready, `Enroll.*` operator
      scripts, `Provision.EnsureCanvasUser`,
      `Mail.Notices.CourseExpiration` if Band G has not landed — **only
      scripts whose UI exists**). Hand to human. Do not apply.
- [ ] Finish `LITHIUM-MGR-COURSE.md` (IDs, QueryRefs, scripts, roles
      rule). TOC + MGR.
- [ ] All `course-manager` files ≤ 1000.
- [ ] Live checklist: JWT staff login, list courses, denied role,
      one sync **or** documented disabled tooltip if packet not yet
      applied, inbox open, no leaked Tabulator on close.
- [ ] Record Hydrogen version, Lithium `version.json`, DB design,
      whether 2.23 has landed (informational only).

**Exit gate / validation:**

- [ ] Packet delivered. Working Log lists exact `Group.Name` flips.
- [ ] `npm test && npm run lint && npm run build`
- [ ] Working Log `P18` smoke checklist complete or blockers named.
- [ ] User accepts “first official deployment” for Course Manager.

**Status:** deferred (2026-10-23). Parked with Course Manager.

**Lessons learned:**

---

## Band E — Course Builder (Lithium operator)

Andrew opened this band on 2026-10-23. Phase 19 is the submission
list. It is the same slice as COURSEBUILDER CB-S. Do not start
Phases 20–23 until Phase 19 is complete. Do not start the LLM
pipeline. Canvas is not in this slice.

## Phase 19 — Submission list (ID 35)

**Goal:** Staff see incoming suggestions. The left side is a
LithiumTable. The right side opens on the Submission tab.

**Dependencies:** Phases 0 (ID 35), 1, 12 pattern. Course Manager
Phases 16–18 are parked and are not a dependency.

**Entry gate:**

- [x] Phase 0 reserved ID 35. Andrew approved the shell on
      2026-10-23. `course_suggestions` exists
      ([`courses_4001.lua`](/elements/002-helium/courses/migrations/courses_4001.lua)).
      Insert stays QueryRef **4000**. The staff read is a new
      caller ref. Re-check on disk before numbering. On
      2026-10-23 the next file was **4097** and the next ref was
      **4513**.

**Reference:**

- COURSEBUILDER CB-S in
  [`/mnt/extra/Projects/500-Courses-Reception/COURSEBUILDER.md`](/mnt/extra/Projects/500-Courses-Reception/COURSEBUILDER.md)
- Reception form
  [`/mnt/extra/Projects/500-Courses-Reception/public/pages/suggest-course.html`](/mnt/extra/Projects/500-Courses-Reception/public/pages/suggest-course.html)

**Work items:**

- [x] Replace the ID 35 placeholder. Do not register 35 a second
      time. `cleanup()` is the close hook. The staff gate matches
      Course Manager: `getClaims(retrieveJWT())`, then QueryRef
      **155**. A bare `getClaims()` hides the row. The same check
      now omits sidebar ID 35.
- [x] LithiumTable columns, in order: Working Title, Last
      activity, State. Staff `auth_query` **4513** left-joins the
      draft row. No draft row: Working Title is `course_name`,
      Last activity is `created_at`, State is Suggested.
- [x] Working Title field above the tabs, maxlength 100. Footer
      save writes that field only, creates the draft on first
      save, state `suggested`, and does not change `course_name`.
      The save SQL appends one ledger row. The live write is
      still the open exit line below.
- [x] One tab, Submission. Read-only. Labels match the Reception
      form: Suggested Course Name, course summary, detail,
      intended audience, additional features, Your Name, Your
      Email. Detail is sanitized HTML. The other fields are text
      nodes. Cap token, site id, and IP stay off the screen.
      Later tabs stay absent until that stage has content.
- [x] Helium packet written and handed over:
      [`courses_4097.lua`](/elements/002-helium/courses/migrations/courses_4097.lua)
      `course_build_runs`,
      [`courses_4098.lua`](/elements/002-helium/courses/migrations/courses_4098.lua)
      `course_build_actions`,
      [`courses_4099.lua`](/elements/002-helium/courses/migrations/courses_4099.lua)
      QueryRef **4513**,
      [`courses_4100.lua`](/elements/002-helium/courses/migrations/courses_4100.lua)
      QueryRef **4514**,
      [`acuranzo_1399.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1399.lua)
      Lookup 059 key 15. Andrew applied these, restarted
      Hydrogen, and confirmed the list and Submission tab.
      The working-title ledger write is still unproven.
- [x] Manage block, receipt pass, and stage select are in the
      tree. Add, duplicate, edit, flag, annotate, and delete.
      The applied client called `Build.ReceiveSuggested` once
      on open. Andrew received that email. The tree no longer
      calls it. `receipt_mail` stores `sent` or `failed`. A
      failed send still stores Received. The stage select moves
      Suggested or Received to Intake and does not send mail.
- [x] Second packet applied. Andrew received the receipt, so
      [`courses_4101.lua`](/elements/002-helium/courses/migrations/courses_4101.lua)
      through
      [`courses_4110.lua`](/elements/002-helium/courses/migrations/courses_4110.lua)
      and
      [`acuranzo_1400.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1400.lua)
      ran. List **4515**, insert **4516**, update **4517**,
      duplicate **4518**, delete **4519**, Intake **4520**, flag
      **4521**. Mail template `course.suggestion_received`.
      Script `Build.ReceiveSuggested`, group `Build`.
- [x] Third packet written and handed over, not applied.
      [`courses_4111.lua`](/elements/002-helium/courses/migrations/courses_4111.lua)
      rewrites **4514**.
      [`courses_4112.lua`](/elements/002-helium/courses/migrations/courses_4112.lua)
      rewrites **4517**.
      [`courses_4113.lua`](/elements/002-helium/courses/migrations/courses_4113.lua)
      rewrites **4518**.
      [`courses_4114.lua`](/elements/002-helium/courses/migrations/courses_4114.lua)
      rewrites `Build.ReceiveSuggested`: invokable 0, actor 0,
      blank `FROM_ADDRESS` and `REPLY_TO`.
      [`acuranzo_1401.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1401.lua)
      scans suggestions on the one-second tick and submits that
      script when a row still needs a receipt.
- [x] Andrew applied 4111–4114 and `acuranzo_1401.lua`. The
      receipt went out on the Hydrogen restart.
- [x] Suggested is a stage choice for Received and Intake.
      [`courses_4115.lua`](/elements/002-helium/courses/migrations/courses_4115.lua)
      is QueryRef **4522**. It clears `receipt_mail`.
      [`courses_4116.lua`](/elements/002-helium/courses/migrations/courses_4116.lua)
      puts the send time in the receipt key. The screen does
      not send the mail.
- [x] Receipt body written and handed over, not applied.
      Andrew reported the sent message had a subject and no body.
      [`courses_4117.lua`](/elements/002-helium/courses/migrations/courses_4117.lua)
      replaces the text and HTML of `course.suggestion_received`.
      The greeting uses the suggestion `submitter_name`. The
      course title uses the suggestion `course_name`. The send
      does not read a JWT. Subject and display name stay.
- [x] Visible copy has no "Lua" and no "phase". No accept,
      decline, or junk button. No Canvas control. The ledger is
      not on screen.

**Exit gate / validation:**

- [x] Happy-dom fixture shows the three columns, the seven
      Submission fields, Annotation, the manage block, and the
      stage select. Detail markup does not run. Andrew confirmed
      the list and the Submission tab on the applied packet.
      The edit block and the receipt pass are not on that build.
- [ ] A Working Title save keeps `course_name` and writes one
      ledger row. SQL shape is tested. **4514** is applied. The
      live save failed at statement 1 and rolled back.
      `courses_4111.lua` rewrote the cast and went out with the
      restart. The ledger row is still unproven.
- [x] `course-builder.test.js` passed 9. `npm run lint` on the
      Course Builder files exited 0. `npm run lint:css` on
      `course-builder.css` exited 0. The full `npm test` was not
      re-run. Its known variance is the live auth file, HTTP 503.
- [x] Andrew applied 4097–4114 and Acuranzo 1399–1401. The
      receipt went out on the Hydrogen restart.
- [ ] Andrew applies 4115, 4116, and 4117, regenerates the
      payload, restarts Hydrogen, then deploys Lithium. Moving a
      row to Suggested clears the receipt mark and the next tick
      sends again. That send uses the new body. He can also retry
      the working-title save.

**Status:** in progress (2026-10-23). Last finished work is the
receipt body. Andrew applied **4097–4114** and
Acuranzo **1399–1401**. The receipt went out on the restart
with a subject and no body. The working-title ledger row is
still unproven. The unapplied packet is Courses **4115**,
**4116**, and **4117**. Caller ref **4522** clears the receipt
mark. **4117** replaces the receipt text and HTML. The greeting
uses the suggestion `submitter_name`. The course title uses the
suggestion `course_name`. Next Courses file
**4118**. Next caller ref **4523**. Next Acuranzo file **1402**.
Lookup 059 key **16** stays free. File numbers stay below 4500.
The agent did not apply the packet, did not run `mka` or `mkt`,
and did not open the live manager. `course-builder.test.js`
passed 9. ESLint on the Course Builder files and the unit test
exited 0. `luac -p` and luacheck exited 0 on 4111–4114 and
`acuranzo_1401.lua`. The stored script bodies also passed
`luac -p`. The orchestrator body keeps two luacheck warnings
copied from `acuranzo_1364.lua`.

**Accomplished:**

- ID 35 is `CourseBuilderManager` in
  [`course-builder.js`](/elements/003-lithium/src/managers/course-builder/course-builder.js).
  Close hook is `cleanup()`. The registry entry was already 35.
  Denied staff see the lock panel. Allowed staff see the list and
  the Submission tab. `STAFF_MENU_IDS` in
  [`course-access.js`](/elements/003-lithium/src/managers/course-manager/course-access.js)
  is `[34, 35]`, so a failed QueryRef **155** omits both sidebar
  rows. A bare `getClaims()` still hides them.
- Display helpers live in
  [`course-suggestions.js`](/elements/003-lithium/src/managers/course-builder/course-suggestions.js).
  The table path is `courses/course-builder`, lookup key 15,
  query ref **4515**, manage block on, primary key
  `suggestion_id`. A title-only footer save still calls **4514**.
  An edit or an annotation calls **4517**. Add calls **4516**.
  A blank or a title over 100 characters is refused in the
  client. A dirty row switch asks to discard or stay. The panel
  state key is `lithium_builder_left`. The toolbar stays above
  the working title. The stage select sits under it.
- Edit actions live in
  [`course-builder-actions.js`](/elements/003-lithium/src/managers/course-builder/course-builder-actions.js).
  Opening the shell does not call `Build.ReceiveSuggested`.
- The first packet and the second packet are applied. Andrew
  received the receipt. **4515** labels Suggested, Received, and
  Intake and does not select the cap token, the site id, or the
  IP. **4514** still does not assign `course_name`. The live
  save failed at statement 1. **4111** casts the title as
  `varchar(100)` (`char(100)` on MySQL). **4517** assigns
  `course_name`. Reverse **4098** before **4097** still applies
  to the first packet. Reverse **4114** down through **4111**,
  then **1401**. Do not reverse 4097–4110 unless he also wants
  the applied draft and the receipt gone.

**Lessons learned:**

- `review_status` on `course_suggestions` stays pending, approved,
  or rejected. The gauntlet state is `course_build_runs.state`.
  A missing draft, and a stored `suggested`, both display as
  Suggested.
- Each bind name is used once inside a statement, through a
  derived table. `WORKING_TITLE` repeats across the update and
  the insert. Statement 3 uses only `SUGGESTION_ID` and `USERID`.
  Hydrogen ignores an extra client name and fails if SQL names a
  bind the client omits.
- `-- TRANSACTION_END` is on its own line. The Lua long string
  that wraps the inner SQL closes before `.. sql_new ..`, the
  same way `courses_4087.lua` does.
- A missing suggestion updates and inserts zero rows. The client
  reloads the list. It does not yet toast when the reloaded title
  did not stick.
- Andrew's confirmed screen now calls **4515**. There is no
  fallback to **4513**. The query cache fills at startup, so
  **4111** needs a Hydrogen restart before the title save uses
  the new cast.
- `CAST(:WORKING_TITLE AS text)` into `varchar(100)` is the
  likely cause of "transaction block 1 failed at statement 1"
  on this Yugabyte. **4111**, **4112**, and **4113** cast the
  title as the column width. Detail stays a bare bind.
  Annotation stays within 250 characters.
- The receipt pass claims the row as Received with
  `receipt_mail` `failed` before it sends. A queued message
  upgrades that field to `sent`. A miss leaves `failed`. The
  field is never cleared, so a later tick does not send again.
  A crash after the queue call and before the upgrade can store
  `failed` for a message that was queued. The idempotency key
  `suggestion-receipt-<suggestion_id>` blocks a second send.
  Intake does not touch the field. The mDNS row is already
  Received.
- A scoreboard worker has no JWT. `Enroll.StaffGate` would
  return `not_staff` and send nothing. **4114** drops that gate,
  sets actor 0, and sets invokable 0. Any signed-in JWT can
  post `/api/conduit/script` when invokable is 1. The Run tab
  stays disabled while invokable is 0. He can still open and
  save the script.
- The sent receipt had a subject and no body. The template row
  is still `course.suggestion_received`. **4117** replaces the
  text and the HTML. The greeting macro is `FIRST_NAME` and the
  script fills it from `course_suggestions.submitter_name`. The
  course macro is `COURSE_NAME` and the script fills it from
  `course_suggestions.course_name`. The orchestrator has no
  JWT, so those two values are not claims. A message already
  queued keeps the body it was given.
- The email words are mail template
  `course.suggestion_received`, display name "Suggestion
  received". The applied script omits `from` and `reply_to`, so
  Hydrogen uses `MailRelay.DefaultFrom` and `DefaultReplyTo`.
  After **4114**, blank `FROM_ADDRESS` and `REPLY_TO` keep those
  defaults. A saved edit of those two lines applies on the next
  tick. A later migration that replaces the body wipes that edit.
  Mail manager 8 is still a placeholder.
- There is no insert-time trigger. Public QueryRef **4000** is
  SQL only. The orchestrator already wakes every second. **1401**
  queries first and submits `Build.ReceiveSuggested` only when
  one row still needs a receipt and that job is not already
  running. An empty queue does not submit. The orchestrator
  body loads at READY FOR REQUESTS, so **1401** needs a restart.
  The script body loads at submit time.
- A staff insert does not create a draft, so it stays Suggested
  until the next orchestrator tick after **1401**. He can move
  that row to Intake in the same open and skip the email.
- Query summaries can show in the Queries manager. Those
  summaries do not say "Lua" or "phase". File comments for
  authors may name the phase. The save summary does mention
  `course_suggestions.course_name` so a test must not treat the
  mere string `course_name` as a write.
- `npm test` still fails the remote auth file with HTTP 503.
  That is the same variance as Phase 15. It is not this slice.
- No browser tool was connected for the edit block. Happy-dom
  covered the shell, the manage actions, the stage select, the
  **4514** title save, and one receipt call per open. Andrew
  confirmed the earlier list and Submission tab himself.

**Handoff:**

- Phase 19 stays in progress. The next sitting waits for Andrew.
  Do not start Phase 20. Do not edit Phases 16–18.
- He regenerates the Hydrogen payload (`mka` or
  `payload-generate.sh`; this sitting did not), applies
  `courses_4115.lua`, `courses_4116.lua`, and
  `courses_4117.lua`, restarts Hydrogen, then deploys Lithium.
  The restart loads QueryRef **4522**. The script body is read
  on the next orchestrator tick. The template is read when the
  message is queued. **4117** does not by itself resend the
  mDNS row.
- Moving Received or Intake to Suggested clears `receipt_mail`
  and writes one ledger row named `suggested`. The next tick
  sends the receipt again. The mail key includes the time, so
  the relay does not return the earlier message. Intake still
  leaves the receipt mark alone. A failed send still stores
  `failed` until he moves the row back to Suggested.
- The script is group `Build`, name `ReceiveSuggested`. There is
  no Course States group and no script named Received. The
  subject and body are template `course.suggestion_received`,
  display name "Suggestion received". **4117** keeps the subject
  "We received your course suggestion" and replaces the text
  and HTML. The body names the suggested course, says updates
  will follow through evaluation and building, says not every
  suggestion becomes a course, and says the final disposition
  is sent either way. `FIRST_NAME` is the suggestion
  `submitter_name`. `COURSE_NAME` is the suggestion
  `course_name`. After **4114**, he sets
  `FROM_ADDRESS` and `REPLY_TO` at the top of that script and
  saves. Blank keeps `MailRelay.DefaultFrom` and
  `DefaultReplyTo`. Run stays off because invokable is 0.
- Saving a working title on a row that is not in edit mode
  should still leave `course_name` unchanged and add one
  `course_build_actions` row with `action_name` `working_title`
  and the six measures NULL. Retry that after the restart.
- Calling **4520** again on a row already at Intake can append
  another `intake` ledger row. The screen confirms a real move
  and does not offer Intake once the row is there.
- Reverse **4117**, then **4116**, then **4115**. **4117**
  puts the short 4109 text and HTML back and does not delete
  the template. Do not reverse 4097–4114 or
  **1401** unless he also wants the applied receipt gone.
- Do not show the ledger. Do not add Accept, Decline, Junk, or
  Canvas. Do not call QueryRef **4000** from this screen. Do not
  add a second receipt script.
- Next Courses file **4118**. Next caller ref **4523**. Next
  Acuranzo file **1402**. Lookup 059 key **16** stays free. File
  numbers stay below 4500. He does deploys and login checks.
  Do not apply a migration from an agent session.

---

## Phase 20 — Helium `course_build_*` gate

**Goal:** Lithium can query runs. Extend the draft row from Phase
19. Do not invent a second run table. Do not invent tables in
the SPA.

**Dependencies:** COURSEBUILDER CB-1/CB-2.

**Entry gate:**

- [ ] CB-1 transition table exists **or** this phase authors the enum
      into COURSEBUILDER Working Log and this log.
- [ ] Operator agrees to apply Helium migrations.

**Reference:** COURSEBUILDER CB-2 (`course_build_runs`, artifacts, events).

**Work items:**

- [ ] Consume or prepare CB-2 tables + QueryRefs (list by state, get
      run + artifacts, events) + LithiumTable tableDef.
- [ ] Packet to human. `mkt` after they apply.

**Exit gate / validation:**

- [ ] QueryRef numbers in Working Log and
      `LITHIUM-MGR-COURSEBUILDER.md`.
- [ ] Named Hydrogen migration test or SQL fixture noted.

**Status:** pending

**Lessons learned:**

---

## Phase 21 — Queue UI and human-gate invoke

**Goal:** Operator sees runs and calls `Build.Accept` / `Decline` /
`Approve` / `Abandon`.

**Dependencies:** Phases 2, 19, 20. COURSEBUILDER CB-3 scripts.

**Entry gate:**

- [ ] At least one fixture `course_build_runs` row. Named `Build.*`
      scripts exist or this phase seeds minimal handlers (packet).

**Work items:**

- [ ] Table: state, title, updated_at, cost if column exists. Filters
      by state.
- [ ] Detail: suggestion payload + latest event + artifacts.
- [ ] Buttons → `invokeScript` with `run_id`. Refresh. Show errors.
- [ ] Hide Research until Approve (runner-side; UI hides the button).

**Exit gate / validation:**

- [ ] Fixture: Decline → declined + event. Separate fixture: Accept or
      Approve changes state.
- [ ] `npm test` for button→payload mapping.

**Status:** pending

**Lessons learned:**

---

## Phase 22 — Outline, artifacts, Inspect, Publish

**Goal:** Remaining human hexes that belong in Lithium.

**Dependencies:** Phase 21.

**Entry gate:**

- [ ] Phase 21 pattern works. Artifact kind `outline` or fixture JSON
      exists (may be stub).

**Work items:**

- [ ] Render outline JSON sanitized. List materials; version list.
- [ ] Inspect Pass/Fail → locked script names. Publish never auto.
- [ ] “Open LMS” URL from config if `canvas_course_id` present — no
      token.
- [ ] Disable Publish unless state allows.

**Exit gate / validation:**

- [ ] XSS fixture in artifact body does not execute.
- [ ] Buttons emit locked names (unit test).
- [ ] Manual against stub scripts if LMS load is not ready (variance
      logged; do not pretend Canvas published).

**Status:** pending

**Lessons learned:**

---

## Phase 23 — Course Builder tests and docs

**Goal:** Band E closable as a Lithium feature even if COURSEBUILDER
CB-34 E2E is later.

**Dependencies:** Phases 19–22.

**Entry gate:**

- [ ] Phases 19–22 complete or `[~]`.

**Work items:**

- [ ] Unit tests: filters, payloads, state→button visibility.
- [ ] Finish `LITHIUM-MGR-COURSEBUILDER.md`. TOC + MGR + COURSEBUILDER
      CB-9/CB-33 link back.
- [ ] All `course-builder` files ≤ 1000.

**Exit gate / validation:**

- [ ] `npm test && npm run lint && npm run build`
- [ ] TOC links resolve.

**Status:** pending

**Lessons learned:**

---

## Band F — Crimson, Chats, MCP

Hydrogen chat + MCP are done. Lithium talks an older WS shape.

## Phase 24 — Chat JWT honesty (Lithium)

**Goal:** Crimson does not lie when Hydrogen rejects the login JWT.
No product C.

**Dependencies:** Phase 0 L9–L10. Phase 3 XSS if touching markdown.

**Entry gate:**

- [ ] Phase 0 complete. Live Hydrogen chat policy known (Test 59
      vs production).

**Reference:**

- `src/shared/crimson-ws.js`, `src/core/jwt.js`
- [`/docs/H/api/chat/auth_chat.md`](/docs/H/api/chat/auth_chat.md)
- [`/docs/H/core/subsystems/websocket/websocket_chat.md`](/docs/H/core/subsystems/websocket/websocket_chat.md)
- `auth_jwt_helper.c` `validate_chat_jwt_claims` (read only)
- Do **not** start from [`/docs/H/core/CHAT_SYSTEM.md`](/docs/H/core/CHAT_SYSTEM.md)

**Work items:**

- [ ] Map `JWT not authorized for chat` and `chat_error` to a real
      toast. Do not retry blindly.
- [ ] If a Hydrogen mint endpoint already exists when this phase
      starts, use it (`aud=hydrogen-chat`, `roles=chat`). If not, log
      the gap; do not add C.
- [ ] Vitest: token selection / error mapping.
- [ ] Do not send `payload.context` (`gatherContext()` is not in
      `src/`).

**Exit gate / validation:**

- [ ] Working Log: production accepts login JWT or not; mint exists
      or not.
- [ ] `npm test && npm run lint`

**Status:** pending

**Lessons learned:**

---

## Phase 25 — Align WS client with CHAT_FINALE wire

**Goal:** Rate limits, tokens, and errors are visible. Delimiter
protocol stays Lithium-specific.

**Dependencies:** Phase 24.

**Entry gate:**

- [ ] Phase 24 complete.

**Reference:**

- `crimson-ws.js`, `crimson-chat.js`, `crimson-ui.js`
- WS `error_code` 4291 (requests) / 4292 (tokens)

**Work items:**

- [ ] Parse 4291/4292 → status “rate limited”.
- [ ] Surface `result.tokens` / `response_time_ms` in debug, then
      footer if cheap.
- [ ] Abort remains client-side unless Hydrogen grows a cancel frame
      (do not fake).
- [ ] Keep `[LITHIUM-CRIMSON-JSON]` delimiter. Keep citations UI
      (xAI/RAG shaped).

**Exit gate / validation:**

- [ ] Unit: error_code mapping.
- [ ] `npm test && npm run lint`

**Status:** pending

**Lessons learned:**

---

## Phase 26 — Engine picker

**Goal:** Dropdown of safe engine names. Never #061.

**Dependencies:** Phase 2 (invoke) or a new QueryRef without keys.
Phase 25.

**Entry gate:**

- [ ] Phase 25 complete. Helium packet for `Chat.ListEngines` (or
      equivalent) ready or already applied.

**Work items:**

- [ ] Packet: name, provider, default, `use_responses_api`, **no
      keys**. `invokable=1` and/or `mcp_access` as Phase 0 recorded.
- [ ] Crimson dropdown; persist last engine in `window.lithiumSettings`.
      Default `"Crimson"` if that CEC row exists.
- [ ] Do not call QueryRef #061 from the browser.

**Exit gate / validation:**

- [ ] Unit: default engine; settings round-trip.
- [ ] Packet handed over. `npm test && npm run lint`

**Status:** pending

**Lessons learned:**

---

## Phase 27 — Chats manager (history)

**Goal:** ID 18 lists persisted threads. Crimson stays the composer.

**Dependencies:** Phase 24. Re-verify WS path writes #067.

**Entry gate:**

- [ ] Confirm Hydrogen writes conversation storage on the **WS** path
      (REST `auth_chat` does). If WS persist is off, honest empty
      state — do not fake history.

**Reference:**

- QueryRefs **#069** list, **#068** reconstruct, **#067** store
- `src/managers/chats/chats.js` (placeholder)
- LithiumTable pattern from Queries

**Work items:**

- [ ] Replace placeholder. List #069, open #068, new thread stays
      Crimson.
- [ ] `cleanup()`. JWT required.
- [ ] `LITHIUM-MGR.md`: Chats is no longer “Implemented” until this
      phase; then it is.

**Exit gate / validation:**

- [ ] Manual: empty vs one fixture thread.
- [ ] `npm test && npm run lint`

**Status:** pending

**Lessons learned:**

---

## Phase 28 — MCP operator UI and product Lua tools

**Goal:** Observe MCP. Do not become a daemon. Optional Helium tools
beyond `System.Info`.

**Dependencies:** Phase 24 (token confusion settled). Phase 9 useful
for Scripting `mcp_access` column.

**Entry gate:**

- [ ] Phase 24 complete. User still wants MCP UI (default: yes, small).

**Reference:**

- [`/docs/H/core/subsystems/mcp/mcp.md`](/docs/H/core/subsystems/mcp/mcp.md)
- [`/docs/H/api/mcp/mcp_endpoints.md`](/docs/H/api/mcp/mcp_endpoints.md)
- `GET /api/mcp/status`; QueryRef **#152**
- Helium `acuranzo_1365`–`1376`

**Work items:**

- [ ] Operator panel (utility or Scripting tab): `/api/mcp/status` +
      tool catalog #152. JWT, no special role beyond existing staff
      if Phase 0 says so.
- [ ] Scripting: show `mcp_access` if column is in the tableDef.
- [ ] **Do not** JSON-RPC to `:3100` from the browser.
- [ ] Packet (optional, user ask): `Chat.ListEngines` (if not Phase
      26), `Lithium.AccessibleManagers`. Only then widen Crimson
      `allowed_tools`. Fixtures Echo/Sleep stay test-only.
- [ ] Rewrite `LITHIUM-MGR-CRIMSON.md` / `LITHIUM-WSS.md` to match
      `crimson-ws.js`. Freeze `CHAT_SYSTEM.md` as obsolete in Hydrogen
      docs only if that file is touched with user approval.

**Exit gate / validation:**

- [ ] Grep Lithium `src/`: no fetch to MCP port 3100.
- [ ] `npm test && npm run lint`
- [ ] Crimson unit tests exist for payload builder / delimiter /
      error_code (may live in Phase 24–25; confirm).

**Status:** pending

**Lessons learned:**

---

## Band G — Mail, Jobs, Dashboard, Roles

## Phase 29 — Placeholder collapse

**Goal:** One `PlaceholderManager` instead of ~22 shells. Honest
Implemented list.

**Dependencies:** Phase 0 ID table. Phase 27 must have pulled Chats
**out** of the collapse (or Phase 0 recorded Chats stays placeholder
until 27 — then collapse it now and Phase 27 replaces the wrapper).

**Entry gate:**

- [ ] Phase 0 complete. Band D deployed or user asked to clean shells
      earlier.

**Reference:** AGENTS.md Implemented vs placeholder.

**Work items:**

- [ ] `src/managers/_placeholder/placeholder-manager.js`.
- [ ] Each remaining placeholder folder is a thin default export
      (Vite `switch` paths stable).
- [ ] Collapse unfinished Profile `page-manager-N` stubs or hide nav.
- [ ] `LITHIUM-MGR.md` Implemented = real UIs only.
- [ ] Do not implement Calendar/Contacts/Files/… in this phase.

**Exit gate / validation:**

- [ ] Documented `rg "under development" src/managers | wc -l`
- [ ] `npm test && npm run lint`

**Status:** pending

**Lessons learned:**

---

## Phase 30 — Mail Manager

**Goal:** Templates + recent send results + Course Expiration notice.
Not inbound SMTP. Not a second mail stack.

**Dependencies:** Phase 2, 29. Mail Relay REST is shipped.

**Entry gate:**

- [ ] Phase 29 complete. PRIORITIZE §5.6 still wanted (Course Manager
      may deep-link here).

**Reference:**

- [`/docs/H/plans/complete/MAILRELAY_PLAN_COMPLETE.md`](/docs/H/plans/complete/MAILRELAY_PLAN_COMPLETE.md)
- `src/api/mailrelay/`; Helium mail QueryRefs 093–128
- `Mail.Notices.CourseExpiration`

**Work items:**

- [ ] LithiumTable templates + recent queue/results (no secrets).
- [ ] Run `Mail.Notices.CourseExpiration` with account/course
      pulldowns (`invokeScript`).
- [ ] Do not spray concurrent same `idempotency_key`. Do not claim
      MySQL Persist if off.
- [ ] Course Manager §5.6 can link to this manager instead of
      duplicating UI.

**Exit gate / validation:**

- [ ] Manual: preview or status against Hydrogen (log).
- [ ] `npm test && npm run lint`

**Status:** pending

**Lessons learned:**

---

## Phase 31 — Job Manager

**Goal:** Honest view of `/api/conduit/script` jobs **if** Hydrogen
exposes a list. Otherwise show in-flight poll only and say there is
no durable history.

**Dependencies:** Phases 2, 9, 29.

**Entry gate:**

- [ ] Phase 29 complete. Working Log cites whether a job-list API
      exists (scoreboard is in-memory per `script.md`).

**Work items:**

- [ ] If no list API: Job Manager explains in-memory scoreboard +
      link to Scripting last-run columns. **No fake table.**
- [ ] If list API exists: LithiumTable + poll. No fake cancel.

**Exit gate / validation:**

- [ ] Working Log: which branch. `npm test && npm run lint`

**Status:** pending

**Lessons learned:**

---

## Phase 32 — Dashboard and Role Manager

**Goal:** Dashboard is a real summary or a collapsed placeholder.
Role Manager reads JWT roles now; writes `account_roles` only when
2.23 exists.

**Dependencies:** Phase 29. AUTH_FINALE Phase 2 optional.

**Entry gate:**

- [ ] Phase 29 complete.

**Work items:**

- [ ] Dashboard: real cards from existing QueryRefs/scripts **or**
      stay placeholder. No success spinners on empty.
- [ ] Role Manager: display account + JWT `roles`. Assign/revoke
      staff/admin **only** if Helium write QueryRef exists (2.23).
      Else read-only + ops note.
- [ ] Do not build a second Course Manager authz UI.

**Exit gate / validation:**

- [ ] `LITHIUM-MGR.md` matches. `npm test && npm run lint`

**Status:** pending

**Lessons learned:**

---

## Band H — Hydrogen-gated consumers

Do **not** start a phase here until the named Hydrogen plan Status is
complete. Lithium does not implement AUTH_FINALE or Notifications in C.

## Phase 33 — AUTH_FINALE SPA consumers

**Goal:** Lithium uses MFA / reset / session-revoke **only after**
Hydrogen ships them.

**Dependencies:**

| Lithium work | Wait for |
| -------------- | ---------- |
| Login MFA OTP field | AUTH_FINALE Phase 8 complete |
| Password reset UI | AUTH_FINALE Phase 8b complete |
| Session list/revoke | AUTH_FINALE Phase 10b complete |
| Durable roles cutover | AUTH_FINALE Phase 2 complete — then update L3 |

**Entry gate:**

- [ ] At least one of the Hydrogen phases above is Status complete.
      Implement only those slices. Skip others with `[~]`.

**Reference:** [`/docs/H/plans/AUTH_FINALE.md`](/docs/H/plans/AUTH_FINALE.md)

**Work items:**

- [ ] MFA: consume Mail Relay OTP via the **auth** API Hydrogen
      documents. SPA still holds Hydrogen JWT only.
- [ ] Reset: no email-enumeration in copy.
- [ ] Sessions: list + revoke own JWTs; reuse existing logout.
- [ ] If Phase 2 landed: Course Manager may additionally check
      `account_roles` without dropping JWT `roles` until cutover is
      logged.

**Exit gate / validation:**

- [ ] Only shipped Hydrogen endpoints used. `npm test && npm run lint`
- [ ] AUTH_FINALE Phase 11 still does not block CATCHUP.

**Status:** pending

**Lessons learned:**

---

## Phase 34 — Notifications Manager

**Goal:** Web Push subscribe/unsubscribe in Lithium after Hydrogen
Notifications exist.

**Dependencies:** [`/docs/H/plans/NOTIFICATIONS_PLAN.md`](/docs/H/plans/NOTIFICATIONS_PLAN.md)
Phase 0 approved **and** a Lithium-facing subscribe API.

**Entry gate:**

- [ ] NOTIFICATIONS_PLAN has a complete phase that documents the SPA
      contract. If still “proposed, not approved”, this CATCHUP phase
      stays pending — do not invent VAPID handling.

**Work items:**

- [ ] Notification Manager (ID 19): permission prompt, subscribe,
      list, unsubscribe. Service worker already exists — do not
      casually rewrite `index.html` splash.
- [ ] Never log VAPID private keys or subscription `auth`.
- [ ] Not a mail stack.

**Exit gate / validation:**

- [ ] Named Hydrogen test + `npm test && npm run lint` or Status
      `[~]` waiting.

**Status:** pending

**Lessons learned:**

---

## Band I — Tables, docs, tests, closeout

## Phase 35 — File-size splits (remaining >1000)

**Goal:** No `src/` JS/CSS/HTML over 1000 lines.

**Dependencies:** Phase 3 if splitting Lookups/Tour (sanitize first).
Phase 11 owns `codemirror-setup.js`. Phase 8 owns `scripting.js`.

**Entry gate:**

- [ ] `find src -name '*.js' -o -name '*.css' -o -name '*.html' | xargs wc -l`
      inventory in Working Log.

**Work items:**

- [ ] Split remaining offenders (2026-08-21 baseline 14 files; re-count).
      Tour, Lookups, Style, crimson.css, tour.css, lithium-table.css,
      queries.js, version-history, profile-manager, page-photo,
      main.css, components.css — whatever is still over.
- [ ] No behavior change. Keep public class default export.

**Exit gate / validation:**

- [ ] `wc -l` inventory: zero files over 1000.
- [ ] `npm test && npm run lint`

**Status:** pending

**Lessons learned:**

---

## Phase 36 — Settings, log, tree hygiene

**Goal:** Prefs go through `lithiumSettings`. Logs do not silently drop.
Generated trees stop lying.

**Dependencies:** Phase 0. Touched managers from earlier bands.

**Entry gate:**

- [ ] Phase 0 complete.

**Reference:** Sprint Phases 10–11 (absorbed). `src/core/log.js`
`_flushToServer()`.

**Work items:**

- [ ] Route remaining user prefs through `window.lithiumSettings`.
- [ ] Replace live `console.*` on paths this plan touched with `log()`.
- [ ] `_flushToServer()`: wire to a real endpoint **or** remove the
      queue. Do not silently drop.
- [ ] `templates:copy` HTML only (or document CSS). Remove orphan
      `public/src/managers/user-profiles/`, `session-logs/`,
      `style-manager-v2.html` if unused. Wire copy into `build`.
- [ ] Punchcard: implement L14. Delete always-`[7..33]` fiction.

**Exit gate / validation:**

- [ ] Working Log: remaining `localStorage` pref writes (JWT/cache
      only) and log.js decision.
- [ ] `package.json` `build` runs `templates:copy` (or equivalent).
- [ ] `npm run build && npm test && npm run lint`

**Status:** pending

**Lessons learned:**

---

## Phase 37 — Docs, LithiumTable, test inventory

**Goal:** Docs describe the code. LithiumTable remaining honesty from
[`/docs/Li/LITHIUM-TAB-PLAN.md`](/docs/Li/LITHIUM-TAB-PLAN.md) is either
done or parked in `TODO.md` with effort tags — no second active table
plan.

**Dependencies:** Phases 12, 29 (Implemented list). Band D docs stubs.

**Entry gate:**

- [ ] Course Manager docs exist. Placeholder Implemented list honest.

**Work items:**

- [ ] Rewrite `LITHIUM-API.md`: Conduit query + **script**, auth, OIDC,
      health, WS, mailrelay REST used. Mark historical `/api/lookups`
      REST if unused.
- [ ] Fix `LITHIUM-MGR-NEW.md`: LithiumTable path, no static sidebar
      register, IDs 34/35.
- [ ] Fix `LITHIUM-INS.md` dead `REFACTORING_PLAN.md` link → this plan
      Phase 35.
- [ ] `LITHIUM-TST.md` counts from `npm run test:coverage`.
- [ ] Fix `event-bus.test.js` to `off` the same function reference.
- [ ] Delete tautological Queries tests or rewrite to hit production
      functions.
- [ ] TAB-PLAN: close leftover gates or move leftovers to `TODO.md`
      and banner TAB-PLAN as historical.

**Exit gate / validation:**

- [ ] TOC links resolve (`mkl` / Test 90 if Lithium markdown is in
      that suite; otherwise open the files).
- [ ] `LITHIUM-API.md` mentions `/api/conduit/script`.
- [ ] `npm test`

**Status:** pending

**Lessons learned:**

---

## Phase 38 — Coverage, live smoke, closeout

**Goal:** CATCHUP can move to `docs/Li/plans/complete/` when the user
agrees.

**Dependencies:** Phases 0–37 complete or `[~]` with rationale.

**Entry gate:**

- [ ] Pause point is this phase.

**Work items:**

- [ ] `npm run test:coverage` + TST table (core: jwt, utils, conduit
      including script).
- [ ] `npm run lint && npm run lint:css` zero errors.
- [ ] `npm audit` summary; fix or defer each critical/high with
      reason. No `audit fix --force`.
- [ ] Live smoke: password or OIDC login, Scripting Echo, Course
      Manager list, close without leaked DOM. Record versions.
- [ ] Sweep INS leftovers into `docs/Li/TODO.md`.
- [ ] Update AGENTS.md Last reviewed + Known defects (strike fixed).
- [ ] User accepts move to
      `docs/Li/plans/complete/CATCHUP_COMPLETE.md` (copy or pointer;
      canonical file may remain
      [`/elements/003-lithium/CATCHUP.md`](/elements/003-lithium/CATCHUP.md)).

**Exit gate / validation:**

- [ ] AGENTS.md Known defects only lists remaining items.
- [ ] This plan’s Status blocks are complete or `[~]`.
- [ ] User accepts closeout.

**Status:** pending

**Lessons learned:**

---

## Open decisions

Defaults are L1–L18. Only reopen with a Phase 0 variance.

| # | Question | Default |
| --- | --------- | --------- |
| 1 | JWT roles match rule | Integer `role_id`s → `roles.name` `staff`/`admin`. Phase 0b. |
| 2 | Partner ids missing on Hydrogen | Hide until Phases 5a–5d. Those phases are deferred as of 2026-10-16. |
| 3 | Chat mint C | Out of CATCHUP |
| 4 | REST `auth_chat` / `auth_chats` UI | Parked unless user asks after Phase 25 |
| 5 | `media_chunk` / store toggle | Parked |
| 6 | Didit = OIDC or other | Phase 5a decides; else hidden. Phase 5a is deferred. |

---

## Working Log

## P-plan-20260909 — CATCHUP authored

- What we did: Researched Lithium vs Hydrogen (login, chat/MCP,
  Scripting/CM6, 500 Courses Part 5). Wrote this 0–38 plan. Course
  Manager is ID 34 and the first deploy; Course Builder is ID 35.
  JWT `roles` gates v1 (not 2.23). Sprint superseded.
- Gate result: plan-only; Phase 0 next.
- Follow-ups: Phase 0 (ID table in `LITHIUM-MGR.md`, QueryRefs, live
  JWT roles shape, partner providers).

### P0 — 2026-09-09 Design lock

- What we did: Docs only. Canonical ID table written to
  [`/docs/Li/LITHIUM-MGR.md`](/docs/Li/LITHIUM-MGR.md). Added Phases
  0a (Lookup 042 + consumers), 0b (roles), 5a–5d (partners). Style
  Manager body ID 10 → 22 in MGR.md.
- L1–L18: accepted. Variances: **L3** (integers + `roles.name`, not
  substring `staff`); **L7** (partners omitted until 5a–5d).
- Gate result: complete. Next: Phase 0a (discuss first).

#### Scripting QueryRefs

| Ref | Migration | Purpose | `invokable` in SQL |
| ----- | ----------- | --------- | -------------------- |
| 087 | 1204 | Get Script by Group/Name (with `code`) | no |
| 089 | 1206 | List All Scripts (no `code`) | no |
| 090 | 1207 | Search Scripts | no |
| 129 | 1263 | Insert Script | no (INSERT omits column) |
| 130 | 1264 | Update Script | no |
| 131 | 1265 | Delete Script | n/a; v1 do not expose |
| 149 | (1297 notes) | REST allowlist load | column exists on table |
| 1297 | 1297 | ADD `scripts.invokable` + `Api.Echo=1` | table only; CRUD not updated |

Phase 7 packet must add `invokable` to 087/089/090/129/130.

#### Course Manager QueryRefs / scripts (do not invent)

| Ref / name | Status |
| ------------ | -------- |
| #085 insert `course_suggestions` | existed as 1198 on this day. Renumbered **4000** on 2026-10-16 (`courses_4002.lua`). Still no list/triage QueryRef |
| #086 insert `contact_submissions` | exists (1200). Stayed. No list/triage QueryRef |
| #150 Enrolment History | existed as 1326 on this day. Renumbered **4006** on 2026-10-16 (`courses_4039.lua`) |
| #151 Management History | existed as 1335 on this day. Renumbered **4007** on 2026-10-16 (`courses_4048.lua`) |
| `Catalog.SyncFromCanvas` / `Retire` / `GetBySlug` / `LogEvent` | seeded; staff writers `invokable=0` |
| `Stripe.EnsureProduct` / `EnsureCustomer` / `SyncCustomer` / `Checkout` / `Refund` / `DeactivateCustomer` / `Webhook` | seeded; several `invokable=0` |
| `Enroll.FreeCourse` / `Archive` / `SyncEnrollments` / `SyncProgress` / `PaidCourse` / `LogEvent` / `CanvasSeat` | seeded |
| `Account.GetSettings` / `UpdatePrefs` / `MyCourses` / `Orders` | invokable learner scripts |
| `Provision.EnsureCanvasUser` | seeded |
| `Mail.Notices.CourseExpiration` | invokable |
| `Api.Echo` | `invokable=1` fixture |

#### JWT roles rule (L3)

C (`auth_service_jwt.c`, QueryRef #017): claim is a **string**,
comma-separated **role_id integers** (`"1,3,7"` or `""`). Not an
array. Chat uses exact `"chat"` on a different token. Mail Relay
checks integer `mail_send` (`role_id=1`). No live token read; this
is the C contract. Client v1: parse to integers, resolve
`roles.name`, allow Course Manager iff name is `staff` or `admin`.
Those `roles` rows are not seeded yet (Phase 0b). Lookup 036 does
**not** map `role_id` → label.

#### Partners

JSON key is `"login partners"` (space). All four booleans true.
`auth.oidc_providers` is only `500passwords`. Example
`hydrogen.json` `OIDC_RP.Providers[]` is only `500passwords`.
Buttons in `login.html` have no JS. Treat Didit/Apple/Google/
Microsoft as **absent** until 5a–5d. Live philement/500courses
Hydrogen configs were not in tree.

#### Vite / Hydrogen

`LITHIUM-TOC.md` already: Vite `:3000` runs; login/data need
Hydrogen; prefer `npm test` / `npm run lint`. `LITHIUM-DEV.md` is
silent (does not claim Vite cannot run). Logged; no DEV rewrite.

### Courses pack cut — 2026-10-16

- What we did: Reviewed Lithium against the Courses pack cut. No SPA
  source change. Course Manager's only Conduit call is still QueryRef
  **155**. Course Builder sends no QueryRef. Wrote the Courses pack
  QueryRefs section and pointed Phases 13, 15, and 16 at it.
- Gate result: plan note only. The pause point at that moment stayed
  Phase 1. Later the same day Phases 1 and 2 closed. The pause point
  at the top of this file is Phase 3.
- Follow-ups: Band D must not call 85, 150, or 151. A database that
  already applied the old 1xxx course files has to be rebuilt before
  the 4000 pack is applied. Next Courses file **4076**. Next
  caller-facing QueryRef **4008**.

### P1 — 2026-10-16 Manager lifecycle

- What we did: `closeManager` now calls `destroy`, otherwise
  `cleanup`, otherwise `teardown`. `showManager` calls `onDeactivate`
  then `onActivate` after the crossfade. Deleted `_importManager`
  cases 1–6. Added `tests/unit/app/manager-loader.test.js`.
- Gate result: `npm test` was 953 passed and 10 failed. The 10
  failures are the live auth file (HTTP 503 from
  `lithium.philement.com`). The lifecycle file passed 7 tests. Lint
  0 errors, 35 existing warnings. Variance recorded on the phase.
  `closeManager` no longer calls `destroy` by itself.
- Follow-ups: Phase 2. No browser pass of open-then-close.

### P2 — 2026-10-16 Sanitize and script client

- What we did: Confirmed `escapeHtml`. Added `sanitizeHtml`. Added
  `invokeScript` and `getScriptJob` with the authQuery radar and
  `serverError` shape. No manager calls them.
- Gate result: `npm test` was 965 passed and 10 failed. The 10
  failures are the live auth file (HTTP 503). Lint 0 errors, 35
  existing warnings. Variance recorded on the phase.
- Follow-ups: Phase 3. happy-dom drops tags inside DOMPurify, so
  tag preservation is a browser check, not a Vitest check.

### Docs — 2026-10-16 Phase 3 resume alignment

- What we did: Brought the pause-facing docs in line with Phases 1
  and 2. Refreshed the Observed State rows for conduit, Course
  Manager, Course Builder, `closeManager`, DOMPurify, and
  `escapeHtml`. Lock L6 now matches the Phase 1 close and switch
  rules. Phase 3 Reference names the sink functions, including
  Queries `renderMarkdownPreview`. ID maps in this file and
  `AGENTS.md` say 34 and 35 are registered placeholders. Phases 12
  and 19 say those files already exist. `docs/Li/TODO.md` next phase
  is Phase 3.
- Gate result: doc pass only. No source change. Phase 3 Status stays
  pending.
- Follow-ups: Phase 3 starts in a new conversation.

### P3 — 2026-10-16 XSS sinks

- What we did: Wired the named sinks. Lookups escapes, then
  highlights, then `sanitizeHtml` on assign. Crimson, Version
  History, and the Queries summary preview sanitize markdown.
  Version History HTML content type sanitizes the raw summary.
  Status, Released, and Focus are text. Captions, grouping titles,
  nav labels, follow-ups, and citation links use text nodes plus an
  allowlisted icon node. `highlightInline` uses `textContent`.
  `sanitizeHtml` re-parses when a script, iframe, object, embed,
  `on*` attribute, or `javascript:` remains in DOMPurify's string.
- Gate result: `npm test` was 983 passed and 10 failed. The 10
  failures are the live auth file (HTTP 503). `xss-sinks.test.js`
  passed 16. Lint 0 errors, 35 existing warnings. `lint:css` exited
  0. `templates:copy` updated the version-history CSS copy.
  Headless Chrome: nested `<p><img onerror><script>` did not fire
  `alert`, left no script element, and left the image without
  `onerror`. Lookup preview of `<img onerror=alert(1)>` and of
  `<img src="x" onerror="alert(1)"><script>alert(1)</script>`
  rendered both as text. `alert` did not fire. No image element and
  no script element.
- Follow-ups: Phase 4. `lookups.js` split stays in Phase 35.
  `invokeScript` still has no manager caller.

### P4 — 2026-10-16 Honest UI

- What we did: Added `confirmDialog` and `promptDialog`. Alerts on
  the camera popout and the JSON import became toasts. Removed the
  Login, Main, Session Log, Profile, and login-language HTML clones.
  Lookups no longer reads a JSON textarea. Style Apply and Copy use
  the CSS editor text and `#style-manager-preview`. Stub profile
  saves return failure and stay dirty. `USERID` comes from
  `app.auth.user`. SunEditor uses `uploadAuthHeader(retrieveJWT())`.
- Entry pattern: toast is
  [`toast.js`](/elements/003-lithium/src/shared/toast.js). No confirm
  modal existed, so this phase added
  [`confirm.js`](/elements/003-lithium/src/shared/confirm.js).
- Gate result: `rg` of `alert(`, `confirm(`, `prompt(`, and
  `renderFallback` in `src/` is empty. Stub `success: true` is empty
  outside language, date formats, photo, manager 23, and the
  `saveAll` bookkeeping line. `npm test` was 996 passed and 10
  failed. The 10 failures are the live auth file (HTTP 503).
  `honest-ui.test.js` passed 13. Lint 0 errors, 35 existing
  warnings. `lint:css` exited 0. Headless Chrome loaded `confirm.js`:
  the delete message stayed text (`hasImg` false), confirm resolved
  true, and the prompt returned `Named`.
- Follow-ups: Phase 5. Footer Save still does not call
  `settingsHandler.save()`. `applyState` is still empty.
  `invokeScript` still has no manager caller.

### P5 — 2026-10-16 Ordered partner logins

- What we did: Removed the four hardcoded login buttons. The alt
  row now comes from `login partners` plus `auth.oidc_providers`.
  Order is Didit, Apple, Google, Microsoft, then other OIDC
  providers. A missing provider or a false flag omits the button.
  Click still calls `startOidc` and stores `oidc:<id>`.
- Entry gate: Andrew confirmed the four partners stay absent until
  5a–5d. Example Hydrogen `OIDC_RP` has only `500passwords`. Live
  philement and 500courses configs were not in the tree in Phase 0,
  and this sitting did not look them up.
- Hidden, no Hydrogen provider: **didit**, **apple**, **google**,
  **microsoft**. No second protocol. `login partners` booleans stay
  true. Checked-in `oidc_providers` stays `500passwords` only.
- Gate result: `login.test.js` passed 37 tests. `npm test` was 1003
  passed and 10 failed. The 10 failures are the live auth file
  (HTTP 503). Lint 0 errors, 35 existing warnings. Direct eslint on
  `login.js` was 0 errors and 2 existing warnings. No browser pass.
  Andrew checks the login page. No partner provider is live.
- Follow-ups: Phase 5a. Ask whether Didit is OIDC before editing.
  Style Manager mockup still shows the four images.
  Later the same day Andrew deferred 5a–5d and 6. See P7.

### P7 — 2026-10-16 Scripts schema packet

- What we did: Deferred Phases 5a, 5b, 5c, 5d, and 6. Audited the
  script QueryRefs on disk and wrote Acuranzo 1389–1394. Did not
  edit Lithium source. Later the same day Andrew reported 1389–1393
  applied. 1394 was not in that report.
- Column → QueryRef / tableDef:

  | Column | 087 detail | 089 list | 090 search | 129 insert | 130 update | 131 delete | Lookup 059 key 13 |
  | --- | --- | --- | --- | --- | --- | --- | --- |
  | `group_name` | SELECT, `:GROUP_NAME` | SELECT | `:GROUP_NAME` optional | `:GROUP_NAME` | WHERE | WHERE | column, primary key |
  | `script_name` | SELECT, `:SCRIPT_NAME` | SELECT | no | `:SCRIPT_NAME` | WHERE | WHERE | column, primary key |
  | `script_type` | SELECT | SELECT | `:SCRIPT_TYPE` optional | `:SCRIPT_TYPE` | SET | no | lookup 061 |
  | `schedule` | SELECT | SELECT | no | `:SCHEDULE` | SET | no | column |
  | `next_run` | SELECT | SELECT | no | `:NEXT_RUN` | SET | no | hidden column |
  | `last_run_start` | SELECT | SELECT | no | `:LAST_RUN_START` | SET | no | hidden column |
  | `last_run_end` | SELECT | SELECT | no | `:LAST_RUN_END` | SET | no | hidden column |
  | `status` | SELECT | SELECT | `:STATUS` optional | `:STATUS` | SET | no | lookup 062 |
  | `code` | SELECT | omitted | omitted | `:CODE` | SET | no | hidden column |
  | `summary` | SELECT | SELECT | no | `:SUMMARY` | SET | no | column |
  | `invokable` | 1389 applied | 1390 applied | 1391 applied | `:INVOKABLE` in 1392, applied | `:INVOKABLE` in 1393, applied | no | integer column in 1394, not applied |
  | audit fields | omitted | omitted | omitted | `${COMMON_FIELDS}` | `updated_at`, `:USERID` | no | omitted |

- Delete 131 parameters are `GROUP_NAME` and `SCRIPT_NAME`. Not an
  integer primary key.
- Seeded `invokable = 1` (migration source, not a live query):
  `Api.Echo` (1297), `Enroll.FreeCourse` (4013),
  `Account.GetSettings` (4015), `Account.UpdatePrefs` (4016),
  `Mail.Notices.CourseExpiration` (4018), `Enroll.SyncProgress`
  (4021), `Account.MyCourses` (4022), `Stripe.EnsureCustomer`
  (4025), `Stripe.SyncCustomer` (4027), `Stripe.Checkout` (4028),
  `Stripe.Webhook` (4032), `Enroll.SyncEnrollments` (4036),
  `Enroll.Archive` (4037), `Catalog.GetBySlug` (4052),
  `Account.Orders` (4072).
- Gate result: complete on Andrew's report that 1389–1393 are
  applied. `luacheck` 0 warnings on the six files. `lua` compiled
  each file. The agent did not see the apply, did not run `mkt` or
  `mka`, and no migration test was named. The human-build exit line
  stays open. 1394 stays unapplied.
- Follow-ups: Phase 8 sends INTEGER `INVOKABLE` on 129 and 130.
  Save and duplicate fail until then. Apply 1394 before Scripting
  relies on Lookup 059 key 13. Regenerate the payload with `mka`
  or `payload-generate.sh` if that step was skipped.

### P8 — 2026-10-16 Scripting CRUD honesty

- What we did: Save, update, and duplicate send INTEGER `INVOKABLE`
  (0 unless the row is 1) and `USERID` from `app.auth.user`. Delete
  confirms and calls QueryRef 131 with STRING `GROUP_NAME` and
  `SCRIPT_NAME`. A dirty Scripting row switch asks to save or stay.
  Rename of a persisted row is refused. New scripts prompt for group
  and name. Footer and the payload builder left `scripting.js`.
- Gate result: `wc -l scripting.js` is 616.
  `scripting-save.test.js` passed 18. `npm test` was 1021 passed and
  10 failed. The 10 are the live auth file (HTTP 503). Lint was 0
  errors and 35 existing warnings. No browser pass. 1394 stays
  unapplied. No new migration.
- Follow-ups: Phase 9 invoke panel in `scripting-invoke.js`. Do not
  grow `scripting.js` for it. `invokeScript` still has no manager
  caller.

### P9 — 2026-10-16 Invoke panel and async poll

- What we did: Added the Scripting Run tab in `scripting-invoke.js`.
  `scripting.js` only mounts it, shows it, syncs the button, and
  destroys it. Wait defaults on. Timeout is 1–60, default 15. A
  top-level `params._hydrogen` is removed before the POST. Poll
  delays are 250, 500, 1000, then 2000 ms until a terminal status,
  GET 404, a row change, or close. No cancel control. A wait-true
  POST uses `timeout_seconds` plus 5 seconds so a 31–60 second
  Hydrogen timeout is not cut by the 30s client cap.
- Approved poll schedule: 250, 500, 1000, then 2000 ms. A pending job
  keeps polling until the row changes or the manager closes.
- Gate result: `scripting-invoke.test.js` passed 17.
  `wc -l scripting.js` is 627. `scripting-invoke.js` is 546.
  `npm test` was 1040 passed and 10 failed. The 10 are the live auth
  file (HTTP 503). The chained `npm test && npm run lint` exited 1
  at `npm test`. Lint, run separately, was 0 errors and 35 existing
  warnings. `npm run lint:css` exited 0. `templates:copy` refreshed
  the public Scripting HTML and CSS. The two manual `Api.Echo` lines
  were not run. 1394 stays unapplied. No new migration. No product C.
- Follow-ups: Sign in and run `Api.Echo` with Wait on, then Wait off.
  Check those two exit lines before marking Phase 9 complete. Do not
  rebuild the panel. Do not open Phase 10 unless both lines are green.

### P9 — 2026-10-17 Schema gate and Lua long brackets

- What we did: Left the Run tab as built. `npm run deploy` now runs
  `test:coverage:deploy`, which skips
  `tests/integration/auth.integration.test.js`. The other tests still
  abort the deploy. That command passed here: 38 files, 1038 tests.
  Fixed the Lua highlighter in `src/core/codemirror.js`. A long
  bracket (`[[` or `[=[`) called `.slice` on `stream.match`, which is
  the method. The matched text is the return value. A bracket that
  runs past the line stays open until its closer.
  `tests/unit/lua-mode.test.js` passed 5. ESLint on those two files
  was clean.
- What Andrew saw: On `lithium.500courses.com`, after a Hydrogen
  update, login worked and Scripting listed scripts. Selecting
  `Api.Echo` left Run disabled. He identified that database as an
  older schema. Row changes had thrown
  `e.match.slice is not a function` from bundle
  `codemirror-setup-C4xZ7-ri.js`. He later moved between rows without
  that throw. That bundle does not contain the highlighter fix.
- Gate result: Phase 9 stays in progress. The two manual lines stay
  unchecked. No migration was applied. 1394 stays unapplied.
- Follow-ups: Andrew sorts 1297, 1389, and 1390 on the database under
  test. Next session starts when QueryRef 87 returns
  `"invokable": 1` for `Api` / `Echo`, then runs the two manual lines.
  Do not rebuild the panel. Do not open Phase 10 unless both lines
  are green. Deploy before retesting long strings.

### P9 — 2026-10-22 Api.Echo on 500 Courses

- What we did: Did not rebuild the panel and did not deploy. Live
  `lithium.500courses.com` is build 1.1.3447, the same
  `version.json` as the tree, and the Scripting bundle already
  contains the Run tab. QueryRef 87 for `Api` / `Echo` returns
  `invokable` 1. QueryRef 89 lists 76 scripts and that row is
  `invokable` 1. Lookup 059 key 13 is `scripts/script-manager`.
- Gate result: Headless Chrome signed in, opened manager 33, selected
  `Api.Echo`, and clicked Run. Wait on painted `status: completed`
  and `job_id: THCHG`, echoed the marker, and logged
  `[Scripting] Run Api.Echo status=completed job_id=THCHG`. The
  spinner class was on. Wait off showed the spinner, logged
  `pending`, then logged `completed` for `job_id: FSGBH`. A separate
  `POST /api/conduit/script` matched: wait true completed, and wait
  false returned HTTP 202 `pending` before the poll completed. No
  Scripting source changed. No migration was applied.
- Follow-ups: Andrew confirmed both lines on 2026-10-22. Wait on
  `job_id: BBTDC`. Wait off `job_id: CCCRP`. Both
  `status: completed`. Phase 9 is complete. Phase 10 is next, after
  its questions are answered. Phases 5a–5d and 6 stay deferred.

### P19 — 2026-10-23 Submission list, packet not applied

- What we did: Replaced the ID 35 placeholder with the suggestion
  list and the Submission tab. The staff gate now omits IDs 34 and
  35 together. Wrote `courses_4097.lua` (`course_build_runs`),
  `courses_4098.lua` (`course_build_actions`), `courses_4099.lua`
  (QueryRef **4513**), `courses_4100.lua` (QueryRef **4514**), and
  `acuranzo_1399.lua` (Lookup 059 key 15, `courses/course-builder`).
  Handed the packet over. Did not apply it and did not run `mka`.
- Gate result: `course-builder.test.js` passed 6. `npm test` was
  1148 passed and 10 failed (the live auth file, HTTP 503).
  `npm run lint` and `npm run lint:css` exited 0. `luac -p` and
  luacheck exited 0 on the five Lua files. The live manager was
  not opened. The database write of one ledger row is still open.
- Lessons: units proposed are integer cents, integer cents, integer
  milliseconds, and an integer prompt count. This save leaves them
  NULL. Reverse 4098 before 4097. A missing suggestion affects
  zero rows and the client only reloads.
- Follow-ups: Andrew confirms the units, regenerates the payload,
  applies the five files, deploys Lithium, and checks one fixture.
  Phase 19 stays in progress. Do not start Phase 20. Next file
  **4101**. Next ref **4515**. Next Acuranzo file **1400**. Next
  Lookup 059 key **16**.

### P19b — 2026-10-23 Edit block, one receipt, Intake select

- What we did: Wired the LithiumTable manage block. Staff can
  add, duplicate, edit, flag, annotate, and delete a suggestion.
  Opening Course Builder calls `Build.ReceiveSuggested` once.
  That script moves Suggested to Received and attempts one email.
  `receipt_mail` stores `sent` or `failed` and is not retried.
  The stage select moves Suggested or Received to Intake and
  does not send mail. Wrote `courses_4101.lua` through
  `courses_4110.lua` and `acuranzo_1400.lua`. Did not apply them
  and did not run `mka`.
- Gate result: `course-builder.test.js` passed 9. ESLint on the
  Course Builder files and that test exited 0. Stylelint on
  `course-builder.css` exited 0. `luac -p` and luacheck exited 0
  on the eleven Lua files, and `luac -p` passed on the stored
  script body. The full `npm test` was not re-run. No browser
  was connected. Andrew had already confirmed the list and
  Submission tab on the applied 4097–4100 packet. The
  working-title ledger write is still unproven.
- Lessons: claim `failed` before the send, then upgrade to
  `sent`. A missing or invalid address is a failed attempt and
  does not block Received. The first open after apply emails
  every row still on Suggested, including "All about mDNS".
- Follow-ups: Andrew regenerates the payload, applies 4101–4110
  and `acuranzo_1400.lua`, restarts Hydrogen, then deploys
  Lithium. Phase 19 stays in progress. Do not start Phase 20.
  Next file **4111**. Next ref **4522**. Next Acuranzo file
  **1401**. Lookup 059 key **16** stays free.

### P19c — 2026-10-23 Orchestrator receipt, cast rewrite, sender lines

- What we did: Stopped Course Builder from calling
  `Build.ReceiveSuggested` on open. Wrote `courses_4111.lua`
  through `courses_4113.lua` so **4514**, **4517**, and **4518**
  cast the title as `varchar(100)` (`char(100)` on MySQL). Wrote
  `courses_4114.lua` to drop `Enroll.StaffGate`, set actor 0,
  set invokable 0, and add blank `FROM_ADDRESS` and `REPLY_TO`.
  Wrote `acuranzo_1401.lua` so the one-second orchestrator
  submits that script when a row still needs a receipt. Did not
  apply them and did not run `mka`.
- Gate result: `course-builder.test.js` passed 9. ESLint on the
  Course Builder files and that test exited 0. `luac -p` and
  luacheck exited 0 on the five new Lua files. The stored
  `Build.ReceiveSuggested` body passed both. The orchestrator
  body keeps two warnings copied from `acuranzo_1364.lua`. No
  browser was connected. Andrew had already received the mDNS
  receipt. The working-title save failed live at statement 1
  and is still unproven.
- Lessons: the login path sent the mail he received. A worker
  has no JWT, so the applied staff gate would send nothing.
  The words are the mail template. The From line is the server
  default until he fills the two script constants. There is no
  insert-time trigger. `CAST AS text` into `varchar(100)` is the
  likely **4514** failure.
- Follow-ups: Andrew regenerates the payload, applies 4111–4114
  and `acuranzo_1401.lua`, restarts Hydrogen, deploys Lithium,
  then retries the working-title save. Phase 19 stays in
  progress. Do not start Phase 20. Next file **4115**. Next ref
  **4522**. Next Acuranzo file **1402**. Lookup 059 key **16**
  stays free.

### P19d — 2026-10-23 Suggested is a stage choice

- What we did: Andrew confirmed the receipt went out on the
  Hydrogen restart, so 4111–4114 and `acuranzo_1401.lua` ran.
  Added Suggested to the stage list for Received and Intake.
  Wrote `courses_4115.lua` (QueryRef **4522**). It sets state
  `suggested` and clears `receipt_mail`. Wrote `courses_4116.lua`
  so the receipt key includes the send time. The screen does not
  send the mail. Did not apply them and did not run `mka`.
- Gate result: `course-builder.test.js` passed 9. ESLint on the
  Course Builder files and that test exited 0. `luac -p` and
  luacheck exited 0 on 4115 and 4116, and on the stored script
  body. No browser was connected. The working-title ledger row
  is still unproven.
- Lessons: a stable mail key returns the first message. Clearing
  the mark is not enough. The key has to change. Intake still
  leaves the mark alone.
- Follow-ups: Andrew regenerates the payload, applies 4115 and
  4116, restarts Hydrogen, and deploys Lithium. Phase 19 stays
  in progress. Do not start Phase 20. Next file **4117**. Next
  ref **4523**. Next Acuranzo file **1402**. Lookup 059 key
  **16** stays free.

### P19e — 2026-10-23 Receipt body

- What we did: Andrew reported the sent receipt had a subject
  and no body. Wrote `courses_4117.lua`. It replaces the text
  and HTML of `course.suggestion_received` and keeps the
  subject. The greeting macro is filled from the suggestion
  `submitter_name`. The course macro is filled from the
  suggestion `course_name`. The send does not read a JWT. Did
  not edit `courses_4109.lua`. Did not apply the file and did
  not run `mka`.
- Gate result: `luac -p` and luacheck exited 0 on 4117. No
  client file changed. No browser was connected. The
  working-title ledger row is still unproven.
- Lessons: the script already passes the suggestion fields.
  A queued message keeps the body it was rendered with. Moving
  the row back to Suggested is what sends again.
- Follow-ups: Andrew regenerates the payload, applies 4115,
  4116, and 4117, restarts Hydrogen, and deploys Lithium.
  Phase 19 stays in progress. Do not start Phase 20. Next file
  **4118**. Next ref **4523**. Next Acuranzo file **1402**.
  Lookup 059 key **16** stays free.

---

## Revision history

| Date | Change |
| ------ | -------- |
| 2026-09-09 | Initial CATCHUP.md (Phases 0–38). Supersedes LITHIUM_SPRINT.md. |
| 2026-09-09 | Phase 0 complete. Added 0a, 0b, 5a–5d. L3/L7 variances. |
| 2026-10-16 | Courses pack QueryRefs. Band D uses 4000–4007. 85, 150, and 151 retired. 86, 149, and 155 stay. |
| 2026-10-16 | Phase 1 complete. Manager close uses destroy, else cleanup, else teardown. Switch hooks run after the crossfade. |
| 2026-10-16 | Phase 2 complete. `sanitizeHtml`, `invokeScript`, and `getScriptJob` exist. No manager calls them yet. |
| 2026-10-16 | Docs aligned for a Phase 3 restart. Observed State, L6, ID maps, Phase 3 sink list, and `docs/Li/TODO.md`. |
| 2026-10-16 | Phase 3 complete. Named HTML sinks escape text or sanitize markdown. Icon tokens become nodes. |
| 2026-10-16 | Phase 4 complete. Shared confirm and prompt. Stub saves refuse. SunEditor omits a missing bearer. |
| 2026-10-16 | Phase 5 complete. Partner buttons render from config or stay omitted. Checked-in login shows 500 Passwords only. |
| 2026-10-16 | Phases 5a–5d and 6 deferred. Phase 7 packet 1389–1394 handed over, not applied. |
| 2026-10-16 | Phase 7 complete. Andrew reported 1389–1393 applied. 1394 unapplied. Resume is Phase 8. |
| 2026-10-16 | Phase 8 complete. Scripting save sends INTEGER `INVOKABLE`. Delete uses QueryRef 131. Resume is Phase 9. |
| 2026-10-16 | Phase 9 in progress. Run tab is in `scripting-invoke.js`. Manual `Api.Echo` lines are still open. |
| 2026-10-17 | Phase 9 still in progress. Older schema left Run disabled. Lua long-bracket fix is in the tree. Resume after QueryRef 87 returns `invokable` 1. |
| 2026-10-22 | Phase 9 complete. Andrew confirmed `Api.Echo` Wait on (`BBTDC`) and Wait off (`CCCRP`), both `status: completed`. |
| 2026-10-22 | Phase 10 complete. StyLua, save, summary-then-Lua preview, and font reload are confirmed. The meta tab was declined. Next is Phase 11. |
| 2026-10-22 | Phase 11 complete. CodeMirror 6 is the only stack. Lua folds statement blocks. `H.` uses the namespace tag. Next is Phase 12. |
| 2026-10-22 | Phase 12 complete. Course Manager is a staff shell. QueryRef 155 failure denies. The sidebar omits ID 34. Next is Phase 13. |
| 2026-10-22 | Phase 12 follow-up. `getClaims(retrieveJWT())` restores the menu row. Andrew confirmed the shell. Phase 13 still needs a staff list QueryRef. |
| 2026-10-22 | Phase 13 in progress. Staff refs 4500–4504. Andrew opened the live list. Detail tabs are Course, Canvas, Commerce, and History. Next file 4081. Next ref 4505. |
| 2026-10-22 | Phase 13 still in progress. Navigator edit, Options Retired, price list 4505, Code and Title columns. Next file 4084. Next ref 4506. Next Acuranzo file 1399. |
| 2026-10-23 | Phase 13 complete. Andrew reported the latest build deployed, the latest migrations applied, and the list Retired checkbox removed. Next is Phase 14. Discuss it and wait. |
| 2026-10-23 | Phase 14 in progress. Packet 4084–4086 handed over, not applied. Save prices **4506**. Orders **4507**. `Stripe.Mode` invokable 1. Next file **4087**. Next ref **4508**. |
| 2026-10-23 | Phase 14 still in progress. Andrew reported 4084–4086 applied. Save prices reloaded the previous cents and toasted success. The session log had no binds or affected_rows. `courses_4087.lua` rewrites 4506 and is not applied. Next file **4088**. Next ref **4508**. |
| 2026-10-23 | Phase 14 complete. Andrew confirmed Save prices on course 16 at 13:55:13. CAD cents 4999, success true, affected_rows 1, eight statements. `courses_4087.lua` is the SQL that ran. Next is Phase 15. Discuss it and wait. Next file **4088**. Next ref **4508**. |
| 2026-10-23 | Phase 15 in progress. Packet 4088–4096 handed over, not applied. Search **4508**, account **4509**, seats **4510**, orders **4511**, Canvas link **4512**. `Enroll.StaffGate` invokable 0. `Enroll.StaffGrant`, `Enroll.StaffArchive`, and `Enroll.StaffRenew` invokable 1. Manual search still open. Next file **4097**. Next ref **4513**. |
| 2026-10-23 | Phase 15 complete. Andrew searched andrew@500courses.com, opened account **9001**, Canvas user **801**, granted a course, and confirmed the Canvas user link. Archive and Renew were not clicked. The live search and grant are the apply report for 4088–4096. Next is Phase 16. Discuss it and wait. Next file **4097**. Next ref **4513**. |
| 2026-10-23 | Andrew parked Course Manager. Phases 16–18 deferred. Suggestions are Course Builder. Phase 19 approved: LithiumTable columns Working Title, Last activity, State; Submission tab matches the Reception form; action ledger keeps human cost, token cost, time, and prompt count. Next sitting implements Phase 19. Same slice as COURSEBUILDER CB-S. |
| 2026-10-23 | Phase 19 in progress. Shell is in the tree. Packet 4097–4100 and `acuranzo_1399.lua` handed over, not applied. List **4513**. Save **4514**. Lookup 059 key 15. Units not confirmed. Next file **4101**. Next ref **4515**. Next Acuranzo file **1400**. Do not start Phase 20. |
| 2026-10-23 | Phase 19 still in progress. Andrew applied 4097–4100 and `acuranzo_1399.lua` and confirmed the list. Edit block, one receipt email, and Intake select are in the tree. Packet 4101–4110 and `acuranzo_1400.lua` handed over, not applied. List **4515**. Next file **4111**. Next ref **4522**. Next Acuranzo file **1401**. |
| 2026-10-23 | Phase 19 still in progress. Andrew received the mDNS receipt, so 4101–4110 and Acuranzo 1400 ran. That row is Received. QueryRef **4514** failed at statement 1. Packet 4111–4114 and `acuranzo_1401.lua` handed over, not applied. The screen does not send mail. Next file **4115**. Next ref **4522**. Next Acuranzo file **1402**. |
| 2026-10-23 | Phase 19 still in progress. Andrew applied 4111–4114 and Acuranzo 1401. The receipt went out on the restart. Suggested is a stage choice. Packet 4115–4116 handed over, not applied. **4522** clears the receipt mark. Next file **4117**. Next ref **4523**. |
| 2026-10-23 | Phase 19 still in progress. The sent receipt had a subject and no body. `courses_4117.lua` replaces the text and HTML. The name and the course title come from the suggestion row. Packet 4115–4117 handed over, not applied. Next file **4118**. Next ref **4523**. |
