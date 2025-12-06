# test_95_stylelint.md

## Overview

The `test_95_stylelint.sh` script validates CSS files using stylelint to ensure consistent styling, detect errors, and enforce best practices.

## Purpose

This test ensures CSS code quality by:

- Analyzing CSS files in the project (primarily web interface styles)
- Detecting syntax errors and invalid property values
- Enforcing consistent formatting and naming conventions
- Identifying unused or redundant CSS rules
- Validating accessibility and performance considerations

## Test Categories

### Style Validation

- **Syntax Checking**: Ensures valid CSS syntax and structure
- **Property Validation**: Verifies correct property usage and values
- **Formatting**: Enforces consistent indentation and spacing
- **Naming Conventions**: Validates class and ID naming patterns
- **Best Practices**: Promotes maintainable and efficient CSS

## Configuration

The test uses:

- Standard stylelint configuration for CSS validation
- Project-specific rules aligned with the Hydrogen web interface
- Ignores vendor prefixes where appropriate

## Expected Outcomes

- **Pass**: All CSS files pass stylelint validation
- **Fail**: Syntax errors or critical issues found
- **Warning**: Style inconsistencies that should be reviewed

## Integration

This test is part of the static analysis suite (90+ range) and runs automatically with `test_00_all.sh`.