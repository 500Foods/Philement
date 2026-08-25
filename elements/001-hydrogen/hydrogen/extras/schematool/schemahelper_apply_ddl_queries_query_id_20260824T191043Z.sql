-- SchemaHelper apply (catalog DDL; not executed by SchemaTool)
-- finding cat:queries:query_id:nullable
-- confirm queries.query_id
-- This ALTER statement mutates live DDL shape.
ALTER TABLE queries ALTER COLUMN query_id DROP NOT NULL;
