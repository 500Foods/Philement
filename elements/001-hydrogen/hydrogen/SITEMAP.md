# SITEMAP.md

This file serves as a sitemap for all Markdown (.md) files in the repository, organized by folder.
It tracks which files have been checked to ensure their internal links (relative links to other .md files) are valid.
The goal is to process one folder at a time, appending results to this file, until all .md files are indexed.

## Instructions for AI Models

Please follow these steps to update this SITEMAP.md file:

1. **Identify an Unindexed Folder**:
   - Scan the repository’s directory structure.
   - Find one folder containing `.md` files that is not yet listed under "Indexed Folders" below.
   - If no unindexed folders remain, stop and report completion.
   - If this is the first run through, please start with the root folder

2. **Process the Folder**:
   - List all `.md` files in the selected folder as a link to the files themselves, with the link including the title from the file if available.
   - For each `.md` file:
     - Extract all internal relative links (e.g., `[text](subfolder/file.md)`, `[text](../file.md)`, `[text](/docs/file.md)`) using Markdown syntax (`[.*?\]\((.*?)\)`).
     - Verify that each relative link points to an existing `.md` file in the repository:
       - Relative links are relative to the file’s folder (e.g., `subfolder/file.md` from `docs/guide.md` points to `docs/subfolder/file.md`).
       - Root-relative links (e.g., `/docs/file.md`) start at the repository root.
     - If a link appears invalid, try and see if you can determine the proper link and update it. Not all links will be fixable.
     - Mark the file as "Links valid: Yes" if all relative links exist, or "Links valid: No" with details (e.g., "link to missing.md not found") if any are broken.

3. **Update SITEMAP.md**:
   - Append a new section to this file under "Indexed Folders" using this format:

     ```markdown
     ## Folder: <folder_path>
     - <file1.md>: Links valid: [Yes/No, details if No]
     - <file2.md>: Links valid: [Yes/No, details if No]
     ```

   - Use the folder’s path relative to the repo root (e.g., `docs`, `src/notes`).

4. **Report Progress**:
   - After updating, provide a summary:
     - Folder processed.
     - Number of `.md` files checked.
     - Any files with broken links.
     - Any unindexed folders remaining (if known).
   - Use DeepSearch mode if needed to resolve complex link paths.

5. **Restart Handling**:
   - If this task is restarted, resume by selecting a new unindexed folder.
   - Do not modify existing "Indexed Folders" sections unless explicitly instructed.

## Indexed Folders

### Folder: /

- [README.md](README.md): Links valid: No - link to oidc-client-examples/README.md not found, link to payload/README.md should be payloads/README.md
- [RECIPE.md](RECIPE.md): Links valid: Yes
- [RELEASES.md](RELEASES.md): Links valid: Yes
- [SECRETS.md](SECRETS.md): Links valid: Yes
- [SITEMAP.md](SITEMAP.md): Links valid: Yes

### Folder: /docs

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

- [2024-07-08.md](docs/releases/2024-07/2024-07-08.md): Links valid: Yes
- [2024-07-11.md](docs/releases/2024-07/2024-07-11.md): Links valid: Yes
- [2024-07-15.md](docs/releases/2024-07/2024-07-15.md): Links valid: Yes
- [2024-07-18.md](docs/releases/2024-07/2024-07-18.md): Links valid: Yes

### Folder: /docs/releases/2025-02

- [2025-02-08.md](docs/releases/2025-02/2025-02-08.md): Links valid: Yes
- [2025-02-12.md](docs/releases/2025-02/2025-02-12.md): Links valid: Yes
- [2025-02-13.md](docs/releases/2025-02/2025-02-13.md): Links valid: Yes
- [2025-02-14.md](docs/releases/2025-02/2025-02-14.md): Links valid: Yes
- [2025-02-15.md](docs/releases/2025-02/2025-02-15.md): Links valid: Yes
- [2025-02-16.md](docs/releases/2025-02/2025-02-16.md): Links valid: Yes
- [2025-02-17.md](docs/releases/2025-02/2025-02-17.md): Links valid: Yes
- [2025-02-18.md](docs/releases/2025-02/2025-02-18.md): Links valid: Yes
- [2025-02-19.md](docs/releases/2025-02/2025-02-19.md): Links valid: Yes
- [2025-02-20.md](docs/releases/2025-02/2025-02-20.md): Links valid: Yes
- [2025-02-21.md](docs/releases/2025-02/2025-02-21.md): Links valid: Yes
- [2025-02-22.md](docs/releases/2025-02/2025-02-22.md): Links valid: Yes
- [2025-02-23.md](docs/releases/2025-02/2025-02-23.md): Links valid: Yes
- [2025-02-24.md](docs/releases/2025-02/2025-02-24.md): Links valid: Yes
- [2025-02-25.md](docs/releases/2025-02/2025-02-25.md): Links valid: Yes
- [2025-02-26.md](docs/releases/2025-02/2025-02-26.md): Links valid: Yes
- [2025-02-27.md](docs/releases/2025-02/2025-02-27.md): Links valid: Yes
- [2025-02-28.md](docs/releases/2025-02/2025-02-28.md): Links valid: Yes

