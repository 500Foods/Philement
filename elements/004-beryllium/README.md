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

## First Draft
The first thing out of the gate is a G-code analyzer. This is a C program that takes in a G-code file and produces various bits of information about it. In particular, we're interested in the time estimate for the print job, and the amount of filament required. This isn't particularly well tested against every conceivable print job, but it does a reasonable amount of work to produce something of value. And the code has been bundled up in a way that should make it not difficult to use in other Philement elements as well. 

- beryllium.c / beryllium.h - These contain the analysis code
- beryllium_analyzec. - This is the wrapper for the above code

Building the project couldn't be much simpler.
```
#! /bin/bash
gcc -std=c17 -c beryllium.c -lm -o beryllium.o
gcc -std=c17 -c beryllium_analyze.c -lm -o beryllium_analyze.o
gcc -std=c17 beryllium.o beryllium_analyze.o -lm -o beryllium_analyze
```

Running the program without parameters lists the available options.
```
$ ./beryllium_analyze
Philement/Beryllium G-Code Analyzer
Usage: ./beryllium_analyze [OPTIONS] <filename>
Options:
  -a, --acceleration       ACCEL  Set acceleration           (default: 1000.00 mm/s^2)
  -z, --z-acceleration     ACCEL  Set Z-axis acceleration    (default: 250.00 mm/s^2)
  -e, --extruder-accel     ACCEL  Set extruder acceleration  (default: 2000.00 mm/s^2)
  -x, --max-speed-xy       SPEED  Set max XY speed           (default: 5000.00 mm/s)
  -t, --max-speed-travel   SPEED  Set max travel speed       (default: 5000.00 mm/s)
  -m, --max-speed-z        SPEED  Set max Z speed            (default: 10.00 mm/s)
  -f, --default-feedrate   RATE   Set default feedrate       (default: 7500.00 mm/min)
  -d, --filament-diameter  DIAM   Set filament diameter      (default: 1.75 mm)
  -g, --filament-density   DENS   Set filament density       (default: 1.04 g/cm^3)
  -l, --layertimes                Output layer times         (default: not listed)
```
Running against a small G-code file with only --layertimes as a parameter gets us the desired results.
```
$ ./beryllium_analyze sample.gcode -l
Philement/Beryllium G-Code Analyzer
Analysis start: 2024-06-07T19:50:41Z
Analysis end: 2024-06-07T19:50:41Z
Analysis duration: 39.409000 ms

File size: 1166356 bytes
Total lines: 29530
G-code lines: 26387
Number of layers (height): 26
Number of layers (slicer): 25
Filament used: 5332.90 mm (12.83 cm^3)
Filament weight: 13.34 grams
Estimated print time: 00:00:40:18

Layer  Start Time   End Time     Duration
00001  00:00:00:00  00:00:00:00  00:00:00:00
00002  00:00:00:00  00:00:03:17  00:00:03:17
00003  00:00:03:17  00:00:06:43  00:00:03:25
00004  00:00:06:43  00:00:10:08  00:00:03:25
00005  00:00:10:08  00:00:11:18  00:00:01:11
00006  00:00:11:18  00:00:12:33  00:00:01:14
00007  00:00:12:33  00:00:13:49  00:00:01:17
00008  00:00:13:49  00:00:15:07  00:00:01:17
00009  00:00:15:07  00:00:16:20  00:00:01:14
00010  00:00:16:20  00:00:17:33  00:00:01:13
00011  00:00:17:33  00:00:18:23  00:00:00:49
00012  00:00:18:23  00:00:19:12  00:00:00:50
00013  00:00:19:12  00:00:20:01  00:00:00:49
00014  00:00:20:01  00:00:20:50  00:00:00:48
00015  00:00:20:50  00:00:21:38  00:00:00:49
00016  00:00:21:38  00:00:22:27  00:00:00:49
00017  00:00:22:27  00:00:23:17  00:00:00:50
00018  00:00:23:17  00:00:24:06  00:00:00:49
00019  00:00:24:06  00:00:25:17  00:00:01:11
00020  00:00:25:17  00:00:26:31  00:00:01:14
00021  00:00:26:31  00:00:27:54  00:00:01:23
00022  00:00:27:54  00:00:29:59  00:00:02:04
00023  00:00:29:59  00:00:33:24  00:00:03:25
00024  00:00:33:24  00:00:36:49  00:00:03:25
00025  00:00:36:49  00:00:40:18  00:00:03:29

Configuration:
  Acceleration: 1000.00 mm/s^2
  Z-axis acceleration: 250.00 mm/s^2
  Extruder acceleration: 2000.00 mm/s^2
  Max XY speed: 5000.00 mm/s
  Max travel speed: 5000.00 mm/s
  Max Z speed: 10.00 mm/s
  Default feedrate: 7500.00 mm/min
  Filament diameter: 1.75 mm
  Filament density: 1.04 g/cm^3
```
## Additional Information
- As a first draft, this isn't going to be perfect for everything, but with enough detail about a specific printer, it should be able to generate something reasonably respectable.
- More defaults could be gleaned from the G-code file, but it isn't always the case that the slicer puts the data in there. Like filament density, for example.
- The default filament density is for ASA, for example, which is by no means the most popular or most or least dense filament.
- The defaults are not especially ideal for any printer but should be easy enough to customize if you happen to know the correct values.
- More could be done to improve accuracy. To help not go too far down that rabbit hole, an array of layer times is generated. We could use this to compare to actual times during a print to make adjustments on the fly.
- The calculations are assuming Core XY kinematics, but this might look very different for different styles of printers.
- Fiddling with the parameters, particularly the default feedrate, can produce dramatically different results.
- Could use feedrate as a way to calibrate the algorithm, in theory, if it isn't really a known figure for a given printer.
- Similarly, the weight calculation (grams) is important, but its accuracy depends on accurately knowing the density.
- Lots of room for improvement, but for a first draft this gives us some data to work with.
