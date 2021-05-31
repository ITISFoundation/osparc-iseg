/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#pragma once

#include "Data/SlicesHandlerInterface.h"
#include "Data/Transform.h"

#include "Core/Outline.h" // BL TODO get rid of this
#include "Core/RGB.h"
#include "Core/UndoElem.h"
#include "Core/UndoQueue.h"

// boost 1.48, Qt and [Parse error at "BOOST_JOIN"] error
// https://bugreports.qt.io/browse/QTBUG-22829
#ifndef Q_MOC_RUN
#	include <boost/signals2.hpp>
#endif
#include <boost/variant.hpp>

#include <functional>
#include <memory>
#include <string>

class QString;
class vtkImageData;

namespace iseg {

class Transform;
class TissueHiearchy;
class ColorLookupTable;
class Bmphandler;
class ProgressInfo;

class SlicesHandler : public SlicesHandlerInterface
{
public:
	SlicesHandler();
	~SlicesHandler();

	void Newbmp(unsigned short width1, unsigned short height1, unsigned short nrofslices, const std::function<void(float**)>& init_callback = std::function<void(float**)>());
	void Freebmp();
	void ClearBmp();
	void ClearWork();
	void ClearOverlay();
	void NewOverlay();
	void SetBmp(unsigned short slicenr, float* bits, unsigned char mode);
	void SetWork(unsigned short slicenr, float* bits, unsigned char mode);
	void SetTissue(unsigned short slicenr, tissues_size_t* bits);
	void Copy2bmp(unsigned short slicenr, float* bits, unsigned char mode);
	void Copy2work(unsigned short slicenr, float* bits, unsigned char mode);
	void Copy2tissue(unsigned short slicenr, tissues_size_t* bits);
	void Copyfrombmp(unsigned short slicenr, float* bits);
	void Copyfromwork(unsigned short slicenr, float* bits);
	void Copyfromtissue(unsigned short slicenr, tissues_size_t* bits);
#ifdef TISSUES_SIZE_TYPEDEF
	void Copyfromtissue(unsigned short slicenr, unsigned char* bits);
#endif // TISSUES_SIZE_TYPEDEF
	void Copyfromtissuepadded(unsigned short slicenr, tissues_size_t* bits, unsigned short padding);
	void SetRgbFactors(int redFactor, int greenFactor, int blueFactor);
	int LoadDIBitmap(std::vector<const char*> filenames);
	int LoadDIBitmap(std::vector<const char*> filenames, Point p, unsigned short dx, unsigned short dy);
	int LoadPng(std::vector<const char*> filenames);
	int LoadPng(std::vector<const char*> filenames, Point p, unsigned short dx, unsigned short dy);
	int LoadDIJpg(std::vector<const char*> filenames);
	int LoadDIJpg(std::vector<const char*> filenames, Point p, unsigned short dx, unsigned short dy);
	int LoadDICOM(std::vector<const char*> lfilename); //Assumption Filenames: fnxxx.bmp xxx: 3 digit number
	int LoadDICOM(std::vector<const char*> lfilename, Point p, unsigned short dx, unsigned short dy);
	int ReadImage(const char* filename);
	int ReadOverlay(const char* filename, unsigned short slicenr);
	int ReadAvw(const char* filename);
	int ReadRTdose(const char* filename);
	bool LoadSurface(const std::string& filename, bool overwrite_working, bool intersect);

	int LoadAllXdmf(const char* filename);
	int LoadAllHDF(const char* filename);

	void UpdateColorLookupTable(std::shared_ptr<ColorLookupTable> new_lut = nullptr);
	std::shared_ptr<ColorLookupTable> GetColorLookupTable() { return m_ColorLookupTable; }

