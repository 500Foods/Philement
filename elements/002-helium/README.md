# helium

The Helium project is primarily focused on dealing with database schemas, represented mostly by sets of "migrations" that can be used to create schemas inside of a given database engine. Different schemas have been added for different parts of this and other projects. As Hydrogen has grown in comoplexity and capability, its applications range far beyond the original aspirations of Philement, and as a result we'll end up with more schemas here that many not have anyting to do with 3D printing, but these are still valuable as they help pave the way (and pay!) for enhancements that we might not otherwise have gotten around to.

## Supported Databases

- PostgreSQL - PostgreSQL v16 and v17 should work, most testing has been with v17
- MySQL/MariaDB - MariaDB is what has been used during development
- SQLite - SQLite v3, note that the sqlean extension for base64 support is needed
- DB2 - tested with v10-v12, note that the BASE64ENCODE UDF is required (in Hydrogen extras)

## Lua

Migrations are essentially a collection of Lua scripts. Why Lua? Well, why not? Lua is a relatively simple scripting language that can be directly incorporated into a C app (like Hydrogen) as an extremely lightweight module, something like 200 KB or so. Better than Python for this application, and waaaay better than Jinja2. It also has some unique syntax that make it particularly well suited to some of the tasks we have to perform.

Generally speaking, the syntax of Lua scripts isn't complicated. It isn't tab-sensitive like Python, thankfully. Here are some tips to help get you started.

- Comments start with two dashes -- like this
- Multiline strings are wrapped with [[ long text ]]
- Multiline strings can be nested (!!) by adding equal signs, like [=[ nested text ]=] and [==[ nested nested text ]==]
- SQL statements containing multiline strings have those strings converted by the script automatically into Base64
- Nested multiline strings contain nested Base64 function calls, naturally
- Macro replacements have been set up using the same convention as Bash, as in ${MACRO}, using all-uppercase names
- Macro replacements can also be nested - check out the individual database scripts for examples
- Macro replacements can also reference local environment variables at runtime
- Query delimiters like ${QUERY_DELIMITER} and ${SUBQUERY_DELIMITER} are important as it saves us from having to parse queries

In addition to SQL statements, JSON, Markdown, HTLM and CSS, any text format really, can be directly included in migration files, without having to jump through any hoops related to signle or double quotes or quote escaping. Indentation is included in the migration file for readability, but is removed when the statements are generated.

## Scripts

The [migration_index.sh](migration_index.sh) script is used to populate a README.md file with an index of migrations automatically generated from the migration files themselves.

The [migration_update.sh](migration_update.sh) script simply runs the migration_index for each of the schemas.

## Schemas

Each schema, then, describes a set of tables and potentially als a set of queries and other SQL statements that can be applied to a database. The Hydrogen project includes mechanisms for automatically updating a database for a given engine, with automatic migration mangement, testing (forward and reverse migrations) and of course using the migrations. NOte that the Hydrogen app uses no SQL itself and is largely both schema and database agnostic in most respects. Sure, it has drivers for the supported databases, but knows nothing about their contents.

A single database can also support multiple schemas, so long as their unerlying "queries" or "migrations" table is unique to the schema, and that none of the table names overlap, if the schema creates tables. This makes it possible to combine the Acuranzo schema, for example, with one of the others, providing a web front-end to another potentially pre-existing database, as is the case with GLM.

## Schema: Acuranzo

The [Acuranzo](acuranzo/README.md) schema is intended for use with the JavaScript-based web app that forms the basis of the front-end, akin to Mainsail or others.

## Schema: GAIUS

The [GAIUS](gaius/README.md) schema describes tables for the [GAIUS Project](https://www.gaiusmodel.com) which has nothing at all to do with 3D printing.

## Schema: GLM

The [GLM](glm/README.md) schema describes tables form the [GLM Project](https://www.500foods.com) which also has nothing at all to do with 3D printing.

## Schema: Helium

The [Helium](helium/README.md) schema describes tables targeting 3D printing directly, including printer information, filament mangement, and so on.
