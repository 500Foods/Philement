# beryllium
G-code is of course the language of 3D printing (and CNC machining - shouldn't they be offered a seat at the table?). But as they say, the great thing about standards is that there are so many to choose from. In this case, there are naturally several flavors of G-code, and even within the 3D printing realm, there are variations between Klipper, Marlin, and others. Would be nice if this were not fractured so, but at the end of the day it means having to support whatever comes our way.

## Brainstorming
- Mechanism to call Klipper directly to convert their G-code (macros and so on) into code that can be fed to Klipper directly.
- This likely duplicates some of what Moonraker does.
- Tools for working with G-code. How much time does this G-code take to print? How much filament? G-code analyzer, basically.
- Alternative scripting solution. Probably Lua at this stage.
- Visualizing G-code.
- Storing or otherwise managing G-Code files.
- Potentially other files as well, like the original STL or other 3D formats.
- Maybe send files to the slicer as well as receive files from the slicer.
- Augment G-code with progress forecast data.
- Alternative to Klipper for handling G-code in the first place.
