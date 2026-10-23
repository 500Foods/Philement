# Courses pack

Courses is a pack on the Acuranzo database. It uses the same connection, the same SQL schema, and the same `queries`, `lookups`, and `scripts` tables. It does not bootstrap those tables. `PAYLOAD:courses` alone is refused. The suite strings that include it are `PAYLOAD:acuranzo+courses` and `PAYLOAD:acuranzo+courses+argent`.

Files are `courses_4000.lua` through `courses_4083.lua`. The next file is 4084. File numbers stay below 4500. Caller-facing QueryRefs already used are 4000–4007 and 4500–4505. Refs that `auth_query` calls stay in 4500–4999 and are never file numbers. Later patches keep the ref the installer used. A rewrite keeps that same ref. The next QueryRef is 4506.

Lookup 055 keeps family id 055. `Orchestrators.Orchestrator` stays in Acuranzo (`acuranzo_1210.lua`, plus the courses-era patches 1289, 1299, 1343, and 1364). File 1289 also inserts the first `Provision.EnsureCanvasUser`. Later bodies of that script are in this pack. The old-to-new inventory is [`/docs/He/COURSES_MIGRATION.md`](/docs/He/COURSES_MIGRATION.md).

Macro scripts are the shared set in [`/elements/002-helium/helium/`](/elements/002-helium/helium/). This design does not keep a copy.

## Migrations

