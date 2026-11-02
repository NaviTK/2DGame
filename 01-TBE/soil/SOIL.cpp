#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "SOIL.h"

unsigned char* SOIL_load_image(const char *filename, int *width, int *height, int *channels, int force_channels)
{
	int dummy = 0;
	return stbi_load(filename, width, height, channels ? channels : &dummy, force_channels);
}

void SOIL_free_image_data(unsigned char *result_data)
{
	if(result_data)
		stbi_image_free(result_data);
}
