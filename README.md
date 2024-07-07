# ssi-img
This repository is a companion to my blog post about the reverse engineering of the '.IMG' file format used with *Red Lightning* by *Strategic Simulations, Inc. (SSI)*. For more detail please read [my blog post](https://canadianavenger.io/2024/07/08/thunderbolts-and-lightning).

## SSI Titles Known to Use the IMG Format
- Red Lightning (1989)
- Conflict: Middle East (1991)
- Conflict: Korea (1992) [Only for CGA graphics, currently not supported]

## The Code

In this repo there are two C programs, each is a standalone utility for converting between the SSI-IMG format and the Windows BMP format. Currently the code only supports 4-plane pixel arrangements (EGA/VGA Graphics), additional work would be required to support CGA or Hercules/Monochrome. The code is written to be portable, and should be able to be compiled for Windows, Linux, or Mac. The code is offered without warranty under the MIT License. Use it as you will personally or commercially, just give credit if you do.

- `img2bmp.c` converts from `.img` to `.bmp` as no image metadata exists in the img file it must be passed as a parameter on the command-line, along with the filename eg `img2bmp 640x200 EGAHEXES.img`. The resultant BMP file will be a 16 colour indexed image with the EGA palette.
- `bmp2img.c` converts from `.bmp` to `.img` only the file name is requred in this case, as the BMP file carries all the necessary information eg `bmp2img CUSTOMHEXES.bmp`. The BMP file in this case **must** be a uncompressed 16 colour indexed image. It is assumed that the colour indexes align with those of the EGA palette. No colour matching/palette remapping is performed.

Note: Both programs accept an optional 2nd filename parameter for the output file. If this parameter is not provided then the output file will have the same name as the input, just with extension changed to match the format. 

## The IMG File Format
In the end this format turned out to be nothing more than a raw framebuffer capture, and thus its organization is dependant on the video mode being utilized. This essentially appears to be the Borland BGI libraries `getimage()` image data with the width and height prefix removed. The data is raw framebuffer memory, so entirely dependant on the video mode being used. 

For EGA 16 colour graphics mode the image data is separated into 4 separate image planes. Pixels are packed 8 per byte on each plane. Reconstruction requires reading of all 4 planes and extracting the corresponding bits.

For CGA 4 colour graphics the data is separated into two blocks of memory the first block stores the even scanlines, and the 2nd the odd scanlines. Each pixel occupies 2 bits and are stored linearly in a scanline, packed 4 to a byte. Reconstruction requires selecting the block based on the scanline and then extracting the corresponding two bits for the given pixel. Note that the file is a full 16384 bytes in size, representing the entire CGA cards memory buffer. Only 16000 bytes are used for the 320x200 graphics, however the first block of 8000 starts at `0x0000`, the second block of 8000 starts at `0x2000`, so there is some padding after each block (192 bytes).


