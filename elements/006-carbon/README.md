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

## Klipper Configuration
To configure Klipper to perform checks using carbon, let's first ensure the following conditions are met.
- Timelapse is configured to generate suitable images (see above) after each layer.
- The gcode_shell_command extension has been installed and tested.
- More than passing familiarity with both Klipper macros and Linux Bash scripts.
- Necessary permissions to make changes to both Klipper macros and Linus Bash scripts.
- The Klipper PAUSE macro is configured and works as expected, or something equivalent.
- The mechanism for sending Klipper commands from Linux has been tested.

## Compile carbon
As carbon is a plain C application, it can be compiled easily using GCC, which is likely already installed and
configured on any device running Klipper, particularly a Raspberry Pi or a clone. Typically all of Philement is
installed in separate folders, so in this case, we just need to download the source file and compile it
```
mkdir ~/Philement
cd ~/Philement
mkdir carbon
cd carbon
wget https://raw.githubusercontent.com/500Foods/Philement/main/elements/006-carbon/carbon/carbon.c
gcc -std=c17 carbon.c -lm -ljpeg -lpng -o carbon
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
    # Assign printer name and send current layer number
    {% set printer_name="Mr.Troodon" %}
    RUN_SHELL_COMMAND CMD=shell_print_check PARAMS="{printer_name} {printer.print_stats.info.current_layer} {printer.print_stats.info.total_layer}"

[gcode_macro PRINT_CHECK_TEST]
description: Run Philement/carbon to compare Timelapse images
gcode:
    # Assign printer name and send current layer number
    {% set printer_name="Mr.Troodon" %}
    RUN_SHELL_COMMAND CMD=shell_print_check PARAMS="{printer_name} {printer.print_stats.info.current_layer} {printer.print_stats.info.total_layer} TEST"
```
The script being called has been placed in ~/scripts/print_check.sh. 

## Bash print_check.sh Script
This Bash script then takes the parameters that were passed and sorts out where the various parts of Klipper and carbon
can be found. All that's left is to run carbon and process the results.
```
#!/usr/bin/bash

# Usage:
#  print_check.sh <PrinterName> <CurrentLayerNumber> <TotalLayerCount> <TEST>
#
# NOTE: If the last parameter is TEST then it will automatically trigger the
#       action, which includes sending an e-mail and posting a message to the
#       Klipper log.

####################################################################################################
# Script settings - this should cover most of the things that people might want to change
####################################################################################################

# What this script calls itself - likely the same as the Klipper macro that calls it
appname="PRINT_CHECK"

# Where to send Klipper commands
KlipperCmd=~/printer_data/comms/klippy.serial

# Printer name, current layer, and total layer are passed in as parameters $1, $2, and $3.
# $2 and $3 are formatted with padded 0's at the beginning, eg Layer 00032 of 00048
printername=$1
currentlayer=$2
currentlayername=`printf "%05d" $2`
totallayer=$3
totallayername=`printf "%05d" $3`

# Where to output the regular and debugging logs.
# Note: Summary information in the notification e-mail is dervied from the debug logs,
#       so don't disable them if you want to see this information. Also, debug logs are
#       overwritten by each print job, so they won't continue to grow.
log1=~/printer_data/logs/carbon-$printername.log
log2=~/printer_data/logs/carbon-$printername-debug-$currentlayername.log
#log2=/dev/null

# Where to find the timelapse images
timelapse=/tmp/timelapse

# Where to find the Philement/carbon app and what parameters to pass it
precision=100
tolerance=4
carbon=~/philement/carbon/carbon
carbonprofile=~/philement/carbon/profile$precision.txt

# Messages sent to Klipper
monitoring="Montirong is active"
failure="Possible failure detected"

#---------------------------------------------------------------------------------------------------
# E-mail settings
#
# Note: This script uses "mutt" to send HTML emails so be sure to test it with something like
#         echo "Test body" | mutt user@example.com -s "Test subject"
#       to make sure that it is working before trying to use it here.
# Note: The summary that carbon produces is also appended to the email if it is available
# Note: An image is attached highlighting flagged areas if available
#---------------------------------------------------------------------------------------------------
email="andrew@500foods.com"

subject="[ $printername ] $appname: Possible failure detected on Layer # $currentlayername of $totallayername"

body="<h5>$printername</h5>"
body+="<p>$appname: Possible failure detected on Layer # $currentlayername of $totallayername</p>"
body+="<p>Please review the attached image:"
body+="<ul><li>Differences detected are shaded in RED.</li>"
body+="    <li>Areas outside of the print area are shaded in BLUE.</li>"
body+="    <li>Excluded areas are not monitored and are shaded in BLACK.</li></ul></p>"
#---------------------------------------------------------------------------------------------------

####################################################################################################
# End of script settings
####################################################################################################

# Time this script
script_start=`date +%s%3N`

# Get the images to compare
image1=`ls -t1 $timelapse | grep -v failure | head -n2 | tail -n1`
image2=`ls -t1 $timelapse | grep -v failure | head -n1`

# Make a note in the log as a reminder that this is running
if [ $currentlayer -eq 5 ]
then
    echo "M118 $appname $monitoring" > $KlipperCmd
fi

# By default, we're waiting for the first image to compare
assessment="Wait"
passfail=-1

# The first few layers are ignored as they're likely to be dramatically different anyway and
# would typically trigger a failed assessment all the time.
if [ $currentlayer -gt 5 ]
then

    # if the images aren't the same file (first layer or no images) then get to work
    assessment="Skip"
    if [ $image1 != $image2 ]
    then
        sleep 1
        `nice -15 $carbon $timelapse/$image1 $timelapse/$image2 $precision $tolerance $carbonprofile HTML > $log2`
        passfail=$?
    fi
fi

# It passed, so not really anything to do
if [ $passfail -eq 0 ]
then
    assessment="Pass"
fi

# It failed, so we should send out a notification and inform Klipper
# Note: We should also do this if the TEST option is supplied as an optional last parameter
#       This can also be triggered from Klipper via a PRINT_CHECK_TEST macro
if [ $passfail -eq 1 ] || [ $4 ==  'TEST' ]
then
    assessment="Fail"

    # Send commands to Klipper
    echo "M118 $appname $failure" > $KlipperCmd
    # echo "PRINT_PAUSE" > $KlipperCmd

    # Append details of failure to e-mail message if available
    if [ -f $log2 ] && [ $log2 != "/dev/null" ]
    then
        body+=`tail -n 3 $log2`
    fi

    # Attach failed image if available
    imagef=`ls -t1 $timelapse | grep failure | head -n1`

    # Send the e-mail either with the image or without if it isn't available
    if [ -f $timelapse/$imagef ]
    then
        echo "$body" | mutt -e "set content_type=text/html" $email -s "$subject" -a $timelapse/$imagef
    else
        echo "$body" | mutt -e "set content_type=text/html" $email -s "$subject"
    fi
fi

# It encountered a different error likely related to the configuration
# In this case, there's likely no image to send, but a more urgent need
# to sort out whatever has caused this mechanism to fail
if [ $passfail -gt 1 ]
then
    assessment="Err?"
fi

# How long has the script been running?
script_end=`date +%s%3N`
timediff=$(($script_end-$script_start))

# Make a note in the log of what went down
echo `date +"%Y-%m-%dT%H:%M:%S%z"` " $printername.$currentlayername  [$assessment]  Elapsed Time: $timediff ms" >> $log1
```
Note that if a print failure is detected, it issues a log G-code command to Klipper (M118). Calling another macro to 
actually pause or cancel the print would normally be the case once it has been through a little more testing.
Other actions, like sending an email, can also be performed here, likely outside of Klipper.

