# Courses pack — running list

Inventory of the Courses pack after the cut out of Acuranzo. Files are
`courses_4000.lua` through `courses_4083.lua`. The next file is 4084.
File numbers stay below 4500. Caller-facing QueryRefs are 4000–4007
and 4500–4505. The next SPA ref is 4506. `auth_query` must not use a
ref that equals a file number: bootstrap orders by query type
descending, and the first cache row is the type-1001 reverse
migration. `Orchestrators.Orchestrator` stayed in Acuranzo.

**Last updated:** 2026-10-22.

Reception is a separate tree, `500-Courses-Reception`, not this repo.
Lithium's course manager has barely started. Both still call the old
QueryRefs. Update them from the tables below. Script names stay.

## Decision already made

Acuranzo stays the base design. Courses is a pack on that same
database, the same SQL schema, and the same `queries`, `lookups`, and
`scripts` tables. Argent is the pattern: file number, bookkeeping
`query_ref`, and caller-facing QueryRef share one thousand.

| Pack | Band | State |
| --- | --- | --- |
| Acuranzo | 1000–1999 | Base. Courses files leave a hole. MCP, mail, and dialect files keep their numbers. |
| Argent | 2000–2999 | Live pack. |
| Gaius | 3000–3999 | Reserved. The on-disk `gaius/` tree is still its own bootstrap, not this pack. |
| Courses | 4000–4999 | Files 4000–4080. Older caller refs 4000–4007. SPA refs 4500–4504. File numbers stay below 4500. |

The printing design under `elements/002-helium/helium/` is gone, so
band 4 is free. `database.lua` and `database_<engine>.lua` live in
that folder, outside `migrations/`, as the one copy every pack loads.
The loader, the payload, and SchemaTool use that copy.

`PAYLOAD:courses` alone is refused, same as Argent and Gaius. The
full suite string is `PAYLOAD:acuranzo+courses+argent`. The 500
Courses connection (Hydrogen database name `Lithium` in the Reception
config) uses that string. It does not become a second database.
The Lua on disk is cut. `mka` rebuilt the Hydrogen archive on
2026-10-16, and that archive contains `courses/`. Course objects load
only when the connection names `courses`. Tests 32–40, 62, and 73,
and the SchemaTool wrappers, use `PAYLOAD:acuranzo+courses+argent`.
`PAYLOAD:acuranzo` and `PAYLOAD:acuranzo+argent` stay valid and omit
the pack. A database that already applied the old 1xxx course files
must be rebuilt before this pack is applied. The 4000 files create
those tables again.

A rebuilt database applies the files that exist, in numeric order.
A missing number is ignored. Inside one thousand, a file at or below
that thousand's high-water mark is skipped. After the rebuild, the
next Acuranzo file is above the highest Acuranzo file still on disk.
The retired numbers in the hole stay retired.

## Reception — change these QueryRefs

Reception reads the numbers from config, then falls back to literals.
Update both places, and the docs that repeat them.

| Today | Config keys | Installed by | Later SQL patches | New QueryRef |
| --- | --- | --- | --- | --- |
| 85 | `queryRefs.suggestCourse` | [4002](/elements/002-helium/courses/migrations/courses_4002.lua) | — | 4000 |
| 147 | `queryRefs.catalogList`, `catalog.listRef` | [4011](/elements/002-helium/courses/migrations/courses_4011.lua) | 4026, 4051, 4059, 4075 | 4004 |
| 148 | `queryRefs.catalogBySlug`, `catalog.detailRef` | [4012](/elements/002-helium/courses/migrations/courses_4012.lua) | 4026, 4051, 4059 | 4005 |

Call sites, all under `500-Courses-Reception`:

- `src/config.js` — defaults, including the `147` / `148` fallbacks.
- `public/config.json` — the file the built app loads. It repeats 85, 86, 147, 148.
- `src/courses-api.js` — `listRef` / `detailRef` fall back to 147 and 148.
- `src/cap-submissions.js` — fallback `{ suggestCourse: 85, contactForm: 86 }`.
- `src/scripts.js` — comment on the Courses page only.
- `docs/HELIUM.md`, `docs/HYDROGEN.md`, `docs/FORMS.md`, `docs/ARCHITECTURE.md`, `docs/SECURITY.md`.

QueryRef **86** (`contactForm`, [1200](/elements/002-helium/acuranzo/migrations/acuranzo_1200.lua),
`contact_submissions`) stays. It sits in the same config object as 85.
Leave 86 alone when 85 moves.

## Reception — script names stay

JWT `POST /api/conduit/script`. Names do not change. The rows move
with the pack.