| M# | Table | Version | Updated | Stmts | Diagram | Description |
| ---- | ------- | --------- | --------- | ------- | --------- | ------------- |
| [4000](/elements/002-helium/courses/migrations/courses_4000.lua) | lookups | 1.0.0 | 2025-12-26 | 6 | ✗ | Defaults for Lookup 055 - Canvas LMS Links |
| [4001](/elements/002-helium/courses/migrations/courses_4001.lua) | course_suggestions | 1.0.0 | 2026-06-24 | 8 | ✓ | Creates the course_suggestions table (Cap Phase 2b: Suggest a Course form) |
| [4002](/elements/002-helium/courses/migrations/courses_4002.lua) | queries | 1.0.0 | 2026-06-24 | 4 | ✗ | QueryRef #4000 - Insert Course Suggestion (Cap-protected, slow queue) |
| [4003](/elements/002-helium/courses/migrations/courses_4003.lua) | account_canvas_links | 1.0.0 | 2026-08-08 | 7 | ✓ | Creates the account_canvas_links table (Band E/F Phase 23/30) |
| [4004](/elements/002-helium/courses/migrations/courses_4004.lua) | queries | 1.0.0 | 2026-08-08 | 4 | ✗ | QueryRef #4001 - Canvas: Lookup Link by Account (Phase 30) |
| [4005](/elements/002-helium/courses/migrations/courses_4005.lua) | queries | 1.0.0 | 2026-08-08 | 4 | ✗ | QueryRef #4002 - Canvas: Link Account to Canvas User (Phase 30) |
| [4006](/elements/002-helium/courses/migrations/courses_4006.lua) | queries | 1.0.0 | 2026-08-08 | 4 | ✗ | QueryRef #4003 - Canvas: Touch Link (Phase 30) |
| [4007](/elements/002-helium/courses/migrations/courses_4007.lua) | scripts | 1.0.4 | 2026-08-08 | 4 | ✗ | Phase 31: intro course enrollment on first Canvas provision |
| [4008](/elements/002-helium/courses/migrations/courses_4008.lua) | courses | 1.0.0 | 2026-08-08 | 8 | ✓ | Phase 35: courses catalog table (Band G) |
| [4009](/elements/002-helium/courses/migrations/courses_4009.lua) | course_prices | 1.0.0 | 2026-08-08 | 8 | ✓ | Phase 35: course_prices table (Band G + STRIPE_PLAN Phase 3) |
| [4010](/elements/002-helium/courses/migrations/courses_4010.lua) | courses | 1.0.3 | 2026-08-09 | 4 | ✗ | Phase 36: seed courses from live Canvas catalog + assets (free; no prices) |
| [4011](/elements/002-helium/courses/migrations/courses_4011.lua) | queries | 1.0.0 | 2026-08-08 | 4 | ✗ | QueryRef #4004 - Courses: List Published (Phase 37) |
| [4012](/elements/002-helium/courses/migrations/courses_4012.lua) | queries | 1.0.0 | 2026-08-08 | 4 | ✗ | QueryRef #4005 - Courses: Get by Slug (Phase 37) |
| [4013](/elements/002-helium/courses/migrations/courses_4013.lua) | scripts | 1.0.0 | 2026-08-12 | 4 | ✗ | Phase 49: seed Enroll.FreeCourse (invokable) for Reception free enroll |
| [4014](/elements/002-helium/courses/migrations/courses_4014.lua) | user_preferences | 1.0.0 | 2026-08-12 | 6 | ✓ | Creates the user_preferences table (Band L Phase 52) |
| [4015](/elements/002-helium/courses/migrations/courses_4015.lua) | scripts | 1.0.0 | 2026-08-13 | 4 | ✗ | Phase 53: seed Account.GetSettings (invokable) for Reception settings read |
| [4016](/elements/002-helium/courses/migrations/courses_4016.lua) | scripts | 1.0.0 | 2026-08-13 | 4 | ✗ | Phase 53: seed Account.UpdatePrefs (invokable) for Reception settings write |
| [4017](/elements/002-helium/courses/migrations/courses_4017.lua) | mail_templates | 1.0.0 | 2026-08-12 | 4 | ✗ | Seed Reception user-notification mail templates (Band L Phase 55) |
| [4018](/elements/002-helium/courses/migrations/courses_4018.lua) | scripts | 1.0.0 | 2026-08-13 | 4 | ✗ | Phase 56: seed Mail.Notices.CourseExpiration (invokable) for expiring/expired mail |
| [4019](/elements/002-helium/courses/migrations/courses_4019.lua) | scripts | 1.0.0 | 2026-08-13 | 4 | ✗ | Phase 56 follow-up: rename Mail.Notices / CourseExpiration to first-dot split |
| [4020](/elements/002-helium/courses/migrations/courses_4020.lua) | user_enrollments | 1.0.0 | 2026-08-13 | 9 | ✓ | Creates the user_enrollments table (Band M Phase 59) |
| [4021](/elements/002-helium/courses/migrations/courses_4021.lua) | scripts | 1.0.0 | 2026-08-13 | 4 | ✗ | Phase 60: seed Enroll.SyncProgress (invokable) for Canvas progress cache |
| [4022](/elements/002-helium/courses/migrations/courses_4022.lua) | scripts | 1.0.0 | 2026-08-13 | 4 | ✗ | Phase 61: seed Account.MyCourses (invokable) for Reception My Courses list |
| [4023](/elements/002-helium/courses/migrations/courses_4023.lua) | accounts | 1.0.0 | 2026-08-13 | 6 | ✗ | Adds accounts.stripe_customer_id (STRIPE_PLAN Phase 1) |
| [4024](/elements/002-helium/courses/migrations/courses_4024.lua) | orders | 1.0.0 | 2026-08-13 | 8 | ✓ | Creates the orders table (STRIPE_PLAN Phase 1) |
| [4025](/elements/002-helium/courses/migrations/courses_4025.lua) | scripts | 1.0.0 | 2026-08-13 | 4 | ✗ | STRIPE_PLAN Phase 2: seed Stripe.EnsureCustomer (invokable) |
| [4026](/elements/002-helium/courses/migrations/courses_4026.lua) | queries | 1.0.0 | 2026-08-13 | 6 | ✗ | STRIPE_PLAN Phase 3: QueryRefs 4004/4005 SELECT stripe_price_id |
| [4027](/elements/002-helium/courses/migrations/courses_4027.lua) | scripts | 1.0.0 | 2026-08-13 | 4 | ✗ | STRIPE_PLAN Phase 2 follow-up: seed Stripe.SyncCustomer (invokable) |
| [4028](/elements/002-helium/courses/migrations/courses_4028.lua) | scripts | 1.0.0 | 2026-08-13 | 4 | ✗ | STRIPE_PLAN Phase 5: seed Stripe.Checkout (invokable) |
| [4029](/elements/002-helium/courses/migrations/courses_4029.lua) | courses | 1.0.0 | 2026-08-13 | 6 | ✗ | STRIPE_PLAN Phase 3: seed sandbox paid course + course_prices |
| [4030](/elements/002-helium/courses/migrations/courses_4030.lua) | scripts | 1.0.0 | 2026-08-13 | 4 | ✗ | STRIPE_PLAN Phase 5 follow-up: Stripe.Checkout nil-bind INSERT fix |
| [4031](/elements/002-helium/courses/migrations/courses_4031.lua) | scripts | 1.0.0 | 2026-08-13 | 4 | ✗ | STRIPE_PLAN Phase 7/8: seed Enroll.PaidCourse (module, not invokable) |
| [4032](/elements/002-helium/courses/migrations/courses_4032.lua) | scripts | 1.0.0 | 2026-08-13 | 4 | ✗ | STRIPE_PLAN Phase 7: seed Stripe.Webhook (invokable) |
| [4033](/elements/002-helium/courses/migrations/courses_4033.lua) | scripts | 1.0.0 | 2026-08-17 | 4 | ✗ | PRIORITIZE 2.11: Account.UpdatePrefs empty-optional nil-bind fix |
| [4034](/elements/002-helium/courses/migrations/courses_4034.lua) | scripts | 1.0.0 | 2026-08-20 | 4 | ✗ | PRIORITIZE 2.8: Enroll.FreeCourse writes user_enrollments |
| [4035](/elements/002-helium/courses/migrations/courses_4035.lua) | scripts | 1.0.0 | 2026-08-20 | 4 | ✗ | PRIORITIZE 2.8: Provision.EnsureCanvasUser writes intro user_enrollments |
| [4036](/elements/002-helium/courses/migrations/courses_4036.lua) | scripts | 1.0.0 | 2026-08-20 | 4 | ✗ | PRIORITIZE 2.8: seed Enroll.SyncEnrollments (invokable) Canvas→Helium backfill |
| [4037](/elements/002-helium/courses/migrations/courses_4037.lua) | scripts | 1.0.0 | 2026-08-20 | 4 | ✗ | PRIORITIZE 2.7: seed Enroll.Archive (invokable) hide overlay |
| [4038](/elements/002-helium/courses/migrations/courses_4038.lua) | enrollment_events | 1.0.0 | 2026-08-20 | 8 | ✓ | PRIORITIZE 2.9: create enrollment_events table |
| [4039](/elements/002-helium/courses/migrations/courses_4039.lua) | queries | 1.0.0 | 2026-08-20 | 4 | ✗ | QueryRef #4006 - Enrolment History lines (PRIORITIZE 2.9) |
| [4040](/elements/002-helium/courses/migrations/courses_4040.lua) | scripts | 1.0.0 | 2026-08-20 | 4 | ✗ | PRIORITIZE 2.9: seed Enroll.LogEvent (module, not invokable) |
| [4041](/elements/002-helium/courses/migrations/courses_4041.lua) | scripts | 1.0.0 | 2026-08-20 | 4 | ✗ | PRIORITIZE 2.9: Enroll.FreeCourse appends enrollment_events |
| [4042](/elements/002-helium/courses/migrations/courses_4042.lua) | scripts | 1.0.0 | 2026-08-20 | 4 | ✗ | PRIORITIZE 2.9: Enroll.PaidCourse appends enrollment_events |
| [4043](/elements/002-helium/courses/migrations/courses_4043.lua) | scripts | 1.0.0 | 2026-08-20 | 4 | ✗ | PRIORITIZE 2.9: Enroll.Archive appends enrollment_events |
| [4044](/elements/002-helium/courses/migrations/courses_4044.lua) | scripts | 1.0.0 | 2026-08-20 | 4 | ✗ | PRIORITIZE 2.9: Provision.EnsureCanvasUser appends enrollment_events |
| [4045](/elements/002-helium/courses/migrations/courses_4045.lua) | scripts | 1.0.0 | 2026-08-20 | 4 | ✗ | PRIORITIZE 2.9: Enroll.SyncEnrollments appends enrollment_events |
| [4046](/elements/002-helium/courses/migrations/courses_4046.lua) | courses | 1.0.0 | 2026-08-20 | 9 | ✗ | PRIORITIZE 2.29: courses.tags JSON + canvas_updated_at |
| [4047](/elements/002-helium/courses/migrations/courses_4047.lua) | catalog_events | 1.0.0 | 2026-08-20 | 7 | ✓ | PRIORITIZE 2.29: create catalog_events table |
| [4048](/elements/002-helium/courses/migrations/courses_4048.lua) | queries | 1.0.0 | 2026-08-20 | 4 | ✗ | QueryRef #4007 - Management History lines (PRIORITIZE 2.29) |
| [4049](/elements/002-helium/courses/migrations/courses_4049.lua) | scripts | 1.0.0 | 2026-08-20 | 4 | ✗ | PRIORITIZE 2.29: seed Catalog.LogEvent (module, not invokable) |
| [4050](/elements/002-helium/courses/migrations/courses_4050.lua) | scripts | 1.0.0 | 2026-08-20 | 4 | ✗ | PRIORITIZE 2.29: seed Catalog.SyncFromCanvas (not invokable) |
| [4051](/elements/002-helium/courses/migrations/courses_4051.lua) | queries | 1.0.0 | 2026-08-20 | 6 | ✗ | PRIORITIZE 2.29: QueryRefs 4004/4005 SELECT tags |
| [4052](/elements/002-helium/courses/migrations/courses_4052.lua) | scripts | 1.0.0 | 2026-08-21 | 4 | ✗ | PRIORITIZE 2.27: seed Catalog.GetBySlug (invokable) enrolled unpublished detail |
| [4053](/elements/002-helium/courses/migrations/courses_4053.lua) | account_canvas_alerts | 1.0.0 | 2026-08-21 | 6 | ✓ | PRIORITIZE 2.1: account_canvas_alerts |
| [4054](/elements/002-helium/courses/migrations/courses_4054.lua) | mail_templates | 1.0.0 | 2026-08-21 | 4 | ✗ | PRIORITIZE 2.1: ops.canvas_user_unlinked mail template |
| [4055](/elements/002-helium/courses/migrations/courses_4055.lua) | scripts | 1.0.0 | 2026-08-21 | 4 | ✗ | PRIORITIZE 2.1: replace Provision.EnsureCanvasUser |
| [4056](/elements/002-helium/courses/migrations/courses_4056.lua) | scripts | 1.0.0 | 2026-08-21 | 4 | ✗ | PRIORITIZE 2.1: replace Enroll.FreeCourse |
| [4057](/elements/002-helium/courses/migrations/courses_4057.lua) | scripts | 1.0.0 | 2026-08-21 | 4 | ✗ | PRIORITIZE 2.34: seed Stripe.EnsureProduct (not invokable) |
| [4058](/elements/002-helium/courses/migrations/courses_4058.lua) | courses | 1.0.0 | 2026-08-21 | 6 | ✗ | PRIORITIZE 2.35: courses.retired (searchable, not buyable) |
| [4059](/elements/002-helium/courses/migrations/courses_4059.lua) | queries | 1.0.0 | 2026-08-21 | 6 | ✗ | PRIORITIZE 2.35: QueryRefs 4004/4005 SELECT retired |
| [4060](/elements/002-helium/courses/migrations/courses_4060.lua) | scripts | 1.0.0 | 2026-08-21 | 4 | ✗ | PRIORITIZE 2.35: Catalog.GetBySlug SELECT retired |
| [4061](/elements/002-helium/courses/migrations/courses_4061.lua) | scripts | 1.0.0 | 2026-08-21 | 4 | ✗ | PRIORITIZE 2.35: seed Catalog.Retire (not invokable) |
| [4062](/elements/002-helium/courses/migrations/courses_4062.lua) | scripts | 1.0.0 | 2026-08-21 | 4 | ✗ | PRIORITIZE 2.35: Stripe.Checkout fail-closed on retired |
| [4063](/elements/002-helium/courses/migrations/courses_4063.lua) | scripts | 1.0.0 | 2026-08-21 | 4 | ✗ | PRIORITIZE 2.35: Enroll.FreeCourse fail-closed on retired |
| [4064](/elements/002-helium/courses/migrations/courses_4064.lua) | scripts | 1.0.0 | 2026-08-21 | 4 | ✗ | PRIORITIZE 2.35: Enroll.PaidCourse fail-closed on retired |
| [4065](/elements/002-helium/courses/migrations/courses_4065.lua) | scripts | 1.0.0 | 2026-08-21 | 4 | ✗ | PRIORITIZE 2.1 leftover: Enroll.PaidCourse Helium-first, find-don't-create |
| [4066](/elements/002-helium/courses/migrations/courses_4066.lua) | scripts | 1.0.0 | 2026-08-21 | 4 | ✗ | PRIORITIZE 2.1 leftover: EnsureCanvasUser projects entitled Canvas seats |
| [4067](/elements/002-helium/courses/migrations/courses_4067.lua) | orders | 1.0.0 | 2026-08-21 | 12 | ✗ | PRIORITIZE 2.18: orders refund columns |
| [4068](/elements/002-helium/courses/migrations/courses_4068.lua) | queries | 1.0.1 | 2026-10-16 | 4 | ✗ | PRIORITIZE 2.18: QueryRef #4006 revoked / unenrolled lines |
| [4069](/elements/002-helium/courses/migrations/courses_4069.lua) | scripts | 1.0.0 | 2026-08-21 | 4 | ✗ | PRIORITIZE 2.18: seed Enroll.CanvasSeat (module, not invokable) |
| [4070](/elements/002-helium/courses/migrations/courses_4070.lua) | scripts | 1.0.0 | 2026-08-21 | 4 | ✗ | PRIORITIZE 2.18: Enroll.Archive Canvas unenroll / reenroll |
| [4071](/elements/002-helium/courses/migrations/courses_4071.lua) | scripts | 1.0.0 | 2026-08-21 | 4 | ✗ | PRIORITIZE 2.18: Enroll.SyncEnrollments Canvas heal |
| [4072](/elements/002-helium/courses/migrations/courses_4072.lua) | scripts | 1.0.0 | 2026-08-21 | 4 | ✗ | PRIORITIZE 2.18: seed Account.Orders (invokable) |
| [4073](/elements/002-helium/courses/migrations/courses_4073.lua) | scripts | 1.0.0 | 2026-08-21 | 4 | ✗ | PRIORITIZE 2.18: seed Stripe.Refund (not invokable) |
| [4074](/elements/002-helium/courses/migrations/courses_4074.lua) | scripts | 1.0.0 | 2026-08-21 | 4 | ✗ | PRIORITIZE 2.18: seed Stripe.DeactivateCustomer (not invokable) |
| [4075](/elements/002-helium/courses/migrations/courses_4075.lua) | queries | 1.0.0 | 2026-08-21 | 4 | ✗ | PRIORITIZE 2.19: QueryRef 4004 Q / LEVEL / PAID_FREE / TAG / SORT / HIDE_RETIRED |
| [4076](/elements/002-helium/courses/migrations/courses_4076.lua) | queries | 1.0.0 | 2026-10-22 | 4 | ✗ | QueryRef #4500 - Staff: List Courses |
| [4077](/elements/002-helium/courses/migrations/courses_4077.lua) | queries | 1.0.0 | 2026-10-22 | 4 | ✗ | QueryRef #4501 - Staff: Insert Course |
| [4078](/elements/002-helium/courses/migrations/courses_4078.lua) | queries | 1.0.0 | 2026-10-22 | 4 | ✗ | QueryRef #4502 - Staff: Update Course |
| [4079](/elements/002-helium/courses/migrations/courses_4079.lua) | queries | 1.0.0 | 2026-10-22 | 4 | ✗ | QueryRef #4503 - Staff: Enrolment History |
| [4080](/elements/002-helium/courses/migrations/courses_4080.lua) | queries | 1.0.0 | 2026-10-22 | 4 | ✗ | QueryRef #4504 - Staff: Management History |
| [4081](/elements/002-helium/courses/migrations/courses_4081.lua) | queries | 1.0.0 | 2026-10-22 | 4 | ✗ | QueryRef #4502 also writes retired |
| [4082](/elements/002-helium/courses/migrations/courses_4082.lua) | queries | 1.0.0 | 2026-10-22 | 4 | ✗ | QueryRef #4501 also writes retired |
| [4083](/elements/002-helium/courses/migrations/courses_4083.lua) | queries | 1.0.0 | 2026-10-22 | 4 | ✗ | QueryRef #4505 - Staff: List Course Prices |
| **84** | | | | **398** | **84** | |
