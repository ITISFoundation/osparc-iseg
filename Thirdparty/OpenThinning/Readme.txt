Description:
------------
OpenThinning is an open source project that is able to read 3D volumes (or 2D images),
convert the voxels values into set or unset voxels, and thin these inputs with the help
of three different lookup tables that are also provided within this software package.


Instructions:
-------------
Create a new build folder within the OpenThinning folder.
Run cmake from within this build folder ("cmake ..").
Compile the OpenThinning project.
This should create a debug or release folder within the build folder.
Run the OpenThinning executable from within this debug or release folder.
OpenThinning was tested with VS 2013 and VTK 6.2.


Usage:
------
Call the OpenThinning executable with parameters as follows:
OpenThinning <Lookup Table Filename> <Input Volume Filename> <Size in X> <Size in Y> <Size in Z> <Threshold> [<Output Volume Filename>]
If the <Input Volume Filename> ends with "png" (lower case), one png file is read for each slice on the Z axis.
Here, the filename is the pattern for the png files (see examples below). Otherwise, a raw file (one unsigned byte per voxel) is read as input volume.
It is optional to provide an <Output Volume Filename>, but the thinned volume is written only if the filename is provided.
The ending of the <Output Volume Filename> determines, if png files or a raw file is written.
If no or an invalid number of parameters is provided, a default lookup table and input volume is used for demonstration purposes.

Examples (Win):
OpenThinning.exe
OpenThinning.exe "../../Data/LookupTables/Thinning_Simple.bin" "../../Data/Volumes/VolumeA.raw" 256 256 256 100.0
OpenThinning.exe "../../Data/LookupTables/Thinning_Simple.bin" "../../Data/Volumes/VolumeA.raw" 256 256 256 100.0 "../../Data/Volumes/Thinned_VolumeA.raw"
OpenThinning.exe "../../Data/LookupTables/Thinning_Simple.bin" "../../Data/Volumes/VolumeA.raw" 256 256 256 100.0 "../../Data/Volumes/Thinned_VolumeA/Slice%%03i.png"
OpenThinning.exe "../../Data/LookupTables/Thinning_MedialAxis.bin" "../../Data/Volumes/VolumeB/Slice%%03i.png" 512 512 512 80.0
OpenThinning.exe "../../Data/LookupTables/Thinning_MedialAxis.bin" "../../Data/Volumes/VolumeB/Slice%%03i.png" 512 512 512 80.0 "../../Data/Volumes/Thinned_VolumeB.raw"
OpenThinning.exe "../../Data/LookupTables/Thinning_MedialAxis.bin" "../../Data/Volumes/VolumeB/Slice%%03i.png" 512 512 512 80.0 "../../Data/Volumes/Thinned_VolumeB/Slice%%03i.png"


Legal stuff:
------------
See License.txt for the license OpenThinning is under.
