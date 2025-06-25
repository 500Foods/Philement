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

## Folder: /examples

- [README.md](examples/README.md): Links valid: Yes

## Folder: /payloads

- [README.md](payloads/README.md): Links valid: No - link to ./payload/README.md not found (payload subdirectory does not exist)

## Folder: /src/api

- [README.md](src/api/README.md): Links valid: No - link to ../../docs/api/oidc/oidc_endpoints.md not found, link to ../../tests/test_system_endpoints.sh not found, link to ../../oidc-client-examples/ not found

## Folder: /tests

- [README.md](tests/README.md): Links valid: Yes
- [latest_test_results.md](tests/results/latest_test_results.md): Links valid: Yes
- [repository_info.md](tests/results/repository_info.md): Links valid: Yes

## Folder: tests/unity/framework/Unity

- [README.md](tests/unity/framework/Unity/README.md)

## Folder: tests/unity/framework/Unity/docs

- [CODE_OF_CONDUCT.md](tests/unity/framework/Unity/docs/CODE_OF_CONDUCT.md)
- [CONTRIBUTING.md](tests/unity/framework/Unity/docs/CONTRIBUTING.md)
- [MesonGeneratorRunner.md](tests/unity/framework/Unity/docs/MesonGeneratorRunner.md)
- [ThrowTheSwitchCodingStandard.md](tests/unity/framework/Unity/docs/ThrowTheSwitchCodingStandard.md)
- [UnityAssertionsReference.md](tests/unity/framework/Unity/docs/UnityAssertionsReference.md)
- [UnityChangeLog.md](tests/unity/framework/Unity/docs/UnityChangeLog.md)
- [UnityConfigurationGuide.md](tests/unity/framework/Unity/docs/UnityConfigurationGuide.md)
- [UnityGettingStartedGuide.md](tests/unity/framework/Unity/docs/UnityGettingStartedGuide.md)
- [UnityHelperScriptsGuide.md](tests/unity/framework/Unity/docs/UnityHelperScriptsGuide.md)
- [UnityKnownIssues.md](tests/unity/framework/Unity/docs/UnityKnownIssues.md)

## Folder: tests/unity/framework/Unity/extras/bdd

- [readme.md](tests/unity/framework/Unity/extras/bdd/readme.md)

## Folder: tests/unity/framework/Unity/extras/fixture

- [readme.md](tests/unity/framework/Unity/extras/fixture/readme.md)

## Folder: tests/unity/framework/Unity/extras/memory

- [readme.md](tests/unity/framework/Unity/extras/memory/readme.md)