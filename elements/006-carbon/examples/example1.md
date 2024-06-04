# carbon - Example One
In this example, a single run of the carbon app using two very similar images is performed. The images are taken only a few seconds apart, so the only real difference is in the digital displays on the left. These are images captured by the Klipper Timelapse module at their original native resolution of 2592x1944 from a common 4K webcam.

| Image 1 | Image 2|
|---------|--------|
|<img src=frame000001.jpg> |<img src=frame000002.jpg>|

For this example, a precision of 30 is being used. This means that the width of the image is divided up into 30 blocks. The aspect ratio of the image determines how many blocks are ultimately used. As they are square, for this image, there will be 23 blocks. So the analysis is done over this array of 30x23 blocks. First, the content of each block is reduced to an RGB triple (0-255). The profile is then overlaid to mask any parts of the images that need to be blocked. In this case, we'd like to block the digital displays on the left as they will always be different. The toolhead is also blocked out, just as an example. 

```
profile30.txt
NNNNNPPPPPPPPXXXXXXPPPPPPPPPPP
NNNNNPPPPPPPPXXXXXXPPPPPPPPPPP
NNNNNPPPPPPPPXXXXXXPPPPPPPPPPP
NNNNNPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNPPPPPPPPPPPPPPPPPPPPPPPPP
NXXXXPPPPPPPPPPPPPPPPPPPPPPPPP
NXXXXPPPPPPPPPPPPPPPPPPPPPPPPP
NXXXXPPPPPPPPPPPPPPPPPPPPPPPPP
NXXXXPPPPPPPPPPPPPPPPPPPPPPPPP
NXXXXPPPPPPPPPPPPPPPPPPPPPPPPP
NXXXXPPPPPPPPPPPPPPPPPPPPPPPPP
NXXXXPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNPPPPPPPPPPPPPPPPPPPPPPPPP
```
Running carbon against these images then produces the following output.
```
$ ./carbon frame000001.jpg frame000002.jpg 30 10 profile30.txt
Input Parameters:
- Image 1: frame000001.jpg
- Image 2: frame000002.jpg
- Precision: 30
- Tolerance: 10
- Profile: profile30.txt
Image 1: JPG (2592 x 1944)
Image 2: JPG (2592 x 1944)
Block size: 87 x 87
Block array: 30 x 23
Block size: 87 x 87
Block array: 30 x 23
Image 1:
░░░░░▒▒▓█████      ████▓▒▒▒▒░░
░░░░░▒▒▓▓████      ████▓▒▒▒▒░░
░░░░░▒▒▓▓████      ████▓▒▒▒▒░░
░░░░░▒▒▒▓▓█████▒▒█████▓▒▒▒▒▒░░
░░░░░▒▒▒▒▓█████▓▓████▓▒▒▒▒▒░░░
░░ ░░▒▒▒▒▒▓▓▓▓▓▓▓▓█▓▓▒▒▒▒▒▒░░░
░░  ░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░
░░ ░░░░░░░▒▒▒▒▒▒▒▒▒▒░▒▒░░░░░░░
 ░ ░░░░░░░░░░▒░▒░░░░░░░░░░░░░░
   ░░░░░░░░░░░░░░░░░░░░░░░░░░░
   ░░░░░░░░░░░░░░░░░░░░░░░░░░
  ░░░░░░░░░░░░░░░░░░░░░░░░░░░
░    ░░░░░░░░░░░░░░░░░░░░░░░░
░    ░░░░░░░░░░░░░░░░░░░░░░░░
░    ░░░░░░░░░░░░░░░░░░░░░░░░
░    ░░    ░░░░░░░░░░░░ ░░░░░
░    ░░    ░░░░░░░░ ░░░░░░░░░
     ░░     ░░░░░░░░░   ░░░░░
     ░░░░░░ ░░ ░░░░      ░░
  ░░░  ░░░░
          ░                ░
                          ░░░
                           ░

Image 2:
░░░░░▒▒▓█████      ████▓▒▒▒▒░░
░░░░░▒▒▓▓████      ████▓▒▒▒▒░░
░░░░░▒▒▓▓████      ████▓▒▒▒▒░░
░░░░░▒▒▒▓▓█████▒▒█████▓▒▒▒▒▒░░
░░░░░▒▒▒▒▓█████▓▓████▓▒▒▒▒▒░░░
░░ ░░▒▒▒▒▒▓▓▓▓▓▓▓▓█▓▓▒▒▒▒▒▒░░░
░░  ░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░
░░ ░░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░
 ░ ░░░░░░░░░░▒░▒░░░░░░░░░░░░░░
   ░░░░░░░░░░░░░░░░░░░░░░░░░░░
   ░░░░░░░░░░░░░░░░░░░░░░░░░░
  ░░░░░░░░░░░░░░░░░░░░░░░░░░░
░    ░░░░░░░░░░░░░░░░░░░░░░░░
░    ░░░░░░░░░░░░░░░░░░░░░░░░
░    ░░░░░░░░░░░░░░░░░░░░░░░░
░    ░░░░  ░░░░░░░░░░░░ ░░░░░
░    ░░    ░░░░░░░░░░░░░░░░░░
     ░░     ░░░░░░░░░   ░░░░░
     ░░░░░░ ░░ ░░░░ ░    ░░
  ░░░  ░░░░
          ░                ░
                          ░░░
                           ░

Comparison Grid:
0000000000000      00000000000
0000000000000      00000000000
0000000000000      00000000000
000000000000000000000000000000
000000000000000000000000000000
000000000000000000000000000000
000000000000000000000000000000
000000000000000000000000000000
000000000000000000000000000000
000000000000000000000000000000
000000000000000000000000000000
000000000000000000000000000000
0    0000000000000000000000000
0    0000000000000000000000000
0    0000000000000000000000000
0    0000000000000000000000000
0    0000000000000000000000000
0    0000000000000000000000000
0    0000000000000000000000000
000000000000000000000000000000
000000000000000000000000000000
000000000000000000000000000000
000000000000000000000000000000

Summary:
- 0-blocks: 644
- X-blocks: 46
Assessment: Pass
Print job appears to be progressing normally.
Execution Time: 138.23 ms
```
The result is that we can see a representation of each image showing the block calculations. It isn't the RGB representation that it calculates internally, but enough to show that the image is indeed being parsed correctly, and that the profile is being correctly applied.

In the comparison grid, everything matches perfectly, as we'd expect, as the only areas of the image that are different have been excluded. The final result is that the assessment is a "Pass" meaning that no printing fault was found.

Note: When running in a proper terminal, the above are presented with a bit of color that doesn't quite make it through the GitHub Markdown formatting filter.
