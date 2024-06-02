# Philement
While Klipper has been an enormous boon to the 3D printing community as a whole, it isn't without its faults. Philement has been conceived as a full-on replacement, but it will take quite awhile to get there. In the interim it may end up serving as a front-end to Klipper, a bit like Obico perhaps. The main item to address upfront is that a large chunk of the lower-level stuff has been written in C and specifically not Python. For so many reasons. 

As for the name, it is a combination of terms like Phi (referencing the number 500 among other meanings), filament (that's what we're primarily obsessed with in 3D printing) and element (we're taking things down to the element level, that sort of thinking). The individual components of the project have been named after atomic elements, for example.

## Elements
This project has a number of components, each named after an element in the periodic table.
- hydrogen - A C program that is the basic building block of Philement. This is intended to run on or close to the printer (or perhaps control several printers), like on a Raspberry Pi, typically alongside Klipper. It provides a combination of what Klipper and Moonraker provide, in that it serves up a websocket interface, but provides the low-level control of the printer. In the near-term, this will act more like Moonraker where it interfaces with Klipper, which we would need to do anyway.
- helium - Everything database-related. And there's a lot of database stuff going on. Support for the most popular flavors, naturally - SQLite, Postgres, MySQL/MariaDB, and IBM DB2 just because we can.
- lithium - The UI. This is all CSS/HTML/JS. Talks almost exclusively to the hydrogen websocket interface. 
- beryllium - Deals with everything gcode-related. Analyzing gcode for print times and filament requirements. Macro expansion. Lua support. Things like that.
- boron - Rhymes with Voron! This component is concerned with managing the vast amount of information about printers and components. Like Vorons. Or Troodons.
- carbon - A C program used to compare two images. This is part of a basic print failure detection system that uses timelapse-style images taken between layers to see if an object has been knocked out of position, or if we're dealing with spaghetti, that sort of thing.
- Nitrogen - Think "clapperboard" for 3D printers.  
- Oxygen - Notifications.
- Fluorine - Security stuff.
- Neon - Well, lighting, obviously.
- Sodium - MMU stuff.
- Magnesium - Print farm stuff.

## Additional Notes
While this project is currently under active development, feel free to give it a try and post any issues you encounter.  Or start a discussion if you would like to help steer the project in a particular direction.  Early days yet, so a good time to have your voice heard.  As the project unfolds, additional resources will be made available, including platform binaries, more documentation, demos, and so on.

## Repository Information 
[![Count Lines of Code](https://github.com/500Foods/Template/actions/workflows/main.yml/badge.svg)](https://github.com/500Foods/Template/actions/workflows/main.yml)
<!--CLOC-START -->
```
Last updated at 2024-06-02 01:20:19 UTC
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
Markdown                         4              5              2             41
YAML                             2              8             13             35
-------------------------------------------------------------------------------
SUM:                             6             13             15             76
-------------------------------------------------------------------------------
3 Files (without source code) were skipped
```
<!--CLOC-END-->

## Sponsor / Donate / Support
If you find this work interesting, helpful, or valuable, or that it has saved you time, money, or both, please consider directly supporting these efforts financially via [GitHub Sponsors](https://github.com/sponsors/500Foods) or donating via [Buy Me a Pizza](https://www.buymeacoffee.com/andrewsimard500). Also, check out these other [GitHub Repositories](https://github.com/500Foods?tab=repositories&q=&sort=stargazers) that may interest you.