| Script | Reception file | Seeded in | New file |
| --- | --- | --- | --- |
| `Enroll.FreeCourse` | `src/config.js` `enroll.freeEnrollScript`, `src/enroll-free.js` | [1300](/elements/002-helium/courses/migrations/courses_4013.lua), replaced 1344, 1351 | |
| `Account.GetSettings` | `profile.getSettingsScript`, `src/profile-api.js` | [1302](/elements/002-helium/courses/migrations/courses_4015.lua) | |
| `Account.UpdatePrefs` | `profile.updatePrefsScript` | [1303](/elements/002-helium/courses/migrations/courses_4016.lua), nil-bind fix [1320](/elements/002-helium/courses/migrations/courses_4033.lua) | |
| `Account.MyCourses` | `myCourses.listScript`, `src/my-courses-api.js` | [1309](/elements/002-helium/courses/migrations/courses_4022.lua) | |
| `Account.Orders` | `profile.ordersScript` | [1360](/elements/002-helium/courses/migrations/courses_4072.lua) | |
| `Enroll.SyncEnrollments` | `myCourses.syncScript` | [1323](/elements/002-helium/courses/migrations/courses_4036.lua), events 1332, Canvas heal 1359 | |
| `Enroll.Archive` | `src/config.js` default `myCourses.archiveScript` | [1324](/elements/002-helium/courses/migrations/courses_4037.lua), events 1330, Canvas unenroll 1358 | |
| `Enroll.RenewFree` | `src/config.js` default `myCourses.renewScript`, `src/my-courses-api.js` | **No Helium seed.** Reception already calls it. `PRIORITIZE.md` in Reception says it is unseeded. | |
| `Stripe.Checkout` | `stripe.checkoutScript`, `src/stripe-checkout.js` | [1315](/elements/002-helium/courses/migrations/courses_4028.lua), nil-bind 1317, retired check 1350 | |
| `Catalog.GetBySlug` | `catalog.getBySlugScript` | [1339](/elements/002-helium/courses/migrations/courses_4052.lua), `retired` column 1348 | |

`public/config.json` currently spells enroll, profile, myCourses, and
stripe. It does not yet list `archiveScript` or `renewScript`. The
`src/config.js` defaults still supply those two. Check the deployed
config when the Reception pass happens.

`Enroll.Archive` and `Enroll.RenewFree` are in `src/config.js`. The
checked-in `public/config.json` myCourses block only has `listScript`
and `syncScript`.

## QueryRefs the SPA does not send

Lua calls these. Hydrogen C does not hardcode them. They move with
the pack, and any `run_ref` inside a moved script moves with that file.

| Today | What | Installed by | Patched by | New QueryRef |
| --- | --- | --- | --- | --- |
| 144 | Canvas: lookup link by account | [4004](/elements/002-helium/courses/migrations/courses_4004.lua) | — | 4001 |
| 145 | Canvas: link account to Canvas user | [4005](/elements/002-helium/courses/migrations/courses_4005.lua) | — | 4002 |
| 146 | Canvas: touch link | [4006](/elements/002-helium/courses/migrations/courses_4006.lua) | — | 4003 |
| 150 | Enrolment History lines by `COURSEID` | [4039](/elements/002-helium/courses/migrations/courses_4039.lua) | [4068](/elements/002-helium/courses/migrations/courses_4068.lua) | 4006 |
| 151 | Management History lines by `COURSEID` | [4048](/elements/002-helium/courses/migrations/courses_4048.lua) | — | 4007 |

`Enroll.LogEvent` ([4040](/elements/002-helium/courses/migrations/courses_4040.lua),
was 1327) reads QueryRef 4006 (was 150). `Catalog.LogEvent`
([4049](/elements/002-helium/courses/migrations/courses_4049.lua),
was 1336) reads QueryRef 4007 (was 151). Both are `invokable=0`.

## Scripts the pack owns that Reception does not call

Names stay. Rows move.

