/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "Precompiled.h"

#include "vtkGenericDataSetWriter.h"

#include <vtkDemandDrivenPipeline.h>
#include <vtkExecutive.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>

#include <vtkDataSet.h>
#include <vtkPolyData.h>
#include <vtkRectilinearGrid.h>
#include <vtkStructuredGrid.h>
#include <vtkStructuredPoints.h>
#include <vtkUnstructuredGrid.h>

#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkFloatArray.h>
#include <vtkIdList.h>
#include <vtkIntArray.h>
#include <vtkShortArray.h>
#include <vtkStringArray.h>

#include <vtkDataSetWriter.h>
#include <vtkSTLWriter.h>
#include <vtkXMLImageDataWriter.h>
#include <vtkXMLPolyDataWriter.h>
#include <vtkXMLStructuredGridWriter.h>
#include <vtkXMLUnstructuredGridWriter.h>

#include <vtkSmartPointer.h>
#define vtkNew(type, name)                                                     \
	vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

#include <cassert>
#include <map>
#include <string>

using namespace std;

vtkStandardNewMacro(vtkGenericDataSetWriter);

vtkGenericDataSetWriter::vtkGenericDataSetWriter()
{
	this->SetNumberOfInputPorts(1);
	this->SetNumberOfOutputPorts(0);
	this->FileName = nullptr;
	this->FileType = VTK_BINARY;
	this->DataMode = vtkXMLWriter::Appended;
	this->EncodeAppendedData = 0;
	this->Loud = 0;
	this->MaterialArrayName = 0;
	this->SetMaterialArrayName("TissueIndex");
	this->FlipTriangles = false;
}

vtkGenericDataSetWriter::~vtkGenericDataSetWriter() { this->SetFileName(nullptr); }

void vtkGenericDataSetWriter::SetInput(vtkDataObject *input)
{
	this->SetInput(0, input);
}

void vtkGenericDataSetWriter::SetInput(int index, vtkDataObject *input)
{
	if (input) {
		this->SetInputData(index, input);
	}
	else {
		// Setting a nullptr input remove the connection.
		this->SetInputConnection(index, 0);
	}
}

int vtkGenericDataSetWriter::RequestData(
		vtkInformation *vtkNotUsed(request), vtkInformationVector **inputVector,
		vtkInformationVector *vtkNotUsed(outputVector))
{
	vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
	vtkDataObject *input = inInfo->Get(vtkDataObject::DATA_OBJECT());

	string ibase, iext, iname = this->FileName;
	decomposeFileName(ibase, iext, iname);

	if (iext == "stl") {
		vtkNew(vtkSTLWriter, writer);
		writer->SetInputData(input);
		writer->SetFileName(iname.c_str());
		writer->SetFileType(this->FileType);
		writer->Update();
	}
	else if (iext == "vtp") {
		vtkNew(vtkXMLPolyDataWriter, writer);
		writer->SetInputData(input);
		writer->SetFileName(iname.c_str());
		writer->SetCompressorTypeToNone();
		writer->SetDataMode(this->DataMode);
		writer->SetEncodeAppendedData(this->EncodeAppendedData);
		writer->Update();
	}
	else if (iext == "dat") {
		this->WriteEsraTriangles(vtkPolyData::SafeDownCast(input), FileName);
	}
	else if (iext == "vtu") {
		vtkNew(vtkXMLUnstructuredGridWriter, writer);
		writer->SetInputData(input);
		writer->SetFileName(iname.c_str());
		writer->SetCompressorTypeToNone();
		writer->SetDataMode(this->DataMode);
		writer->SetEncodeAppendedData(this->EncodeAppendedData);
		writer->Update();
	}
	else if (iext == "vti") {
		vtkNew(vtkXMLImageDataWriter, writer);
		writer->SetInputData(input);
		writer->SetFileName(iname.c_str());
		writer->SetCompressorTypeToNone();
		writer->SetDataMode(this->DataMode);
		writer->SetEncodeAppendedData(this->EncodeAppendedData);
		writer->Update();
	}
	else if (iext == "vts") {
		vtkNew(vtkXMLStructuredGridWriter, writer);
		writer->SetInputData(input);
		writer->SetFileName(iname.c_str());
		writer->SetCompressorTypeToNone();
		writer->SetDataMode(this->DataMode);
		writer->SetEncodeAppendedData(this->EncodeAppendedData);
		writer->Update();
	}
	else if (iext == "vtk") {
		vtkNew(vtkDataSetWriter, writer);
		writer->SetInputData(input);
		writer->SetFileName(iname.c_str());
		writer->SetFileType(this->FileType);
		writer->Update();
	}
	else {
		std::cerr << "vtkGenericDataSetWriter::RequestData(): Cannot write this file "
						"format."
				 << endl;
		return 0;
	}

	return 1;
}

