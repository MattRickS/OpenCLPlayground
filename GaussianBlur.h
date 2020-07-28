#pragma once
#include <CL/cl.hpp>

#include "Op.h"

#include <string>

class GaussianBlur : public Op::Operator
{
public:
	float strength{ 3.0f };
	int radius{ 5 };

	float* Distribution(float sigma, int radius) const;
	bool Parse(int argc, char* argv[]) override;
	void PrintUsage() const override;
	void Execute(cl::Context &context, cl::CommandQueue& queue, cl::Image &src, cl::Image &dst) override;
};
