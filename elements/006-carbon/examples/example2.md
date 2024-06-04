# carbon - Example Two
In this example, a single run of the carbon app using the same two images is performe again, but with a precision of 100.

| Image 1 | Image 2|
|---------|--------|
|<img src=frame000001.jpg> |<img src=frame000002.jpg>|

For this example, a precision of 100 is used. This means that the width of the image is divided up into 100 blocks. 
The aspect ratio of the image determines how many blocks are ultimately used. 
As they are square, for this image, there will be 75 vertical blocks. 
So the analysis is done over this array of 100x75 blocks. 
First, the content of each block is reduced to an RGB triple (0-255). 
The profile is then overlaid to mask any parts of the images that need to be blocked. 

In this example, the profile excludes the bottom perimeter but not the digital displays. 
The left side is marked as non-print area, meaning that any changes here are counted at a higher precedence.
And the tolerance is changed from the default of 10 down to 2, meaning almost any variation will get flagged.

```
profile100.txt
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
NNNNNNNNNNNNNNNNNNXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
```
Running carbon against these images then produces the following output.
```
$ ./carbon frame000001.jpg frame000002.jpg 100 2 profile100.txt
Input Parameters:
- Image 1: frame000001.jpg
- Image 2: frame000002.jpg
- Precision: 100
- Tolerance: 2
- Profile: profile100.txt
Image 1: JPG (2592 x 1944)
Image 2: JPG (2592 x 1944)
Block size: 26 x 26
Block array: 100 x 75
Block size: 26 x 26
Block array: 100 x 75
Image 1:
░░░░░░░░░░░░░░░░░▒▒▒▒▒▒▒▓▓▓▓████████████████████▒░░░░░░░░░░░░░▒▓█████████████▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░
░░░░░░░░░░░░░░░░░▒▒▒▒▒▒▒▓▓▓▓████████████████████▒░░░        ░░▒███████████████▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░
░░░░░░░░░░░░░░░░░▒▒▒▒▒▒▒▓▓▓▓████████████████████▒░░░       ░░░▓██████████████▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░
░░░░░░░░░░░░░░░░░▒▒▒▒▒▒▒▓▓▓▓████████████████████▓░░░░░░░░░░░░▒███████████████▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░
░░░░░░░░░░░░░▒░░░▒▒▒▒▒▒▒▓▓▓▓▓████████████████████░░░░     ░░░████████████████▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░
░░░░░░░░░░░░░░░░░▒▒▒▒▒▒▒▓▓▓▓▓████████████████████▒░         ▒████████████████▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░
░░░░░░░░░░░░░░░░░▒▒▒▒▒▒▒▓▓▓▓▓████████████████████▓░        ░▓███████████████▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░
░░░░░░░░░░░░░░░░░▒▒▒▒▒▒▒▒▓▓▓▓▓████████████████████▒░       ▒████████████████▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░
░░░░░░░░░░░░░░░░░▒▒▒▒▒▒▒▒▓▓▓▓▓████████████████████▓▒░    ░▒████████████████▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░
░░░░░░░░░░░░░░░░░▒▒▒▒▒▒▒▒▒▓▓▓▓█████████████████████▓▒░  ░▒▓█████████████████▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░
░░░░░░░░░░░░░░░░░▒▒▒▒▒▒▒▓▒▓▓▓▓█████████████████████▓▒░░░▒▓████████████████▓▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░
░░░░░░░░░░░░░░░░░▒▒▒▒▒▒▒▒▒▒▓▓▓▓▓██████████████████▓▓▒▒▒▒▒▓███████████████▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░
░░░░░░░░░░░░░░░░░▒▒▒▒▒▒▒▒▒▒▒▓▓▓▓▓█████████████████▓▒▒▒▒▒▒▓██████████████▓▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░░
░░░░░░░░░ ░░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▓▓▓▓████████████████▓▓▒▒▒▒▒▒▓▓████████████▓▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░░
░░░░░░░░░░░░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▓▓▓▓▓███████████████▓▓▒▒▒▒▒▓▓▓███████████▓▓▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░
░░░░░░░░░░░░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▓▓▓▓▓▓▓██████████████▓▓▓▓▓▓▓▓███████████▓▓▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░░
░░░░░░░░ ░░░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▓▓▓▓▓▓▓███████████████████████████████▓▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░░░░
░░░░░░░ ░░░ ░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▓▓▓▓▓▓▓▓███████████████████████████▓▓▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░░░░
░░░░░░░     ░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▓▒▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓█▓█████▓▓▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░░░░
 ░░░░░░     ░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▓▓▓▓▓▓▓▓▓▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░░░░░
 ░░░░░░     ░░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░░░░░
 ░░░░░░     ░░░░░░░░▒▒░░▒▒░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░▒░░░░░░░░░░░░
 ░░░░░░  ░  ░░░░░░░░░░░░░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░▒░░░░░░░░░░░░
  ░░░░░     ░░░░░░░░░░░░░░░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒█▒▒░░░░░░░░░░░░░░░░░░░░░░
  ░░░░░  ░░░░░░░░░░░▒░░░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░▒▒░░░░░░░░░░░░░░░░░░░░░░░░
  ░░░░░     ░░░░░░░░▒░░░░░░░░░░░░░▒░░▒░░▒▒▒▒░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░▒▒▒▒░░▒▒▒░░░░▒░░░░░░░░░░░░░░░░░░░░░░
  ░░░░░     ░░░░░░░░▒░░░░░░░░░░░░░░░░░░░▒▒▒░░▒▒░░░░░░░░░░░░░▒░░░░░▒░░░░▒░░░░░░░░░░░░░░░░░░░░░░░░░░░░
  ░░░░░    ░░░░░░░░░▒░░░░░░░░░░░░░░░░░░▒▒░░░░▒▒░░░▒▒░░░░▒░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
  ░░░░░░  ░░░░░░░░░░▒░░░░░░░░░░░░░░░░░░▒▒░░░░▒░░░░▒▒░░░░▒▒▒░░░▒▒▒░░░▒▒░░░░▒▒░░░░▒▒░░░░░░░░░░░░░░░░░░
  ░░░░░░░░░░░░░░░░░░▒░░░░░░░░░░░░░░░░░▒▒▒░░░▒▒▒░░░▒▒░░░░▒▒░░░░▒▒░░░░▒░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
  ░░    ░░░░░░░░░░░░▒░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░▒░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
  ░░     ░ ░░░░░░░░░▒░░░░░░░▒▒░░▒▒░░░░░░░░░░░░░░░░░░░▒░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
  ░░     ░ ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░▒░░░░▒▒░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
  ░░     ░ ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░▒▒░░░▒░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ ░░
  ░░     ░ ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░▒░░░░▒░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ ░
  ░░     ░ ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
  ░░     ░ ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░  ░
  ░░     ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
  ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
  ░░    ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ ░░░░░░░░░░░░░░░░░░░░░░░ ░
  ░░      ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░▒░░░░░░░░░░░░░░░░░░░░ ░░░░  ░░░░░░░░░░░░░░░░ ░
░ ░▒▓▓▓▓▓▓▓▓▓▓▓▓▓▒░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░▒░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ ░░
░ ▒▓█████████████▒░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ ░░░░  ░░░░░░░░░░░░░  ░
░░▓▒█████████████▒░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ ░░░░ ░░░░░░░░░░░░░  ░
░░▒░░░░░░░░░░░░▒▒▒░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ ░░░░░ ░░░░░░░░░░░░░░░░░░░
░░▒░░░░░░░░░░░░░▒▒░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ ░░░░ ░░░░░ ░░░░░░░░░░░░░░░░  ░
░░▒░░ ░░░░░░░░░░▒▒░░░░░░░░░░░░░░░ ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ ░░░░  ░░░░░░░░░░░░░░░░░
░░▓░░░ ░░░ ░░░░░░▒░░░░░░░░░░░ ░░░ ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ ░░░░ ░░░░░░░░░░░░░░░ ░
░░▓░░░░░░░░░░░░░░▒░░░░░░    ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░  ░░░░  ░ ░  ░░░░░░░░░░░░░  ░
░░▓░░░░░░░░░░░░░▒▒░░░░░░░░░░░░░░░░░░░░ ░░░░░░░░░░░░░░░░░░░░░ ░░░░░░░░░░ ░░░ ░ ░░░ ░░░░░░░░░░░░  ░
░░▓▒▓▓▓██████████▒░░░░░ ░░░░░░░░░░░░░░ ░░░░░░░░ ░ ░░░░░░░░░░ ░░░ ░ ░░░░ ░░░░░ ░░░░░░░░░░░░░░░░░ ░
░░▓▒█████████████▒░░░░░░░░░░░░░   ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ ░░ ░░  ░░░   ░░░░░░░░░░░░░░░░░
░░▓▒█████████████▒░░░░░ ░░░         ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░  ░░░  ░░░░░░░░░░░░░░░░░
░░▓▒█████████████▒░░░░░ ░░░        ░ ░░░░░░░░░░░░░░░░░░░░░░░░░░ ░░ ░  ░░   ░░░   ░░░░░░░░░░░░░░░░
░░▓▒░░▒█▒▒▒▒▓▒▒▒█▒░░░░░ ░░░          ░ ░░░░░ ░░ ░ ░░░ ░░░░ ░ ░░░ ░ ░░ ░ ░░░   ░░  ░░░░░░░░░░░░░░░
░░▓░░░░▓░░░▒▒░░▒█▒░░░░░  ░             ░░░ ░░░░ ░ ░░░ ░░░░   ░░░ ░ ░░ ░ ░░░   ░░  ░░░░░░░░░░░░░░░
░ ▒▒▓▓▓▓▓▓▓▓▓▓▓▓▓░ ░░░░  ░     ░       ░░░ ░░░░ ░ ░░░░░ ░░ ░ ░░░ ░ ░░ ░  ░░ ░  ░░ ░░░░░░░░░░░░░░░
   ░   ░░▒▒▒▒▒▒▒░  ░░░░       ░░     ░░ ░ ░░ ░░░░░░░░░░ ░░░░░ ░░░░   ░░░   ░░    ░░░░░░░░░░░░░░ ░
   ░  ▓█▓▓▓▓▓▓▓█▓░  ░░░              ░░ ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   ░░░   ░░░░░░░░░░░░░  ░
   ░  ▓▒░░░░░░░▒█░  ░░░                 ░  ░ ░░ ░ ░░  ░ ░  ░  ░  ░ ░  ░     ░     ░░░░░░░░░░░░  ░
   ░░ ▓▒░░ ░ ░░▒█░  ░░░░    ░░░░░░░░░░ ░░    ░░   ░░░   ░░   ░░░   ░░    ░░   ░░  ░░ ░░░░░░░░   ░
   ░  ▓▒░    ░░▒█░   ░░░░░░░░░░░░░░░░  ░░    ░░   ░░░ ░ ░░   ░░░ ░ ░░    ░     ░  ░░ ░░░░░░░
   ░  ▓▒░░ ░░░░▒█░   ░░░░░░░░░░░░░░░░░  ░ ░  ░░ ░  ░ ░░ ░░ ░  ░  ░ ░░       ░     ░░ ░░░░ ░
   ░  ▓█▓▓▓▓▓▓▓█▓░   ░░░░░░░░░░░░░░░░░  ░ ░░ ░░░░░ ░░░░░░░░░░ ░░░░░  ░░░   ░░    ░░░░░░░
   ░  ░▓▓▓▓▓▓▓▓▓░       ░░░░ ░░░░░░░░░ ░░ ░░ ░░ ░░░░ ░░ ░  ░░ ░ ░░ ░  ░
   ░                       ░░░░░░░░░░░                  ░     ░
   ░                         ░░░        ░    ░░   ░░    ░░   ░░                            ░░
   ░                           ░░            ░░    ░    ░░    ░                            ░░░
   ░                             ░░     ░    ░░    ░     ░ ░  ░░                          ░░░░
   ░                               ░░                                                     ░░░ ░
   ░                                ░░                                                   ░░░▒░░
   ░                                   ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░        ░░░░░░
   ░                                                                                     ░▒▒▒▒▒
                                                                                         ░▒▒▒▒▒


Image 2:
░░░░░░░░░░░░░░░░░▒▒▒▒▒▒▒▓▓▓▓████████████████████▒░░░░░░░░░░░░░▒▓█████████████▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░
░░░░░░░░░░░░░░░░░▒▒▒▒▒▒▓▓▓▓▓████████████████████▒░░░        ░░▒██████████████▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░
░░░░░░░░░░░░░░░░░▒▒▒▒▒▒▒▓▓▓▓████████████████████▒░░░░      ░░░▓██████████████▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░
░░░░░░░░░░░░░░▒░░▒▒▒▒▒▒▒▓▓▓▓████████████████████▓░░░░░░░░░░░░▒███████████████▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░
░░░░░░░░░░░░░░░░░▒▒▒▒▒▒▒▓▓▓▓▓████████████████████░░░░     ░░░████████████████▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░
░░░░░░░░░░░░░░░░░▒▒▒▒▒▒▒▓▓▓▓▓████████████████████▒░         ▒████████████████▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░
░░░░░░░░░░░░░░░░░▒▒▒▒▒▒▒▓▓▓▓▓████████████████████▓░        ░▓███████████████▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░
░░░░░░░░░░░░░░░░░▒▒▒▒▒▒▒▒▓▓▓▓█████████████████████▒░       ▒████████████████▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░
░░░░░░░░░░░░░░░░░▒▒▒▒▒▒▒▒▓▓▓▓▓████████████████████▓▒░    ░▒████████████████▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░
░░░░░░░░░░░░░░░░░▒▒▒▒▒▒▒▒▓▓▓▓▓█████████████████████▓▒░  ░▒▓█████████████████▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░
░░░░░░░░░░░░░░░░░▒▒▒▒▒▒▒▓▒▓▓▓▓█████████████████████▓▒░░░▒▓████████████████▓▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░
░░░░░░░░░░░░░░░░░▒▒▒▒▒▒▒▒▒▒▓▓▓▓▓███████████████████▓▒▒▒▒▒▓███████████████▓▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░
░░░░░░░░░░░░░░░░░▒▒▒▒▒▒▒▒▒▒▒▓▓▓▓▓█████████████████▓▒▒▒▒▒▒▓██████████████▓▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░
░░░░░░░░░ ░░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▓▓▓▓████████████████▓▓▒▒▒▒▒▒▓▓████████████▓▓▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░
░░░░░░░░░░░░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▓▓▓▓▓███████████████▓▓▒▒▒▒▒▓▓▓████████████▓▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░
░░░░░░░░░░░░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▓▓▓▓▓▓▓██████████████▓▓▓▓▓▓▓▓███████████▓▓▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░░
░░░░░░░░ ░░░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▓▓▓▓▓▓▓███████████████████████████████▓▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░░░
░░░░░░░ ░░░ ░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▓▓▓▓▓▓▓▓███████████████████████████▓█▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░░░░
░░░░░░░     ░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▓▒▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓█▓█████▓▓▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░░░░
 ░░░░░░     ░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▓▒▒▒▓▓▓▓▓▓▓▓▓▓▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░░░░░
 ░░░░░░     ░░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░░░░░
 ░░░░░░     ░░░░░░░░▒▒░░▒▒░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░▒░▒░░░░░░░░░░░░
 ░░░░░░  ░  ░░░░░░░░░░░░░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░▒░░░░░░░░░░░░
  ░░░░░     ░░░░░░░░░░░░░░░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒█▒▒▒░░░░░░░░░░░░░░░░░░░░░
  ░░░░░  ░░░░░░░░░░░▒░░░░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░▒▒░░░░░░░░░░░░░░░░░░░░░░░░
  ░░░░░  ░  ░░░░░░░░▒░░░░░░░░░░░░░▒░░▒░░▒▒▒▒░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░▒▒▒▒░░▒▒▒░░░░▒░░░░░░░░░░░░░░░░░░░░░░
  ░░░░░     ░░░░░░░░▒░░░░░░░░░░░░░░░░░░░▒▒▒░░▒▒░░░░░░░░▒░░░░▒░░░░▒▒░░░░▒░░░░░░░░░░░░░░░░░░░░░░░░░░░░
  ░░░░░    ░░░░░░░░░▒░░░░░░░░░░░░░░░░░░▒▒░░░░▒▒░░░▒▒░░░░▒░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
  ░░░░░░  ░░░░░░░░░░▒░░░░░░░░░░░░░░░░░░▒▒░░░░▒░░░░▒▒░░░░▒▒▒░░░▒▒▒░░░▒▒░░░░▒▒░░░░▒▒░░░░░░░░░░░░░░░░░░
  ░░░░░░░░░░░░░░░░░░▒░░░░░░░░░░░░░░░░░▒▒▒░░░▒▒▒░░░▒▒░▒░░▒▒░░░▒▒▒░░░░▒░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
  ░░    ░░░░░░░░░░░░▒░░░░░░░░░░░░░░░░░░░░░░░░▒░░░░░▒░▒░░░░░░░░░░▒░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
  ░░     ░ ░░░░░░░░░▒░░░░░░░▒▒░░▒▒░░░░░░░░░░░░░░░░░░░▒░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
  ░░     ░ ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░▒░░░░▒▒░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
  ░░     ░ ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░▒▒▒░░░▒░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ ░░
  ░      ░ ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░▒░░░░░▒░░░░▒░░░░░▒░░░░░░▒░░░░░░░░░░░░░░░░░░░░░░░░░░░
  ░      ░ ░░░░░░░░░░░░░░░░░▒░░▒░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
  ░░     ░ ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
  ░░    ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
  ░░░  ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ ░
  ░░    ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ ░░░░░░░░░░░░░░░░░░░░░░░░░
  ░░      ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░▒░░░░░░░░░░░░░░░░░░░░░░░░░ ░░░░░░░░░░░░░░░░░ ░
░ ░▒▓▓▓▓▓▓▓▓▓▓▓▓▓▒░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░▒░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
░ ▒▓█████████████▒░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ ░░░░  ░░░░░░░░░░░░░  ░
░░▒▒█████████████▒░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ ░░░░░░░░░░░░░░░░░░  ░
░░▒░░░░░░░░░░░░▒▒▒░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ ░░░░░ ░░░░░░░░░░░░░░░░ ░░
░░▒░░░░░░░░░░░░░▒▒░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ ░░░░░  ░░░░░░░░░░░░░░░  ░
░░▒░░ ░░░░░░░░░░▒▒░░░░░░░░░░░░░░░ ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ ░░░░  ░░░░░░░░░░░░░░░░░
░░▒░░░ ░░░ ░░░░░░▒░░░░░░░░░░░  ░░ ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ ░░░░ ░░░░░░░░░░░░░░░ ░
░░▒░░░ ░░░░░░░░░░▒░░░░░░    ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ ░░░░  ░ ░  ░░░░░░░░░░░░░  ░
░░▒░░░░░░░░░░░░░▒▒░░░░░░░░░░░░░░░░░░░░ ░░░░░░░░░░░░░░░░░░░░░ ░░░░░░░░░░ ░░░ ░ ░░░ ░░░░░░░░░░░░  ░
░░▒▒▓████████████▒░░░░░ ░░░░░░░░░░░░░░ ░░░░░░░░ ░ ░░░░░░░░░░ ░░░ ░ ░░░░ ░░░ ░ ░░░░░░░░░░░░░░░░  ░
░░▒▒█████████████▒░░░░░ ░░░░░░    ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ ░░ ░░  ░░░   ░░░░░░░░░░░░░░░ ░
░░▓▒█████████████▒░░░░░ ░░░░        ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░  ░░░  ░░░░░░░░░░░░░░░░░
░░▓▒█████████████▒░░░░░ ░░░░   ░ ░ ░ ░░░░░░░░░░░░░░░░░░░░░░░░░░ ░░░░  ░░   ░░░   ░░░░░░░░░░░░░░░░
░░▓▒░░▒█▒▒▒▒▓▒▒▒█▒░░░░░ ░░░          ░ ░░░░░ ░░ ░ ░░░ ░░░░ ░ ░░░ ░ ░░ ░ ░░░   ░░  ░░░░░░░░░░░░░░░
░░▓░░░░▓░░░▒▒░░▒█▒░░░░░ ░░░            ░░░  ░░░ ░ ░░░ ░░░░ ░ ░░░ ░ ░░   ░░░   ░░  ░░░░░░░░░░░░░░░
░ ▒▒▓▓▓▓▓▓▓▓▓▓▓▓▓░ ░░░░  ░    ░░       ░░░ ░░░░ ░ ░░░ ░ ░░ ░ ░░░ ░ ░░    ░░ ░  ░░ ░░░░░░░░░░░░░░░
   ░   ░▒▒▒▒▒▒▒▒░  ░░░░  ░    ░░     ░  ░ ░░ ░░░░░░░░░░ ░░░░░ ░░░░    ░░   ░░    ░░░░░░░░░░░░░░ ░
   ░  ▓█▓▓▓▓▓▓▓█▓░ ░░░░  ░           ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   ░░░   ░░░░░░░░░░░░░  ░
   ░  ▓▒░░░░░░░▒█░ ░░░░              ░  ░  ░ ░░ ░ ░░  ░ ░  ░  ░  ░ ░  ░     ░     ░░░░░░░░░░░░  ░
   ░░ ▓▒░░ ░ ░░▒█░ ░░░░░    ░░░░░░░░░░ ░░    ░░   ░░░   ░░   ░░░   ░░    ░░   ░░  ░░ ░░░░░░░░   ░
   ░  ▓▒░    ░░▒█░  ░░░░░░░░░░░░░░░░░░ ░░    ░░   ░░░   ░░   ░░░   ░░    ░░    ░  ░  ░░░░ ░░
   ░  ▓▒░░ ░░░░▒█░   ░░░░░░░░░░░░░░░░░  ░    ░░ ░ ░░ ░░ ░░ ░  ░  ░ ░░       ░     ░░ ░░░░░░
   ░  ▓█▓▓▓▓▓▓▓█▓░   ░░░░░░░░░░░░░░░░░  ░ ░░ ░░░░░ ░░░░░░░░░░ ░░░░░  ░░░   ░░    ░░░░░░░
   ░  ░▓▓▓▓▓▓▓▓▓░        ░░░░ ░░░░░░░░  ░ ░░ ░░ ░ ░░ ░░ ░  ░░ ░ ░░ ░  ░           ░
   ░                       ░░░  ░ ░░░                   ░     ░
   ░                         ░░░   ░░   ░    ░░   ░░    ░░   ░░                            ░░
   ░                           ░░       ░    ░     ░    ░░    ░                            ░░░
   ░                             ░░     ░    ░░    ░     ░    ░░                          ░░░░
   ░                              ░░░                                                     ░░░ ░
   ░                                 ░                                                   ░░░▒░░
   ░                                  ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░        ░░░░░░
   ░                                                                                     ░▒▒▒▒▒
                                                                                         ░▒▒▒▒▒


Comparison Grid:
1144233233142264310110101210110100000000000000012101112212211111000000000000101110111112111010012022
1111111122221122211110111100000001000000000000011111112112100110000000000001010111101111100111111211
1322211111121111211001011010000001000000000000001111211112221110000000000010101011111101110111111011
1123122232212122110111101111011001000000000000001101111022121100000000000011010010111111111100111112
1111321221211111210100001010000000000000000000000122111111112100000000000001010110111101111121211111
2222123221231221111101100001000010000000000000000111011211112000000000000000001011100110111111101110
1114121132323221100111111000001010000000000000000001110111011000000000000000010111011111101111111110
2241221231222222111111111100000000000000000000000001111111100000000000000110011110100111111101111001
2211111132132111111110011010110100000000000000000000111101100000000000000101101101111111010111101110
1212314211121312210000000011110000000000000000000001001000000000000001010011111001011111111011111111
1123111122223221211111100011010101100100000000000001111110000000000000111100010111111111011101120100
3211321222211112121110110110011000110000000000000010110111100000000001101010100111100111111101120111
2113112113121213111000101011111110000000000000000000111100000100100010110000011010011111111111111011
1113212112312111211110111000101100100000000010000011011101110000011011010111111010011011100111101111
1213212122121222211010111001101100111101100101011110110100110001101001111101111101001110112110111111
3112122011212211110111010001110111111000100010010111111110001001100010100100001111110101110121121011
1122111212121223111110010101011010111010000000100000000100101000000111101111001001111101111001111101
1321221111123412320001000100011011110001000000101000100010000100000110111100110100101010111101101101
2312321121213211111110111101011111110001111011111110111100111001000000101111111110111001111001111111
1121221113102131321011111101000111100101110100011111111111101001010100111100111110011011011111001111
1222311221121111311111111111110110011110011101011111001111101111010111000011010111111111101111111011
2122112111121211231110111111111111111110010010011111110011011111111110011011111110011101001111111111
1111121211112222110101011111111110101101101010110001101110011111101111112111100111112111010101211111
2123212120111222221111111011111021101011011110001110110011011111111100211111011101011011101000110111
3144132123122114221001111111111111101110101101111000111001111101111111211011101110000012002211111011
2423131122113121221111101011111111211111211022210123101232101221012211112121122111111111101102111111
2112322111121322210112001111111111211101111111111111121211012101122111111011121100110101111101120011
2212332112212232221101111221111111111101112111111111210011111111111111121110111111111111011101111000
1132122121311121231010112212111111110110012110111111011111111112111123111022011121011211010111111201
1123222222434321231001211101111111210012211112210122111121101321001121011121011110111211011110111111
4133131214123111111100011111112111111011100112110111110131111111001211112101111211110111011111101101
3222123212111122230111111111111111210111210011111112110122201111112110112101121121110011100211010111
3143121121014211221211011110113111211121111021111111110111201110111111110111111111111110111101011001
3142112231133221121000111011111111111101101111112001111011111010111111110112111111111111110110111011
4233123211111222221111111111101111121111101111111111101012011021111121111212011010111101111110101111
2243227141112111110111202101111111111111211111200121211121101111100011110011111112111101100111011111
2242322111112212121111211111111121111111110112111102101111111221010111111110111112111111101011111101
3443122111112112220012112110111111111121011102101110011210111111111111111101011101011010121110101101
2322321222222122231111111011111211010011111111101011110100111011211111011111111101121110111111112101
2132211112313211211101110011120211110111121111101110101110121012101111011101011011110111111211011001
3323211121311233211101111111112001110121110111111111111112111111011011111110111111101211110112110011
3135122211312111212111111111112111111001211112211011111111111111111111111121112111100121111021111111
3277122101212201311111011001121001211111101101211121101111111120011102111111121011111100011010010011
3276010011101111211012110111111001111111111111111101111111111111110111111111011111111011111111001112
3273221422411112311121111111001100110112101111111111001111111111101010110111101011111111111201111111
4271113223269812221121111110011211111110112111000011111111111110111211111011001012102111101011011110
4272331193129999521011111110111201011111210111002121101011112011111111122111111011101111110111111111
3372112221324545210111101020111100111110111111111111111120111111101301111111111111011111112110111111
4473112392111969211211111011111001111111111111101112111110110111112311111111111111121101110011121102
1361122231199999221100101011111101000010210011111110011120111111001011011111110011111111011112202111
3266332121112121121121111111011101101110101111111121111111111011111110011111110110211012111111111111
3177111111122110230111011110111111111121121111011121112110111012111121111111111111220101112211111121
1267101111233201311011111111001110101111111111110111012111101101100101112001111111111111101211111111
2166112111223223311110110011111111010111111111101111011111110011012110101110110111121110001112111111
3266113213122112211001111111101111011021100111011111121111112111122011100111112110111101101101111111
3165122131033122211010101011101111111111112112111110111112111111110111111111110111101111111101011111
3336121111111112131111011110101011101111111111011021111111011111122121111221111100111211111011121111
4242113231121222211111100110111011110011111111011111110111000011111001111111111012111111111111101001
3323232111101110122311111110111100100111000111101111111111001111111111111111111011111111101211111111
3232211121222111111101211101011101111111111211111101011101111011011112111111111110211001211111111121
3122121111122122122011111100111111111000221111111111011002110111111111111111111111121101011111111111
3112221112222311111100111111111120101100111011111111112111011111111020010110100120211111111211111112
2223310121131111111111111111111111111111111111111111101001011010111011101000111111101111101111011111
3222211121222221011111101111111101111111111211201111001110111111121101100011110001110111111111101121
2231112121112111121111101110111002211111111012111110111111001110101012221111110001111111011110011101
2224231121432223221111111120111212211101110111001001111111110101101110110002111111111111121011121011
3225322111212112211011111110121111111101011112111010101011100111110111011010111101111011110101111111
3224222111211121211110111011111011011111211011111111011201011100011211010111001001111021101111111111
3123122222223322221111111112111111011111211111110111101000111111010011110001010101010101111110101111
2411311113211311231111012111111111011111002001100111110111110011101111011011011100111111111211111101
3232423222121211320000101211111111101110111100011111111111111121010111101112111111110110010111211102
2441331232121132211220011111101101111111110111010011101011110111011102011111101110101110111111101111
3212132223122212231100110111011000111110101011111010011001100111010110101111100002110110110101001112
3222331224341112311110100101101101101000011110111110101101101011110100010011121101111201010111111112
122231221223111112

Summary:
- 0-blocks: 1670
- 1-blocks: 4662
- 2-blocks: 800
- 3-blocks: 195
- 4-blocks: 44
- 5-blocks: 6
- 6-blocks: 14
- 7-blocks: 12
- 8-blocks: 1
- 9-blocks: 14
- X-blocks: 82
Reason: More than 0 '9' blocks
Assessment: Fail
WARNING: Print failure detected! Consider aborting the print job.
Failure Image: frame000002-failure.jpg
Execution Time: 117.69 ms
```
In this example, with a "tolerance" of 2, the print fails. 

In the comparison grid, there's some variation everywhere, which is a result of JPEG images being ever so slightly different when scrutinized more closely, but well below the threshold where it would be a fail on its own, even at a tolerance of 2. 

However, because we're looking at the image in such detail, the clock changing is enough to trigger the failure. 
An image is then generated showing the blocks shaded in red that have been responsible for the failure. The excluded blocks are also shaded in black for calibration purposes.

| Original Image 2 | Highlighted Image 2|
|---------|--------|
|<img src=frame000002.jpg> |<img src=frame000002-failure.jpg>|

Normally, a tolerance of 2 would be far too low. And the digital displays, perhaps even the belts in this case, should be excluded from consideration. As the print proceeds, there will be variations in the printed area as well, particularly when going from having no plastic present in a block to having a plastic-filled block. Some tuning of both the precision and tolerance parameters will get us part way towards not triggering false positives.

Longer-term, more extensive analysis will likely be needed, but as we're trying to do this on-device with not especially powerful devices, this is sufficient to get us up and running.
