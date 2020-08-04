#define __CL_ENABLE_EXCEPTIONS
#include <CL/cl.hpp>

#include "GaussianBlur.h"
#include "CLUtil.h"
#include "ImageUtil.h"
#include "Op.h"

#include <algorithm>
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
		std::cerr << "Usage: " << argv[0] << " KERNEL [SETTINGS] [-- KERNEL [SETTINGS] ...]" << std::endl;
		return 1;
	}

	// Parse all operators and their settings
	OpList operators = ParseOperators(argc, argv, 1);
	if (operators.empty())
	{
		return 1;
	}

	// Gather kernels for program
	std::vector<std::string> kernels;
	for (const auto& op : operators)
	{
		std::string s = op->Kernel();
		if (s != "")
		{
			kernels.emplace_back(s);
		}
	}

	cl::Program::Sources sources;
	for (const auto& s : kernels)
	{
		sources.push_back(std::make_pair(s.c_str(), s.length() + 1));
	}


	try
	{
		// Start a CL context
		cl::Context context = CLUtil::CreateContext();
		cl::Program program = CLUtil::BuildProgramFromSources(context, sources);
		auto devices = context.getInfo<CL_CONTEXT_DEVICES>();
		cl::Device &device = devices.front();

		// Execute the op queue -- TODO: Operators should actually add to the queue and the whole queue executed once with events for waiting
		cl::CommandQueue queue(context, device);
		std::shared_ptr<cl::Image> current(nullptr);
		for (const auto& op : operators)
		{
			if (!op->Execute(program, queue, current))
			{
				throw std::runtime_error("Operator '" + op->Name() + "' failed to execute");
			}
			current = op->outputImage;
		}

		return 0;
	}
	catch (cl::Error error)
	{
		std::cerr << error.what() << '(' << error.err() << ')' << std::endl;
		return 1;
	}
}
