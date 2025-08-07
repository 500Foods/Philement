# hydrogen
The innermost core of the Philement project is hydrogen. This is roughly analogous to the combination of Klipper and Moonraker, which are soon to be merged as well. The basic purpose is to have a service running as close to the hardware as possible. This might mean that it is running on a Raspberry Pi located physically inside a printer. Or wired up via CAN bus to a series of printers in an external enclosure. Or just running on a PC in the same network. But, like Klipper, this is the module that takes in G-code (or potentially other alternatives) and then directs one or more 3D printers or related hardware based on those instructions.

## Brainstorming
At the outset of this project, there are a number of thoughts about what kinds of things we'd like this element to offer.

- C-based application that can be compiled with GCC on virtually any platform.
- GitHub Actions set up to build binaries automatically for platforms like Windows/macOS/Linux and so on.
- Use WebSockets as its primary interface.
- Single executable file to deal with, mostly self-contained except for a database.
- Able to talk to other printers (Klipper primarily) or other hydrogen instances.
- Intended to be run as a service (or perhaps several services) on whatever system it is running on.
- Can pass it G-code or alternatives and it can analyze those, track progress through the print, and so on.
- Multithreaded, at least one thread per printer, that sort of thing.
- Various logging interfaces.
- Aside from G-code, primarily communicates using JSON.
- Scripting support, likely with Lua. TBD.
- Optimized for speed and minimal resource usage.
- This is likely what the slicer will be talking to.

## Prototyping: [deuterium](https://github.com/500Foods/Philement/tree/main/elements/001-hydrogen/deuterium) - Klipper API Access
This is a small self-contained test project that runs locally alongside Klipper, much like Moonraker. It talks to Klipper over a unix socket connection and outputs what it gets back to the console. 
This is intended as a test of communicating with Klipper where the bulk of the effort is actually in reading the various configuration files to figure out where Klipper is running in the first place. 
A version in both Python and C is provided.

## Analysis: [tritium](https://github.com/500Foods/Philement/tree/main/elements/001-hydrogen/tritium) - Testing Analysis
Over a short period of time, the Test Suite used to build the hydrogen project has evolved into a bit of a work of art all its own. One of the attributes that makes it potentially interesting is in its
level of optimization. To help with that, the tritium project is used as a sidecar of sorts, running alongside the Test Suite gathering analytics about its processes, memory, and CPU usage to help 
identify bottlenecks and general information about how it is working. As an extensive collection of Bash scripts, numerous forks are done - thousands and thousands - so every effort is made to try and
limit those as much as possible. This is a tool to help understand more about exactlyl that.

## [hydrogen](https://github.com/500Foods/Philement/blob/main/elements/001-hydrogen/hydrogen)
This is the main course. A C-based server that is intended to run in close proximity to the printer, typically on a Raspberry Pi or equivalent. Initially as a back-end proxy for Klipper, not unlike Moonraker, it needs to be running on the same system to be able to connect to the (default) Unix socket that Klipper offers. 

At a high level, this is a program architected around a collection of thread-safe JSON queues. Something happens. A JSON object might get tossed onto a queue. A queue manager might pull some JSON off of a queue. There's a connection to Klipper. There's a WebSocket server. Everything has its own thread. 



