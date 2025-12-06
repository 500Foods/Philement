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
- [cppcheck](https://cppcheck.sourceforge.io/) - Static analysis for C/C++ code
- [shellcheck](https://www.shellcheck.net/) - Static linting for bash
- [markdownlint](https://github.com/DavidAnson/markdownlint) - Markdown file linting and style checking
- [eslint](https://eslint.org/) - JavaScript code linting and style checking
- [stylelint](https://stylelint.io/) - CSS/SCSS linting and style checking
- [htmlhint](https://htmlhint.com/) - HTML code linting and validation
- [xmlstarlet](https://xmlstar.sourceforge.net/) - XML/SVG validation
- [jsonlint](https://github.com/zaach/jsonlint) - JSON file validation and formatting
- [websocat](https://github.com/vi/websocat) - Testing websocket connections
- [cmake](https://cmake.org/) - Build system
- [ninja](https://ninja-build.org/) - Faster build system
- [unity](https://www.throwtheswitch.org/unity) - Unit test framework for C
- [swagge-cli](https://github.com/APIDevTools/swagger-cli) - Validate generated swagger.json

## Build-time Related Projects

A handful of other projects are directly included in the tests/lib folder of this project.

- [GitHub Sitmemap](https://github.com/500Foods/Scripts/github-sitemap) - Cross-reference all the markdown in a repository [lib/github-sitemap.sh](../tests/lib/github-sitemap.sh)) ([Docs](../tests/docs/github-sitemap.md))

## Example Ubuntu Build Environment

```bash
sudo apt update
sudo apt install -y build-essential wget curl jq nodejs npm cloc ninja-build upx-ucl brotli
sudo apt install -y libjansson-dev libmicrohttpd-dev libssl-dev libwebsockets-dev libbrotli-dev libcurl-openssl-dev
sudo apt install -y valgrind cppcheck libxml2-utils
npm install -g markdownlint jsonlint stylelint htmlhint swagger-cli
```

NOTE: Latest NodeJS should be used (eg. Node 24)

With that all installed, perform the following steps.

- Generate PAYLOAD_LOCK and PAYLOAD_KEY (see [SECRETS.md](SECRETS.md) for setup instructions)
- Run `swagger_generate.sh` and `payroll_generate.sh` in the payloads directory
- Run `tests_00_all.sh` in the `tests` folder to build and test the entire project

## Example macOS Build Environment

- Install homebrew and OhMyZsh
- `brew install bash`
- `brew install gcc`
- `brew install upx`
- `brew install jq`
- `brew install coreutils findutils cmake ccache stylelint jsonlint`
- `brew install node`
- `brew install jansson libxml2`
- `npm install -g swagger-cli eslint htmlhint`
- `brew install markdownlint-cli`
- Add to ~/.zshrc, set OhMyZsh to auto-update while you're at it

``` ~/.zshrc
export PATH=/usr/local/bin:$PATH
export PATH="$PATH:/Applications/Visual Studio Code.app/Contents/Resources/app/bin"
```

- `sudo codesign --force --deep --sign - "/Applications/Visual Studio Code.app"` - this re-signs VSC so that you can run `code <filename>` and have it open without issuing error messages

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

### SHELLCHECK

This is what is being used for shellcheck during development. A little harsher than it needs to be but we're angling for long-term stability, so perhaps not a waste of time.
Basically, this enables all the checks possible, and displays the output in a far more concise one-line-per-issue format.  

The `--external-sources` option is a bit tricky as
this primarily trips over the test scripts, given how they're called from an orchestration script (Test 00). This means we have to tell shellcheck where the files actually are.

Note that the test suite has a shellcheck-specific test, Test 92, which uses the same values but sets up a caching mechanism so that if the scripts themselves aren't changing,
then the shellcheck validation doesn't slow us down.

Consider adding this to ~/.bashrc or ~/.zshrc as needed.

```bash
export SHELLCHECK_OPTS=--enable=all --severity=style --external-sources --format=gcc
```

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
```

## GitHub Codespaces

Should you want to build the project in a GitHub codespace, the following can be used. Place these in the `.devcontainer` folder of your repo. Once the repo starts you can then run the usual sorts of commands.

- `git clone https://github.com/500Foods/Philement`
- `cd Philement/elements/001-hydrogen/hydrogen/tests`
- `./test_00_all.sh`

The hydrogen build should proceed, downloading unity if needed and running the payload build as well, so long as you've defined PAYLOAD_LOCK, PAYLOAD_KEY and WEBSOCKET_KEY as GitHub secrets in your repo.  See [SECRETS](SECRETS.md) for more information on those environment variables.

### dockerfile

```dockerfile
FROM ubuntu:24.04

# Install base tools
RUN apt-get update && apt-get install -y \
    git \
    jq \
    curl \
    gpg \
    && rm -rf /var/lib/apt/lists/*
```

### devcontainer.json

```devcontainer.json
{
  "name": "Codespace",
  "build": {
    "dockerfile": "Dockerfile",
    "context": "."
  },
  "features": {
    "ghcr.io/devcontainers/features/docker-in-docker:2": {
      "version": "latest",
      "enableNonRootDocker": "true",
      "moby": "true"
    }
  },
  "customizations": {
    "vscode": {
      "settings": {},
      "extensions": [
        "ms-kubernetes-tools.vscode-kubernetes-tools",
        "github.vscode-pull-request-github",
      ]
    }
  },
  "containerEnv": {
  },
  "postCreateCommand": "/bin/bash .devcontainer/setup.sh 2>&1 | tee /tmp/post-create.log && cat /tmp/post-create.log",
  "postStartCommand": "zsh -c 'source ~/.zshrc && echo \"zshrc sourced\"'"
}
```

### setup.sh

```setup.sh
#!/bin/bash
set -x  # Print commands as they run

apt install -y locales
locale-gen en_US.UTF-8
export LANG=en_US.UTF-8
export LC_ALL=en_US.UTF-8

# Update and install base packages
apt-get update
DEBIAN_FRONTEND=noninteractive apt-get install -y jq tzdata apt-transport-https ca-certificates curl wget tar gnupg lsb-release

# Set timezone
ln -fs /usr/share/zoneinfo/America/Vancouver /etc/localtime
dpkg-reconfigure -f noninteractive tzdata

# Install Docker
mkdir -p /etc/apt/keyrings
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | gpg --dearmor -o /etc/apt/keyrings/docker.gpg
echo "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.gpg] https://download.docker.com/linux/ubuntu $(lsb_release -cs) stable" > /etc/apt/sources.list.d/docker.list

apt-get update
DEBIAN_FRONTEND=noninteractive apt-get install -y kubectl docker-ce docker-ce-cli containerd.io docker-compose-plugin

# Configure Docker for non-root user
groupadd docker || true
usermod -aG docker $(whoami) || true

# Remove zsh profile to avoid interactive prompt
rm -f /etc/zsh/zprofile

# Install additional apps from apps.json if it exists
if [ -f .devcontainer/apps.json ]; then
  for app in $(jq -r ".apps[]" .devcontainer/apps.json); do
    DEBIAN_FRONTEND=noninteractive apt-get install -y "$app"
  done
else
  echo "Warning: apps.json not found" >&2
fi

# Install Oh My Zsh
yes | sh -c "$(curl -fsSL https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh)" --unattended

# Set up environment variables and files *after* Oh My Zsh
echo "source /workspaces/Festival/.devcontainer/.zshrc" >> ~/.zshrc
echo "export LANG=en_US.UTF-8" >> ~/.zshrc
echo "export LC_ALL=en_US.UTF-8" >> ~/.zshrc

# Deal with shellcheck
export SHELLCHECK_OPTS=--enable=all --severity=style --external-sources --format=gcc
echo "export SHELLCHECK_OPTS=--enable=all --severity=style --external-sources --format=gcc" > ~/.zshrc

# Set Zsh as default shell (with fallback)
DEBIAN_FRONTEND=noninteractive chsh -s /bin/zsh
echo "if [ -t 1 ] && [ -n \"$(command -v zsh)\" ]; then exec zsh; fi" >> ~/.bashrc

# Install websocat,  used by test 36 for websocket testing
wget -qO /usr/local/bin/websocat https://github.com/vi/websocat/releases/latest/download/websocat.x86_64-unknown-linux-musl
chmod +x /usr/local/bin/websocat

echo "Setup complete" >&2
```

### apps.json

```apps.json
{
  "apps": [
    "gawk",
    "gh",
    "lsof",
    "golang",
    "net-tools",
    "netcat-openbsd",
    "vim",
    "tcpdump",
    "iputils-ping",
    "traceroute",
    "wget",
    "ftp",
    "telnet",
    "kubectl",
    "helm",
    "ceph-common",
    "htop",
    "less",
    "dnsutils",
    "apache2-utils",
    "zsh",
    "iproute2",
    "build-essential",
    "wget",
    "curl", 
    "jq",
    "nodejs",
    "npm",
    "cloc",
    "cmake",
    "ninja-build",
    "upx-ucl",
    "brotli",
    "libjansson-dev",
    "libmicrohttpd-dev",
    "libssl-dev",
    "libwebsockets-dev",
    "libbrotli-dev",
    "libcurl4-openssl-dev",
    "libtar-dev",
    "valgrind",
    "cppcheck",
    "eslint",
    "shellcheck",
    "bc",
    "markdownlint",
    "systemd-coredump",
    "build-essential",
    "gdb",
    "lldb",
    "cmake",
    "clang",
    "libc6-dbg",
    "libstdc++6-13-dbg",
    "linux-headers-$(uname -r)",
    "apport"
  ]
}
```

### .zshrc

```.zshrc
# Custom .zshrc for devcontainer
echo "Loaded devcontainer .zshrc"

# Why give Microsft more money than we need to? 
# Shutdown the codespace rather than waiting 20m for the automatic timeout
# They sure don't make it easy, though, right? Says something, but we already knew that
alias bye='cd $CODESPACE_VSCODE_FOLDER && gh codespace stop -c $(gh api /user/codespaces --jq '\''.codespaces[] | select(.state == "Available") | .name'\'' | head -n 1)'
```