	// Description: write project data into an Xdmf file
	int SaveAllXdmf(const char* filename, int compression, bool save_work, bool naked);
	bool SaveMarkersHDF(const char* filename, bool naked, unsigned short version);
	int SaveMergeAllXdmf(const char* filename, std::vector<QString>& mergeImagefilenames, unsigned short nrslicesTotal, int compression);
	int ReadRaw(const char* filename, short unsigned w, short unsigned h, unsigned bitdepth, unsigned short slicenr, unsigned short nrofslices);
	int ReadRaw(const char* filename, short unsigned w, short unsigned h, unsigned bitdepth, unsigned short slicenr, unsigned short nrofslices, Point p, unsigned short dx, unsigned short dy);
	int ReadRawFloat(const char* filename, short unsigned w, short unsigned h, unsigned short slicenr, unsigned short nrofslices);
	int ReadRawFloat(const char* filename, short unsigned w, short unsigned h, unsigned short slicenr, unsigned short nrofslices, Point p, unsigned short dx, unsigned short dy);
	int ReadRawOverlay(const char* filename, unsigned bitdepth, unsigned short slicenr);
	int SaveRawResized(const char* filename, int dxm, int dxp, int dym, int dyp, int dzm, int dzp, bool work);
	int SaveTissuesRawResized(const char* filename, int dxm, int dxp, int dym, int dyp, int dzm, int dzp);
	bool SwapXY();
	bool SwapYZ();
	bool SwapXZ();
	int SaveRawXySwapped(const char* filename, bool work);
	static int SaveRawXySwapped(const char* filename, std::vector<float*> bits_to_swap, short unsigned width, short unsigned height, short unsigned nrslices);
	int SaveRawXzSwapped(const char* filename, bool work);
	static int SaveRawXzSwapped(const char* filename, std::vector<float*> bits_to_swap, short unsigned width, short unsigned height, short unsigned nrslices);
	int SaveRawYzSwapped(const char* filename, bool work);
	static int SaveRawYzSwapped(const char* filename, std::vector<float*> bits_to_swap, short unsigned width, short unsigned height, short unsigned nrslices);
	int SaveTissuesRaw(const char* filename);
	int SaveTissuesRawXySwapped(const char* filename);
	int SaveTissuesRawXzSwapped(const char* filename);
	int SaveTissuesRawYzSwapped(const char* filename);
	int ReloadDIBitmap(std::vector<const char*> filenames);
	int ReloadDIBitmap(std::vector<const char*> filenames, Point p);
	int ReloadDICOM(std::vector<const char*> lfilename);
	int ReloadDICOM(std::vector<const char*> lfilename, Point p);
	int ReloadRaw(const char* filename, unsigned bitdepth, unsigned short slicenr);
	int ReloadRaw(const char* filename, short unsigned w, short unsigned h, unsigned bitdepth, unsigned short slicenr, Point p);
	int ReloadRawFloat(const char* filename, unsigned short slicenr);
	int ReloadRawFloat(const char* filename, short unsigned w, short unsigned h, unsigned short slicenr, Point p);
	static std::vector<float*> LoadRawFloat(const char* filename, unsigned short startslice, unsigned short endslice, unsigned short slicenr, unsigned int area);
	int ReloadRawTissues(const char* filename, unsigned bitdepth, unsigned short slicenr);
	int ReloadRawTissues(const char* filename, short unsigned w, short unsigned h, unsigned bitdepth, unsigned short slicenr, Point p);
	int ReloadImage(const char* filename, unsigned short slicenr);
	int ReloadRTdose(const char* filename, unsigned short slicenr);
	int ReloadAVW(const char* filename, unsigned short slicenr);
	FILE* SaveHeader(FILE* fp, short unsigned nr_slices_to_write, Transform transform_to_write);
	FILE* SaveProject(const char* filename, const char* imageFileExtension);
	bool SaveCommunicationFile(const char* filename);
	FILE* SaveActiveSlices(const char* filename, const char* imageFileExtension);
	void LoadHeader(FILE* fp, int& tissuesVersion, int& version);
	FILE* MergeProjects(const char* savefilename, std::vector<QString>& mergeFilenames);
	FILE* LoadProject(const char* filename, int& tissuesVersion);
	bool LoadS4Llink(const char* filename, int& tissuesVersion);
	int SaveBmpRaw(const char* filename);
	int SaveWorkRaw(const char* filename);
	int SaveTissueRaw(const char* filename);
	int SaveBmpBitmap(const char* filename);
	int SaveWorkBitmap(const char* filename);
	int SaveTissueBitmap(const char* filename);
	void SaveContours(const char* filename);
	void ShiftContours(int dx, int dy);
	void SetextrusionContours(int top, int bottom);
	void ResetextrusionContours();
	FILE* SaveContourprologue(const char* filename, unsigned nr_slices);
	FILE* SaveContoursection(FILE* fp, unsigned startslice1, unsigned endslice1, unsigned offset);
	FILE* SaveTissuenamescolors(FILE* fp);
	void Work2bmp();
	void Bmp2work();
	void SwapBmpwork();
	void Work2bmpall();
	void Bmp2workall();
	void Work2tissueall();
	void SwapBmpworkall();

