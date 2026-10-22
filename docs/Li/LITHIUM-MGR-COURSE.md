# Course Manager

Operator surface for 500 Courses. Menu ID **34**.

Requirements live in Part 5 of
[`/mnt/extra/Projects/500-Courses-Reception/PRIORITIZE.md`](/mnt/extra/Projects/500-Courses-Reception/PRIORITIZE.md).
The active build plan is
[`/elements/003-lithium/CATCHUP.md`](/elements/003-lithium/CATCHUP.md)
Band D (Phases 12–18).

---

## Overview

| Property | Value |
|----------|-------|
| **ID** | 34 |
| **Location** | `src/managers/course-manager/` |
| **Type** | Menu manager |
| **Cleanup** | `cleanup()` |
| **Authz** | JWT `roles` resolved through QueryRef **155** |

A person sees this manager when a resolved `roles.name` is exactly `staff` or `admin`. Anyone else gets a lock panel, and the sidebar omits the row. Opening the manager directly still shows that lock panel.

Phase 12 is the shell: a title and one sentence. Courses, commerce, learners, and inbox are later phases. The shell has no buttons and does not call a course QueryRef.

---

## Files

| File | Purpose |
|------|---------|
| `course-manager.js` | Shell and lock panel |
| `course-access.js` | QueryRef 155 gate and sidebar filter |
| `course-manager.css` | Shell layout |

---

## Access

The JWT `roles` claim is a CSV of integer `role_id` values. The client calls QueryRef **155** once per id (`INTEGER` `ROLEID`) and reads `roles.name`. Access is granted when any name is `staff` or `admin`.

A missing API client, a failed query, or a non-array payload denies access and writes a session-log line. Role ids 2 and 3 are not treated as staff on their own.

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

- [LITHIUM-MGR.md](LITHIUM-MGR.md) — manager index
- [CATCHUP.md](/elements/003-lithium/CATCHUP.md) — Phase 12 and the later Band D phases
