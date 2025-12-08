# Databases

This project makes heavy use of SQL-style databases. It is in effect a SQL-gateway, providing a REST API and WebSocket interface to typically browser client apps against hosted databases.

But, while SQL is used extensively, it is important to note that there is ZERO SQL embedded in the actual C app directly. This is a deliberate design choice based mostly on the fact that SQL itself isn't as portable as we'd like to think it is, and because we expect folks to point this at databases where we have zero advanced notice about their configuration. Different schemas, conventions, and so on. And, we'd like to be as DB-agnostic as we can.

Currently, we've built this with support for four database engines - PostgreSQL, MySQL/MariaDB, SQLite, and IBM's DB2 (LUW variant). We've assumed current versions of these, though that distinction doesn't matter all that much as we, or the integrators, have full control over what is sent to the engine, so any issues can be resolved at run-time rather than at build time.

There are of course variations in what these database engines are each capable of. In the sections that follow, the issues that have been encountered are listed. None of these have been showstoppers, but these may be the kinds of things that will help when looking to add additional database engines, and also help explain some of the design choices made so far.

## Main considerations

- **QUEUE SYSTEM.** The system employs a Database Queue Manager (DQM) architecture where each database connection has a Lead queue that orchestrates query processing and spawns child worker queues for different priorities (slow/medium/fast/cache), ensuring efficient resource utilization and performance isolation.

- **QUERY ID SYSTEM.** Rather than sending raw SQL queries, the API uses a query ID system where clients reference pre-defined query templates by numeric ID with parameters. This provides schema independence, security through query whitelisting, and enables cross-database compatibility without exposing internal SQL structures.

- **MIGRATION SYSTEM.** Database schema evolution is handled through Lua scripts that generate engine-specific SQL migrations. The system uses a two-phase LOAD/APPLY process: LOAD extracts migration metadata into the database, while APPLY executes the actual schema changes with transaction safety and rollback capabilities.

- **SQL DIALECT VARIATIONS.** While basic SQL is largely compatible, engines differ in advanced features like window functions, common table expressions (CTEs), recursive queries, and specific built-in functions. Our migration system uses a macro system to adjust queries for similar features across engines, such as "CURRENT TIMESTAMP" (DB2) vs. "current_timestamp()" (PostgreSQL, MySQL/MariaDB, and SQLite).

- **JSON.** We store a lot of stuff in JSON objects within the database. This makes it a lot easier to add generic information to the database without needing a lot of extra columns or rows for things ahead of time, particularly for data that is not normally queried directly. Not all databases support JSON the same way, and they all have different levels of robustness in terms of how JSON is presented and how willing they are to accept it. So we've opted to largely go with user-defined functions so we can pass in pretty-printed JSON written the way we want, and leave it to those functions to make it work with the specific engines.

- **BASE64.** When building our migration scripts, we wanted to embed content directly into the queries, including JSON directly, single quotes, double quotes, and so on. Lua supports this easily by having a unique way of addressing multi-line strings using [[ ]], [=[ ]=], [==[ ]==], and so on, allowing arbitrary nesting. This is great in that we can mix and match quotes all we like and Lua happily works with it. However, between Lua and our database tables are several layers of scripts and conversions and so on that have to happen. To get this all to work transparently, these multi-line strings are converted into Base64-encoded strings, sometimes nested within one another. Then, when the database records are created, the database itself decodes the strings just at the moment they are inserted, guaranteeing that what we put into Lua comes out in the database unchanged. Works great! But a bit tricky to get it all working seamlessly across all engines.

