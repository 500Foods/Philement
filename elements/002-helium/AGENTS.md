# Helium — Agent Guide

Read this before adding or editing a Helium migration. The numbering rules live in the Migration Creation Guide, section **Designs, packs, and numbers**: [`/docs/He/GUIDE.md`](/docs/He/GUIDE.md). That section wins if this file and the guide ever disagree.

**Last reviewed:** 2026-10-06.

## What to read

1. This file.
2. [`/docs/He/GUIDE.md`](/docs/He/GUIDE.md) — **Designs, packs, and numbers**, then the "For AI / LLM Migration Generation" study order.
3. [`/docs/He/MIGRATION_ANATOMY.md`](/docs/He/MIGRATION_ANATOMY.md) — the `queries` insert and the 1000 / 1003 / 1001 state machine.
4. [`/docs/He/MACRO_REFERENCE.md`](/docs/He/MACRO_REFERENCE.md) — the only macros you may use.
5. [`/docs/H/plans/ARGENT_PLAN.md`](/docs/H/plans/ARGENT_PLAN.md) when the work is the Argent pack.

## Three numbers

Do not treat these as one sequence.

- **Migration number** is `cfg.MIGRATION`, the file number. Acuranzo is 1000–1999. Argent is 2000–2999. A future Gaius pack on the Acuranzo database is 3000–3999. Take the next number above the highest file already in that thousand. The forward, reverse, and diagram rows store that number in the `query_ref` column. Hydrogen tracks AVAIL, LOAD, and APPLY once per thousand the payload ships. A thousand left out of the payload is not migrated. Inside one thousand, a file at or below that band's high-water mark is skipped.
- **`query_id`** is `MAX(query_id)+1` on the shared `queries` table. No design owns a range.
- **Caller-facing QueryRef** is `cfg.QUERY_REF`, one per migration, in the migration that installs it. It is a different sequence from the file number. `acuranzo_1232.lua` is migration 1232 and installs QueryRef 102. Uniqueness is `(query_ref, query_type_a28)`. Argent's caller-facing refs are 2000–2999, the same range as its file numbers. The same integer may be both. The query type tells them apart.

Lookup ids on the Acuranzo database are a fourth sequence. Acuranzo keeps 0–199. Argent uses 2000–2999, the same range as its file numbers and its caller-facing QueryRefs. The same integer may be a file number, a QueryRef, and a lookup id. A later pack uses its migration thousand. One lookup family per migration. The column name is `*_aN` (`status_a2000`). A key may be 0 or negative when that value is meaningful.

## Packs

Argent is an optional pack on the Acuranzo database. Same connection, same schema, same `queries`, `lookups`, and `scripts`. It adds rows and domain tables. It does not create those three tables.

Allowed payloads are `PAYLOAD:acuranzo` and `PAYLOAD:acuranzo+argent`. Later, `PAYLOAD:acuranzo+gaius` and `PAYLOAD:acuranzo+gaius+argent`. Argent alone and Gaius alone are rejected.

The folder `elements/002-helium/gaius/` still bootstraps its own database in `gaius_2000.lua`. That tree is not the 3xxx pack. Do not apply it onto Acuranzo, and do not edit it into the pack.

Each design keeps its own `database.lua` and `database_<engine>.lua`. The loader reads the copy beside the migration file. Argent's copies stay.

## Do not

- Apply a migration. Do not run `schematool` or `schemahelper` apply. Hand the packet to a human.
- Invent a `${MACRO}`.
- Put more than one caller-facing QueryRef in one file, or split one migration into per-engine files.
- Number an Acuranzo file in 2xxx, or an Argent file in 1xxx or 5xxx.
- Seed an Acuranzo lookup at 200 or above, or an Argent lookup outside 2000–2999.

## After an edit

Regenerate the payload with `payload-generate.sh` or `zsh -ic 'mka'` before migration tests 30–38 or 71. `mkt` does not refresh the archive. The binary embeds the Lua. Test 71 still diagrams Acuranzo only until Argent Phase 10.
