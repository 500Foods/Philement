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
- Tools available as standalone as well for testing/config.
- UI from Lithium for selecting image layers.
  
## First Draft
To get the ball rolling, the first draft uses captured timelapse images and compares them to see if there's something amiss. There are a lot of assumptions being made here.
- The camera view is directly overhead and fixed relative to the print bed, which presumably is fixed as well.
- The toolhead is in the same position in each image, and anything running to the toolhead (filament, cabling, etc.) is either out-of-frame or in a fixed position as well.
- The user has identified which parts of the image are the print area, the non-print area, and that should be excluded (visible timers, parked toolhead, edges, etc.)

So this can be very limiting/ Future versions might do this very differently of course. But this means that this isn't going to work particularly well for things like nozzle cams, or anything other than fixed-bed
printers at the moment. But if you've got a Voron 2.4 or a Troodon, this might be workable.
