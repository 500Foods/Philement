# Hydrogen Environment Variables and Secrets

This document provides a comprehensive reference for all environment variables used in the Hydrogen project. Variables are organized by their purpose and usage context.

## Table of Contents

1. [Project Path Variables](#1-project-path-variables)
   - [PHILEMENT_ROOT](#philement_root)
   - [HYDROGEN_ROOT](#hydrogen_root)
   - [HYDROGEN_DOCS_ROOT](#hydrogen_docs_root)
   - [HELIUM_ROOT](#helium_root)
   - [HELIUM_DOCS_ROOT](#helium_docs_root)

2. [Key Project Parameters](#2-key-project-parameters)
   - [HYDROGEN_SCHEMA](#hydrogen_schema)
   - [HYDROGEN_DEV_EMAIL](#hydrogen_dev_email)

3. [Security and Encryption Keys](#3-security-and-encryption-keys)
   - [PAYLOAD_LOCK](#payload_lock)
   - [PAYLOAD_KEY](#payload_key)
   - [WEBSOCKET_KEY](#websocket_key)
   - [HYDROGEN_INSTALLER_LOCK](#hydrogen_installer_lock)
   - [HYDROGEN_INSTALLER_KEY](#hydrogen_installer_key)

4. [Demo and Test Authentication](#4-demo-and-test-authentication)
   - [HYDROGEN_DEMO_API_KEY](#hydrogen_demo_api_key)
   - [HYDROGEN_DEMO_JWT_KEY](#hydrogen_demo_jwt_key)
   - [HYDROGEN_DEMO_USER_NAME](#hydrogen_demo_user_name)
   - [HYDROGEN_DEMO_USER_PASS](#hydrogen_demo_user_pass)
   - [HYDROGEN_DEMO_ADMIN_NAME](#hydrogen_demo_admin_name)
   - [HYDROGEN_DEMO_ADMIN_PASS](#hydrogen_demo_admin_pass)
   - [HYDROGEN_DEMO_EMAIL](#hydrogen_demo_email)

5. [Database Credentials - PostgreSQL (Acuranzo)](#5-database-credentials---postgresql-acuranzo)
   - [ACURANZO_DB_HOST](#acuranzo_db_host)
   - [ACURANZO_DB_NAME](#acuranzo_db_name)
   - [ACURANZO_DB_USER](#acuranzo_db_user)
   - [ACURANZO_DB_PASS](#acuranzo_db_pass)
   - [ACURANZO_DB_PORT](#acuranzo_db_port)
   - [ACURANZO_DB_TYPE](#acuranzo_db_type)

6. [Database Credentials - MySQL (Canvas)](#6-database-credentials---mysql-canvas)
   - [CANVAS_DB_HOST](#canvas_db_host)
   - [CANVAS_DB_NAME](#canvas_db_name)
   - [CANVAS_DB_USER](#canvas_db_user)
   - [CANVAS_DB_PASS](#canvas_db_pass)
   - [CANVAS_DB_PORT](#canvas_db_port)
   - [CANVAS_DB_TYPE](#canvas_db_type)

7. [Database Credentials - DB2 (Hydrotst)](#7-database-credentials---db2-hydrotst)
   - [HYDROTST_DB_NAME](#hydrotst_db_name)
   - [HYDROTST_DB_USER](#hydrotst_db_user)
   - [HYDROTST_DB_PASS](#hydrotst_db_pass)
   - [HYDROTST_DB_TYPE](#hydrotst_db_type)

---

## 1. Project Path Variables

These variables define the root directories for the project and are required by all test scripts and build processes.

### PHILEMENT_ROOT

**Description:** Root directory path of the Philement repository.

**Tests:** All tests

**Files:**

- [/elements/001-hydrogen/hydrogen/tests/test_03_shell.sh](/elements/001-hydrogen/hydrogen/tests/test_03_shell.sh)
- [/elements/001-hydrogen/hydrogen/tests/test_04_check_links.sh](/elements/001-hydrogen/hydrogen/tests/test_04_check_links.sh)
- [/elements/001-hydrogen/hydrogen/tests/test_90_markdownlint.sh](/elements/001-hydrogen/hydrogen/tests/test_90_markdownlint.sh)
- [/elements/001-hydrogen/hydrogen/tests/lib/github-sitemap.sh](/elements/001-hydrogen/hydrogen/tests/lib/github-sitemap.sh)

**Setup:**

```bash
export PHILEMENT_ROOT="/path/to/Philement"
```

### HYDROGEN_ROOT

**Description:** Root directory path of the Hydrogen project. Required for all test scripts and build processes.

**Tests:** All tests

**Files:**

- [/elements/001-hydrogen/hydrogen/tests/test_00_all.sh](/elements/001-hydrogen/hydrogen/tests/test_00_all.sh)
- [/elements/001-hydrogen/hydrogen/tests/test_03_shell.sh](/elements/001-hydrogen/hydrogen/tests/test_03_shell.sh)
- [/elements/001-hydrogen/hydrogen/tests/test_89_coverage.sh](/elements/001-hydrogen/hydrogen/tests/test_89_coverage.sh)
- [/elements/001-hydrogen/hydrogen/tests/test_99_code_size.sh](/elements/001-hydrogen/hydrogen/tests/test_99_code_size.sh)
- [/elements/001-hydrogen/hydrogen/tests/lib/framework.sh](/elements/001-hydrogen/hydrogen/tests/lib/framework.sh)
- [/elements/001-hydrogen/hydrogen/tests/lib/coverage.sh](/elements/001-hydrogen/hydrogen/tests/lib/coverage.sh)
- [/elements/001-hydrogen/hydrogen/tests/lib/cloc.sh](/elements/001-hydrogen/hydrogen/tests/lib/cloc.sh)
- [/elements/001-hydrogen/hydrogen/tests/lib/discrepancy.sh](/elements/001-hydrogen/hydrogen/tests/lib/discrepancy.sh)
- [/elements/001-hydrogen/hydrogen/payloads/payload-generate.sh](/elements/001-hydrogen/hydrogen/payloads/payload-generate.sh)
- [/elements/001-hydrogen/hydrogen/payloads/swagger-generate.sh](/elements/001-hydrogen/hydrogen/payloads/swagger-generate.sh)
- [/elements/001-hydrogen/hydrogen/extras/make-trial.sh](/elements/001-hydrogen/hydrogen/extras/make-trial.sh)

**Setup:**

```bash
export HYDROGEN_ROOT="/path/to/Philement/elements/001-hydrogen/hydrogen"
```

### HYDROGEN_DOCS_ROOT

**Description:** Root directory path for Hydrogen documentation.

**Tests:** All tests

**Files:**

- [/elements/001-hydrogen/hydrogen/tests/test_00_all.sh](/elements/001-hydrogen/hydrogen/tests/test_00_all.sh)
- [/elements/001-hydrogen/hydrogen/tests/test_04_check_links.sh](/elements/001-hydrogen/hydrogen/tests/test_04_check_links.sh)
- [/elements/001-hydrogen/hydrogen/tests/test_90_markdownlint.sh](/elements/001-hydrogen/hydrogen/tests/test_90_markdownlint.sh)
- [/elements/001-hydrogen/hydrogen/tests/lib/cloc.sh](/elements/001-hydrogen/hydrogen/tests/lib/cloc.sh)
- [/elements/001-hydrogen/hydrogen/tests/lib/discrepancy.sh](/elements/001-hydrogen/hydrogen/tests/lib/discrepancy.sh)
- [/elements/001-hydrogen/hydrogen/tests/lib/log_output.sh](/elements/001-hydrogen/hydrogen/tests/lib/log_output.sh)
- [/elements/001-hydrogen/hydrogen/extras/make-email.sh](/elements/001-hydrogen/hydrogen/extras/make-email.sh)
- [/elements/001-hydrogen/hydrogen/extras/hbm_browser/hbm_browser_cli.js](/elements/001-hydrogen/hydrogen/extras/hbm_browser/hbm_browser_cli.js)

**Setup:**

```bash
export HYDROGEN_DOCS_ROOT="/path/to/Philement/docs/H"
```

### HELIUM_ROOT

**Description:** Root directory path of the Helium project. Required for payload generation and cross-project operations.

**Tests:** All tests

**Files:**

- [/elements/001-hydrogen/hydrogen/tests/test_00_all.sh](/elements/001-hydrogen/hydrogen/tests/test_00_all.sh)
- [/elements/001-hydrogen/hydrogen/tests/test_03_shell.sh](/elements/001-hydrogen/hydrogen/tests/test_03_shell.sh)
- [/elements/001-hydrogen/hydrogen/tests/test_71_database_diagrams.sh](/elements/001-hydrogen/hydrogen/tests/test_71_database_diagrams.sh)
- [/elements/001-hydrogen/hydrogen/tests/test_98_luacheck.sh](/elements/001-hydrogen/hydrogen/tests/test_98_luacheck.sh)
- [/elements/001-hydrogen/hydrogen/tests/lib/framework.sh](/elements/001-hydrogen/hydrogen/tests/lib/framework.sh)
- [/elements/001-hydrogen/hydrogen/tests/lib/get_diagram.sh](/elements/001-hydrogen/hydrogen/tests/lib/get_diagram.sh)
- [/elements/001-hydrogen/hydrogen/tests/lib/get_migration.sh](/elements/001-hydrogen/hydrogen/tests/lib/get_migration.sh)
- [/elements/001-hydrogen/hydrogen/payloads/payload-generate.sh](/elements/001-hydrogen/hydrogen/payloads/payload-generate.sh)

**Setup:**

```bash
export HELIUM_ROOT="/path/to/Philement/elements/002-helium"
```

### HELIUM_DOCS_ROOT

**Description:** Root directory path for Helium documentation.

**Tests:** All tests

**Files:**

- [/elements/001-hydrogen/hydrogen/tests/test_03_shell.sh](/elements/001-hydrogen/hydrogen/tests/test_03_shell.sh)

**Setup:**

```bash
export HELIUM_DOCS_ROOT="/path/to/Philement/elements/002-helium/docs"
```

---

## 2. Key Project Parameters

### HYDROGEN_SCHEMA

**Description:** The location of the JSON schema file (2020-12) used for validating Hydrogen configuration files.

**Tests:** 10-69

**Files:**

- [/elements/001-hydrogen/hydrogen/src/config/config.c](/elements/001-hydrogen/hydrogen/src/config/config.c)
- [/elements/001-hydrogen/hydrogen/tests/test_03_shell.sh](/elements/001-hydrogen/hydrogen/tests/test_03_shell.sh)
- [/elements/001-hydrogen/hydrogen/tests/test_93_jsonlint.sh](/elements/001-hydrogen/hydrogen/tests/test_93_jsonlint.sh)

**Setup:**

```bash
export HYDROGEN_SCHEMA="/path/to/hydrogen_config_schema.json"
```

### HYDROGEN_DEV_EMAIL

**Description:** The email address of the developer responsible for this Hydrogen instance. Used for notifications and support contact.

**Tests:** 02, 12

**Files:**

- [/elements/001-hydrogen/hydrogen/tests/test_03_shell.sh](/elements/001-hydrogen/hydrogen/tests/test_03_shell.sh)
- [/elements/001-hydrogen/hydrogen/extras/make-email.sh](/elements/001-hydrogen/hydrogen/extras/make-email.sh)
- [/elements/001-hydrogen/hydrogen/extras/README.md](/elements/001-hydrogen/hydrogen/extras/README.md)

**Setup:**

```bash
export HYDROGEN_DEV_EMAIL="your-email@example.com"
```

---

## 3. Security and Encryption Keys

These keys are critical for payload encryption/decryption, WebSocket authentication, and installer signing.

### PAYLOAD_LOCK

**Description:** RSA public key (base64-encoded) used for encrypting payload data during build.

**Tests:** 02, 12

**Files:**

- [/elements/001-hydrogen/hydrogen/tests/test_02_secrets.sh](/elements/001-hydrogen/hydrogen/tests/test_02_secrets.sh)
- [/elements/001-hydrogen/hydrogen/tests/test_03_shell.sh](/elements/001-hydrogen/hydrogen/tests/test_03_shell.sh)
- [/elements/001-hydrogen/hydrogen/payloads/payload-generate.sh](/elements/001-hydrogen/hydrogen/payloads/payload-generate.sh)
- [/elements/001-hydrogen/hydrogen/payloads/README.md](/elements/001-hydrogen/hydrogen/payloads/README.md)

**Generation:**

```bash
# Generate RSA key pair
openssl genrsa -out private_key.pem 2048
openssl rsa -in private_key.pem -pubout -out public_key.pem

# Export as base64 (PAYLOAD_LOCK is the public key)
export PAYLOAD_LOCK=$(cat public_key.pem | base64 -w 0)
```

**Validation:**

```bash
echo "${PAYLOAD_LOCK}" | base64 -d | openssl pkey -pubin -check -noout
```

### PAYLOAD_KEY

**Description:** RSA private key (base64-encoded) used for decrypting payload data at runtime.

**Tests:** 02, 12

**Files:**

- [/elements/001-hydrogen/hydrogen/tests/test_02_secrets.sh](/elements/001-hydrogen/hydrogen/tests/test_02_secrets.sh)
- [/elements/001-hydrogen/hydrogen/tests/test_03_shell.sh](/elements/001-hydrogen/hydrogen/tests/test_03_shell.sh)
- [/elements/001-hydrogen/hydrogen/src/config/config_defaults.c](/elements/001-hydrogen/hydrogen/src/config/config_defaults.c)
- [/elements/001-hydrogen/hydrogen/src/config/config_server.c](/elements/001-hydrogen/hydrogen/src/config/config_server.c)
- [/elements/001-hydrogen/hydrogen/tests/configs/*.json](/elements/001-hydrogen/hydrogen/tests/configs/) (multiple config files)
- [/elements/001-hydrogen/hydrogen/examples/configs/hydrogen.json](/elements/001-hydrogen/hydrogen/examples/configs/hydrogen.json)

**Generation:**

```bash
# Use the private key generated with PAYLOAD_LOCK
export PAYLOAD_KEY=$(cat private_key.pem | base64 -w 0)
```

**Validation:**

```bash
echo "${PAYLOAD_KEY}" | base64 -d | openssl rsa -check -noout
```

### WEBSOCKET_KEY

**Description:** Key used for WebSocket connections and authentication. Must be at least 8 characters.

**Tests:** 02, 12

**Files:**

- [/elements/001-hydrogen/hydrogen/tests/test_02_secrets.sh](/elements/001-hydrogen/hydrogen/tests/test_02_secrets.sh)
- [/elements/001-hydrogen/hydrogen/tests/test_03_shell.sh](/elements/001-hydrogen/hydrogen/tests/test_03_shell.sh)
- [/elements/001-hydrogen/hydrogen/tests/test_23_websockets.sh](/elements/001-hydrogen/hydrogen/tests/test_23_websockets.sh)
- [/elements/001-hydrogen/hydrogen/tests/test_26_terminal.sh](/elements/001-hydrogen/hydrogen/tests/test_26_terminal.sh)
- [/elements/001-hydrogen/hydrogen/src/config/config_defaults.c](/elements/001-hydrogen/hydrogen/src/config/config_defaults.c)
- [/elements/001-hydrogen/hydrogen/src/config/config_websocket.c](/elements/001-hydrogen/hydrogen/src/config/config_websocket.c)
- [/elements/001-hydrogen/hydrogen/tests/configs/hydrogen_test_17_startup_max.json](/elements/001-hydrogen/hydrogen/tests/configs/hydrogen_test_17_startup_max.json)
- [/elements/001-hydrogen/hydrogen/tests/configs/hydrogen_test_23_websocket_*.json](/elements/001-hydrogen/hydrogen/tests/configs/)
- [/elements/001-hydrogen/hydrogen/tests/configs/hydrogen_test_26_terminal_*.json](/elements/001-hydrogen/hydrogen/tests/configs/)
- [/elements/001-hydrogen/hydrogen/examples/configs/hydrogen.json](/elements/001-hydrogen/hydrogen/examples/configs/hydrogen.json)

**Generation:**

```bash
# Generate a secure random 64-character hex key
export WEBSOCKET_KEY=$(openssl rand -hex 32)
```

**Requirements:**

- Minimum 8 characters
- Printable ASCII characters only (no spaces or control characters)
- Should be random and unique per installation

### HYDROGEN_INSTALLER_LOCK

**Description:** RSA public key (base64-encoded) used for verifying installer signatures.

**Tests:** 02

**Files:**

- [/elements/001-hydrogen/hydrogen/tests/test_02_secrets.sh](/elements/001-hydrogen/hydrogen/tests/test_02_secrets.sh)
- [/elements/001-hydrogen/hydrogen/tests/test_03_shell.sh](/elements/001-hydrogen/hydrogen/tests/test_03_shell.sh)
- [/elements/001-hydrogen/hydrogen/tests/test_70_installer.sh](/elements/001-hydrogen/hydrogen/tests/test_70_installer.sh)
- [/elements/001-hydrogen/hydrogen/installer/installer_wrapper.sh](/elements/001-hydrogen/hydrogen/installer/installer_wrapper.sh)
- [/elements/001-hydrogen/hydrogen/installer/README.md](/elements/001-hydrogen/hydrogen/installer/README.md)

**Generation:**

```bash
# Generate RSA key pair for installer
openssl genrsa -out HYDROGEN_INSTALLER_KEY.pem 2048
openssl rsa -in HYDROGEN_INSTALLER_KEY.pem -pubout -out HYDROGEN_INSTALLER_LOCK.pem

# Export as base64
export HYDROGEN_INSTALLER_LOCK=$(cat HYDROGEN_INSTALLER_LOCK.pem | base64 -w 0)
```

**Validation:**

```bash
echo "${HYDROGEN_INSTALLER_LOCK}" | base64 -d | openssl pkey -pubin -check -noout
```

### HYDROGEN_INSTALLER_KEY

**Description:** RSA private key (base64-encoded) used for signing installer packages.

**Tests:** 02

**Files:**

- [/elements/001-hydrogen/hydrogen/tests/test_02_secrets.sh](/elements/001-hydrogen/hydrogen/tests/test_02_secrets.sh)
- [/elements/001-hydrogen/hydrogen/tests/test_03_shell.sh](/elements/001-hydrogen/hydrogen/tests/test_03_shell.sh)
- [/elements/001-hydrogen/hydrogen/tests/test_70_installer.sh](/elements/001-hydrogen/hydrogen/tests/test_70_installer.sh)
- [/elements/001-hydrogen/hydrogen/installer/README.md](/elements/001-hydrogen/hydrogen/installer/README.md)

**Generation:**

```bash
# Use the private key generated with HYDROGEN_INSTALLER_LOCK
export HYDROGEN_INSTALLER_KEY=$(cat HYDROGEN_INSTALLER_KEY.pem | base64 -w 0)
```

**Validation:**

```bash
echo "${HYDROGEN_INSTALLER_KEY}" | base64 -d | openssl rsa -check -noout
```

---

## 4. Demo and Test Authentication

These variables are used for testing authentication systems and are incorporated into database migrations.

### HYDROGEN_DEMO_API_KEY

**Description:** API key used for authenticating client applications to the Hydrogen API.

**Tests:** 40-49

**Files:**

- [/elements/001-hydrogen/hydrogen/tests/test_03_shell.sh](/elements/001-hydrogen/hydrogen/tests/test_03_shell.sh)
- [/elements/001-hydrogen/hydrogen/tests/test_40_auth.sh](/elements/001-hydrogen/hydrogen/tests/test_40_auth.sh)
- [/elements/002-helium/acuranzo/migrations/acuranzo_1145.lua](/elements/002-helium/acuranzo/migrations/acuranzo_1145.lua)

**Setup:**

```bash
export HYDROGEN_DEMO_API_KEY="demo-api-key-valid"
```

### HYDROGEN_DEMO_JWT_KEY

**Description:** Secret key used for signing JWT tokens for Hydrogen API authentication.

**Tests:** 40-49

**Files:**

- [/elements/001-hydrogen/hydrogen/tests/test_03_shell.sh](/elements/001-hydrogen/hydrogen/tests/test_03_shell.sh)
- [/elements/001-hydrogen/hydrogen/tests/unity/src/config/config_api_test_load_api_config.c](/elements/001-hydrogen/hydrogen/tests/unity/src/config/config_api_test_load_api_config.c)
- [/elements/001-hydrogen/hydrogen/tests/configs/hydrogen_test_40_*.json](/elements/001-hydrogen/hydrogen/tests/configs/)
- [/elements/001-hydrogen/hydrogen/tests/artifacts/hydrogen_config_schema.json](/elements/001-hydrogen/hydrogen/tests/artifacts/hydrogen_config_schema.json)

**Generation:**

```bash
export HYDROGEN_DEMO_JWT_KEY=$(openssl rand -hex 32)
```

### HYDROGEN_DEMO_USER_NAME

**Description:** Demo user username for testing login and authentication processes.

**Tests:** 40-49

**Files:**

- [/elements/001-hydrogen/hydrogen/tests/test_03_shell.sh](/elements/001-hydrogen/hydrogen/tests/test_03_shell.sh)
- [/elements/001-hydrogen/hydrogen/tests/test_40_auth.sh](/elements/001-hydrogen/hydrogen/tests/test_40_auth.sh)
- [/elements/002-helium/acuranzo/migrations/acuranzo_1144.lua](/elements/002-helium/acuranzo/migrations/acuranzo_1144.lua)

**Setup:**

```bash
export HYDROGEN_DEMO_USER_NAME="demouser"
```

### HYDROGEN_DEMO_USER_PASS

**Description:** Demo user password for testing login and authentication processes.

**Tests:** 40-49

**Files:**

- [/elements/001-hydrogen/hydrogen/tests/test_03_shell.sh](/elements/001-hydrogen/hydrogen/tests/test_03_shell.sh)
- [/elements/001-hydrogen/hydrogen/tests/test_40_auth.sh](/elements/001-hydrogen/hydrogen/tests/test_40_auth.sh)
- [/elements/002-helium/acuranzo/migrations/acuranzo_1144.lua](/elements/002-helium/acuranzo/migrations/acuranzo_1144.lua)

**Setup:**

```bash
export HYDROGEN_DEMO_USER_PASS="DemoPass123!"
```

### HYDROGEN_DEMO_ADMIN_NAME

**Description:** Demo admin username for testing admin login and authentication processes.

**Tests:** 40-49

**Files:**

- [/elements/001-hydrogen/hydrogen/tests/test_03_shell.sh](/elements/001-hydrogen/hydrogen/tests/test_03_shell.sh)
- [/elements/001-hydrogen/hydrogen/tests/test_40_auth.sh](/elements/001-hydrogen/hydrogen/tests/test_40_auth.sh)
- [/elements/002-helium/acuranzo/migrations/acuranzo_1144.lua](/elements/002-helium/acuranzo/migrations/acuranzo_1144.lua)

**Setup:**

```bash
export HYDROGEN_DEMO_ADMIN_NAME="demoadmin"
```

### HYDROGEN_DEMO_ADMIN_PASS

**Description:** Demo admin password for testing admin login and authentication processes.

**Tests:** 40-49

**Files:**

- [/elements/001-hydrogen/hydrogen/tests/test_03_shell.sh](/elements/001-hydrogen/hydrogen/tests/test_03_shell.sh)
- [/elements/001-hydrogen/hydrogen/tests/test_40_auth.sh](/elements/001-hydrogen/hydrogen/tests/test_40_auth.sh)
- [/elements/002-helium/acuranzo/migrations/acuranzo_1144.lua](/elements/002-helium/acuranzo/migrations/acuranzo_1144.lua)

**Setup:**

```bash
export HYDROGEN_DEMO_ADMIN_PASS="AdminPass123!"
```

### HYDROGEN_DEMO_EMAIL

**Description:** Demo email address used in authentication testing and account creation.

**Tests:** 40-49

**Files:**

- [/elements/001-hydrogen/hydrogen/tests/test_03_shell.sh](/elements/001-hydrogen/hydrogen/tests/test_03_shell.sh)
- [/elements/001-hydrogen/hydrogen/tests/test_40_auth.sh](/elements/001-hydrogen/hydrogen/tests/test_40_auth.sh)
- [/elements/002-helium/acuranzo/migrations/acuranzo_1144.lua](/elements/002-helium/acuranzo/migrations/acuranzo_1144.lua)

**Setup:**

```bash
export HYDROGEN_DEMO_EMAIL="demo@example.com"
```

---

## 5. Database Credentials - PostgreSQL (Acuranzo)

These variables configure the PostgreSQL database connection for the Acuranzo schema.

### ACURANZO_DB_HOST

**Description:** Hostname or IP address of the Acuranzo PostgreSQL database server.

**Tests:** 30-35

**Files:**

- [/elements/001-hydrogen/hydrogen/tests/test_03_shell.sh](/elements/001-hydrogen/hydrogen/tests/test_03_shell.sh)
- [/elements/001-hydrogen/hydrogen/tests/configs/hydrogen_test_30_postgres.json](/elements/001-hydrogen/hydrogen/tests/configs/hydrogen_test_30_postgres.json)
- [/elements/001-hydrogen/hydrogen/tests/configs/hydrogen_test_30_multi.json](/elements/001-hydrogen/hydrogen/tests/configs/hydrogen_test_30_multi.json)
- [/elements/001-hydrogen/hydrogen/tests/configs/hydrogen_test_32_postgres.json](/elements/001-hydrogen/hydrogen/tests/configs/hydrogen_test_32_postgres.json)
- [/elements/001-hydrogen/hydrogen/tests/configs/hydrogen_test_40_postgres.json](/elements/001-hydrogen/hydrogen/tests/configs/hydrogen_test_40_postgres.json)
- [/elements/001-hydrogen/hydrogen/tests/configs/hydrogen_test_51_postgres.json](/elements/001-hydrogen/hydrogen/tests/configs/hydrogen_test_51_postgres.json)
- [/elements/001-hydrogen/hydrogen/examples/configs/hydrogen.json](/elements/001-hydrogen/hydrogen/examples/configs/hydrogen.json)

**Setup:**

```bash
export ACURANZO_DB_HOST="localhost"
```

### ACURANZO_DB_NAME

**Description:** Name of the Acuranzo database.

**Tests:** 30-35

**Setup:**

```bash
export ACURANZO_DB_NAME="acuranzo"
```

### ACURANZO_DB_USER

**Description:** Username for connecting to the Acuranzo database.

**Tests:** 30-35

**Setup:**

```bash
export ACURANZO_DB_USER="postgres"
```

### ACURANZO_DB_PASS

**Description:** Password for connecting to the Acuranzo database.

**Tests:** 30-35

**Setup:**

```bash
export ACURANZO_DB_PASS="your_secure_password"
```

### ACURANZO_DB_PORT

**Description:** Port number for the Acuranzo PostgreSQL database server.

**Tests:** 30-35

**Setup:**

```bash
export ACURANZO_DB_PORT="5432"
```

### ACURANZO_DB_TYPE

**Description:** Type of the Acuranzo database (should be `postgresql` or `postgres`).

**Tests:** 30-35

**Setup:**

```bash
export ACURANZO_DB_TYPE="postgresql"
```

---

## 6. Database Credentials - MySQL (Canvas)

These variables configure the MySQL database connection for the Canvas schema.

### CANVAS_DB_HOST

**Description:** Hostname or IP address of the Canvas MySQL database server.

**Tests:** 30-35

**Files:**

- [/elements/001-hydrogen/hydrogen/tests/test_03_shell.sh](/elements/001-hydrogen/hydrogen/tests/test_03_shell.sh)
- [/elements/001-hydrogen/hydrogen/tests/configs/hydrogen_test_30_mysql.json](/elements/001-hydrogen/hydrogen/tests/configs/hydrogen_test_30_mysql.json)
- [/elements/001-hydrogen/hydrogen/tests/configs/hydrogen_test_30_multi.json](/elements/001-hydrogen/hydrogen/tests/configs/hydrogen_test_30_multi.json)
- [/elements/001-hydrogen/hydrogen/tests/configs/hydrogen_test_33_mysql.json](/elements/001-hydrogen/hydrogen/tests/configs/hydrogen_test_33_mysql.json)
- [/elements/001-hydrogen/hydrogen/tests/configs/hydrogen_test_40_mysql.json](/elements/001-hydrogen/hydrogen/tests/configs/hydrogen_test_40_mysql.json)
- [/elements/001-hydrogen/hydrogen/tests/configs/hydrogen_test_51_postgres.json](/elements/001-hydrogen/hydrogen/tests/configs/hydrogen_test_51_postgres.json)
- [/elements/001-hydrogen/hydrogen/examples/configs/hydrogen.json](/elements/001-hydrogen/hydrogen/examples/configs/hydrogen.json)

**Setup:**

```bash
export CANVAS_DB_HOST="localhost"
```

### CANVAS_DB_NAME

**Description:** Name of the Canvas database.

**Tests:** 30-35

**Setup:**

```bash
export CANVAS_DB_NAME="canvas"
```

### CANVAS_DB_USER

**Description:** Username for connecting to the Canvas database.

**Tests:** 30-35

**Setup:**

```bash
export CANVAS_DB_USER="root"
```

### CANVAS_DB_PASS

**Description:** Password for connecting to the Canvas database.

**Tests:** 30-35

**Setup:**

```bash
export CANVAS_DB_PASS="your_secure_password"
```

### CANVAS_DB_PORT

**Description:** Port number for the Canvas MySQL database server.

**Tests:** 30-35

**Setup:**

```bash
export CANVAS_DB_PORT="3306"
```

### CANVAS_DB_TYPE

**Description:** Type of the Canvas database (should be `mysql` or `mariadb`).

**Tests:** 30-35

**Setup:**

```bash
export CANVAS_DB_TYPE="mysql"
```

---

## 7. Database Credentials - DB2 (Hydrotst)

These variables configure the DB2 database connection for the Hydrotst schema.

### HYDROTST_DB_NAME

**Description:** Name of the Hydrotst DB2 database.

**Tests:** 30-35

**Files:**

- [/elements/001-hydrogen/hydrogen/tests/test_03_shell.sh](/elements/001-hydrogen/hydrogen/tests/test_03_shell.sh)
- [/elements/001-hydrogen/hydrogen/tests/configs/hydrogen_test_30_db2.json](/elements/001-hydrogen/hydrogen/tests/configs/hydrogen_test_30_db2.json)
- [/elements/001-hydrogen/hydrogen/tests/configs/hydrogen_test_30_multi.json](/elements/001-hydrogen/hydrogen/tests/configs/hydrogen_test_30_multi.json)
- [/elements/001-hydrogen/hydrogen/tests/configs/hydrogen_test_35_db2.json](/elements/001-hydrogen/hydrogen/tests/configs/hydrogen_test_35_db2.json)
- [/elements/001-hydrogen/hydrogen/tests/configs/hydrogen_test_40_db2.json](/elements/001-hydrogen/hydrogen/tests/configs/hydrogen_test_40_db2.json)
- [/elements/001-hydrogen/hydrogen/tests/configs/hydrogen_test_51_db2.json](/elements/001-hydrogen/hydrogen/tests/configs/hydrogen_test_51_db2.json)

**Setup:**

```bash
export HYDROTST_DB_NAME="HYDROTST"
```

### HYDROTST_DB_USER

**Description:** Username for connecting to the Hydrotst DB2 database.

**Tests:** 30-35

**Setup:**

```bash
export HYDROTST_DB_USER="db2admin"
```

### HYDROTST_DB_PASS

**Description:** Password for connecting to the Hydrotst DB2 database.

**Tests:** 30-35

**Setup:**

```bash
export HYDROTST_DB_PASS="your_secure_password"
```

### HYDROTST_DB_TYPE

**Description:** Type of the Hydrotst database (should be `db2`).

**Tests:** 30-35

**Setup:**

```bash
export HYDROTST_DB_TYPE="db2"
```

---

## Quick Setup Script

Here's a complete script to set up all required environment variables:

```bash
#!/bin/bash
# Hydrogen Environment Setup Script

# Project Paths
export PHILEMENT_ROOT="/path/to/Philement"
export HYDROGEN_ROOT="${PHILEMENT_ROOT}/elements/001-hydrogen/hydrogen"
export HYDROGEN_DOCS_ROOT="${PHILEMENT_ROOT}/docs/H"
export HELIUM_ROOT="${PHILEMENT_ROOT}/elements/002-helium"
export HELIUM_DOCS_ROOT="${HELIUM_ROOT}/docs"

# Key Project Parameters
export HYDROGEN_SCHEMA="${HYDROGEN_ROOT}/tests/artifacts/hydrogen_config_schema.json"
export HYDROGEN_DEV_EMAIL="developer@example.com"

# Generate Security Keys (run once, then save the output)
mkdir -p /tmp/hydrogen_keys
cd /tmp/hydrogen_keys

# Payload keys
openssl genrsa -out payload_private.pem 2048
openssl rsa -in payload_private.pem -pubout -out payload_public.pem
export PAYLOAD_LOCK=$(cat payload_public.pem | base64 -w 0)
export PAYLOAD_KEY=$(cat payload_private.pem | base64 -w 0)

# Installer keys
openssl genrsa -out installer_private.pem 2048
openssl rsa -in installer_private.pem -pubout -out installer_public.pem
export HYDROGEN_INSTALLER_LOCK=$(cat installer_public.pem | base64 -w 0)
export HYDROGEN_INSTALLER_KEY=$(cat installer_private.pem | base64 -w 0)

# WebSocket key
export WEBSOCKET_KEY=$(openssl rand -hex 32)

# Demo Authentication (customize as needed)
export HYDROGEN_DEMO_API_KEY="demo-api-key-valid"
export HYDROGEN_DEMO_JWT_KEY=$(openssl rand -hex 32)
export HYDROGEN_DEMO_USER_NAME="demouser"
export HYDROGEN_DEMO_USER_PASS="DemoPass123!"
export HYDROGEN_DEMO_ADMIN_NAME="demoadmin"
export HYDROGEN_DEMO_ADMIN_PASS="AdminPass123!"
export HYDROGEN_DEMO_EMAIL="demo@example.com"

# PostgreSQL (Acuranzo) - customize for your environment
export ACURANZO_DB_HOST="localhost"
export ACURANZO_DB_NAME="acuranzo"
export ACURANZO_DB_USER="postgres"
export ACURANZO_DB_PASS="your_postgres_password"
export ACURANZO_DB_PORT="5432"
export ACURANZO_DB_TYPE="postgresql"

# MySQL (Canvas) - customize for your environment
export CANVAS_DB_HOST="localhost"
export CANVAS_DB_NAME="canvas"
export CANVAS_DB_USER="root"
export CANVAS_DB_PASS="your_mysql_password"
export CANVAS_DB_PORT="3306"
export CANVAS_DB_TYPE="mysql"

# DB2 (Hydrotst) - customize for your environment
export HYDROTST_DB_NAME="HYDROTST"
export HYDROTST_DB_USER="db2admin"
export HYDROTST_DB_PASS="your_db2_password"
export HYDROTST_DB_TYPE="db2"

cd -
echo "Environment setup complete!"
```

---

## Security Best Practices

1. **Never commit secrets to version control** - Use environment variables or secure secrets management
2. **Rotate keys periodically** - Generate new RSA key pairs and update configurations
3. **Use strong passwords** - For database credentials, use complex, unique passwords
4. **Restrict key access** - Keep private keys secure with appropriate file permissions
5. **Use different keys per environment** - Development, staging, and production should have separate keys

---

## Validation Tests

Run these tests to verify your environment is properly configured:

```bash
# Test 02: Validates security keys
./tests/test_02_secrets.sh

# Test 03: Validates all environment variables
./tests/test_03_shell.sh

# Test 12: Tests environment variable processing
./tests/test_12_env_variables.sh
```

---

## References

- [README.md](/docs/H/README.md) - General Hydrogen documentation
- [payloads/README.md](/elements/001-hydrogen/hydrogen/payloads/README.md) - Payload system details
- [installer/README.md](/elements/001-hydrogen/hydrogen/installer/README.md) - Installer documentation
- [OpenSSL Documentation](https://www.openssl.org/docs/) - OpenSSL reference
