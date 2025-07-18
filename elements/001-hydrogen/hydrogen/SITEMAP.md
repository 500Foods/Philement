# SITEMAP.md

This file serves as a sitemap for all Markdown (.md) files in the repository, organized by folder.

## Folder: /

- [README.md](README.md): Main Hydrogen project README - eseentially the Table of Contents for the project
- [RECIPE.md](RECIPE.md): Instructions for helping AI models (or humans!) that are working on the project
- [RELEASES.md](RELEASES.md): Version history files
- [SECRETS.md](SECRETS.md): Outlines how secrets are managed in the project
- [SETUP.md](SETUP.md): Describes how to setup the dev environment
- [SITEMAP.md](SITEMAP.md): This file
- [STRUCTURE.md](STRUCTURE.md): Describes the file layout for the project
- [COVERAGE.svg](COVERAGE.svg): Visual coverage analysis report (auto-generated)
- [COMPLETE.svg](COMPLETE.svg): Complete test suite results visualization (auto-generated)

## Folder: /extras

- [README.md](extras/README.md): Build scripts and diagnostic tools

## Folder: /cmake

- [README.md](cmake/README.md): Description of the CMake build system

## Folder: /docs

- [README.md](docs/README.md): Links valid: No - multiple missing files in reference/ subdirectory: faq.md, troubleshooting.md, glossary.md, monitoring.md, terminal_configuration.md, security_configuration.md, and others
- [ai_integration.md](docs/ai_integration.md): Links valid: Yes
- [api.md](docs/api.md): Links valid: No - link to ../payload/README.md should be ../payloads/README.md, missing files in api/ subdirectory
- [coding_guidelines.md](docs/coding_guidelines.md): Links valid: No - link to ./security_best_practices.md not found
- [configuration.md](docs/configuration.md): Links valid: No - multiple missing files in reference/ subdirectory
- [data_structures.md](docs/data_structures.md): Links valid: Yes
- [developer_onboarding.md](docs/developer_onboarding.md): Links valid: No - link to ../payload/README.md should be ../payloads/README.md, missing files in reference/ subdirectory
- [mdns_server.md](docs/mdns_server.md): Links valid: Yes
- [oidc_integration.md](docs/oidc_integration.md): Links valid: Yes
- [print_queue.md](docs/print_queue.md): Links valid: Yes
- [service.md](docs/service.md): Links valid: Yes
- [shutdown_architecture.md](docs/shutdown_architecture.md): Links valid: Yes
- [system_info.md](docs/system_info.md): Links valid: Yes
- [testing.md](docs/testing.md): Links valid: No - links to ../tests/hydrogen_test_min.json and ../tests/hydrogen_test_max.json not found (files are in tests/configs/ subdirectory)
- [thread_monitoring.md](docs/thread_monitoring.md): Links valid: Yes
- [web_socket.md](docs/web_socket.md): Links valid: No - link to ./SECRETS.md should be ../SECRETS.md

### Folder: /docs/api/system

- [system_health.md](docs/api/system/system_health.md): Links valid: Yes
- [system_info.md](docs/api/system/system_info.md): Links valid: Yes
- [system_version.md](docs/api/system/system_version.md): Links valid: Yes

### Folder: /docs/deployment

- [docker.md](docs/deployment/docker.md): Links valid: Yes

### Folder: /docs/development

- [ai_development.md](docs/development/ai_development.md): Links valid: Yes
- [coding_guidelines.md](docs/development/coding_guidelines.md): Links valid: No - link to ../reference/service.md not found

### Folder: /docs/guides

- [print-queue.md](docs/guides/print-queue.md): Links valid: Yes
- [quick-start.md](docs/guides/quick-start.md): Links valid: Yes

### Folder: /docs/guides/use-cases

- [home-workshop.md](docs/guides/use-cases/home-workshop.md): Links valid: Yes
- [print-farm.md](docs/guides/use-cases/print-farm.md): Links valid: Yes

