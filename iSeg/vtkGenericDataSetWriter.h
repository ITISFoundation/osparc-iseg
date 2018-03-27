/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */

#ifndef __vtkGenericDataSetWriter_h
#define __vtkGenericDataSetWriter_h

#include <string>
#include <vector>

#include "vtkDataSetAlgorithm.h"
#include "vtkWriter.h"
#include "vtkXMLWriter.h"

/*
  Writer for several file formats, including: vtk, vtp, vtu, stl, dat

*/
class vtkGenericDataSetWriter : public vtkDataSetAlgorithm
{
public:
	static vtkGenericDataSetWriter *New();
	vtkTypeMacro(vtkGenericDataSetWriter, vtkDataSetAlgorithm);
	void PrintSelf(ostream &os, vtkIndent indent);

	// Set the input dataset
	void SetInput(int index, vtkDataObject *input);
	void SetInput(vtkDataObject *input);

	// Set/get the file name. Will be used to choose the correct reader
	vtkSetStringMacro(FileName) vtkGetStringMacro(FileName)

			// Alias for Update
			void Write()
	{
		this->Update();
	}

	// Set file type to binary or ASCII
	vtkSetClampMacro(FileType, int, VTK_ASCII, VTK_BINARY)
			vtkGetMacro(FileType, int) void SetFileTypeToASCII()
	{
		this->FileType = VTK_ASCII;
	};
	void SetFileTypeToBinary() { this->FileType = VTK_BINARY; };

	// This option will only be used if writing to a vtk xml format
	void SetDataModeToAppended() { this->DataMode = vtkXMLWriter::Appended; };
	void SetDataModeToAscii() { this->DataMode = vtkXMLWriter::Ascii; };
	void SetDataModeToBinary() { this->DataMode = vtkXMLWriter::Binary; };
	vtkSetMacro(EncodeAppendedData, int) vtkGetMacro(EncodeAppendedData, int)

			/**	Set/get the name of the material array: default TissueIndex
		This option only makes sense if you are writing .dat files
		\warning: note that the convention for FieldData arrays (color, tissue 
		names is fixed: 
			tissue names -> "TissueNames"
			color        -> "Colors"
	*/
			vtkSetStringMacro(MaterialArrayName) vtkGetStringMacro(MaterialArrayName)

					vtkSetMacro(FlipTriangles, bool) vtkBooleanMacro(FlipTriangles, bool)

							vtkSetMacro(Loud, int) vtkBooleanMacro(Loud, int)

									protected : vtkGenericDataSetWriter();
	~vtkGenericDataSetWriter();

	int RequestData(vtkInformation *, vtkInformationVector **,
									vtkInformationVector *);

	//BTX
	void decomposeFileName(std::string &bname, std::string &extname,
												 const std::string &fname);
	//ETX

	bool WriteEsraTriangles(vtkPolyData *, const char *);

	char *FileName;
	int FileType;
	int DataMode;
	int EncodeAppendedData;
	char *MaterialArrayName;
	bool FlipTriangles;
	int Loud;

private:
	vtkGenericDataSetWriter(const vtkGenericDataSetWriter &); // Not implemented.
	void operator=(const vtkGenericDataSetWriter &);					// Not implemented.
};

#endif
