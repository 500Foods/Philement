# Argent Database Design

Argent is optional double-entry bookkeeping on the **Acuranzo database**. It is a second set of migrations in the same schema, using the same `queries`, `lookups`, and `scripts` tables. It is never applied by itself.

Allowed packs on that database:

- Acuranzo alone (1xxx)
- Acuranzo + Argent (1xxx, then 2xxx)
- Acuranzo + Gaius (1xxx, then 3xxx) — Gaius pack is later
- Acuranzo + Gaius + Argent

Gaius alone and Argent alone are refused. The on-disk `elements/002-helium/gaius/` tree still bootstraps its own `queries` table at `gaius_2000.lua`. That file is not this pack. When Gaius joins the Acuranzo database, that pack is numbered 3xxx. GLM and the Helium printing design stay on their own databases.

Primary target is PostgreSQL 15+ via YugabyteDB. Authoritative guidance is in `/docs/He/GUIDE.md`, `/docs/He/MIGRATION_ANATOMY.md`, and `/docs/He/MACRO_REFERENCE.md`. The plan is [`/docs/H/plans/ARGENT_PLAN.md`](/docs/H/plans/ARGENT_PLAN.md).

`database*.lua` here is a copy of the Acuranzo macro set (including MariaDB and MSSQL). The copy stays. The payload packs it as `argent/database*.lua`, and the loader uses that copy when it runs an `argent_*.lua` file.

Lookup ids **2000–2999** are Argent's block on the shared `lookups` table, the same range as Argent's migration numbers and caller-facing QueryRefs. The same integer may be all three. `organizations.status_a2000` is lookup 2000. `ledgers.ledger_type_a2001` is lookup 2001 (asset, liability, equity, income, expense). `ledgers.status_a2002` is lookup 2002 (open, closed, archive). Lookups 2000–2002 are seeded by `argent_2002.lua` through `argent_2004.lua`. Lookup 2005 is `argent_2005.lua`. Lookups 2003, 2004, and 2011 are `argent_2009.lua`, `argent_2010.lua`, and `argent_2011.lua`. QueryRef 2000 is `argent_2014.lua`. Lookup 2006 is `argent_2015.lua`. Lookups 2007 and 2008 are `argent_2017.lua` and `argent_2018.lua`. Lookup 2012 is `argent_2020.lua`. QueryRef 2001 is `argent_2022.lua`. Lookups 2009 and 2010 are `argent_2025.lua` and `argent_2026.lua`. See [`/docs/H/plans/ARGENT_PLAN.md`](/docs/H/plans/ARGENT_PLAN.md).

## Database Files

| File | Purpose |
| ------ | --------- |
| [`database.lua`](migrations/database.lua) | Converts migration files to engine-specific SQL using macros and fancy formatting tricks |
| [`database_mysql.lua`](migrations/database_mysql.lua) | MySQL-specific database configuration |
| [`database_mariadb.lua`](migrations/database_mariadb.lua) | MariaDB-specific database configuration |
| [`database_postgresql.lua`](migrations/database_postgresql.lua) | PostgreSQL-specific database configuration |
| [`database_sqlite.lua`](migrations/database_sqlite.lua) | SQLite-specific database configuration |
| [`database_db2.lua`](migrations/database_db2.lua) | IBM DB2-specific database configuration |
| [`database_firebird.lua`](migrations/database_firebird.lua) | Firebird-specific database configuration |
| [`database_mssql.lua`](migrations/database_mssql.lua) | MS SQL Server-specific database configuration |

## Migrations

