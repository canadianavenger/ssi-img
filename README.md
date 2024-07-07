# ssi-img
This repository is a companion to my blog post about the reverse engineering of the '.IMG' file format used with *Red Lightning* by *Strategic Simulations, Inc. (SSI)*. For more detail please read [my blog post](https://canadianavenger.io/2024/07/08/thunderbolts-and-lightning).

## The Code

In this repo there are two C programs, each is a standalone utility for converting between the SSI-IMG format and the Windows BMP format. The code is written to be portable, and should be able to be compiled for Windows, Linux, or Mac. The code is offered without warranty under the MIT License. Use it as you will personally or commercially, just give credit if you do.

- `img2bmp.c` converts from `.img` to `.bmp` as no image metadata exists in the img file it must be passed as a parameter on the command-line, along with the filename eg `img2bmp 640x200 EGAHEXES.img`. The resultant BMP file will be a 16 colour indexed image with the EGA palette.
- `bmp2img.c` converts from `.bmp` to `.img` only the file name is requred in this case, as the BMP file carries all the necessary information eg `bmp2img CUSTOMHEXES.bmp`. The BMP file in this case **must** be a uncompressed 16 colour indexed image. It is assumed that the colour indexes align with those of the EGA palette. No colour matching/palette remapping is performed.

Note: Both programs accept an optional 2nd filename parameter for the output file. If this parameter is not provided then the output file will have the same name as the input, just with extension changed to match the format. 

## The IMG File Format
In the end this format turned out to be nothing more than a raw framebuffer capture, and thus its organization is dependant on the video mode being utilized. This essentially appears to be the Borland BGI libraries `getimage()` image data with the width and height prefix removed. As the game runs in an EGA graphics mode the image data is separated into 4 separate image planes.

