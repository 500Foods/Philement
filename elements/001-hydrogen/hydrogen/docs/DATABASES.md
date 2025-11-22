# Databases

This project makes heavy use of SQL-style databases. It is in effect a SQL-gateway, providing a REST API and Websocket interface to typically browser client apps against hosted datbases.

But, while SQL is used extensively, it is important to note that there is ZERO SQL embedded in the actual C app directly. This is a deliberate design choice based mostly on the fact that SQL itself isn't as portable as we'd like to think it is, and because we expect folks to point this at databases where we have zero advanced notice about their configuration. Different schemas, conventions, and so on. And, we'd like to be as DB-agnostic as we can.

Currently, we've built this with support for four database engines - PostgreSQL, MySQL/MariaDB, SQLite, and IBM's DB2 (LUW variant). We've assumed current versions of these, though that distinction doesn't matter all that much as we, or the integrators, have full control over what is sent to the engine, so any issues can be resolved at run-time rather than at build time.

There are of course variations in what these database engines are each capable of. In the sections that follow, the issues that have been encountered are listed. None of these have been showstoppers, but these may be the kinds of things that will help when looking to add additional database engines, and also help explain some of the design choices made so far.

## Main considerations

- **JSON.** We store a lot of stuff in JSON objects within the database. This makes it a lot easier to add generic information to the database without needing a lot of extra columns or rows for things ahead of time, particularly for data that is not normally queried directly. Not all databases support JSON the same way, and they all have different levels of robustness in terms of how JSON is presented and how willing they are to accept it. So we've opted to largely go with user defined functions so we can pass in pretty-printed JSON written the way we want, and leave it to those functions to make it work with the specific engines.

- **BASE64.** When building our migration scripts, we wanted to embed content directly into the queries, including JSON directly, single quotes, double quotes, and so on. Lua supports this easily by having a unique way of addressing multi-line strings using [[ ]], [=[ ]=], [==[ ]==], and so on, allowing arbitrary nesting. This is great in that we can mix and match quotes all we like and Lua happily works with it. However, between Lua and our database tables are several layers of scripts and conversions and so on that have to happen. To get this all to work transparently, these multi-line strings are converted into Base64-encoded strings, sometimes nested within one another. Then, when the database records are created, the database itself decodes the strings just at the moment they are inserted, guaranteeing that what we put into Lua comes out in the database unchanged. Works great! But a bit tricky to get it all working seamlessly across all engines.

- **SEQUENCE.** Many databases (maybe even all of them by now) have this idea of auto-generating primary keys by using a sequence or increment function of some kind, and plenty of folks like to use those. We've opted to not use them but instead generate a INTEGER+1 value as we populate our tables. This can be problematic in some cases, particularly in distributed databases or when there are many such inserts happening concurrently, but these aren't currently things we have to deal with. There's nothing preventing the use of these mechanisms at all, just a design choice not to, based mostly on how they didn't exist when the earliest iterations of this work were first completed (yes, that was a long, long time ago).

## PostgreSQL

- **JSON.** PG doesn't like passing in NULL as a JSON value but instead requires '{}' which is unique to** PG it seems.

- **JSON.** As with MySQL/MariaDB and DB2, sending pretty-printed JSON with newlines and so on doesn't work particularly well, so a special JSON_INGEST funciton is provided to strip out newlines and basically take the JSON as we've laid it out in the Lua migration scripts and work with it.

- **BASE64.** Nothing special here as there are Base64 encode/decode functions available and the output can easily be converted to strings as needed.

## MySQL/MariaDB

- **FLAVORS.** MySQL and MariaDB parted ways a good long time ago, but are often treated as the same engine, using the same C libraries and command-line commands. As we are in fact doing. But they are not the same and it is something to keep in mind when problems arise. While every effort is made to try to ensure that they work as one in terms of our implementation, MariaDB is what is used for dev work, so incompatibilities may well be present when using MySQL or even older or newer versions of MariaDB.

- **JSON.** As with PostgreSQL and DB2, sending pretty-printed JSON with newlines and so on doesn't work particularly well, so a special JSON_INGEST funciton is provided to strip out newlines and basically take the JSON as we've laid it out in the Lua migration scripts and work with it.

- **BASE64.** Nothing special here as there are Base64 encode/decode functions available and the output can easily be converted to strings as needed.

- **INSERT.** One of the curious differences between engines is that this one doesn't take too kindly to INSERT statements against a table that has sub-selects that reference the same table. Everyone else understands how to implement this, but here this is a no-go. This is why our migrations might seem to be structured in a little bit of an odd way.

## SQLite

- **JSON.** No direct support for JSON is provided, so we're basically just storing it as text. This is a bit of a nuisance when it comes to writing queries to extract values from JSON.

- **BASE64.** There are no built-in Base64 encode/decode functions, but they are easily made available through sqlean - a set of SQLite extensions that has these tucked into its crypto library. So that's an additional dependency specifically for SQLite.

## IBM DB2 (LUW)

- **TEXT.** While other engines all have generic "text" or "JSON" datatytpes that have no set limits, DB2 tends to want hard limits specified. So we're typically using CLOB(1M) for these kinds of things, which is plenty large for most JSON needs and even everything else, but something to be aware of particuarly if working with long documents with embedded images, as we're often doing. It can of course be increased to much larger, but this is typically the default unless specifically overriden somewhere.

- **JSON.** As with PostgreSQL and MySQL/MariaDB, sending pretty-printed JSON with newlines and so on doesn't work particularly well, so a special JSON_INGEST funciton is provided to strip out newlines and basically take the JSON as we've laid it out in the Lua migration scripts and work with it.

- **BASE64.** Less fun here. There are some versions of DB2 (z-Series, for example) that have Base64 features natively. LUW doesn't seem to be one of those. So we've written a C-based UDF for Base64 decode to provide support for this, found in the extras/base64decode_udf_db2 folder. This should be treated as a dependency as the migration scripts rely upon it heavily. Some effort went into this to ensure that it works with our CLOB(1M) size, so not restricted to a 32KB VARCHAR that UDFs might typically default to. This should work on all DB2 variants > 9.7 or so, but has only been officially tested on 11 and 12 so far. Be sure to look over the README carefully if building this for other databases.

- **PERFORMANCE.** Technically, DB2 should be soundly beating the crap out of everyone else in this party in benchmarks. However, for our migrations, DB2 tends to run at about half the speed of the others. Some time was spent trying to figure out why this is. The culprit seems to be DB2's strong desire to ensure changes are written out to disk before continuing. The others all have caches they write to, not durable storage, so they kind of cheat a little bit. Beyond migrations, we should see DB2 perk up a bit when that isn't the majorpart of the equation. Don't think of this as a bad thing - DB2 is designed first and foremost to be ultra-reliable. Which it certainly is. Whereas SQLite, in comparison, doesn't even have a database manager and writes everyting to a single file. These things are not the same.