	std::vector<const float*> SourceSlices() const override;
	std::vector<float*> SourceSlices() override;
	std::vector<const float*> TargetSlices() const override;
	std::vector<float*> TargetSlices() override;
	std::vector<const tissues_size_t*> TissueSlices(tissuelayers_size_t layeridx) const override;
	std::vector<tissues_size_t*> TissueSlices(tissuelayers_size_t layeridx) override;

	std::vector<std::string> TissueNames() const override;
	std::vector<bool> TissueLocks() const override;
	std::vector<tissues_size_t> TissueSelection() const override;

	boost::signals2::signal<void(const std::vector<tissues_size_t>& sel)> m_OnTissueSelectionChanged;
	void SetTissueSelection(const std::vector<tissues_size_t>& sel) override;

	bool HasColors() const override { return m_ColorLookupTable != nullptr; }
	size_t NumberOfColors() const override;
	void GetColor(size_t, unsigned char& r, unsigned char& g, unsigned char& b) const override;

	void SetTargetFixedRange(bool on) override { SetModeall(on ? 2 : 1, false); }

	float* ReturnBmp(unsigned short slicenr1);
	float* ReturnWork(unsigned short slicenr1);
	tissues_size_t* ReturnTissues(tissuelayers_size_t layeridx, unsigned short slicenr1);
	float* ReturnOverlay();
	float GetWorkPt(Point p, unsigned short slicenr);
	void SetWorkPt(Point p, unsigned short slicenr, float f);
	float GetBmpPt(Point p, unsigned short slicenr);
	void SetBmpPt(Point p, unsigned short slicenr, float f);
	tissues_size_t GetTissuePt(Point p, unsigned short slicenr);
	void SetTissuePt(Point p, unsigned short slicenr, tissues_size_t f);
	unsigned int MakeHistogram(bool includeoutofrange);
	unsigned int ReturnArea();
	unsigned short Width() const override;
	unsigned short Height() const override;
	unsigned short NumSlices() const override;
	unsigned short StartSlice() const override;
	unsigned short EndSlice() const override;
	void SetStartslice(unsigned short startslice1);
	void SetEndslice(unsigned short endslice1);
	bool Isloaded() const;
	void Threshold(float* thresholds);
	void BmpSum();
	void BmpAdd(float f);
	void BmpDiff();
	void BmpMult();
	void BmpMult(float f);
	void BmpOverlay(float alpha);
	void BmpAbs();
	void BmpNeg();
	void ScaleColors(Pair p);
	void CropColors();
	void GetRange(Pair* pp);
	void ComputeRangeMode1(Pair* pp);
	void ComputeRangeMode1(unsigned short updateSlicenr, Pair* pp);
	void GetBmprange(Pair* pp);
	void ComputeBmprangeMode1(Pair* pp);
	void ComputeBmprangeMode1(unsigned short updateSlicenr, Pair* pp);
	void GetRangetissue(tissues_size_t* pp);
	void Gaussian(float sigma);
	void Average(unsigned short n);
	void MedianInterquartile(bool median);
	void AnisoDiff(float dt, int n, float (*f)(float, float), float k, float restraint);
	void ContAnisodiff(float dt, int n, float (*f)(float, float), float k, float restraint);
	void StepsmoothZ(unsigned short n);
	void SmoothTissues(unsigned short n);
	void Sigmafilter(float sigma, unsigned short nx, unsigned short ny);
	void Hysteretic(float thresh_low, float thresh_high, bool connectivity, unsigned short nrpasses);
	void DoubleHysteretic(float thresh_low_l, float thresh_low_h, float thresh_high_l, float thresh_high_h, bool connectivity, unsigned short nrpasses);
	void DoubleHystereticAllslices(float thresh_low_l, float thresh_low_h, float thresh_high_l, float thresh_high_h, bool connectivity, float set_to);
	void ThresholdedGrowing(short unsigned slicenr, Point p, float threshfactor_low, float threshfactor_high, bool connectivity, unsigned short nrpasses);
	void ThresholdedGrowing(short unsigned slicenr, Point p, float thresh_low, float thresh_high, float set_to); //bool connectivity,float set_to);
	void AddMark(Point p, unsigned label, std::string str = "");
	void AddMark(Point p, unsigned label, unsigned short slicenr, std::string str = "");
	bool RemoveMark(Point p, unsigned radius);
	bool RemoveMark(Point p, unsigned radius, unsigned short slicenr);
	void ClearMarks();
	void GetLabels(std::vector<AugmentedMark>* labels);
	void AddVm(std::vector<Mark>* vm1);
	bool RemoveVm(Point p, unsigned radius);
	void AddVm(std::vector<Mark>* vm1, unsigned short slicenr);
	bool RemoveVm(Point p, unsigned radius, unsigned short slicenr);
	void AddLimit(std::vector<Point>* vp1);
	bool RemoveLimit(Point p, unsigned radius);
	void AddLimit(std::vector<Point>* vp1, unsigned short slicenr);
	bool RemoveLimit(Point p, unsigned radius, unsigned short slicenr);
	void ZeroCrossings(bool connectivity);
	void DougpeuckLine(float epsilon);
	void Kmeans(unsigned short slicenr, short nrtissues, unsigned int iternr, unsigned int converge);
	void KmeansMhd(unsigned short slicenr, short nrtissues, short dim, std::vector<std::string> mhdfiles, float* weights, unsigned int iternr, unsigned int converge);
	void KmeansPng(unsigned short slicenr, short nrtissues, short dim, std::vector<std::string> pngfiles, std::vector<int> exctractChannel, float* weights, unsigned int iternr, unsigned int converge, const std::string initCentersFile = "");
	void Em(unsigned short slicenr, short nrtissues, unsigned int iternr, unsigned int converge);
	void ExtractContours(int minsize, std::vector<tissues_size_t>& tissuevec);
	void ExtractContours2Xmirrored(int minsize, std::vector<tissues_size_t>& tissuevec);
	void ExtractContours2Xmirrored(int minsize, std::vector<tissues_size_t>& tissuevec, float epsilon);
	void ExtractinterpolatesaveContours(int minsize, std::vector<tissues_size_t>& tissuevec, unsigned short between, bool dp, float epsilon, const char* filename);
	void ExtractinterpolatesaveContours2Xmirrored(int minsize, std::vector<tissues_size_t>& tissuevec, unsigned short between, bool dp, float epsilon, const char* filename);
	void Add2tissue(tissues_size_t tissuetype, Point p, bool override);
	void Add2tissueall(tissues_size_t tissuetype, Point p, bool override);
	void Add2tissueall(tissues_size_t tissuetype, Point p, unsigned short slicenr, bool override);
	void Add2tissueall(tissues_size_t tissuetype, float f, bool override);
	void Add2tissueConnected(tissues_size_t tissuetype, Point p, bool override);
	void Add2tissueallConnected(tissues_size_t tissuetype, Point p, bool override);
	void Add2tissueThresh(tissues_size_t tissuetype, Point p);
	void Add2tissue(tissues_size_t tissuetype, bool* mask, unsigned short slicenr, bool override);
	void SubtractTissue(tissues_size_t tissuetype, Point p);
	void SubtractTissueall(tissues_size_t tissuetype, Point p);
	void SubtractTissueall(tissues_size_t tissuetype, Point p, unsigned short slicenr);
	void SubtractTissueall(tissues_size_t tissuetype, float f);
	void SubtractTissueConnected(tissues_size_t tissuetype, Point p);
	void SubtractTissueallConnected(tissues_size_t tissuetype, Point p);
	void Tissue2workall3D();
	void Tissue2workall();
	void Selectedtissue2work(const std::vector<tissues_size_t>& tissuetype);
	void Selectedtissue2work3D(const std::vector<tissues_size_t>& tissuetype);
	void Cleartissue(tissues_size_t tissuetype);
	void Cleartissue3D(tissues_size_t tissuetype);
	void Cleartissues();
	void Cleartissues3D();
	void Interpolatetissue(unsigned short slice1, unsigned short slice2, tissues_size_t tissuetype, bool connected);
	void InterpolatetissueMedianset(unsigned short slice1, unsigned short slice2, tissues_size_t tissuetype, bool connectivity, bool handleVanishingComp = true);
	void Extrapolatetissue(unsigned short origin1, unsigned short origin2, unsigned short target, tissues_size_t tissuetype);
	void Interpolatework(unsigned short slice1, unsigned short slice2);
	void Extrapolatework(unsigned short origin1, unsigned short origin2, unsigned short target);
	void Interpolatetissuegrey(unsigned short slice1, unsigned short slice2);
	void InterpolatetissuegreyMedianset(unsigned short slice1, unsigned short slice2, bool connectivity, bool handleVanishingComp = true);
	void Interpolateworkgrey(unsigned short slice1, unsigned short slice2, bool connected);
	void InterpolateworkgreyMedianset(unsigned short slice1, unsigned short slice2, bool connectivity, bool handleVanishingComp = true);
	void Interpolate(unsigned short slice1, unsigned short slice2);
	void Extrapolate(unsigned short origin1, unsigned short origin2, unsigned short target);
	void Interpolate(unsigned short slice1, unsigned short slice2, float* bmp1, float* bmp2);

