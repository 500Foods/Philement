# [Philement](https://www.philement.com)
While Klipper has been an enormous boon to the 3D printing community as a whole, it isn't without its faults. Philement has been conceived as a full-on replacement, but it will take quite awhile to get there. In the interim it may end up serving as a front-end to Klipper, a bit like Obico perhaps. 

The main differentiator to address upfront is that a large chunk of the lower-level code has been written in C and specifically not Python. For so many reasons. To help make this go a little more quickly, various AI engines are being tasked to help out. It isn't written by AI, but written with the help of AI. If that distinction matters. Sort of like relying on something like Copilot to do the grunt work. This has helped cover a lot of ground far more quickly than what might have otherwise been possible. When looking at any of the code, be sure to examine it critically - it may very well be in need of some proper attention.

As for the name, it is a combination of terms like Phi (referencing the number 500 among other meanings), filament (that's what we're primarily obsessed with in 3D printing) and element (we're taking things down to the element level, that sort of thinking). The individual components of the project have been named after atomic elements, for example.

## Elements
This project has a number of, well, elements. Each is named after an element in the periodic table. Like elements in the real world, some of these will be hugely important while others may be relatively insignificant. Some will require thousands and thousands of developer hours and others not so much. And the effort applied to each will likely be equally unbalanced as the focus shifts among the different pieces needed to get this up and running.

| Element  | Description |
|:---------:|:---|
| [hydrogen](https://github.com/500Foods/Philement/tree/main/elements/001-hydrogen/README.md) | A websocket-equipped service, like Klipper+Moonraker combined. <tr></tr> |
| [helium](https://github.com/500Foods/Philement/tree/main/elements/002-helium/README.md) | Everything database-related. <tr></tr> |
| [lithium](https://github.com/500Foods/Philement/tree/main/elements/003-lithium/README.md) | Web-based UI.  <tr></tr> |
| [beryllium](https://github.com/500Foods/Philement/tree/main/elements/004-beryllium/README.md) | Deals with everything gcode-related. <tr></tr> |
| [boron](https://github.com/500Foods/Philement/tree/main/elements/005-boron/README.md) | Rhymes with Voron! Hardware database. Like Vorons. Or [Troodons](https://github.com/500Foods/WelcomeToTroodon). <tr></tr> |
| [carbon](https://github.com/500Foods/Philement/tree/main/elements/006-carbon/README.md) | Print fault detection, a bit like what Obico is for. <tr></tr> |
| [Nitrogen](https://github.com/500Foods/Philement/tree/main/elements/007-nitrogen/README.md) | Think "clapperboard" for 3D printers. Or KlipperScreen. Or Knomi. That sort of thing.   <tr></tr> |
| [Oxygen](https://github.com/500Foods/Philement/tree/main/elements/008-oxygen/README.md) | Notifications. <tr></tr> |
| [Fluorine](https://github.com/500Foods/Philement/tree/main/elements/009-fluorine/README.md) | Filament management system. <tr></tr> |
| [Neon](https://github.com/500Foods/Philement/tree/main/elements/010-neon/README.md) | Well, lighting, obviously. <tr></tr> |
| [Sodium](https://github.com/500Foods/Philement/tree/main/elements/011-sodium/README.md) | An MMU and MMU support. <tr></tr> |
| [Magnesium](https://github.com/500Foods/Philement/tree/main/elements/012-magnesium/README.md) | Print farm management tool. <tr></tr> || 

## Additional Notes
While this project is currently under active development, feel free to give it a try and post any issues you encounter.  Or start a discussion if you would like to help steer the project in a particular direction.  Early days yet, so a good time to have your voice heard.  As the project unfolds, additional resources will be made available, including platform binaries, more documentation, demos, and so on.

## Repository Information 
[![Count Lines of Code](https://github.com/500Foods/Template/actions/workflows/main.yml/badge.svg)](https://github.com/500Foods/Template/actions/workflows/main.yml)
<!--CLOC-START -->
```
Last updated at 2024-06-03 04:59:45 UTC
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
Markdown                        13             19              2            158
YAML                             2              8             13             35
-------------------------------------------------------------------------------
SUM:                            15             27             15            193
-------------------------------------------------------------------------------
3 Files (without source code) were skipped
```
<!--CLOC-END-->

## Sponsor / Donate / Support
If you find this work interesting, helpful, or valuable, or that it has saved you time, money, or both, please consider directly supporting these efforts financially via [GitHub Sponsors](https://github.com/sponsors/500Foods) or donating via [Buy Me a Pizza](https://www.buymeacoffee.com/andrewsimard500). Also, check out these other [GitHub Repositories](https://github.com/500Foods?tab=repositories&q=&sort=stargazers) that may interest you.
