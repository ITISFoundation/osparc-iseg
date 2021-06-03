/*=========================================================================

  Program: GDCM (Grassroots DICOM). A DICOM library
  Module:  $URL$

  Copyright (c) 2006-2008 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMyGDCMPolyDataReader - read DICOM PolyData files (Contour Data...)
// .SECTION Description
// For now only support RTSTRUCT (RTStructureSetStorage)
// See Contour Data
// .SECTION TODO
// Need to do the same job for DVH Sequence/DVH Data...
//
// .SECTION See Also
// vtkMedicalImageReader2 vtkMedicalImageProperties vtkGDCMImageReader

#ifndef __vtkMyGDCMPolyDataReader_h
#define __vtkMyGDCMPolyDataReader_h

#include "vtkGDCMApi.h"
#include "vtkPolyDataAlgorithm.h"

namespace gdcm
{
class Reader;
}
class vtkMedicalImageProperties;

class vtkGDCM_API vtkMyGDCMPolyDataReader : public vtkPolyDataAlgorithm
{
public:
  static vtkMyGDCMPolyDataReader* New();
  vtkTypeMacro(vtkMyGDCMPolyDataReader, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Description:
  // Set/Get the filename of the file to be read
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Get the medical image properties object
  vtkGetObjectMacro(MedicalImageProperties, vtkMedicalImageProperties);

protected:
  vtkMyGDCMPolyDataReader();
  ~vtkMyGDCMPolyDataReader() override;

  char* FileName;
  vtkMedicalImageProperties* MedicalImageProperties;
  // BTX
  void FillMedicalImageInformation(const gdcm::Reader& reader);
  // ETX

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestInformation(vtkInformation* vtkNotUsed(request),
    vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector) override;
  // BTX
  int RequestInformation_RTStructureSetStorage(gdcm::Reader const& reader);
  int RequestData_RTStructureSetStorage(
    gdcm::Reader const& reader, vtkInformationVector* outputVector);
  int RequestInformation_HemodynamicWaveformStorage(gdcm::Reader const& reader);
  int RequestData_HemodynamicWaveformStorage(
    gdcm::Reader const& reader, vtkInformationVector* outputVector);
  // ETX

private:
  vtkMyGDCMPolyDataReader(const vtkMyGDCMPolyDataReader&) = delete;
  void operator=(const vtkMyGDCMPolyDataReader&) = delete;
};

namespace gdcmvtk_rtstruct
{
class vtkGDCM_API tissuestruct
{
public:
  std::string name;
  float color[3];
  std::vector<unsigned int> outlinelength;
  std::vector<float> points;
};

class vtkGDCM_API tissuevec : public std::vector<tissuestruct*>
{
public:
  ~tissuevec();
};

vtkGDCM_API int RequestData_RTStructureSetStorage(const char* filename, tissuevec& tissues);
vtkGDCM_API bool GetDicomUsingGDCM(
  const char* filename, float* bits, unsigned short& w, unsigned short& h);
vtkGDCM_API bool GetDicomUsingGDCM(const char* filename, float* bits, unsigned short& w,
  unsigned short& h, unsigned short& nrslices);
vtkGDCM_API bool GetSizeUsingGDCM(const char* filename, unsigned short& w, unsigned short& h,
  unsigned short& nrslices, float& dx, float& dy, float& dz, float* disp, float* dc);
vtkGDCM_API bool GetSizeUsingGDCM(const std::vector<std::string>& filenames, unsigned short& w,
  unsigned short& h, unsigned short& nrslices, float& dx, float& dy, float& dz, float* disp,
  float* c1, float* c2, float* c3);
vtkGDCM_API double GetZSPacing(const std::vector<std::string>& files);
}

#endif
