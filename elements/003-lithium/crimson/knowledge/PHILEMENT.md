# [Philement](https://www.philement.com)

While Klipper has been an enormous boon to the 3D printing community as a whole, it isn't without its faults. Philement has been conceived as a full-on replacement, but it will take quite awhile to get there. In the interim it may end up serving as a front-end to Klipper, a bit like Obico perhaps.

The main differentiator to address upfront is that a large chunk of the lower-level code has been written in C and specifically not Python. For so many reasons. To help make this go a little more quickly, various AI engines are being tasked to help out. It isn't written by AI, but written with the help of AI. If that distinction matters to anyone. If nothing else, this gives a degree of plausible deniability if there's something crazy going on in the code.

As for the name, it is a combination of terms like 'Phi' (referencing the number 500 among *many* other meanings), 'filament' (that's what 3D printing is primarily obsessed with), and 'element' (small pieces of something larger). The individual components of the project have been named after atomic elements, for example.

As far as progress reporting goes, well, there are likely 500 steps or more to be completed before anyone takes this project seriously. Some of those steps will yield useful tools, as is already the case. But it is an ambitious project with many items to complete, and many more that likely haven't even been conceived of yet.
<br/><br/><img src="https://progressbar-guibranco.vercel.app/50/?scale=500&title=%20Completed%20&width=415&suffix=%20%2F%20500%20Steps" alt="Completed 49 / 500 Steps">

## Elements

This project has a number of, well, elements. Each is named after an element in the periodic table. Like elements in the real world, some of these will be hugely important while others may be relatively insignificant. Some will require thousands and thousands of developer hours, and others not so much. And the effort applied to each will likely be equally unbalanced as the focus shifts among the different pieces needed to get this up and running. The main [Philement Documentation](/docs/README.md) index is also available, covering elements that are a little further along.

| Element | Status | Description |
| :-------: | :------: | :------------ |
| [hydrogen](/docs/H/README.md) | 🔨 | A websocket-equipped service, like Klipper+Moonraker combined |
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
| [vanadium](/docs/V/README.md) | 🏆 | Custom font for Philement based off of Iosevka |

<sup>💡 → Idea and Planning Stage &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; 🔨 → Working on it &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; 🏆 → Nowhere near done but... Check it out!</sup>

## Additional Notes

While this project is currently under active development, feel free to give it a try and post any issues you encounter.  Or start a discussion if you would like to help steer the project in a particular direction.  Early days yet, so a good time to have your voice heard.  As the project unfolds, additional resources will be made available, including platform binaries, more documentation, demos, and so on.

## Repository Information

[![Count Lines of Code](https://github.com/500Foods/Philement/actions/workflows/main.yml/badge.svg)](https://github.com/500Foods/Philement/actions/workflows/main.yml)

NOTE: Please refer to individual projects for a more nuanced breakdown.
The Hydrogen project, for example, shows the lines of C code grouped into core project code and unit testing code, and combines C and C header files into the same row, along with providing additional statistics.
<!--CLOC-START -->
```cloc
Last updated at 2026-07-16 21:43:45 UTC
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
JSON                           425            393              0        1407118
SVG                            207            405           6391         267140
C                             1311          56067          53127         216606
Text                           293            394              0         117928
Markdown                       713          30307            242          89472
JavaScript                     274          10488          13561          51470
Lua                            279           6734           4674          44533
Bourne Shell                   136           6728          91957          33036
CSS                            124           2769           1563          14045
C/C++ Header                   308           3678          13063          10928
CMake                           22            197            356           3287
HTML                            54            307            232           3083
make                            11            112             75            607
TOML                             1             55             35            449
YAML                             3             23             15            309
Python                           1             36              9            195
SQL                             10             21             23            191
zsh                              2             31             42             97
Delphi Form                      1              1              0             43
Pascal                           2             11              2             31
-------------------------------------------------------------------------------
SUM:                          4177         118757         185367        2260568
-------------------------------------------------------------------------------
3134 Files were skipped (duplicate, binary, or without source code):
  svg: 887
  gcno: 856
  css: 440
  br: 370
  html: 345
  png: 54
  md: 39
  js: 24
  mp4: 15
  lua: 12
  gcda: 9
  json: 7
  jpg: 6
  woff2: 6
  gitignore: 5
  ico: 4
  clp: 3
  lintignore-markdown: 3
  lintignore-bash: 2
  lintignore-c: 2
  lintignore-lua: 2
  lintignore: 2
  ninja: 2
  sqlite: 2
  sqruff_db2: 2
  sqruff_mysql: 2
  sqruff_postgresql: 2
  sqruff_sqlite: 2
  3mf: 1
  ansi: 1
  auth_code_flow_debug: 1
  backup: 1
  check_cache: 1
  client_credentials_debug: 1
  control: 1
  detailed: 1
  disabled: 1
  dproj: 1
  gcode: 1
  ggignore: 1
  gitattributes: 1
  jsonc: 1
  key: 1
  ninja_deps: 1
  ninja_log: 1
  password_flow_debug: 1
  payload_generated: 1
  pem: 1
  sh: 1
  sqlite-shm: 1
  sqlite-wal: 1
  stl: 1
  stylelintcache: 1
  stylelintrc: 1
  supp: 1
  trial-ignore: 1
  txt: 1
```
<!--CLOC-END-->

## Sponsor / Donate / Support

If you find this work interesting, helpful, or valuable, or that it has saved you time, money, or both, please consider directly supporting these efforts financially via [GitHub Sponsors](https://github.com/sponsors/500Foods) or donating via [Buy Me a Pizza](https://www.buymeacoffee.com/andrewsimard500). Also, check out these other [GitHub Repositories](https://github.com/500Foods?tab=repositories&q=&sort=stargazers) that may interest you.
