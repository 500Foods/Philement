# Hydrogen Installer System

## Overview

The installer directory contains the Hydrogen installation system, which provides a self-contained, automated installer for deploying Hydrogen on target systems. The system creates distributable installer scripts that bundle the Hydrogen executable, configuration files, and documentation.

## Architecture

The installer system uses a template-based approach:

- **`installer_wrapper.sh`**: Template script containing installer logic and placeholders
- **`hydrogen_installer_*.sh`**: Generated installer executables with embedded content

## Generated Installer Files

| File | Description |
|------|-------------|
| `hydrogen_installer_YYYYMMDD.sh` | Self-contained installer script for specific build date |
| `installer_wrapper.sh` | Template used to generate installer executables |

## Build Process

Installers are automatically generated during the compilation test (`test_01_compilation.sh`):

1. **Extract Metadata**: Version and release information from `hydrogen_release` executable
2. **Encode Assets**: Base64 encode executable, configuration, and license files
3. **Generate Script**: Replace placeholders in `installer_wrapper.sh` template
4. **Optional Signing**: Cryptographically sign installer if keys are available
5. **Cleanup**: Maintain only 5 most recent installer versions

## Embedded Content

Each installer contains base64-encoded versions of:

- **Hydrogen Executable**: The compiled `hydrogen_release` binary
- **Default Configuration**: `configs/hydrogen_default.json`
- **License**: `LICENSE.md`
- **Optional Signature**: Cryptographic signature for verification

## Installer Features

### Command Line Options

```bash
hydrogen_installer_YYYYMMDD.sh [OPTIONS]

Options:
  --version                    Show version and release information
  --help                       Show help message
  --skip-dependency-check      Skip system dependency verification
  --skip-signature-check       Skip installer signature validation
  --skip-service-install       Skip systemd service installation
  -y                           Automatically answer 'yes' to prompts
```

### Installation Process

1. **Initial Checks**: Verify embedded content availability
2. **User Prompts**: Interactive confirmation with colored, word-wrapped output
3. **Optional Validation**: Dependency checking, signature verification
4. **Service Setup**: Systemd service installation (if not skipped)
5. **File Deployment**: Extract and install executable and configuration files

## Security Features

### Cryptographic Signing

- **RSA Key Pair**: Uses environment variables `HYDROGEN_INSTALLER_KEY` (private) and `HYDROGEN_INSTALLER_LOCK` (public)
- **SHA-256 Signatures**: Cryptographically signs entire installer content
- **Base64 Encoding**: Secure encoding of binary assets

### Content Verification

- **SHA-256 Hashes**: Embedded content includes integrity hashes
- **Signature Validation**: Optional installer signature verification
- **Embedded Markers**: Uses `<<< HERE BE ME TREASURE >>>` for payload location

## Development Workflow

### Building Installers

Installers are built automatically during compilation testing:

```bash
# Run full test suite (includes installer generation)
./tests/test_00_all.sh

# Or run compilation test specifically
./tests/test_01_compilation.sh
```

### Manual Generation

To manually trigger installer generation:

```bash
# Ensure release build exists
cmake --build build --target hydrogen_release

# Set signing keys (optional)
export HYDROGEN_INSTALLER_KEY="base64-private-key"
export HYDROGEN_INSTALLER_LOCK="base64-public-key"

# Run compilation test
./tests/test_01_compilation.sh
```

## Distribution

### File Naming Convention

Installers follow the pattern: `hydrogen_installer_YYYYMMDD.sh`

- **YYYYMMDD**: Build date in ISO format
- **Version Tracking**: Date-based versioning for easy identification

### Deployment Options

- **Standalone Executable**: Self-contained bash script, no external dependencies
- **Automated Installation**: Supports both interactive and non-interactive modes
- **System Integration**: Optional systemd service installation

## Maintenance

### Cleanup Process

The build system automatically:

- Maintains 5 most recent installer versions
- Removes older installers to prevent accumulation
- Preserves installer integrity during cleanup

### Version Management

- **Automatic Versioning**: Extracts version from executable metadata
- **Date-Based Releases**: Uses build date for release identification
- **Content Hashing**: SHA-256 hashes for all embedded assets

## Troubleshooting

### Common Issues

#### Missing Executable

```bash
# Ensure release build exists
cmake --build build --target hydrogen_release
```

#### Signature Verification Fails

```bash
# Check signing key environment variables
echo $HYDROGEN_INSTALLER_KEY
echo $HYDROGEN_INSTALLER_LOCK
```

#### Permission Issues

```bash
# Make installer executable
chmod +x hydrogen_installer_*.sh
```

### Debug Information

#### Installer Contents

```bash
# View embedded content markers
grep "### HYDROGEN" hydrogen_installer_*.sh
```

#### Version Information

```bash
# Check installer version
./hydrogen_installer_YYYYMMDD.sh --version
```

## Security Considerations

- **Key Management**: Never commit private keys to version control
- **Environment Variables**: Use secure secrets management for production keys
- **Signature Validation**: Always verify signatures in production deployments
- **Regular Rotation**: Periodically rotate signing keys
- **Access Control**: Restrict installer execution to authorized users