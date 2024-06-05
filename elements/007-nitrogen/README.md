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
