#pragma once
#include <CL/cl.hpp>
#include "Op.h"
#include <memory>

namespace Op
{
	class Invert : public Operator
	{
	public:
		bool Parse(int argc, char* argv[]) override;
		void PrintUsage() const override;
		void Execute(cl::Context &context, cl::CommandQueue& queue, cl::Image &src, cl::Image &dst) override;
		static std::shared_ptr<Invert> Create() { return std::make_shared<Invert>(); }
	};
}
