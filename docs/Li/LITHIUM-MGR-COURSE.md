# Course Manager

Operator surface for 500 Courses. Menu ID **34**.

Requirements live in Part 5 of
[`/mnt/extra/Projects/500-Courses-Reception/PRIORITIZE.md`](/mnt/extra/Projects/500-Courses-Reception/PRIORITIZE.md).
The active build plan is
[`/elements/003-lithium/CATCHUP.md`](/elements/003-lithium/CATCHUP.md)
Band D (Phases 12–18). Phase 13 is in progress. The packet is written
and not applied.

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
| **Table path** | `courses/course-manager` (Lookup 059 key 14) |

A person sees this manager when a resolved `roles.name` is exactly `staff` or `admin`. Anyone else gets a lock panel, and the sidebar omits the row. Opening the manager directly still shows that lock panel. The gate reads `getClaims(retrieveJWT())`.

Staff see the intro sentence, a course list, and a detail form. Search and the Retired chip run in the browser. The list query returns published, unpublished, and retired rows. **4004** stays the published catalog.

---

## Files

| File | Purpose |
|------|---------|
| `course-manager.js` | Gate, list, form, and actions |
| `course-catalog.js` | Query refs, form payloads, and button state |
| `course-access.js` | QueryRef 155 gate and sidebar filter |
| `course-manager.css` | Staff split and the denied lock panel |

Helium, not applied:

| File | What |
|------|------|
| [`courses_4076.lua`](/elements/002-helium/courses/migrations/courses_4076.lua) | QueryRef 4500, every `courses` row |
| [`courses_4077.lua`](/elements/002-helium/courses/migrations/courses_4077.lua) | QueryRef 4501, insert. `published` stays 0 |
| [`courses_4078.lua`](/elements/002-helium/courses/migrations/courses_4078.lua) | QueryRef 4502, Lithium-owned columns |
| [`courses_4079.lua`](/elements/002-helium/courses/migrations/courses_4079.lua) | QueryRef 4503, enrolment lines |
| [`courses_4080.lua`](/elements/002-helium/courses/migrations/courses_4080.lua) | QueryRef 4504, management lines |
| [`acuranzo_1397.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1397.lua) | Lookup 059 key 14 |

Lua still reads enrolment history **4006** and management history **4007**. The SPA does not. Those integers are also file numbers, and `auth_query` keeps the first cache row.

---

## What the form writes

Create requires code, slug, and title. It also stores image path, canvas course id, pricing (`free` or `paid`), featured, sort order, and a JSON object of tags. A blank canvas id is stored as null. A blank image path is stored as null. Tags are `{}` when the box is empty.

Edit writes code, slug, image path, canvas course id, pricing, featured, sort order, and tags. Title, summary, description, published, retired, and `stripe_product_id` stay as they are. Summary, description, published, the stripe product id, and the last sync time are plain text.

Sync from Canvas, Sync all linked, and Retire call `invokeScript` only when QueryRef **87** returns `invokable` 1 for `Catalog.SyncFromCanvas` or `Catalog.Retire`. Those scripts are still 0. The buttons stay disabled. The wrapper title is “not enabled on this server”.

Open Canvas uses `https://canvas.500courses.com/courses/{id}` for a positive id. An image is shown only for an `http` or `https` URL, or a site path that does not start with `//`.

A dirty row asks to save or stay. A successful insert clears the form so a second click does not insert the same row again.

---

## Access

The JWT `roles` claim is a CSV of integer `role_id` values. The client reads that claim with `getClaims(retrieveJWT())`. `getClaims()` with no argument returns null and the sidebar hides the row. The client then calls QueryRef **155** once per id (`INTEGER` `ROLEID`) and reads `roles.name`. Access is granted when any name is `staff` or `admin`.

A missing API client, a failed query, or a non-array payload denies access and writes a session-log line. Role ids 2 and 3 are not treated as staff on their own.

`auth_query` does not check staff. Any Hydrogen JWT can call 4500–4504 after the packet is applied. The manager is what hides the screen.

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
- [CATCHUP.md](/elements/003-lithium/CATCHUP.md) — Phase 13 and the later Band D phases
- [COURSES_MIGRATION.md](/docs/He/COURSES_MIGRATION.md) — pack numbers
