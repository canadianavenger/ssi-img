# ssi-img
This repository is a companion to my blog post about the reverse engineering of the '.IMG' file format used with *Red Lightning* by *Strategic Simulations, Inc. (SSI)*. For more detail please read [my blog post](https://canadianavenger.io/2024/07/08/thunderbolts-and-lightning).

## SSI Titles Known to Use the IMG Format
- Red Lightning (1989)
- Conflict: Middle East (1991)
- Conflict: Korea (1992) [Only for CGA graphics]

## The Code

In this repo there are two C programs, each is a standalone utility for converting between the SSI-IMG format and the Windows BMP format. The code is written to be portable, and should be able to be compiled for Windows, Linux, or Mac. The code is offered without warranty under the MIT License. Use it as you will personally or commercially, just give credit if you do.

- `img2bmp.c` converts from `.img` to `.bmp` as no image metadata exists in the img file it must be passed as a parameter on the command-line, along with the filename eg `img2bmp 640x200 EGAHEXES.img`. The resultant BMP file will be a 16 colour indexed image with the EGA palette. an additional suffix of 'c' or 'e' can be added to the resolution parameter to indicate a CGA (c) or EGA (e) file, this may alos optionally be followed by a single digit on the range of 0-5 to denote which palette to use, by defauly palette 1 is used if omitted, and EGA is assumed if the character parameter is omitted. eg `img2bmp 320x200c1 CGAHEXES.img` Note that the palette selection is for rendering to the BMP only, and has no effect on how the image would be presented in-game.
- `bmp2img-ega.c` converts from `.bmp` to `.img` (EGA/VGA variant) only the file name is required in this case, as the BMP file carries all the necessary information eg `bmp2img-ega CUSTOMHEXES.bmp`. The BMP file in this case **must** be a uncompressed 16 colour indexed image. It is assumed that the colour indexes align with those of the EGA palette. No colour matching/palette remapping is performed.
- `bmp2img-cga.c` converts from `.bmp` to `.img` (CGA variant) only the file name is required in this case, as the BMP file carries all the necessary information eg `bmp2img-cga CUSTOMHEXES.bmp`. The BMP file in this case **must** be a uncompressed 16 colour indexed image, though the indices must only range 0-3. It is assumed that the colour indexes align with those of the CGA palette. No colour matching/palette remapping is performed.

Note: All the programs accept an optional 2nd filename parameter for the output file. If this parameter is not provided then the output file will have the same name as the input, just with extension changed to match the format. 

## The IMG File Format
In the end this format turned out to be nothing more than a raw framebuffer capture, and thus its organization is dependant on the video mode being utilized. This essentially appears to be the *Borland BGI* libraries `getimage()` image data with the width and height prefix removed. (It may be possible that this generation of the BGI library did not prefix with width and height as well) So far I've only come across EGA/VGA and CGA variants of this format. 

### EGA Framebuffer Organization
For EGA 16 colour graphics mode the image data is separated into 4 separate image planes. Each plane is 16000 bytes in size (80 bytes per line, 200 lines for 640x200) When writing to EGA memory, it is possible to write to more than one plane at a time through the use of a mask register. However when reading from EGA video memory, only a single plane can be accessed at a time. Pixels are packed 8 per byte on each plane. Reconstruction requires reading of all 4 planes and extracting the corresponding bits.

### CGA Framebuffer Organization
For CGA and 4 colour graphics, the data is separated into two blocks of memory. The scanlines are interlaced with the first block storing the even scanlines, and the 2nd the odd scanlines. Each pixel occupies 2 bits and are stored linearly in a scanline, packed 4 to a byte. Reconstruction of the image requires selecting the block based on the scanline and then extracting the corresponding two bits for the given pixel. Note that the file is a full 16384 bytes in size, representing the entire CGA cards memory buffer. Only 16000 bytes are used for the 320x200 graphics, however the first block of 8000 bytes (80 bytes per line, 100 lines) starts at `0x0000`, the second block of 8000 starts at `0x2000` (power of 2 boundary), so there is some padding after each block (192 bytes).




