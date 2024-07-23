# potassium
This project is concerned primarily with overall power monitoring. 
In order to extract the most value from a 3D printer it has to be, well, printing. And with some print jobs extending to a day or more, having a printer running 24/7 isn't particularly uncommon. 
Over time, this can add significantly to the power consumption of the home or office where the printer is running. This doesn't mean that it will be expensive to run - maybe the equivalent of running a couple of 100W lightbulbs continuously. 

To give some perspective, let's consider a cost of $0.10/kWh of electricity. A 100W light bulb running for a year would then consume (100W x 24h x 365d) = $87.60 per year, or $7.30 per month. 

This is not likely to be a huge concern given that the cost of filament will be more. For a back-of-the-napkin calculation, let's say it takes 20 minutes to print a benchy, and that it uses 12g of filament. Let's assume filament costs $20/kg.
That works out to about 36g/hour or $0.72/hour. Extrapolating, this would be $6,307.20/year or $525.60/month. Which also works out to just over 26 rolls per month, so nearly a roll of filament per day.

Still, electricity is a shared resource for all of us, so minimizing our use, or at least being aware of it, goes a long way towards being responsible about it.

## Brainstorming
- Use a clip-on current meter to monitor the complete power consumption of the printer
- Use an andalog-to-digital converter to get this into an i2c interface and into a computer
- Track usage at 1m, 1h, 1d intervals.
- Log in a database or other system for long-term storage
- Make graphs available to show usage over time.
- Combine with markers where possible (print started, print ended, heat soak started, etc.)
- Option to run standalone, so as to be fed into another display or system like Home Assistant
- App should be as efficient as possible. No python if it can be avoided.
- Potentially deal with multiple sensors, multiple voltage levels, etc.
- The analog-to-digital board is likely to have multiple inputs - main power, bed heater, etc.
- Got to be inexpensive if anyone is going to use it.
