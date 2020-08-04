#pragma once
#include <CL/cl.hpp>

#include "Op.h"

#include <memory>
#include <string>

namespace Op
{
	class GaussianBlur : public Operator
	{
	public:
		float strength{ 3.0f };
		int radius{ 5 };

		std::string Kernel() const override;
		std::string Name() const override { return "GaussianBlur"; }
		float* Distribution(float sigma, int radius) const;
		bool Parse(int argc, char* argv[]) override;
		void PrintUsage() const override;
		bool Execute(cl::Program& program, cl::CommandQueue& queue, const std::shared_ptr<cl::Image> image) override;
		static std::shared_ptr<GaussianBlur> Create()
		{
			return std::make_shared<GaussianBlur>();
		};
	};
}
