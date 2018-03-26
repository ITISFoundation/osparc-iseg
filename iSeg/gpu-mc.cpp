/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "GPUMC.h"
#include "gpu-mc.hpp"

#include <sstream>
#include <iostream>
#include <fstream>
#include <utility>
#include <string>
#include <cmath>
#include <algorithm>

#include <GL/glew.h>
#include "OpenCLUtilities/openCLGLUtilities.hpp"

using namespace cl;

#ifdef max
#  undef max
#endif

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

typedef struct {
	int x,y,z;
} Size;

typedef struct {
	float x,y,z;
} Sizef;

class GpuMcPimple{
public:
	~GpuMcPimple();
	Program program;
	CommandQueue queue;
	Context context;
	Image3D rawData;
	Image3D cubeIndexesImage;
	Buffer cubeIndexesBuffer;
	Kernel constructHPLevelKernel;
	Kernel constructHPLevelCharCharKernel;
	Kernel constructHPLevelCharShortKernel;
	Kernel constructHPLevelShortShortKernel;
	Kernel constructHPLevelShortIntKernel;
	Kernel classifyCubesKernel;
	Kernel traverseHPKernel;
	std::vector<Image3D> images;
	std::vector<Buffer> buffers;
	cl::size_t<3> origin; 
	cl::size_t<3> region;
	Sizef scalingFactor;
	Sizef translation;
};
GpuMcPimple::~GpuMcPimple(){
	buffers.clear();
	images.clear();
	buffers.shrink_to_fit();
	images.shrink_to_fit();
			}

GPUMC::Contour::Contour(){}
GPUMC::Contour::~Contour(){}

GPUMC::GPUMC(){
	VBO_ID= 0;
	implementation = new GpuMcPimple;
	contour = new Contour;
	contour->isolevel=1;
	extractSurfaceOnEveryFrame = false;
	extractSurface = true;
	camX=0;
	camY=0;
	camZ = 4.0f; //X, Y, and Z
	speed = 0.1f; //Movement speed
	xrot=0;
	yrot=0;
	yrotrad=0;
	xrotrad=0;
	lastx=0;
	lasty=0;

}

GPUMC::~GPUMC()
{
	delete implementation;
	delete contour;
	delete VBOBuffer;

}

// Some functions missing in windows
inline double log2(double x) {
	return log(x)/log(2.0);
}

inline double round(double d) {
	return floor(d + 0.5);
}

void GPUMC::reshape(int width, int height) {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glViewport(0, 0, width, height);
	gluPerspective(45.0f, (GLfloat)width/(GLfloat)height, 0.5f, 10.0f);
}


