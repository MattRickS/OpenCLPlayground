#define __CL_ENABLE_EXCEPTIONS
#include <CL/cl.hpp>

#include "GaussianBlur.h"
#include "CLUtil.h"
#include "ImageUtil.h"
#include "Op.h"

#include <iostream>

int main(int argc, char *argv[])
{
	if (argc < 5)
	{
		std::cerr << "Usage: " << argv[0] << " SOURCE DESTINATION GAMMA KERNEL [SETTINGS] [-- KERNEL [SETTINGS] ...]" << std::endl;
		return 1;
	}

	char *source = argv[1];
	char *destination = argv[2];
	float gamma = std::stof(argv[3]);
	float gammaInv = 1.0f / gamma;

	// Parse all operators and their settings
	std::vector<std::shared_ptr<Op::Operator>> operators;
	int start = 4;
	const std::string sep("--");
	for (int i = start; i <= argc; i++)
	{
		if (i == argc || argv[i] == sep)
		{
			char *kernelName = argv[start];
			std::shared_ptr<Op::Operator> op = Op::Registry::Create(kernelName);
			if (op == nullptr)
			{
				std::cerr << "Unknown kernel: " << kernelName << std::endl;
				return 1;
			}
			if (!op->Parse(i - start - 1, argv + start + 1))
			{
				op->PrintUsage();
				return 1;
			}
			operators.push_back(op);
			start = ++i;
		}
	}

	try
	{
		// Start a CL context
		cl::Context context = CLUtil::CreateContext();
		auto devices = context.getInfo<CL_CONTEXT_DEVICES>();
		cl::Device &device = devices.front();

		// Load the image onto the device - explicitly handle colorspace ourselves instead of stbi_loadf
		int width, height, src_components, components = 4;
		float *srcData = ImageUtil::ReadImage(source, gamma, &width, &height, &src_components, components);
		cl::Image2D src(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, cl::ImageFormat(CL_RGBA, CL_FLOAT), width, height, 0, (void *)srcData);
		delete[] srcData;

		// Create an output image on the device
		cl::Image2D dst(context, CL_MEM_READ_WRITE, cl::ImageFormat(CL_RGBA, CL_FLOAT), width, height, 0, nullptr);

		// Execute the op queue -- TODO: Operators should actually add to the queue and the whole queue executed once with events for waiting
		cl::CommandQueue queue(context, device);
		cl::Image2D* from = &src;
		cl::Image2D* to = &dst;
		for (const auto& op : operators)
		{
			op->Execute(context, queue, *from, *to);
			// Ping pong the images. Note, this leaves the "from" pointer as the final output
			cl::Image2D* tmp = from;
			from = to;
			to = tmp;
		}

		// Read the image back onto the host. Whatever the source image components, the kernel returns a float4 image
		cl::size_t<3> origin;
		cl::size_t<3> region;
		region[0] = width;
		region[1] = height;
		region[2] = 1;
		float *dstData = new float[width * height * components];
		queue.enqueueReadImage(*from, CL_TRUE, origin, region, 0, 0, (void *)dstData);

		// Output the image - fill the alpha unless the source imaged had an alpha to be modified
		ImageUtil::WriteImage(destination, dstData, width, height, components, gammaInv, src_components != 4);
		delete[] dstData;

		return 0;
	}
	catch (cl::Error error)
	{
		std::cerr << error.what() << '(' << error.err() << ')' << std::endl;
		return 1;
	}
}
