# Deuterium aka klippertest

This is a prototype that is intended to demonstrate how to work with the Klipper API natively. As in, directly with its Unix socket interface and explicitly not through Moonraker or anything else.
A Python version is provided to prove the concept is workable, and then a C program is used to do largely the same thing, demonstrating that this can be a more performant system overall.

NOTE: As a comparison, the initial connection establishment time for the C program is on the order of 0.005s. For Python, it is 0.5s. So a 100x improvement in that aspect alone. Of course, establishing a connection
is only part of the process, and the performance gains on average will likely be less, but still rather significant - enough to justify bypassing Python entirely for the Philement project, where feasible.

The first part of the test involves parsing what we can from the Klipper configuration files, which themselves are located by looking at the processes running on the system.
Note that this has to be run on the same system running Klipper in order to access the Unix socket, in a default Klipper configuration. What we're primarily after is the socket in the underlying filesystem, assuming a Linux-style environment.

The second part of the test involves setting up a subscription for toolhead position notifications. Once subscribed, Klipper will send any toolhead position changes back to our program via the same Unix socket, which is then output to the console.

## klippertest.py

- The asyncio library is used to try to make this as performant as possible
- Lots of effort to track down the configuration information
- Shouldn't need to pass parameters as we can determine everything with a bit of work
- Klipper configuration files are a complete mess at the best of times, let's not make it worse
- It's Python, so no build step required, just run as-is.
- The printer in this case is a Troodon 2.0 Pro running with a BTT-CB1 Pi clone (very similar to a Voron 2.4R2)

Sample output.

