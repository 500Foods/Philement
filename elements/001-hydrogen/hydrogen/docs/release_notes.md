# Release Notes

## 2025-Feb-20

Major WebSocket server refactoring:

- Split monolithic implementation into focused modules:
  - Connection lifecycle management
  - Authentication handling
  - Message processing
  - Event dispatching
  - Status reporting
- Improved state management with WebSocketServerContext
- Enhanced initialization sequence and error handling
- Added port fallback mechanism for better reliability
- Fixed session validation during vhost creation
- Improved thread safety with proper mutex protection
- Better error reporting and logging throughout

## 2025-Feb-19

Enhanced system status reporting:

- Added server timing information to /api/system/info endpoint
- New `server_started` field shows ISO-formatted UTC start time
- New `server_runtime` field shows uptime in seconds
- New `server_runtime_formatted` field shows human-readable uptime
- Updated API documentation with real-world examples

Documentation improvements:

- Added link to Print Queue Management documentation
- Moved release notes to a dedicated file
- Enhanced System Dependencies section with detailed descriptions
- Improved documentation organization and cross-linking

## 2025-Feb-18

Extended IPv6 support to web and websocket servers:

- Added EnableIPv6 configuration flags for web and websocket servers
- Implemented dual-stack support in web server
- Added IPv6 interface binding in websocket server
- All network services (mDNS, web, websocket) now have consistent IPv6 configuration

## 2025-Feb-17

Lots of configuration and service documentation.
Revisited how thread and process memory is reported.
Example of Bash script that does the same thing.
File Descriptor information added to info endpoint.

## 2025-Feb-16

Work on comments, trying to make things a little easier to follow.
Added IPv6Enable flag to JSON configuration for mDNS.
Updated logging output to look the same regardless of how logging is performed

## 2025-Feb-15

Started work on documentation for project and added example code for the status API endpoint
which is now also synchronized with the WebSockets output showing the same information.

Configuration options to control servers independently. We might run hydrogen just as a generic
web server, or a generic websockets server or a print server without needing all of the servers
running, so this ideally allows us to control them independently.

- JSON updated with Enabled flags for each server
- PrintServer added to JSON (wasn't there previously)
- Startup and Shutdown functions updated to check for these flags
- mDNS, if started, should advertise only what has been started

## 2025-Feb-14

Basic REST API Implemented

- SystemService/Info Endpoint added
- Logging API calls added

## 2025-Feb-13

More maintenance

- Reviewed code and comments in hydrogen.c
- Reviewed code and comments in Makefile

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
