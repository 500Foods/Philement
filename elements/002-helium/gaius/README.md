# GAIUS Database Design

This is the database schema for the [GAIUS](http://gaiusmodel.com) database. This is mostly an add-on to the Acuranzo database, outfitting it with additional DDL and SQL specific to the GAIUS project. The project overall involves a lot of Lua work, so you're likely to find some Lua-specific UDFs in the mix.

## Migrations

| M# | Table | Version | Updated | Stmts | Diagram | Description |
|----|-------|---------|---------|-------|---------|-------------|
| [2000](gaius/migrations/gaius_2000.lua) | queries | 3.1.0 | 2025-11-23 | 9 | ✓ | Bootstraps the migration system by creating the queries table and supporting user-defined functions |
| **1** | | | | **9** | **1** | |
