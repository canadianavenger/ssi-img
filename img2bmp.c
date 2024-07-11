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
#include "bmp.h"

typedef struct {
    size_t      len;         // length of buffer in bytes
    size_t      pos;         // current byte position in buffer
    uint8_t     *data;       // pointer to bytes in memory
} memstream_buf_t;

// default EGA/VGA 16 colour palette
static bmp_palette_entry_t ega_pal[16] = { 
  {0x00,0x00,0x00,0x00}, {0xaa,0x00,0x00,0x00}, {0x00,0xaa,0x00,0x00}, {0xaa,0xaa,0x00,0x00}, 
  {0x00,0x00,0xaa,0x00}, {0xaa,0x00,0xaa,0x00}, {0x00,0x55,0xaa,0x00}, {0xaa,0xaa,0xaa,0x00},
  {0x55,0x55,0x55,0x00}, {0xff,0x55,0x55,0x00}, {0x55,0xff,0x55,0x00}, {0xff,0xff,0x55,0x00}, 
  {0x55,0x55,0xff,0x00}, {0xff,0x55,0xff,0x00}, {0x55,0xff,0xff,0x00}, {0xff,0xff,0xff,0x00},
};
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

int save_bmp(const char *dst, memstream_buf_t *src, uint16_t width, uint16_t height, bmp_palette_entry_t *pal);
void pln2lin(memstream_buf_t *dst, memstream_buf_t *src);
void lace2lin(memstream_buf_t *dst, memstream_buf_t *src, uint16_t width, uint16_t height);

size_t filesize(FILE *f);
void drop_extension(char *fn);
char *filename(char *path);
#define fclose_s(A) if(A) fclose(A); A=NULL
#define free_s(A) if(A) free(A); A=NULL

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
    bool is_amiga = false;
    bmp_palette_entry_t *img_pal = NULL; // the dynamically allocated CGA palette
    bmp_palette_entry_t *pal = NULL; // set to point to which palette used (not allocted)

    printf("SSI-IMG to BMP image converter\n");

    if((argc < 3) || (argc > 4)) {
        printf("USAGE: %s [resolution]<adapter><palette> [infile] <outfile>\n", filename(argv[0]));
        printf("where [resolution] is in the form width x height eg '320x200'\n");
        printf("The resolution paramter can have a number of optional suffixes to\n");
        printf("change the interpretation. (EGA is default)\n");
        printf("- a suffix of 'e' '640x200e' will force EGA interpretation of the input file\n");
        printf("- a suffix of 'c' '320x200c' will force CGA interpretation of the input file\n");
        printf("- a suffix of 'c' '320x200a' will force Amiga interpretation of the input file\n");
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
        } else if('e' != tolower(charfmt)) {
            printf("Invalid format specifier\n");
            goto CLEANUP;
        }
    }
    printf("Resolution: %d x %d %s\n", width, height, is_cga?"CGA":is_amiga?"Amiga":"EGA");

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
        if(NULL == (img_pal = calloc(16, sizeof(bmp_palette_entry_t)))) {
            printf("Unable to allocate memory\n");
            goto CLEANUP;
        }
        // copy the EGA palette entries over to their CGA locations
        // for the selected palette
        for(int p = 0; p < 4; p++) {
            img_pal[p] = ega_pal[cga2ega[pal_sel][p]];
        }
        pal = img_pal;
    } else if(is_amiga) {
        size_t realsize = img.len;
        img.len -= 64;
        pln2lin(&src, &img); // deplane the image
        if(NULL == (img_pal = calloc(16, sizeof(bmp_palette_entry_t)))) {
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

            double comp = entry & 0x0f;
            comp /= 15;
            comp *= 255;
            if(comp > 255.0) comp = 255;
            img_pal[p].b = comp;
            entry >>= 4;
            comp = entry & 0x0f;
            comp /= 15;
            comp *= 255;
            if(comp > 255.0) comp = 255;
            img_pal[p].g = comp;
            entry >>= 4;
            comp = entry & 0x0f;
            comp /= 15;
            comp *= 255;
            if(comp > 255.0) comp = 255;
            img_pal[p].r = comp;
        }

        pal = img_pal;
    } else { // must be ega
        pln2lin(&src, &img); // deplane the image
        pal = ega_pal;
    }
    src.pos = 0; // reset the src position

    printf("Creating BMP File: '%s'\n", fo_name);
    rval = save_bmp(fo_name, &src, width, height, pal);
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

