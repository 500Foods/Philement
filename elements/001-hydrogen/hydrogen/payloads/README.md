# Hydrogen Encrypted Payload System

## Overview

Hydrogen uses an encrypted payload system to securely embed assets directly into executables. The system supports multiple payload types with RSA+AES hybrid encryption, environment-based key management, and Brotli compression.

**Current Payloads:**

- **SwaggerUI**: OpenAPI 3.1.0 documentation interface
- **Terminal**: xterm.js-based web terminal interface
- **Database Migrations**: Helium project database schema files

**Key Features:**

- Hybrid RSA+AES encryption with environment variable key management
- Brotli compression for optimal size
- Dynamic content adaptation for server URLs and configuration
- Extensible architecture for future payload types

## Payload Mechanism

The basic payload mechanism is trivial. A payload file is created, generally with suitable encryption. This file is simply appended to the binary executable directly, and an extra
marker is added, <<< HERE BE ME TREASURE >>>, after the payload, to make it easier to identify
that a payload has been attached.  Finally, a separate value is appended indicating the offset
of the payload so the program can properly locate and decode it.

Of special note, when using the release build, it is compressed with UPX and stripped of symbols. So strings like the payload marker appear exactly once, where one would expect the payload marker to appear. The coverage build, on the other hand, is neither compressed nor stripped of its debug symbols, leading to the revelation that the payload marker appears numerous times in the file, and thus we have to be mindful to examine the *last* payload marker when using it as designed.

## Payload Scripts

| Script | Purpose |
|--------|---------|
| `swagger-generate.sh` | Scans C source code for annotations and generates OpenAPI 3.1.0 specification |
| `terminal-generate.sh` | Downloads xterm.js and creates web terminal interface assets |
| `payload-generate.sh` | Downloads dependencies, compresses assets, packages payloads, and creates encrypted tarball |

## Generated Files

| File | Description |
|------|-------------|
| `swagger.json` | Generated OpenAPI 3.1.0 specification |
| `payload.tar.br.enc` | Encrypted, Brotli-compressed payload tarball |
| `xtermjs/` | Temporary directory with xterm.js assets (cleaned up after packaging) |
| `swaggerui/` | Temporary directory with SwaggerUI assets (cleaned up after packaging) |

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

## SwaggerUI Payload

The SwaggerUI payload provides an interactive API documentation interface:

**Features:**

- Latest SwaggerUI version with selective file inclusion
- Brotli compression for static assets (JS/CSS/HTML)
- Dynamic server URL adaptation
- OAuth 2.0 authorization support
- Configurable UI options in `hydrogen.json`

**Configuration Example:**

```json
"Swagger": {
  "Enabled": true,
  "Prefix": "/swagger",
  "UIOptions": {
    "tryItEnabled": true,
    "docExpansion": "list",
    "syntaxHighlightTheme": "agate"
  }
}
```

**Key UI Options:**

- `tryItEnabled`: Enable "Try It Out" by default
- `docExpansion`: Initial expansion mode ("list", "full", "none")
- `syntaxHighlightTheme`: Code highlighting theme

## Terminal Payload

The terminal payload provides a web-based terminal interface using xterm.js:

**Features:**

- Latest xterm.js with fit and attach addons
- WebSocket-based communication with Hydrogen server
- VS Code Dark theme with custom styling
- Automatic terminal resizing and reconnection
- Secure connection with configurable WebSocket key

**Components:**

- `xterm.js`: Core terminal emulation library
- `xterm-addon-fit.js`: Automatic terminal sizing
- `xterm-addon-attach.js`: WebSocket data streaming
- `terminal.html`: Main interface with VS Code-style theming
- `terminal.css`: Additional styling and scrollbar customization

**WebSocket Connection:**

- Connects to port 5261 with 'terminal' protocol
- Requires WebSocket key for authentication
- Supports terminal resize events and data streaming

## Payload Structure

The payload uses a structured tar format supporting multiple payload types:

```directory
payload.tar.br.enc (encrypted, brotli-compressed)
├── swagger/
│   ├── swagger-ui-bundle.js.br (brotli-compressed)
│   ├── swagger-ui-standalone-preset.js.br (brotli-compressed)
│   ├── swagger-ui.css.br (brotli-compressed)
│   ├── oauth2-redirect.html.br (brotli-compressed)
│   ├── swagger.json (uncompressed for runtime modification)
│   ├── swagger.html (uncompressed for runtime modification)
│   ├── swagger-initializer.js (uncompressed for runtime modification)
│   ├── favicon-32x32.png (uncompressed)
│   └── favicon-16x16.png (uncompressed)
├── terminal/
│   ├── terminal.html.br (brotli-compressed)
│   ├── terminal.css.br (brotli-compressed)
│   ├── xterm.js.br (brotli-compressed)
│   ├── xterm.css.br (brotli-compressed)
│   ├── xterm-addon-attach.js.br (brotli-compressed)
│   ├── xterm-addon-fit.js.br (brotli-compressed)
│   └── xtermjs_version.txt (version info)
└── helium/, acuranzo/, etc./
    ├── database.lua (database configuration)
    └── *.lua (migration files)
```

This modular structure allows easy addition of new payload types in separate directories.

## API Documentation Annotations

API endpoints are documented using C source code annotations processed by `swagger-generate.sh`:

**Format:**

```c
//@ swagger:<type> <value>
```

**Common Annotations:**

- `@swagger:tag "Name" Description` - Service-level tags
- `@swagger:path /endpoint` - Endpoint path
- `@swagger:method GET|POST|PUT|DELETE` - HTTP method
- `@swagger:summary Brief description` - Endpoint summary
- `@swagger:description Detailed description` - Full description
- `@swagger:response 200 application/json {...}` - Response schema
- `@swagger:operationId customOperationId` - Optional operation ID

## Workflow

**Generate API Documentation:**

```bash
cd payloads
./swagger-generate.sh  # Creates swagger.json from C annotations
```

**Build Encrypted Payload:**

```bash
export PAYLOAD_LOCK="base64-encoded-rsa-public-key"  # Required for encryption
./payload-generate.sh  # Downloads dependencies, compresses, encrypts
```

**View Documentation:**

1. Configure Swagger in `hydrogen.json` and set `PAYLOAD_KEY` environment variable
2. Start Hydrogen server
3. Access at `http://localhost:8080/swagger/`

## Development Guidelines

**Adding New Endpoints:**

1. Add Swagger annotations to endpoint header files
2. Run `./swagger-generate.sh` to update `swagger.json`
3. Run `./payload-generate.sh` to rebuild encrypted payload
4. Test in Swagger UI

**Adding New Payload Types:**

1. Add files to payload directory structure
2. Update `payload-generate.sh` to include new files
3. Modify server code to handle new payload types
4. Rebuild and test

## Security Considerations

- Keep `PAYLOAD_KEY` secure; never commit to version control
- Use environment variables or secure secrets management in production
- `PAYLOAD_LOCK` can be shared (used only for encryption)
- Rotate encryption keys periodically
- Test encryption/decryption when modifying payload system
- See [SECRETS.md](../docs/SECRETS.md) for environment variable management
