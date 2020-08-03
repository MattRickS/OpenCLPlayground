#include <iostream>
#include "Op.h"
#include "Invert.h"
#include "CLUtil.h"

namespace Op
{
	bool Invert::Parse(int argc, char* argv[]) { return (argc == 0); }

	void Invert::PrintUsage() const
	{
		std::cout << "Usage: Invert" << std::endl;
	}

	void Invert::Execute(cl::Context &context, cl::CommandQueue& queue, cl::Image &src, cl::Image &dst)
	{
		// Prepare the kernel
		cl_int err = 0;
		cl::Program program = CLUtil::BuildProgramFromSource(context, "Invert.cl", err);
		cl::Kernel kernel(program, "Invert");
		kernel.setArg<cl::Image>(0, src);
		kernel.setArg<cl::Image>(1, dst);

		// Run the kernel, wait for it to finish
		size_t width = src.getImageInfo<CL_IMAGE_WIDTH>();
		size_t height = src.getImageInfo<CL_IMAGE_HEIGHT>();
		queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(width, height));
		cl::finish();
	}

	REGISTER_PLUGIN(Invert, Invert::Create);
}