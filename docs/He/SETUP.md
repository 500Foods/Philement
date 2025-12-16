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

# Install Lua 5.1 (recommended version)
sudo apt-get install lua5.1

# Install luarocks (Lua package manager)
sudo apt-get install luarocks

# Verify installation
lua5.1 -v  # Should show Lua 5.1.x
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
# Download Lua 5.1 source
curl -L -o lua.tar.gz http://www.lua.org/ftp/lua-5.1.5.tar.gz
tar -xzf lua.tar.gz
cd lua-5.1.5

# Build and install
make linux
sudo make install

# Install luarocks
wget https://luarocks.org/releases/luarocks-3.9.2.tar.gz
tar -xzf luarocks-3.9.2.tar.gz
cd luarocks-3.9.2
./configure
make
sudo make install
```

## Installing Lua Libraries

### lua-brotli (Required for compression)

```bash
# Install lua-brotli library
luarocks install lua-brotli

# Verify installation
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