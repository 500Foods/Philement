#!/usr/bin/env bash

set -eou pipefail

sqlite3 2> /dev/null hydrotst.sqlite < sqlite_init.sql
