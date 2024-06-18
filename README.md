# [Philement](https://www.philement.com)
While Klipper has been an enormous boon to the 3D printing community as a whole, it isn't without its faults. Philement has been conceived as a full-on replacement, but it will take quite awhile to get there. In the interim it may end up serving as a front-end to Klipper, a bit like Obico perhaps. 

The main differentiator to address upfront is that a large chunk of the lower-level code has been written in C and specifically not Python. For so many reasons. To help make this go a little more quickly, various AI engines are being tasked to help out. It isn't written by AI, but written with the help of AI. If that distinction matters to anyone. If nothing else, this gives a degree of plausible deniability if there's something crazy going on in the code.

As for the name, it is a combination of terms like 'Phi' (referencing the number 500 among *many* other meanings), 'filament' (that's what 3D printing is primarily obsessed with), and 'element' (small pieces of something larger). The individual components of the project have been named after atomic elements, for example.

## Elements
This project has a number of, well, elements. Each is named after an element in the periodic table. Like elements in the real world, some of these will be hugely important while others may be relatively insignificant. Some will require thousands and thousands of developer hours and others not so much. And the effort applied to each will likely be equally unbalanced as the focus shifts among the different pieces needed to get this up and running.

| Element | Status | Description |
|:-------:|:------:|:------------|
| [hydrogen](https://github.com/500Foods/Philement/tree/main/elements/001-hydrogen/README.md) | 🔨 | A websocket-equipped service, like Klipper+Moonraker combined. <tr></tr> |
| [helium](https://github.com/500Foods/Philement/tree/main/elements/002-helium/README.md) | 💡 | Everything database-related. <tr></tr> |
| [lithium](https://github.com/500Foods/Philement/tree/main/elements/003-lithium/README.md) | 💡 | Web-based UI for desktops and larger systems.  <tr></tr> |
| [beryllium](https://github.com/500Foods/Philement/tree/main/elements/004-beryllium/README.md) | 🏆 | Deals with everything gcode-related. <tr></tr> |
| [boron](https://github.com/500Foods/Philement/tree/main/elements/005-boron/README.md) | 💡 | Rhymes with Voron! Hardware database. Like Vorons. Or [Troodons](https://github.com/500Foods/WelcomeToTroodon). <tr></tr> |
| [carbon](https://github.com/500Foods/Philement/tree/main/elements/006-carbon/README.md) | 🏆 | Print fault detection, a bit like what Obico is for. <tr></tr> |
| [nitrogen](https://github.com/500Foods/Philement/tree/main/elements/007-nitrogen/README.md) | 🔨 | LVGL-based UI for controllers and smaller systems. <tr></tr> |
| [oxygen](https://github.com/500F1oods/Philement/tree/main/elements/008-oxygen/README.md) | 💡 | Notifications. <tr></tr> |
| [fluorine](https://github.com/500Foods/Philement/tree/main/elements/009-fluorine/README.md) | 💡 | Filament management system. <tr></tr> |
| [neon](https://github.com/500Foods/Philement/tree/main/elements/010-neon/README.md) | 💡 | Well, lighting, obviously. <tr></tr> |
| [sodium](https://github.com/500Foods/Philement/tree/main/elements/011-sodium/README.md) | 💡 | An MMU and MMU support. <tr></tr> |
| [magnesium](https://github.com/500Foods/Philement/tree/main/elements/012-magnesium/README.md) | 💡 | Print farm management tool. <tr></tr> || 
| [aluminum](https://github.com/500Foods/Philement/tree/main/elements/013-aluminum/README.md) | 💡 | Home Assistant integration. <tr></tr> || 
| [silicon](https://github.com/500Foods/Philement/tree/main/elements/014-silicon/README.md) | 💡 | Printer experiment - Voron 2.4r2 without an MCU. <tr></tr> || 
| [phosphorus](https://github.com/500Foods/Philement/tree/main/elements/015-phosphorus/README.md) | 💡 | Printer experiment - Beltless printer. <tr></tr> || 
| [sulfur](https://github.com/500Foods/Philement/tree/main/elements/016-sulfur/README.md) | 💡 | Printer experiment - Robotic arm printer. <tr></tr> || 
| [chlorine](https://github.com/500Foods/Philement/tree/main/elements/017-chlorine/README.md) | 🔨 | The www.philement.com website. <tr></tr> || 

<sup>💡 → Idea and Planning Stage &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; 🔨 → Working on it &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; 🏆 → Check it out!</sup>

## Additional Notes
While this project is currently under active development, feel free to give it a try and post any issues you encounter.  Or start a discussion if you would like to help steer the project in a particular direction.  Early days yet, so a good time to have your voice heard.  As the project unfolds, additional resources will be made available, including platform binaries, more documentation, demos, and so on.

## Development Preferences
There are countless tools, frameworks, coding styles, conventions, languages, and so on, that are readily available out in the world today. A big part of working on any project is selecting a suite of tools that can surface the best code in the least amount of time for the lowest cost given a particular pool of developer talent. When the number of developers is small, tool selection is often subject to the whims of the developers. This project is no exception.

### C Coding
- Uses GCC wherever possible, written to C17, usually with _GNU_SOURCE included as well, for things like usleep().
- Should have a simple Makefile that can be run with 'make', 'make clean' and so on.
- Code should be tested on Linux/X86, Linux/x64, Windows/X64, macOS/Intel, macOS/Arm, RaspberryPi, and where appropriate RP2040.
- Code is often written with the help of AI LLMs, generally at a very low level to reduce dependencies on obscure third-party libraries.
- This is a conscious design choice as a lot of the code is designed specifically to be performant and, well, simple.
- Try to avoid the way modern JS and Python projects are built with hundreds or even thousands of dependencies.
- Try to reuse code between projects where it makes sense, such as some of the network code.
- Prefer JSON rather than YAML or other file formats.

### Web Coding
- Most of this has been done with TMS WEB Core and Delphi. Think of it like TypeScript. It generates 100% HTML/CSS/JS.
- Should be tested on Firefox/Chrome/Safari on Linux/Windows/macOS. As these are web pages, they *should* run everywhere.
- Designed with the idea that any code can be self-hosted without making any changes other than to configuration files.
- Try to limit the number of third-party JavaScript libraries.
- Prefer JSON rather than YAML or other file formats.

## Repository Information 
[![Count Lines of Code](https://github.com/500Foods/Template/actions/workflows/main.yml/badge.svg)](https://github.com/500Foods/Template/actions/workflows/main.yml)
<!--CLOC-START -->
```
Last updated at 2024-06-18 07:56:20 UTC
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
C                               13            499             87           2573
Markdown                        22            128              2           1235
C/C++ Header                     9             53              9            172
Bourne Shell                     3             28             56             79
Text                             1              0              0             75
make                             2             23             15             39
YAML                             2              8             13             35
-------------------------------------------------------------------------------
SUM:                            52            739            182           4208
-------------------------------------------------------------------------------
6 Files (without source code) were skipped
```
<!--CLOC-END-->

## Sponsor / Donate / Support
If you find this work interesting, helpful, or valuable, or that it has saved you time, money, or both, please consider directly supporting these efforts financially via [GitHub Sponsors](https://github.com/sponsors/500Foods) or donating via [Buy Me a Pizza](https://www.buymeacoffee.com/andrewsimard500). Also, check out these other [GitHub Repositories](https://github.com/500Foods?tab=repositories&q=&sort=stargazers) that may interest you.