void GPUMC::renderScene(int windowWidth, int windowHeight,int xRotw,int yRotw,int zRotw, float zoom, float red, float green, float blue, int op) 
{
	if(extractSurfaceOnEveryFrame || extractSurface) 
	{
		// Delete VBO buffer
		glDeleteBuffers(1, &VBO_ID);

		/*
		// For some reason this doesn't work?
		if(VBOBuffer != NULL)
		delete VBOBuffer;
		*/
		histoPyramidConstruction();

		// Read top of histoPyramid an use this size to allocate VBO below
		int * sum = new int[8];
		if(writingTo3DTextures) {
			implementation->queue.enqueueReadImage(implementation->images[implementation->images.size()-1], CL_FALSE, implementation->origin, implementation->region, 0, 0, sum);
		} else {
			implementation->queue.enqueueReadBuffer(implementation->buffers[implementation->buffers.size()-1], CL_FALSE, 0, sizeof(int)*8, sum);
		}

		implementation->queue.finish();
		totalSum = sum[0] + sum[1] + sum[2] + sum[3] + sum[4] + sum[5] + sum[6] + sum[7] ;

		if(totalSum == 0) {
			std::cout << "No triangles were extracted. Check isovalue." << std::endl;
			return;
		}

		// Create new VBO
		glGenBuffers(1, &VBO_ID);
		glBindBuffer(GL_ARRAY_BUFFER, VBO_ID);
		glBufferData(GL_ARRAY_BUFFER, totalSum*18*sizeof(cl_float), NULL, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// Traverse the histoPyramid and fill VBO
		histoPyramidTraversal(totalSum);
		implementation->queue.flush();
	}

	// Render VBO
	glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT);

	reshape(windowWidth,windowHeight);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glTranslatef(-camX, -camY, -camZ);

	glRotatef(xRotw/16.f,1.f,0.f,0.f);
	glRotatef(yRotw/16.f,0.f, 1.f, 0.f);

	glPushMatrix();
	glColor3f(red/255, green/255, blue/255);
	glScalef(zoom*implementation->scalingFactor.x, zoom*implementation->scalingFactor.y, zoom*implementation->scalingFactor.z);
	glTranslatef(implementation->translation.x, implementation->translation.y, implementation->translation.z);

	glRotatef(zRotw/16.f, 0.0f, 0.0f, 1.0f);
	// Normal Buffer
	glBindBuffer(GL_ARRAY_BUFFER, VBO_ID);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDepthMask(GL_FALSE);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);

	glVertexPointer(3, GL_FLOAT, 24, BUFFER_OFFSET(0));
	glNormalPointer(GL_FLOAT, 24, BUFFER_OFFSET(12));    

	if(extractSurfaceOnEveryFrame || extractSurface) 
		implementation->queue.finish();

	//glWaitSync(traversalSync, 0, GL_TIMEOUT_IGNORED);
	glDrawArrays(GL_TRIANGLES, 0, totalSum*3);

	struct SMaterial
	{
		float diffuse[4];
		float specular[4];
		float ambient[4];
		float shininess;

		SMaterial(float red, float green, float blue, float alpha)
		{
			diffuse[0] = red;
			diffuse[1] = green;
			diffuse[2] = blue;

			specular[0] = red * 0.6f;
			specular[1] = green * 0.6f;
			specular[2] = blue * 0.6f;

			std::fill_n(ambient, 3, 0.1f);
			diffuse[3] = specular[3] = ambient[3] = 1.0f;

			shininess = 6.0f;
		}
	} material(red/255.f, green/255.f, blue/255.f, op/10.f);

	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,   material.diffuse);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  material.specular);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,   material.ambient);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, &material.shininess);

	// Release buffer
	glBindBuffer(GL_ARRAY_BUFFER, 0); 
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);

	glPopMatrix();

	glPopAttrib();

	//BLglutSwapBuffers();
	extractSurface = false;
}


void GPUMC::setupOpenGL(int size, int sizeX, int sizeY, int sizeZ, float spacingX, float spacingY, float spacingZ)
{
	glewInit();

	implementation->origin[0] = 0;
	implementation->origin[1] = 0;
	implementation->origin[2] = 0; 
	implementation->region[0] = 2;
	implementation->region[1] = 2;
	implementation->region[2] = 2;
	implementation->scalingFactor.x = spacingX*1.5f/size;
	implementation->scalingFactor.y = spacingY*1.5f/size;
	implementation->scalingFactor.z = spacingZ*1.5f/size;

	implementation->translation.x = (float)sizeX/2.0f;
	implementation->translation.y = -(float)sizeY/2.0f;
	implementation->translation.z = -(float)sizeZ/2.0f;

	extractSurface = true;
	extractSurfaceOnEveryFrame = false;
}

void GPUMC::setupOpenGL(int * argc, char ** argv, int size, int sizeX, int sizeY, int sizeZ, float spacingX, float spacingY, float spacingZ) {

	// init OpenGL window using QT

	/* Initialize GLUT */
	//BL    glutInit(argc, argv);
	//BL    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	//BL    glutInitWindowPosition(0, 0);
	//BL    //glutInitWindowSize(glutGet(GLUT_SCREEN_WIDTH),glutGet(GLUT_SCREEN_HEIGHT));
	//BL    glutInitWindowSize(800, 800);
	//BL    windowID = glutCreateWindow("GPU Marching Cubes");
	//BL    //glutFullScreen();	
	//BL    glutDisplayFunc(renderScene);
	//BL    glutIdleFunc(idle);
	//BL    glutReshapeFunc(reshape);
	//BL	glutKeyboardFunc(keyboard);
	//BL	glutMotionFunc(mouseMovement);

	glewInit();
	glEnable(GL_NORMALIZE);
	//glEnable(GL_DEPTH_TEST);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHTING);
	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


	implementation->origin[0] = 0;
	implementation->origin[1] = 0;
	implementation->origin[2] = 0; 
	implementation->region[0] = 2;
	implementation->region[1] = 2;
	implementation->region[2] = 2;
	implementation->scalingFactor.x = spacingX*1.5f/size;
	implementation->scalingFactor.y = spacingY*1.5f/size;
	implementation->scalingFactor.z = spacingZ*1.5f/size;

	implementation->translation.x = (float)sizeX/2.0f;
	implementation->translation.y = -(float)sizeY/2.0f;
	implementation->translation.z = -(float)sizeZ/2.0f;

	extractSurface = true;
	extractSurfaceOnEveryFrame = false;
}

