# Secrets Management in Hydrogen

This document explains how secrets are managed in the Hydrogen project using environment variables, with a particular focus on the encrypted payload system.

## Environment Variables for Secrets Management

Hydrogen uses environment variables to manage sensitive configuration values such as encryption keys, authentication tokens, and other secrets. This approach provides several advantages:

1. **Security**: Secrets aren't stored in the codebase or configuration files
2. **Separation of concerns**: Code and configuration are separate from sensitive credentials
3. **Environment-specific values**: Different environments (development, testing, production) can use different secrets
4. **Compatibility**: Works across different platforms and deployment methods

### Using Environment Variables in Bash and Zsh

To verify your environment variables are set correctly, use the test script:
```bash
# Run the environment variable test
./tests/test_env_payload.sh
```

This script validates that:
1. Required environment variables (PAYLOAD_KEY and PAYLOAD_LOCK) are present
2. The keys are valid RSA key pairs in the correct format

To set environment variables in Bash or Zsh:

**Temporary (current session only):**

```bash
# Simple assignment
export PAYLOAD_KEY="your-secret-key-here"

# Multi-line values
export PAYLOAD_KEY=$(cat <<EOF
-----BEGIN RSA PRIVATE KEY-----
Example
RSA
Key 
Here
-----END RSA PRIVATE KEY-----
EOF
)

# Reading from a file
export PAYLOAD_KEY=$(cat /path/to/private_key.pem)
```

**Persistent (add to shell profile):**

For Bash, add to `~/.bashrc` or `~/.bash_profile`:
```bash
export PAYLOAD_KEY="your-secret-key-here"
```

For Zsh, add to `~/.zshrc`:
```bash
export PAYLOAD_KEY="your-secret-key-here"
```

**Applying changes:**
```bash
# For current shell session
source ~/.bashrc   # or ~/.zshrc

# For new shell sessions, changes take effect automatically
```

**For a single command:**
```bash
PAYLOAD_KEY="your-secret-key-here" ./hydrogen
```

## Encrypted Payload System

The Hydrogen project uses an RSA+AES hybrid encryption system to secure embedded payloads. This system requires two environment variables:

1. **PAYLOAD_LOCK**: The RSA public key used during build to encrypt payload contents
2. **PAYLOAD_KEY**: The RSA private key used during runtime to decrypt payload contents

### Generating Keys for the Payload System

Generate a suitable RSA key pair using OpenSSL:

```bash
# Generate a 2048-bit RSA private key
openssl genrsa -out private_key.pem 2048

# Extract the public key
openssl rsa -in private_key.pem -pubout -out public_key.pem

# Set environment variables
export PAYLOAD_LOCK=$(cat public_key.pem | base64 -w 0)
export PAYLOAD_KEY=$(cat private_key.pem | base64 -w 0)
```

### How the Payload Encryption Works

1. During build time:
   - A random AES-256 key is generated for payload encryption
   - A random 16-byte IV is generated for AES-CBC mode
   - The payload (currently SwaggerUI) is compressed with Brotli
   - The compressed payload is encrypted with AES-256-CBC using the random key and IV
   - The AES key is encrypted with the RSA public key (PAYLOAD_LOCK)
   - The components are combined into a single file:
     ```
     [key_size(4 bytes)] + [encrypted_aes_key] + [iv(16 bytes)] + [encrypted_payload]
     ```
   - This file is appended to the executable with a marker

2. During runtime:
   - The application extracts the encrypted data from itself
   - The key size is read from the first 4 bytes
   - The encrypted AES key is extracted and decrypted using PAYLOAD_KEY
   - The 16-byte IV is extracted
   - The encrypted payload is decrypted using AES-256-CBC with the decrypted key and IV
   - The decrypted payload (Brotli-compressed tar) is decompressed and processed

### Encryption Details

- **AES Configuration**:
  - Algorithm: AES-256-CBC
  - Key: 256-bit random key
  - IV: 16-byte random initialization vector
  - Padding: PKCS7
  - Implementation: OpenSSL EVP API

- **RSA Configuration**:
  - Key Size: 2048 bits
  - Padding: PKCS1
  - Implementation: OpenSSL EVP API

- **Compression**:
  - Algorithm: Brotli
  - Quality: Maximum (11)
  - Window Size: 24 (16MB)

This approach provides strong security while maintaining good performance, as AES is efficient for large data while RSA securely protects the smaller AES key.

## Future Secret Management

The Hydrogen project is expected to incorporate additional secrets for various subsystems:

### Terminal Subsystem

The Terminal feature may require authentication tokens or other credentials that will be managed through environment variables such as:
- `TERMINAL_AUTH_TOKEN`
- `TERMINAL_SESSION_SECRET`

### OIDC Integration

The OpenID Connect functionality will require various secrets:
- `OIDC_CLIENT_SECRET`
- `OIDC_SIGNING_KEY`
- `OIDC_ENCRYPTION_KEY`

All these secrets will follow the same environment variable pattern documented here.

## References and Resources

- [README.md](./README.md): Main project documentation
- [payload/README.md](./payload/README.md): Payload system details
- [OpenSSL Documentation](https://www.openssl.org/docs/)
- [RSA and AES Hybrid Encryption](https://en.wikipedia.org/wiki/Hybrid_cryptosystem)