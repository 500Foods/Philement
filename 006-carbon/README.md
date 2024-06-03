# carbon
The idea here is about monitoring a print or other activity, and taking actions when failures are detected. Might be as simple as pausing a print. This is the main reason to use Obico at the moment (spaghetti detection as they like to call it).

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
  