### Folder: /docs/releases/2025-03

- [2025-03-01.md](docs/releases/2025-03/2025-03-01.md): Links valid: Yes
- [2025-03-02.md](docs/releases/2025-03/2025-03-02.md): Links valid: Yes
- [2025-03-03.md](docs/releases/2025-03/2025-03-03.md): Links valid: Yes
- [2025-03-04.md](docs/releases/2025-03/2025-03-04.md): Links valid: Yes
- [2025-03-05.md](docs/releases/2025-03/2025-03-05.md): Links valid: Yes
- [2025-03-06.md](docs/releases/2025-03/2025-03-06.md): Links valid: Yes
- [2025-03-07.md](docs/releases/2025-03/2025-03-07.md): Links valid: Yes
- [2025-03-08.md](docs/releases/2025-03/2025-03-08.md): Links valid: Yes
- [2025-03-09.md](docs/releases/2025-03/2025-03-09.md): Links valid: Yes
- [2025-03-10.md](docs/releases/2025-03/2025-03-10.md): Links valid: Yes
- [2025-03-11.md](docs/releases/2025-03/2025-03-11.md): Links valid: Yes
- [2025-03-12.md](docs/releases/2025-03/2025-03-12.md): Links valid: Yes
- [2025-03-13.md](docs/releases/2025-03/2025-03-13.md): Links valid: Yes
- [2025-03-14.md](docs/releases/2025-03/2025-03-14.md): Links valid: Yes
- [2025-03-15.md](docs/releases/2025-03/2025-03-15.md): Links valid: Yes
- [2025-03-16.md](docs/releases/2025-03/2025-03-16.md): Links valid: Yes
- [2025-03-17.md](docs/releases/2025-03/2025-03-17.md): Links valid: Yes
- [2025-03-18.md](docs/releases/2025-03/2025-03-18.md): Links valid: Yes
- [2025-03-19.md](docs/releases/2025-03/2025-03-19.md): Links valid: Yes
- [2025-03-20.md](docs/releases/2025-03/2025-03-20.md): Links valid: Yes
- [2025-03-21.md](docs/releases/2025-03/2025-03-21.md): Links valid: Yes
- [2025-03-22.md](docs/releases/2025-03/2025-03-22.md): Links valid: Yes
- [2025-03-23.md](docs/releases/2025-03/2025-03-23.md): Links valid: Yes
- [2025-03-24.md](docs/releases/2025-03/2025-03-24.md): Links valid: Yes
- [2025-03-25.md](docs/releases/2025-03/2025-03-25.md): Links valid: Yes
- [2025-03-26.md](docs/releases/2025-03/2025-03-26.md): Links valid: Yes
- [2025-03-27.md](docs/releases/2025-03/2025-03-27.md): Links valid: Yes
- [2025-03-28.md](docs/releases/2025-03/2025-03-28.md): Links valid: Yes
- [2025-03-29.md](docs/releases/2025-03/2025-03-29.md): Links valid: Yes
- [2025-03-30.md](docs/releases/2025-03/2025-03-30.md): Links valid: Yes
- [2025-03-31.md](docs/releases/2025-03/2025-03-31.md): Links valid: Yes

### Folder: /docs/releases/2025-04

- [2025-04-01.md](docs/releases/2025-04/2025-04-01.md): Links valid: Yes
- [2025-04-02.md](docs/releases/2025-04/2025-04-02.md): Links valid: Yes
- [2025-04-03.md](docs/releases/2025-04/2025-04-03.md): Links valid: Yes
- [2025-04-04.md](docs/releases/2025-04/2025-04-04.md): Links valid: Yes
- [2025-04-05.md](docs/releases/2025-04/2025-04-05.md): Links valid: Yes

### Folder: /examples

- [README.md](examples/README.md): Links valid: Yes

### Folder: /payloads

- [README.md](payloads/README.md): Links valid: No - link to ./payload/README.md not found (payload subdirectory does not exist)

### Folder: /src/api

- [README.md](src/api/README.md): Links valid: No - link to ../../docs/api/oidc/oidc_endpoints.md not found, link to ../../tests/test_system_endpoints.sh not found, link to ../../oidc-client-examples/ not found

### Folder: /tests

- [README.md](tests/README.md): Links valid: Yes
- [latest_test_results.md](tests/results/latest_test_results.md): Links valid: Yes
- [repository_info.md](tests/results/repository_info.md): Links valid: Yes
