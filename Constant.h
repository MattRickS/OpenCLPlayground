#pragma once
#include <CL/cl.h>
#include "Op.h"

namespace Op
{
	class Constant : public Operator
	{
	public:
		int width;
		int height;
		cl_float4 color;

		std::string Kernel() const override { return ""; }
		std::string Name() const override { return "Constant"; }
		bool Parse(int argc, char* argv[]) override;
		void PrintUsage() const override;
		bool Execute(cl::Program& program, cl::CommandQueue& queue, const std::shared_ptr<cl::Image> image) override;
		static std::shared_ptr<Constant> Create() { return std::make_shared<Constant>(); }
	};
}
