#pragma once
#include "Op.h"

namespace Op
{
	class Read : public Operator
	{
	public:
		float gamma = 1.0f;
		char* sourceFile;

		bool Parse(int argc, char* argv[]) override;
		void PrintUsage() const override;
		bool Execute(cl::Context &context, cl::CommandQueue& queue, const std::shared_ptr<cl::Image> image) override;
		static std::shared_ptr<Read> Create() { return std::make_shared<Read>(); }
	};
}
