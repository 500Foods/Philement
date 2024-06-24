# hydrogen
The innermost core of the Philement project is hydrogen. This is roughly analogous to the combination of Klipper and Moonraker, which are soon to be merged as well. The basic purpose is to have a service running as close to the hardware as possible. This might mean that it is running on a Raspberry Pi located physically inside a printer. Or wired up via CAN bus to a series of printers in an external enclosure. Or just running on a PC in the same network. But, like Klipper, this is the module that takes in G-code (or potentially other alternatives) and then directs one or more 3D printers or related hardware based on those instructions.

## Brainstorming
At the outset of this project, there are a number of thoughts about what kinds of things we'd like this element to offer.

- C-based application that can be compiled with GCC on virtually any platform.
- GitHub Actions set up to build binaries automatically for platforms like Windows/macOS/Linux and so on.
- Use websockets as its primary interface.
- Single executable file to deal with, mostly self-contained except for a database.
- Able to talk to other printers (Klipper primarily) or other hydrogen instances.
- Intended to be run as a service (or perhaps several services) on whatever system it is running on.
- Can pass it G-code or alternatives and it can.
- Multithreaded, at least one thread per printer, that sort of thing.
- Various logging interfaces.
- Aside from G-code, primarily communicates using JSON.
- Scripting support with Lua. TBD.
- Optimized for speed and minimal resource usage.
- This is likely what the slicer will be talking to.

## Prototyping: klippertest - Klipper API Access
This is a small self-contained test project that runs locally alongside Klipper, much like Moonraker. It talks to Klipper over a unix socket connection and outputs what it gets back to the console. 
This is intended as a test of communicating with Klipper where the bulk of the effort is actually in reading the various configuration files to figure out where Klipper is running in the first place. 
A version in both Python and C is provided.
