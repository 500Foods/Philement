# hydrogen

Initially, this works very much like Moonraker, providing a WebSocket proxy to the Klipper API. Later, this will also be the component that replaces Klipper itself, but that's going to be a little bit further down the road. 

This is a C program that is essentially a JSON queue manager. Most of what this program does involves moving elements around in different multithreaded and thread-safe queues. Even logging activities use a queue - the first queue created in fact. 

Each queue has its own queue manager that determines what happens when something arrives in its queue. Logging, for example, will look at the queued element and either print it to the console, write it out to a log file, or add it to the database. This is handled separately from whatever generated the log event so *that* process doesn't have to wait for any of these actions to complete before continuing on. 

## Files

- `hydrogen.json` Configuration file for the Hydrogen server.
- `Makefile`  Build instructions for compiling the Hydrogen program.
- `src/beryllium.c` Implements G-code analysis functionality.
- `src/beryllium.h` Header file for G-code analysis declarations.
- `src/configuration.c` Handles loading and managing configuration settings.
- `src/configuration.h` Defines configuration-related structures and constants.
- `src/hydrogen.c` Main entry point of the program, initializes components.
- `src/keys.c` Implements secret key generation functionality.
- `src/keys.h` Header file for secret key generation.
- `src/logging.c` Implements logging functionality.
- `src/logging.h` Defines logging-related functions and macros.
- `src/log_queue_manager.c` Manages the log message queue.
- `src/log_queue_manager.h` Header file for log queue management.
- `src/mdns_server.h` Defines mDNS-related structures and functions.
- `src/mdns_linux.c` Implements mDNS functionality for Linux.
- `src/network.h` Defines network-related structures and functions.
- `src/network_linux.c` Implements network functionality for Linux.
- `src/print_queue_manager.c` Manages the print job queue.
- `src/print_queue_manager.h` Header file for print queue management.
- `src/queue.c` Implements a generic queue data structure.
- `src/queue.h` Defines queue-related structures and functions.
- `src/shutdown.c` Implements graceful shutdown procedures.
- `src/shutdown.h` Header file for shutdown management.
- `src/startup.c` Implements system initialization procedures.
- `src/startup.h` Header file for startup management.
- `src/state.c` Implements system state management.
- `src/state.h` Defines state management structures and functions.
- `src/utils.c` Implements utility functions.
- `src/utils.h` Defines utility functions and constants.
- `src/web_server.c` Implements the web server interface.
- `src/web_server.h` Header file for web server interface.
- `src/websocket_server.c` Implements the websocket server interface.
- `src/websocket_server.h` Header file for websocket server interface.
- `src/websocket_server_status.c` Implementation of WebSocket "status" endpoint

## C Dependencies
- pthreads
- jansson
- microhttpd
  
# Release Notes
## 2025-Feb-12
Worked on various maintenance tasks
- Cleaning up the amount of detail in logging, particularly web sockets (lws)
- Try to have a smoother shutdown, again, particularly around web sockets (lws)
- Fix errors with not being able to identify file size or timestamp
- Bit of refactoring to move startup and shutdown code into their own files

## 2025-Feb-08
Brought into Visual Studio Code.
- Just reviewing where things are at, getting more of a footing when coding inside of VSC/GitHub directly

## 2024-Jul-18
Added a WebSocket server to the project. This is what the web interface (lithium) will be talking to.
- Uses Authorization: Key abc in the header to grant access
- Uses Protocol: hydrogen-protocol, one of the JSON options
- Uses libwebsockets - not for the squeamish.

## 2024-Jul-15
Bit of cleanup of mDNS code. Not sure it is working entirely yet. Some other updates.
- Addressed issues with log_this not being all that reliable
- Fixed memory issues with app_config
- Cleanup of shutdown code
- Added generic WebServer support to serve lithium content
  
## 2024-Jul-11
Focus was all about mDNS and the initial groundwork for the WebSockets interface.
- Integrated the code from the nitrogen/nitro prototype project.
  
## 2024-Jul-08
Updated code to implement an HTTP service that is used to handle incoming print requests, typically from OrcaSlicer. So, in Orca, you can configure the printer connection by specifying a URL and port. It then assumes a bunch of API endpoints, like /api/version or /api/files/upload or something like that. This is what the "test" and "upload" and "upload and print" functions use to communicate with Klipper. In the case of hydrogen, it responds to the requests to get Orca to think we're Octoprint-compatible, enough so that it will send along a print job. At the moment, it does the following.
- Accepts the print job and stores it in a /tmp location with a unique (GUID) filename
- Generates JSON showing the filename mapping, and other data extracted using the beryllium code
- This includes things like layer times, estimated filament, etc.
- This JSON is then added to a PrintQueue, ready to be handled in whatever way we like
- A separate /print/queue endpoint will show all the PrintQueue JSON objects as an array
- This also includes the preview image that is added to the G-code by Orca.
  
