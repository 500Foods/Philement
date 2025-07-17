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

## Payload Mechanism

The basic payload mechanism is trivial. A payload file is created, generally with suitable encryption. This file is simply appended to the binary executable directly, and an extra
marker is added, <<< HERE BE ME TREASURE >>>, after the payload, to make it easier to identify
that a payload has been attached.  Finally, a separate value is appended indicating the offset
of the payload so the program can properly locate and decode it.

Of special note, when using the release build, it is compressed with UPX and stripped of symbols. So strings like the payload marker appear exactly once, where one would expect the payload marker to appear. The coverage build, on the other hand, is neither compressed nor stripped of its debug symbols, leading to the revelation that the payload marker appears numerous times in the file, and thus we have to be mindful to examine the *last* payload marker when using it as designed.

## Payload Contents

- `swagger-generate.sh` - Script that processes annotations and generates the OpenAPI specification
- `swagger.json` - Generated OpenAPI 3.1.0 specification
- `payload-generate.sh` - Script to download, package, and encrypt payloads for distribution
- `payload.tar.br` - Encrypted, Brotli-compressed tarball containing payload files

## Payload Encryption System

The Hydrogen project uses a hybrid RSA+AES encryption approach for payloads:

1. **Key Management**:
   - `PAYLOAD_LOCK` environment variable: Contains the RSA public key used for encryption
   - `PAYLOAD_KEY` environment variable: Contains the RSA private key used for decryption
   - Use `test_env_payload.sh` to validate environment variables:

     ```bash
     # Verify environment variables are set correctly
     ../tests/test_env_payload.sh
     ```

     This test ensures both variables are present and contain valid RSA keys

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

   ```diagram
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
- **SwaggerUI Configuration**: All UI options can be configured in `hydrogen.json`:

  ```json
  "Swagger": {
    "Enabled": true,
    "Prefix": "/swagger",
    "UIOptions": {
      "tryItEnabled": true,        // Enable/disable Try It Out by default
      "displayOperationId": false, // Show/hide operation IDs
      "defaultModelsExpandDepth": 1, // How deep to expand the models section
      "defaultModelExpandDepth": 1,  // How deep to expand individual models
      "showExtensions": false,     // Show/hide vendor extensions
      "showCommonExtensions": true, // Show/hide common extensions
      "docExpansion": "list",     // "list", "full", or "none"
      "syntaxHighlightTheme": "agate" // Code syntax highlighting theme
    }
  }
  ```

  Each option controls a specific aspect of the UI:
  - `tryItEnabled`: When true, enables the "Try It Out" feature by default
  - `displayOperationId`: When false, hides operation IDs from the UI
  - `defaultModelsExpandDepth`: Controls initial expansion depth of the models section
  - `defaultModelExpandDepth`: Controls initial expansion depth of individual models
  - `showExtensions`: Shows vendor-specific OpenAPI extensions when true
  - `showCommonExtensions`: Shows commonly used OpenAPI extensions when true
  - `docExpansion`: Controls initial expansion of operations ("list", "full", "none")
  - `syntaxHighlightTheme`: Theme for code samples and responses

The `payload-generate.sh` script handles downloading, compressing, encrypting, and packaging the payload distribution. Run this script when you want to update or recreate the payload package.

## API Documentation Annotation System

API endpoints are documented using special comments in the C source code files. These annotations are processed by the `swagger-generate.sh` script to generate the OpenAPI specification.

### Annotation Format

Annotations use the following format:

```c
//@ swagger:<annotation_type> <annotation_value>
```

### Common Annotations

#### Service-level Annotations (in service header files)

```c
//@ swagger:tag "System Service" Provides system-level operations and monitoring
```

#### Endpoint Annotations (in endpoint header files)

```c
//@ swagger:path /system/info
//@ swagger:method GET
//@ swagger:summary System information endpoint
//@ swagger:description Returns comprehensive system information
//@ swagger:response 200 application/json {"type":"object","properties":{"status":{"type":"string"}}}
```

The `operationId` field is optional and can be added if needed:

```c
//@ swagger:operationId getSystemInfo
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
cd payloads
./swagger-generate.sh
```

This will scan the source code for annotations and produce an updated `swagger.json` file.

## Building the Encrypted Payload

To build the encrypted payload:

```bash
cd payloads
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
   
   # Package and encrypt the payloads
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
   - Default URL: <http://localhost:8080/swagger/>
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