| M# | Table | Version | Updated | Stmts | Diagram | Description |
| ---- | ------- | --------- | --------- | ------- | --------- | ------------- |
| [2000](/elements/002-helium/argent/migrations/argent_2000.lua) | organizations | 1.0.1 | 2026-10-06 | 6 | ✓ | Creates the organizations table |
| [2001](/elements/002-helium/argent/migrations/argent_2001.lua) | ledgers | 1.0.1 | 2026-10-06 | 6 | ✓ | Creates the ledgers table |
| [2002](/elements/002-helium/argent/migrations/argent_2002.lua) | lookups | 1.0.0 | 2026-10-07 | 7 | ✓ | Seeds lookup 2000, organization status |
| [2003](/elements/002-helium/argent/migrations/argent_2003.lua) | lookups | 1.0.0 | 2026-10-07 | 7 | ✓ | Seeds lookup 2001, ledger type |
| [2004](/elements/002-helium/argent/migrations/argent_2004.lua) | lookups | 1.0.0 | 2026-10-07 | 7 | ✓ | Seeds lookup 2002, ledger status |
| [2005](/elements/002-helium/argent/migrations/argent_2005.lua) | lookups | 1.0.0 | 2026-10-07 | 7 | ✓ | Seeds lookup 2005, contact role |
| [2006](/elements/002-helium/argent/migrations/argent_2006.lua) | currencies | 1.0.0 | 2026-10-07 | 8 | ✓ | Creates the currencies table and seeds cad and usd |
| [2007](/elements/002-helium/argent/migrations/argent_2007.lua) | ledger_terms | 1.0.0 | 2026-10-07 | 6 | ✓ | Creates the ledger_terms table |
| [2008](/elements/002-helium/argent/migrations/argent_2008.lua) | contacts | 1.0.0 | 2026-10-07 | 6 | ✓ | Creates the contacts table |
| [2009](/elements/002-helium/argent/migrations/argent_2009.lua) | lookups | 1.0.0 | 2026-10-07 | 7 | ✓ | Seeds lookup 2003, transaction status |
| [2010](/elements/002-helium/argent/migrations/argent_2010.lua) | lookups | 1.0.0 | 2026-10-07 | 7 | ✓ | Seeds lookup 2004, transaction kind |
| [2011](/elements/002-helium/argent/migrations/argent_2011.lua) | lookups | 1.0.0 | 2026-10-07 | 7 | ✓ | Seeds lookup 2011, calendar state |
| [2012](/elements/002-helium/argent/migrations/argent_2012.lua) | transactions | 1.0.0 | 2026-10-07 | 6 | ✓ | Creates the transactions table |
| [2013](/elements/002-helium/argent/migrations/argent_2013.lua) | lines | 1.0.0 | 2026-10-07 | 6 | ✓ | Creates the lines table |
| [2014](/elements/002-helium/argent/migrations/argent_2014.lua) | queries | 1.0.0 | 2026-10-07 | 5 | ✓ | QueryRef #2000 - Argent balance |
| [2015](/elements/002-helium/argent/migrations/argent_2015.lua) | lookups | 1.0.0 | 2026-10-07 | 7 | ✓ | Seeds lookup 2006, reconciliation status |
| [2016](/elements/002-helium/argent/migrations/argent_2016.lua) | reconciliations | 1.0.0 | 2026-10-07 | 6 | ✓ | Creates the reconciliations table |
| [2017](/elements/002-helium/argent/migrations/argent_2017.lua) | lookups | 1.0.0 | 2026-10-07 | 7 | ✓ | Seeds lookup 2007, schedule status |
| [2018](/elements/002-helium/argent/migrations/argent_2018.lua) | lookups | 1.0.0 | 2026-10-07 | 7 | ✓ | Seeds lookup 2008, horizon mode |
| [2019](/elements/002-helium/argent/migrations/argent_2019.lua) | schedules | 1.0.0 | 2026-10-07 | 6 | ✓ | Creates the schedules table |
| [2020](/elements/002-helium/argent/migrations/argent_2020.lua) | lookups | 1.0.0 | 2026-10-07 | 7 | ✓ | Seeds lookup 2012, rate source |
| [2021](/elements/002-helium/argent/migrations/argent_2021.lua) | rates | 1.0.0 | 2026-10-07 | 6 | ✓ | Creates the rates table |
| [2022](/elements/002-helium/argent/migrations/argent_2022.lua) | queries | 1.0.0 | 2026-10-07 | 5 | ✓ | QueryRef #2001 - Argent rollup |
| [2023](/elements/002-helium/argent/migrations/argent_2023.lua) | tax_codes | 1.0.0 | 2026-10-07 | 6 | ✓ | Creates the tax_codes table |
| [2024](/elements/002-helium/argent/migrations/argent_2024.lua) | tax_rates | 1.0.0 | 2026-10-07 | 6 | ✓ | Creates the tax_rates table |
| [2025](/elements/002-helium/argent/migrations/argent_2025.lua) | lookups | 1.0.0 | 2026-10-07 | 7 | ✓ | Seeds lookup 2009, entity type |
| [2026](/elements/002-helium/argent/migrations/argent_2026.lua) | lookups | 1.0.0 | 2026-10-07 | 7 | ✓ | Seeds lookup 2010, attachment type |
| [2027](/elements/002-helium/argent/migrations/argent_2027.lua) | tags | 1.0.0 | 2026-10-07 | 6 | ✓ | Creates the tags table |
| [2028](/elements/002-helium/argent/migrations/argent_2028.lua) | tag_links | 1.0.0 | 2026-10-07 | 6 | ✓ | Creates the tag_links table |
| [2029](/elements/002-helium/argent/migrations/argent_2029.lua) | attachments | 1.0.0 | 2026-10-07 | 6 | ✓ | Creates the attachments table |
| [2030](/elements/002-helium/argent/migrations/argent_2030.lua) | scripts | 1.0.0 | 2026-10-07 | 6 | ✓ | Argent.ListOrganizations and Argent.UpsertOrganization |
| [2031](/elements/002-helium/argent/migrations/argent_2031.lua) | scripts | 1.0.0 | 2026-10-07 | 6 | ✓ | Argent.ListLedgers and Argent.GetLedger |
| [2032](/elements/002-helium/argent/migrations/argent_2032.lua) | scripts | 1.0.0 | 2026-10-07 | 5 | ✓ | Argent.UpsertLedger |
| [2033](/elements/002-helium/argent/migrations/argent_2033.lua) | scripts | 1.0.0 | 2026-10-07 | 6 | ✓ | Argent.UpsertLedgerTerms and Argent.UpsertContact |
| [2034](/elements/002-helium/argent/migrations/argent_2034.lua) | scripts | 1.0.0 | 2026-10-07 | 5 | ✓ | Argent.PostTransaction |
| [2035](/elements/002-helium/argent/migrations/argent_2035.lua) | scripts | 1.0.0 | 2026-10-07 | 7 | ✓ | Argent.AddTags, Argent.RemoveTags, and Argent.AddAttachment |
| [2036](/elements/002-helium/argent/migrations/argent_2036.lua) | scripts | 1.0.0 | 2026-10-07 | 6 | ✓ | Argent.UpsertTaxCode and Argent.UpsertTaxRate |
| [2037](/elements/002-helium/argent/migrations/argent_2037.lua) | scripts | 1.0.0 | 2026-10-07 | 7 | ✓ | Argent.GetTransaction, Argent.ListTransactions, and Argent.QueryBalances |
| **38** | | | | **241** | **38** | |
