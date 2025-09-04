#!/usr/bin/env bash

# This script runs through a check of the postgres database
# - Create database 'hydrogen'
# - Create table 'test456'
# - Insert value '789'
# - Select data from table
# - Drop table
# - Drop database

set -eou pipefail

# Input SQL file
SQL_FILE="mysql_test.sql"

# Check if the SQL file exists
if [[ ! -f "${SQL_FILE}" ]]; then
  echo "Error: ${SQL_FILE} not found"
  exit 1
fi

# Test MySQL/MariaDB connection
if ! mysql -e "SELECT 1;" >/dev/null 2>&1; then
  echo "Error: Cannot connect to MySQL/MariaDB. Check ~/.my.cnf or server status."
  exit 1
fi

# Read statements from SQL file, ignoring empty lines and comments
statements=()
current_statement=""
while IFS= read -r line || [[ -n "${line}" ]]; do
  # Skip empty lines or comments
  if [[ -z "${line}" || "${line}" =~ ^\s*-- || "${line}" =~ ^\s*# ]]; then
    continue
  fi
  current_statement="${current_statement} ${line}"
  # Check for semicolon to end statement
  if [[ "${line}" =~ \;[[:space:]]*$ ]]; then
    # Trim and remove trailing semicolon
    current_statement=$(echo "${current_statement}" | xargs | sed 's/;$//' || true)
    statements+=("${current_statement}")
    current_statement=""
  fi
done < "${SQL_FILE}"

# Execute the SQL file in one session and capture output
output=$(mysql --table --show-warnings < "${SQL_FILE}" 2>&1)
exit_status=$?

# Check for overall failure
if [[ ${exit_status} -ne 0 ]]; then
  echo "Error executing SQL script:"
  echo "${output}"
  exit 1
fi

# Print each statement and its result
select_statement="SELECT * FROM TEST456"
for ((i=0; i<${#statements[@]}; i++)); do
  echo "--------------"
  echo "${statements[${i}]}"
  echo "--------------"
  # If the statement is the SELECT query, print the output
  if [[ "${statements[${i}]}" == "${select_statement}" ]]; then
    echo -e "${output}"
  fi
  echo "Command completed successfully"
done