int GPUMC::prepareDataset(uchar ** voxels, int sizeX, int sizeY, int sizeZ) {
	// If all equal and power of two exit
	if(sizeX == sizeY && sizeY == sizeZ && sizeX == pow(2, log2(sizeX)))
		return sizeX;

	// Find largest size and find closest power of two
	int largestSize = std::max(sizeX, std::max(sizeY, sizeZ));
	int size = 0;
	int i = 1;
	while(pow(2, i) < largestSize)
		i++;
	size = pow(2, i);

	// Make new voxel array of this size and fill it with zeros
	uchar * newVoxels = new uchar[size*size*size];
	for(int j = 0; j < size*size*size; j++) 
		newVoxels[j] = 0;

	// Fill the voxel array with previous data
	for(int x = 0; x < sizeX; x++) {
		for(int y = 0; y < sizeY; y++) {
			for(int z = 0; z <sizeZ; z++) {
				newVoxels[x + y*size + z*size*size] = voxels[0][x + y*sizeX + z*sizeX*sizeY];
			}
		}
	}
	delete[] voxels[0];
	voxels[0] = newVoxels;
	return size;
}



template <class T>
inline std::string to_string(const T& t) {
	std::stringstream ss;
	ss << t;
	return ss.str();
}

bool GPUMC::setupOpenCL(uchar * voxels, int size) {
	contour->m_Size = size; 
	try { 
		// Create a context that use a GPU and OpenGL interop.
		implementation->context = createCLGLContext(CL_DEVICE_TYPE_GPU, VENDOR_ANY);

		// Get a list of devices on this platform
		std::vector<Device> devices = implementation->context.getInfo<CL_CONTEXT_DEVICES>();

		// Create a command queue and use the first device
		implementation->queue = CommandQueue(implementation->context, devices[0]);

		// Check if writing to 3D textures are supported
		std::string sourceFilename;
		if((int)devices[0].getInfo<CL_DEVICE_EXTENSIONS>().find("cl_khr_3d_image_writes") > -1) {
			writingTo3DTextures = true;
			sourceFilename = "gpu-mc.cl";
		} else {
			std::cout << "Writing to 3D textures is not supported on this device. Writing to regular buffers instead." << std::endl;
			std::cout << "Note that this is a bit slower." << std::endl;
			writingTo3DTextures = false;
			sourceFilename = "gpu-mc-morton.cl";
		}

		// Read source file
		std::ifstream sourceFile(sourceFilename.c_str());
		if(sourceFile.fail()) {
			std::cout << "Failed to open OpenCL source file" << std::endl;
			exit(-1);
		}
		std::string sourceCode(
			std::istreambuf_iterator<char>(sourceFile),
			(std::istreambuf_iterator<char>()));

		// Insert size
		Program::Sources source(1, std::make_pair(sourceCode.c_str(), sourceCode.length()+1));

		// Make program of the source code in the context
		implementation->program = Program(implementation->context, source);

		// Build program for these specific devices
		try{
			std::string buildOptions = "-DSIZE=" + to_string(contour->m_Size);
			implementation->program.build(devices, buildOptions.c_str());
		} catch(Error error) {
			if(error.err() == CL_BUILD_PROGRAM_FAILURE) {
				std::cout << "Build log:\t" << implementation->program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[0]) << std::endl;
			}   
			throw error;
		} 

		if(writingTo3DTextures) {
			// Create images for the HistogramPyramid
			int bufferSize = contour->m_Size;
			// Make the two first buffers use INT8
			implementation->images.push_back(Image3D(implementation->context, CL_MEM_READ_WRITE, ImageFormat(CL_RGBA, CL_UNSIGNED_INT8), bufferSize, bufferSize, bufferSize));
			bufferSize /= 2;
			implementation->images.push_back(Image3D(implementation->context, CL_MEM_READ_WRITE, ImageFormat(CL_R, CL_UNSIGNED_INT8), bufferSize, bufferSize, bufferSize));
			bufferSize /= 2;
			// And the third, fourth and fifth INT16
			implementation->images.push_back(Image3D(implementation->context, CL_MEM_READ_WRITE, ImageFormat(CL_R, CL_UNSIGNED_INT16), bufferSize, bufferSize, bufferSize));
			bufferSize /= 2;
			implementation->images.push_back(Image3D(implementation->context, CL_MEM_READ_WRITE, ImageFormat(CL_R, CL_UNSIGNED_INT16), bufferSize, bufferSize, bufferSize));
			bufferSize /= 2;
			implementation->images.push_back(Image3D(implementation->context, CL_MEM_READ_WRITE, ImageFormat(CL_R, CL_UNSIGNED_INT16), bufferSize, bufferSize, bufferSize));
			bufferSize /= 2;
			// The rest will use INT32
			for(int i = 5; i < (log2((float)contour->m_Size)); i ++) {
				if(bufferSize == 1)
					bufferSize = 2; // Image cant be 1x1x1
				implementation->images.push_back(Image3D(implementation->context, CL_MEM_READ_WRITE, ImageFormat(CL_R, CL_UNSIGNED_INT32), bufferSize, bufferSize, bufferSize));
				bufferSize /= 2;
			}

			// If writing to 3D textures is not supported we to create buffers to write to 
		} else {
			int bufferSize = contour->m_Size*contour->m_Size*contour->m_Size;
			implementation->buffers.push_back(Buffer(implementation->context, CL_MEM_READ_WRITE, sizeof(char)*bufferSize));
			bufferSize /= 8;
			implementation->buffers.push_back(Buffer(implementation->context, CL_MEM_READ_WRITE, sizeof(char)*bufferSize));
			bufferSize /= 8;
			implementation->buffers.push_back(Buffer(implementation->context, CL_MEM_READ_WRITE, sizeof(short)*bufferSize));
			bufferSize /= 8;
			implementation->buffers.push_back(Buffer(implementation->context, CL_MEM_READ_WRITE, sizeof(short)*bufferSize));
			bufferSize /= 8;
			implementation->buffers.push_back(Buffer(implementation->context, CL_MEM_READ_WRITE, sizeof(short)*bufferSize));
			bufferSize /= 8;
			for(int i = 5; i < log2((float)contour->m_Size); i ++) {
				implementation->buffers.push_back(Buffer(implementation->context, CL_MEM_READ_WRITE, sizeof(int)*bufferSize));
				bufferSize /= 8;
			}

			implementation->cubeIndexesBuffer = Buffer(implementation->context, CL_MEM_WRITE_ONLY, sizeof(char)*contour->m_Size*contour->m_Size*contour->m_Size);
			implementation->cubeIndexesImage = Image3D(implementation->context, CL_MEM_READ_ONLY, //CL_MEM_READ_ONLY
				ImageFormat(CL_R, CL_UNSIGNED_INT8),
				contour->m_Size, contour->m_Size, contour->m_Size);
		}

		// Transfer dataset to device
		implementation->rawData = Image3D(
			implementation->context, 
			CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, //CL_MEM_READ_ONLY,//
			ImageFormat(CL_R, CL_UNSIGNED_INT8), 
			contour->m_Size, contour->m_Size,contour-> m_Size,
			0, 0, voxels
			);
		delete[] voxels;

		// Make kernels
		implementation->constructHPLevelKernel = Kernel(implementation->program, "constructHPLevel");
		implementation->classifyCubesKernel = Kernel(implementation->program, "classifyCubes");
		implementation->traverseHPKernel = Kernel(implementation->program, "traverseHP");

		if(!writingTo3DTextures) {
			implementation->constructHPLevelCharCharKernel = Kernel(implementation->program, "constructHPLevelCharChar");
			implementation->constructHPLevelCharShortKernel = Kernel(implementation->program, "constructHPLevelCharShort");
			implementation->constructHPLevelShortShortKernel = Kernel(implementation->program, "constructHPLevelShortShort");
			implementation->constructHPLevelShortIntKernel = Kernel(implementation->program, "constructHPLevelShortInt");
		}

	} catch(Error error) {
		std::cout << error.what() << "(" << error.err() << ")" << std::endl;
		std::cout << getCLErrorString(error.err()) << std::endl;
		return false;
	}
	return true;
}



