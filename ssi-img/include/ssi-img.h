#include "memstream.h"


/// @brief converts a planerized image to a linear one, assumes 16 colour 4 bits per pixel
/// @param dst memstream buffer pointing to buffer large enough for 1 byte per pixel
/// @param src memstream buffer pointing to a buffer containing the packed planar image
void pln2lin(memstream_buf_t *dst, memstream_buf_t *src);

/// @brief converts an interleaved planerized image to a linear one, assumes 16 colour 4 bits per pixel
/// @param dst memstream buffer pointing to buffer large enough for 1 byte per pixel
/// @param src memstream buffer pointing to a buffer containing the packed planar image
/// @param width  // image width
/// @param height // inmage height
void ipln2lin(memstream_buf_t *dst, memstream_buf_t *src, uint16_t width, uint16_t height);

/// @brief converts a interleved image to a linear one, assumes 4 colour 2 bits per pixel
/// @param dst memstream buffer pointing to buffer large enough for 1 byte per pixel
/// @param src memstream buffer pointing to a buffer containing the packed planar image
void lace2lin(memstream_buf_t *dst, memstream_buf_t *src, uint16_t width, uint16_t height);

/// @brief converts a linear image to a planerized one, assumes 16 colour 4 bits per pixel
/// @param dst memstream buffer pointing to buffer large enough for packed planar image (4 bits per pixel)
/// @param src memstream buffer pointing to a buffer containing the unpacked linear image (1 byte per pixel)
void lin2pln(memstream_buf_t *dst, memstream_buf_t *src);

/// @brief converts a linear image to an interleaved planerized one, assumes 16 colour 4 bits per pixel
/// @param dst memstream buffer pointing to buffer large enough for packed planar image (4 bits per pixel)
/// @param src memstream buffer pointing to a buffer containing the unpacked linear image (1 byte per pixel)
/// @param width  // image width
/// @param height // inmage height
void lin2ipln(memstream_buf_t *dst, memstream_buf_t *src, uint16_t width, uint16_t height);

/// @brief converts the image form a linear one to a interleved and packed one, assumes 2 bits per pixel
/// @param dst // destination buffer for the packed image, expected to be 16384 bytes
/// @param src // buffer containing the unpacked source image, 1 byte per pixel
/// @param width  // image width
/// @param height // inmage height
void lin2lace(memstream_buf_t *dst, memstream_buf_t *src, uint16_t width, uint16_t height);
