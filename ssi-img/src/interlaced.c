#include "ssi-img.h"

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


void lin2lace(memstream_buf_t *dst, memstream_buf_t *src, uint16_t width, uint16_t height) {
    width /= 4;  // we expect 4 pixels per byte
    height /= 2; // we always expect lines to be in interleved pairs

    int even_pos = 0;
    int odd_pos = dst->len / 2; // 1/2

    for(int y = 0; y < height; y++) {
        // even line
        for(int x = 0; x < width; x++) {
            uint8_t px = 0;
            for(int b = 0; b < 4; b++) { // 4 pixels per byte
                px <<= 2; // make room for the next pixel
                px |= src->data[src->pos++] & 0x03;
            }
            dst->data[even_pos++] = px;
        }
        // odd line
        for(int x = 0; x < width; x++) {
            uint8_t px = 0;
            for(int b = 0; b < 4; b++) { // 4 pixels per byte
                px <<= 2; // make room for the next pixel
                px |= src->data[src->pos++] & 0x03;
            }
            dst->data[odd_pos++] = px;
        }
    }
}
