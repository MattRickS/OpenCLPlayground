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
		std::cerr << "Usage: " << argv[0] << " SOURCE DESTINATION GAMMA KERNEL [SETTINGS]" << std::endl;
		return 1;
	}

	char *source = argv[1];
	char *destination = argv[2];
	float gamma = std::stof(argv[3]);
	float gammaInv = 1.0f / gamma;
	char *kernelName = argv[4];

	std::shared_ptr<Op::Operator> op = Op::Registry::Create(kernelName);
	// TODO: Mapping for looking up the Kernel operator
	if (op == nullptr)
	{
		std::cerr << "Unknown kernel: " << argv[1] << std::endl;
		return 1;
	}

	if (!op->Parse(argc - 5, argv + 5))
	{
		op->PrintUsage();
		return 1;
	}

	try
	{
		// Start a CL context
		cl::Context context = CLUtil::CreateContext();
		auto devices = context.getInfo<CL_CONTEXT_DEVICES>();
		cl::Device &device = devices.front();

		// TODO: Image creation is failing for some reason
		// Load the image onto the device - explicitly handle colorspace ourselves instead of stbi_loadf
		int width, height, components = 4;
		float *srcData = ImageUtil::ReadImage(source, gamma, &width, &height, components);
		cl::Image2D src(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, cl::ImageFormat(CL_RGBA, CL_FLOAT), width, height, 0, (void *)srcData);
		delete[] srcData;

		// Create an output image on the device
		cl::Image2D dst(context, CL_MEM_WRITE_ONLY, cl::ImageFormat(CL_RGBA, CL_FLOAT), width, height, 0, nullptr);

		// Execute the op queue
		cl::CommandQueue queue(context, device);
		op->Execute(context, queue, src, dst);

		// Read the image back onto the host. Whatever the source image components, the kernel returns a float4 image
		cl::size_t<3> origin;
		cl::size_t<3> region;
		region[0] = width;
		region[1] = height;
		region[2] = 1;
		float *dstData = new float[width * height * components];
		queue.enqueueReadImage(dst, CL_TRUE, origin, region, 0, 0, (void *)dstData);

		// Output the image
		ImageUtil::WriteImage(destination, dstData, width, height, components, gammaInv);
		delete[] dstData;

		return 0;
	}
	catch (cl::Error error)
	{
		std::cerr << error.what() << '(' << error.err() << ')' << std::endl;
		return 1;
	}
}