- **BROTLI.** Unfortunately, it turns out that DB2 has a hard limit on how big a string can be when the DB2 parser first analyzes an SQL statement - 32 KB. There are no such restrictions when passing parameters to DB2. So not the biggest problem. But it turns out that the migrations contain themes - large CSS blocks (ironically around 30 KB) and larger still when encoded as Base64. To get around this limit, and to be a tiny bit more efficient in terms of storage, the Lua multi-line strings that are greater than 1,000 characters are now first compressed with `brotli --best` and then encoded as Base64. This necessitated changes all the way downstream from there, specifically including the test scripts ([Test 31: Migrations](/docs/H/tests/test_31_migrations.md) and [Test 71: Database Diagrams](/docs/H/tests/test_71_database_diagrams.md) in particular), as well as in each database engine where these compressed+encoded strings needed to be decoded+decompressed before they could be inserted into the database. As a result we now have a matching set of brotli-decompress UDFs for our four database engines. Handy.

- **SEQUENCE.** Many databases (maybe even all of them by now) have this idea of auto-generating primary keys by using a sequence or increment function of some kind, and plenty of folks like to use those. We've opted to not use them but instead generate an INTEGER + 1 value as we populate our tables. This can be problematic in some cases, particularly in distributed databases or when there are many such inserts happening concurrently, but these aren't currently things we have to deal with. There's nothing preventing the use of these mechanisms at all, just a design choice not to, based mostly on how they didn't exist when the earliest iterations of this work were first completed (yes, that was a long, long time ago).

## SQL Validation and Testing

To ensure migration scripts work correctly across all supported database engines, we use sqruff for SQL syntax validation. This tool checks migrations for engine-specific SQL dialect compliance and catches potential issues before deployment. Each engine has its own sqruff configuration file (.sqruff_postgresql, .sqruff_mysql, etc.) that defines appropriate rules and exceptions.

[Test 31: Migrations](/docs/H/tests/test_31_migrations.md) runs sqruff validation on migration scripts. The individual engine migration tests ([Test 32: PostgreSQL Migrations](/docs/H/tests/test_32_postgres_migrations.md), [Test 33: MySQL Migrations](/docs/H/tests/test_33_mysql_migrations.md), [Test 34: SQLite Migrations](/docs/H/tests/test_34_sqlite_migrations.md), [Test 35: DB2 Migrations](/docs/H/tests/test_35_db2_migrations.md)) perform the full LOAD/APPLY/REVERSE cycle to validate that schema changes execute successfully and can be rolled back.

For more details on Lua scripting, sqruff usage, and the migration system, see [CURIOSITIES.md](/docs/H/CURIOSITIES.md).

## PostgreSQL

- **JSON.** PG doesn't like passing in NULL as a JSON value but instead requires '{}' which is unique to PG it seems.

- **JSON.** As with MySQL/MariaDB and DB2, sending pretty-printed JSON with newlines and so on doesn't work particularly well, so a special JSON_INGEST function is provided to strip out newlines and basically take the JSON as we've laid it out in the Lua migration scripts and work with it.

- **BASE64.** Nothing special here as there are Base64 encode/decode functions available and the output can easily be converted to strings as needed.

- **BROTLI.** Custom [BROTLI UDF](/elements/001-hydrogen/hydrogen/extras/brotli_udf_postgresql/README.md) for decoding brotli data.

## MySQL/MariaDB

- **FLAVORS.** MySQL and MariaDB parted ways a good long time ago, but are often treated as the same engine, using the same C libraries and command-line commands. As we are in fact doing. But they are not the same and it is something to keep in mind when problems arise. While every effort is made to try to ensure that they work as one in terms of our implementation, MariaDB is what is used for dev work, so incompatibilities may well be present when using MySQL or even older or newer versions of MariaDB.

- **JSON.** As with PostgreSQL and DB2, sending pretty-printed JSON with newlines and so on doesn't work particularly well, so a special JSON_INGEST function is provided to strip out newlines and basically take the JSON as we've laid it out in the Lua migration scripts and work with it.

- **BASE64.** Nothing special here as there are Base64 encode/decode functions available and the output can easily be converted to strings as needed.

- **BROTLI.** Custom [BROTLI UDF](/elements/001-hydrogen/hydrogen/extras/brotli_udf_mysql/README.md) for decoding brotli data.

