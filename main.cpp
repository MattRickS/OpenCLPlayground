#define __CL_ENABLE_EXCEPTIONS
#include <CL/cl.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

const float PI = 3.14159265358979f;

cl::Context CreateContext()
{
	std::vector<cl::Platform> platforms;
	cl::Platform::get(&platforms);

	// Copy operator, is inexpensive
	cl::Platform platform = platforms.front();

	std::vector<cl::Device> devices;
	// TODO: option to pick CPU/GPU
	platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);

	cl::Device device = devices.front();
	cl::Context context(device);

	return context;
}

cl::Program BuildProgramFromSource(cl::Context &context, const std::string &file, cl_int &err)
{
	std::ifstream sourceFile(file);
	std::string src(std::istreambuf_iterator<char>(sourceFile), (std::istreambuf_iterator<char>()));

	cl::Program::Sources sources(1, std::make_pair(src.c_str(), src.length() + 1));
	cl::Program program(context, sources);

	try
	{
		program.build("-cl-std=CL1.2");
	}
	catch (cl::Error error)
	{
		if (error.err() == CL_BUILD_PROGRAM_FAILURE)
		{
			auto devices = context.getInfo<CL_CONTEXT_DEVICES>();
			for (const cl::Device& device : devices)
			{
				if (program.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(device) != CL_BUILD_ERROR) { continue; }
				std::string name = device.getInfo<CL_DEVICE_NAME>();
				std::string log = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device);
				std::cerr << "Build log for " << name << " : " << log << std::endl;
			}
		}
		throw error;
	}
	return program;
}

float* gaussianDistribution(float sigma, int radius)
{
	int maskSize = radius * 2 + 1;
	float* mask = new float[maskSize * maskSize];
	float sum = 0.0f;
	for (int y = -radius; y < radius + 1; y++)
	{
		for (int x = -radius; x < radius + 1; x++)
		{
			float pow = (float)(y * y + x * x) / (2 * sigma * sigma);
			float weight = exp(-pow) * (1.0f / (2 * PI * sigma * sigma));
			mask[y + radius + (x + radius) * maskSize] = weight;
			sum += weight;
		}
	}
	return mask;
}

struct Settings
{
	char* source;
	char* destination;
	float strength = 3.0f;
	int radius = 5;
	float gamma = 1.0f;
	float gammaInv = 1.0f;
};

bool parse(int argc, char* argv[], Settings &settings)
{
	if (argc < 3 || argc > 6)
	{
		std::cerr << "Usage: " << argv[0] << " SOURCE DESTINATION [RADIUS] [STRENGTH] [GAMMA]" << std::endl;
		return false;
	}

	switch (argc)
	{
	case 6:
		settings.gamma = std::stof(argv[5]);
		settings.gammaInv = 1.0f / settings.gamma;
	case 5:
		settings.strength = std::stof(argv[4]);
	case 4:
		settings.radius = std::stoi(argv[3]);
	default:
		settings.source = argv[1];
		settings.destination = argv[2];
	}

	return true;
}

int main(int argc, char* argv[])
{
	Settings settings;
	if (!parse(argc, argv, settings))
	{
		return 1;
	}

	try
	{
		// Start a CL context
		cl::Context context = CreateContext();
		auto devices = context.getInfo<CL_CONTEXT_DEVICES>();
		cl::Device &device = devices.front();

		// Load a convolve kernel on the device
		int maskRadius = 3;
		float* convolve = gaussianDistribution(settings.strength, settings.radius);
		cl::Buffer mask(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, (settings.radius * 2 + 1) * (settings.radius * 2 + 1) * sizeof(float), convolve);
		delete[] convolve;

		// Load the image onto the device - explicitly handle colorspace ourselves instead of stbi_loadf
		int width, height, components, components_per_pixel = 4;
		unsigned char* inData = stbi_load(settings.source, &width, &height, &components, components_per_pixel);
		if (inData == nullptr)
		{
			throw std::runtime_error("Failed to load source image");
		}
		int numElements = width * height * components_per_pixel;
		float* srcData = new float[numElements];
		for (int i = 0; i < numElements; i++)
		{
			srcData[i] = powf(inData[i] / 255.0f, settings.gamma);
		}
		delete[] inData;
		cl::Image2D src(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, cl::ImageFormat(CL_RGBA, CL_FLOAT), width, height, 0, (void*)srcData);
		delete[] srcData;

		// Create an output image on the device
		cl::Image2D dst(context, CL_MEM_WRITE_ONLY, cl::ImageFormat(CL_RGBA, CL_FLOAT), width, height, 0, nullptr);

		// Prepare the kernel
		cl_int err = 0;
		cl::Program program = BuildProgramFromSource(context, "GaussianBlur.cl", err);
		cl::Kernel kernel(program, "GaussianBlur");
		kernel.setArg<cl::Image>(0, src);
		kernel.setArg<cl::Buffer>(1, mask);
		kernel.setArg<int>(2, settings.radius);
		kernel.setArg<cl::Image>(3, dst);

		// Run the convolve kernel, wait for it to finish (TODO: use events)
		cl::CommandQueue queue(context, device, 0);
		queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(width, height));
		cl::finish();

		// Read the image back onto the host. Whatever the source image components, the kernel returns a float4 image
		cl::size_t<3> origin;
		cl::size_t<3> region;
		region[0] = width;
		region[1] = height;
		region[2] = 1;
		float* dstData = new float[numElements];
		queue.enqueueReadImage(dst, CL_TRUE, origin, region, 0, 0, (void*)dstData);

		// Convert float image to unsigned char for use in stb - explictly handle colorspace
		unsigned char* outData = new unsigned char[numElements];
		for (int i = 0; i < numElements; i++)
		{
			// If the source image did not provide an alpha, then output a constant full alpha
			if (components != 4 && i % 4 == 3)
			{
				outData[i] = 255;
			}
			else
			{
				outData[i] = static_cast<unsigned char>(powf(dstData[i], settings.gammaInv) * 255);
			}
		}
		delete[] dstData;

		// Output the image
		stbi_write_png(settings.destination, width, height, components_per_pixel, outData, width * components_per_pixel);
		delete[] outData;

		return 0;
	}
	catch (cl::Error error)
	{
		std::cerr << error.what() << '(' << error.err() << ')' << std::endl;
		return 1;
	}
}
