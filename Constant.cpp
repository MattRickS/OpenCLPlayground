#include "Constant.h"
#include <iostream>


namespace Op
{
	bool Constant::Parse(int argc, char* argv[])
	{
		if (argc != 6)
		{
			return false;
		}

		width = std::stoi(argv[0]);
		height = std::stoi(argv[1]);

		float col[4];
		for (int i = 0; i < 4; i++)
		{
			col[i] = std::stof(argv[i + 2]);
		}

		color.x = col[0];
		color.y = col[1];
		color.z = col[2];
		color.w = col[3];

		return true;
	}

	void Constant::PrintUsage() const
	{
		std::cout << "Usage: Constant WIDTH HEIGHT RED GREEN BLUE ALPHA" << std::endl;
	}

	bool Constant::Execute(cl::Program &program, cl::CommandQueue& queue, const std::shared_ptr<cl::Image> image)
	{
		if (image != nullptr)
		{
			throw std::runtime_error("Constant does not take an input Image");
		}

		cl::Context context = program.getInfo<CL_PROGRAM_CONTEXT>();
		outputImage = std::make_shared<cl::Image2D>(context, CL_MEM_READ_WRITE, cl::ImageFormat(CL_RGBA, CL_FLOAT), width, height, 0, nullptr);

		// Run the kernel, wait for it to finish
		cl::size_t<3> origin;
		cl::size_t<3> region;
		region[0] = width;
		region[1] = height;
		region[2] = 1;
		queue.enqueueFillImage(*outputImage, color, origin, region);
		cl::finish();

		return true;
	}

	REGISTER_PLUGIN(Constant, Constant::Create);
}