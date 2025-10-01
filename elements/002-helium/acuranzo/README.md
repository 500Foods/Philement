# Acuranzo Database Design

This folder contains the migrations (aka database DDL and SQL) for createing a new insttance of an Acuranzo database. Current supported engines include SQLite, PostgreSQL, MySQL, and IBM DB2.

## Design Notes

1. Some tables do not have a primary key, such as "sessions".  This is deliberate.