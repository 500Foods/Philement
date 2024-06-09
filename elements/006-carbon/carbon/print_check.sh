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

# It failed, so we should send out notification and inform Klipper
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
