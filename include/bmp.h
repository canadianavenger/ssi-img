/*
 * bmp.h 
 * interface definitions for a writing an indexed 256 colour Windows BMP file
 * 
 * This code is offered without warranty under the MIT License. Use it as you will 
 * personally or commercially, just give credit if you do.
 */
#include <stdint.h>
#include "memstream.h"
#include "pal.h"

#ifndef IMG_BMP
#define IMG_BMP

/// @brief saves the image pointed to by src as a BMP, assumes 256 colour 1 byte per pixel image data
/// @param fn name of the file to create and write to
/// @param src memstream buffer pointer to the source image data
/// @param width  width of the image in pixels
/// @param height height of the image in pixels or lines
/// @param pal pointer to 256 entry RGB palette
/// @return 0 on success, otherwise an error code
int save_bmp8(const char *fn, memstream_buf_t *src, uint16_t width, uint16_t height, pal_entry_t *xpal);

/// @brief saves the image pointed to by src as a BMP, assumes 16 colour 1 byte per pixel image data
/// @param fn name of the file to create and write to
/// @param src memstream buffer pointer to the source image data
/// @param width  width of the image in pixels
/// @param height height of the image in pixels or lines
/// @param pal pointe to 16 entry palette
/// @return 0 on success, otherwise an error code
int save_bmp4(const char *fn, memstream_buf_t *src, uint16_t width, uint16_t height, pal_entry_t *xpal);

/// @brief loads the BMP image from a file, assumes 16 colour image. palette is ignored, assumed to follow 
///        CGA/EGA/VGA standard palette
/// @param dst pointer to a empty memstream buffer struct. load_bmp will allocate the buffer, image will be stored as 1 byte per pixel
/// @param fn name of file to load
/// @param width  pointer to width of the image in pixels set on return
/// @param height pointer to height of the image in pixels or lines set on return
/// @return  0 on success, otherwise an error code
int load_bmp4(memstream_buf_t *dst, const char *fn, uint16_t *width, uint16_t *height);

#endif
