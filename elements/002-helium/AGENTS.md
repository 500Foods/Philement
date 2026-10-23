# Helium — Agent Guide

Read this before adding or editing a Helium migration. The numbering rules live in the Migration Creation Guide, section **Designs, packs, and numbers**: [`/docs/He/GUIDE.md`](/docs/He/GUIDE.md). That section wins if this file and the guide ever disagree.

**Last reviewed:** 2026-10-23.

## What to read

1. This file.
2. [`/docs/He/GUIDE.md`](/docs/He/GUIDE.md) — **Designs, packs, and numbers**, then the "For AI / LLM Migration Generation" study order.
3. [`/docs/He/MIGRATION_ANATOMY.md`](/docs/He/MIGRATION_ANATOMY.md) — the `queries` insert and the 1000 / 1003 / 1001 state machine.
4. [`/docs/He/MACRO_REFERENCE.md`](/docs/He/MACRO_REFERENCE.md) — the only macros you may use.
5. [`/docs/H/plans/ARGENT_PLAN.md`](/docs/H/plans/ARGENT_PLAN.md) when the work is the Argent pack.
6. [`/docs/PLANNING.md`](/docs/PLANNING.md) when the work is a phase of a plan.
7. [`/docs/He/COURSES_MIGRATION.md`](/docs/He/COURSES_MIGRATION.md) when the work is the Courses pack. The pack is cut. Files run through `courses_4096.lua`. The next file is 4097. Caller-facing QueryRefs for `auth_query` use 4500–4999. The next one is 4513. Andrew confirmed the save on 2026-10-23. `courses_4087.lua` is the SQL that ran for 4506. `courses_4088.lua` through `courses_4096.lua` are not applied. Do not number a file in that band. 4000–4007 stay the earlier caller refs. Acuranzo files run through `acuranzo_1398.lua`. The next Acuranzo file is 1399.

## Three numbers

Do not treat these as one sequence.

- **Migration number** is `cfg.MIGRATION`, the file number. Acuranzo is 1000–1999. Argent is 2000–2999. A future Gaius pack on the Acuranzo database is 3000–3999. Courses is 4000–4499 for files. The cut occupies 4000–4096, so the next file is 4097. Caller-facing QueryRefs already used are 4000–4007 and 4500–4512. Andrew confirmed the save on 2026-10-23. `courses_4087.lua` is the SQL that ran for 4506. `courses_4088.lua` through `courses_4096.lua` are not applied. The inventory is [`/docs/He/COURSES_MIGRATION.md`](/docs/He/COURSES_MIGRATION.md). Take the next number above the highest file already in that thousand, and keep it below 4500. The forward, reverse, and diagram rows store that number in the `query_ref` column. Hydrogen tracks AVAIL, LOAD, and APPLY once per thousand the payload ships. A thousand left out of the payload is not migrated. Inside one thousand, a file at or below that band's high-water mark is skipped.
- **`query_id`** is `MAX(query_id)+1` on the shared `queries` table. No design owns a range.
- **Caller-facing QueryRef** is `cfg.QUERY_REF`, one per migration, in the migration that installs it. It is a different sequence from the file number. `acuranzo_1232.lua` is migration 1232 and installs QueryRef 102. Uniqueness is `(query_ref, query_type_a28)`. Argent's caller-facing refs are 2000–2999, the same range as its file numbers. Courses caller-facing refs 4000–4007 share integers with file numbers. That is safe for a typed lookup and for Lua that reads `queries.code` by `query_id`. It is not safe for `auth_query`, which keeps the first cache row. Bootstrap orders by query type descending, so the type-1001 reverse row hides the type-1 row. New Courses refs that the SPA calls go in 4500–4999. That band is not used for file numbers. The next one is 4513. 4508–4512 are assigned and not applied.

Lookup ids on the Acuranzo database are a fourth sequence. Acuranzo keeps 0–199. Argent uses 2000–2999, the same range as its file numbers and its caller-facing QueryRefs. Courses uses 4000–4999 the same way. The same integer may be a file number, a QueryRef, and a lookup id. Lookup 055 keeps family id 055 because that family already existed in Acuranzo. One lookup family per migration. The column name is `*_aN` (`status_a2000`). A key may be 0 or negative when that value is meaningful.

## Packs

Argent is an optional pack on the Acuranzo database. Same connection, same schema, same `queries`, `lookups`, and `scripts`. It adds rows and domain tables. It does not create those three tables.

The full suite is `PAYLOAD:acuranzo+courses+argent`. `PAYLOAD:acuranzo` and `PAYLOAD:acuranzo+argent` stay valid and omit Courses. Later, `PAYLOAD:acuranzo+gaius` and a list that also names Gaius. Argent, Courses, or Gaius alone is rejected. The archive rebuilt by `mka` on 2026-10-16 contains the Courses files. Tests 32–40, 62, and 73, and the SchemaTool wrappers, use the full suite. A database that already applied the old 1xxx course files must be rebuilt before this pack is applied.

The folder `elements/002-helium/gaius/` still bootstraps its own database in `gaius_2000.lua`. That tree is not the 3xxx pack. Do not apply it onto Acuranzo, and do not edit it into the pack.

One copy of `database.lua` and `database_<engine>.lua` lives in [`/elements/002-helium/helium/`](/elements/002-helium/helium/). It is not a design and it is not under `migrations/`. The payload packs that copy once as `helium/database.lua`. The loader uses a design's own `database.lua` when the payload still contains one. Otherwise it uses the shared copy. Do not put another copy beside a design's migrations.

## Do not

- Apply a migration. Do not run `schematool` or `schemahelper` apply. Hand the packet to a human.
- Invent a `${MACRO}`.
- Put more than one caller-facing QueryRef in one file, or split one migration into per-engine files.
- Number an Acuranzo file in 2xxx, or an Argent file in 1xxx or 5xxx.
- Seed an Acuranzo lookup at 200 or above, or an Argent lookup outside 2000–2999.

## After an edit

Regenerate the payload with `payload-generate.sh` or `zsh -ic 'mka'` before migration tests 30–38 or 71. `mkt` does not refresh the archive. The binary embeds the Lua. Test 71 still diagrams Acuranzo only until Argent Phase 10.
