# hydrogen

Initially, this works very much like Moonraker, providing a WebSocket proxy to the Klipper API. Later, this will also be the component that replaces Klipper itself, but that's going to be a little bit further down the road. 

This is a C program that is essentially a JSON queue manager. Most of what this program does involves moving elements around in different multithreaded and thread-safe queues. Even logging activities use a queue - the first queue created in fact. 

Each queue has its own queue manager that determines what happens when something arrives in its queue. Logging, for example, will look at the queued element and either print it to the console, write it out to a log file, or add it to the database. This is handled separately from whatever generated the log event so *that* process doesn't have to wait for any of these actions to complete before continuing on. 

## C Dependencies
- pthreads
- jansson
- microhttpd
  
## Release Notes
### 2024-Jul-09
Focus was all about mDNS and the initial groundwork for the WebSockets interface.
- Integrated the code from the nitrogen/nitro prototype project.
  
### 2024-Jul-08
Updated code to implement an HTTP service that is used to handle incoming print requests, typically from OrcaSlicer. So, in Orca, you can configure the printer connection by specifying a URL and port. It then assumes a bunch of API endpoints, like /api/version or /api/files/upload or something like that. This is what the "test" and "upload" and "upload and print" functions use to communicate with Klipper. In the case of hydrogen, it responds to the requests to get Orca to think we're Octoprint-compatible, enough so that it will send along a print job. At the moment, it does the following.
- Accepts the print job and stores it in a /tmp location with a unique (GUID) filename
- Generates JSON showing the filename mapping, and other data extracted using the beryllium code
- This includes things like layer times, estimated filament, etc.
- This JSON is then added to a PrintQueue, ready to be handled in whatever way we like
- A separate /print/queue endpoint will show all the PrintQueue JSON objects as an array
- This also includes the preview image that is added to the G-code by Orca.
  