/// @brief determins the size of the file
/// @param f handle to an open file
/// @return returns the size of the file
size_t filesize(FILE *f) {
    size_t szll, cp;
    cp = ftell(f);           // save current position
    fseek(f, 0, SEEK_END);   // find the end
    szll = ftell(f);         // get positon of the end
    fseek(f, cp, SEEK_SET);  // restore the file position
    return szll;             // return position of the end as size
}

/// @brief removes the extension from a filename
/// @param fn sting pointer to the filename
void drop_extension(char *fn) {
    char *extension = strrchr(fn, '.');
    if(NULL != extension) *extension = 0; // strip out the existing extension
}

/// @brief Returns the filename portion of a path
/// @param path filepath string
/// @return a pointer to the filename portion of the path string
char *filename(char *path) {
	int i;

	if(path == NULL || path[0] == '\0')
		return "";
	for(i = strlen(path) - 1; i >= 0 && path[i] != '/'; i--);
	if(i == -1)
		return "";
	return &path[i+1];
}

/// @brief converts a planerized image to a linear one, assumes 16 colour 4 bits per pixel
/// @param dst memstream buffer pointing to buffer large enough for 1 byte per pixel
/// @param src memstream buffer pointing to a buffer containing the packed planar image
void pln2lin(memstream_buf_t *dst, memstream_buf_t *src) {
    int ofs2 = src->len / 2;  // 1/2
    int ofs1 = ofs2 / 2;      // 1/4
    int ofs3 = ofs1 + ofs2;   // 3/4

    for(int i = 0; i < ofs1; i++) {
        uint8_t p0 = src->data[       i];
        uint8_t p1 = src->data[ofs1 + i];
        uint8_t p2 = src->data[ofs2 + i];
        uint8_t p3 = src->data[ofs3 + i];

        for(int b = 0; b < 8; b++) { // 8 pixels packed per byte
            uint8_t px = 0;
            px |= p0 & 0x80; px >>= 1;
            px |= p1 & 0x80; px >>= 1;
            px |= p2 & 0x80; px >>= 1;
            px |= p3 & 0x80; px >>= 4; // final shift
            if(dst->pos < dst->len) dst->data[dst->pos++] = px;
            p0 <<= 1; p1 <<= 1; p2 <<= 1; p3 <<= 1; // shift in the next pixel
        }
    }
}

/// @brief converts a interleved image to a linear one, assumes 4 colour 2 bits per pixel
/// @param dst memstream buffer pointing to buffer large enough for 1 byte per pixel
/// @param src memstream buffer pointing to a buffer containing the packed planar image
void lace2lin(memstream_buf_t *dst, memstream_buf_t *src, uint16_t width, uint16_t height) {
    width /= 4;  // we expect 4 pixels per byte
    height /= 2; // we always expect lines to be in interleved pairs

    int even_pos = 0;
    int odd_pos = src->len / 2; // 1/2

    for(int y = 0; y < height; y++) {
        // even line
        for(int x = 0; x < width; x++) {
            uint16_t pbuf = src->data[even_pos++];
            for(int b = 0; b < 4; b++) { // 4 pixels per byte
                pbuf <<= 2; // shift in the pixel
                uint8_t px = (pbuf >> 8) & 0x03; // move it to position and mask
                if(dst->pos < dst->len) dst->data[dst->pos++] = px; 
            }
        }
        // odd line
        for(int x = 0; x < width; x++) {
            uint16_t pbuf = src->data[odd_pos++];
            for(int b = 0; b < 4; b++) { // 4 pixels per byte
                pbuf <<= 2; // shift in the pixel
                uint8_t px = (pbuf >> 8) & 0x03; // move it to position and mask
                if(dst->pos < dst->len) dst->data[dst->pos++] = px; 
            }
        }
    }
}

// allocate a header buffer large enough for all 3 parts, plus 16 bit padding at the start to 
// maintian 32 bit alignment after the 16 bit signature.
#define HDRBUFSZ (sizeof(bmp_signature_t) + sizeof(bmp_header_t))

