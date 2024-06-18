# lvgldemo
The purpose of this program is just to demonstrate that LVGL is working, as the very first step of fashioning our low-level UI, ultimately to replace KlipperScreen or run on an RP2040 with an SPI/I2C touch display. 

For development, this is a C program that uses an SDL driver on Linux, which should work well enough. Tested on Fedora/KDE/X11 and while it took a bit to get it sorted, it ultimately worked. A future test will run on a BTT-CB1 and
an HDMI display (a Troodon printer) as well as an RP2040.

## Dependencies
As low-level as possible was the goal here, so just straight-up LVGL and the SDL driver are all that is required.

## Files
Not very many files are needed as this is really just a test.

- Makefile - Compiles and builds lvgldemo and sdltest
- libs/lvgl - Clone the LVGL repository from GitHub
- src/main.c - The main() component of lvgldemo
- src/display.h - Header for lvgldemo
- src/display.c - Implementation for lvgldemo
- src/lv_conf.h - Contains configuration settings for LVGL
- src/sdltest.c - Contains the sdltest implementation

## Additional Notes
- Like other projects, this is a C program compiled with -std=C17
- If you change src/lv_conf.h be sure to do a 'make clean && make' to pick up the changes
- Fonts need to be included with a #define before they can be used
- Most of the time on this project was spent debugging SDL issues
- Rotating an object in LVGL seems to be broken currently

## Example
This is it. This is all it does. But it is enough to get our proverbial foot in the LVGL door. If you close the window, it should exit normally and not crash.

![image](https://github.com/500Foods/Philement/assets/41052272/fc4655ae-0e7f-413e-8047-107302280c9e)