	bool ComputeTargetConnectivity(ProgressInfo* progress = nullptr);
	bool ComputeSplitTissues(tissues_size_t tissue, ProgressInfo* progress = nullptr);

	void SetSlicethickness(float t);
	float GetSlicethickness() const;
	void SetPixelsize(float dx1, float dy1);
	Pair GetPixelsize() const;
	void SlicebmpX(float* return_bits, unsigned short xcoord);
	float* SlicebmpX(unsigned short xcoord);
	void SlicebmpY(float* return_bits, unsigned short ycoord);
	float* SlicebmpY(unsigned short ycoord);
	void SliceworkX(float* return_bits, unsigned short xcoord);
	float* SliceworkX(unsigned short xcoord);
	void SliceworkY(float* return_bits, unsigned short ycoord);
	float* SliceworkY(unsigned short ycoord);
	void SlicetissueX(tissues_size_t* return_bits, unsigned short xcoord);
	tissues_size_t* SlicetissueX(unsigned short xcoord);
	void SlicetissueY(tissues_size_t* return_bits, unsigned short ycoord);
	tissues_size_t* SlicetissueY(unsigned short ycoord);
	void SliceworkZ(unsigned short slicenr);
	int ExtractTissueSurfaces(const std::string& filename, std::vector<tissues_size_t>& tissuevec, bool usediscretemc = false, float ratio = 1.0f, unsigned smoothingiterations = 15, float passBand = 0.1f, float featureAngle = 180);
	void NextSlice();
	void PrevSlice();
	unsigned short GetNextFeaturingSlice(tissues_size_t type, bool& found);