- **INSERT.** MySQL has limitations with INSERT statements that include subqueries referencing the same table being inserted into. Other engines handle this scenario more gracefully, which sometimes requires restructuring our migration SQL for MySQL compatibility.

## SQLite

- **JSON.** No direct support for JSON datatypes is provided, so we're basically just storing it as text. The JSON parser used by SQLite for extracting data is able to use JSON as we've stored it - far more robust than the other engines in this regard.

- **BASE64.** There are no built-in Base64 encode/decode functions, but they are easily made available through sqlean - a set of SQLite extensions that has these tucked into its crypto library. So that's an additional dependency specifically for SQLite.

- **BROTLI.** Custom [BROTLI UDF](/elements/001-hydrogen/hydrogen/extras/brotli_udf_sqlite/README.md) for decoding brotli data.

## IBM DB2 (LUW)

- **TEXT.** While other engines all have generic "text" or "JSON" datatypes that have no set limits, DB2 tends to want hard limits specified. So we're typically using CLOB(1M) for these kinds of things, which is plenty large for most JSON needs and even everything else, but something to be aware of particularly if working with long documents with embedded images, as we're often doing. It can of course be increased to much larger, but this is typically the default unless specifically overridden somewhere.

- **TEXT.** As mentioned earlier, DB2 has a curious restriction where static strings in SQL statements are limited to 32 KB. This has implications for our migrations, where the CSS in themes, and potentially some JSON, could extend beyond 32 KB. This is particularly the case when factoring in the 33.333% overhead of Base64 encoding. A 25 KB file would thus be more than 32 KB. This limitation does not apply to parameter passing, so during normal operation, larger strings can be processed via parameters without issue. In the case of migrations, there are not currently any parameters, nor are there any mechanisms to support parameters easily. And even if there were, we'd still have to somehow get the parameter value into the database, where we'd face the exact same issue. So for now, we're limited to using Brotli+Base64 compression+encoding which, when applied to the typical CSS or JSON we're using, should allow for uncompressed+unencoded files in the range of 150 KB - 200 KB. Which is likely far larger than any individual CSS or JSON file in our project, at least at the migration stage.

- **JSON.** As with PostgreSQL and MySQL/MariaDB, sending pretty-printed JSON with newlines and so on doesn't work particularly well, so a special JSON_INGEST function is provided to strip out newlines and basically take the JSON as we've laid it out in the Lua migration scripts and work with it.

- **BASE64.** Less fun here. There are some versions of DB2 (z-Series, for example) that have Base64 features natively. LUW doesn't seem to be one of those. So we've written a C-based UDF for Base64 decode to provide support for this, found in the [extras/base64decode_udf_db2](/elements/001-hydrogen/hydrogen/extras/base64decode_udf_db2/README.md) folder. This should be treated as a dependency as the migration scripts rely upon it heavily. Some effort went into this to ensure that it works with our CLOB(1M) size, so not restricted to a 32KB VARCHAR that UDFs might typically default to. This should work on all DB2 variants > 9.7 or so, but has only been officially tested on 11 and 12 so far. Be sure to look over the README carefully if building this for other databases.

- **BROTLI.** Custom [BROTLI UDF](/elements/001-hydrogen/hydrogen/extras/brotli_udf_db2/README.md) for decoding brotli data.

- **PERFORMANCE.** Technically, DB2 should be soundly beating the crap out of everyone else in this party in benchmarks. However, for our migrations, DB2 tends to run at about half the speed of the others. Some time was spent trying to figure out why this is. The culprit seems to be DB2's strong desire to ensure changes are written out to disk before continuing. The others all have caches they write to, not durable storage, so they kind of cheat a little bit. Beyond migrations, we should see DB2 perk up a bit when that isn't the major part of the equation. Don't think of this as a bad thing - DB2 is designed first and foremost to be ultra-reliable. Which it certainly is. Whereas SQLite, in comparison, doesn't even have a database manager and writes everything to a single file. These things are not the same.
