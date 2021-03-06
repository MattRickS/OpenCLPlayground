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

		float* Distribution(float sigma, int radius) const;
		bool Parse(int argc, char* argv[]) override;
		void PrintUsage() const override;
		void Execute(cl::Context &context, cl::CommandQueue& queue, cl::Image &src, cl::Image &dst) override;
		static std::shared_ptr<GaussianBlur> Create()
		{
			return std::make_shared<GaussianBlur>();
		};
	};
}
