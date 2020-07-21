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

	err = program.build("-cl-std=CL1.2");
	if (err == CL_BUILD_PROGRAM_FAILURE)
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
	return program;
}

// Copied from https://www.eriksmistad.no/gaussian-blur-using-opencl-and-the-built-in-images-textures/
float* createBlurMask(float sigma, int * maskRadiusPointer)
{
	int maskRadius = (int)ceil(3.0f*sigma);
	int maskSize = maskRadius * 2 + 1;
	float * mask = new float[maskSize * maskSize];
	float sum = 0.0f;
	for (int a = -maskRadius; a < maskRadius + 1; a++)
	{
		for (int b = -maskRadius; b < maskRadius + 1; b++)
		{
			float temp = exp(-((float)(a*a + b * b) / (2 * sigma*sigma)));
			sum += temp;
			mask[a + maskRadius + (b + maskRadius)*maskSize] = temp;
		}
	}
	// Normalize the mask
	for (int i = 0; i < maskSize*maskSize; i++)
		mask[i] = mask[i] / sum;

	*maskRadiusPointer = maskRadius;

	return mask;
}

struct Settings
{
	char* source;
	char* destination;
};

bool parse(int argc, char* argv[], Settings &settings)
{
	if (argc != 3)
	{
		std::cerr << "Usage: " << argv[0] << " SOURCE DESTINATION" << std::endl;
		return false;
	}
	settings.source = argv[1];
	settings.destination = argv[2];
	return true;
}

int main(int argc, char* argv[])
{
	Settings settings;
	if (!parse(argc, argv, settings))
	{
		return 1;
	}
	// Load the image
	int width, height, components_per_pixel = 4;
	float* data = stbi_loadf(settings.source, &width, &height, &components_per_pixel, components_per_pixel);

	try
	{
		// Start a CL context
		cl::Context context = CreateContext();
		auto devices = context.getInfo<CL_CONTEXT_DEVICES>();
		cl::Device &device = devices.front();

		// Prepare the convolve kernel
		int maskRadius = 0;
		float* convolve = createBlurMask(3.0f, &maskRadius);
		cl::Buffer mask(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, (maskRadius * 2 + 1) * (maskRadius * 2 + 1) * sizeof(float), convolve);

		// Copy the image onto the device
		cl::Image2D src(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, cl::ImageFormat(CL_RGBA, CL_FLOAT), width, height, 0, (void*)data);

		// Create an output image on the device
		cl::Image2D dst(context, CL_MEM_WRITE_ONLY, cl::ImageFormat(CL_RGBA, CL_FLOAT), width, height, 0, nullptr);

		// Prepare the kernel
		cl_int err = 0;
		cl::Program program = BuildProgramFromSource(context, "GaussianBlur.cl", err);
		cl::Kernel kernel(program, "GaussianBlur");
		kernel.setArg<cl::Image>(0, src);
		kernel.setArg<cl::Buffer>(1, mask);
		kernel.setArg<int>(2, maskRadius);
		kernel.setArg<cl::Image>(3, dst);

		// Run the convolve kernel
		cl::CommandQueue queue(context, device, 0);
		queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(width, height));

		// Read the image back onto the host
		float* outData = new float[width * height * components_per_pixel];
		cl::size_t<3> origin;
		// TODO: There's got to be a nicer way to construct this.
		cl::size_t<3> region;
		region[0] = width;
		region[1] = height;
		region[2] = 1;
		queue.enqueueReadImage(dst, CL_TRUE, origin, region, 0, 0, (void*)outData);

		// Output the image
		stbi_write_png(settings.destination, width, height, components_per_pixel, outData, sizeof(float) * components_per_pixel);
		return 0;
	}
	catch (cl::Error error)
	{
		std::cerr << error.what() << '(' << error.err() << ')' << std::endl;
		return 1;
	}
}
