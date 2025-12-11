# get_migration.sh

This script retrieves migration data using Lua scripts.

## Purpose

The `get_migration.sh` script is a wrapper that executes Lua-based migration retrieval functionality. It navigates to the appropriate migration directory and runs the Lua script with provided parameters.

## Features

- Environment variable validation (HYDROGEN_ROOT, HELIUM_ROOT)
- Directory navigation to migration locations
- Lua script execution for migration processing

## Usage

Requires HYDROGEN_ROOT and HELIUM_ROOT to be set. Takes 4 arguments passed to the Lua script.

## Dependencies

- Lua
- get_migration.lua (companion Lua script)