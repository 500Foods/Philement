# test_96_htmlhint.md

## Overview

The `test_96_htmlhint.sh` script validates HTML files using HTMLHint to ensure proper markup, accessibility, and web standards compliance.

## Purpose

This test ensures HTML code quality by:

- Analyzing HTML files in the project (examples, documentation, web interfaces)
- Detecting malformed markup and syntax errors
- Validating proper semantic structure and accessibility
- Ensuring compliance with HTML5 standards
- Checking for security vulnerabilities in HTML content

## Test Categories

### Markup Validation

- **Syntax Checking**: Ensures well-formed HTML structure
- **Semantic Validation**: Verifies proper use of HTML5 semantic elements
- **Accessibility**: Checks for alt attributes, ARIA labels, and accessibility compliance
- **Standards Compliance**: Validates HTML5 specification adherence
- **Security**: Identifies potential XSS vulnerabilities

## Configuration

The test uses:

- Standard HTMLHint rules for HTML validation
- Accessibility-focused checks for web interface compliance
- Project-specific rules for documentation and example files

## Expected Outcomes

- **Pass**: All HTML files pass HTMLHint validation
- **Fail**: Syntax errors or accessibility issues found
- **Warning**: Minor issues that should be reviewed for compliance

## Integration

This test is part of the static analysis suite (90+ range) and runs automatically with `test_00_all.sh`.