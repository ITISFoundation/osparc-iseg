/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef HPMC_H
#define HPMC_H

#define __CL_ENABLE_EXCEPTIONS
#define __USE_GL_INTEROP

#ifdef GPUMC_EXPORTS
# define GPUMC_API __declspec(dllexport)
#else
# define GPUMC_API __declspec(dllimport)
#endif

namespace cl
{
	class BufferGL;
}
class GpuMcPimple;

class GPUMC_API GPUMC
{
public:
	//using namespace cl;
	GPUMC();
	~GPUMC();
	typedef unsigned int uint;
	typedef unsigned char uchar;

	void setupOpenGL(int size, int sizeX, int sizeY, int sizeZ, float spacingX, float spacingY, float spacingZ);
	bool setupOpenCL(unsigned char * voxels, int size);

	void reshape(int width, int height);
	void renderScene(int windowWidth, int windowHeight, int,int,int,float,float,float,float,int );

	int prepareDataset(uchar ** voxels, int sizeX, int sizeY, int sizeZ);

	void updateScalarField();
	void histoPyramidConstruction();
	void histoPyramidTraversal(int sum);
	void setIso(int);

private:
	// origial code
	void setupOpenGL(int * argc, char ** argv, int size, int sizeX, int sizeY, int sizeZ, float spacingX, float spacingY, float spacingZ); 
	
	struct Contour
	{
		Contour();
		~Contour();
		float r,g,b;
		int isolevel;
		int m_Size;
	};
	Contour* contour;
	bool writingTo3DTextures;
	bool extractSurfaceOnEveryFrame;
	bool extractSurface;
	GpuMcPimple* implementation;

	unsigned int VBO_ID;
	float camX, camY, camZ; //X, Y, and Z
	float lastx, lasty, xrot, yrot, xrotrad, yrotrad; //Last pos and rotation
	float speed; //Movement speed
	int totalSum;
	cl::BufferGL *VBOBuffer;
};

#endif
