#!/usr/bin/env bash

# This script runs through a check of the postgres database
# - Create database 'hydrogen'
# - Create table 'test456'
# - Insert value '789'
# - Select data from table
# - Drop table
# - Drop database

#set -ou pipefail

# Input SQL file
SQL_FILE="postgres_test.sql"

# Check if SQL file exists
if [[ ! -f "${SQL_FILE}" ]]; then
  echo "Error: ${SQL_FILE} not found"
  exit 1
fi

# Test PostgreSQL connection
if ! psql -U postgres -c "SELECT 1;" >/dev/null 2>&1; then
  echo "Error: Cannot connect to PostgreSQL. Check ~/.pgpass or server status."
  exit 1
fi

# Read statements from SQL file
statements=()
current_statement=""
while IFS= read -r line || [[ -n "${line}" ]]; do
  # Skip empty lines or comments
  if [[ -z "${line}" || "${line}" =~ ^\s*-- || "${line}" =~ ^\s*# ]]; then
    continue
  fi
  current_statement="${current_statement} ${line}"
  # Check for semicolon or psql meta-commands (like \c)
  if [[ "${line}" =~ \;[[:space:]]*$ || "${line}" =~ ^\\c[[:space:]] ]]; then
    # Trim whitespace, preserve backslash for \c
    # Trim leading whitespace
    current_statement="${current_statement#"${current_statement%%[![:space:]]*}"}"
    # Trim trailing whitespace
    current_statement="${current_statement%"${current_statement##*[![:space:]]}"}"
    if [[ ! "${current_statement}" =~ ^\\c ]]; then
      # Remove trailing semicolon for SQL statements
      current_statement="${current_statement%;}"
    fi
    statements+=("${current_statement}")
    current_statement=""
  fi
done < "${SQL_FILE}"

# Create temporary files for CREATE DATABASE and remaining statements
TEMP_CREATE=$(mktemp)
TEMP_TEST=$(mktemp)
create_found=false
statement_index=0
while IFS= read -r line || [[ -n "${line}" ]]; do
  if [[ "${line}" =~ ^CREATE\ DATABASE ]]; then
    echo "${line}" >> "${TEMP_CREATE}"
    printf '%s\n' "\\echo MARKER_${statement_index}" >> "${TEMP_CREATE}"
    create_found=true
  else
    echo "${line}" >> "${TEMP_TEST}"
    if [[ "${line}" =~ \;[[:space:]]*$ || "${line}" =~ ^\\c[[:space:]] ]]; then
      printf '%s\n' "\\echo MARKER_${statement_index}" >> "${TEMP_TEST}"
      ((statement_index++))
    fi
  fi
done < "${SQL_FILE}"

# Execute SQL files
if [[ "${create_found}" = true ]]; then
  output_create=$(psql -U postgres -f "${TEMP_CREATE}" 2>&1)
  exit_status_create=$?
else
  output_create=""
  exit_status_create=0
fi
output_test=$(psql -U postgres -f "${TEMP_TEST}" 2>&1)
exit_status_test=$?

# Clean up temporary files
rm "${TEMP_CREATE}" "${TEMP_TEST}"

# Check for failures
if [[ ${exit_status_create} -ne 0 ]]; then
  echo "Error executing CREATE DATABASE:"
  printf '%s\n' "${output_create}"
  exit 1
fi
if [[ ${exit_status_test} -ne 0 ]]; then
  echo "Error executing remaining SQL:"
  printf '%s\n' "${output_test}"
  exit 1
fi

# Combine outputs
output="${output_create}"$'\n'"${output_test}"

# Split output into lines
IFS=$'\n' read -d '' -r -a output_lines <<< "${output}"

# Print each statement and its result
statement_index=1
current_output=""
for line in "${output_lines[@]}"; do
  # Skip empty lines
  if [[ -z "${line}" ]]; then
    continue
  fi
  # Check for marker
  if [[ "${line}" =~ ^MARKER_[0-9]+$ ]]; then
    if [[ ${statement_index} -lt ${#statements[@]} ]]; then
      echo "--------------"
      echo "${statements[${statement_index}]}"
      echo "--------------"
      printf '%b' "${current_output}" | sed '/^[[:space:]]*$/d'
      if [[ "${current_output}" =~ CREATE\ DATABASE || "${current_output}" =~ CREATE\ TABLE || "${current_output}" =~ INSERT\ 0 || "${current_output}" =~ DROP\ TABLE || "${current_output}" =~ DROP\ DATABASE || "${current_output}" =~ You\ are\ now\ connected || "${current_output}" =~ [0-9]+\ row ]]; then
        echo "Command completed successfully"
      fi
      current_output=""
      ((statement_index++))
    fi
  else
    current_output="${current_output}${line}"$'\n'
  fi
done

# Print the last statement and its result
if [[ ${statement_index} -lt ${#statements[@]} ]]; then
  echo "--------------"
  echo "${statements[${statement_index}]}"
  echo "--------------"
  printf '%b' "${current_output}" | sed '/^[[:space:]]*$/d'
  if [[ "${current_output}" =~ CREATE\ DATABASE || "${current_output}" =~ CREATE\ TABLE || "${current_output}" =~ INSERT\ 0 || "${current_output}" =~ DROP\ TABLE || "${current_output}" =~ DROP\ DATABASE || "${current_output}" =~ You\ are\ now\ connected || "${current_output}" =~ [0-9]+\ row ]]; then
    echo "Command completed successfully"
  fi
fi
