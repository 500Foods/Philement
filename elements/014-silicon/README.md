# silicon - Experimental Printer: Voron 2.4r2 without an MCU
Using a microcontroller as an interface to motor drivers has been a standard in 3D printing for some time. This project aims to remove the MCU, but not really. 
Instead, each motor is equipped with its own MCU - something like an MKS-SERVO42D. This is a closed-loop CAN bus controller that is fitted to the back of a NEMA17 motor.
With this approach, the MCU is essentially split up into separate simpler parts and the main MCU can be removed. The whole thing can then be driven by a Raspberry Pi.

## Brainstorming
- The Klipper team doesn't want to be a "Pi-only" effort but its still an idea worth exploring. Just not with Klipper.
- The "electronics bay" now consists of a Pi and a power supply, for the most part.
- Still have to wire up extra stuff not on the CAN bus, or get them onto the CAN bus.
- Starting from a Voron 2.4r2, replace the MCU with 6 motors equipped with the CAN bus controller.
- Provides a solid basis for comparison in terms of performance, reliability, quality, and so on.
- Simplifies the project to almost a Pi-only situation, with the toolboard and motor MCUs being much much simpler.
- Pi doesn't even have to be in the printer, nor does the power supply, technically.
- Smaller, cheaper, faster is the goal. Need to find less expensive MKS-SERVO42D's or equivalents.
- This is an experiment - it may fail miserably. But no doubt something will be learned along the way.
  
