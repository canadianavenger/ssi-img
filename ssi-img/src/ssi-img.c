#include "ssi-img.h"
#include "pal.h"
#include <stdio.h>
#include <stdlib.h>
#include "util.h"

int load_file(memstream_buf_t *buf, const char *fn) {
    int rval = -1;
    FILE *fi = NULL;
    memstream_buf_t src = {0, 0, NULL};

    // open the input file
    if(NULL == (fi = fopen(fn,"rb"))) {
        goto CLEANUP;
    }

    // determine size of image file
    size_t fsz = filesize(fi);

    // check size
    if(fsz != buf->len) {
        goto CLEANUP;
    }

    // read in the file
    int nr = fread(buf->data, buf->len, 1, fi);
    if(1 != nr) {
        goto CLEANUP;
    }

    rval = 0;
CLEANUP:
    fclose_s(fi);
    return rval;
}

// CGA imags have a fixed size
#define CGA_IMG_SZ (16384)
int load_cga_img(memstream_buf_t *dst, const char *fn, int width, int height) {
    int rval = -1;
    memstream_buf_t src = {0, 0, NULL};

    // allocate the packed image buffer based on the expected size
    src.len = CGA_IMG_SZ;
    if(NULL == (src.data = calloc(1, src.len))) {
        goto CLEANUP;
    }

    if(0 != load_file(&src, fn)) {
        goto CLEANUP;
    }

    lace2lin(dst, &src, width, height); // de-interlace the image

    rval = 0;
CLEANUP:
    free_s(src.data);
    return rval;
}

int load_ega_img(memstream_buf_t *dst, const char *fn, int width, int height) {
    int rval = -1;
    memstream_buf_t src = {0, 0, NULL};
    
    // allocate the packed image buffer based on the expected size
    src.len = ((width * height) / 2);
    if(NULL == (src.data = calloc(1, src.len))) {
        goto CLEANUP;
    }

    if(0 != load_file(&src, fn)) {
        goto CLEANUP;
    }

    pln2lin(dst, &src); // deplane the image

    rval = 0;
CLEANUP:
    free_s(src.data);
    return rval;
}

int load_amiga_img(memstream_buf_t *dst, const char *fn, int width, int height, pal_entry_t *pal) {
    int rval = -1;
    memstream_buf_t src = {0, 0, NULL};
    
    // allocate the packed image buffer based on the expected size
    src.len = ((width * height) / 2) + 64;
    if(NULL == (src.data = calloc(1, src.len))) {
        goto CLEANUP;
    }

    if(0 != load_file(&src, fn)) {
        goto CLEANUP;
    }

    size_t realsize = src.len;
    src.len -= 64;
    pln2lin(dst, &src); // deplane the image

    src.pos = src.len; // point to the end of the framebuffer / start of palette
    src.len = realsize;

    // read in the palette, 2 bytes per entry, 4 bits per colour
    for(int p = 0; p < 16; p++) {
        uint16_t entry = src.data[src.pos++];
        entry <<= 8;
        entry |= src.data[src.pos++];
        pal[p].b = entry & 0x0f;
        entry >>= 4;
        pal[p].g = entry & 0x0f;
        entry >>= 4;
        pal[p].r = entry & 0x0f;
    }

    rval = 0;
CLEANUP:
    free_s(src.data);
    return rval;
}

int save_cga_img(const char fn, memstream_buf_t src, int width, int height) {
    return 0;
}

int save_ega_img(const char fn, memstream_buf_t src, int width, int height) {
    return 0;
}

int save_amiga_img(const char fn, memstream_buf_t src, int width, int height, pal_entry_t *pal) {
    return 0;
}