| Script | Seed | Notes |
| --- | --- | --- |
| `Provision.EnsureCanvasUser` | [1289](/elements/002-helium/acuranzo/migrations/acuranzo_1289.lua) | First insert stays in Acuranzo, in the same file as the orchestrator patch. Later bodies are 4007, 4035, 4044, 4055, and 4066. |
| `Orchestrators.Orchestrator` | [1289](/elements/002-helium/acuranzo/migrations/acuranzo_1289.lua) | Stays in Acuranzo. Schema fix 1299. Replaced by 1343. Scoreboard prune [1364](/elements/002-helium/acuranzo/migrations/acuranzo_1364.lua) stays with it. |
| `Stripe.EnsureCustomer` | [1312](/elements/002-helium/courses/migrations/courses_4025.lua) | |
| `Stripe.SyncCustomer` | [1314](/elements/002-helium/courses/migrations/courses_4027.lua) | |
| `Stripe.Webhook` | [1319](/elements/002-helium/courses/migrations/courses_4032.lua) | Calls `Enroll.PaidCourse`. |
| `Enroll.PaidCourse` | [1318](/elements/002-helium/courses/migrations/courses_4031.lua) | Events 1329. Retired check 1352. Helium-first 1353. `invokable=0`. |
| `Enroll.SyncProgress` | [1308](/elements/002-helium/courses/migrations/courses_4021.lua) | |
| `Enroll.LogEvent` | [1327](/elements/002-helium/courses/migrations/courses_4040.lua) | `invokable=0`. |
| `Enroll.CanvasSeat` | [1357](/elements/002-helium/courses/migrations/courses_4069.lua) | `invokable=0`. |
| `Catalog.SyncFromCanvas` | [1337](/elements/002-helium/courses/migrations/courses_4050.lua) | `invokable=0`. |
| `Catalog.Retire` | [1349](/elements/002-helium/courses/migrations/courses_4061.lua) | `invokable=0`. |
| `Stripe.EnsureProduct` | [1345](/elements/002-helium/courses/migrations/courses_4057.lua) | `invokable=0`. |
| `Stripe.Refund` | [1361](/elements/002-helium/courses/migrations/courses_4073.lua) | `invokable=0`. |
| `Stripe.DeactivateCustomer` | [1362](/elements/002-helium/courses/migrations/courses_4074.lua) | `invokable=0`. |
| `Mail.Notices.CourseExpiration` | [1305](/elements/002-helium/courses/migrations/courses_4018.lua) | Group/name split fixed in [1306](/elements/002-helium/courses/migrations/courses_4019.lua). |

## Tables and columns that move

| Object | Created or altered in |
| --- | --- |
| Lookup 055 Canvas LMS Links | [1088](/elements/002-helium/courses/migrations/courses_4000.lua) |
| `course_suggestions` | [1197](/elements/002-helium/courses/migrations/courses_4001.lua) |
| `account_canvas_links` | [1283](/elements/002-helium/courses/migrations/courses_4003.lua) |
| `courses` | [1291](/elements/002-helium/courses/migrations/courses_4008.lua). Seed 1293. Paid seed 1316. `tags` / `canvas_updated_at` 1333. `retired` 1346. |
| `course_prices` | [1292](/elements/002-helium/courses/migrations/courses_4009.lua) |
| `user_preferences` | [1301](/elements/002-helium/courses/migrations/courses_4014.lua) |
| Reception mail templates | [1304](/elements/002-helium/courses/migrations/courses_4017.lua) |
| `user_enrollments` | [1307](/elements/002-helium/courses/migrations/courses_4020.lua) |
| `accounts.stripe_customer_id` | [1310](/elements/002-helium/courses/migrations/courses_4023.lua). Column on the base `accounts` table. The pack may alter it. |
| `orders` | [1311](/elements/002-helium/courses/migrations/courses_4024.lua). Refund columns [1355](/elements/002-helium/courses/migrations/courses_4067.lua). |
| `enrollment_events` | [1325](/elements/002-helium/courses/migrations/courses_4038.lua) |
| `catalog_events` | [1334](/elements/002-helium/courses/migrations/courses_4047.lua) |
| `account_canvas_alerts` | [1340](/elements/002-helium/courses/migrations/courses_4053.lua) |
| `ops.canvas_user_unlinked` mail template | [1341](/elements/002-helium/courses/migrations/courses_4054.lua) |

No Courses-owned lookup family in the 4000s exists yet. Keys added to
Acuranzo families keep those family ids.

## Files that move

Cut. The New column is the `courses_` file number. Orchestrator files
1289, 1299, 1343, and 1364 stayed in Acuranzo and are not in this table.

