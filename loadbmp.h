#pragma once

#ifndef loadbmp_h_
#define loadbmp_h_

#include <stdio.h>
#include <stdlib.h>
#include "libnsbmp.h"

#define BYTES_PER_PIXEL 4
#define MAX_IMAGE_BYTES (48 * 2048 * 2048)
#define TRANSPARENT_COLOR 0xffffffff

/** Write readable bitmap data (convert binary to text format) to a text file.
 *  This function is only for testing. **/
void write_data(FILE* fh, struct bmp_image *bmp);

/** Load image data from BMP file using libnsbmp. **/
bmp_result load_bmp(const char* path, bmp_image *result);

#endif