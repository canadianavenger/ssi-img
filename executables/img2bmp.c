/*
 * img2bmp.c 
 * Converts a given SSI-IMG to a Windows BMP file
 *  
 * This code is offered without warranty under the MIT License. Use it as you will 
 * personally or commercially, just give credit if you do.
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "ssi-img.h"
#include "bmp.h"
#include "util.h"

// EGA's 64 palette table entries
static pal_entry_t ega_table[64] = { 
  {0x00,0x00,0x00}, {0x00,0x00,0xaa}, {0x00,0xaa,0x00}, {0x00,0xaa,0xaa}, // 0x00-0x03
  {0xaa,0x00,0x00}, {0xaa,0x00,0xaa}, {0xaa,0xaa,0x00}, {0xaa,0xaa,0xaa}, // 0x04-0x07
  {0x00,0x00,0x55}, {0x00,0x00,0xff}, {0x00,0xaa,0x55}, {0x00,0xaa,0xff}, // 0x08-0x0b
  {0xAA,0x00,0x55}, {0xAA,0x00,0xFF}, {0xAA,0xAA,0x55}, {0xAA,0xAA,0xFF}, // 0x0c-0x0f
  {0x00,0x55,0x00}, {0x00,0x55,0xAA}, {0x00,0xFF,0x00}, {0x00,0xFF,0xAA}, // 0x10-0x13
  {0xAA,0x55,0x00}, {0xAA,0x55,0xAA}, {0xAA,0xFF,0x00}, {0xAA,0xFF,0xAA}, // 0x14-0x17
  {0x00,0x55,0x55}, {0x00,0x55,0xFF}, {0x00,0xFF,0x55}, {0x00,0xFF,0xFF}, // 0x18-0x1b
  {0xAA,0x55,0x55}, {0xAA,0x55,0xFF}, {0xAA,0xFF,0x55}, {0xAA,0xFF,0xFF}, // 0x1c-0x1f
  {0x55,0x00,0x00}, {0x55,0x00,0xAA}, {0x55,0xAA,0x00}, {0x55,0xAA,0xAA}, // 0x20-0x23
  {0xFF,0x00,0x00}, {0xFF,0x00,0xAA}, {0xFF,0xAA,0x00}, {0xFF,0xAA,0xAA}, // 0x24-0x27
  {0x55,0x00,0x55}, {0x55,0x00,0xFF}, {0x55,0xAA,0x55}, {0x55,0xAA,0xFF}, // 0x28-0x2b
  {0xFF,0x00,0x55}, {0xFF,0x00,0xFF}, {0xFF,0xAA,0x55}, {0xFF,0xAA,0xFF}, // 0x2c-0x2f
  {0x55,0x55,0x00}, {0x55,0x55,0xAA}, {0x55,0xFF,0x00}, {0x55,0xFF,0xAA}, // 0x30-0x33
  {0xFF,0x55,0x00}, {0xFF,0x55,0xAA}, {0xFF,0xFF,0x00}, {0xFF,0xFF,0xAA}, // 0x34-0x37
  {0x55,0x55,0x55}, {0x55,0x55,0xFF}, {0x55,0xFF,0x55}, {0x55,0xFF,0xFF}, // 0x38-0x3b
  {0xFF,0x55,0x55}, {0xFF,0x55,0xFF}, {0xFF,0xFF,0x55}, {0xFF,0xFF,0xFF}  // 0x3c-0x3f
};

// default 16 ega colours (mapping in ega_table)
uint8_t ega_pal[16] = {
     0,  1,  2,  3, 
     4,  5, 20,  7, 
    56, 57, 58, 59, 
    60, 61, 62, 63    
};

// SSI "Western Front" Title image palette
/*
static pal_entry_t ega_pal[16] = { 
   0,  0, 60, 37, 
  31, 59, 35, 10,
  56,  4, 46, 46, 
  39, 20, 62, 63
};
*/

// remaps the CGA colour indicies to the EGA equivalents for each of the palettes
// background is assumed to be black, though in reality it can be programmed to
// any of the 16 colours
uint8_t cga2ega[6][4] = {
    {0,2,4,6},     // mode 4 palette 0 low intensity  [black, dark green, dark red, brown]
    {0,10,12,14},  // mode 4 palette 0 high intensity [black, light green, light red, yellow]
    {0,3,5,7},     // mode 4 palette 1 low intensity  [black, dark cyan, dark magenta, light grey]
    {0,11,13,15},  // mode 4 palette 1 high intensity [black, light cyan, light magenta, white]
    {0,3,4,7},     // mode 5 low intensity            [black, dark cyan, dark red, light gray]
    {0,11,12,15}   // mode 5 high intensity           [black, light cyan, light red, white]
};

