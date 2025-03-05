# Hydrogen Encrypted Payload System

This directory contains the encrypted payload system for the Hydrogen project, which currently includes the OpenAPI/Swagger integration for REST API documentation, with provisions for future expansion to other payloads.

## Overview

The payload system for Hydrogen provides a secure way to embed various assets directly into the executable. The current implementation includes SwaggerUI for API documentation, with a robust hybrid encryption system and optimized compression.

Key features:

- **General-purpose payload system** - Designed to handle multiple payload types
- **RSA+AES hybrid encryption** - Secure key exchange with efficient payload encryption
- **Environment variable-based key management** - Flexible and secure key distribution
- **OpenAPI 3.1.0 compatibility** - Full support for latest OpenAPI specification
- **Optimized with Brotli compression** - High compression ratio for static assets
- **Dynamic content adaptation** - Server URL and configuration auto-adjustment

## Directory Contents

- `swagger-generate.sh` - Script that processes annotations and generates the OpenAPI specification
- `swagger.json` - Generated OpenAPI 3.1.0 specification
- `payload-generate.sh` - Script to download, package, and encrypt payloads for distribution
- `payload.tar.br` - Encrypted, Brotli-compressed tarball containing payload files

## Payload Encryption System

The Hydrogen project uses a hybrid RSA+AES encryption approach for payloads:

1. **Key Management**:
   - `PAYLOAD_LOCK` environment variable: Contains the RSA public key used for encryption
   - `PAYLOAD_KEY` environment variable: Contains the RSA private key used for decryption

2. **Encryption Process**:
   - Generates a random AES-256 key for payload encryption
   - Encrypts the AES key with the RSA public key (PAYLOAD_LOCK)
   - Encrypts the payload with the AES key
   - Combines the encrypted AES key and payload into a single file

3. **Decryption Process**:
   - Extracts the encrypted AES key from the payload file
   - Decrypts the AES key using the RSA private key (PAYLOAD_KEY)
   - Uses the decrypted AES key to decrypt the payload
   - Processes the decrypted payload as normal

4. **File Format**:
   ```
   [key_size(4 bytes)] + [encrypted_aes_key] + [iv(16 bytes)] + [encrypted_payload]
   ```
   - key_size: 4-byte header indicating encrypted AES key length
   - encrypted_aes_key: RSA-encrypted AES-256 key
   - iv: 16-byte initialization vector for AES-CBC
   - encrypted_payload: AES-encrypted, Brotli-compressed tar file

## SwaggerUI Implementation

The primary payload currently embedded is a highly optimized SwaggerUI implementation:

- **SwaggerUI Version**: SwaggerUI 5.19.0 with selective file inclusion
- **Optimized Distribution**: 
  - Static assets are Brotli-compressed:
    - swagger-ui-bundle.js
    - swagger-ui-standalone-preset.js
    - swagger-ui.css
    - oauth2-redirect.html
  - Dynamic files remain uncompressed for runtime modification:
    - index.html
    - swagger-initializer.js
    - favicon-32x32.png
    - favicon-16x16.png
  - All packaged in a flat tar structure for efficient serving
- **Dynamic Server URL**: Automatically adapts to the current server host and protocol
- **OAuth Support**: Full OAuth 2.0 authorization flow support
- **SwaggerUI Configuration**:
  - TryItOut enabled by default
  - Operation IDs displayed
  - Optimized model expansion depth
  - List-style documentation expansion

The `payload-generate.sh` script handles downloading, compressing, encrypting, and packaging the payload distribution. Run this script when you want to update or recreate the payload package.

## API Documentation Annotation System

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
cd payload
./swagger-generate.sh
```

This will scan the source code for annotations and produce an updated `swagger.json` file.

## Building the Encrypted Payload

To build the encrypted payload:

```bash
cd payload
export PAYLOAD_LOCK="base64-encoded-rsa-public-key"
./payload-generate.sh
```

If `PAYLOAD_LOCK` is not set, the script will generate a test key pair for development purposes only.

## Viewing Documentation

You can view the API documentation using Swagger UI:

1. Generate the required files (if not already done):
   ```bash
   # Generate the OpenAPI specification
   ./swagger-generate.sh
   
   # Package and encrypt the payload
   ./payload-generate.sh
   ```

2. Ensure Swagger is enabled in `hydrogen.json` configuration:
   ```json
   "Swagger": {
     "Enabled": true,
     "Prefix": "/swagger",
     "UIOptions": { ... }
   },
   "PayloadKey": "${env.PAYLOAD_KEY}"
   ```

3. Set the PAYLOAD_KEY environment variable for decryption:
   ```bash
   export PAYLOAD_KEY="base64-encoded-rsa-private-key"
   ```

4. Start the Hydrogen server:
   ```bash
   cd ..
   make run
   ```

5. Access SwaggerUI in your browser:
   - Default URL: http://localhost:8080/swagger/
   - The UI will automatically load the API specification
   - Try out endpoints directly from the interface

## Adding New Endpoints

When adding new API endpoints, follow these steps:

1. Add Swagger annotations to the header file for your endpoint
2. Follow the annotation patterns established in existing endpoints
3. Run `./swagger-generate.sh` to update the documentation
4. Run `./payload-generate.sh` to rebuild the encrypted payload
5. Test the documentation in Swagger UI to ensure it displays correctly

## Adding New Payload Types

To add new payload types in the future:

1. Add your files to the payload directory structure
2. Update the payload-generate.sh script if necessary to include these files
3. Rebuild the encrypted payload using the script
4. Update the web_server_swagger.c file if necessary to handle the new payload types

## Security Considerations

- Always keep the PAYLOAD_KEY secure and never commit it to version control
- Use environment variables or secure secrets management for production deployments
- The PAYLOAD_LOCK can be shared more widely since it's only used for encryption
- Periodically rotate encryption keys for enhanced security
- Test both encryption and decryption processes when making changes to the payload system
- For detailed information about secrets management using environment variables, see [SECRETS.md](../SECRETS.md)