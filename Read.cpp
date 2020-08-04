#include <iostream>
#include "Op.h"
#include "Read.h"
#include "CLUtil.h"
#include "ImageUtil.h"

namespace Op
{
	bool Read::Parse(int argc, char* argv[])
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
			sourceFile = argv[0];
		default:
			break;
		}

		return true;
	}

	void Read::PrintUsage() const
	{
		std::cout << "Usage: Read FILEPATH [GAMMA]" << std::endl;
	}

	bool Read::Execute(cl::Program& program, cl::CommandQueue& queue, const std::shared_ptr<cl::Image> image)
	{
		if (image != nullptr)
		{
			throw std::runtime_error("Read does not take an input Image");
		}

		cl::Context context = program.getInfo<CL_PROGRAM_CONTEXT>();

		// Load the image onto the device - explicitly handle colorspace ourselves instead of stbi_loadf
		int width, height, src_components, components = 4;
		float *srcData = ImageUtil::ReadImage(sourceFile, gamma, &width, &height, &src_components, components);
		outputImage = std::make_shared<cl::Image2D>(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, cl::ImageFormat(CL_RGBA, CL_FLOAT), width, height, 0, (void *)srcData);
		delete[] srcData;

		return true;
	}

	REGISTER_PLUGIN(Read, Read::Create);
}