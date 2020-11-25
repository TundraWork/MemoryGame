#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "loadbmp.h"

/** Alloc a piece of memory to save bitmap data **/
static void *bitmap_create(int width, int height, unsigned int state)
{
	(void)state;  /* unused */
				  /* ensure a stupidly large (>50Megs or so) bitmap is not created */
	if (((long long)width * (long long)height) > (MAX_IMAGE_BYTES / BYTES_PER_PIXEL)) {
		return NULL;
	}
	return calloc(width * height, BYTES_PER_PIXEL);
}

/** Prevent to return a invalid bitmap **/
static unsigned char *bitmap_get_buffer(void *bitmap)
{
	assert(bitmap);
	return bitmap;
}

/** Return bytes-per-pixel of the bitmap **/
static size_t bitmap_get_bpp(void *bitmap)
{
	(void)bitmap;  /* unused */
	return BYTES_PER_PIXEL;
}

/** Free bitmap data **/
static void bitmap_destroy(void *bitmap)
{
	assert(bitmap);
	free(bitmap);
}

/** Write readable bitmap data (convert binary to text format) to a text file.
 *  This function is only for testing. **/
void write_data(FILE* fh, struct bmp_image *bmp)
{
	uint16_t row, col;
	uint8_t *image;

	printf("%dx%d %d\n", bmp->width, bmp->height, bmp->bpp);

	image = (uint8_t *)bmp->bitmap;
	for (row = 0; row != bmp->height; row++) {
		for (col = 0; col != bmp->width; col++) {
			size_t z = (row * bmp->width + col) * BYTES_PER_PIXEL;
			fprintf(fh, "%u %u %u ",
				image[z],
				image[z + 1],
				image[z + 2]);
		}
		fprintf(fh, "\n");
	}

}

/** Load a binary file to memory. **/
static unsigned char *load_file(const char *path, size_t *data_size)
{
	FILE *fd;
	struct stat sb;
	unsigned char *buffer;
	size_t size;
	size_t n;

	fd = fopen(path, "rb");
	if (!fd) {
		perror(path);
		exit(EXIT_FAILURE);
	}

	if (stat(path, &sb)) {
		perror(path);
		exit(EXIT_FAILURE);
	}
	size = sb.st_size;

	buffer = malloc(size);
	if (!buffer) {
		fprintf(stderr, "Unable to allocate %lld bytes\n",
			(long long)size);
		exit(EXIT_FAILURE);
	}

	n = fread(buffer, 1, size, fd);
	if (n != size) {
		perror(path);
		exit(EXIT_FAILURE);
	}

	fclose(fd);

	*data_size = size;
	return buffer;
}

/** Load image data from BMP file using libnsbmp. **/
bmp_result load_bmp(const char* path, bmp_image *result)
{
	bmp_bitmap_callback_vt bitmap_callbacks = {
		bitmap_create,
		bitmap_destroy,
		bitmap_get_buffer,
		bitmap_get_bpp
	};
	bmp_result code;
	bmp_image bmp;
	size_t size;

	/* create our bmp image */
	bmp_create(&bmp, &bitmap_callbacks);

	/* load file into memory */
	unsigned char *data = load_file(path, &size);

	/* analyse the BMP */
	code = bmp_analyse(&bmp, size, data);
	if (code != BMP_OK) {
		goto cleanup;
	}

	/* decode the image */
	code = bmp_decode(&bmp);
	/* code = bmp_decode_trans(&bmp, TRANSPARENT_COLOR); */
	if (code != BMP_OK) {
		/* allow partially decoded images */
		if ((code != BMP_INSUFFICIENT_DATA) &&
			(code != BMP_DATA_ERROR)) {
			goto cleanup;
		}

		/* stop if the decoded image would be ridiculously large */
		if ((bmp.width * bmp.height) > 4194304) {
			code = BMP_DATA_ERROR;
			goto cleanup;
		}
	}

	*result = bmp;
	return code;

cleanup:
	/* clean up */
	bmp_finalise(&bmp);
	free(data);
	return code;
}