int main(int argc, char *argv[]) {
    int rval = -1;
    FILE *fi = NULL;
    char *fi_name = NULL;
    char *fo_name = NULL;
    memstream_buf_t img = {0, 0, NULL}; // planar data from file
    memstream_buf_t src = {0, 0, NULL}; // de-planed data
    char resolution[10];
    uint16_t width;
    uint16_t height;
    uint8_t pal_sel = 1; // which CGA palette to use if CGA mode selected
    bool is_cga = false; // flag to indicate to use CGA mode
    bool is_amiga = false; // flag to indicate Amiga mode
    bool is_interleaved = false; // flag to indicate interleaved mode
    pal_entry_t *img_pal = NULL; // the dynamically allocated CGA palette
    pal_entry_t *pal = NULL; // set to point to which palette used (not allocted)

    printf("SSI-IMG to BMP image converter\n");

    if((argc < 3) || (argc > 4)) {
        printf("USAGE: %s [resolution]<adapter><palette> [infile] <outfile>\n", filename(argv[0]));
        printf("where [resolution] is in the form width x height eg '320x200'\n");
        printf("The resolution paramter can have a number of optional suffixes to\n");
        printf("change the interpretation. (EGA is default)\n");
        printf("- a suffix of 'e' '640x200e' will force EGA interpretation of the input file\n");
        printf("- a suffix of 'c' '320x200c' will force CGA interpretation of the input file\n");
        printf("- a suffix of 'a' '320x200a' will force Amiga interpretation of the input file\n");
        printf("- a suffix of 'b' '320x200b' will force planar interleved interpretation fo the data\n");
        printf("The 'c' suffix also allows for an optional additonal suffix in the form of a\n");
        printf("single digit in the range of 0-5. This digit specifies which of the CGA palettes\n");
        printf("to use. eg '320x200c1' Palette 1 is the default if omitted\n");
        printf("CGA Palettes:\n");
        printf(" 0: black, dark green,  dark red,      brown      mode:4 pal:0 low intensity\n");
        printf(" 1: black, light green, light red,     yellow     mode:4 pal:0 high intensity\n");
        printf(" 2: black, dark cyan,   dark magenta,  light grey mode:4 pal:1 low intensity\n");
        printf(" 3: black, light cyan,  light magenta, white      mode:4 pal:1 high intensity\n");
        printf(" 4: black, dark cyan,   dark red,      light gray mode:5 low intensity\n");
        printf(" 5: black, light cyan,  light red,     white      mode:5 high intensity\n");
        printf("[infile] is the name of the input file\n");
        printf("<outfile> is optional and the name of the output file\n");
        printf("if omitted, outfile will be named the same as infile with a .BMP extension\n");
         return -1;
    }
    argv++; argc--; // consume the first arg (program name)

    // get the resolution and filename strings from command line
    strncpy(resolution, argv[0], 10);
    argv++; argc--; // consume the arg (resolution)

    int namelen = strlen(argv[0]);
    if(NULL == (fi_name = calloc(1, namelen+1))) {
        printf("Unable to allocate memory\n");
        goto CLEANUP;
    }
    strncpy(fi_name, argv[0], namelen);
    argv++; argc--; // consume the arg (input file)

    if(argc) { // output file name was provided
        int namelen = strlen(argv[0]);
        if(NULL == (fo_name = calloc(1, namelen+1))) {
            printf("Unable to allocate memory\n");
            goto CLEANUP;
        }
        strncpy(fo_name, argv[0], namelen);
        argv++; argc--; // consume the arg (input file)
    } else { // no name was provded, so make one
        if(NULL == (fo_name = calloc(1, namelen+5))) {
            printf("Unable to allocate memory\n");
            goto CLEANUP;
        }
        strncpy(fo_name, fi_name, namelen);
        drop_extension(fo_name); // remove exisiting extension
        strncat(fo_name,".BMP", namelen+4); // add bmp extension
    }

    // parse the resolution string
    char charfmt = 0;
    char charpal = 0;
    sscanf(resolution, "%hu%*[xX]%hu%c%c", &width, &height, &charfmt, &charpal); // scanf for new resolution line
    if((0 == width) || (0 == height)) {
        printf("Invalid resolution specificaton\n");
        goto CLEANUP;
    }

    // parse the optional suffixes
    if(charfmt) { // format specifier was provided
        if('c' == tolower(charfmt)) {
            is_cga = true;
            if(charpal) { 
                if(isdigit(charpal)) {
                    pal_sel = charpal - '0';
                    if(pal_sel > 5) {
                        printf("Invalid palette specifier");
                        goto CLEANUP;
                    }
                } else {
                    printf("Invalid palette specifier");
                    goto CLEANUP;
                }
            }
        } else if('a' == tolower(charfmt)) { // special secret amiga mode
            is_amiga = true;
        } else if('b' == tolower(charfmt)) { // super secret interleaved mode
            is_interleaved = true;
        } else if('e' != tolower(charfmt)) {
            printf("Invalid format specifier\n");
            goto CLEANUP;
        }
    }
    printf("Resolution: %d x %d %s\n", width, height, is_cga?"CGA":is_amiga?"Amiga":is_interleaved?"EGA interleaved":"EGA");

    // allocate buffer based on resolution
    // allocate the deplaned/unpaced image buffer (width * height)
    if(NULL == (src.data = calloc(height, width))) {
        printf("Unable to allocate memory\n");
        goto CLEANUP;
    }
    src.len = (width * height);

    // open the input file
    printf("Opening IMG File: '%s'", fi_name);
    if(NULL == (fi = fopen(fi_name,"rb"))) {
        printf("Error: Unable to open input file\n");
        goto CLEANUP;
    }

    // determine size of image file
    size_t fsz = filesize(fi);
    printf("\tFile Size: %zu\n", fsz);

    // allocate the packed image buffer based on the file size
    if(NULL == (img.data = calloc(1, fsz))) {
        printf("Unable to allocate memory\n");
        goto CLEANUP;
    }
    img.len = fsz;

    // do some basic checking based on filesize
    if(is_cga) {
        if(fsz != 16384) {
            printf("File image and Specified image size mismatch for CGA\n");
            goto CLEANUP;
        }
    } else if(is_amiga) {
        if(fsz != ((width * height) / 2) + 64) {
            printf("File image and Specified image size mismatch for Amiga\n");
            goto CLEANUP;
        }
    } else if(is_interleaved) {
        if(fsz != ((width * height) / 2)) {
            printf("File image and Specified image size mismatch for EGA Interleaved\n");
            goto CLEANUP;
        }
    } else { // must be ega
        if(fsz != ((width * height) / 2)) {
            printf("File image and Specified image size mismatch for EGA\n");
            goto CLEANUP;
        }
    }

    // read in the file
    int nr = fread(img.data, img.len, 1, fi);
    if(1 != nr) {
        printf("Error Unable read file\n");
        goto CLEANUP;
    }

    if(is_cga) {
        lace2lin(&src, &img, width, height); // de-interlace the image
        // create our palette 
        if(NULL == (img_pal = calloc(16, sizeof(pal_entry_t)))) {
            printf("Unable to allocate memory\n");
            goto CLEANUP;
        }
        // copy the EGA palette entries over to their CGA locations
        // for the selected palette
        for(int p = 0; p < 4; p++) {
            img_pal[p] = ega_table[ega_pal[cga2ega[pal_sel][p]]];
        }
        pal = img_pal;
    } else if(is_amiga) {
        size_t realsize = img.len;
        img.len -= 64;
        pln2lin(&src, &img); // deplane the image
        if(NULL == (img_pal = calloc(16, sizeof(pal_entry_t)))) {
            printf("Unable to allocate memory\n");
            goto CLEANUP;
        }
        img.pos = img.len; // point to the end of the framebuffer / start of palette
        img.len = realsize;
        
        // read in the palette, 2 bytes per entry, 4 bits per colour
        for(int p = 0; p < 16; p++) {
            uint16_t entry = img.data[img.pos++];
            entry <<= 8;
            entry |= img.data[img.pos++];
            img_pal[p].b = entry & 0x0f;
            entry >>= 4;
            img_pal[p].g = entry & 0x0f;
            entry >>= 4;
            img_pal[p].r = entry & 0x0f;
        }
        pal4_to_pal8(img_pal, img_pal, 16);
        pal = img_pal;
    } else if(is_interleaved) {
        ipln2lin(&src, &img, width, height); // deplane (interleaved) the image
        if(NULL == (img_pal = calloc(16, sizeof(pal_entry_t)))) {
            printf("Unable to allocate memory\n");
            goto CLEANUP;
        }
        for(int p = 0; p < 16; p++) {
            img_pal[p] = ega_table[ega_pal[p]];
        }
        pal = img_pal;
    } else { // must be ega
        pln2lin(&src, &img); // deplane the image
        if(NULL == (img_pal = calloc(16, sizeof(pal_entry_t)))) {
            printf("Unable to allocate memory\n");
            goto CLEANUP;
        }
        for(int p = 0; p < 16; p++) {
            img_pal[p] = ega_table[ega_pal[p]];
        }
        pal = img_pal;
    }
    src.pos = 0; // reset the src position

    printf("Creating BMP File: '%s'\n", fo_name);
    rval = save_bmp4(fo_name, &src, width, height, pal);
    if(0 != rval) {
        printf("BMP Save Error (%d)\n", rval);
        goto CLEANUP;
    }

    printf("Done\n");
    rval = 0; // clean exit
CLEANUP:
    fclose_s(fi);
    free_s(fi_name);
    free_s(fo_name);
    free_s(img_pal);
    free_s(img.data);
    free_s(src.data);
    return rval;
}
