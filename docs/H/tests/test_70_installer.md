# Test 70: Installer

## Purpose

Test 70 (Installer) is a standalone test that builds installer scripts from the Hydrogen release executable and associated files. This test was extracted from Test 01 to provide focused testing of the installer building functionality, allowing for independent validation and faster iterative development of installer-related features.

## Overview

The Installer test validates the complete installer creation process, including:

- **Release Executable Validation**: Ensures the hydrogen_release executable exists and has embedded payload
- **File Integrity**: Validates SHA256 hashes for all components (executable, configuration, license)
- **Base64 Encoding**: Compresses and encodes files using Brotli + Base64 for efficient storage
- **Signature Generation**: Optionally creates digital signatures using OpenSSL (when keys are available)
- **Installer Assembly**: Combines all components into a self-contained executable installer script
- **Cleanup Management**: Maintains only the 5 most recent installer files

## Dependencies

This test requires:

1. **Release Executable**: Must have hydrogen_release built and available
2. **Payload Validation**: Release executable must contain embedded payload marker
3. **Template Script**: installer_wrapper.sh must exist in the installer/ directory
4. **Compression Tools**: brotli, base64 utilities
5. **Cryptographic Tools**: openssl (optional, for signature generation)

## Configuration

The test respects several environment variables:

- **HYDROGEN_INSTALLER_KEY**: Private key for signing installers (Base64 encoded, Brotli compressed)
- **HYDROGEN_INSTALLER_LOCK**: Public key for installer signature validation

## Test Structure

The test performs these validation steps:

1. **Release Executable Check**
   - Verifies hydrogen_release file exists
   - Confirms embedded payload marker is present

2. **Installer Generation**
   - Creates dated installer (hydrogen_installer_YYYYMMDD.sh)
   - Implements smart skip logic (if installer is newer than executable)
   - Generates SHA256 hashes for all components
   - Compresses and Base64 encodes files
   - Replaces template placeholders with actual values
   - Adds digital signature (if private key available)

3. **Current Installer Check** *(Always runs against most recent installer)*
    - Validates that the most recent installer file exists
    - Reports the installer date in readable format

4. **Embedded Executable Validation** *(Always runs against most recent installer)*
    - Validates executable section is proper ELF binary
    - Reports executable size in bytes

5. **Embedded Configuration Validation** *(Always runs against most recent installer)*
    - Validates configuration section is valid JSON
    - Reports configuration size in bytes

6. **Embedded License Validation** *(Always runs against most recent installer)*
    - Validates license section is valid markdown
    - Reports license size in bytes

7. **Embedded Signature Validation** *(Always runs against most recent installer)*
    - Validates signature section is proper base64 encoding
    - Reports signature size in bytes (if present)

8. **Signature Verification** *(Always runs against most recent installer)*
    - Verifies installer signature using public key (if available)
    - Validates cryptographic integrity of installer

9. **Optimization Features**
   - **Incremental Builds**: Skips installer creation if hydrogen_release hasn't changed
   - **Size Optimization**: Uses Brotli compression before Base64 encoding
   - **Cleanup Logic**: Automatically removes installers older than the 5 most recent

## Output

Successful test execution produces:

- **Installer Script**: Self-contained executable installer in installer/ directory
- **File Size Report**: Human-readable file size of generated installer
- **Hash Validation**: SHA256 hashes for all embedded components
- **Signature Status**: Information about digital signature generation
- **Current Installer Check**: Date and existence verification of most recent installer
- **Embedded Executable Validation**: Detailed validation results for executable component
- **Embedded Configuration Validation**: Detailed validation results for JSON configuration
- **Embedded License Validation**: Detailed validation results for markdown license
- **Embedded Signature Validation**: Detailed validation results for signature component
- **Signature Verification**: Cryptographic verification results (if keys available)

## Error Handling

The test provides detailed error reporting for:

- Missing hydrogen_release executable
- Missing embedded payload markers
- Failed compression or encoding operations
- Template file issues
- Signature generation failures (non-blocking)

## Usage

### Standalone Execution

```bash
# Run installer test independently
./tests/test_70_installer.sh

# Or from project root
cd elements/001-hydrogen/hydrogen
./tests/test_70_installer.sh
```

### Integration Testing

```bash
# Run as part of full test suite
./tests/test_00_all.sh

# Run with specific tests
./tests/test_00_all.sh 70_installer
```

### Manual Verification

