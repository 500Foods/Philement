# helium
While the hydrogen element is primarily concerned with communicating with printer hardware and relaying status information, the helium element is mainly concerned with providing a database interface. We don't really want to force anyone to use a particular database, so this is likely to result in various "isotopes" (to keep the metaphor going) where a hydrogen element will connect to a helium instance. typically running on the same system but not necessarily a requirement.

## Brainstorming
It's mid-2024 as this is being written. Databases should be the norm for most of these kinds of things. Using .cfg files or .log files might be perfectly acceptable for someone who likes to tinker, but not really a great choice the rest of the time.

- Also a small and lightweight C app running as a service wherever it makes sense. On a Raspberry Pi or elsewhere.
- Database access via a REST API. Probably.
- Hydrogen can proxy the requests to the REST API from its websocket interface.
- Only database folks care about the REST API.
- Meaning, clients connect exclusively to hydrogen. The REST API is how hydrogen connects to helium.
- Intended to be a design where the database engine can be swapped out, but using the same REST API.
- Not a required 1:1 mapping between hydrogen and helium elements.
- Try to be performant, particularly with respect to logging.
- Support for the usual suspects - SQLite, Postgres, MySQL/MariaDB, IBM DB2, MSSQL.
- Be mindful of system dependencies - extra work to get installed
  
