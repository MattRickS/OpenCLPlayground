#include "GaussianBlur.h"
#include "CLUtil.h"

#include <CL/cl.hpp>

#include <cmath>
#include <iostream>
#include <string>

const float PI = 3.14159265358979f;

namespace Op
{
	float* GaussianBlur::Distribution(float sigma, int radius) const
	{
		int maskSize = radius * 2 + 1;
		float* mask = new float[maskSize * maskSize];
		float sum = 0.0f;
		for (int y = -radius; y < radius + 1; y++)
		{
			for (int x = -radius; x < radius + 1; x++)
			{
				float pow = (float)(y * y + x * x) / (2 * sigma * sigma);
				float weight = exp(-pow) * (1.0f / (2 * PI * sigma * sigma));
				mask[y + radius + (x + radius) * maskSize] = weight;
				sum += weight;
			}
		}
		return mask;
	}

	void GaussianBlur::PrintUsage() const
	{
		std::cerr << "Usage: GaussianBlur [RADIUS] [STRENGTH]" << std::endl;
	}

	bool GaussianBlur::Parse(int argc, char* argv[])
	{
		if (argc > 2)
		{
			return false;
		}

		switch (argc)
		{
		case 2:
			strength = std::stof(argv[1]);
		case 1:
			radius = std::stoi(argv[0]);
		default:
			break;
		}

		return true;
	}

	void GaussianBlur::Execute(cl::Context &context, cl::CommandQueue& queue, cl::Image &src, cl::Image &dst)
	{
		// Load a convolve kernel on the device
		float* convolve = Distribution(strength, radius);
		cl::Buffer mask(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, (radius * 2 + 1) * (radius * 2 + 1) * sizeof(float), convolve);
		delete[] convolve;

		// Prepare the kernel
		cl_int err = 0;
		cl::Program program = CLUtil::BuildProgramFromSource(context, "GaussianBlur.cl", err);
		cl::Kernel kernel(program, "GaussianBlur");
		kernel.setArg<cl::Image>(0, src);
		kernel.setArg<cl::Buffer>(1, mask);
		kernel.setArg<int>(2, radius);
		kernel.setArg<cl::Image>(3, dst);

		// Run the convolve kernel, wait for it to finish (TODO: use events)
		size_t width = src.getImageInfo<CL_IMAGE_WIDTH>();
		size_t height = src.getImageInfo<CL_IMAGE_HEIGHT>();
		queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(width, height));
		cl::finish();
	}

	REGISTER_PLUGIN(GaussianBlur, GaussianBlur::Create);
}
