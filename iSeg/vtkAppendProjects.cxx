/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */

#include <cassert>
#include <set>
#include <string>

#include <vtkAppendImageData.h>
#include <vtkCell.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkDataSet.h>
#include <vtkDoubleArray.h>
#include <vtkIdList.h>
#include <vtkImageAppend2.h>
#include <vtkImageData.h>
#include <vtkIntArray.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkSmartPointer.h>
#include <vtkVolume.h>
#include <vtkXMLImageDataReader.h>
#include <vtkXMLImageDataWriter.h>

#include <Options.h>
#include <Tools.h>

#define vtkNew(type, name)                                                     \
	vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

using namespace std;
using namespace Tools;

int main(int argc, char **argv)
{
	string ifnames, ofname;
	string sname;
	int shift = 0;
	int background = 0;
	Options opts(argc, argv);
	opts.add("i", &ifnames);
	opts.add("o", &ofname);
	opts.add("scalar", &sname);
	opts.add("shift", &shift);
	opts.parse();

	opts.print();

	//	if(sname.empty())
	//		error("no scalar");

	vector<string> fnames = tokenize(ifnames, ',');
	if (fnames.size() < 2)
		error("need at least 2 inputs");
	if (ofname.empty())
		error("need output filename");

	//	vtkSmartPointer<vtkImageData> image;
	vtkNew(vtkXMLImageDataReader, reader);

	//	vtkNew(vtkAppendImageData,append);
	// if(shift) append->SetShiftByOrigin(1);
	//   vtkNew(vtkImageAppend,append);
	vtkNew(vtkImageAppend2, append);
	append->SetPreserveExtents(1);
	append->SetUseTransparency(1);

	// cerr << "using threads: " << append->GetNumberOfThreads() << endl;
	// append->SetNumberOfThreads(1);
	cerr << "using threads: " << append->GetNumberOfThreads() << endl;

	double org0[3] = {0.0, 0.0, 0.0};
	double org[3] = {0.0, 0.0, 0.0};
	double dorg[3] = {0.0, 0.0, 0.0};
	double spc[3] = {0.0, 0.0, 0.0};
	int ext[6] = {0, 0, 0, 0, 0, 0};
	int dext[6] = {0, 0, 0, 0, 0, 0};
	int ext1[6] = {0, 0, 0, 0, 0, 0};

	for (int i = 0; i < fnames.size(); ++i) {
		cerr << i << " " << fnames[i] << endl;
		reader->SetFileName(fnames[i].c_str());
		reader->Update();
		vtkNew(vtkImageData, image);
		image->DeepCopy(reader->GetOutput());
		//		append->AddInput(reader->GetOutput());
		// int code = image->GetPointData()->SetActiveScalars(sname.c_str());
		// if(code<0)
		// error("no such data: " + sname);

		image->GetOrigin(org);
		image->GetSpacing(spc);
		image->GetExtent(ext);

		if (shift) {
			if (i == 0) {
				memcpy(org0, org, 3 * sizeof(double));
			}
			else {
				image->SetOrigin(org0);
				cerr << "new origin: " << org0[0] << " " << org0[1] << " " << org0[2]
						 << endl;
				for (int j = 0; j < 3; ++j) {
					dorg[j] = org[j] - org0[j];
				}
				for (int j = 0; j < 3; ++j) {
					dext[j] = int(dorg[j] / spc[j]);
				}
				for (int j = 0; j < 3; ++j) {
					ext1[2 * j + 0] = ext[2 * j + 0] + dext[j];
					ext1[2 * j + 1] = ext[2 * j + 1] + dext[j];
				}
				cerr << "new extent: " << ext1[0] << " " << ext1[1] << " " << ext1[2]
						 << " " << ext1[3] << " " << ext1[4] << " " << ext1[5] << endl;
				image->SetExtent(ext1);
			}
		}

		vtkNew(vtkXMLImageDataWriter, writer);
		writer->SetDataModeToAppended();
		writer->SetEncodeAppendedData(0);
		writer->SetInput(image);
		char buf[1024];
		sprintf(buf, "test-%d.vti", i);
		writer->SetFileName(buf);
		writer->Write();

		append->AddInput(image);
	}

	append->Update();
	// append->GetOutput()->GetPointData()->GetScalars()->SetName(sname.c_str());
	cerr << "writing output image..." << endl;

	//	string obname, oextname;
	//	Tools::decomposeFileName(obname,oextname,ofname);
	vtkNew(vtkXMLImageDataWriter, writer);
	writer->SetDataModeToAppended();
	writer->SetEncodeAppendedData(0);
	writer->SetInput(append->GetOutput());
	writer->SetFileName(ofname.c_str());
	writer->Write();

	return EXIT_SUCCESS;
}