### Folder: /docs/reference

- [api.md](docs/reference/api.md): Links valid: No - link to ./thread_monitoring.md not found (should be ../thread_monitoring.md), link to ../print_queue.md exists
- [configuration.md](docs/reference/configuration.md): Links valid: No - link to ./service.md not found
- [data_structures.md](docs/reference/data_structures.md): Links valid: No - link to ../development/coding_guidelines.md not found (development folder doesn't exist)
- [database_architecture.md](docs/reference/database_architecture.md): Links valid: No - link to ../../SECRETS.md exists, link to ./network_architecture.md exists
- [database_configuration.md](docs/reference/database_configuration.md): Links valid: No - link to ../oidc_configuration.md not found, link to ../logging_configuration.md not found, link to ../system_architecture.md not found
- [launch_system_architecture.md](docs/reference/launch_system_architecture.md): Links valid: No - multiple missing files in launch/ subdirectory: registry_subsystem.md, and others
- [logging_configuration.md](docs/reference/logging_configuration.md): Links valid: No - link to ../monitoring.md not found
- [mdns_client_architecture.md](docs/reference/mdns_client_architecture.md): Links valid: Yes
- [mdns_configuration.md](docs/reference/mdns_configuration.md): Links valid: No - link to network_configuration.md not found
- [network_architecture.md](docs/reference/network_architecture.md): Links valid: No - link to ../system_info.md exists, link to ../mdns_server.md exists, link to ../reference/configuration.md should be ./configuration.md
- [oidc_architecture.md](docs/reference/oidc_architecture.md): Links valid: Yes
- [print_queue_architecture.md](docs/reference/print_queue_architecture.md): Links valid: No - link to ../beryllium.md not found
- [print_subsystem.md](docs/reference/print_subsystem.md): Links valid: Yes
- [printqueue_configuration.md](docs/reference/printqueue_configuration.md): Links valid: Yes
- [resources_configuration.md](docs/reference/resources_configuration.md): Links valid: No - link to ../monitoring.md not found, link to ../performance.md not found
- [smtp_configuration.md](docs/reference/smtp_configuration.md): Links valid: No - link to network_configuration.md not found, link to ../notifications.md not found
- [smtp_relay_architecture.md](docs/reference/smtp_relay_architecture.md): Links valid: No - link to ../../SECRETS.md exists
- [subsystem_registry_architecture.md](docs/reference/subsystem_registry_architecture.md): Links valid: No - link to ../reference/state_management.md not found (should be ./state_management.md but file doesn't exist)
- [swagger_architecture.md](docs/reference/swagger_architecture.md): Links valid: Yes
- [swagger_configuration.md](docs/reference/swagger_configuration.md): Links valid: No - link to ../api.md exists, link to security_configuration.md not found
- [system_architecture.md](docs/reference/system_architecture.md): Links valid: No - multiple missing files: ../payload/README.md should be ../payloads/README.md, ../SECRETS.md exists, ../ai_integration.md exists
- [system_info.md](docs/reference/system_info.md): Links valid: No - link to ./thread_monitoring.md not found (should be ../thread_monitoring.md)
- [terminal_architecture.md](docs/reference/terminal_architecture.md): Links valid: No - link to ../SECRETS.md exists
- [web_socket.md](docs/reference/web_socket.md): Links valid: Yes
- [webserver_configuration.md](docs/reference/webserver_configuration.md): Links valid: No - multiple missing files: api_configuration.md, security_configuration.md
- [webserver_subsystem.md](docs/reference/webserver_subsystem.md): Links valid: Yes
- [websocket_architecture.md](docs/reference/websocket_architecture.md): Links valid: No - link to ../security.md not found
- [websocket_configuration.md](docs/reference/websocket_configuration.md): Links valid: No - link to network_configuration.md not found
- [websocket_subsystem.md](docs/reference/websocket_subsystem.md): Links valid: Yes

### Folder: /docs/reference/launch

- [payload_subsystem.md](docs/reference/launch/payload_subsystem.md): Links valid: No - link to launch_system_architecture.md should be ../launch_system_architecture.md
- [threads_subsystem.md](docs/reference/launch/threads_subsystem.md): Links valid: Yes
- [webserver_subsystem.md](docs/reference/launch/webserver_subsystem.md): Links valid: No - link to launch_system_architecture.md should be ../launch_system_architecture.md

### Folder: /docs/releases/2024-07

- [2024-07-08.md](docs/releases/2024-07/2024-07-08.md)
- [2024-07-11.md](docs/releases/2024-07/2024-07-11.md)
- [2024-07-15.md](docs/releases/2024-07/2024-07-15.md)
- [2024-07-18.md](docs/releases/2024-07/2024-07-18.md)

### Folder: /docs/releases/2025-02

- [2025-02-08.md](docs/releases/2025-02/2025-02-08.md)
- [2025-02-12.md](docs/releases/2025-02/2025-02-12.md)
- [2025-02-13.md](docs/releases/2025-02/2025-02-13.md)
- [2025-02-14.md](docs/releases/2025-02/2025-02-14.md)
- [2025-02-15.md](docs/releases/2025-02/2025-02-15.md)
- [2025-02-16.md](docs/releases/2025-02/2025-02-16.md)
- [2025-02-17.md](docs/releases/2025-02/2025-02-17.md)
- [2025-02-18.md](docs/releases/2025-02/2025-02-18.md)
- [2025-02-19.md](docs/releases/2025-02/2025-02-19.md)
- [2025-02-20.md](docs/releases/2025-02/2025-02-20.md)
- [2025-02-21.md](docs/releases/2025-02/2025-02-21.md)
- [2025-02-22.md](docs/releases/2025-02/2025-02-22.md)
- [2025-02-23.md](docs/releases/2025-02/2025-02-23.md)
- [2025-02-24.md](docs/releases/2025-02/2025-02-24.md)
- [2025-02-25.md](docs/releases/2025-02/2025-02-25.md)
- [2025-02-26.md](docs/releases/2025-02/2025-02-26.md)
- [2025-02-27.md](docs/releases/2025-02/2025-02-27.md)
- [2025-02-28.md](docs/releases/2025-02/2025-02-28.md)

### Folder: /docs/releases/2025-03

- [2025-03-01.md](docs/releases/2025-03/2025-03-01.md)
- [2025-03-02.md](docs/releases/2025-03/2025-03-02.md)
- [2025-03-03.md](docs/releases/2025-03/2025-03-03.md)
- [2025-03-04.md](docs/releases/2025-03/2025-03-04.md)
- [2025-03-05.md](docs/releases/2025-03/2025-03-05.md)
- [2025-03-06.md](docs/releases/2025-03/2025-03-06.md)
- [2025-03-07.md](docs/releases/2025-03/2025-03-07.md)
- [2025-03-08.md](docs/releases/2025-03/2025-03-08.md)
- [2025-03-09.md](docs/releases/2025-03/2025-03-09.md)
- [2025-03-10.md](docs/releases/2025-03/2025-03-10.md)
- [2025-03-11.md](docs/releases/2025-03/2025-03-11.md)
- [2025-03-12.md](docs/releases/2025-03/2025-03-12.md)
- [2025-03-13.md](docs/releases/2025-03/2025-03-13.md)
- [2025-03-14.md](docs/releases/2025-03/2025-03-14.md)
- [2025-03-15.md](docs/releases/2025-03/2025-03-15.md)
- [2025-03-16.md](docs/releases/2025-03/2025-03-16.md)
- [2025-03-17.md](docs/releases/2025-03/2025-03-17.md)
- [2025-03-18.md](docs/releases/2025-03/2025-03-18.md)
- [2025-03-19.md](docs/releases/2025-03/2025-03-19.md)
- [2025-03-20.md](docs/releases/2025-03/2025-03-20.md)
- [2025-03-21.md](docs/releases/2025-03/2025-03-21.md)
- [2025-03-22.md](docs/releases/2025-03/2025-03-22.md)
- [2025-03-23.md](docs/releases/2025-03/2025-03-23.md)
- [2025-03-24.md](docs/releases/2025-03/2025-03-24.md)
- [2025-03-25.md](docs/releases/2025-03/2025-03-25.md)
- [2025-03-26.md](docs/releases/2025-03/2025-03-26.md)
- [2025-03-27.md](docs/releases/2025-03/2025-03-27.md)
- [2025-03-28.md](docs/releases/2025-03/2025-03-28.md)
- [2025-03-29.md](docs/releases/2025-03/2025-03-29.md)
- [2025-03-30.md](docs/releases/2025-03/2025-03-30.md)
- [2025-03-31.md](docs/releases/2025-03/2025-03-31.md)

### Folder: /docs/releases/2025-04

- [2025-04-01.md](docs/releases/2025-04/2025-04-01.md)
- [2025-04-02.md](docs/releases/2025-04/2025-04-02.md)
- [2025-04-03.md](docs/releases/2025-04/2025-04-03.md)
- [2025-04-04.md](docs/releases/2025-04/2025-04-04.md)
- [2025-04-05.md](docs/releases/2025-04/2025-04-05.md)

### Folder: /docs/releases/2025-05

- [2025-05-06.md](docs/releases/2025-05/2025-05-06.md)

### Folder: /docs/releases/2025-06

- [2025-06-01.md](docs/releases/2025-06/2025-06-01.md)
- [2025-06-03.md](docs/releases/2025-06/2025-06-03.md)
- [2025-06-09.md](docs/releases/2025-06/2025-06-09.md)
- [2025-06-16.md](docs/releases/2025-06/2025-06-16.md)
- [2025-06-17.md](docs/releases/2025-06/2025-06-17.md)
- [2025-06-18.md](docs/releases/2025-06/2025-06-18.md)
- [2025-06-19.md](docs/releases/2025-06/2025-06-19.md)
- [2025-06-20.md](docs/releases/2025-06/2025-06-20.md)
- [2025-06-23.md](docs/releases/2025-06/2025-06-23.md)
- [2025-06-25.md](docs/releases/2025-06/2025-06-25.md)
- [2025-06-28.md](docs/releases/2025-06/2025-06-28.md)
- [2025-06-29.md](docs/releases/2025-06/2025-06-29.md)
- [2025-06-30.md](docs/releases/2025-06/2025-06-30.md)

### Folder: /docs/releases/2025-07

- [2025-07-01.md](docs/releases/2025-07/2025-07-01.md)
- [2025-07-02.md](docs/releases/2025-07/2025-07-02.md)
- [2025-07-03.md](docs/releases/2025-07/2025-07-03.md)
- [2025-07-04.md](docs/releases/2025-07/2025-07-04.md)

## Folder: /examples

- [README.md](examples/README.md): Links valid: Yes

## Folder: /payloads

- [README.md](payloads/README.md): Links valid: No - link to ./payload/README.md not found (payload subdirectory does not exist)

## Folder: /src/api

- [README.md](src/api/README.md): Links valid: No - link to ../../docs/api/oidc/oidc_endpoints.md not found, link to ../../tests/test_32_system_endpoints.sh should be ../../tests/test_32_system_endpoints.sh, link to ../../oidc-client-examples/ not found

## Folder: /tests

- [README.md](tests/README.md): Links valid: Yes - Testing documentation and procedures

### Folder: /tests/docs

- [cloc.md](tests/docs/cloc.md): CLOC (Count Lines of Code) library documentation for code analysis and line counting functionality
- [coverage.md](tests/docs/coverage.md): Comprehensive coverage analysis system documentation including coverage.sh, coverage-common.sh, coverage-unity.sh, coverage-blackbox.sh, and coverage_table.sh
- [env_utils.md](tests/docs/env_utils.md): Environment utility functions documentation
- [file_utils.md](tests/docs/file_utils.md): File utility functions documentation
- [framework.md](tests/docs/framework.md): Testing framework overview and architecture
- [github-sitemap.md](tests/docs/github-sitemap.md): GitHub repository sitemap
- [LIBRARIES.md](tests/docs/LIBRARIES.md): Table of Contents for modular library scripts in the 'lib/' directory
- [lifecycle.md](tests/docs/lifecycle.md): Test lifecycle management documentation
- [log_output.md](tests/docs/log_output.md): Log output formatting and analysis
- [network_utils.md](tests/docs/network_utils.md): Network utility functions documentation
- [Oh.md](tests/docs/Oh.md): ANSI terminal output to SVG converter documentation
- [test_00_all.md](tests/docs/test_00_all.md): Test orchestration and execution framework
- [test_01_compilation.md](tests/docs/test_01_compilation.md): Compilation and build verification tests
- [test_12_env_variables.md](tests/docs/test_12_env_variables.md): Environment variable validation
- [test_14_env_payload.md](tests/docs/test_14_env_payload.md): Environment payload testing
- [test_16_library_dependencies.md](tests/docs/test_16_library_dependencies.md): Library dependency verification
- [test_18_json_error_handling.md](tests/docs/test_18_json_error_handling.md): JSON error handling tests
- [test_13_crash_handler.md](tests/docs/test_13_crash_handler.md): Crash handling and recovery tests
- [test_22_startup_shutdown.md](tests/docs/test_22_startup_shutdown.md): Startup and shutdown lifecycle tests
- [test_24_signals.md](tests/docs/test_24_signals.md): Signal handling tests
- [test_26_shutdown.md](tests/docs/test_26_shutdown.md): Shutdown sequence testing
- [test_28_socket_rebind.md](tests/docs/test_28_socket_rebind.md): Socket rebinding tests
- [test_29_api_prefix.md](tests/docs/test_29_api_prefix.md): API prefix configuration tests
- [test_32_system_endpoints.md](tests/docs/test_32_system_endpoints.md): System endpoint functionality tests
- [test_34_swagger.md](tests/docs/test_34_swagger.md): Swagger documentation tests
- [test_36_websockets.md](tests/docs/test_36_websockets.md): WebSocket server functionality and integration tests
- [test_03_code_size.md](tests/docs/test_03_code_size.md): Code size analysis and metrics
- [test_06_check_links.md](tests/docs/test_06_check_links.md): Link validation tests
- [test_19_leaks_like_a_sieve.md](tests/docs/test_19_leaks_like_a_sieve.md): Memory leak detection tests
- [test_11_unity.md](tests/docs/test_11_unity.md): Unity testing framework integration
- [test_91_cppcheck.md](tests/docs/test_91_cppcheck.md): C/C++ static analysis tests
- [test_92_shellcheck.md](tests/docs/test_92_shellcheck.md): Shell script linting tests
- [test_94_eslint.md](tests/docs/test_94_eslint.md): JavaScript linting tests
- [test_95_stylelint.md](tests/docs/test_95_stylelint.md): CSS linting tests
- [test_96_htmlhint.md](tests/docs/test_96_htmlhint.md): HTML validation tests
- [test_97_jsonlint.md](tests/docs/test_97_jsonlint.md): JSON validation tests
- [test_98_markdownlint.md](tests/docs/test_98_markdownlint.md): Markdown linting tests
- [test_99_coverage.md](tests/docs/test_99_coverage.md): Build system coverage information

### Folder: /tests/results

- [latest_test_results.md](tests/results/latest_test_results.md): Links valid: Yes
- [repository_info.md](tests/results/repository_info.md): Links valid: Yes
