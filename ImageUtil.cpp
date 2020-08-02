#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "ImageUtil.h"

#include <stdexcept>


float* ImageUtil::ReadImage(char filepath[], float gamma, int* width, int* height, int* returned_components, int components)
{
	// Load the image onto the device - explicitly handle colorspace ourselves instead of stbi_loadf
	unsigned char* inData = stbi_load(filepath, width, height, returned_components, components);
	if (inData == nullptr)
	{
		throw std::runtime_error("Failed to load source image");
	}
	int numElements = (*width) * (*height) * components;
	float* srcData = new float[numElements];
	for (int i = 0; i < numElements; i++)
	{
		srcData[i] = powf(inData[i] / 255.0f, gamma);
	}
	delete[] inData;

	return srcData;
}

void ImageUtil::WriteImage(char filepath[], float* imgData, int width, int height, int components, float gamma, bool fill_alpha)
{
	// Convert float image to unsigned char for use in stb - explictly handle colorspace
	int numElements = width * height * components;
	unsigned char* outData = new unsigned char[numElements];
	for (int i = 0; i < numElements; i++)
	{
		// If the source image did not provide an alpha, then output a constant full alpha
		if (fill_alpha && i % 4 == 3)
		{
			outData[i] = 255;
		}
		else
		{
			outData[i] = static_cast<unsigned char>(powf(imgData[i], gamma) * 255);
		}
	}

	// Output the image
	stbi_write_png(filepath, width, height, components, outData, width * components);
	delete[] outData;
}