	unsigned short ActiveSlice() const override;
	boost::signals2::signal<void(unsigned short)> m_OnActiveSliceChanged;
	void SetActiveSlice(unsigned short slice, bool signal_change = false) override;

	Bmphandler* GetActivebmphandler();
	tissuelayers_size_t ActiveTissuelayer() const override;
	void SetActiveTissuelayer(tissuelayers_size_t idx);
	unsigned PushstackBmp();
	unsigned PushstackBmp(unsigned int slice);
	unsigned PushstackWork();
	unsigned PushstackWork(unsigned int slice);
	unsigned PushstackTissue(tissues_size_t i);
	unsigned PushstackTissue(tissues_size_t i, unsigned int slice);
	unsigned PushstackHelp();
	void Removestack(unsigned i);
	void ClearStack();
	float* Getstack(unsigned i, unsigned char& mode);
	void GetstackBmp(unsigned i);
	void GetstackBmp(unsigned int slice, unsigned i);
	void GetstackWork(unsigned i);
	void GetstackWork(unsigned int slice, unsigned i);
	void GetstackHelp(unsigned i);
	void GetstackTissue(unsigned i, tissues_size_t tissuenr, bool override);
	void GetstackTissue(unsigned int slice, unsigned i, tissues_size_t tissuenr, bool override);
	void PopstackBmp();
	void PopstackWork();
	void PopstackHelp();
	unsigned Loadstack(const char* filename);
	void Savestack(unsigned i, const char* filename);
	void FillHoles(float f, int minsize);
	void RemoveIslands(float f, int minsize);
	void FillGaps(int minsize, bool connectivity);
	void FillHolestissue(tissues_size_t f, int minsize);
	void RemoveIslandstissue(tissues_size_t f, int minsize);
	void FillGapstissue(int minsize, bool connectivity);
	float AddSkin(int i);
	void AddSkintissue(int i1, tissues_size_t f);
	float AddSkin3D(int i);
	float AddSkin3D(int ix, int iy, int iz);
	void AddSkin3D(int ix, int iy, int iz, float setto);
	void AddSkintissue3D(int ix, int iy, int iz, tissues_size_t f);
	float AddSkinOutside(int i);
	void AddSkintissueOutside(int i1, tissues_size_t f);
	float AddSkin3DOutside(int i);
	void AddSkin3DOutside(int ix, int iy, int iz, float setto);
	float AddSkin3DOutside2(int ix, int iy, int iz);
	void AddSkin3DOutside2(int ix, int iy, int iz, float setto);
	void AddSkintissue3DOutside(int ixyz, tissues_size_t f);
	void AddSkintissue3DOutside2(int ix, int iy, int iz, tissues_size_t f);
	bool ValueAtBoundary3D(float value);
	bool TissuevalueAtBoundary3D(tissues_size_t value);
	void FillSkin3d(int thicknessX, int thicknessY, int thicknessZ, tissues_size_t backgroundID, tissues_size_t skinID);
	void GammaMhd(unsigned short slicenr, short nrtissues, short dim, std::vector<std::string> mhdfiles, float* weights, float** centers, float* tol_f, float* tol_d);
	void FillUnassigned();
	void FillUnassignedtissue(tissues_size_t f);
	void StartUndo(DataSelection& dataSelection);
	bool StartUndoall(DataSelection& dataSelection);
	bool StartUndo(DataSelection& dataSelection, std::vector<unsigned> vslicenr1);
	void AbortUndo();
	void EndUndo();
	void MergeUndo();
	DataSelection Undo();
	DataSelection Redo();
	void ClearUndo();
	void ReverseUndosliceorder();
	unsigned ReturnNrundo();
	unsigned ReturnNrredo();
	bool ReturnUndo3D() const;
	unsigned ReturnNrundosteps();
	unsigned ReturnNrundoarrays();
	void SetUndo3D(bool undo3D1);
	void SetUndonr(unsigned nr);
	void SetUndoarraynr(unsigned nr);
	void MaskSource(bool all_slices, float maskvalue);
	void MapTissueIndices(const std::vector<tissues_size_t>& indexMap);
	void RemoveTissue(tissues_size_t tissuenr);
	void RemoveTissues(const std::set<tissues_size_t>& tissuenrs);
	void RemoveTissueall();
	std::vector<tissues_size_t> FindUnusedTissues();
	void CapTissue(tissues_size_t maxval);
	void Buildmissingtissues(tissues_size_t j);
	void GroupTissues(std::vector<tissues_size_t>& olds, std::vector<tissues_size_t>& news);
	void GetDICOMseriesnr(std::vector<const char*>* vnames, std::vector<unsigned>* dicomseriesnr, std::vector<unsigned>* dicomseriesnrlist);
	void SetModeall(unsigned char mode, bool bmporwork);
	bool PrintAmascii(const char* filename);
	bool PrintTissuemat(const char* filename);
	bool PrintBmpmat(const char* filename);
	bool PrintWorkmat(const char* filename);

