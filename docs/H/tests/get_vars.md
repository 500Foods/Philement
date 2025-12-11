# get_vars.sh

This utility script extracts environment variables from source files for whitelisting purposes.

## Purpose

The `get_vars.sh` script scans files to find environment variable references and formats them for use in whitelist arrays, particularly for test configurations.

## Features

- Supports both shell script (${VAR}) and JSON (${env.VAR}) variable patterns
- Recursive directory scanning
- Automatic file type detection
- Deduplication and sorting of variables
- Output formatting for whitelist arrays

## Usage

./get_vars.sh [file|directory]

Auto-detects file types and searches for appropriate patterns.

## Examples

./get_vars.sh tests/configs/                    # All vars in configs
./get_vars.sh tests/lib/framework.sh --shell    # Shell vars in framework.sh
./get_vars.sh . --json                          # JSON vars in entire project

## Output Format

Matches the whitelist array format in test_03_shell.sh