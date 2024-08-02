# calcim - Optimization Wizard
With modern 3D printers, there's an enormous ecosystem of parts and modifications that can help improve the print speed, flow rate, quality, or any number of other metrics. 
Finding the right modifications to hit performance targets, or estimating what performance targets might be hit with different configuration changes, is an interesting puzzle.

## Brainstorming
- Read in the currnet Klipper config to figure out what is on hand.
- Use a benchy G-code file as a way to calculate a print time.
- Use the Boron database to see what parts are compatible
- Some kind of linkage to show how to get from config1 to config2
- Recalculate benchy time to see performance metric changes
- Figure out what parameters in printer.cfg are most important
- Figure out what other parameters not in printer.cfg are most important (eg: filament)
- Incorporate some kind of cost+effort into the model
  
