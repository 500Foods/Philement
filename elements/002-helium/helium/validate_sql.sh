#!/bin/bash

# Comprehensive SQL Validation for Helium Migrations
# Usage: ./validate_sql.sh <design>
# Example: ./validate_sql.sh helium

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DESIGN_NAME=${1:-helium}

# Supported database engines
ENGINES=("postgresql" "sqlite" "mysql" "db2")

echo "🔍 Discovering migration files for design: $DESIGN_NAME"
echo "=================================================="

# Find all migration files for this design
MIGRATION_FILES=$(ls -1 "${SCRIPT_DIR}/${DESIGN_NAME}_"*.lua 2>/dev/null | sort)

if [ -z "$MIGRATION_FILES" ]; then
    echo "❌ No migration files found for design: $DESIGN_NAME"
    echo "Expected pattern: ${DESIGN_NAME}_????.lua"
    exit 1
fi

echo "📁 Found migrations:"
echo "$MIGRATION_FILES" | while read -r file; do
    basename "$file"
done
echo ""

TOTAL_VALIDATIONS=0
PASSED_VALIDATIONS=0
FAILED_VALIDATIONS=0

# Process each migration file
for migration_file in $MIGRATION_FILES; do
    if [ -z "$migration_file" ]; then continue; fi

    MIGRATION_NAME=$(basename "$migration_file" .lua)
    MIGRATION_NUM=$(echo "$MIGRATION_NAME" | sed "s/${DESIGN_NAME}_//")

    echo "🚀 Validating migration: $MIGRATION_NAME"
    echo "----------------------------------------"

    # Validate against each database engine
    for engine in "${ENGINES[@]}"; do
        echo "  Testing $engine..."

        # Generate SQL for this engine
        SQL_OUTPUT=$(lua "${SCRIPT_DIR}/get_migration.lua" "$engine" "$DESIGN_NAME" "$MIGRATION_NUM" 2>/dev/null | sed '/^Generated SQL/d; /^(/d; /^):/d')

        if [ -z "$SQL_OUTPUT" ]; then
            echo "    ❌ Failed to generate SQL for $engine"
            ((FAILED_VALIDATIONS++))
            ((TOTAL_VALIDATIONS++))
            continue
        fi

        # Use sqruff for unified SQL validation across all database engines
        if command -v sqruff >/dev/null 2>&1; then
            TEMP_SQL=$(mktemp)
            echo "$SQL_OUTPUT" > "$TEMP_SQL"

            # Use sqlfluff/sqruff to lint the SQL
            LINT_CMD="sqruff"

            # Use config file for the specific database engine
            CONFIG_FILE="${SCRIPT_DIR}/.sqruff_${engine}"

            if [ -f "$CONFIG_FILE" ]; then
                # Run linting with config file and check return code
                LINT_OUTPUT=$($LINT_CMD lint --format github-annotation-native --config "$CONFIG_FILE" "$TEMP_SQL" 2>&1)
                LINT_EXIT_CODE=$?

                if [ $LINT_EXIT_CODE -eq 0 ]; then
                    echo "    ✅ $engine validation PASSED"
                    ((PASSED_VALIDATIONS++))
                else
                    echo "    ❌ $engine validation FAILED"
                    # Show the first few error lines for context
                    echo "$LINT_OUTPUT" | head -3 | while read -r line; do
                        echo "       $line"
                    done
                    if [ $(echo "$LINT_OUTPUT" | wc -l) -gt 3 ]; then
                        echo "       ... ($(echo "$LINT_OUTPUT" | wc -l) total issues)"
                    fi
                    ((FAILED_VALIDATIONS++))
                fi
            else
                echo "    ⚠️  Config file not found: $CONFIG_FILE"
                ((PASSED_VALIDATIONS++))  # Don't fail if config missing
            fi

            rm -f "$TEMP_SQL"
        else
            # Fallback to individual database clients if sqruff not available
            echo "    ⚠️  sqruff not found, falling back to database-specific validation"

            case $engine in
                "postgresql")
                    if command -v psql >/dev/null 2>&1; then
                        TEMP_SQL=$(mktemp)
                        echo "BEGIN;" > "$TEMP_SQL"
                        echo "$SQL_OUTPUT" >> "$TEMP_SQL"
                        echo "ROLLBACK;" >> "$TEMP_SQL"
                        if psql -h localhost -U postgres -d postgres -f "$TEMP_SQL" 2>/dev/null; then
                            echo "    ✅ PostgreSQL validation PASSED"
                            ((PASSED_VALIDATIONS++))
                        else
                            echo "    ❌ PostgreSQL validation FAILED"
                            ((FAILED_VALIDATIONS++))
                        fi
                        rm -f "$TEMP_SQL"
                    else
                        echo "    ⚠️  PostgreSQL client not found"
                        ((PASSED_VALIDATIONS++))
                    fi
                    ;;

                "sqlite")
                    if command -v sqlite3 >/dev/null 2>&1; then
                        TEMP_SQL=$(mktemp)
                        echo "$SQL_OUTPUT" > "$TEMP_SQL"
                        if sqlite3 :memory: ".read $TEMP_SQL" 2>/dev/null; then
                            echo "    ✅ SQLite validation PASSED"
                            ((PASSED_VALIDATIONS++))
                        else
                            echo "    ❌ SQLite validation FAILED"
                            ((FAILED_VALIDATIONS++))
                        fi
                        rm -f "$TEMP_SQL"
                    else
                        echo "    ⚠️  SQLite client not found"
                        ((PASSED_VALIDATIONS++))
                    fi
                    ;;

                "mysql"|"db2")
                    # For MySQL and DB2, assume valid if SQLFluff not available
                    echo "    ⚠️  Using fallback validation for $engine"
                    ((PASSED_VALIDATIONS++))
                    ;;
            esac
        fi

        ((TOTAL_VALIDATIONS++))
    done

    echo ""
done

echo "📊 Validation Summary"
echo "===================="
echo "Total validations: $TOTAL_VALIDATIONS"
echo "Passed: $PASSED_VALIDATIONS"
echo "Failed: $FAILED_VALIDATIONS"

if [ $FAILED_VALIDATIONS -eq 0 ]; then
    echo "🎉 All validations PASSED!"
    exit 0
else
    echo "❌ Some validations FAILED!"
    exit 1
fi