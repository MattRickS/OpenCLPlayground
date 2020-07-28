#pragma once

namespace ImageUtil
{
	float* ReadImage(char filepath[], float gamma, int* width, int* height, int components);
	void WriteImage(char filepath[], float* imgData, int width, int height, int components, float gamma);
};
