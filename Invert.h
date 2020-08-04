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
		bool Execute(cl::Context &context, cl::CommandQueue& queue, const std::shared_ptr<cl::Image> image) override;
		static std::shared_ptr<Invert> Create() { return std::make_shared<Invert>(); }
	};
}
