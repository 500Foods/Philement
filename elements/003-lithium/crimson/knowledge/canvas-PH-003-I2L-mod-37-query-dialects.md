---
course_code: "PH-003-I2L"
module_num: "37"
page: "query-dialects"
canvas_url: "courses/<COURSE_ID>/pages/query-dialects"
---

For many Lithium projects, a specific deployment will be performed where the database is of a certain type or in a certain location. This might mean that Lithium, and its backend Hydrogen server, is running inside of a PostgreSQL database. Or maybe SQLite if it is something small. Or DB2 if it something large. Or even MySQL/MariaDB for people who prefer those database engines.

SQL has been around a long time. Since the 1970\'s.  Over that time, it has evolved in different directions, and some database vendors have implemented things differently. Microsoft, in their true-to-form \"embrace-extend-extinguish\" business model, even renamed their version Transact-SQL. And they rely so heavily on stored procedures for *everything* that it does indeed feel like a different language.

For the most part, though, SQL in day-to-day use is largely the same. There are just minor differences to be dealt with.  To address this, the Philement project has a sub-project called Helium.  There will one day be a course just on that. It is essentially a \"migration\" system - a term used to describe how a database is constructed and evolved over time to be in a current form. Lithium relies primarily (initially) on a database schema called Acuranzo, one of its earliest incantations. The Helium project looks after converting SQL into the more refined dialects that each database needs to perform optimally.

From the prespective of Lithium, once you\'re in the Query Manager, you\'re already seeing the post-conversion SQL tailored for the environment you\'re working in. 

There is one exception though, and that is in how SQL query parameters are handled. This is one of the areas where each vendor has gone in different directions with precious little commonality in their approach. And they\'ve been creative! For reasons that aren\'t immediately obvious.  So what we\'ve done in the Lithium project is standardize on using named parameters consistently.

    SELECT
      column1
    FROM
      table
    WHERE
       column2 = :SOMEVALUE

The :SOMEVALUE is a parameter to the query. It can be used multiple times in the same query. Convention is that it is all uppercase letters and numbers, no symbols, and never starting with a number. 

When Hydrogen runs these queries against a database - any database - it does the job of converting named parameters into the parameter marshalling method of choice for the given database. Note that we\'re not talking about other methods here as they\'re just not in any way useful for our purposes.

The catch is that we, too, are then routinely not using anything resembling standard SQL dialects for these databases that use alternate methods. Only SQLite and DB2 support named parameters natively, so in those cases, the SQL is indeed standard, but for PostgreSQL and the MySQL family, not at all.

This isn\'t a huge concern as moving SQL from here into an environment where it needs to run natively isn\'t something we have to worry about in our project, but this may be a consideration if you\'re migrating SQL into or out of Lithium.  

Another consideration is that our SQL Formatter in the Query Manager has been updated to \"know\" about our preference for named parameters, despite the database, so it works as we need it to work. But this is again an exception to what might be needed outside of Lithium.

 
