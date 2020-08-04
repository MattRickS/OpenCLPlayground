#pragma once
#include <CL/cl.hpp>
#include "Op.h"
#include <memory>

namespace Op
{
	class Invert : public Operator
	{
	public:
		std::string Kernel() const override;
		std::string Name() const override { return "Invert"; }
		bool Parse(int argc, char* argv[]) override;
		void PrintUsage() const override;
		bool Execute(cl::Program& program, cl::CommandQueue& queue, const std::shared_ptr<cl::Image> image) override;
		static std::shared_ptr<Invert> Create() { return std::make_shared<Invert>(); }
	};
}