void GPUMC::histoPyramidConstruction() 
{
	updateScalarField();

	if(writingTo3DTextures) {
		// Run base to first level
		implementation->constructHPLevelKernel.setArg(0, implementation->images[0]);
		implementation->constructHPLevelKernel.setArg(1,implementation-> images[1]);

		implementation->queue.enqueueNDRangeKernel(
			implementation->constructHPLevelKernel, 
			NullRange, 
			NDRange(contour->m_Size/2, contour->m_Size/2, contour->m_Size/2), 
			NullRange
			);

		int previous = contour->m_Size / 2;
		// Run level 2 to top level
		for(int i = 1; i < log2((float)contour->m_Size)-1; i++) {
			implementation->constructHPLevelKernel.setArg(0, implementation->images[i]);
			implementation->constructHPLevelKernel.setArg(1, implementation->images[i+1]);
			previous /= 2;
			implementation->queue.enqueueNDRangeKernel(
				implementation->constructHPLevelKernel, 
				NullRange, 
				NDRange(previous, previous, previous), 
				NullRange
				);
		}
	} else {

		// Run base to first level
		implementation->constructHPLevelCharCharKernel.setArg(0, implementation->buffers[0]);
		implementation->constructHPLevelCharCharKernel.setArg(1, implementation->buffers[1]);

		implementation->queue.enqueueNDRangeKernel(
			implementation->constructHPLevelCharCharKernel, 
			NullRange, 
			NDRange(contour->m_Size/2, contour->m_Size/2, contour->m_Size/2), 
			NullRange
			);

		int previous = contour->m_Size / 2;

		implementation->constructHPLevelCharShortKernel.setArg(0, implementation->buffers[1]);
		implementation->constructHPLevelCharShortKernel.setArg(1, implementation->buffers[2]);

		implementation->queue.enqueueNDRangeKernel(
			implementation->constructHPLevelCharShortKernel, 
			NullRange, 
			NDRange(previous/2, previous/2, previous/2), 
			NullRange
			);

		previous /= 2;

		implementation->constructHPLevelShortShortKernel.setArg(0, implementation->buffers[2]);
		implementation->constructHPLevelShortShortKernel.setArg(1, implementation->buffers[3]);

		implementation->queue.enqueueNDRangeKernel(
			implementation->constructHPLevelShortShortKernel, 
			NullRange, 
			NDRange(previous/2, previous/2, previous/2), 
			NullRange
			);

		previous /= 2;

		implementation->constructHPLevelShortShortKernel.setArg(0, implementation->buffers[3]);
		implementation->constructHPLevelShortShortKernel.setArg(1, implementation->buffers[4]);

		implementation->queue.enqueueNDRangeKernel(
			implementation->constructHPLevelShortShortKernel, 
			NullRange, 
			NDRange(previous/2, previous/2, previous/2), 
			NullRange
			);

		previous /= 2;

		implementation->constructHPLevelShortIntKernel.setArg(0, implementation->buffers[4]);
		implementation->constructHPLevelShortIntKernel.setArg(1, implementation->buffers[5]);

		implementation->queue.enqueueNDRangeKernel(
			implementation->constructHPLevelShortIntKernel, 
			NullRange, 
			NDRange(previous/2, previous/2, previous/2), 
			NullRange
			);

		previous /= 2;

		// Run level 2 to top level
		for(int i = 5; i <  log2(contour->m_Size)-1; i++) {
			implementation->constructHPLevelKernel.setArg(0, implementation->buffers[i]);
			implementation->constructHPLevelKernel.setArg(1, implementation->buffers[i+1]);
			previous /= 2;
			implementation->queue.enqueueNDRangeKernel(
				implementation->constructHPLevelKernel, 
				NullRange, 
				NDRange(previous, previous, previous), 
				NullRange
				);
		}
	}
}

