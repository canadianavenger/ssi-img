/*
 * bmp2img.c 
 * Converts a given Windows BMP file to a SSI-IMG (for EGA/VGA graphics)
 * 
 * The BMP file must be an uncompressed 16 colour indexed BMP. This program does
 * not do any palette matching or remapping, so it is expected that the colour
 * indicies are those of the EGA/VGA 16 colour palette
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
#include <libgen.h>
#include "bmp.h"

typedef struct {
    size_t      len;         // length of buffer in bytes
    size_t      pos;         // current byte position in buffer
    uint8_t     *data;       // pointer to bytes in memory
} memstream_buf_t;

// default EGA/VGA 16 colour palette
static const bmp_palette_entry_t pal[16] = { 
  {0x00,0x00,0x00,0x00}, {0xaa,0x00,0x00,0x00}, {0x00,0xaa,0x00,0x00}, {0xaa,0xaa,0x00,0x00}, 
  {0x00,0x00,0xaa,0x00}, {0xaa,0x00,0xaa,0x00}, {0x00,0x55,0xaa,0x00}, {0xaa,0xaa,0xaa,0x00},
  {0x55,0x55,0x55,0x00}, {0xff,0x55,0x55,0x00}, {0x55,0xff,0x55,0x00}, {0xff,0xff,0x55,0x00}, 
  {0x55,0x55,0xff,0x00}, {0xff,0x55,0xff,0x00}, {0x55,0xff,0xff,0x00}, {0xff,0xff,0xff,0x00},
};
 
int load_bmp(memstream_buf_t *dst, const char *src, uint16_t *width, uint16_t *height);
void lin2pln(memstream_buf_t *dst, memstream_buf_t *src);

size_t filesize(FILE *f);
void drop_extension(char *fn);
#define fclose_s(A) if(A) fclose(A); A=NULL
#define free_s(A) if(A) free(A); A=NULL

int main(int argc, char *argv[]) {
    int rval = -1;
    FILE *fo = NULL;
    char *fi_name = NULL;
    char *fo_name = NULL;
    memstream_buf_t img = {0, 0, NULL}; // planar data from file
    memstream_buf_t src = {0, 0, NULL}; // de-planed data
    char resolution[10];
    uint16_t width;
    uint16_t height;

    printf("BMP to SSI-IMG image converter\n");

    if((argc < 2) || (argc > 3)) {
        printf("USAGE: %s [infile] <outfile>\n", basename(argv[0]));
        printf("[infile] is the name of the input file\n");
        printf("<outfile> is optional and the name of the output file\n");
        printf("if omitted, the output will be named the same as infile, except with a .IMG extension\n");
        return -1;
    }
    argv++; argc--; // consume the first arg (program name)

    // get the filename strings from command line
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
        strncat(fo_name,".IMG", namelen+4); // add bmp extension
    }

    printf("Loading BMP File: '%s'\n", fi_name);
    rval = load_bmp(&src, fi_name, &width, &height);
    if(0 != rval) {
        printf("BMP Load Error (%d)\n", rval);
        goto CLEANUP;
    }

    printf("Resolution: %d x %d\n", width, height);

    if(NULL == (img.data = calloc(height, width/2))) {
        printf("Unable to allocate memory\n");
        goto CLEANUP;
    }
    img.len = (width * height) / 2;

    lin2pln(&img, &src);

    // create/open the input file
    printf("Creating IMG File: '%s'\n", fo_name);
    if(NULL == (fo = fopen(fo_name,"wb"))) {
        printf("Error: Unable to open output file\n");
        goto CLEANUP;
    }
    int nr = fwrite(img.data, img.len, 1, fo);
    if(1 != nr) {
        printf("Error Unable write file\n");
        goto CLEANUP;
    }

    printf("Done\n");
    rval = 0; // clean exit
CLEANUP:
    fclose_s(fo);
    free_s(fi_name);
    free_s(fo_name);
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

/// @brief converts a linear image to a planerized one, assumes 16 colour 4 bits per pixel
/// @param dst memstream buffer pointing to buffer large enough for packed planar image (4 bits per pixel)
/// @param src memstream buffer pointing to a buffer containing the unpacked linear image (1 byte per pixel)
void lin2pln(memstream_buf_t *dst, memstream_buf_t *src) {
    int ofs2 = dst->len / 2;  // 1/2
    int ofs1 = ofs2 / 2;      // 1/4
    int ofs3 = ofs1 + ofs2;   // 3/4
    uint8_t p0 = 0;
    uint8_t p1 = 0;
    uint8_t p2 = 0;
    uint8_t p3 = 0;

    for(int i = 0; i < ofs1; i++) {
        for(int b = 0; b < 8; b++) { // 8 pixels packed per byte
            p0 <<= 1; p1 <<= 1; p2 <<= 1; p3 <<= 1; // make room for the next pixel
            uint8_t px = 0;
            if(src->pos < src->len) px = src->data[src->pos++];
            p0 |= px & 0x01; px >>= 1;
            p1 |= px & 0x01; px >>= 1;
            p2 |= px & 0x01; px >>= 1;
            p3 |= px & 0x01; px >>= 1; // final shift
        }

        dst->data[       i] = p0;
        dst->data[ofs1 + i] = p1;
        dst->data[ofs2 + i] = p2;
        dst->data[ofs3 + i] = p3;
    }
}

/// @brief loads the BMP image from a file, assumes 16 colour image. palette is ignored, assumed to follow 
///        CGA/EGA/VGA standard palette
/// @param dst pointer to a empty memstream buffer struct. load_bmp will allocate the buffer, image will be stored as 1 byte per pixel
/// @param fn name of file to load
/// @param width  pointer to width of the image in pixels set on return
/// @param height pointer to height of the image in pixels or lines set on return
/// @return  0 on success, otherwise an error code
int load_bmp(memstream_buf_t *dst, const char *fn, uint16_t *width, uint16_t *height) {
    int rval = 0;
    FILE *fp = NULL;
    uint8_t *buf = NULL; // line buffer
    bmp_header_t *bmp = NULL;

    // do some basic error checking on the inputs
    if((NULL == fn) || (NULL == dst) || (NULL == width) || (NULL == height)) {
        rval = -1;  // NULL pointer error
        goto bmp_cleanup;
    }

    // try to open input file
    if(NULL == (fp = fopen(fn,"rb"))) {
        rval = -2;  // can't open input file
        goto bmp_cleanup;
    }

    bmp_signature_t sig = 0;
    int nr = fread(&sig, sizeof(bmp_signature_t), 1, fp);
    if(1 != nr) {
        rval = -3;  // unable to read file
        goto bmp_cleanup;
    }
    if(BMPFILESIG != sig) {
        rval = -4; // not a BMP file
        goto bmp_cleanup;
    }

    // allocate a buffer to hold the header 
    if(NULL == (bmp = calloc(1, sizeof(bmp_header_t)))) {
        rval = -5;  // unable to allocate mem
        goto bmp_cleanup;
    }
    nr = fread(bmp, sizeof(bmp_header_t), 1, fp);
    if(1 != nr) {
        rval = -3;  // unable to read file
        goto bmp_cleanup;
    }

    // check some basic header vitals to make sure it's in a format we can work with
    if((1 != bmp->bmi.num_planes) || 
       (sizeof(bmi_header_t) != bmp->bmi.header_size) || 
       (0 != bmp->dib.RES)) {
        rval = -6;  // invalid header
        goto bmp_cleanup;
    }
    if((4 != bmp->bmi.bits_per_pixel) || 
       (16 != bmp->bmi.num_colors) || 
       (0 != bmp->bmi.compression)) {
        rval = -7;  // unsupported BMP format
        goto bmp_cleanup;
    }
    
    // seek to the start of the image data, as we don't use the palette data
    // we assume the standard CGA/EGA/VGA 16 colour palette
    fseek(fp, bmp->dib.image_offset, SEEK_SET);

    // check if the destination buffer is null, if not, free it
    // we will allocate it ourselves momentarily
    if(NULL != dst->data) {
        free(dst->data);
        dst->data = NULL;
    }

    // if height is negative, flip the render order
    bool flip = (bmp->bmi.image_height < 0); 
    bmp->bmi.image_height = abs(bmp->bmi.image_height);

    uint16_t lw = bmp->bmi.image_width;
    uint16_t lh = bmp->bmi.image_height;

    // stride is the bytes per line in the BMP file, which are padded
    // we get 2 pixels per byte for being 16 colour
    uint32_t stride = ((lw + 3) & (~0x0003)) / 2; 

    // allocate our line and output buffers
    if(NULL == (dst->data = calloc(1, lw * lh))) {
        rval = -5;  // unable to allocate mem
        goto bmp_cleanup;
    }
    dst->len = lw * lh;
    dst->pos = 0;

    if(NULL == (buf = calloc(1, stride))) {
        rval = -5;  // unable to allocate mem
        goto bmp_cleanup;
    }

    // now we need to read the image scanlines. 
    // start by pointing to start of last line of data
    uint8_t *px = &dst->data[dst->len - lw]; 
    if(flip) px = dst->data; // if flipped, start at beginning
    // loop through the lines
    for(int y = 0; y < lh; y++) {
        nr = fread(buf, stride, 1, fp); // read a line
        if(1 != nr) {
            rval = -3;  // unable to read file
            goto bmp_cleanup;
        }

        // loop through all the pixels for a line
        // we are packing 2 pixels per byte, so width is half
        for(int x = 0; x < ((lw + 1) / 2); x++) {
            uint8_t sp = buf[x];      // get the pixel pair
            *px++ = (sp >> 4) & 0x0f; // write the 1st pixel
            if((x * 2 + 1) < lw) {    // test for odd pixel end
                *px++ = sp & 0x0f;    // write the 2nd pixel
            }
        }
        if(!flip) { // if not flipped, wehave to walk backwards
            px -= (lw * 2); // move back to start of previous line
        }
    }

    *width = lw;
    *height = lh;

bmp_cleanup:
    fclose_s(fp);
    free_s(buf);
    free_s(bmp);
    return rval;
}