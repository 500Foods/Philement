# migration_index.sh

## Overview

The `migration_index.sh` script is a Bash utility that automatically generates a standardized "Migrations" table section for README files. It analyzes the contents of a migrations folder and creates a markdown table with detailed information about each migration file.

**Location:** `/elements/002-helium/scripts/migration_index.sh`

## Usage

```bash
./migration_index.sh <readme_file> <migrations_folder>
```

### Parameters

- `readme_file`: Path to the README.md file to update
- `migrations_folder`: Path to the directory containing migration Lua files

### Example

```bash
./migration_index.sh acuranzo/README.md acuranzo/migrations
```

## Functionality

The script performs the following operations:

1. **Validation**: Checks that both the README file and migrations folder exist
2. **File Processing**: Scans all `*_*.lua` files in the migrations folder using natural sort order
3. **Metadata Extraction**: For each migration file, extracts:
   - Migration number from filename
   - Table name from `cfg.TABLE` configuration
   - Version from CHANGELOG comments
   - Release date from CHANGELOG comments
   - Description from the second line of the file
   - Migration count (number of `table.insert(queries` calls + QUERY_DELIMITER occurrences)
   - Diagram presence (checks for `TYPE_DIAGRAM_MIGRATION`)
4. **Table Generation**: Creates a markdown table with columns:
   - M#: Migration number (linked to the file)
   - Table: Target table name
   - Version: Migration version
   - Updated: Release date
   - Stmts: Number of SQL statements
   - Diagram: ✓ if diagram migration exists
   - Description: Migration description
5. **README Update**: Replaces any existing "## Migrations" section or appends to the end of the file

## Output Format

The generated table follows this structure:

```markdown
## Migrations

| M# | Table | Version | Updated | Stmts | Diagram | Description |
|----|-------|---------|---------|-------|---------|-------------|
| [1000](/elements/002-helium/acuranzo/migrations/acuranzo_1000.lua) | queries | 1.0.0 | 2025-09-13 | 5 | ✓ | Description here |
...
| **69** | | | | **411** | **24** | |
```

The final row shows totals for files, statements, and diagrams.

## Dependencies

- Bash shell
- Standard Unix tools: grep, sed, find, sort, mktemp, mv
- Lua migration files with specific comment and configuration formats

## Error Handling

The script will exit with an error message if:

- Incorrect number of arguments provided
- README file does not exist
- Migrations folder does not exist

## Integration

This script is typically called by `migration_update.sh` to update all schema READMEs automatically, or can be run individually for specific schemas.