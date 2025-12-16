# Supported Databases

The Helium project supports multiple database engines, allowing you to deploy the same schema and migrations across different database platforms.

## Primary Databases

### PostgreSQL v16/v17

- **Status**: Primary development platform
- **Features**: Full JSON support, advanced indexing, excellent concurrency
- **Use Cases**: Production deployments, complex queries, high concurrency
- **Setup**: Native support with no additional extensions required

### MySQL/MariaDB

- **Status**: Widely tested alternative
- **Features**: Broad compatibility, extensive tooling ecosystem
- **Use Cases**: Legacy systems, web applications, cost-effective deployments
- **Setup**: Requires FROM_BASE64() function for content decoding

## Embedded Databases

### SQLite v3

- **Status**: Embedded applications
- **Features**: Zero-configuration, single-file database, ACID compliance
- **Use Cases**: Desktop applications, mobile apps, development/testing
- **Setup**: Requires sqlean extension for crypto and advanced functions

## Enterprise Databases

### IBM DB2 v10-12

- **Status**: Enterprise deployments
- **Features**: Advanced security, high availability, enterprise features
- **Use Cases**: Large-scale enterprise applications, regulated industries
- **Setup**: Requires custom UDFs for Base64/Brotli processing

## Database-Specific Considerations

### Data Types

Each database has different implementations of common data types:

| Concept | PostgreSQL | MySQL | SQLite | DB2 |
|---------|------------|-------|--------|-----|
| Auto-increment | `SERIAL` | `AUTO_INCREMENT` | `AUTOINCREMENT` | `GENERATED ALWAYS AS IDENTITY` |
| Large text | `TEXT` | `TEXT` | `TEXT` | `CLOB(1M)` |
| JSON | `JSONB` | `LONGTEXT` | `TEXT` | `CLOB(1M)` |
| Timestamps | `TIMESTAMPTZ` | `TIMESTAMP` | `TEXT` | `TIMESTAMP` |

### Encoding and Compression

- **Base64 Decoding**: All databases support, but with different function names
- **Brotli Compression**: Supported via UDFs on PostgreSQL, MySQL, DB2; SQLite uses extension
- **Large Content**: Automatically compressed when >1KB to reduce storage

### Performance Characteristics

- **PostgreSQL**: Best for complex queries and high concurrency
- **MySQL**: Excellent for simple CRUD operations and web applications
- **SQLite**: Fast for single-user applications, limited concurrency
- **DB2**: Optimized for enterprise workloads with advanced optimization

## Migration Compatibility

All migrations are designed to work across all supported databases through the macro system. The same migration file produces database-appropriate SQL for each engine without modification.

## Testing Requirements

For comprehensive testing, set up all supported databases in your CI/CD pipeline:

```bash
# PostgreSQL
docker run -d --name postgres -e POSTGRES_PASSWORD=test postgres:16

# MySQL
docker run -d --name mysql -e MYSQL_ROOT_PASSWORD=test mysql:8

# SQLite (no setup required)
# DB2 (requires license)
```

## Choosing a Database

### For Development

- **PostgreSQL**: Most feature-complete, best represents production
- **SQLite**: Quick setup, good for prototyping

### For Production

- **PostgreSQL**: Complex applications, high concurrency
- **MySQL**: Web applications, cost-sensitive deployments
- **DB2**: Enterprise requirements, regulatory compliance

### For Embedded/Mobile

- **SQLite**: Self-contained, no server required

Each database is fully supported with identical schema structures and migration capabilities.