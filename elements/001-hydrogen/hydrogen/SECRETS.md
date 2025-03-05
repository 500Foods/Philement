# Secrets Management in Hydrogen

This document explains how secrets are managed in the Hydrogen project using environment variables, with a particular focus on the encrypted payload system.

## Environment Variables for Secrets Management

Hydrogen uses environment variables to manage sensitive configuration values such as encryption keys, authentication tokens, and other secrets. This approach provides several advantages:

1. **Security**: Secrets aren't stored in the codebase or configuration files
2. **Separation of concerns**: Code and configuration are separate from sensitive credentials
3. **Environment-specific values**: Different environments (development, testing, production) can use different secrets
4. **Compatibility**: Works across different platforms and deployment methods

### Using Environment Variables in Bash and Zsh

To set environment variables in Bash or Zsh:

**Temporary (current session only):**

```bash
# Simple assignment
export PAYLOAD_KEY="your-secret-key-here"

# Multi-line values
export PAYLOAD_KEY=$(cat <<EOF
-----BEGIN RSA PRIVATE KEY-----
MIIBOgIBAAJBAKj34GkxFhD90vcNLYLInFEX6Ppy1tPf9Cnzj4p4WGeKLs1Pt8Qu
KUpRKfFLfRYC9AIKjbJTWit+CqvjWYzvQwECAwEAAQJAIJLixBy2qpFoS4DSmoEm
o3qGy0t6z09AIJtH+5OeRV1be+N4cDYJKffGzDa88vQENZiRm0GRq6a+HPGQMd2k
TQIhAKMSvzIBnni7ot/OSie2TmJLY4SwTQAEvXysE2RbFDYdAiEBCUEaRQnMnbp7
9mxDXDf6AU0cN/RPBjb9qSHDcWZHGzUCIG2Es59z8ugGrDY+pxLQnwfotadxd+Uy
v/Ow5T0q5gIJAiEAyS4RaI9YG8EWx/2w0T67ZUVAw8eOMB6BIUg0Xcu+3okCIBOs
/5OiPgoTdSy7bcF9IGpSE8ZgGKzgYQVZeN97YE00
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

> **Note:** The system uses SHA-1 as the digest algorithm for key derivation during the AES encryption/decryption process. This is explicitly specified in both the encryption and decryption code.

### How the Payload Encryption Works

1. During build time:
   - A random AES-256 key is generated
   - The payload (currently containing SwaggerUI) is encrypted with this AES key
   - The AES key is then encrypted with the RSA public key (PAYLOAD_LOCK)
   - Both the encrypted AES key and encrypted payload are combined into a single file
   - This file is appended to the executable

2. During runtime:
   - The application extracts the encrypted data from itself
   - It uses the RSA private key (PAYLOAD_KEY) to decrypt the AES key
   - The decrypted AES key is then used to decrypt the payload
   - The decrypted payload is processed normally

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