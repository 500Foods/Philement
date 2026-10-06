# helium

The primary Helium project documentation can be found in [Helium Docs](/docs/He/README.md). But since you're here, these might be of interest.

## Scripts

- [migration_index.sh](scripts/migration_index.sh) script is used to populate a README.md file with an index of migrations automatically generated from the migration files themselves.
- [helium_update.sh](scripts/helium_update.sh) script runs the migration_index for each of the schemas as well as updates the repository and performs other checks

## Schemas

- The [Acuranzo](acuranzo/README.md) schema is intended for use with the JavaScript-based web app that forms the basis of the front-end, akin to Mainsail or others. (Most active migrations; primary target PostgreSQL 15+ via YugabyteDB.)
- The [Argent](argent/README.md) pack is optional double-entry bookkeeping on the Acuranzo database (`argent_2xxx`). It is never applied alone. A future Gaius pack on that same database is reserved for the 3xxx range. The on-disk Gaius tree remains its own design.
- The [GAIUS](gaius/README.md) schema describes tables for the [GAIUS Project](https://www.gaiusmodel.com) which has nothing at all to do with 3D printing.
- The [GLM](glm/README.md) schema describes tables form the [GLM Project](https://www.500foods.com) which also has nothing at all to do with 3D printing.
- The [Helium](helium/README.md) schema describes tables targeting 3D printing directly, including printer information, filament mangement, and so on.

**For migration authors (especially AI/LLMs)**: Start at [`/elements/002-helium/AGENTS.md`](/elements/002-helium/AGENTS.md). The numbering rules are **Designs, packs, and numbers** in [`/docs/He/GUIDE.md`](/docs/He/GUIDE.md). Then read [`/docs/He/MIGRATION_ANATOMY.md`](/docs/He/MIGRATION_ANATOMY.md) and [`/docs/He/MACRO_REFERENCE.md`](/docs/He/MACRO_REFERENCE.md).
