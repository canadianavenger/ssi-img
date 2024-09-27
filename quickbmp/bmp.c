#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bmp_int.h"
#include "bmp.h"
#include "util.h"
#include <stdbool.h>

// allocate a header buffer large enough for all 3 parts, plus 16 bit padding at the start to 
// maintian 32 bit alignment after the 16 bit signature.
#define HDRBUFSZ (sizeof(bmp_signature_t) + sizeof(bmp_header_t))

int save_bmp8(const char *fn, memstream_buf_t *src, uint16_t width, uint16_t height, pal_entry_t *xpal) {
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
    uint32_t stride = ((width + 3) & (~0x0003)); 
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
    size_t palsz = sizeof(bmp_palette_entry_t) * 256;
    bmp->dib.image_offset = HDRBUFSZ + palsz;
    bmp->dib.file_size = bmp->dib.image_offset + bmp_img_sz;

    // setup the bmi header fields
    bmp->bmi.header_size = sizeof(bmi_header_t);
    bmp->bmi.image_width = width;
    bmp->bmi.image_height = height;
    bmp->bmi.num_planes = 1;           // always 1
    bmp->bmi.bits_per_pixel = 8;       // 256 colour image
    bmp->bmi.compression = 0;          // uncompressed
    bmp->bmi.bitmap_size = bmp_img_sz;
    bmp->bmi.horiz_res = BMP96DPI;
    bmp->bmi.vert_res = BMP96DPI;
    bmp->bmi.num_colors = 256;         // palette has 256 colours
    bmp->bmi.important_colors = 0;     // all colours are important

    // write out the header
    int nr = fwrite(sig, HDRBUFSZ, 1, fp);
    if(1 != nr) {
        rval = -4;  // unable to write file
        goto bmp_cleanup;
    }

    bmp_palette_entry_t *pal = calloc(256, sizeof(bmp_palette_entry_t));
    if(NULL == pal) {
        rval = -3;  // unable to allocate mem
        goto bmp_cleanup;
    }

    // copy the external RGB palette to the BMP BGRA palette
    for(int i = 0; i < 256; i++) {
        pal[i].r = xpal[i].r;
        pal[i].g = xpal[i].g;
        pal[i].b = xpal[i].b;
    }

    // write out the palette
    nr = fwrite(pal, palsz, 1, fp);
    if(1 != nr) {
        rval = -4;  // can't write file
        goto bmp_cleanup;
    }
    // we can free the palette now as we don't need it anymore
    free(pal);

    // now we need to output the image scanlines. For maximum
    // compatibility we do so in the natural order for BMP
    // which is from bottom to top. 
    // start by pointing to start of last line of data
    uint8_t *px = &src->data[src->len - width];
    // loop through the lines
    for(int y = 0; y < height; y++) {
        memset(buf, 0, stride); // zero out the line in the output buffer
        // loop through all the pixels for a line
        for(int x = 0; x < width; x++) {
            buf[x] = *px++;
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

int save_bmp4(const char *fn, memstream_buf_t *src, uint16_t width, uint16_t height, pal_entry_t *xpal) {
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
    uint32_t stride = ((((width + 1) / 2) + 3) & (~0x0003)); // we get 2 pixels per byte for being 16 colour
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

    bmp_palette_entry_t *pal = calloc(16, sizeof(bmp_palette_entry_t));
    if(NULL == pal) {
        rval = -3;  // unable to allocate mem
        goto bmp_cleanup;
    }

    // copy the external RGB palette to the BMP BGRA palette
    for(int i = 0; i < 16; i++) {
        pal[i].r = xpal[i].r;
        pal[i].g = xpal[i].g;
        pal[i].b = xpal[i].b;
    }

    // write out the palette
    nr = fwrite(pal, palsz, 1, fp);
    if(1 != nr) {
        rval = -4;  // can't write file
        goto bmp_cleanup;
    }
    // we can free the palette now as we don't need it anymore
    free(pal);

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

int load_bmp4(memstream_buf_t *dst, const char *fn, uint16_t *width, uint16_t *height) {
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