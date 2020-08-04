#define __CL_ENABLE_EXCEPTIONS
#include <CL/cl.hpp>

#include "CLUtil.h"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>


cl::Context CLUtil::CreateContext()
{
	std::vector<cl::Platform> platforms;
	cl::Platform::get(&platforms);

	// Copy operator, is inexpensive
	cl::Platform platform = platforms.front();

	std::vector<cl::Device> devices;
	// TODO: option to pick CPU/GPU
	platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);

	cl::Device device = devices.front();
	cl::Context context(device);

	return context;
}

cl::Program CLUtil::BuildProgramFromSource(cl::Context &context, const std::string &file)
{
	std::ifstream sourceFile(file);
	std::string src(std::istreambuf_iterator<char>(sourceFile), (std::istreambuf_iterator<char>()));

	cl::Program::Sources sources(1, std::make_pair(src.c_str(), src.length() + 1));

	return BuildProgramFromSources(context, sources);
}


cl::Program CLUtil::BuildProgramFromSources(cl::Context &context, cl::Program::Sources sources)
{

	cl::Program program(context, sources);

	try
	{
		program.build("-cl-std=CL1.2");
	}
	catch (cl::Error error)
	{
		if (error.err() == CL_BUILD_PROGRAM_FAILURE)
		{
			auto devices = context.getInfo<CL_CONTEXT_DEVICES>();
			for (const cl::Device& device : devices)
			{
				if (program.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(device) != CL_BUILD_ERROR) { continue; }
				std::string name = device.getInfo<CL_DEVICE_NAME>();
				std::string log = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device);
				std::cerr << "Build log for " << name << " : " << log << std::endl;
			}
		}
		throw error;
	}
	return program;
}