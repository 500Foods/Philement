# sulfur - Printer Experiment: Robotic Arm
3D printing has been largely a 2.5D affair. Moving a toolhead around in a 3D volume, but with just three degrees of motion. Non-planar 3D printers exist, mostly as small experimental projects. At the same time, robotic arms have become increasingly accessible with 
better designs, more 3D printed parts, and big steps in terms of repeatability and precision, just on the cusp of what is needed for consistent 3D printing. And a robot arm is, from an electronics perspective, nearly identical to a 3D printer. Even the same number of motors
in a lot of cases. So let's build one using the same motors and the same ideas and see if it can work.

## Brainstorming
- This is not a new idea at all. Lots of examples, but somehow nothing really shipping as a product.
- Build off what is learned in the silicon and phosphorus projects.
- Robotic arm base should be moved around a track (straight or circular) by a rack-and-pinion mechanism, for example.
- CAN bus and MKS SERVO42D controllers like the silicon project. Maybe beefier NEMA 23 equivalents.
- End actuator should be a 3D printer toolhead that can point in more than one direction.
- Same goal as phosphorus in terms of different toolheads - 3D printer, CNC router, maybe painting of some kind.
- Design with coordinating multiple arms in mind from the beginning.
- Lots of safety considerations.
- Simliar coffee table design. Park an arm on the corner and let it do its thing.
- Build a smaller mini arm as a controller. Lots of YouTube videos about that.
- Use closed loop motors to help with repeatability and precision.
- Explore how a camera system might also be able to help with calibration.
- Lay out QR codes or something like that as an idea for an analog for limit switches.
  
