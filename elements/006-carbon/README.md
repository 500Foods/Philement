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

## Compile carbon
As carbon is a plain C application, it can be compiled easily using GCC, which is likely already installed and
configured on any device running Klipper, particuarly a Raspberry Pi or a clone. Typically all of Philement is
installed in separate folders, so in this case, we just need to download the source file and compile it
```
mkdir ~/Philement
cd ~/Philement
mkdir carbon
cd carbon
wget https://raw.githubusercontent.com/500Foods/Philement/main/elements/006-carbon/carbon/carbon.c
gcc carbon.c -lm -ljpeg -lpng -o carbon
```
## [PRINT_CHECK] Macro
With that out of the way, let's create a [PRINT_CHECK] macro that calls a Linux Bash script that can then call carbon.
Based on the output of carbon, this script can then log the output, trigger a pause, and so on. But first, Klipper.
This requires the gcode_shell_command extension to be installed as part of KIAUH.
```
#######################################
# PRINT_CHECK macro
#######################################
[gcode_shell_command shell_print_check]
command: ~/scripts/print_check.sh
timeout: 10.
verbose: False

[gcode_macro PRINT_CHECK]
description: Run Philement/carbon to compare Timelapse images
gcode:
    # Show elapsed time (Note: Requires TIMER 
    {% set printer_name="Troondon" %}
    {% set current_layer='%05d' % printer.print_stats.info.current_layer|int %}
    RUN_SHELL_COMMAND CMD=shell_print_check PARAMS="{printer_name} {current_layer}"
```
The script being called is ~/scripts/print_check.sh, passing the printer name and the current layer as parameters.

## Bash print_check.sh Script
This Bash script then takes the parameters that were passed and sorts out where the various parts of Klipper and carbon
can be found. All that's left is to run carbon and process the results.
```
#!/usr/bin/bash

# Usage:
#  print_check.sh <PrinterName> <CurrentLayerNumber>

# Time this script
script_start=`date +%s%3N`

# Sort out where stuff lives
log1=~/printer_data/logs/carbon.log
log2=~/printer_data/logs/carbon-debug.log
timelapse=/tmp/timelapse
carbon=~/philement/carbon/carbon
KlipperCmd=~/printer_data/comms/klippy.serial
carbonprofile=/philement/carbon/profile100.txt
assessment="Skip"

# Get the images to compare
image1=`ls -t1 $timelapse | head -n2 | tail -n1`
image2=`ls -t1 $timelapse | head -n1`

# if the iamges aren't the same file (first layer or no images) then get to work
if [ $image1 != $image2 ]
then
    echo `date +"%Y-%m-%dT%H:%M:%S%z"` " $1.$2 " >> $log2
    `$carbon $timelapse/$image1 $timelapse/$image2 100 10 $carbonprofile >> $log2`
    passfail=$?
    echo " " >> $log2
fi

# It passed, so not really anything to do
if [ $passfail -eq 0 ]
then
    assessment="Pass"
fi

# It failed, so we should send out notification and then pause the print
if [ $passfail -eq 1 ]
then
    assessment="Fail"
    echo "PRINT_PAUSE" > $KlipperCmd
fi

# How long has the script been running?
script_end=`date +%s%3N`
timediff=$(($script_end-$script_start))

# Make a note in the log of what went down
echo `date +"%Y-%m-%dT%H:%M:%S%z"` " $1.$2   [$assessment]  Elapsed Time: $timediff ms" >> $log1
```
Note that if a print failure is detected, it runs the [PRINT_PAUSE] command in Klipper. This isn't a default Klipper macro,
so either update Klipper with a macro named [PRINT_PAUSE] to trigger the pause, or change this script to use a different
macro or take a different action. Other actions, like sending an email, can also be performed here, likely outside of
Klipper.
