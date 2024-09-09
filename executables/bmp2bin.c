/*
 * bmp2img-ega.c 
 * Converts a given Windows BMP file to a SSI-BIN/IMG (for EGA/VGA graphics)
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
#include "ssi-img.h"
#include "bmp.h"
#include "util.h"

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

    printf("BMP to SSI-BIN IMG image converter\n");

    if((argc < 2) || (argc > 3)) {
        printf("USAGE: %s [infile] <outfile>\n", filename(argv[0]));
        printf("[infile] is the name of the input file\n");
        printf("<outfile> is optional and the name of the output file\n");
        printf("if omitted, outfile will be named the same as infile with a .BIN extension\n");
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
        strncat(fo_name,".BIN", namelen+4); // add bmp extension
    }

    printf("Loading BMP File: '%s'\n", fi_name);
    rval = load_bmp4(&src, fi_name, &width, &height);
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

    lin2ipln(&img, &src, width, height);

    // create/open the output file
    printf("Creating BIN File: '%s'\n", fo_name);
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