``` bash
$ ./klippertest.py
2024-06-24 13:29:31,403 - DEBUG - Using selector: EpollSelector
2024-06-24 13:29:31,658 - DEBUG - Found processes:
2024-06-24 13:29:31,659 - DEBUG - - crowsnest (PID: 828): /bin/bash /usr/local/bin/crowsnest -c /home/biqu/printer_data/config/crowsnest.conf
2024-06-24 13:29:31,659 - DEBUG - - klipper (PID: 836): /home/biqu/klippy-env/bin/python /home/biqu/klipper/klippy/klippy.py /home/biqu/printer_data/config/printer.cfg -I /home/biqu/printer_data/comms/klippy.serial -l /home/biqu/printer_data/logs/klippy.log -a /home/biqu/printer_data/comms/klippy.sock
2024-06-24 13:29:31,660 - DEBUG - - moonraker (PID: 837): /home/biqu/moonraker-env/bin/python /home/biqu/moonraker/moonraker/moonraker.py -d /home/biqu/printer_data
2024-06-24 13:29:31,661 - DEBUG - - crowsnest (PID: 1728): /bin/bash /usr/local/bin/crowsnest -c /home/biqu/printer_data/config/crowsnest.conf
2024-06-24 13:29:31,661 - DEBUG - - crowsnest (PID: 2008): xargs /home/biqu/crowsnest/bin/ustreamer/src/ustreamer.bin
2024-06-24 13:29:31,662 - DEBUG - - crowsnest (PID: 2009): /bin/bash /usr/local/bin/crowsnest -c /home/biqu/printer_data/config/crowsnest.conf
2024-06-24 13:29:31,662 - DEBUG - - crowsnest (PID: 2010): /home/biqu/crowsnest/bin/ustreamer/src/ustreamer.bin --host 127.0.0.1 -p 8080 -d /dev/video0 --device-timeout=2 -m MJPEG --encoder=HW -r 2592x1944 -f 5 --allow-origin=* --static /home/biqu/crowsnest/ustreamer-www
2024-06-24 13:29:31,663 - DEBUG -
Found config files:
2024-06-24 13:29:31,663 - DEBUG - - /home/biqu/printer_data/config/crowsnest.conf
2024-06-24 13:29:31,664 - DEBUG - - /home/biqu/printer_data/config/printer.cfg
2024-06-24 13:29:31,665 - DEBUG - - /home/biqu/printer_data/config/moonraker.conf
2024-06-24 13:29:31,665 - DEBUG - - /home/biqu/printer_data/config/crowsnest.conf
2024-06-24 13:29:31,666 - DEBUG - - /home/biqu/printer_data/config/crowsnest.conf
2024-06-24 13:29:31,781 - DEBUG - Found Moonraker port in /home/biqu/printer_data/config/crowsnest.conf: 8080                              # HTTP/MJPG Stream/Snapshot Port
2024-06-24 13:29:31,782 - DEBUG - Found Klipper Unix Domain Socket address in /home/biqu/printer_data/config/moonraker.conf: /home/biqu/printer_data/comms/klippy.sock
2024-06-24 13:29:31,782 - DEBUG - Found Moonraker port in /home/biqu/printer_data/config/moonraker.conf: 7125
2024-06-24 13:29:31,783 - DEBUG - Attempting to connect to Unix Domain Socket: /home/biqu/printer_data/comms/klippy.sock (Attempt 1/3)
2024-06-24 13:29:31,786 - DEBUG - Connected to Unix Domain Socket
2024-06-24 13:29:31,789 - DEBUG - Received response: {"id":123,"result":{"objects":["gcode","webhooks","configfile","mcu","gcode_move","print_stats","virtual_sdcard","pause_resume","display_status","gcode_macro CANCEL_PRINT","gcode_macro PAUSE","gcode_macro RESUME","gcode_macro SET_PAUSE_NEXT_LAYER","gcode_macro SET_PAUSE_AT_LAYER","gcode_macro SET_PRINT_STATS_INFO","gcode_macro _TOOLHEAD_PARK_PAUSE_CANCEL","gcode_macro _CLIENT_EXTRUDE","gcode_macro _CLIENT_RETRACT","exclude_object","stepper_enable","tmc2209 stepper_x","tmc2209 stepper_y","tmc2209 stepper_z","tmc2209 stepper_z1","tmc2209 stepper_z2","tmc2209 stepper_z3","tmc2209 extruder","heaters","heater_bed","probe","quad_gantry_level","temperature_host BTT-CB1","temperature_sensor BTT-CB1","temperature_sensor Octopus","fan","heater_fan hotend_fan","controller_fan octopus_controller_fan","fan_generic exhaust_fan","output_pin case_light","gcode_macro lights_toggle","idle_timeout","bed_mesh","filament_switch_sensor filament_sensor","gcode_macro CHECK_ENDSTOPS","gcode_macro G32","gcode_macro mesh_leveling","gcode_macro save_mesh","gcode_macro current_date_time","gcode_macro timer_start","gcode_macro timer_elapsed","gcode_macro PRINT_START","gcode_macro PRINT_END","gcode_macro PRINT_CANCEL","gcode_macro PRINT_CHECK","gcode_macro PRINT_CHECK_TEST","gcode_macro M600","gcode_macro UNLOAD_FILAMENT","gcode_macro LOAD_FILAMENT","gcode_macro NOZZLE_CLEAN","gcode_macro NOZZLE_PRIME","gcode_macro RESONANCES_TEST","gcode_macro END_PRINT","gcode_macro Calibrate_Probe_Offset","gcode_macro GET_TIMELAPSE_SETUP","gcode_macro _SET_TIMELAPSE_SETUP","gcode_macro TIMELAPSE_TAKE_FRAME","gcode_macro _TIMELAPSE_NEW_FRAME","gcode_macro HYPERLAPSE","gcode_macro TIMELAPSE_RENDER","gcode_macro TEST_STREAM_DELAY","motion_report","query_endstops","system_stats","manual_probe","toolhead","extruder"]}}
2024-06-24 13:29:31,790 - DEBUG - Parsed response: {'id': 123, 'result': {'objects': ['gcode', 'webhooks', 'configfile', 'mcu', 'gcode_move', 'print_stats', 'virtual_sdcard', 'pause_resume', 'display_status', 'gcode_macro CANCEL_PRINT', 'gcode_macro PAUSE', 'gcode_macro RESUME', 'gcode_macro SET_PAUSE_NEXT_LAYER', 'gcode_macro SET_PAUSE_AT_LAYER', 'gcode_macro SET_PRINT_STATS_INFO', 'gcode_macro _TOOLHEAD_PARK_PAUSE_CANCEL', 'gcode_macro _CLIENT_EXTRUDE', 'gcode_macro _CLIENT_RETRACT', 'exclude_object', 'stepper_enable', 'tmc2209 stepper_x', 'tmc2209 stepper_y', 'tmc2209 stepper_z', 'tmc2209 stepper_z1', 'tmc2209 stepper_z2', 'tmc2209 stepper_z3', 'tmc2209 extruder', 'heaters', 'heater_bed', 'probe', 'quad_gantry_level', 'temperature_host BTT-CB1', 'temperature_sensor BTT-CB1', 'temperature_sensor Octopus', 'fan', 'heater_fan hotend_fan', 'controller_fan octopus_controller_fan', 'fan_generic exhaust_fan', 'output_pin case_light', 'gcode_macro lights_toggle', 'idle_timeout', 'bed_mesh', 'filament_switch_sensor filament_sensor', 'gcode_macro CHECK_ENDSTOPS', 'gcode_macro G32', 'gcode_macro mesh_leveling', 'gcode_macro save_mesh', 'gcode_macro current_date_time', 'gcode_macro timer_start', 'gcode_macro timer_elapsed', 'gcode_macro PRINT_START', 'gcode_macro PRINT_END', 'gcode_macro PRINT_CANCEL', 'gcode_macro PRINT_CHECK', 'gcode_macro PRINT_CHECK_TEST', 'gcode_macro M600', 'gcode_macro UNLOAD_FILAMENT', 'gcode_macro LOAD_FILAMENT', 'gcode_macro NOZZLE_CLEAN', 'gcode_macro NOZZLE_PRIME', 'gcode_macro RESONANCES_TEST', 'gcode_macro END_PRINT', 'gcode_macro Calibrate_Probe_Offset', 'gcode_macro GET_TIMELAPSE_SETUP', 'gcode_macro _SET_TIMELAPSE_SETUP', 'gcode_macro TIMELAPSE_TAKE_FRAME', 'gcode_macro _TIMELAPSE_NEW_FRAME', 'gcode_macro HYPERLAPSE', 'gcode_macro TIMELAPSE_RENDER', 'gcode_macro TEST_STREAM_DELAY', 'motion_report', 'query_endstops', 'system_stats', 'manual_probe', 'toolhead', 'extruder']}}
2024-06-24 13:29:31,791 - DEBUG - API request succeeded
Available objects:
  - gcode
  - webhooks
  - configfile
  - mcu
  - gcode_move
  - print_stats
  - virtual_sdcard
  - pause_resume
  - display_status
  - gcode_macro CANCEL_PRINT
  - gcode_macro PAUSE
  - gcode_macro RESUME
  - gcode_macro SET_PAUSE_NEXT_LAYER
  - gcode_macro SET_PAUSE_AT_LAYER
  - gcode_macro SET_PRINT_STATS_INFO
  - gcode_macro _TOOLHEAD_PARK_PAUSE_CANCEL
  - gcode_macro _CLIENT_EXTRUDE
  - gcode_macro _CLIENT_RETRACT
  - exclude_object
  - stepper_enable
  - tmc2209 stepper_x
  - tmc2209 stepper_y
  - tmc2209 stepper_z
  - tmc2209 stepper_z1
  - tmc2209 stepper_z2
  - tmc2209 stepper_z3
  - tmc2209 extruder
  - heaters
  - heater_bed
  - probe
  - quad_gantry_level
  - temperature_host BTT-CB1
  - temperature_sensor BTT-CB1
  - temperature_sensor Octopus
  - fan
  - heater_fan hotend_fan
  - controller_fan octopus_controller_fan
  - fan_generic exhaust_fan
  - output_pin case_light
  - gcode_macro lights_toggle
  - idle_timeout
  - bed_mesh
  - filament_switch_sensor filament_sensor
  - gcode_macro CHECK_ENDSTOPS
  - gcode_macro G32
  - gcode_macro mesh_leveling
  - gcode_macro save_mesh
  - gcode_macro current_date_time
  - gcode_macro timer_start
  - gcode_macro timer_elapsed
  - gcode_macro PRINT_START
  - gcode_macro PRINT_END
  - gcode_macro PRINT_CANCEL
  - gcode_macro PRINT_CHECK
  - gcode_macro PRINT_CHECK_TEST
  - gcode_macro M600
  - gcode_macro UNLOAD_FILAMENT
  - gcode_macro LOAD_FILAMENT
  - gcode_macro NOZZLE_CLEAN
  - gcode_macro NOZZLE_PRIME
  - gcode_macro RESONANCES_TEST
  - gcode_macro END_PRINT
  - gcode_macro Calibrate_Probe_Offset
  - gcode_macro GET_TIMELAPSE_SETUP
  - gcode_macro _SET_TIMELAPSE_SETUP
  - gcode_macro TIMELAPSE_TAKE_FRAME
  - gcode_macro _TIMELAPSE_NEW_FRAME
  - gcode_macro HYPERLAPSE
  - gcode_macro TIMELAPSE_RENDER
  - gcode_macro TEST_STREAM_DELAY
  - motion_report
  - query_endstops
  - system_stats
  - manual_probe
  - toolhead
  - extruder
Time taken to get initial response: 0.387059 seconds
2024-06-24 13:29:31,793 - DEBUG - Sent JSON-RPC request to subscribe to printer objects
Toolhead Position: X=152.30, Y=215.93, Z=4.82
Toolhead Position: X=191.57, Y=222.32, Z=4.82
Toolhead Position: X=197.32, Y=183.43, Z=4.81
Toolhead Position: X=158.43, Y=177.68, Z=4.82
Toolhead Position: X=152.68, Y=216.57, Z=4.82
```