	bool ExportTissue(const char* filename, bool binary) const;
	bool ExportBmp(const char* filename, bool binary) const;
	bool ExportWork(const char* filename, bool binary) const;

	bool PrintXmlregionextent(const char* filename, bool onlyactiveslices, const char* projname = nullptr);
	bool PrintTissueindex(const char* filename, bool onlyactiveslices, const char* projname = nullptr);
	bool PrintAtlas(const char* filename);
	vtkImageData* MakeVtktissueimage();
	float CalculateVolume(Point p, unsigned short slicenr);
	float CalculateTissuevolume(Point p, unsigned short slicenr);
	void Inversesliceorder();

	Vec3 Spacing() const override;
	Transform ImageTransform() const override;
	Transform GetTransformActiveSlices() const;
	void SetTransform(const Transform& tr);
	void GetDisplacement(float disp[3]) const;
	void SetDisplacement(const float disp[3]);
	void GetDirectionCosines(float dc[6]) const;
	void SetDirectionCosines(const float dc[6]);

	bool GetExtent(tissues_size_t tissuenr, bool onlyactiveslices, unsigned short extent[3][2]);
	void Regrow(unsigned short sourceslicenr, unsigned short targetslicenr, int n);
	bool Unwrap(float jumpratio, float shift = 0);
	unsigned GetNumberOfUndoSteps();
	void SetNumberOfUndoSteps(unsigned);
	unsigned GetNumberOfUndoArrays();
	void SetNumberOfUndoArrays(unsigned);
	int GetCompression() const { return this->m_Hdf5Compression; }
	void SetCompression(int c) { this->m_Hdf5Compression = c; }
	bool GetContiguousMemory() const { return m_ContiguousMemoryIo; }
	void SetContiguousMemory(bool v) { m_ContiguousMemoryIo = v; }
	bool SaveTarget() const { return m_SaveTarget; }
	void SetSaveTarget(bool v) { m_SaveTarget = v; }

