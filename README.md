# [Philement](https://www.philement.com)

Philement is a family of small pieces that are supposed to add up to a machine you can run locally and then forget about.

The first piece is [Hydrogen](/docs/H/README.md): a compact, multithreaded C process that already knows how to be a web server, a websocket host, an identity provider, a database client, a mail and print queue, and an MCP endpoint. It is meant to be generic. Print farms are one use. So are other appliances that need a sturdy host and do not want a cloud account or a sidecar for every concern.

Extensibility is pushed where it belongs. Almost everything you will want to add is Lua, against a sandboxed host API, or it is data in the database layer. There is no plugin folder and no `pip install` into the host. On the rare day the host is actually missing a primitive — MCP was one of those — you change the C. That code is structured, documented, and tested on purpose, so changing it is a normal patch, not a hack bolted on the side.

If the only thing this project accomplishes is a C host with Lua on top instead of a Python one, that is already a service. The process stays small. Your logic stays in the pot that cannot take the box down with a bad extra.

3D printing is the long job that named the project and still sets the roadmap. Philement was aimed at the role Klipper and Moonraker occupy on a printer computer: motion and a service layer, without making that computer into a Python distribution. Getting all the way there will take a while. Along the way the same core is already usable for anything else that needs a local, scriptable appliance runtime.

The name is Phi (500, among other things), filament, and element. The pieces are named after the periodic table for the same reason: a few of them do almost all the work; the rest are specific.

As far as progress reporting goes, well, there are likely 500 steps or more to be completed before anyone takes this project seriously. Some of those steps will yield useful tools, as is already the case. But it is an ambitious project with many items to complete, and many more that likely haven't even been conceived of yet. 
<br/><br/><img src="https://progressbar-guibranco.vercel.app/150/?scale=500&title=%20Completed%20&width=415&suffix=%20%2F%20500%20Steps" alt="Completed 149 / 500 Steps">

## Elements

This project has a number of, well, elements. Each is named after an element in the periodic table. Like elements in the real world, some of these will be hugely important while others may be relatively insignificant. Some will require thousands and thousands of developer hours, and others not so much. And the effort applied to each will likely be equally unbalanced as the focus shifts among the different pieces needed to get this up and running. The main [Philement Documentation](/docs/README.md) index is also available, covering elements that are a little further along.

