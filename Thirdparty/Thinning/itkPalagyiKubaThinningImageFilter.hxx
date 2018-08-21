#pragma once

// implement thinning templates given by
// Palagyi and Kuba in "A Parallel 3D 12-Subiteration Thinning Algorithm", 1999
// http://www.caip.rutgers.edu/~cornea/CurveSkelApp/
#include "thinning.h"

namespace itk {

inline void CopyNeighborhoodInBuffer(unsigned char *vol, int L, int M, int N, int idx, unsigned char nb[3][3][3], const int volNeighbors[27], bool changeValues = true)
{
	int nidx;
	short i, j, k, ii;
	
	ii = 0;
	for(k = 0; k < 3; k++) 
	{
		for(j = 0; j < 3; j++) 
		{
			for(i = 0; i < 3; i++)
			{
				nidx = idx + volNeighbors[ii];
			
				// \todo BL I can cast the input to '0' and OBJECT, so don't need to have any if/else here
				if(!changeValues)
				{
					nb[i][j][k] = vol[nidx];
				} 
				else 
				{
					if(vol[nidx] != 0)
					{
						nb[i][j][k] = OBJECT;
					}
					else
					{
						nb[i][j][k] = 0;
					}
				}
			
				ii++;
			}
		}
	}
}

template<class TInputImage>
PalagyiKubaThinningImageFilter<TInputImage>::PalagyiKubaThinningImageFilter()
{
	this->SetNumberOfRequiredOutputs(1);

	auto thinImage = OutputImage::New();
	this->SetNthOutput(0, thinImage.GetPointer());
}

template<class TInputImage>
void PalagyiKubaThinningImageFilter<TInputImage>::GenerateData()
{
	auto inputImage = dynamic_cast<const TInputImage*>(ProcessObject::GetInput(0));
	auto thinImage = dynamic_cast<OutputImage*>(this->GetOutput(0));

	//thinImage->SetRegions(region);
	//thinImage->Allocate();
	//thinImage->FillBuffer(0);
	//thinImage->SetSpacing(inputImage->GetSpacing());
	//thinImage->SetOrientation(inputImage->GetOrientation());

	int volNeighbors[27] = {};
	
	// \todo BL cast input to output
	unsigned char* vol = nullptr;
	
	// \todo BL use 64bit integer type where necessary

	// run thinning
	long lsize;
	int nrDel;
	long idx;
	char dir;
	unsigned char nb[3][3][3];
	unsigned char USn[3][3][3];
	int i, j, k;
	int maxnsp;
	int sz, slsz;
	int L, M, N;
	int *dim = image->GetDimensions();
	L = dim[0];
	M = dim[1];
	N = dim[2];

	sz = L * M * N;
	slsz = L * M;

	// initialize global neighbors array
	// lower plane
	volNeighbors[0] = (-slsz - L - 1);
	volNeighbors[1] = (-slsz - L + 0);
	volNeighbors[2] = (-slsz - L + 1);
	volNeighbors[3] = (-slsz + 0 - 1);
	volNeighbors[4] = (-slsz + 0 + 0);
	volNeighbors[5] = (-slsz + 0 + 1);
	volNeighbors[6] = (-slsz + L - 1);
	volNeighbors[7] = (-slsz + L + 0);
	volNeighbors[8] = (-slsz + L + 1);
	// same plane
	volNeighbors[9] = (+0 - L - 1);
	volNeighbors[10] = (+0 - L + 0);
	volNeighbors[11] = (+0 - L + 1);
	volNeighbors[12] = (+0 + 0 - 1);
	volNeighbors[13] = (+0 + 0 + 0);
	volNeighbors[14] = (+0 + 0 + 1);
	volNeighbors[15] = (+0 + L - 1);
	volNeighbors[16] = (+0 + L + 0);
	volNeighbors[17] = (+0 + L + 1);
	// upper plane
	volNeighbors[18] = (+slsz - L - 1);
	volNeighbors[19] = (+slsz - L + 0);
	volNeighbors[20] = (+slsz - L + 1);
	volNeighbors[21] = (+slsz + 0 - 1);
	volNeighbors[22] = (+slsz + 0 + 0);
	volNeighbors[23] = (+slsz + 0 + 1);
	volNeighbors[24] = (+slsz + L - 1);
	volNeighbors[25] = (+slsz + L + 0);
	volNeighbors[26] = (+slsz + L + 1);

	assert(image->GetScalarType() == VTK_UNSIGNED_CHAR);
	vol = (unsigned char *)image->GetScalarPointer();
	lsize = sz;

	//Threshold8bit(thres, vol, lsize);


	// set all object voxels to OBJECT
	int nFG = 0, nBG = 0;
	for (idx = 0; idx < lsize; idx++) {
		if (vol[idx] == 01) {
			vol[idx] = OBJECT;
			nFG++;
		}
		else {
			nBG++;
		}
	}

	nrDel = 1;
	maxnsp = 0;

	while (nrDel > 0) 
	{
		nrDel = 0;
		for (dir = 0; dir < 12; dir++) {

			switch (dir) {
			case 0: // UP_SOUTH = 0,
					// UP
				markBoundaryInDirection(vol, L, M, N, 12);
				// SOUTH
				markBoundaryInDirection(vol, L, M, N, 17);
				break;
			case 1: // NORT_EAST,
					// NOTH
				markBoundaryInDirection(vol, L, M, N, 16);
				// EAST
				markBoundaryInDirection(vol, L, M, N, 14);
				break;
			case 2: // DOWN_WEST,
					// DOWN
				markBoundaryInDirection(vol, L, M, N, 13);
				// WEST
				markBoundaryInDirection(vol, L, M, N, 15);
				break;
			case 3: //  SOUTH_EAST,
					// SOUTH
				markBoundaryInDirection(vol, L, M, N, 17);
				// EAST
				markBoundaryInDirection(vol, L, M, N, 14);
				break;
			case 4: // UP_WEST,
					// UP
				markBoundaryInDirection(vol, L, M, N, 12);
				// WEST
				markBoundaryInDirection(vol, L, M, N, 15);
				break;
			case 5: // DOWN_NORTH,
					// DOWN
				markBoundaryInDirection(vol, L, M, N, 13);
				// NORTH
				markBoundaryInDirection(vol, L, M, N, 16);
				break;
			case 6: // SOUTH_WEST,
					// SOUTH
				markBoundaryInDirection(vol, L, M, N, 17);
				// WEST
				markBoundaryInDirection(vol, L, M, N, 15);
				break;
			case 7: // UP_NORTH,
					// UP
				markBoundaryInDirection(vol, L, M, N, 12);
				// NORTH
				markBoundaryInDirection(vol, L, M, N, 16);
				break;
			case 8: // DOWN_EAST,
					// DOWN
				markBoundaryInDirection(vol, L, M, N, 13);
				// EAST
				markBoundaryInDirection(vol, L, M, N, 14);
				break;
			case 9: //  NORT_WEST,
					// NORTH
				markBoundaryInDirection(vol, L, M, N, 16);
				// WEST
				markBoundaryInDirection(vol, L, M, N, 15);
				break;
			case 10: // UP_EAST,
					 // UP
				markBoundaryInDirection(vol, L, M, N, 12);
				// EAST
				markBoundaryInDirection(vol, L, M, N, 14);
				break;
			case 11: // DOWN_SOUTH,
					 // DOWN
				markBoundaryInDirection(vol, L, M, N, 13);
				// SOUTH
				markBoundaryInDirection(vol, L, M, N, 17);
				break;
			}

			// check each boundary point and remove it if it matches a template
			for (k = 1; k < (N - 1); k++) {
				for (j = 1; j < (M - 1); j++) {
					for (i = 1; i < (L - 1); i++) {
						idx = k * slsz + j * L + i;

						if (vol[idx] == D_BORDER) {
							// copy neighborhood into buffer
							CopyNeighborhoodInBuffer(vol, L, M, N, idx, nb, volNeighbors);

							TransformNeighborhood(nb, dir, USn);

							if (MatchesATemplate(USn)) {
								// delete the point
								// can be removed
								vol[idx] = SIMPLE;
								nrDel++;
							}
						}
					}
				}
			}

			// reset all object voxels to OBJECT
			for (idx = 0; idx < lsize; idx++) {
				// delete simple points
				if (vol[idx] == SIMPLE)
					vol[idx] = 0;
				if (vol[idx] != 0)
					vol[idx] = OBJECT;
			}
		}
	}

}

} // namespace itk
