# test_97_xmlstarlet.md

## Overview

The `test_97_xmlstarlet.sh` script validates XML and SVG files using xmlstarlet to ensure proper markup and standards compliance.

## Purpose

This test ensures XML/SVG code quality by:

- Analyzing XML and SVG files in the project (configuration, documentation, graphics)
- Detecting malformed markup and syntax errors
- Validating proper XML structure and namespace declarations
- Ensuring compliance with XML standards
- Checking for well-formedness and validity

## Test Categories

### Markup Validation

- **Syntax Checking**: Ensures well-formed XML/SVG structure
- **Namespace Validation**: Verifies proper namespace declarations
- **DTD/Schema Compliance**: Validates against DTD if specified
- **Standards Compliance**: Validates XML 1.0/1.1 specification adherence
- **Well-formedness**: Checks for proper opening/closing tags and attributes

## Configuration

The test uses:

- xmlstarlet val for validation against DTD (if present)
- Standard XML parsing rules for well-formedness
- Project-specific file exclusions for generated or external files

## Expected Outcomes

- **Pass**: All XML/SVG files pass xmlstarlet validation
- **Fail**: Syntax errors or structural issues found
- **Warning**: Files without DTD that cannot be fully validated

## Integration

This test is part of the static analysis suite (90+ range) and runs automatically with `test_00_all.sh`.