| Now | What it does | New |
| --- | --- | --- |
| 1088 | Lookup 055 Canvas LMS Links | 4000 |
| 1197 | `course_suggestions` | 4001 |
| 1198 | QueryRef 85 insert suggestion | 4002 |
| 1283 | `account_canvas_links` | 4003 |
| 1286 | QueryRef 144 lookup link by account | 4004 |
| 1287 | QueryRef 145 link account to Canvas user | 4005 |
| 1288 | QueryRef 146 touch link | 4006 |
| 1290 | Intro course enrollment on first Canvas provision | 4007 |
| 1291 | `courses` | 4008 |
| 1292 | `course_prices` | 4009 |
| 1293 | Seed courses from the Canvas catalog | 4010 |
| 1294 | QueryRef 147 list published | 4011 |
| 1295 | QueryRef 148 get by slug | 4012 |
| 1300 | `Enroll.FreeCourse` | 4013 |
| 1301 | `user_preferences` | 4014 |
| 1302 | `Account.GetSettings` | 4015 |
| 1303 | `Account.UpdatePrefs` | 4016 |
| 1304 | Reception notification mail templates | 4017 |
| 1305 | `Mail.Notices.CourseExpiration` | 4018 |
| 1306 | Rename that mail script to the first-dot split | 4019 |
| 1307 | `user_enrollments` | 4020 |
| 1308 | `Enroll.SyncProgress` | 4021 |
| 1309 | `Account.MyCourses` | 4022 |
| 1310 | `accounts.stripe_customer_id` | 4023 |
| 1311 | `orders` | 4024 |
| 1312 | `Stripe.EnsureCustomer` | 4025 |
| 1313 | QueryRefs 147/148 select `stripe_price_id` | 4026 |
| 1314 | `Stripe.SyncCustomer` | 4027 |
| 1315 | `Stripe.Checkout` | 4028 |
| 1316 | Sandbox paid course and prices | 4029 |
| 1317 | `Stripe.Checkout` nil-bind fix | 4030 |
| 1318 | `Enroll.PaidCourse` | 4031 |
| 1319 | `Stripe.Webhook` | 4032 |
| 1320 | `Account.UpdatePrefs` empty-optional nil-bind | 4033 |
| 1321 | `Enroll.FreeCourse` writes `user_enrollments` | 4034 |
| 1322 | Intro provision writes `user_enrollments` | 4035 |
| 1323 | `Enroll.SyncEnrollments` | 4036 |
| 1324 | `Enroll.Archive` | 4037 |
| 1325 | `enrollment_events` | 4038 |
| 1326 | QueryRef 150 | 4039 |
| 1327 | `Enroll.LogEvent` | 4040 |
| 1328 | `Enroll.FreeCourse` appends `enrollment_events` | 4041 |
| 1329 | `Enroll.PaidCourse` appends `enrollment_events` | 4042 |
| 1330 | `Enroll.Archive` appends `enrollment_events` | 4043 |
| 1331 | Provision appends `enrollment_events` | 4044 |
| 1332 | `Enroll.SyncEnrollments` appends `enrollment_events` | 4045 |
| 1333 | `courses.tags`, `canvas_updated_at` | 4046 |
| 1334 | `catalog_events` | 4047 |
| 1335 | QueryRef 151 | 4048 |
| 1336 | `Catalog.LogEvent` | 4049 |
| 1337 | `Catalog.SyncFromCanvas` | 4050 |
| 1338 | QueryRefs 147/148 select `tags` | 4051 |
| 1339 | `Catalog.GetBySlug` | 4052 |
| 1340 | `account_canvas_alerts` | 4053 |
| 1341 | `ops.canvas_user_unlinked` template | 4054 |
| 1342 | Replace `Provision.EnsureCanvasUser` | 4055 |
| 1344 | Replace `Enroll.FreeCourse` | 4056 |
| 1345 | `Stripe.EnsureProduct` | 4057 |
| 1346 | `courses.retired` | 4058 |
| 1347 | QueryRefs 147/148 select `retired` | 4059 |
| 1348 | `Catalog.GetBySlug` selects `retired` | 4060 |
| 1349 | `Catalog.Retire` | 4061 |
| 1350 | `Stripe.Checkout` fail-closed on retired | 4062 |
| 1351 | `Enroll.FreeCourse` fail-closed on retired | 4063 |
| 1352 | `Enroll.PaidCourse` fail-closed on retired | 4064 |
| 1353 | `Enroll.PaidCourse` Helium-first | 4065 |
| 1354 | EnsureCanvasUser projects entitled Canvas seats | 4066 |
| 1355 | `orders` refund columns | 4067 |
| 1356 | QueryRef 150 revoked / unenrolled lines | 4068 |
| 1357 | `Enroll.CanvasSeat` | 4069 |
| 1358 | `Enroll.Archive` Canvas unenroll / reenroll | 4070 |
| 1359 | `Enroll.SyncEnrollments` Canvas heal | 4071 |
| 1360 | `Account.Orders` | 4072 |
| 1361 | `Stripe.Refund` | 4073 |
| 1362 | `Stripe.DeactivateCustomer` | 4074 |
| 1363 | QueryRef 147 filters `Q` / `LEVEL` / `PAID_FREE` / `TAG` / `SORT` / `HIDE_RETIRED` | 4075 |

