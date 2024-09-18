#include "ssi-img.h"

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

void ipln2lin(memstream_buf_t *dst, memstream_buf_t *src, uint16_t width, uint16_t height) {
    int step = width / 2; // bytes per line
    int ofs2 = step / 2;      // 1/2
    int ofs1 = ofs2 / 2;      // 1/4
    int ofs3 = ofs1 + ofs2;   // 3/4
    int base = 0;
    for(int y = 0; y < height; y++) {
        for(int x = 0; x < (width / 8); x++) {
            uint8_t p0 = src->data[base + x];
            uint8_t p1 = src->data[base + ofs1 + x];
            uint8_t p2 = src->data[base + ofs2 + x];
            uint8_t p3 = src->data[base + ofs3 + x];

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
        base += step;
    }
}


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

void lin2ipln(memstream_buf_t *dst, memstream_buf_t *src, uint16_t width, uint16_t height) {
    int step = width / 2; // bytes per line
    int ofs2 = step / 2;      // 1/2
    int ofs1 = ofs2 / 2;      // 1/4
    int ofs3 = ofs1 + ofs2;   // 3/4

    int base = 0;
    for(int y = 0; y < height; y++) {
        for(int x = 0; x< (width / 8); x++) {
            uint8_t p0 = 0;
            uint8_t p1 = 0;
            uint8_t p2 = 0;
            uint8_t p3 = 0;
            for(int b = 0; b < 8; b++) { // 8 pixels packed per byte
                p0 <<= 1; p1 <<= 1; p2 <<= 1; p3 <<= 1; // make room for the next pixel
                uint8_t px = 0;
                if(src->pos < src->len) px = src->data[src->pos++];
                p0 |= px & 0x01; px >>= 1;
                p1 |= px & 0x01; px >>= 1;
                p2 |= px & 0x01; px >>= 1;
                p3 |= px & 0x01; px >>= 1; // final shift
            }
            // write it to the output
            dst->data[base +        x] = p0;
            dst->data[base + ofs1 + x] = p1;
            dst->data[base + ofs2 + x] = p2;
            dst->data[base + ofs3 + x] = p3;
        }
        base += step;
    }
}