| Element | Status | Description |
| :-------: | :------: | :------------ |
| [hydrogen](/docs/H/README.md) | 🏆 | A websocket-equipped service, like Klipper+Moonraker combined |
| [helium](/docs/He/README.md) | 🔨 | Everything database-related |
| [lithium](/docs/Li/README.md) | 🔨 | Web-based UI for desktops and larger systems |
| [beryllium](/elements/004-beryllium/README.md) | 🏆 | Deals with everything gcode-related |
| [boron](/elements/005-boron/README.md) | 💡 | Rhymes with Voron! Hardware database, like for Vorons or [Troodons](https://github.com/500Foods/WelcomeToTroodon) |
| [carbon](/elements/006-carbon/README.md) | 🏆 | Print fault detection, a bit like what Obico is for |
| [nitrogen](/elements/007-nitrogen/README.md) | 🔨 | LVGL-based UI for controllers and smaller systems |
| [oxygen](/elements/008-oxygen/README.md) | 💡 | Notifications |
| [fluorine](/elements/009-fluorine/README.md) | 💡 | Filament management system |
| [neon](/elements/010-neon/README.md) | 💡 | Well, lighting, obviously |
| [sodium](/elements/011-sodium/README.md) | 💡 | An MMU and general MMU support |
| [magnesium](/elements/012-magnesium/README.md) | 💡 | Print farm management tool |
| [aluminum](/elements/013-aluminum/README.md) | 💡 | Home Assistant integration |
| [silicon](/elements/014-silicon/README.md) | 💡 | Printer experiment - Voron 2.4r2 without an MCU |
| [phosphorus](/elements/015-phosphorus/README.md) | 💡 | Printer experiment - Beltless printer |
| [sulfur](/elements/016-sulfur/README.md) | 💡 | Printer experiment - Robotic arm printer |
| [chlorine](/elements/017-chlorine/README.md) | 🔨 | [www.philement.com](https://www.philement.com) website source |
| [argon](/elements/018-argon/README.md) | 💡 | Filament extruder - recycle that waste plastic! |
| [potassium](/elements/019-potassium/README.md) | 💡 | Power monitoring |
| [calcium](/elements/020-calcium/README.md) | 💡 | Optimization Wizard - building on beryllium and boron |
| [scandium](/elements/021-scandium/README.md) | 💡 | Implementation of x3dp.com - 3D Printer Exchange |
| [titanium](/elements/022-titanium/README.md) | 💡 | High-performance video streaming for remote monitoring |
| [vanadium](/elements/023-vanadium/README.md) | 🏆 | Custom font for Philement based off of Iosevka |
| [chromium](/elements/024-chromium/README.md) | 💡 | We're going to skip this one! |
| [scandium](/elements/025-manganese/README.md) | 💡 | Lua support layer |
| [iron](/elements/026-iron/README.md) | 💡 | Lua implementation of, well, a motion control system |

<sup>💡 → Idea and Planning Stage &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; 🔨 → Working on it &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; 🏆 → Nowhere near done but... Check it out!</sup>

## Additional Notes

While this project is currently under active development, feel free to give it a try and post any issues you encounter.  Or start a discussion if you would like to help steer the project in a particular direction.  Early days yet, so a good time to have your voice heard.  As the project unfolds, additional resources will be made available, including platform binaries, more documentation, demos, and so on.

## Repository Information

[![Count Lines of Code](https://github.com/500Foods/Philement/actions/workflows/main.yml/badge.svg)](https://github.com/500Foods/Philement/actions/workflows/main.yml)

NOTE: Please refer to individual projects for a more nuanced breakdown.
The Hydrogen project, for example, shows the lines of C code grouped into core project code and unit testing code, and combines C and C header files into the same row, along with providing additional statistics.
<!--CLOC-START -->
```cloc
Last updated at 2026-08-28 18:27:12 UTC
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
JSON                           498            433              0        1650922
SVG                            298            581          11383         526773
C                             1617          65316          57015         268135
Text                           336            332              0         139622
Markdown                       750          33996            248         101937
Lua                            422          11439           7255          91539
JavaScript                     224          10445          13582          49798
Bourne Shell                   175           7853         107895          43340
CSS                             84           2769           1563          14005
C/C++ Header                   341           4096          14005          12537
HTML                            42            258            219           2836
CMake                           18            176            355           1412
make                            11            112             75            607
TOML                             1             55             35            449
Python                           1             36              9            195
SQL                             11             21             24            195
zsh                              2             31             42             97
Delphi Form                      1              1              0             43
YAML                             2              8             13             37
Pascal                           2             11              2             31
-------------------------------------------------------------------------------
SUM:                          4836         137969         213720        2904510
-------------------------------------------------------------------------------
1717 Files were skipped (duplicate, binary, or without source code):
  svg: 1222
  css: 165
  html: 134
  md: 39
  js: 24
  png: 23
  br: 16
  lua: 12
  pem: 9
  kid: 8
  gitignore: 6
  jpg: 5
  mp4: 5
  clp: 3
  ico: 3
  sqlite: 3
  json: 2
  lintignore-markdown: 2
  sqruff_db2: 2
  sqruff_mysql: 2
  sqruff_postgresql: 2
  sqruff_sqlite: 2
  woff2: 2
  3mf: 1
  ansi: 1
  backup: 1
  bmp: 1
  control: 1
  disabled: 1
  dproj: 1
  gcode: 1
  ggignore: 1
  gitattributes: 1
  jsonc: 1
  key: 1
  lintignore-bash: 1
  lintignore-c: 1
  lintignore-lua: 1
  lintignore: 1
  list: 1
  payload_generated: 1
  sqlite-shm: 1
  sqlite-wal: 1
  stl: 1
  stylelintcache: 1
  stylelintrc: 1
  supp: 1
  trial-ignore: 1
  webp: 1
```
<!--CLOC-END-->

## Sponsor / Donate / Support

If you find this work interesting, helpful, or valuable, or that it has saved you time, money, or both, please consider directly supporting these efforts financially via [GitHub Sponsors](https://github.com/sponsors/500Foods) or donating via [Buy Me a Pizza](https://www.buymeacoffee.com/andrewsimard500). Also, check out these other [GitHub Repositories](https://github.com/500Foods?tab=repositories&q=&sort=stargazers) that may interest you.
