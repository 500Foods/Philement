# Hydrogen API Documentation with Swagger (OpenAPI)

This directory contains the OpenAPI/Swagger integration for the Hydrogen project's REST API documentation.

## Overview

The Swagger implementation for Hydrogen uses a source code annotation-based approach to generate comprehensive API documentation directly from the codebase. This ensures documentation stays in sync with implementation and reduces maintenance overhead.

Key features:

- OpenAPI 3.1.0 compatibility
- Source code annotations for documentation
- Automated generation of swagger.json
- Optimized SwaggerUI with Brotli compression for minimal distribution size
- Support for all API endpoints (system and OIDC)
- Path, method, parameter, and response documentation
- Integrated security schemes

## Directory Contents

- `swagger-generate.sh` - Script that processes annotations and generates the OpenAPI specification
- `swagger.json` - Generated OpenAPI 3.1.0 specification
- `swagger.json.br` - Brotli-compressed version of the specification
- `swaggerui-generate.sh` - Script to download and package SwaggerUI for distribution
- `swaggerui.tar.br` - Optimized Brotli-compressed tarball with SwaggerUI for distribution

## SwaggerUI Implementation

The Hydrogen project uses a highly optimized SwaggerUI implementation:

- **SwaggerUI Version**: We use SwaggerUI 5.19.0 with selectively included files
- **Optimized Distribution**: 
  - Static assets (swagger-ui-bundle.js, swagger-ui.css, swagger.json, oauth2-redirect.html) are compressed with Brotli
  - Dynamic files (index.html, swagger-initializer.js) remain uncompressed for runtime modification
  - All packaged in a flat tar structure for efficient serving
- **Size Optimization**: The compressed distribution is ~345KB, significantly smaller than default SwaggerUI
- **OAuth Support**: Includes oauth2-redirect.html for OAuth 2.0 authorization flows
- **SwaggerUI Configuration**: Built-in settings configured for optimal usability (TryItEnabled, AlwaysExpanded)

The `swaggerui-generate.sh` script handles downloading, compressing, and packaging the SwaggerUI distribution. Run this script when you want to update or recreate the SwaggerUI package.

## Annotation System

API endpoints are documented using special comments in the C source code files. These annotations are processed by the `swagger-generate.sh` script to generate the OpenAPI specification.

### Annotation Format

Annotations use the following format:

```c
//@ swagger:<annotation_type> <annotation_value>
```

### Common Annotations

#### Service-level Annotations

```c
//@ swagger:service Service Name
//@ swagger:description Service description goes here
//@ swagger:tag tagname Tag description
```

#### Endpoint Annotations

```c
//@ swagger:path /path/to/endpoint
//@ swagger:method GET
//@ swagger:method POST
//@ swagger:operationId operationName
//@ swagger:tags tag1,tag2
//@ swagger:summary Short summary of what this endpoint does
//@ swagger:description Longer description with more details
```

#### Parameter Annotations

```c
//@ swagger:parameter name type required|optional|conditional Description
```

#### Response Annotations

```c
//@ swagger:response 200 application/json {"type":"object","properties":{...}}
//@ swagger:response 400 application/json {"type":"object","properties":{...}}
```

#### Security Annotations

```c
//@ swagger:security SecurityScheme
```

## Generating Documentation

To generate the OpenAPI specification:

```bash
cd swagger
./swagger-generate.sh
```

This will scan the source code for annotations and produce an updated `swagger.json` file.

## Viewing Documentation

You can view the API documentation using Swagger UI:

1. Generate the required files (if not already done):
   ```bash
   # Generate the OpenAPI specification
   ./swagger-generate.sh
   
   # Package SwaggerUI for distribution
   ./swaggerui-generate.sh
   ```

2. Ensure Swagger is enabled in `hydrogen.json` configuration:
   ```json
   "Swagger": {
     "Enabled": true,
     "Prefix": "/swagger",
     "UIOptions": { ... }
   }
   ```

3. Start the Hydrogen server with appropriate permissions:
   ```bash
   cd ..
   make run
   ```

4. Access SwaggerUI in your browser:
   - Default URL: http://localhost:8080/swagger/
   - The UI will automatically load the API specification
   - Try out endpoints directly from the interface
   - Endpoints requiring authentication will have a padlock icon

5. OAuth2 endpoints can be tested using the included OAuth flows - click "Authorize" to begin authentication

## Adding New Endpoints

When adding new API endpoints, follow these steps:

1. Add Swagger annotations to the header file for your endpoint
2. Follow the annotation patterns established in existing endpoints
3. Run `./swagger-generate.sh` to update the documentation
4. Test the documentation in Swagger UI to ensure it displays correctly

## Known Issues

- Multiple HTTP methods (GET and POST) on a single endpoint may be displayed as "get\npost" instead of two separate method entries
- Some parameters defined in annotations may not appear in the generated documentation
- Security schemes may not be consistently applied across all endpoints

## Example Annotation

Example from `system/health/health.h`:

```c
//@ swagger:service System Health Service
//@ swagger:description Provides system health endpoints for monitoring and diagnostics
//@ swagger:tag system System operations
//@ swagger:tag monitoring Health monitoring endpoints

//@ swagger:path /api/system/health
//@ swagger:method GET
//@ swagger:operationId getSystemHealth
//@ swagger:tags system,monitoring
//@ swagger:summary Health check endpoint
//@ swagger:description Returns a simple health check response indicating the service is alive. Used primarily by load balancers for health monitoring in distributed deployments.
//@ swagger:response 200 application/json {"type":"object","properties":{"status":{"type":"string","example":"Yes, I'm alive, thanks!"}}}
//@ swagger:response 500 application/json {"type":"object","properties":{"error":{"type":"string","example":"Failed to create response"}}}
enum MHD_Result handle_system_health_request(struct MHD_Connection *connection);