# Profile Test Suite Script Documentation

## Overview

The `profile_test_suite.sh` script provides performance profiling capabilities for test scripts in the Hydrogen test suite. It uses `strace` to monitor system calls, tracking fork operations and command executions to analyze the performance characteristics of test runs.

## Script Information

- **Script**: `profile_test_suite.sh`
- **Purpose**: Performance profiling and system call analysis for test scripts
- **Dependencies**: strace, standard Unix utilities

## Key Features

- **System Call Tracing**: Uses strace to monitor fork and execve system calls
- **Command Execution Analysis**: Tracks all external command invocations
- **Performance Metrics**: Counts executions by command type and frequency
- **Resource Monitoring**: Captures system limits and memory usage
- **Error Logging**: Comprehensive error capture and reporting

## Usage

```bash
./profile_test_suite.sh [script_name]
```

### Parameters

- `script_name`: Optional test script to profile (default: `test_00_all.sh`)

### Output Files

- `profile_trace.txt`: Raw strace output with all system calls
- `profile_summary.txt`: Formatted summary of profiling results
- `profile_error.txt`: Error log and diagnostic information

## Functionality

### Profiling Process

1. **Setup Phase**: Validates strace availability and test script executability
2. **System Preparation**: Sets file descriptor limits and captures system state
3. **Tracing Execution**: Runs strace with fork/execve tracing on target script
4. **Data Analysis**: Processes trace output to categorize command executions
5. **Reporting**: Generates formatted summary with execution counts

### Command Categories

The script categorizes command executions into logical groups:

- **Core Execution**: hydrogen binary, bash, sh, xargs
- **File Operations**: cat, find, file manipulation utilities
- **Text Processing**: grep, sed, awk, text utilities
- **System Utilities**: mkdir, mktemp, path utilities
- **Build Tools**: cmake, make, cc, gcov
- **Quality Tools**: cppcheck, shellcheck, linting tools
- **Analysis Tools**: cloc, tables

## Integration with Test Suite

Used for performance analysis and optimization of the test suite:

- **Bottleneck Identification**: Reveals frequently called commands
- **Resource Usage Analysis**: Tracks system call patterns
- **Optimization Opportunities**: Identifies potential performance improvements
- **Debugging Support**: Provides detailed execution traces

## Dependencies

- **strace**: System call tracer (required)
- **Standard Unix Tools**: grep, date, ulimit, free
- **Test Scripts**: Target scripts must be executable

## Error Handling

- Validates strace availability before execution
- Checks test script executability
- Captures and logs all errors during profiling
- Continues processing even with partial failures
- Provides clear error messages and diagnostic information

## Usage Examples

### Profile Main Test Suite

```bash
./profile_test_suite.sh
# Profiles test_00_all.sh by default
```

### Profile Specific Test

```bash
./profile_test_suite.sh test_01_compilation.sh
# Profiles compilation test specifically
```

### Analyze Results

```bash
# View summary
cat profile_summary.txt

# Examine detailed trace
head -50 profile_trace.txt

# Check for errors
cat profile_error.txt
```

## Output Interpretation

### Summary Format

```log
Profiling Summary for test_00_all.sh [timestamp]
-----------------------------------
  Total exec: 1250

  hydrogen: 45

  bash: 234
  sh: 12
  xargs: 89

  [additional command counts...]
```

### Key Metrics

- **Total exec**: Total number of execve system calls
- **hydrogen**: Number of times the hydrogen binary was executed
- **Command counts**: Frequency of each command type execution

## Performance Considerations

- **File Descriptor Limits**: Automatically increases ulimit for large traces
- **Memory Monitoring**: Captures system memory state
- **Large Output Handling**: Manages potentially large trace files
- **Incremental Processing**: Processes trace data in categorized chunks

## Troubleshooting

Common issues and solutions:

- **strace not found**: Ensure strace is installed in /usr/bin
- **Permission denied**: Script must be executable
- **Large trace files**: Monitor disk space for long-running tests
- **Incomplete traces**: Check profile_error.txt for strace failures

## Related Documentation

- [Test 00 All](test_00_all.md) - Main test orchestration script
- [Framework Library](framework.md) - Test framework utilities
- [LIBRARIES.md](LIBRARIES.md) - Complete library documentation index