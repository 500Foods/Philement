# nitro
This is a C program that takes a couple of parameters and then starts up a websocket server and advertises it over mDNS. 

There are lots of bits and pieces here that make up the core of both hydrogen and lithium projects, potentially others as well.

This is a prototype really, test that it works, doesn't crash, no memory leaks, that sort of thing. And then use it in
other projects by basically duplicating this work or whatever pieces are needed.

## Additional Notes
- Makefile is provided
- Uses jansson for dealing with JSON
- Uses libwebsockets for dealing with websockets
- Native mDNS code (no library used)
- Includes Base64 encode/decode
- Websockets, mDNS announcer, and mdNS responder all in separate threads
- Exits cleanly with Ctrl+C
- No memory leaks detected at this stage
- JWT and Webscokets code are primarily stubs but some bits work
- TXT record in mDNS broadcast has everything hydrogen or HA might need
- Sends goodbye packets so when not running, mDNS caches should be cleared
- Tries really hard to be smart about IP addresses
- Tested with IPv4, but code is there for IPv6 - untested though
  
