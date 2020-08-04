#pragma once
#include <CL/cl.hpp>
#include <functional>
#include <unordered_map>
#include <memory>
#include <string>

#define REGISTER_PLUGIN(plugin_name, create_func) \
    bool plugin_name ## _entry = Registry::Register(#plugin_name, (create_func))

namespace Op
{
	class Operator
	{
	public:
		std::shared_ptr<cl::Image> outputImage;

		virtual bool Parse(int argc, char* argv[]) = 0;
		virtual void PrintUsage() const = 0;
		virtual bool Execute(cl::Context &context, cl::CommandQueue& queue, const std::shared_ptr<cl::Image> image) = 0;
	};

	using OpCreator = std::function<std::shared_ptr<Operator>()>;
	using FactoryMap = std::unordered_map<std::string, OpCreator>;

	class Registry
	{
	private:
		FactoryMap operators;

		static FactoryMap& GetMap()
		{
			static FactoryMap map;
			return map;
		}

	public:
		static bool Register(const std::string name, OpCreator op)
		{
			GetMap()[name] = op;
			return true;
		}

		static std::shared_ptr<Operator> Create(const std::string name)
		{
			auto iterator = GetMap().find(name);
			if (iterator != GetMap().end())
			{
				return iterator->second();
			}

			return nullptr;
		}
	};
}
