#pragma once
#include "Op.h"

namespace Op
{
	class Write : public Operator
	{
	public:
		float gamma = 1.0f;
		char* destinationFile;

		std::string Kernel() const override { return ""; };
		std::string Name() const override { return "Write"; }
		bool Parse(int argc, char* argv[]) override;
		void PrintUsage() const override;
		bool Execute(cl::Program& program, cl::CommandQueue& queue, const std::shared_ptr<cl::Image> image) override;
		static std::shared_ptr<Write> Create() { return std::make_shared<Write>(); }
	};
}
#pragma once
