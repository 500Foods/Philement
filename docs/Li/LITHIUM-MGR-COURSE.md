# Course Manager

Operator surface for 500 Courses. Menu ID **34**.

Requirements live in Part 5 of
[`/mnt/extra/Projects/500-Courses-Reception/PRIORITIZE.md`](/mnt/extra/Projects/500-Courses-Reception/PRIORITIZE.md).
The active build plan is
[`/elements/003-lithium/CATCHUP.md`](/elements/003-lithium/CATCHUP.md)
Band D (Phases 12–18). Phase 13 is complete (2026-10-23). Andrew
reported the latest build deployed and the latest migrations
applied. QueryRefs 4500–4505 and Lookup 059 key 14 are on that
database. The list has no Retired checkbox. Phase 14 is in
progress. Save prices **4506** and orders **4507** are written and
not applied.

---

## Overview

| Property | Value |
|----------|-------|
| **ID** | 34 |
| **Location** | `src/managers/course-manager/` |
| **Type** | Menu manager |
| **Cleanup** | `cleanup()` |
| **Authz** | JWT `roles` resolved through QueryRef **155** |
| **List** | QueryRef **4500** |
| **Insert** | QueryRef **4501** |
| **Update** | QueryRef **4502** |
| **History** | QueryRefs **4503** and **4504** |
| **Prices** | QueryRef **4505** |
| **Save prices** | QueryRef **4506** (packet, not applied) |
| **Orders** | QueryRef **4507** (packet, not applied) |
| **Table path** | `courses/course-manager` (Lookup 059 key 14) |

A person sees this manager when a resolved `roles.name` is exactly `staff` or `admin`. Anyone else gets a lock panel, and the sidebar omits the row. Opening the manager directly still shows that lock panel. The gate reads `getClaims(retrieveJWT())`.

The slot title is the caption. The left side is a LithiumTable: midnight theme, splitter, and navigator. Add, duplicate, and delete are the navigator group. Local search matches code, slug, title, and canvas id. The list has no Retired checkbox. Save and Cancel live in the slot footer. The right side has four tabs. Course holds the brochure fields and an Options section for Retired and Featured. Canvas holds the course image, summary, description, published, last sync, and the sync actions. Commerce edits cad, usd, eur, and gbp, shows test or live, and lists this course's orders. History holds the enrolment and management lines. The list query returns published, unpublished, and retired rows. **4004** stays the published catalog. Default visible columns are Code and Title after `acuranzo_1398.lua`. A saved template can keep the older columns until it is reset.

---

## Files

| File | Purpose |
|------|---------|
| `course-manager.js` | Gate, LithiumTable, tabs, and actions |
| `course-edit.js` | Footer save and cancel, duplicate, and the delete refusal |
| `course-catalog.js` | Query refs, tab list, form payloads, and button state |
| `course-commerce.js` | Price edit, orders, and Stripe actions |
| `course-access.js` | QueryRef 155 gate and sidebar filter |
| `course-manager.css` | Staff shell and the denied lock panel |

Helium:

| File | What |
|------|------|
| [`courses_4076.lua`](/elements/002-helium/courses/migrations/courses_4076.lua) | QueryRef 4500, every `courses` row |
| [`courses_4077.lua`](/elements/002-helium/courses/migrations/courses_4077.lua) | QueryRef 4501, insert. `published` stays 0 |
| [`courses_4078.lua`](/elements/002-helium/courses/migrations/courses_4078.lua) | QueryRef 4502, Lithium-owned columns |
| [`courses_4079.lua`](/elements/002-helium/courses/migrations/courses_4079.lua) | QueryRef 4503, enrolment lines |
| [`courses_4080.lua`](/elements/002-helium/courses/migrations/courses_4080.lua) | QueryRef 4504, management lines |
| [`courses_4081.lua`](/elements/002-helium/courses/migrations/courses_4081.lua) | Rewrites QueryRef 4502 so it stores `retired` |
| [`courses_4082.lua`](/elements/002-helium/courses/migrations/courses_4082.lua) | Rewrites QueryRef 4501 so it stores `retired` |
| [`courses_4083.lua`](/elements/002-helium/courses/migrations/courses_4083.lua) | QueryRef 4505, price rows for one course |
| [`courses_4084.lua`](/elements/002-helium/courses/migrations/courses_4084.lua) | QueryRef 4506, save the four currency amounts. Not applied |
| [`courses_4085.lua`](/elements/002-helium/courses/migrations/courses_4085.lua) | QueryRef 4507, orders for one course. Not applied |
| [`courses_4086.lua`](/elements/002-helium/courses/migrations/courses_4086.lua) | `Stripe.Mode`, invokable 1. Not applied |
| [`acuranzo_1397.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1397.lua) | Lookup 059 key 14 |
| [`acuranzo_1398.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1398.lua) | Same key. Default columns are Code and Title |

