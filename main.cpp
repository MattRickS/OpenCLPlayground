#define __CL_ENABLE_EXCEPTIONS
#include <CL/cl.hpp>

#include "GaussianBlur.h"
#include "CLUtil.h"
#include "ImageUtil.h"
#include "Op.h"

#include <iostream>

using OpList = std::vector<std::shared_ptr<Op::Operator>>;

OpList ParseOperators(int argc, char* argv[], int start)
{
	static const std::string sep("--");
	OpList operators;
	for (int i = start; i <= argc; i++)
	{
		if (i == argc || argv[i] == sep)
		{
			char *kernelName = argv[start];
			std::shared_ptr<Op::Operator> op = Op::Registry::Create(kernelName);
			if (op == nullptr)
			{
				std::cerr << "Unknown kernel: " << kernelName << std::endl;
				return OpList();
			}
			if (!op->Parse(i - start - 1, argv + start + 1))
			{
				op->PrintUsage();
				return OpList();
			}
			operators.push_back(op);
			start = ++i;
		}
	}

	return operators;
}

int main(int argc, char *argv[])
{
	if (argc < 5)
	{
		std::cerr << "Usage: " << argv[0] << " DESTINATION GAMMA KERNEL [SETTINGS] [-- KERNEL [SETTINGS] ...]" << std::endl;
		return 1;
	}

	char *destination = argv[1];
	float gammaInv = 1.0f / std::stof(argv[2]);

	// Parse all operators and their settings
	OpList operators = ParseOperators(argc, argv, 3);
	if (operators.empty())
	{
		return 1;
	}

	try
	{
		// Start a CL context
		cl::Context context = CLUtil::CreateContext();
		auto devices = context.getInfo<CL_CONTEXT_DEVICES>();
		cl::Device &device = devices.front();

		// Execute the op queue -- TODO: Operators should actually add to the queue and the whole queue executed once with events for waiting
		cl::CommandQueue queue(context, device);
		std::shared_ptr<cl::Image> current(nullptr);
		for (const auto& op : operators)
		{
			if (!op->Execute(context, queue, current))
			{
				throw std::runtime_error("Operator failed to execute");
			}
			current = op->outputImage;
		}

		int width = (int)current->getImageInfo<CL_IMAGE_WIDTH>();
		int height = (int)current->getImageInfo<CL_IMAGE_HEIGHT>();
		// TODO: Actually handle image formats rather than guessing
		int components = 4;

		// Read the image back onto the host. Whatever the source image components, the kernel returns a float4 image
		cl::size_t<3> origin;
		cl::size_t<3> region;
		region[0] = width;
		region[1] = height;
		region[2] = 1;
		float *dstData = new float[width * height * components];
		queue.enqueueReadImage(*current, CL_TRUE, origin, region, 0, 0, (void *)dstData);

		// Output the image - fill the alpha unless the source imaged had an alpha to be modified
		// TODO: "fill alpha" shouldn't be an option, should just respect input
		ImageUtil::WriteImage(destination, dstData, width, height, components, gammaInv, true);
		delete[] dstData;

		return 0;
	}
	catch (cl::Error error)
	{
		std::cerr << error.what() << '(' << error.err() << ')' << std::endl;
		return 1;
	}
}
