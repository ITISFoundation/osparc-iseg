#pragma once

namespace itk {

template<class TInputImage, class TOutputImage>
OpenThinning<TInputImage, TOutputImage>::OpenThinning()
{
	this->SetNumberOfRequiredOutputs(1);

	auto thinImage = OutputImageType::New();
	this->SetNthOutput(0, thinImage.GetPointer());
}

template<class TInputImage, class TOutputImage>
void OpenThinning<TInputImage, TOutputImage>::PrintSelf(std::ostream& os, Indent indent) const
{
	Superclass::PrintSelf(os, indent);
}

template<class TInputImage, class TOutputImage>
void itk::OpenThinning<TInputImage, TOutputImage>::SetLookupTable(const LookupTable& lut)
{
	m_LookupTable = lut;

	this->Modified();
}

template<class TInputImage, class TOutputImage>
void OpenThinning<TInputImage, TOutputImage>::GenerateData()
{
	auto inputImage = dynamic_cast<const TInputImage*>(ProcessObject::GetInput(0));
	auto thinImage = dynamic_cast<TOutputImage*>(this->GetOutput(0));

	// allocate output
	auto region = thinImage->GetRequestedRegion();
	thinImage->SetBufferedRegion(region);
	thinImage->CopyInformation(inputImage);
	thinImage->Allocate();

	// copy input to output
	using OutputPixel = OutputImagePixelType;
	itk::ImageRegionConstIterator<TInputImage> sit(inputImage, region);
	itk::ImageRegionIterator<TOutputImage> dit(thinImage, region);

	for (sit.GoToBegin(), dit.GoToBegin(); !dit.IsAtEnd(); ++sit, ++dit)
	{
		dit.Set(sit.Get() != 0 ? itk::NumericTraits<OutputPixel>::OneValue() : itk::NumericTraits<OutputPixel>::ZeroValue());
	}

	ComputeThinImage(thinImage, m_LookupTable);
}

template<class TInputImage, class TOutputImage>
void OpenThinning<TInputImage, TOutputImage>::ComputeThinImage(TOutputImage* thinImage, const LookupTable& _lookupTable)
{
	using Voxel = OutputImagePixelType;

	struct VolumeDataWrapper
	{
		VolumeDataWrapper(Voxel* _voxels, size_t _sizeX, size_t _sizeY, size_t _sizeZ)
				: m_voxels(_voxels), m_sizeX(_sizeX), m_sizeY(_sizeY), m_sizeZ(_sizeZ)
		{
			m_number_of_voxels = m_sizeX * m_sizeY * m_sizeZ;
		}

		inline void setVoxel(size_t _x, size_t _y, size_t _z, Voxel _voxel) { m_voxels[getVoxelIdx(_x, _y, _z)] = _voxel; }
		inline Voxel getVoxel(size_t _x, size_t _y, size_t _z) const { return m_voxels[getVoxelIdx(_x, _y, _z)]; }
		inline Voxel getVoxelChecked(size_t _x, size_t _y, size_t _z) const
		{
			// unsigned: we only need to check upper bound
			if (_x < m_sizeX && _y < m_sizeY && _z < m_sizeZ)
			{
				return m_voxels[getVoxelIdx(_x, _y, _z)];
			}
			return m_constant;
		}

		// Get the 3x3x3 neighborhood of voxels around the given voxel position
		void getNeighborhood(size_t _x, size_t _y, size_t _z, Voxel _neighborhood[27]) const
		{
			if (_x + 1 < m_sizeX && _y + 1 < m_sizeY && _z + 1 < m_sizeZ && _x > 0 && _y > 0 && _z > 0)
			{
				_neighborhood[0] = getVoxel(_x - 1, _y - 1, _z - 1);
				_neighborhood[1] = getVoxel(_x, _y - 1, _z - 1);
				_neighborhood[2] = getVoxel(_x + 1, _y - 1, _z - 1);
				_neighborhood[3] = getVoxel(_x - 1, _y, _z - 1);
				_neighborhood[4] = getVoxel(_x, _y, _z - 1);
				_neighborhood[5] = getVoxel(_x + 1, _y, _z - 1);
				_neighborhood[6] = getVoxel(_x - 1, _y + 1, _z - 1);
				_neighborhood[7] = getVoxel(_x, _y + 1, _z - 1);
				_neighborhood[8] = getVoxel(_x + 1, _y + 1, _z - 1);

				_neighborhood[9] = getVoxel(_x - 1, _y - 1, _z);
				_neighborhood[10] = getVoxel(_x, _y - 1, _z);
				_neighborhood[11] = getVoxel(_x + 1, _y - 1, _z);
				_neighborhood[12] = getVoxel(_x - 1, _y, _z);
				_neighborhood[13] = getVoxel(_x, _y, _z);
				_neighborhood[14] = getVoxel(_x + 1, _y, _z);
				_neighborhood[15] = getVoxel(_x - 1, _y + 1, _z);
				_neighborhood[16] = getVoxel(_x, _y + 1, _z);
				_neighborhood[17] = getVoxel(_x + 1, _y + 1, _z);

				_neighborhood[18] = getVoxel(_x - 1, _y - 1, _z + 1);
				_neighborhood[19] = getVoxel(_x, _y - 1, _z + 1);
				_neighborhood[20] = getVoxel(_x + 1, _y - 1, _z + 1);
				_neighborhood[21] = getVoxel(_x - 1, _y, _z + 1);
				_neighborhood[22] = getVoxel(_x, _y, _z + 1);
				_neighborhood[23] = getVoxel(_x + 1, _y, _z + 1);
				_neighborhood[24] = getVoxel(_x - 1, _y + 1, _z + 1);
				_neighborhood[25] = getVoxel(_x, _y + 1, _z + 1);
				_neighborhood[26] = getVoxel(_x + 1, _y + 1, _z + 1);
			}
			else
			{
				_neighborhood[0] = getVoxelChecked(_x - 1, _y - 1, _z - 1);
				_neighborhood[1] = getVoxelChecked(_x, _y - 1, _z - 1);
				_neighborhood[2] = getVoxelChecked(_x + 1, _y - 1, _z - 1);
				_neighborhood[3] = getVoxelChecked(_x - 1, _y, _z - 1);
				_neighborhood[4] = getVoxelChecked(_x, _y, _z - 1);
				_neighborhood[5] = getVoxelChecked(_x + 1, _y, _z - 1);
				_neighborhood[6] = getVoxelChecked(_x - 1, _y + 1, _z - 1);
				_neighborhood[7] = getVoxelChecked(_x, _y + 1, _z - 1);
				_neighborhood[8] = getVoxelChecked(_x + 1, _y + 1, _z - 1);

				_neighborhood[9] = getVoxelChecked(_x - 1, _y - 1, _z);
				_neighborhood[10] = getVoxelChecked(_x, _y - 1, _z);
				_neighborhood[11] = getVoxelChecked(_x + 1, _y - 1, _z);
				_neighborhood[12] = getVoxelChecked(_x - 1, _y, _z);
				_neighborhood[13] = getVoxelChecked(_x, _y, _z);
				_neighborhood[14] = getVoxelChecked(_x + 1, _y, _z);
				_neighborhood[15] = getVoxelChecked(_x - 1, _y + 1, _z);
				_neighborhood[16] = getVoxelChecked(_x, _y + 1, _z);
				_neighborhood[17] = getVoxelChecked(_x + 1, _y + 1, _z);

				_neighborhood[18] = getVoxelChecked(_x - 1, _y - 1, _z + 1);
				_neighborhood[19] = getVoxelChecked(_x, _y - 1, _z + 1);
				_neighborhood[20] = getVoxelChecked(_x + 1, _y - 1, _z + 1);
				_neighborhood[21] = getVoxelChecked(_x - 1, _y, _z + 1);
				_neighborhood[22] = getVoxelChecked(_x, _y, _z + 1);
				_neighborhood[23] = getVoxelChecked(_x + 1, _y, _z + 1);
				_neighborhood[24] = getVoxelChecked(_x - 1, _y + 1, _z + 1);
				_neighborhood[25] = getVoxelChecked(_x, _y + 1, _z + 1);
				_neighborhood[26] = getVoxelChecked(_x + 1, _y + 1, _z + 1);
			}
		}

	private:
		// Calculate the index in the stored data
		inline size_t getVoxelIdx(size_t _x, size_t _y, size_t _z) const 
		{ 
#if 1
			return m_sizeX * (m_sizeY * _z + _y) + _x;
#else
			size_t idx = m_sizeX * (m_sizeY * _z + _y) + _x;
			if (idx < m_number_of_voxels)
			{
				return idx;
			}
			throw std::out_of_range("Index out of range");
#endif
		}

	private:
		Voxel* m_voxels = nullptr;
		Voxel m_constant = 0;

		// The size of the volume
		size_t m_sizeX = 0;
		size_t m_sizeY = 0;
		size_t m_sizeZ = 0;
		size_t m_number_of_voxels = 0;
	};

	auto region = thinImage->GetRequestedRegion();
	VolumeDataWrapper volumeData(thinImage->GetPixelContainer()->GetImportPointer(),
			region.GetSize(0),
			region.GetSize(1),
			region.GetSize(2));

	// One position offset in x, y and z for each of the six direction (left, right, down, up, backward, forward)
	static const int OFFSETS[6][3] = {
			{-1, 0, 0},
			{1, 0, 0},
			{0, -1, 0},
			{0, 1, 0},
			{0, 0, -1},
			{0, 0, 1}};

	// Get the size of the stord volume data
	unsigned short sizeX = static_cast<unsigned short>(region.GetSize(0));
	unsigned short sizeY = static_cast<unsigned short>(region.GetSize(1));
	unsigned short sizeZ = static_cast<unsigned short>(region.GetSize(2));

	// Iterate as long as the volume data was modified.
	// To stop this, the volume data has to be unmodified after all six direction subcycles (not just one).
	while (true)
	{
		// The volume data was not modified so far
		bool modified = false;

		// Loop through all six directions (left, right, down, up, backward, forward)
		for (int directionIdx = 0; directionIdx < 6; ++directionIdx)
		{
			// Get the position offset for the current direction
			const int* offset = OFFSETS[directionIdx];

			// Gather all the candidate positions for the current direction.
			// We first gather all candidates instead of trying to delete (set to 0)
			// the voxels immediately, because immediate deletion could lead to ripple effects
			// that delete more than one front voxel coming from the current direction.
			// This is done to ensure that the thinning result is most likely to be in the middle.
			std::deque<std::tuple<unsigned short, unsigned short, unsigned short>> candidates;
			{
				// Check each voxel once
				for (unsigned short z = 0; z < sizeZ; ++z)
				{
					for (unsigned short y = 0; y < sizeY; ++y)
					{
						for (unsigned short x = 0; x < sizeX; ++x)
						{
							// The voxel has to be set to 1
							if (!volumeData.getVoxel(x, y, z))
								continue;

							// The predecessor voxel coming from the current direction has to be 0
							if (volumeData.getVoxelChecked(x + offset[0], y + offset[1], z + offset[2]))
								continue;

							// Get the local neighborhood of the current voxel
							Voxel neighborhood[27];
							volumeData.getNeighborhood(x, y, z, neighborhood);

							// Check the lookup table to see if the voxel / the neighborhood fulfills the Euler criterion,
							// the Simple Point criterion and - depending on the lookup table - the medial axis endpoint or
							// medial surface point criterions
							if (_lookupTable.getEntry(neighborhood))
								candidates.push_back(std::make_tuple(x, y, z));
						}
					}
				}
			}

			// Recheck all candidate positions. The deletion of one candidate voxel might invalidate a later candidate.
			for (const auto& candidate : candidates)
			{
				// Get the position of the current candidate
				size_t x = std::get<0>(candidate);
				size_t y = std::get<1>(candidate);
				size_t z = std::get<2>(candidate);

				// Get the local neighborhood of the current candidate voxel, again.
				// Though, because of earlier deletions, this neighborhood might have changed in the meantime.
				Voxel neighborhood[27];
				volumeData.getNeighborhood(x, y, z, neighborhood);

				// Recheck the neighborhood
				if (_lookupTable.getEntry(neighborhood))
				{
					// Delete (set to 0) the candidate voxel
					volumeData.setVoxel(x, y, z, 0);

					// The volume data was modified. Another iteration is needed.
					modified = true;
				}
			}
		}

		// If the volume data was not modified after a all six direction subcycles, stop.
		if (!modified)
			break;
	}
}

} // namespace itk