Lua still reads enrolment history **4006** and management history **4007**. The SPA does not. Those integers are also file numbers, and `auth_query` keeps the first cache row.

---

## What the form writes

Create requires code, slug, and title. It also stores image path, canvas course id, pricing (`free` or `paid`), featured, retired, sort order, and a JSON object of tags. A blank canvas id is stored as null. A blank image path is stored as null. Tags are `{}` when the box is empty. `published` stays 0.

Edit writes code, slug, image path, canvas course id, pricing, featured, retired, sort order, and tags. Title, summary, description, published, and `stripe_product_id` stay as they are. Summary, description, published, the stripe product id, and the last sync time are plain text. Price edits use Save prices (QueryRef **4506**) after the course row exists. The footer still saves the brochure. A blank amount deactivates that currency and keeps the saved cents. `courses_4081.lua` and `courses_4082.lua` store Retired. Andrew reported those files applied on 2026-10-23. Ensure Product, Update Stripe prices, Refund, and Deactivate stay disabled until QueryRef **87** says the script is invokable. Dashboard links open only for a `pi_` or `cus_` id when `Stripe.Mode` is test or live.

Sync from Canvas, Sync all linked, and Retire call `invokeScript` only when QueryRef **87** returns `invokable` 1 for `Catalog.SyncFromCanvas` or `Catalog.Retire`. Those scripts are still 0. The buttons stay disabled. The wrapper title is “not enabled on this server”.

Open Canvas uses `https://canvas.500courses.com/courses/{id}` for a positive id. That address is a normal Canvas course URL. Canvas uses its own session. The image on the Canvas tab follows the image path. It is drawn at 524×292, twice the 262×146 card, stretched to that box, with rounded corners and a little space above it. An image is shown for an `http` or `https` URL, or a site path that does not start with `//`.

Save and Cancel are the slot footer. Fields stay disabled until edit mode. LithiumTable saves a dirty row before it switches rows. A successful insert clears the form so a second save does not insert the same row again. Navigator delete leaves the course in the catalog and points at Options Retired. Duplicate appends `-copy` to the code and slug and ` (Copy)` to the title, clears the Canvas id and the Stripe product, and stores published and retired as 0.

---

## Access

The JWT `roles` claim is a CSV of integer `role_id` values. The client reads that claim with `getClaims(retrieveJWT())`. `getClaims()` with no argument returns null and the sidebar hides the row. The client then calls QueryRef **155** once per id (`INTEGER` `ROLEID`) and reads `roles.name`. Access is granted when any name is `staff` or `admin`.

A missing API client, a failed query, or a non-array payload denies access and writes a session-log line. Role ids 2 and 3 are not treated as staff on their own.

`auth_query` does not check staff. Any Hydrogen JWT can call 4500–4505. Andrew reported that packet applied on 2026-10-23. 4506 and 4507 are not applied yet. The manager is what hides the screen.

Menu rows come from Lookup 042 keys 34 and 35
([`acuranzo_1378.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1378.lua))
and the Learning group
([`acuranzo_1379.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1379.lua)).
Andrew reported those files, plus the `staff` and `admin` seed
([`acuranzo_1380.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1380.lua))
and QueryRef 155
([`acuranzo_1381.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1381.lua)),
applied on the 500 Courses database on 2026-10-22.

---

## Related

- [LITHIUM-MGR.md](/docs/Li/LITHIUM-MGR.md) — manager index
- [CATCHUP.md](/elements/003-lithium/CATCHUP.md) — Phase 14 is in progress. The packet is not applied
- [COURSES_MIGRATION.md](/docs/He/COURSES_MIGRATION.md) — pack numbers
