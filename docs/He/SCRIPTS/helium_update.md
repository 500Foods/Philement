# helium_update.sh

## Overview

The `helium_update.sh` script is a comprehensive Bash utility that automates the process of updating migration indexes across all Helium database schemas and performs documentation quality checks. It calls `migration_index.sh` for each schema, validates documentation links, and checks markdown quality.

**Location:** `/elements/002-helium/scripts/helium_update.sh`

## Usage

```bash
./helium_update.sh
```

This script takes no parameters and operates on a fixed set of schemas.

## Environment Variables

- `HELIUM_ROOT`: Optional. Path to the Helium project root directory. If not set, the script assumes it's located in `scripts/` subdirectory of the project root.
- `HELIUM_DOCS_ROOT`: Optional. Path to the Helium documentation directory. If not set, defaults to `${HELIUM_ROOT}/../docs/He`.

## Functionality

The script performs comprehensive documentation maintenance operations:

### 1. Migration Index Updates

Updates migration indexes for all schemas by calling `migration_index.sh`:

```bash
./migration_index.sh ${HELIUM_ROOT}/acuranzo/README.md ${HELIUM_ROOT}/acuranzo/migrations
./migration_index.sh ${HELIUM_ROOT}/gaius/README.md ${HELIUM_ROOT}/gaius/migrations
./migration_index.sh ${HELIUM_ROOT}/glm/README.md ${HELIUM_ROOT}/glm/migrations
./migration_index.sh ${HELIUM_ROOT}/helium/README.md ${HELIUM_ROOT}/helium/migrations
```

Each call updates the corresponding schema's README.md file with the latest migration information from its migrations folder.

### 2. Documentation Link Validation

Runs `github-sitemap.sh` to check for broken links and orphaned files in the documentation:

```bash
${HELIUM_ROOT}/scripts/github-sitemap.sh ${HELIUM_DOCS_ROOT}/README.md --webroot ${HELIUM_ROOT}/../.. --include ${HELIUM_ROOT} --include ${HELIUM_DOCS_ROOT} --noreport --quiet
```

### 3. Markdown Quality Checks

Runs `markdownlint` on all markdown files to check for formatting issues:

```bash
markdownlint [all .md files in ${HELIUM_ROOT} and ${HELIUM_DOCS_ROOT}]
```

### 4. Lua Code Quality Checks

Runs `luacheck` on all migration files to validate Lua syntax and coding standards:

```bash
luacheck [all .lua files in ${HELIUM_ROOT}]
```

### 5. Repository Statistics Generation

Generates live cloc (Count Lines of Code) statistics and updates the README.md:

```bash
cloc --quiet --hide-rate ${HELIUM_ROOT} ${HELIUM_DOCS_ROOT}
```

Updates the Repository Information section with current file counts, language breakdown, and totals.

## Purpose

This script serves as a comprehensive documentation maintenance tool to:

1. **Migration Documentation**: Keep all schema README files synchronized with migration files
2. **Quality Assurance**: Validate documentation links, markdown formatting, and Lua code quality
3. **Repository Statistics**: Generate live code metrics and repository information
4. **Automation**: Simplify the complete documentation maintenance workflow

## Dependencies

- `migration_index.sh` must be in the same directory
- All schema directories (acuranzo, gaius, glm, helium) must exist
- Each schema must have a README.md and migrations/ folder
- Optional tools (quality checks skipped if not available):
  - `cloc` for code statistics
  - `luacheck` for Lua code quality
  - `github-sitemap.sh` for link validation
  - `markdownlint` for markdown quality

## Error Handling

The script will exit with a non-zero status if any quality checks fail. Individual operation failures are reported but don't necessarily stop the entire process unless critical.

## Integration

This script is typically run after:

- Adding new migrations to any schema
- Modifying existing migration files
- Updating migration metadata (versions, descriptions, etc.)
- Making changes to documentation structure
- Adding new quality assurance tools

It ensures that all documentation remains current, consistent, and high-quality.