	int SaveRaw(const char* filename, bool work);
	float DICOMsort(std::vector<const char*>* lfilename);

	TissueHiearchy* GetTissueHierachy() { return m_TissueHierachy; }

	void Mergetissues(tissues_size_t tissuetype);

private:
	unsigned short m_Activeslice;
	std::vector<Bmphandler> m_ImageSlices;
	short unsigned m_Width;
	short unsigned m_Height;
	short unsigned m_Startslice;
	short unsigned m_Endslice;
	short unsigned m_Nrslices;
	unsigned int m_Area;
	float m_Thickness;
	float m_Dx;
	float m_Dy;
	Transform m_Transform;
	tissuelayers_size_t m_ActiveTissuelayer;
	std::shared_ptr<ColorLookupTable> m_ColorLookupTable;
	TissueHiearchy* m_TissueHierachy;
	float* m_Overlay;
	std::vector<Pair> m_SliceRanges;
	std::vector<Pair> m_SliceBmpranges;
	OutlineSlices m_Os;

	bool m_Loaded;
	UndoElem* m_Uelem;
	UndoQueue m_UndoQueue;
	bool m_Undo3D;
	int m_Hdf5Compression = 1;
	bool m_ContiguousMemoryIo = false; // Default: slice-by-slice
	bool m_SaveTarget = false;
};

} // namespace iseg
