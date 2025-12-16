# helium

The primary Helium project documentation can be found in [Helium Docs](/docs/He/README.md). But since you're here, these might be of interest.

## Scripts

- [migration_index.sh](scripts/migration_index.sh) script is used to populate a README.md file with an index of migrations automatically generated from the migration files themselves.
- [migration_update.sh](scripts/migration_update.sh) script simply runs the migration_index for each of the schemas.

## Schemas

- The [Acuranzo](acuranzo/README.md) schema is intended for use with the JavaScript-based web app that forms the basis of the front-end, akin to Mainsail or others.
- The [GAIUS](gaius/README.md) schema describes tables for the [GAIUS Project](https://www.gaiusmodel.com) which has nothing at all to do with 3D printing.
- The [GLM](glm/README.md) schema describes tables form the [GLM Project](https://www.500foods.com) which also has nothing at all to do with 3D printing.
- The [Helium](helium/README.md) schema describes tables targeting 3D printing directly, including printer information, filament mangement, and so on.