void vtkGenericDataSetWriter::decomposeFileName(std::string &bname,
																								std::string &extname,
																								const std::string &fname)
{
	const std::string::size_type idx = fname.rfind(".");
	bname = (idx != std::string::npos) ? fname.substr(0, idx) : fname;
	extname = (idx != std::string::npos) ? fname.substr(idx + 1) : "";
}

void vtkGenericDataSetWriter::PrintSelf(ostream &os, vtkIndent indent)
{
	this->Superclass::PrintSelf(os, indent);
}

bool vtkGenericDataSetWriter::WriteEsraTriangles(vtkPolyData *surface,
																								 const char *fname)
{
	if (surface == nullptr)
		return false;

	FILE *fp;
	if ((fp = fopen(fname, "wb")) == nullptr)
		return false;

	// Find index array for tissues
	vtkDataArray *tissueArray =
			surface->GetCellData()->GetArray(this->MaterialArrayName);

	// try other typical names
	if (tissueArray == nullptr) {
		tissueArray = surface->GetCellData()->GetArray("TissueIndex");
	}
	if (tissueArray == nullptr) {
		tissueArray = surface->GetCellData()->GetArray("Material");
	}
	if (tissueArray == nullptr && surface->GetCellData()->GetNumberOfArrays()) {
		tissueArray = surface->GetCellData()->GetArray(0);
		std::cerr << "Warning: could not find named array " << std::endl;
	}
	vtkIntArray *tissueArrayInt = vtkIntArray::SafeDownCast(tissueArray);
	vtkShortArray *tissueArrayShort = vtkShortArray::SafeDownCast(tissueArray);

	// Find tissue name array
	vtkStringArray *tissueNames = vtkStringArray::SafeDownCast(
			surface->GetFieldData()->GetAbstractArray("TissueNames"));
	vtkDataArray *tissueColors = surface->GetFieldData()->GetArray("Colors");

	// Split surface into tissue parts
	std::map<int, vtkCellArray *> tissueCells;
	vtkIdType npts, *pts;
	surface->BuildCells();
	if (tissueArray != nullptr) {
		for (vtkIdType k = 0; k < surface->GetNumberOfCells(); k++) {
			//assert(surface->GetCellType(k) == VTK_TRIANGLE);
			if (surface->GetCellType(k) != VTK_TRIANGLE) {
				surface->GetCellPoints(k, npts, pts);
				vtkWarningMacro(<< "surface->GetCellType(k) != VTK_TRIANGLE "
												<< surface->GetCellType(k) << " " << npts);
				return false;
			}
			int tissueId;
			if (tissueArrayInt)
				tissueId = tissueArrayInt->GetValue(k);
			else if (tissueArrayShort)
				tissueId = tissueArrayShort->GetValue(k);
			else
				tissueId = tissueArray->GetTuple1(k);

			if (tissueCells.find(tissueId) == tissueCells.end()) {
				tissueCells[tissueId] = vtkCellArray::New();
				tissueCells[tissueId]->Allocate(surface->GetNumberOfCells() / 2 + 1);
			}
			surface->GetCellPoints(k, npts, pts);
			assert(npts == 3);
			tissueCells[tissueId]->InsertNextCell(3);
			tissueCells[tissueId]->InsertCellPoint(pts[0]);
			if (this->FlipTriangles == false) {
				tissueCells[tissueId]->InsertCellPoint(pts[1]);
				tissueCells[tissueId]->InsertCellPoint(pts[2]);
			}
			else {
				tissueCells[tissueId]->InsertCellPoint(pts[2]);
				tissueCells[tissueId]->InsertCellPoint(pts[1]);
			}
		}
	}
	else {
		tissueCells[0] = surface->GetPolys();
	}

	// Now write the surfaces to file
	unsigned Ntissues = static_cast<unsigned>(tissueCells.size());
	fwrite(&Ntissues, sizeof(unsigned), 1, fp);

	std::map<int, vtkCellArray *>::iterator it;
	for (it = tissueCells.begin(); it != tissueCells.end(); ++it) {
		vtkIdType tissueId = it->first;
		vtkStdString name;
		if (tissueNames) {
			name = tissueNames->GetValue(tissueId);
		}
		if (name.length() == 0) {
			char buf[20];
			sprintf(buf, "tissue%03d", static_cast<int>(tissueId));
			name = buf;
		}

		// Write color and name
		float rgb[3] = {1.0, 0.0, 0.0};
		if (tissueColors) {
			double *color = tissueColors->GetTuple3(tissueId);
			rgb[0] = color[0];
			rgb[1] = color[1];
			rgb[2] = color[2];
		}
		if (Loud)
			std::cerr << "Color " << rgb[0] << " " << rgb[1] << " " << rgb[2]
								<< std::endl;
		if (Loud)
			std::cerr << "Tissue " << name << std::endl;
		fwrite(rgb, 3 * sizeof(float), 1, fp);
		int l1 = static_cast<int>(name.length());
		fwrite(&l1, sizeof(int), 1, fp);
		const char *cstr_name = name.c_str();
		fwrite(cstr_name, sizeof(char) * (l1 + 1), 1,
					 fp); // ? should I use l1+1 or l1 ?

		// Write triangle for this tissue
		unsigned Ntriids = 3 * tissueCells[tissueId]->GetNumberOfCells();
		if (Loud)
			std::cerr << "Ntris " << Ntriids / 3 << std::endl;
		fwrite(&Ntriids, sizeof(unsigned), 1, fp);
		unsigned *trisarray = (unsigned *)malloc(Ntriids * sizeof(unsigned));
		(it->second)->InitTraversal();
		for (vtkIdType k = 0; k < (it->second)->GetNumberOfCells(); k++) {
			(it->second)->GetNextCell(npts, pts);
			assert(npts == 3);
			trisarray[3 * k] = pts[0];
			trisarray[3 * k + 1] = pts[1];
			trisarray[3 * k + 2] = pts[2];
		}
		fwrite(trisarray, Ntriids * sizeof(unsigned), 1, fp);
		free(trisarray);

		if (tissueArray != nullptr)
			(it->second)->Delete();
	}

	// Write points to file
	unsigned Npoints = surface->GetNumberOfPoints();
	if (Loud)
		std::cerr << "Npoints " << Npoints << std::endl;
	fwrite(&Npoints, sizeof(unsigned), 1, fp);
	float *koordarray = (float *)malloc(Npoints * 3 * sizeof(float));
	double x[3];
	for (vtkIdType i = 0; i < Npoints; i++) {
		surface->GetPoint(i, x);
		koordarray[i * 3] = x[0];
		koordarray[i * 3 + 1] = x[1];
		koordarray[i * 3 + 2] = x[2];
	}
	fwrite(koordarray, Npoints * 3 * sizeof(float), 1, fp);
	free(koordarray);

	fclose(fp);
	return true;
}
