#pragma once
#include <CL/cl.hpp>

#include <string>

namespace CLUtil
{
	cl::Context CreateContext();
	cl::Program BuildProgramFromSource(cl::Context &context, const std::string &file, cl_int &err);
};

