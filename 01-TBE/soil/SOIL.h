#ifndef HEADER_SIMPLE_OPENGL_IMAGE_LIBRARY
#define HEADER_SIMPLE_OPENGL_IMAGE_LIBRARY

#ifdef __cplusplus
extern "C" {
#endif

enum
{
	SOIL_LOAD_AUTO = 0,
	SOIL_LOAD_L = 1,
	SOIL_LOAD_LA = 2,
	SOIL_LOAD_RGB = 3,
	SOIL_LOAD_RGBA = 4
};

unsigned char* SOIL_load_image
	(
		const char *filename,
		int *width, int *height, int *channels,
		int force_channels
	);

void SOIL_free_image_data
	(
		unsigned char *result_data
	);

#ifdef __cplusplus
}
#endif

#endif