void GPUMC::updateScalarField() {
	if(writingTo3DTextures) {
		implementation->classifyCubesKernel.setArg(0, implementation->images[0]);
		implementation->classifyCubesKernel.setArg(1, implementation->rawData);
		implementation->classifyCubesKernel.setArg(2, contour->isolevel);
		implementation->queue.enqueueNDRangeKernel(
			implementation->classifyCubesKernel, 
			NullRange, 
			NDRange(contour->m_Size,contour-> m_Size, contour->m_Size),
			NullRange
			);
	} else {
		implementation->classifyCubesKernel.setArg(0, implementation->buffers[0]);
		implementation->classifyCubesKernel.setArg(1, implementation->cubeIndexesBuffer);
		implementation->classifyCubesKernel.setArg(2, implementation->rawData);
		implementation->classifyCubesKernel.setArg(3, contour->isolevel);
		implementation->queue.enqueueNDRangeKernel(
			implementation->classifyCubesKernel, 
			NullRange, 
			NDRange(contour->m_Size,contour-> m_Size, contour->m_Size),
			NullRange
			);

		cl::size_t<3> offset;
		offset[0] = 0;
		offset[1] = 0;
		offset[2] = 0;
		cl::size_t<3> region;
		region[0] = contour->m_Size;
		region[1] = contour->m_Size;
		region[2] = contour->m_Size;

		// Copy buffer to image
		implementation->queue.enqueueCopyBufferToImage(implementation->cubeIndexesBuffer, implementation->cubeIndexesImage, 0, offset, region);
	}
}

