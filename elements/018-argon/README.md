# argon - Filament Extruder
Waste is an inevitable part of production, whether it is 3D printing, CNC milling, or pretty much anything else. In the case of 3D printing though, often the waste plastic can be recycled immediately and turned back into filament ready to be used again. 
For some plastics, in fact, this can be done repeatedly without loss of quality. For others, the strength of subsequent prints will be reduced, but even this degraded filament can still be useful for support materials or cosmetic parts that don't require the same strength.

## Brainstorming
- Generally speaking, recycling waste plastic in a home setting hasn't been remotely cost effective.
Shredder-style machines to break down the prints typically *start* at $1,000 USD.
Machines to convert the resulting particles tend to be another machine, starting also at $1,000 USD or often much more.
- Everyone likes to do the math, where it costs, say, $50/roll to recylce, and $20/roll to buy new, so why bother?
- This is short-sighted though, as without trying new things, the status quo will likely never change.
- Two main parts to the problem - breaking down waste material and then re-extruding the broken down pieces.
- 2200W blender seems to work *really well* at producing < 2mm particles from ASA print waste, so that's 1/2 the problem solved!
- A typical 3D printer converts 1.75mm filament into something like 0.4mm filament.
- Let's build a similar extruder using the same approach, converting 25mm cylinders into 1.75mm filament.
- 25mm cylinders can be created using candlestick molds. Fill with 2mm particles, melt in the oven, get a 30cm "candle".
- Probably less than 30cm given that filament will melt. Maybe 20cm candles when it is done.
- Could likely work with much larger particles, so long as they fit in the mold and melt evenly.
- Feed these candles into an extruder with a 1.75mm nozzle and a 25mm filament path.
- Will take a while to extrude, so just need to add a new candle to the extruder every so often.
- Could also "join" the candles together to make a 1kg cylinder. Note that 1kg would be about 2m at 25mm diameter (for, say ASA).
- That'd be about 10 candles to produce a roll of filament. Probably want to start with that many candle molds anyway.
- Output of extruder can be collected and wound onto spool using common rewinder prints, nothing special there.
- OK, a little special in that you want to cool it right away and be able to monitor the 1.75mm output.
- Should be able to be built in a small and simple form factor, not much bigger than a filament box.
- Filament should be cooled but also dried so it is ready to print as soon as possible.
