# Getting Started with Helium

This guide provides a step-by-step introduction to working with the Helium database migration system.

## Environment Setup

### Install Dependencies

```bash
# Ubuntu/Debian
sudo apt-get install lua5.1 luarocks

# macOS with Homebrew
brew install lua luarocks

# Install Lua libraries
luarocks install lua-brotli
```

### Verify Installation

```bash
lua -v  # Should show Lua 5.1+
luarocks list  # Should include lua-brotli
```

## Learning Path

### 1. Learn the Basics

- **[Lua in 15 Minutes](https://tylerneylon.com/a/learn-lua/)** - Quick Lua introduction
- **[Database Migration Concepts](https://en.wikipedia.org/wiki/Schema_migration)** - General migration overview
- **Read our [Lua Introduction](/docs/He/LUA_INTRO.md)** and **[Migrations Introduction](/docs/He/MIGRATIONS_INTRO.md)**

### 2. Explore the Codebase

- **Review the [Migration Creation Guide](/docs/He/GUIDE.md)**
- **Examine simple migrations** like `acuranzo_1024.lua` in `/elements/002-helium/acuranzo/migrations/`
- **Study the [database.lua](/docs/He/DATABASES/database.md) framework**

### 3. Create Your First Migration

- **Follow the patterns in existing migrations**
- **Test on your preferred database engine first**
- **Run `migration_update.sh` to update documentation**

### 4. Advanced Topics

- **Learn about the [macro system](/docs/He/MACRO_REFERENCE.md)**
- **Understand [Brotli compression](/docs/He/BROTLI_COMPRESSION.md) for large content**
- **Explore multi-engine support and testing**

## Quick Start Tutorial

For hands-on learning, follow our **[10-minute Quick Start Tutorial](/docs/He/QUICKSTART.md)** to create, test, and rollback your first migration.

## Development Workflow

### Local Development

1. **Set up your database** (PostgreSQL, MySQL, SQLite, or DB2)
2. **Clone the repository** and navigate to the Helium directory
3. **Create a test migration** following the patterns in existing files
4. **Test locally** using the database.lua processor
5. **Verify the results** by checking table creation and data insertion

### Testing Strategy

1. **Syntax validation** - Ensure Lua code is valid
2. **Macro expansion** - Test that macros resolve correctly
3. **Database testing** - Run migrations against actual databases
4. **Cross-engine validation** - Test on all supported database engines

### Deployment Preparation

1. **Run the full test suite** to ensure compatibility
2. **Document your migrations** with clear names and descriptions
3. **Test rollback scenarios** to ensure reversibility
4. **Update documentation** using the migration_update.sh script

## Common Pitfalls

### Environment Issues

- **Missing dependencies**: Ensure lua-brotli is installed
- **Database not running**: Start your database server before testing
- **Permissions**: Ensure your database user has CREATE/INSERT privileges

### Migration Issues

- **Syntax errors**: Use luacheck to validate Lua syntax
- **Macro problems**: Test macro expansion before full migration runs
- **State conflicts**: Ensure migration numbering is sequential and unique

### Content Issues

- **Large content**: Files >1KB are automatically compressed
- **Special characters**: Use `[=[...]=]` syntax for content with quotes
- **Encoding problems**: Ensure content is valid UTF-8

## Next Steps

Once you're comfortable with the basics:

1. **Read the [Testing Guide](/docs/He/TESTING_GUIDE.md)** for validation techniques
2. **Explore [Migration Anatomy](/docs/He/MIGRATION_ANATOMY.md)** for deep technical understanding
3. **Learn about [Environment Variables](/docs/He/ENVIRONMENT_VARIABLES.md)** for configuration
4. **Study the [Macro Reference](/docs/He/MACRO_REFERENCE.md)** for advanced templating

## Getting Help

- **Check existing documentation** for answers to common questions
- **Review example migrations** in the codebase for patterns
- **Run the test suite** to validate your setup
- **Examine error messages** carefully for debugging information

The Helium system is designed to be approachable for developers familiar with SQL and basic programming concepts. Start with the Quick Start Tutorial and build from there.