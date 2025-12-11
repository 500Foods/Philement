# get_diagram.sh

This script generates SVG database diagrams from migration JSON data.

## Purpose

The `get_diagram.sh` script processes database migration files to create visual SVG diagrams showing database schema changes. It supports before/after comparisons and can highlight differences between migration states.

## Features

- Processes migration files in Lua format
- Extracts JSON data from migrations (supports base64 encoding and Brotli compression)
- Generates before/after state comparisons
- Produces SVG output using Node.js and get_diagram.js
- Includes metadata output for integration

## Usage

The script requires HYDROGEN_ROOT and HELIUM_ROOT environment variables to be set.

It takes parameters for migration selection and diagram generation options.

## Dependencies

- Node.js
- jq
- Brotli (for compressed migrations)
- get_diagram.js (companion JavaScript file)