# Environment Variables in Migrations

This document lists environment variables that can be used in Helium migrations for runtime configuration and sensitive data injection.

## How Environment Variables Work

Environment variables are resolved at migration execution time by the Hydrogen server system. Use the `${VARIABLE_NAME}` syntax in your migration SQL, and the system will substitute the actual environment value.

## Known Environment Variables

### Authentication and User Data

| Variable | Description | Example Usage |
|----------|-------------|---------------|
| `HYDROGEN_USERNAME` | Admin account username | `acuranzo_1024.lua` - Default admin account creation |
| `HYDROGEN_PASSHASH` | SHA256 password hash for admin account | `acuranzo_1024.lua` - Admin password hash |

### Database Configuration

| Variable | Description | Example Usage |
|----------|-------------|---------------|
| `DB_HOST` | Database server hostname | Custom migrations requiring specific connections |
| `DB_PORT` | Database server port | Custom migrations requiring specific connections |
| `DB_NAME` | Database name | Schema-specific configurations |

### Deployment Configuration

| Variable | Description | Example Usage |
|----------|-------------|---------------|
| `ENVIRONMENT` | Deployment environment (dev/staging/prod) | Conditional logic in migrations |
| `REGION` | Geographic region | Region-specific data seeding |
| `INSTANCE_ID` | Server instance identifier | Instance-specific configurations |

## Using Custom Environment Variables

You can define and use any environment variable in your migrations:

```lua
-- In migration
INSERT INTO config (key, value) VALUES ('environment', '${ENVIRONMENT}');
INSERT INTO config (key, value) VALUES ('region', '${REGION}');

-- Set in deployment
export ENVIRONMENT="production"
export REGION="us-west-2"
```

## Security Considerations

- **Never hardcode sensitive data** in migration files
- **Use environment variables** for passwords, keys, and secrets
- **Validate variable presence** in your deployment scripts
- **Document required variables** in your deployment documentation

## Error Handling

If a referenced environment variable is not set, the migration will fail with an undefined variable error. Ensure all required variables are set before running migrations.

## Best Practices

1. **Document required variables** in your migration comments
2. **Provide default values** when possible using shell parameter expansion
3. **Use descriptive variable names** that indicate their purpose
4. **Group related variables** with common prefixes (e.g., `DB_*`, `APP_*`)
5. **Test with different environments** to ensure variables are available

## Setting Variables in Different Environments

### Development

```bash
export HYDROGEN_USERNAME="admin"
export HYDROGEN_PASSHASH="5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8"
export ENVIRONMENT="development"
```

### Production

```bash
export HYDROGEN_USERNAME="prod_admin"
export HYDROGEN_PASSHASH="$(echo -n 'secure_password' | sha256sum | cut -d' ' -f1)"
export ENVIRONMENT="production"
```

### Docker

```yaml
environment:
  - HYDROGEN_USERNAME=admin
  - HYDROGEN_PASSHASH=5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8
  - ENVIRONMENT=production
```

Environment variables provide a secure, flexible way to inject runtime configuration into your migrations without compromising sensitive data or deployment portability.