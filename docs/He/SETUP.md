# Environment Setup Guide

This guide provides detailed instructions for setting up your development environment to work with Helium migrations.

## Prerequisites

Before setting up Helium, ensure your system meets these requirements:

- **Operating System**: Linux, macOS, or Windows (with WSL)
- **Shell**: Bash-compatible shell (bash, zsh, etc.)
- **Disk Space**: At least 100MB free space
- **Internet Connection**: Required for downloading dependencies

## Installing Lua

### Ubuntu/Debian

```bash
# Update package list
sudo apt-get update

# Runtime for migrations is Hydrogen's embedded Lua (currently 5.5).
# For local syntax checks, install a matching CLI when possible:
#   Fedora 43: system packages may still be 5.4; build 5.5 into /usr/local
#   or use the same lua binary Hydrogen's pkg-config finds.
# Authoring tip: migration Lua is plain 5.x; avoid mutating for-loop control
# variables (read-only in Lua 5.5).

# Example (Debian/Ubuntu older docs used 5.1 — prefer 5.4+ CLI):
# sudo apt-get install lua5.4 luarocks

# Verify installation
lua -v  # Prefer 5.5.x when matching Hydrogen embed
luarocks --version  # Should show luarocks version
```

### CentOS/RHEL/Fedora

```bash
# CentOS/RHEL
sudo yum install lua lua-devel
# or for newer versions
sudo dnf install lua lua-devel

# Install luarocks
sudo yum install luarocks
# or
sudo dnf install luarocks

# Verify
lua -v
luarocks --version
```

### macOS

```bash
# Using Homebrew (recommended)
brew install lua
brew install luarocks

# Verify
lua -v
luarocks --version
```

### Windows

```powershell
# Using Chocolatey
choco install lua

# Or download from https://luabinaries.sourceforge.net/
# Extract to a folder and add to PATH
```

### Manual Installation

If your package manager doesn't have the required versions:

```bash
# Prefer Lua 5.5 to match Hydrogen embed (example: 5.5.1)
curl -L -o lua.tar.gz https://www.lua.org/ftp/lua-5.5.1.tar.gz
tar -xzf lua.tar.gz
cd lua-5.5.1
make linux
sudo make install INSTALL_TOP=/usr/local

# pkg-config helper (Hydrogen uses pkg_check_modules(LUA ...))
sudo tee /usr/local/lib/pkgconfig/lua.pc >/dev/null <<'EOF'
V= 5.5
R= 5.5.1
prefix=/usr/local
libdir=${prefix}/lib
includedir=${prefix}/include
Name: Lua
Version: ${R}
Libs: -L${libdir} -llua -lm -ldl
Cflags: -I${includedir}
EOF
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH
```

## Installing Lua Libraries

### lua-brotli (Required for compression)

Hydrogen migrations `require("brotli")` from the **embedded** Lua state.
With a **static** `liblua.a`, the hydrogen binary must export Lua symbols
(`-rdynamic` on the link line) and `brotli.so` must be on `package.cpath`
(e.g. `/usr/local/lib/lua/5.5/brotli.so` or `./brotli.so` next to the cwd).

```bash
# luarocks may not detect Lua 5.5 headers yet; build the rock by hand:
#   luarocks unpack lua-brotli && cd lua-brotli-*/lua-brotli
#   make LUA_INCDIR=/usr/local/include LUA_LIBDIR=/usr/local/lib
#   sudo cp brotli.so /usr/local/lib/lua/5.5/

# Verify with the same lua as the embed
lua -e "require 'brotli'"  # Should not error
```

If installation fails, you may need development libraries:

```bash
# Ubuntu/Debian
sudo apt-get install libbrotli-dev

# CentOS/RHEL
sudo yum install brotli-devel

# macOS
brew install brotli

# Then retry
luarocks install lua-brotli
```

## Setting Up the Project

### Clone the Repository

```bash
# Clone the Philement repository
git clone https://github.com/your-org/philement.git
cd philement

# Navigate to Helium
cd elements/002-helium
```

### Verify Project Structure

```bash
# Check that migrations exist
ls acuranzo/migrations/ | head -5

# Check that scripts are executable
ls -la scripts/
```

## Database Setup (Optional for Development)

While you can develop migrations without a database, testing requires one:

### PostgreSQL (Recommended)

```bash
# Ubuntu/Debian
sudo apt-get install postgresql postgresql-contrib

# Create test database
sudo -u postgres createdb helium_test
sudo -u postgres createuser --createdb helium_user
```

### MySQL/MariaDB

```bash
# Ubuntu/Debian
sudo apt-get install mysql-server

# Secure installation
sudo mysql_secure_installation

# Create test database
mysql -u root -p
CREATE DATABASE helium_test;
CREATE USER 'helium_user'@'localhost' IDENTIFIED BY 'password';
GRANT ALL PRIVILEGES ON helium_test.* TO 'helium_user'@'localhost';
```

### SQLite

SQLite usually comes pre-installed on most systems. If not:

```bash
# Ubuntu/Debian
sudo apt-get install sqlite3

# Verify
sqlite3 --version
```

## Testing Your Setup

### Basic Lua Test

```bash
# Create a test file
cat > test_lua.lua << 'EOF'
print("Lua is working!")
local brotli = require("brotli")
print("Brotli library loaded successfully")
EOF

# Run it
lua test_lua.lua

# Clean up
rm test_lua.lua
```

### Migration Processing Test

```bash
# Try processing a simple migration
cd elements/002-helium
lua acuranzo/migrations/database.lua \
  postgresql \
  acuranzo \
  public \
  < acuranzo/migrations/acuranzo_1024.lua
```

## Troubleshooting

### Common Issues

#### "lua: command not found"

- Ensure Lua is installed and in your PATH
- Try `lua5.1` instead of `lua`
- Check installation: `which lua`

#### "luarocks: command not found"

- Install luarocks if missing
- Add luarocks to PATH: `export PATH=$PATH:$(luarocks path --bin)`

#### "lua-brotli installation fails"

- Install system brotli development libraries first
- Check if you have sufficient permissions
- Try installing as root: `sudo luarocks install lua-brotli`

#### "Permission denied" on scripts

```bash
# Make scripts executable
chmod +x elements/002-helium/scripts/*.sh
```

#### Database connection issues

- Verify database is running: `sudo systemctl status postgresql`
- Check credentials in connection strings
- Ensure user has proper permissions

### Getting Help

If you encounter issues:

1. Check the [main README](/docs/He/README.md) for prerequisites
2. Review error messages carefully
3. Search existing issues in the project repository
4. Ask in project discussions or create an issue

## Next Steps

Once your environment is set up:

1. Read the [Migration Creation Guide](/docs/He/GUIDE.md)
2. Study existing migrations in `acuranzo/migrations/`
3. Try creating a simple test migration
4. Run the test suite to verify everything works

Happy coding!