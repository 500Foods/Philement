<!-- markdownlint-disable MD007 MD024 -->
# Firebase Engine Plan (stub)

> **Superseded 2026-09-18.** Do not implement from this file.

The fifth Hydrogen engine is **Firebird**, not Cloud Firestore. Firestore
is not a SQL server; Helium still emits SQL. The live plan is
[`FIREBIRD.md`](/docs/H/plans/FIREBIRD.md). MS SQL Server (Lookup 030
key 5, already seeded) is [`MSSQL.md`](/docs/H/plans/MSSQL.md).

Historical Working Log, locks, and Phases 0–7 notes:

[`FIREBASE_SUPERSEDED.md`](/docs/H/plans/complete/FIREBASE_SUPERSEDED.md)

Teardown of `src/database/firebase/`, `database_firebase.lua`, extras
emulator, and Lookup 030 key 6 relabel is **FIREBIRD Phases 3–4**. Do not
start FIREBASE Phase 8. Do not delete Cockroach names from this stub.
