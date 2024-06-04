#!/usr/bin/bash

# Usage:
#  print_check.sh <PrinterName> <CurrentLayerNumber>

# Time this script
script_start=`date +%s%3N`

# Sort out where stuff lives
KlipperCmd=~/printer_data/comms/klippy.serial
printername=$1
currentlayer=$2
currentlayername=`printf "%05d" $2`
log1=~/printer_data/logs/carbon.log
log2=~/printer_data/logs/carbon-debug-$currentlayername.log
timelapse=/tmp/timelapse
carbon=~/philement/carbon/carbon
carbonprofile=~/philement/carbon/profile100.txt
precision=100
tolerance=4
assessment="Wait"
passfail=-1

# Get the images to compare
image1=`ls -t1 $timelapse | grep -v failure | head -n2 | tail -n1`
image2=`ls -t1 $timelapse | grep -v failure | head -n1`

# if the images aren't the same file (first layer or no images) then get to work
if [ $currentlayer -gt 5 ]
then
    assessment="Skip"
    if [ $image1 != $image2 ]
    then
        sleep 1
        `nice -15 $carbon $timelapse/$image1 $timelapse/$image2 $precision $tolerance $carbonprofile > $log2`
        passfail=$?
    fi
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
    echo "M118 PRINT_CHECK Failure Detected" > $KlipperCmd
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
