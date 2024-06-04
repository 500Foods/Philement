# carbon
This element is concerned with monitoring a print or other activity, and taking actions when failures are detected. 

## Brainstorming
- The first draft is just comparing timelapse images.
- Select part of an image to exclude vs. print area, etc.
- Changes in non-print area that is not excluded -> problem.
- Changes in print area that are substantial -> problem.
- Can be run on-device (a few seconds per layer, max).
- Can interact with the current print job - pause print.
- Log activity for tuning purposes.
- Code likely to be incorporated directly into hydrogen.
- Tools available as standalone as well for testing/tuning.
- UI from Lithium for creating profiles.
  
## First Draft
To get the ball rolling, the first draft uses captured timelapse images and compares them to see if there's something amiss. There are a lot of assumptions being made here.
- The camera view is directly overhead and fixed relative to the print bed, which presumably is fixed as well.
- The toolhead is in the same position in each image, and anything running to the toolhead (filament, cabling, etc.) is either out-of-frame or in a fixed position as well.
- The user has identified which parts of the image are the print area, non-print area, or that should be excluded (visible timers, parked toolhead, edges, etc.)

So this can be very limiting. Future versions might do this very differently of course. This isn't going to work particularly well for things like nozzle cams, or anything 
other than fixed-bed printers at the moment. But if you've got a Voron 2.4 or a Troodon, this might be quite workable.

The basic algorithm is the following.
- Get two images to compare. These should be just a pair of JPG timelapse images, though PNG could also be used.
- Divide the images up into square blocks. The number of blocks across is determined by the precision parameter.
- Read in a profile that describes whether each block is part of the print area, the non-print area, or should be excluded.
- The number of blocks in the profile should match the number of blocks in the image. IE, 100x75 or something like that.
- The blocks are summarized into an RGB value. Nothing very fancy going on here. But the summary is output for inspection.
- An array is generated using the RGB values' Euclidean distance as the comparison, with tolerance as a factor.
- The resulting array is also output for comparison which can be used to help with profile calibration and so on.
- An assessment is made based on whether there are any egregious block discrepancies.
- If the assessment is a fail, an image is generated highlighting the portions of the image that don't match.

## carbon Examples
- [Example One](https://github.com/500Foods/Philement/blob/main/elements/006-carbon/examples/example1.md) - Shows the output run with precision=30 and tolerance=10 (Pass).
- [Example Two](https://github.com/500Foods/Philement/blob/main/elements/006-carbon/examples/example2.md) - Shows the same images with precision=100 and tolerance=2 (Fail).

## carbon Klipper Configuration
To configure Klipper to perform checks using carbon, let's first ensure the following conditions are met.
- Timelapse is configured to generate suitable images (see above) after each layer.
- The gcode_shell_command extension has been installed and tested.
- More than passing familiarity with both Klipper macros and Linux Bash scripts.
- Necessary permissions to make changes to both Klipper macros and Linus Bash scripts.
- The Klipper PAUSE macro is configured and works as expected, or something equivalent.
- The mechanism for sending Klipper commands from Linux has been tested.

With that out of the way, lets create a [PRINT_CHECK] macro that calls a Linux Bash script that can then call carbon.
Based on the output of carbon, this script can then log the output, trigger a pause, and so on. But first, Klipper.
```
[PRINT_CHECK]
```



