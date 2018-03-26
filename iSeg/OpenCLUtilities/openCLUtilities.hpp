#ifndef OPENCL_UTILITIES_H
#define OPENCL_UTILITIES_H

#define __CL_ENABLE_EXCEPTIONS


#if defined(__APPLE__) || defined(__MACOSX)
    #include <OpenCLpp/cl.hpp>
#else
    #include <CL/cl.hpp>
#endif


#include <string>
#include <iostream>
#include <fstream>


enum cl_vendor {
    VENDOR_ANY,
    VENDOR_NVIDIA,
    VENDOR_AMD,
    VENDOR_INTEL
};

cl::Context createCLContextFromArguments(int argc, char ** argv);

cl::Context createCLContext(cl_device_type type = CL_DEVICE_TYPE_ALL, cl_vendor vendor = VENDOR_ANY);

cl::Platform getPlatform(cl_device_type = CL_DEVICE_TYPE_ALL, cl_vendor vendor = VENDOR_ANY); 

cl::Program buildProgramFromSource(cl::Context context, std::string filename);

char *getCLErrorString(cl_int err);

#endif
