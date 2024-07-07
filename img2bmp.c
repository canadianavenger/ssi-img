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

// default CGA/EGA/VGA 16 colour palette
static const bmp_palette_entry_t pal[16] = { 
  {0x00,0x00,0x00,0x00}, {0xaa,0x00,0x00,0x00}, {0x00,0xaa,0x00,0x00}, {0xaa,0xaa,0x00,0x00}, 
  {0x00,0x00,0xaa,0x00}, {0xaa,0x00,0xaa,0x00}, {0x00,0x55,0xaa,0x00}, {0xaa,0xaa,0xaa,0x00},
  {0x55,0x55,0x55,0x00}, {0xff,0x55,0x55,0x00}, {0x55,0xff,0x55,0x00}, {0xff,0xff,0x55,0x00}, 
  {0x55,0x55,0xff,0x00}, {0xff,0x55,0xff,0x00}, {0x55,0xff,0xff,0x00}, {0xff,0xff,0xff,0x00},
};
 
int save_bmp(const char *dst, memstream_buf_t *src, uint16_t width, uint16_t height);
void pln2lin(memstream_buf_t *dst, memstream_buf_t *src);

size_t filesize(FILE *f);
void drop_extension(char *fn);
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

    printf("SSI-IMG to BMP image converter\n");

    if(argc < 3) {
        printf("USAGE %s [resolution] [infile] <outfile>\n", argv[0]);
        printf("where [resolution] is in the form width x height eg '320x200'\n");
        printf("[infile] is the name of the input file\n");
        printf("<outfile> is optional and the name of the output file\n");
        printf("if omitted, the output will be named the same as infile, except with a .BMP extension\n");
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
    sscanf(resolution, "%hu%*[xX]%hu", &width, &height);
    printf("Resolution: %d x %d\n", width, height);

    // allocate buffers based on resolution
    // allocate the packed image buffer (width * height) / 2
    if(NULL == (img.data = calloc(height, width/2))) {
        printf("Unable to allocate memory\n");
        goto CLEANUP;
    }
    img.len = (width * height) / 2;
    // allocate the deplaned/unpaced image buffer (width * height)
    if(NULL == (src.data = calloc(height, width))) {
        printf("Unable to allocate memory\n");
        goto CLEANUP;
    }
    src.len = (width * height);

    // open the input file
    printf("Opening IMG File: %s", fi_name);
    if(NULL == (fi = fopen(fi_name,"rb"))) {
        printf("Error: Unable to open input file\n");
        goto CLEANUP;
    }

        // determine size of image file
    size_t fsz = filesize(fi);
    printf("\tFile Size: %zu\n", fsz);

    if(fsz != img.len) {
        printf("File image and Specified image size mismatch\n");
        goto CLEANUP;
    }

    int nr = fread(img.data, img.len, 1, fi);
    if(1 != nr) {
        printf("Error Unable read file\n");
        goto CLEANUP;
    }

    pln2lin(&src, &img); // deplane the image
    src.pos = 0; // reset the src position

    printf("Creating BMP File: %s\n", fo_name);
    rval = save_bmp(fo_name, &src, width, height);
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
    free_s(img.data);
    free_s(src.data);
    return rval;
}

/// @brief determins the size of the file
/// @param f handle to an open file
/// @return returns the size of the file
size_t filesize(FILE *f) {
    size_t szll, cp;
    cp = ftell(f); // save current position
    fseek(f, 0, SEEK_END); // find the end
    szll = ftell(f); // get positon of the end
    fseek(f, cp, SEEK_SET); // restor the file position
    return szll; // return position of the end as size
}

/// @brief removes the extension form a filename
/// @param fn sting pointer to the filename
void drop_extension(char *fn) {
    char *extension = strrchr(fn, '.');
    if(NULL != extension) *extension = 0; // strip out the existing extension
}

/// @brief converts a panerized image to a linear one
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


// allocate a header buffer large enough for all 3 parts, plus 16 bit padding at the start to 
// maintian 32 bit alignment after the 16 bit signature.
#define HDRBUFSZ (sizeof(bmp_signature_t) + sizeof(bmp_header_t))

/// @brief saves the image pointed to by src as a BMP, assumes 16 colour 1 byte per pixel image data
/// @param fn name of the file to create and write to
/// @param src memstream buffer pointer to the source image data
/// @param width  width of the image in pixels
/// @param height height of the image in pixels or lines
/// @return 0 on success, otherwise an error code
int save_bmp(const char *fn, memstream_buf_t *src, uint16_t width, uint16_t height) {
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
    bmp->dib.image_offset = HDRBUFSZ + sizeof(pal);
    bmp->dib.file_size = bmp->dib.image_offset + bmp_img_sz;

    // setup the bmi header fields
    bmp->bmi.header_size = sizeof(bmi_header_t);
    bmp->bmi.image_width = width;
    bmp->bmi.image_height = height;
    bmp->bmi.num_planes = 1;     // always 1
    bmp->bmi.bits_per_pixel = 4; // 16 colour image
    bmp->bmi.compression = 0;    // uncompressed
    bmp->bmi.bitmap_size = bmp_img_sz;
    bmp->bmi.horiz_res = BMP96DPI;
    bmp->bmi.vert_res = BMP96DPI;
    bmp->bmi.num_colors = 16; // palette has 16 colours
    bmp->bmi.important_colors = 0; // all colours are important

    // write out the header
    int nr = fwrite(sig, HDRBUFSZ, 1, fp);
    if(1 != nr) {
        rval = -4;  // unable to write file
        goto bmp_cleanup;
    }

    // we're using our global palette here, wich is already in BMP format
    // write out the palette
    nr = fwrite(pal, sizeof(pal), 1, fp);
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
        bzero(buf, stride); // zero out the line in the output buffer
        // loop through all the pixels for a line
        // we are packing 2 pixels per byte, so width is half
        for(int x = 0; x < ((width + 1) / 2); x++) {
            uint8_t sp = *px++; // get the first pixel
            sp <<= 4; // shift it to make room for the next one
            if((x * 2 + 1) < width) { // test for odd pixel end
                sp |= (*px++) & 0x0f; // get the next pixel
            }
            buf[x] = sp; // write it to the line buffer
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
