#pragma once

// implement thinning templates given by
// Palagyi and Kuba in "A Parallel 3D 12-Subiteration Thinning Algorithm", 1999
// http://www.caip.rutgers.edu/~cornea/CurveSkelApp/
#include "thinning.h"

namespace itk {

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
	using namespace pk;

	auto inputImage = dynamic_cast<const TInputImage*>(ProcessObject::GetInput(0));
	auto thinImage = dynamic_cast<OutputImage*>(this->GetOutput(0));

	// allocate output
	auto region = thinImage->GetRequestedRegion();
	thinImage->SetBufferedRegion(region);
	thinImage->CopyInformation(inputImage);
	thinImage->Allocate();

	// image dimensions
	using index_t = long long;
	index_t L = static_cast<index_t>(region.GetSize(0));
	index_t M = static_cast<index_t>(region.GetSize(1));
	index_t N = static_cast<index_t>(region.GetSize(2));
	index_t slsz = L * M;
	index_t sz = slsz * N;
	index_t volNeighbors[27] = {};

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

	// neighbor index in 18 directions (only last 6 used)
	index_t neighborOffset[18] = {
		+slsz - L,  // UP_SOUTH,   0
		+L + 1,     // NORT_EAST   1
		-slsz - 1,  // DOWN_WEST,  2 

		-L + 1,     // SOUTH_EAST  3
		+slsz - 1,  // UP_WEST,    4
		-slsz + L,  // DOWN_NORTH, 5 

		-L - 1,     // SOUTH_WEST  6
		+slsz + L,  // UP_NORTH,   7
		-slsz + 1,  // DOWN_EAST,  8

		+L - 1,     // NORT_WEST   9
		+slsz + 1,  // UP_EAST,   10
		-slsz - L   // DOWN_SOUTH 11

		+ slsz,     // UP         12
		-slsz,      // DOWN       13
		+1,         // EAST,      14
		-1,         // WEST,      15
		+L,         // NORTH,     16
		-L,         // SOUTH,     17
	};

	// set all object voxels to OBJECT
	itk::ImageRegionConstIterator<TInputImage> iit(inputImage, region);
	itk::ImageRegionIterator<OutputImage> oit(thinImage, region);
	for (iit.GoToBegin(), oit.GoToBegin(); !iit.IsAtEnd(); ++iit, ++oit)
	{
		bool change = iit.Get() != 0;
		oit.Set(iit.Get() != 0 ? SpecialValues::OBJECT : 0);
	}

	// pointer to output image
	unsigned char* vol = thinImage->GetPixelContainer()->GetImportPointer();

	// run thinning
	char dir;
	unsigned char nb[3][3][3];
	unsigned char USn[3][3][3];
	index_t idx;
	index_t i, j, k;
	index_t nrDel = 1;

	while (nrDel > 0) 
	{
		nrDel = 0;
		for (dir = 0; dir < 12; dir++) 
		{
			switch (dir) {
			case 0: // UP_SOUTH = 0,
					// UP
				markBoundaryInDirection(vol, L, M, N, neighborOffset[12]);
				// SOUTH
				markBoundaryInDirection(vol, L, M, N, neighborOffset[17]);
				break;
			case 1: // NORT_EAST,
					// NOTH
				markBoundaryInDirection(vol, L, M, N, neighborOffset[16]);
				// EAST
				markBoundaryInDirection(vol, L, M, N, neighborOffset[14]);
				break;
			case 2: // DOWN_WEST,
					// DOWN
				markBoundaryInDirection(vol, L, M, N, neighborOffset[13]);
				// WEST
				markBoundaryInDirection(vol, L, M, N, neighborOffset[15]);
				break;
			case 3: //  SOUTH_EAST,
					// SOUTH
				markBoundaryInDirection(vol, L, M, N, neighborOffset[17]);
				// EAST
				markBoundaryInDirection(vol, L, M, N, neighborOffset[14]);
				break;
			case 4: // UP_WEST,
					// UP
				markBoundaryInDirection(vol, L, M, N, neighborOffset[12]);
				// WEST
				markBoundaryInDirection(vol, L, M, N, neighborOffset[15]);
				break;
			case 5: // DOWN_NORTH,
					// DOWN
				markBoundaryInDirection(vol, L, M, N, neighborOffset[13]);
				// NORTH
				markBoundaryInDirection(vol, L, M, N, neighborOffset[16]);
				break;
			case 6: // SOUTH_WEST,
					// SOUTH
				markBoundaryInDirection(vol, L, M, N, neighborOffset[17]);
				// WEST
				markBoundaryInDirection(vol, L, M, N, neighborOffset[15]);
				break;
			case 7: // UP_NORTH,
					// UP
				markBoundaryInDirection(vol, L, M, N, neighborOffset[12]);
				// NORTH
				markBoundaryInDirection(vol, L, M, N, neighborOffset[16]);
				break;
			case 8: // DOWN_EAST,
					// DOWN
				markBoundaryInDirection(vol, L, M, N, neighborOffset[13]);
				// EAST
				markBoundaryInDirection(vol, L, M, N, neighborOffset[14]);
				break;
			case 9: //  NORT_WEST,
					// NORTH
				markBoundaryInDirection(vol, L, M, N, neighborOffset[16]);
				// WEST
				markBoundaryInDirection(vol, L, M, N, neighborOffset[15]);
				break;
			case 10: // UP_EAST,
					 // UP
				markBoundaryInDirection(vol, L, M, N, neighborOffset[12]);
				// EAST
				markBoundaryInDirection(vol, L, M, N, neighborOffset[14]);
				break;
			case 11: // DOWN_SOUTH,
					 // DOWN
				markBoundaryInDirection(vol, L, M, N, neighborOffset[13]);
				// SOUTH
				markBoundaryInDirection(vol, L, M, N, neighborOffset[17]);
				break;
			}

			// check each boundary point and remove it if it matches a template
			for (k = 1; k < (N - 1); k++) 
			{
				for (j = 1; j < (M - 1); j++) 
				{
					for (i = 1; i < (L - 1); i++) 
					{
						idx = k * slsz + j * L + i;

						if (vol[idx] == SpecialValues::D_BORDER)
						{
							// copy neighborhood into buffer
							copyNeighborhoodInBuffer(vol, idx, volNeighbors, nb);

							transformNeighborhood(nb, dir, USn);

							if (matchesATemplate(USn)) 
							{
								// can be removed
								vol[idx] = SpecialValues::SIMPLE;
								nrDel++;
							}
						}
					}
				}
			}

			// reset all object voxels to OBJECT
			for (idx = 0; idx < sz; idx++) 
			{
				// delete simple points
				if (vol[idx] == SpecialValues::SIMPLE)
					vol[idx] = 0;
				if (vol[idx] != 0)
					vol[idx] = SpecialValues::OBJECT;
			}
		}
	}

}

} // namespace itk