The above script uses a precision value of 100, meaning that it is expecting to receive a profile that is 100 characters
across by as many down that fit the aspect ratio of the images. A profile100.txt file can be found in the examples folder.
Be sure to edit it to mask areas (X) or mark print area (P) versus non-print area (N) regions.

The above script uses a tolerance value of 4. This can be adjusted to reduce the rates of false positives. It needs to
be low enough to catch obvious issues. It can be tested by simply adding or removing an object from the build plate.
The default action defined above just logs an entry in Klipper, so be sure to test and ensure that the tolerance is
low enough to trigger the warning. 

## Orca Configuration
Finally, for reasons unknown, Orca doesn't necessarily tell Klipper about layer changes. Perhaps this is configured for
each printer individually. In any event, two changes need to be made.

In the Machine start G-code, where normally there is just the call to PRINT_START, the total number of layers is 
passed into Klipper.
```
PRINT_START EXTRUDER=[nozzle_temperature_initial_layer] BED=[bed_temperature_initial_layer_single]
SET_PRINT_STATS_INFO TOTAL_LAYER=[total_layer_count]
```
In the Layer change G-code section, where TIMELAPSE_TAKE_FRAME] was added as part of installing Timelapse support, 
this is where the call to the [PRINT_CHECK] macro is added, and where the layer counter is updated.
```
;AFTER_LAYER_CHANGE
;[layer_z]
SET_PRINT_STATS_INFO CURRENT_LAYER={layer_num + 1} 
TIMELAPSE_TAKE_FRAME
PRINT_CHECK
```
Note that both of these have to be configured in order for the layer-counting mechanism to work. The layer count
is used by our script to skip checking the first few layers as those are likely to change more dramatically than
subsequent layers.

## Additional Notes
- This is running an algorithm after each layer print. It usually only takes around 1100 msec in testing on a BTT-CB1, a low-powered Raspberry Pi clone.
- These systems don't have a lot of cycles to spare. Klipper can get tripped up if it takes too long to process an image.
- The 'nice' and 'sleep' commands were added in an attempt to smooth this over. Adjust or remove as needed.
- If Fluidd, Mainsail, Obico, and everything else imaginable are also running, this might be too much of a load for the system.
- The debug log can be redirected to /dev/null when testing is complete, but it regenerates a file for each layer during its run.
- They aren't deleted, so if a new run has fewer layers, the debug logs will have a mix of print jobs contained within them. Pay attention to the timestamps.
- Running as configured, an image will be generated in the same timelapse folder as the other images, one for each failure.
- These do seem to get cleaned up when the timelapse completes.
