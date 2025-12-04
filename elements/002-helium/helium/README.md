# Helium Database Design

The Helium migration is intended for use within the 3D printing environment. This could be used by someone with a single printer, in an embedded capacity, typically with SQLite. Or it could be used to host substantially larger datasets like broad printer part assemblies. Or maybe it could be used for someone with a print farm, managing jobs and that ssort of thing. Filament tracking also comes to mind.

Early days, so lots of things could find their way into here.

## Migrations

| M# | Table | Version | Updated | Stmts | Diagram | Description |
|----|-------|---------|---------|-------|---------|-------------|
| [4000](helium/migrations/helium_4000.lua) | queries | 3.1.0 | 2025-11-23 | 9 | ✓ | Bootstraps the migration system by creating the queries table and supporting user-defined functions |
| **1** | | | | **9** | **1** | |