## klippertest.c

- Makefile provided
- Uses jansson library to provide JSON support for C programs
- Written to C17 standards like all the other Philement projects
- A queue library was created to help with integrating this in other projects

Sample output.

```bash
$ ./klippertest
Starting Klipper Proxy
Initializing Klipper connection
Searching for Klipper socket in /tmp
Could not find Klipper socket in /tmp
Searching for Moonraker config file
Checking path: /home/biqu/.config/moonraker.conf
Checking path: /home/biqu/klipper_config/moonraker.conf
Checking path: /home/biqu/printer_data/config/moonraker.conf
Found Moonraker config file: /home/biqu/printer_data/config/moonraker.conf
Parsing config file: /home/biqu/printer_data/config/moonraker.conf
Found klippy_uds_address: /home/biqu/printer_data/comms/klippy.sock

Attempting to connect to socket: [/home/biqu/printer_data/comms/klippy.sock]
Successfully connected to Klipper socket
Klipper connection initialized successfully
Received response:
{
  "id": 1,
  "result": {
    "objects": [
      "gcode",
      "webhooks",
      "configfile",
      "mcu",
      "gcode_move",
      "print_stats",
      "virtual_sdcard",
      "pause_resume",
      "display_status",
      "gcode_macro CANCEL_PRINT",
      "gcode_macro PAUSE",
      "gcode_macro RESUME",
      "gcode_macro SET_PAUSE_NEXT_LAYER",
      "gcode_macro SET_PAUSE_AT_LAYER",
      "gcode_macro SET_PRINT_STATS_INFO",
      "gcode_macro _TOOLHEAD_PARK_PAUSE_CANCEL",
      "gcode_macro _CLIENT_EXTRUDE",
      "gcode_macro _CLIENT_RETRACT",
      "exclude_object",
      "stepper_enable",
      "tmc2209 stepper_x",
      "tmc2209 stepper_y",
      "tmc2209 stepper_z",
      "tmc2209 stepper_z1",
      "tmc2209 stepper_z2",
      "tmc2209 stepper_z3",
      "tmc2209 extruder",
      "heaters",
      "heater_bed",
      "probe",
      "quad_gantry_level",
      "temperature_host BTT-CB1",
      "temperature_sensor BTT-CB1",
      "temperature_sensor Octopus",
      "fan",
      "heater_fan hotend_fan",
      "controller_fan octopus_controller_fan",
      "fan_generic exhaust_fan",
      "output_pin case_light",
      "gcode_macro lights_toggle",
      "idle_timeout",
      "bed_mesh",
      "filament_switch_sensor filament_sensor",
      "gcode_macro CHECK_ENDSTOPS",
      "gcode_macro G32",
      "gcode_macro mesh_leveling",
      "gcode_macro save_mesh",
      "gcode_macro current_date_time",
      "gcode_macro timer_start",
      "gcode_macro timer_elapsed",
      "gcode_macro PRINT_START",
      "gcode_macro PRINT_END",
      "gcode_macro PRINT_CANCEL",
      "gcode_macro PRINT_CHECK",
      "gcode_macro PRINT_CHECK_TEST",
      "gcode_macro M600",
      "gcode_macro UNLOAD_FILAMENT",
      "gcode_macro LOAD_FILAMENT",
      "gcode_macro NOZZLE_CLEAN",
      "gcode_macro NOZZLE_PRIME",
      "gcode_macro RESONANCES_TEST",
      "gcode_macro END_PRINT",
      "gcode_macro Calibrate_Probe_Offset",
      "gcode_macro GET_TIMELAPSE_SETUP",
      "gcode_macro _SET_TIMELAPSE_SETUP",
      "gcode_macro TIMELAPSE_TAKE_FRAME",
      "gcode_macro _TIMELAPSE_NEW_FRAME",
      "gcode_macro HYPERLAPSE",
      "gcode_macro TIMELAPSE_RENDER",
      "gcode_macro TEST_STREAM_DELAY",
      "motion_report",
      "query_endstops",
      "system_stats",
      "manual_probe",
      "toolhead",
      "extruder"
    ]
  }
}
Time taken: 0.004201 seconds
Received subscription response:
{
  "params": {
    "eventtime": 51520.876362483003,
    "status": {
      "toolhead": {
        "estimated_print_time": 51521.857229845235
      }
    }
  }
}
Waiting for updates (press Ctrl+C to stop)...
Received update:[157.684,181.239,5.4156002691134102,37266.472640005231]
Received update:[190.791,220.10900000000001,5.4156506234878981,37267.82047000523]
Received update:[158.221,180.82400000000001,5.4157249316966194,37269.168280005229]
Received update:[191.297,219.65799999999999,5.4152116038433444,37270.516080005233]
Received update:[158.703,180.34200000000001,5.4158667163213785,37271.863880005236]
```
