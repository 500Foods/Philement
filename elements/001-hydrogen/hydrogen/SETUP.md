# Hydrogen System Dependencies

This document outlines all system dependencies required to build and run Hydrogen.

## Build-time Dependencies

These tools are required when building or developing with the Hydrogen codebase:

- C compiler and build tools (gcc and friends)
- [curl](https://curl.se/) - Data transfer tool for downloading dependencies
- [tar](https://www.gnu.org/software/tar/) - Archiving utility (typically part of core utils)
- [wget](https://www.gnu.org/software/wget/) - Alternative network downloader
- [brotli](https://github.com/google/brotli) - Modern compression algorithm
- [jq](https://stedolan.github.io/jq/) - Command-line JSON processor
- [upx](https://upx.github.io/) - Executable compressor for release builds
- [cloc](https://github.com/AlDanial/cloc) - Count Lines of code
- [markdownlint](https://github.com/DavidAnson/markdownlint) - Markdown file linting and style checking
- [jsonlint](https://github.com/zaach/jsonlint) - JSON file validation and formatting
- [cppcheck](https://cppcheck.sourceforge.io/) - Static analysis for C/C++ code
- [eslint](https://eslint.org/) - JavaScript code linting and style checking
- [stylelint](https://stylelint.io/) - CSS/SCSS linting and style checking
- [htmlhint](https://htmlhint.com/) - HTML code linting and validation

## Example Ubuntu Build Environment

```bash
sudo apt update
sudo apt install -y build-essential wget curl jq nodejs npm cloc libjansson-dev libmicrohttpd-dev libssl-dev libwebsockets-dev libbrotli-dev upx-ucl valgrind cppcheck eslint
npm install -g markdownlint jsonlint stylelint htmlhint
```

NOTE: Latest NodeJS should be used (eg. Node 24)

With that all installed, perform the following steps.

- Generate PAYLOAD_LOCK and PAYLOAD_KEY (see [SECRETS.md](SECRETS.md) for setup instructions)
- Run `swagger_generate.sh` and `payroll_generate.sh` in the payloads directory
- Run `make trial` in the `src` folder to perform a trial build
- Run `tests_00_all.sh` in the `tests` folder to build and test the entire project

## Runtime Dependencies

These libraries are required for Hydrogen to run:

- [pthreads](https://pubs.opengroup.org/onlinepubs/7908799/xsh/pthread.h.html) - POSIX thread support for concurrent operations
- [jansson](https://github.com/akheron/jansson) - Efficient and memory-safe JSON parsing/generation
- [microhttpd](https://www.gnu.org/software/libmicrohttpd/) - Lightweight embedded HTTP server
- [libwebsockets](https://github.com/warmcat/libwebsockets) - Full-duplex WebSocket communication
- [OpenSSL](https://www.openssl.org/) (libssl/libcrypto) - Industry-standard encryption and .security
- [libm](https://www.gnu.org/software/libc/manual/html_node/Mathematics.html) - Mathematical operations support
- [libbrotlidec](https://github.com/google/brotli) - Brotli decompression library
- [libtar](https://github.com/tklauser/libtar) - TAR file manipulation

## Environment Variables

Environment variables provide a flexible way to configure Hydrogen without modifying its configuration files. Any configuration value can be substituted with an environment variable using the `${env.VARIABLE_NAME}` syntax, making it easy to adapt settings across different environments. The following variables are used in the default configuration:

### Configuration File Location

- `HYDROGEN_CONFIG` - Override the default configuration file location

### Server Settings

- `PAYLOAD_KEY` - Key used for encrypting payload data (see [SECRETS.md](SECRETS.md) for setup instructions)

### Database Connections

For each database (Log, OIDC, Acuranzo, Helium, Canvas), the following variables are used:

- `*_DB_HOST` - Database server hostname
- `*_DB_PORT` - Database server port
- `*_DB_NAME` - Database name
- `*_DB_USER` - Database username
- `*_DB_PASS` - Database password

Example for Log database:

- `LOG_DB_HOST` - Log database server hostname
- `LOG_DB_PORT` - Log database server port
- `LOG_DB_NAME` - Log database name
- `LOG_DB_USER` - Log database username
- `LOG_DB_PASS` - Log database password

### Mail Relay Configuration

For each SMTP server (up to 2 servers supported):

- `SMTP_SERVER1_HOST` - Primary SMTP server hostname
- `SMTP_SERVER1_PORT` - Primary SMTP server port
- `SMTP_SERVER1_USER` - Primary SMTP server username
- `SMTP_SERVER1_PASS` - Primary SMTP server password
- `SMTP_SERVER2_*` - Same variables for secondary SMTP server

### Usage in Configuration Files

To use these variables in the configuration file, use the format `${env.VARIABLE_NAME}`. For example:

```json
{
    "Server": {
        "PayloadKey": "${env.PAYLOAD_KEY}"
    },
    "Databases": {
        "Log": {
            "Host": "${env.LOG_DB_HOST}",
            "Port": "${env.LOG_DB_PORT}"
        }
    }
}
