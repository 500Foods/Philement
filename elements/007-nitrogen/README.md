# nitrogen
While Lithium is the primary UI, there's an interest in additional UIs. Maybe for display-only, but likely UIs that we can interact with.

## Brainstorming
- Replacement for KlipperScreen. Lots of room for improvement!
- Angling for something like Knomi, or maybe even support for Knomi hardware.
- A "clapperboard" interface - a small touch display with current printer statistics. Like Knomi but 4-inch display.
- This device can also be equipped with sensors, particularly a VOC sensor.
- Another one can be placed on your desk.
- A set can be used to monitor a print farm.
- Adafruit/Feather/WiFi/Can bus/Arduino project is the current thought process.
- Use the same tooling for the MMU project.
- An actual product that could be made for sale.
- Maybe develop a production version rather than the Adafruit feather implementation.
- RP2040 support.
- Seems like LVGL is the way to go here.
- Supports all the above platforms plus as a bonus it can spit out WebASM code!
- Native C library that supports all the things we'd like it to support.

## Prototyping: [nitro - mDNS](https://github.com/500Foods/Philement/blob/main/elements/007-nitrogen/nitro/README.md)
This is a small self-contained test project that implements an mDNS advertising mechanism. 
The various nitrogen elements (whether just one to replace KlipperScreen, or an army of them across a print farm) can advertise their IP address and other attributes.
This allows for an element like hydrogen to pick them up without having to do any configuration. It also makes it possible to tie them to Home Assistant more easily.
This code will likely find its way into both nitrogen and hydrogen elements, as hydrogen itself might want to broadcast its location as well.

## Prototyping: [netscan - Network Discovery](https://github.com/500Foods/Philement/blob/main/elements/007-nitrogen/netscan/README.md)
This is a small self-contained test project that takes an inventory of local network resources. First, it itemizes the available local network interfaces and then for
wireless interfaces, it displays all the available SSIDs. This is likely to be incorporated into nitrogen elements. Imagine starting the equivalent of KlipperScreen
on a device that has not yet been connected to a network. This kind of information is used to feed the UI so the user can select a network interface to configure
and in the case of WiFi, select an SSID for the connection. 

## Prototyping: lvgldemo
This is a small self-contained test project that uses LVGL to display a gradient background and some text. Nothing too fancy, just enough to demonstrate that LVGL 
is working in our code, and can display something non-trivial. Well, it's trivial but a white screen or a black screen could be displayed with all the driver stuff
horribly wrong.