## Files that stay in Acuranzo

These numbers sit in or just after the Courses span. They stay, and
their numbers stay. The hole is whatever is in the move table.

| Stays | Why |
| --- | --- |
| [1199](/elements/002-helium/acuranzo/migrations/acuranzo_1199.lua), [1200](/elements/002-helium/acuranzo/migrations/acuranzo_1200.lua) | `contact_submissions`, QueryRef 86. Reception keeps calling 86. |
| [1284](/elements/002-helium/acuranzo/migrations/acuranzo_1284.lua), [1285](/elements/002-helium/acuranzo/migrations/acuranzo_1285.lua) | `user_registration_meta`, QueryRef 143. OIDC registration. |
| [1296](/elements/002-helium/acuranzo/migrations/acuranzo_1296.lua) | `Api.Echo` conduit fixture. |
| [1297](/elements/002-helium/acuranzo/migrations/acuranzo_1297.lua) | `scripts.invokable`. |
| [1298](/elements/002-helium/acuranzo/migrations/acuranzo_1298.lua) | QueryRef **149**. Hydrogen hardcodes it as `H_SCRIPTING_INVOKABLE_SCRIPT_QUERYREF` in `src/scripting/scripting_invoke.c`. |
| [1365](/elements/002-helium/acuranzo/migrations/acuranzo_1365.lua) through [1377](/elements/002-helium/acuranzo/migrations/acuranzo_1377.lua) | MCP scripts and QueryRefs 152/153, then mail-claim QueryRef 154. |
| [1380](/elements/002-helium/acuranzo/migrations/acuranzo_1380.lua) through [1388](/elements/002-helium/acuranzo/migrations/acuranzo_1388.lua) | Staff roles, QueryRef 155, role-origin lookup, dialect lookup, transaction fixtures. |
| [1210](/elements/002-helium/acuranzo/migrations/acuranzo_1210.lua), [1289](/elements/002-helium/acuranzo/migrations/acuranzo_1289.lua), [1299](/elements/002-helium/acuranzo/migrations/acuranzo_1299.lua), [1343](/elements/002-helium/acuranzo/migrations/acuranzo_1343.lua), [1364](/elements/002-helium/acuranzo/migrations/acuranzo_1364.lua) | `Orchestrators.Orchestrator`. 1289 also inserts the first `Provision.EnsureCanvasUser`. Moving 1289 would replay that orchestrator rewrite after 1364. |

QueryRef **149** and QueryRef **155** do not move. Lithium
`src/managers/course-manager/course-manager.js` already calls 155 for
role names. That call keeps working.

## Lithium, when that pass starts

Not part of the Reception update. Recorded so it is not rediscovered.

- [1378](/elements/002-helium/acuranzo/migrations/acuranzo_1378.lua) adds Lookup 042 keys 33 Scripting, 34 Course Manager, 35 Course Builder. Key 33 is general. Keys 34 and 35 are the course UI. Leave the file in Acuranzo until the Lithium pass decides whether those two keys move.
- [1379](/elements/002-helium/acuranzo/migrations/acuranzo_1379.lua) adds Lookup 048 group Learning. Same decision.
- [`/elements/003-lithium/CATCHUP.md`](/elements/003-lithium/CATCHUP.md) already names QueryRefs 85, 150, and 151 for the course manager. Those three move. 155 stays.

## When a number is assigned

The cut below is assigned. Phase 13 adds files 4076–4083. Caller
refs 4500–4504 came with 4076–4080. `courses_4081.lua` rewrites
4502 and `courses_4082.lua` rewrites 4501, so they keep those refs.
`courses_4083.lua` installs 4505. Andrew opened the live list, so
4076 and Lookup 059 key 14 are on that database. 4077–4083 and
`acuranzo_1398.lua` still need his apply check. The next Courses
file is 4084. The next caller-facing QueryRef is 4506. One migration
installs one caller-facing ref. A rewrite keeps the ref it edits.

File numbers stay in 4000–4499. Caller refs that `auth_query` calls
stay in 4500–4999. 4000–4007 remain, and Lua still loads 4006 and
4007 by `query_id`. A new SPA ref must not reuse those integers.
Lookup 055 keeps family id 055 because that family already existed
in Acuranzo. Lookup 059 key 14 (`courses/course-manager`) is
`acuranzo_1397.lua`. `acuranzo_1398.lua` updates that same key so
the default visible columns are Code and Title. The next Acuranzo
file is 1399.

The Acuranzo README points here for the retired numbers. The generated
migration index only lists files that exist, so the hole does not
show up there by itself.
