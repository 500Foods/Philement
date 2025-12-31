# [Philement](https://www.philement.com)

While Klipper has been an enormous boon to the 3D printing community as a whole, it isn't without its faults. Philement has been conceived as a full-on replacement, but it will take quite awhile to get there. In the interim it may end up serving as a front-end to Klipper, a bit like Obico perhaps.

The main differentiator to address upfront is that a large chunk of the lower-level code has been written in C and specifically not Python. For so many reasons. To help make this go a little more quickly, various AI engines are being tasked to help out. It isn't written by AI, but written with the help of AI. If that distinction matters to anyone. If nothing else, this gives a degree of plausible deniability if there's something crazy going on in the code.

As for the name, it is a combination of terms like 'Phi' (referencing the number 500 among *many* other meanings), 'filament' (that's what 3D printing is primarily obsessed with), and 'element' (small pieces of something larger). The individual components of the project have been named after atomic elements, for example.

As far as progress reporting goes, well, there are likely 500 steps or more to be completed before anyone takes this project seriously. Some of those steps will yield useful tools that people can use, as is already the case. But it is an ambitious project with many items to complete, and many more that likely haven't even been added to the list yet.
<br/><br/><img src="https://progressbar-guibranco.vercel.app/49/?scale=500&title=%20Completed%20&width=415&suffix=%20%2F%20500%20Steps" alt="Completed 49 / 500 Steps">

## Elements

This project has a number of, well, elements. Each is named after an element in the periodic table. Like elements in the real world, some of these will be hugely important while others may be relatively insignificant. Some will require thousands and thousands of developer hours and others not so much. And the effort applied to each will likely be equally unbalanced as the focus shifts among the different pieces needed to get this up and running.

| Element | Status | Description |
|:-------:|:------:|:------------|
| [hydrogen](https://github.com/500Foods/Philement/tree/main/elements/001-hydrogen/README.md) | 🔨 | A websocket-equipped service, like Klipper+Moonraker combined. <tr></tr> |
| [helium](https://github.com/500Foods/Philement/tree/main/elements/002-helium/README.md) | 🔨 | Everything database-related. <tr></tr> |
| [lithium](https://github.com/500Foods/Philement/tree/main/elements/003-lithium/README.md) | 💡 | Web-based UI for desktops and larger systems.  <tr></tr> |
| [beryllium](https://github.com/500Foods/Philement/tree/main/elements/004-beryllium/README.md) | 🏆 | Deals with everything gcode-related. <tr></tr> |
| [boron](https://github.com/500Foods/Philement/tree/main/elements/005-boron/README.md) | 💡 | Rhymes with Voron! Hardware database. Like Vorons. Or [Troodons](https://github.com/500Foods/WelcomeToTroodon). <tr></tr> |
| [carbon](https://github.com/500Foods/Philement/tree/main/elements/006-carbon/README.md) | 🏆 | Print fault detection, a bit like what Obico is for. <tr></tr> |
| [nitrogen](https://github.com/500Foods/Philement/tree/main/elements/007-nitrogen/README.md) | 🔨 | LVGL-based UI for controllers and smaller systems. <tr></tr> |
| [oxygen](https://github.com/500Foods/Philement/tree/main/elements/008-oxygen/README.md) | 💡 | Notifications. <tr></tr> |
| [fluorine](https://github.com/500Foods/Philement/tree/main/elements/009-fluorine/README.md) | 💡 | Filament management system. <tr></tr> |
| [neon](https://github.com/500Foods/Philement/tree/main/elements/010-neon/README.md) | 💡 | Well, lighting, obviously. <tr></tr> |
| [sodium](https://github.com/500Foods/Philement/tree/main/elements/011-sodium/README.md) | 💡 | An MMU and general MMU support. <tr></tr> |
| [magnesium](https://github.com/500Foods/Philement/tree/main/elements/012-magnesium/README.md) | 💡 | Print farm management tool. <tr></tr> |
| [aluminum](https://github.com/500Foods/Philement/tree/main/elements/013-aluminum/README.md) | 💡 | Home Assistant integration. <tr></tr> |
| [silicon](https://github.com/500Foods/Philement/tree/main/elements/014-silicon/README.md) | 💡 | Printer experiment - Voron 2.4r2 without an MCU. <tr></tr> |
| [phosphorus](https://github.com/500Foods/Philement/tree/main/elements/015-phosphorus/README.md) | 💡 | Printer experiment - Beltless printer. <tr></tr> |
| [sulfur](https://github.com/500Foods/Philement/tree/main/elements/016-sulfur/README.md) | 💡 | Printer experiment - Robotic arm printer. <tr></tr> |
| [chlorine](https://github.com/500Foods/Philement/tree/main/elements/017-chlorine/README.md) | 🔨 | TMS WEB Core project for the <www.philement.com> website. <tr></tr> |
| [argon](https://github.com/500Foods/Philement/tree/main/elements/018-argon/README.md) | 💡 | Filament extruder - recycle that waste plastic! <tr></tr> |
| [potassium](https://github.com/500Foods/Philement/tree/main/elements/019-potassium/README.md) | 💡 | Power monitoring <tr></tr> |
| [calcium](https://github.com/500Foods/Philement/tree/main/elements/020-calcium/README.md) | 💡 | Optimization Wizard - building on beryllium and boron <tr></tr> |
| [scandium](https://github.com/500Foods/Philement/tree/main/elements/021-scandium/README.md) | 💡 | Implementation of x3dp.com - 3D Printer Exchange <tr></tr> |
| [titanium](https://github.com/500Foods/Philement/tree/main/elements/022-titanium/README.md) | 💡 | High-performance video streaming for remote monitoring <tr></tr> |
| [vanadium](https://github.com/500Foods/Philement/tree/main/elements/023-vanadium/README.md) | 🏆 | Custom font for Philement based off of Iosevka <tr></tr> |

<sup>💡 → Idea and Planning Stage &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; 🔨 → Working on it &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; 🏆 → Nowhere near done but... Check it out!</sup>

## Additional Notes

While this project is currently under active development, feel free to give it a try and post any issues you encounter.  Or start a discussion if you would like to help steer the project in a particular direction.  Early days yet, so a good time to have your voice heard.  As the project unfolds, additional resources will be made available, including platform binaries, more documentation, demos, and so on.

## Development Preferences

There are countless tools, frameworks, coding styles, conventions, languages, and so on, that are readily available out in the world today. A big part of working on any project is selecting a suite of tools that can surface the best code in the least amount of time for the lowest cost given a particular pool of developer talent. When the number of developers is small, tool selection is often subject to the whims of the developers. This project is no exception.

### Working with C

- Uses GCC wherever possible, written to C17, usually with _GNU_SOURCE included as well.
- Should have a simple Makefile that can be run with 'make', 'make clean', and so on.
- Code should be tested on Linux/X86, Linux/x64, Linux/Arm, Windows/X64, macOS/x64, macOS/Arm.
- Code is often written with the help of AI LLMs and at a very low level to reduce dependencies.
- Try to avoid the way modern JS and Python projects are built with thousands of dependencies.
- Try to reuse code between projects where it makes sense, such as some of the network code.
- Prefer JSON rather than YAML or other file formats.

### Working with HTML/CSS/JS

- Most of this has been done with TMS WEB Core and Delphi. It generates 100% HTML/CSS/JS.
- Should be tested on Firefox/Chrome/Safari on Linux/Windows/macOS, desktop and mobile.
- As these are web pages, they *should* run everywhere, not deliberately excluding anyone.
- Designed to be self-hosted without making any changes other than to configuration files.
- Try to stick to the same set of JavaScript libraries. Bootstrap. Tabulator. Etc.
- Prefer JSON rather than YAML or other file formats.

## Repository Information

[![Count Lines of Code](https://github.com/500Foods/Philement/actions/workflows/main.yml/badge.svg)](https://github.com/500Foods/Philement/actions/workflows/main.yml)

NOTE: Please refer to individual projects for a more nuanced breakdown.
The Hydrogen project, for example, shows the lines of C code grouped into core project code and unit testing code, and combines C and C header files into the same row, along with providing additional statistics.
<!--CLOC-START -->
```cloc
Last updated at 2025-12-31 14:17:23 UTC
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
JSON                           148             96              0         382240
C                              875          35916          34319         126099
SVG                            116            224           2862         110046
Markdown                       486          13961             64          39863
Text                            94             95              0          33150
Bourne Shell                   104           4965          71123          23422
Lua                            136           3141           2867          21702
C/C++ Header                   184           2015           5227           5891
JavaScript                      31           1049           1164           4857
CSS                             10            303            140           2007
HTML                            15            174             64           1231
CMake                           17            171            318           1153
make                             9             95             60            517
TOML                             1             55             35            449
Python                           1             36              9            195
SQL                              9             14             13            174
zsh                              2             31             42             97
Delphi Form                      1              1              0             43
YAML                             2              8             13             37
Pascal                           2             11              2             31
-------------------------------------------------------------------------------
SUM:                          2243          62361         118322         753204
-------------------------------------------------------------------------------
513 Files were skipped (duplicate, binary, or without source code):
  svg: 386
  ttf: 26
  lua: 12
  sh: 9
  md: 7
  css: 6
  png: 6
  woff2: 6
  gitignore: 5
  json: 4
  ico: 3
  jpg: 3
  lintignore-markdown: 3
  clp: 2
  lintignore-bash: 2
  lintignore-c: 2
  lintignore-lua: 2
  lintignore: 2
  sqruff_db2: 2
  sqruff_mysql: 2
  sqruff_postgresql: 2
  sqruff_sqlite: 2
  3mf: 1
  ansi: 1
  backup: 1
  br: 1
  control: 1
  detailed: 1
  disabled: 1
  dproj: 1
  gcode: 1
  ggignore: 1
  gitattributes: 1
  html: 1
  payload_generated: 1
  sqlite: 1
  stl: 1
  stylelintcache: 1
  stylelintrc: 1
  supp: 1
  trial-ignore: 1
```
<!--CLOC-END-->

## Sponsor / Donate / Support

If you find this work interesting, helpful, or valuable, or that it has saved you time, money, or both, please consider directly supporting these efforts financially via [GitHub Sponsors](https://github.com/sponsors/500Foods) or donating via [Buy Me a Pizza](https://www.buymeacoffee.com/andrewsimard500). Also, check out these other [GitHub Repositories](https://github.com/500Foods?tab=repositories&q=&sort=stargazers) that may interest you.