/// @brief saves the image pointed to by src as a BMP, assumes 16 colour 1 byte per pixel image data
/// @param fn name of the file to create and write to
/// @param src memstream buffer pointer to the source image data
/// @param width  width of the image in pixels
/// @param height height of the image in pixels or lines
/// @param pal pointe to 16 entry palette
/// @return 0 on success, otherwise an error code
int save_bmp(const char *fn, memstream_buf_t *src, uint16_t width, uint16_t height, bmp_palette_entry_t *pal) {
    int rval = 0;
    FILE *fp = NULL;
    uint8_t *buf = NULL; // line buffer, also holds header info

    // do some basic error checking on the inputs
    if((NULL == fn) || (NULL == src) || (NULL == src->data)) {
        rval = -1;  // NULL pointer error
        goto bmp_cleanup;
    }

    // try to open/create output file
    if(NULL == (fp = fopen(fn,"wb"))) {
        rval = -2;  // can't open/create output file
        goto bmp_cleanup;
    }

    // stride is the bytes per line in the BMP file, which are padded
    // out to 32 bit boundaries
    uint32_t stride = ((width + 3) & (~0x0003)) / 2; // we get 2 pixels per byte for being 16 colour
    uint32_t bmp_img_sz = (stride) * height;

    // allocate a buffer to hold the header and a single scanline of data
    // this could be optimized if necessary to only allocate the larger of
    // the line buffer, or the header + padding as they are used at mutually
    // exclusive times
    if(NULL == (buf = calloc(1, HDRBUFSZ + stride + 2))) {
        rval = -3;  // unable to allocate mem
        goto bmp_cleanup;
    }

    // signature starts after padding to maintain 32bit alignment for the rest of the header
    bmp_signature_t *sig = (bmp_signature_t *)&buf[stride + 2];

    // bmp header starts after signature
    bmp_header_t *bmp = (bmp_header_t *)&buf[stride + 2 + sizeof(bmp_signature_t)];

    // setup the signature and DIB header fields
    *sig = BMPFILESIG;
    size_t palsz = sizeof(bmp_palette_entry_t) * 16;
    bmp->dib.image_offset = HDRBUFSZ + palsz;
    bmp->dib.file_size = bmp->dib.image_offset + bmp_img_sz;

    // setup the bmi header fields
    bmp->bmi.header_size = sizeof(bmi_header_t);
    bmp->bmi.image_width = width;
    bmp->bmi.image_height = height;
    bmp->bmi.num_planes = 1;           // always 1
    bmp->bmi.bits_per_pixel = 4;       // 16 colour image
    bmp->bmi.compression = 0;          // uncompressed
    bmp->bmi.bitmap_size = bmp_img_sz;
    bmp->bmi.horiz_res = BMP96DPI;
    bmp->bmi.vert_res = BMP96DPI;
    bmp->bmi.num_colors = 16;          // palette has 16 colours
    bmp->bmi.important_colors = 0;     // all colours are important

    // write out the header
    int nr = fwrite(sig, HDRBUFSZ, 1, fp);
    if(1 != nr) {
        rval = -4;  // unable to write file
        goto bmp_cleanup;
    }

    // we're using our global palette here, wich is already in BMP format
    // write out the palette
    nr = fwrite(pal, palsz, 1, fp);
    if(1 != nr) {
        rval = -4;  // can't write file
        goto bmp_cleanup;
    }

    // now we need to output the image scanlines. For maximum
    // compatibility we do so in the natural order for BMP
    // which is from bottom to top. For 16 colour/4 bit image
    // the pixels are packed two per byte, left most pixel in
    // the most significant nibble.
    // start by pointing to start of last line of data
    uint8_t *px = &src->data[src->len - width];
    // loop through the lines
    for(int y = 0; y < height; y++) {
        memset(buf, 0, stride); // zero out the line in the output buffer
        // loop through all the pixels for a line
        // we are packing 2 pixels per byte, so width is half
        for(int x = 0; x < ((width + 1) / 2); x++) {
            uint8_t sp = *px++;          // get the first pixel
            sp <<= 4;                    // shift to make room
            if((x * 2 + 1) < width) {    // test for odd pixel end
                sp |= (*px++) & 0x0f;    // get the next pixel
            }
            buf[x] = sp;                 // write it to the line buffer
        }
        nr = fwrite(buf, stride, 1, fp); // write out the line
        if(1 != nr) {
            rval = -4;  // unable to write file
            goto bmp_cleanup;
        }
        px -= (width * 2); // move back to start of previous line
    }

bmp_cleanup:
    fclose_s(fp);
    free_s(buf);
    return rval;
}