void GPUMC::histoPyramidTraversal(int sum)
{
	// Make OpenCL buffer from OpenGL buffer
	unsigned int i = 0;
	if(writingTo3DTextures) {
		for(i = 0; i < implementation->images.size(); i++) {
			implementation->traverseHPKernel.setArg(i, implementation->images[i]);
		}
	} else {
		implementation->traverseHPKernel.setArg(0, implementation->rawData);
		implementation->traverseHPKernel.setArg(1, implementation->cubeIndexesImage);
		for(i = 0; i < implementation->buffers.size(); i++) {
			implementation->traverseHPKernel.setArg(i+2, implementation->buffers[i]);
		}
		i += 2;
	}

	VBOBuffer = new BufferGL(implementation->context, CL_MEM_WRITE_ONLY, VBO_ID);
	implementation->traverseHPKernel.setArg(i, *VBOBuffer);
	implementation->traverseHPKernel.setArg(i+1, contour->isolevel);
	implementation->traverseHPKernel.setArg(i+2, sum);
	//cl_event syncEvent = clCreateEventFromGLsyncKHR((cl_context)context(), (cl_GLsync)glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0), 0);
	//glFinish();
	std::vector<Memory> v;
	v.push_back(*VBOBuffer);
	//std::vector<Event> events;
	//Event e;
	//events.push_back(Event(syncEvent));
	implementation->queue.enqueueAcquireGLObjects(&v);

	// Increase the global_work_size so that it is divideable by 64
	int global_work_size = sum + 64 - (sum - 64*(sum / 64));
	// Run a NDRange kernel over this buffer which traverses back to the base level
	implementation->queue.enqueueNDRangeKernel(implementation->traverseHPKernel, NullRange, NDRange(global_work_size), NDRange(64));

	Event traversalEvent;
	implementation->queue.enqueueReleaseGLObjects(&v, 0, &traversalEvent);
	//	traversalSync = glCreateSyncFromCLeventARB((cl_context)context(), (cl_event)traversalEvent(), 0); // Need the GL_ARB_cl_event extension
	implementation->queue.flush();
}
void GPUMC::setIso(int iso)
{
	contour->isolevel=iso;
}
