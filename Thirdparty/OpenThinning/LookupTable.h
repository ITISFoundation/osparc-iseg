#ifndef LOOKUPTABLE_H
#define LOOKUPTABLE_H

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

// A LookupTable is a lookup table of 2^26 boolean entries, one for each combination
// of a lokal 3x3x3 neighborhood of digital voxels (either set to 0 or 1), where
// the middle voxel always is 1 (thus 2^26 and not 2^27).
// The lookup table stores a combination of the Euler criterion, the Simple Point criterion
// and - depending on the lookup table - the medial axis endpoint or medial surface point criterions.
//
class LookupTable
{
	public:
		typedef unsigned char Entry;

	public:
		// Read/write a lookup table binary file
		inline bool readFile ( const std::string &_filename );
		inline bool writeFile( const std::string &_filename ) const;

		// Get the stored lookup table entry. The value of the middle voxel is ignored.
		template<typename Voxel>
		Entry getEntry( const Voxel _neighborhood[27] ) const;

	private:
		// The stored lookup table entries
		std::vector<Entry> m_entries;
};

// Read a lookup table binary file. The file is 2^26 bits = 8 MiB in size.
bool LookupTable::readFile( const std::string &_filename )
{
	// Open the file
	std::ifstream ifs( _filename, std::ios::binary );
	if( !ifs.is_open() )
	{
		std::cerr << "Could not open file \"" << _filename << "\"." << std::endl;
		return false;
	}

	// Allocate memory
	m_entries.clear();
	m_entries.resize( 1<<26 );

	// Read 2^23 bytes (= 2^26 bits)
	for( int byteIdx = 0; byteIdx < (1<<23); ++byteIdx )
	{
		// Read the current byte
		unsigned char byte = 0;
		ifs >> std::noskipws >> byte;

		// Check that no errors occured
		if( !ifs.good() )
		{
			std::cerr << "Could not read file \"" << _filename << "\"." << std::endl;
			ifs.close();
			return false;
		}

		// Set the lookup table entries for the current byte
		m_entries[8 * byteIdx + 0] = (byte >> 7) & 0x1;
		m_entries[8 * byteIdx + 1] = (byte >> 6) & 0x1;
		m_entries[8 * byteIdx + 2] = (byte >> 5) & 0x1;
		m_entries[8 * byteIdx + 3] = (byte >> 4) & 0x1;
		m_entries[8 * byteIdx + 4] = (byte >> 3) & 0x1;
		m_entries[8 * byteIdx + 5] = (byte >> 2) & 0x1;
		m_entries[8 * byteIdx + 6] = (byte >> 1) & 0x1;
		m_entries[8 * byteIdx + 7] = (byte >> 0) & 0x1;
	}

	// Close the file and return success
	ifs.close();
	return true;
}

// Write a lookup table binary file. The file is 2^26 bits = 8 MiB in size.
bool LookupTable::writeFile( const std::string &_filename ) const
{
	// Open the file
	std::ofstream ofs( _filename, std::ios::binary );
	if( !ofs.is_open() )
	{
		std::cerr << "Could not open file \"" << _filename << "\"." << std::endl;
		return false;
	}

	// Write 2^23 bytes (= 2^26 bits)
	for( int byteIdx = 0; byteIdx < (1<<23); ++byteIdx )
	{
		// Set the current byte
		unsigned char byte =
			(m_entries[8 * byteIdx + 0] << 7) |
			(m_entries[8 * byteIdx + 1] << 6) |
			(m_entries[8 * byteIdx + 2] << 5) |
			(m_entries[8 * byteIdx + 3] << 4) |
			(m_entries[8 * byteIdx + 4] << 3) |
			(m_entries[8 * byteIdx + 5] << 2) |
			(m_entries[8 * byteIdx + 6] << 1) |
			(m_entries[8 * byteIdx + 7] << 0);

		// Write the current byte
		ofs << byte;
	}

	// Close the file and return success
	ofs.close();
	return true;
}

// Get the stored lookup table entry. The value of the middle voxel is ignored.
template<typename Voxel>
LookupTable::Entry LookupTable::getEntry( const Voxel _neighborhood[27] ) const
{
	// Get the index in the lookup table
	int entryIdx = (
		(_neighborhood[ 0] <<  0) | (_neighborhood[ 1] <<  1) | (_neighborhood[ 2] <<  2) |
		(_neighborhood[ 3] <<  3) | (_neighborhood[ 4] <<  4) | (_neighborhood[ 5] <<  5) |
		(_neighborhood[ 6] <<  6) | (_neighborhood[ 7] <<  7) | (_neighborhood[ 8] <<  8) |
	
		(_neighborhood[ 9] <<  9) | (_neighborhood[10] << 10) | (_neighborhood[11] << 11) |
		(_neighborhood[12] << 12) |      /* ignored */          (_neighborhood[14] << 13) |
		(_neighborhood[15] << 14) | (_neighborhood[16] << 15) | (_neighborhood[17] << 16) |

		(_neighborhood[18] << 17) | (_neighborhood[19] << 18) | (_neighborhood[20] << 19) |
		(_neighborhood[21] << 20) | (_neighborhood[22] << 21) | (_neighborhood[23] << 22) |
		(_neighborhood[24] << 23) | (_neighborhood[25] << 24) | (_neighborhood[26] << 25)
	);

	// Return the lookup table entry
	return m_entries[ entryIdx ];
}


#endif // LOOKUPTABLE_H