```bash
# Check if installer was generated
ls -la installer/hydrogen_installer_*.sh

# Verify installer contents
head -20 installer/hydrogen_installer_YYYYMMDD.sh

# Test installer (if desired)
chmod +x installer/hydrogen_installer_YYYYMMDD.sh
./installer/hydrogen_installer_YYYYMMDD.sh --help
```

## Validation Behavior

### Always-On Validation

A key feature of the enhanced test is that **validation always runs**, regardless of whether a new installer was built:

1. **Automatic Installer Discovery**: If no new installer was created (due to skip logic), the test automatically finds the most recent existing installer
2. **Consistent Validation**: Embedded file validation and signature verification run against the most recent available installer
3. **Fallback Handling**: If no installer exists at all, validation is gracefully skipped with appropriate messaging

### Validation Priority Order

1. Use newly created installer (if built in current test run)
2. Find most recent existing installer (using file timestamps)
3. Skip validation gracefully (if no installer available)

## Optimization Details

### Smart Skip Logic

The test implements intelligent skipping to avoid unnecessary work:

- **Timestamp Comparison**: Checks if existing installer is newer than hydrogen_release
- **Incremental Builds**: Only regenerates installer when source files have changed
- **Performance**: Reduces build time from ~10-15 seconds to ~1 second for unchanged builds

### Compression Strategy

The installer system uses multi-layer compression:

1. **Brotli Compression**: Applied to all components for maximum size reduction
2. **Base64 Encoding**: Line-wrapped at 76 characters for script compatibility
3. **Comment Prefixing**: All encoded data prefixed with '#' for shell script safety

### Signature Security

Digital signatures provide installer integrity:

- **OpenSSL Integration**: Uses SHA256 hashing with RSA signing
- **Key Management**: Private keys stored encrypted and compressed
- **Validation**: Public key embedded for signature verification during installation

## File Structure

The installer test works with these key files:

```files
installer/
├── installer_wrapper.sh           # Template script with placeholders
├── hydrogen_installer_YYYYMMDD.sh # Generated installer (output)
└── hydrogen_installer_*.sh        # Historical installers (maintained)
```

## Embedded File Validation

The test includes comprehensive validation of all embedded files:

### Validation Process

1. **Executable Validation**
   - Base64 decode and Brotli decompress the embedded executable
   - Verify ELF magic number (`\x7fELF`) to confirm valid binary format
   - Check file integrity using SHA256 hash comparison

2. **Configuration Validation**
   - Base64 decode and Brotli decompress the embedded JSON config
   - Validate JSON syntax using `jq` parser
   - Confirm required configuration fields are present

3. **License Validation**
   - Base64 decode and Brotli decompress the embedded markdown license
   - Verify markdown format (presence of `#` headers)
   - Check content structure and readability

4. **Signature Validation**
   - Base64 decode the embedded signature
   - Verify signature format and encoding
   - Optional validation (non-blocking if missing)

### Signature Verification

The test performs cryptographic signature verification:

1. **Key Handling**
   - Decode public key from `HYDROGEN_INSTALLER_LOCK` environment variable
   - Brotli decompress and Base64 decode the key material

2. **Signature Processing**
   - Extract signature from installer file
   - Base64 decode the signature content
   - Create temporary installer copy without signature section

3. **Cryptographic Verification**
   - Use OpenSSL to verify SHA256 signature
   - Validate signature against installer content
   - Provide detailed success/failure reporting

## Related Documentation

- [LIBRARIES.md](/docs/H/tests/LIBRARIES.md) - Table of Contents for modular library scripts in the 'lib/' directory
- [framework.md](/docs/H/tests/framework.md) - Testing framework overview and architecture
- [file_utils.md](/docs/H/tests/file_utils.md) - File utility functions documentation
- [log_output.md](/docs/H/tests/log_output.md) - Log output formatting and analysis
- [installer/README.md](/elements/001-hydrogen/hydrogen/installer/README.md) - Installer system documentation
- [test_01_compilation.md](/docs/H/tests/test_01_compilation.md) - Original compilation test (installer functionality removed)

## Version History

- **1.3.0** - 2025-12-03 - Restructured validation tests to match expected output format with separate tests for each embedded file type
- **1.2.0** - 2025-12-03 - Validation now runs against most recent installer regardless of build status
- **1.1.0** - 2025-12-03 - Added embedded file validation and signature verification
- **1.0.0** - 2025-12-03 - Initial standalone installer test, extracted from Test 01 compilation