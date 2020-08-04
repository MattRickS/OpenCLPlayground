#include <fstream>
#include <iostream>
#include "Op.h"
#include "Invert.h"
#include "CLUtil.h"

namespace Op
{
	std::string Invert::Kernel() const
	{
		std::ifstream sourceFile("Invert.cl");
		return std::string(std::istreambuf_iterator<char>(sourceFile), (std::istreambuf_iterator<char>()));
	}

	bool Invert::Parse(int argc, char* argv[]) { return (argc == 0); }

	void Invert::PrintUsage() const
	{
		std::cout << "Usage: Invert" << std::endl;
	}

	bool Invert::Execute(cl::Program &program, cl::CommandQueue& queue, const std::shared_ptr<cl::Image> image)
	{
		if (image == nullptr)
		{
			throw std::runtime_error("Invert requires an input Image");
		}

		cl::Context context = program.getInfo<CL_PROGRAM_CONTEXT>();

		size_t width = image->getImageInfo<CL_IMAGE_WIDTH>();
		size_t height = image->getImageInfo<CL_IMAGE_HEIGHT>();
		outputImage = std::make_shared<cl::Image2D>(context, CL_MEM_READ_WRITE, cl::ImageFormat(CL_RGBA, CL_FLOAT), width, height, 0, nullptr);

		// Prepare the kernel
		cl::Kernel kernel(program, Name().c_str());
		kernel.setArg<cl::Image>(0, *image);
		kernel.setArg<cl::Image>(1, *outputImage);

		// Run the kernel, wait for it to finish
		queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(width, height));
		cl::finish();

		return true;
	}

	REGISTER_PLUGIN(Invert, Invert::Create);
}