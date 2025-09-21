#!/usr/bin/env bash

set -eou pipefail

sqlite3 hydrotst.sqlite < sqlite_init.sql
