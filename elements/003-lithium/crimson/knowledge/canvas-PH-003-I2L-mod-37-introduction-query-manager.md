---
course_code: "PH-003-I2L"
module_num: "37"
page: "introduction-query-manager"
canvas_url: "courses/<COURSE_ID>/pages/introduction-query-manager"
---

The Lithium app is essentialy the client end of a traditional client-server application. Virtually everything you do in Lithium is dependent on a backend SQL database. Hydrogen is the server that provides access to this database through its Conduit REST API. This API provides a level of abstraction where Lithium doesn\'t need to know anything about the backend database - whether it is PostgreSQL or DB2 for example. It just knows that if it asks for a specific query to run and provides the appropriate parameters in its JSON request, that the query will run and a JSON result will be returned, either with the data from an SQL SELECT statement, or a count of affected rows in the case of an SQL INSERT, UPDATE or DELETE statement. 

When you see a table in Lithium, it is most likely a direct mapping, or a subset, of a table in the backend database retrieved via an SQL SELECT statement. When you change something, it is most likely going to trigger an SQL UPDATE statement of some kind. Lithium knows what queries it is expecting to run and what data is coming back because it is built around a particular database schema. Database schemas are the domain of the Helium project, and Lithium\'s core has been based around the Acuranzo schema.

The Lithium cannot run SQL code directly against the backend database. It \*could\* be designed for that, but it isn\'t for a host of reasons.

- **Effort.** It would have to be aware of different SQL dialects, like how PostgreSQL and DB2 use different SQL commands for things like \"current timestamp\", among many, many other differences.
- **Security.** If Lithium, as an app, can run native SQL, likely an attacker could as well, and this would be a comically bad security exposure.
- **Maintenance.** Any change in Lithium\'s code would require that all backends that Lithium uses would need to be updated at the same time, or multiple versions of every SQL statement would need to be maintained. Neither is particularly a good idea from a maintenance perspective.
- **Separation.** Good design generally involves a separation of concerns, and this is a solid break. The client app doesn\'t need to know anything about the database, and the database doesn\'t need to know anything about the client. These are often two very large and specific domains, where in larger teams they would be handled by entirely different people with different skillsets.
- **Performance.** By keeping SQL out of the client and using an API like the Conduit API build into Hydrogen, the client can be small and nimble. And Hydrogen itself can also be incredibly efficient as it simply passes off the queries to the database and returns the results - hence the Conduit name. Stuff just passes through it for the most part. This is highly efficient.

This doesn\'t mean that the queries are hidden or in some way inaccessible. Not at all, just that they are managed entirely outside of both the Lithium and Hydrogen code bases - they actually live directly in the database being used. 

The Query Manager then, is the interface to those queries. A person with appropriate privileges and skills (developers, primarily) can look at virtually all of the queries that Lithium relies upon, in the native database environment that runs them, and then make adjustments as needed. There are about 100 queries that govern the basics of Lithium, like logging in, managing the main menu, and the core Managers like the Query Manager, Crimson, Session Logs and so on. But there are many more queries also used for reporting and for other Managers beyond the core Lithium project. In some environments, you can even add your own queries to perform maintainence or custom notifications and that sort of thing.

The rest of this Module will go into more detail about how to get the most out of the Query Manager, and will describe how specific features work in more detail.

 

 
