# GLM Database Design

This migration contains the extensive DDL and SQL for the GLM database - Greenhouse Labour Management. This is a project that started in the late 90's as a DB2 database used primarily by commercial growers. Initially this was reforestation seedling nurseries, then tomato, pepper, and cucumber growers. Over the last while, clients have dwindled to primarily tomato growers and applications specificaly for PTI (Produce Traceability Initiative - aka fancy labelling).

The goal of including a migration here is to provide a path forward from an aging Windows app into a modern JavasScript web-app. It also makes for an interesting proving ground as there are plenty of examples of more complex queries and lots of sample data to play with.

As this gets implemented, it, like the other schemas here, will be structured to support DB2, MySQL/MariaDB, PostgreSQL, and SQLite engines.

## Migrations

| M# | Table | Version | Updated | Stmts | Diagram | Description |
|----|-------|---------|---------|-------|---------|-------------|
| [3000](glm/migrations/glm_3000.lua) | queries | 3.1.0 | 2025-11-23 | 9 | ✓ | Bootstraps the migration system by creating the queries table and supporting user-defined functions |
| **1** | | | | **9** | **1** | |
