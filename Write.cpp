#include <iostream>
#include "Op.h"
#include "Write.h"
#include "CLUtil.h"
#include "ImageUtil.h"

namespace Op
{
	bool Write::Parse(int argc, char* argv[])
	{
		if (argc < 1 || argc > 2)
		{
			return false;
		}

		switch (argc)
		{
		case 2:
			gamma = std::stof(argv[1]);
		case 1:
			destinationFile = argv[0];
		default:
			break;
		}

		return true;
	}

	void Write::PrintUsage() const
	{
		std::cout << "Usage: Write FILEPATH [GAMMA]" << std::endl;
	}

	bool Write::Execute(cl::Program& program, cl::CommandQueue& queue, const std::shared_ptr<cl::Image> image)
	{
		if (image == nullptr)
		{
			throw std::runtime_error("Write requires an input Image");
		}

		int width = (int)image->getImageInfo<CL_IMAGE_WIDTH>();
		int height = (int)image->getImageInfo<CL_IMAGE_HEIGHT>();
		// TODO: Actually handle image formats rather than guessing
		int components = 4;

		// Read the image back onto the host. Whatever the source image components, the kernel returns a float4 image
		cl::size_t<3> origin;
		cl::size_t<3> region;
		region[0] = width;
		region[1] = height;
		region[2] = 1;
		float *dstData = new float[width * height * components];
		queue.enqueueReadImage(*image, CL_TRUE, origin, region, 0, 0, (void *)dstData);

		// Output the image - fill the alpha unless the source imaged had an alpha to be modified
		// TODO: "fill alpha" shouldn't be an option, should just respect input
		ImageUtil::WriteImage(destinationFile, dstData, width, height, components, gamma, true);
		delete[] dstData;

		return true;
	}

	REGISTER_PLUGIN(Write, Write::Create);
}