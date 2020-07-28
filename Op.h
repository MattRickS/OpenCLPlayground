#pragma once
#include <CL/cl.hpp>
#include <string>

namespace Op
{
	class Operator
	{
	public:
		static const std::string name;

		virtual bool Parse(int argc, char* argv[]) = 0;
		virtual void PrintUsage() const = 0;
		virtual void Execute(cl::Context &context, cl::CommandQueue& queue, cl::Image &src, cl::Image &dst) = 0;
	};
}
