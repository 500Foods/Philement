#!/usr/bin/env bash

# This script runs through a check of the postgres database
# - Create database 'hydrogen'
# - Create table 'test456'
# - Insert value '789'
# - Select data from table
# - Drop table
# - Drop database

set -eou pipefail

sqlite3 hydrogen.db < sqlite_test.sql && rm hydrogen.db
