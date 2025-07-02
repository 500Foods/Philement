# Test Scripts Documentation

This document serves as a table of contents for the detailed documentation of each test script in the Hydrogen test suite. Each link below leads to a dedicated markdown file with comprehensive information about the respective test script, including its purpose, key features, usage instructions, and additional notes.

## Test Scripts

- [Test 00 All](test_00_all.md) - Primary test orchestration tool for running the entire test suite. [Script](../test_00_all.sh)
- [Test 10 Compilation](test_10_compilation.md) - Verifies compilation of Hydrogen components without errors or warnings. [Script](../test_10_compilation.sh)
- [Test 12 Environment Payload](test_12_env_payload.md) - Validates configuration of payload encryption environment variables. [Script](../test_12_env_payload.sh)
- [Test 15 Startup Shutdown](test_15_startup_shutdown.md) - Tests startup and shutdown sequences with minimal and maximal configurations. [Script](../test_15_startup_shutdown.sh)
- [Test 20 Shutdown](test_20_shutdown.md) - Validates shutdown behavior for clean resource release. [Script](../test_20_shutdown.sh)
- [Test 25 Library Dependencies](test_25_library_dependencies.md) - Checks presence and versions of required library dependencies. [Script](../test_25_library_dependencies.sh)
- [Test 30 Unity Tests](test_30_unity_tests.md) - Runs unit tests using the Unity testing framework. [Script](../test_30_unity_tests.sh)
- [Test 35 Environment Variables](test_35_env_variables.md) - Validates handling of environment variables for server configuration. [Script](../test_35_env_variables.sh)
- [Test 40 JSON Error Handling](test_40_json_error_handling.md) - Tests handling of malformed JSON configurations and error reporting. [Script](../test_40_json_error_handling.sh)
- [Test 45 Signals](test_45_signals.md) - Validates server response to various system signals. [Script](../test_45_signals.sh)
- [Test 50 Crash Handler](test_50_crash_handler.md) - Tests crash handler functionality and core dump generation. [Script](../test_50_crash_handler.sh)
- [Test 55 Socket Rebind](test_55_socket_rebind.md) - Validates socket binding and port reuse after unclean shutdowns. [Script](../test_55_socket_rebind.sh)
- [Test 60 API Prefixes](test_60_api_prefixes.md) - Tests API routing with different URL prefixes and configurations. [Script](../test_60_api_prefixes.sh)
- [Test 65 System Endpoints](test_65_mdns.md) - Validates system endpoints functionality. [Script](../test_65_system_endpoints.sh)
- [Test 70 Swagger](test_70_swagger_ui.md) - Tests Swagger/OpenAPI integration for API documentation. [Script](../test_70_swagger.sh)
- [Test 95 Leaks Like a Sieve](test_95_thread_monitoring.md) - Validates memory leak detection. [Script](../test_95_leaks_like_a_sieve.sh)
- [Test 99 Codebase Analysis](test_99_codebase.md) - Performs comprehensive codebase analysis including build cleaning, source analysis, and linting. [Script](../test_99_codebase.sh)

## Related Documentation

- [LIBRARIES.md](LIBRARIES.md) - Table of Contents for modular library scripts in the 'lib/' directory.
- [Migration Plan](Migration_Plan.md) - Overview of the test suite overhaul strategy.
- [README.md](../README.md) - Main documentation for the Hydrogen test suite.
