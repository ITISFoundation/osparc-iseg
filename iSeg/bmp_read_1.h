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

#include "Data/Mark.h"
#include "Data/Types.h"

#include "Core/Contour.h"
#include "Core/FeatureExtractor.h"
#include "Core/Pair.h"

#include <list>
#include <set>
#include <vector>

namespace iseg {

class ImageForestingTransformRegionGrowing;
class ImageForestingTransformLivewire;
class ImageForestingTransformFastMarching;
class SliceProvider;
class SliceProviderInstaller;

const unsigned int unvisited = 222222;
const float f_tol = 0.00001f;

struct Basin
{
	unsigned m_G;
	unsigned m_R;
	unsigned m_L;
};

struct MergeEvent
{
	unsigned m_K;
	unsigned m_A;
	unsigned m_G;
};

struct WshedObj
{
	std::vector<Basin> m_B;
	std::vector<MergeEvent> m_M;
	std::vector<Point> m_Marks;
};

class Bmphandler
{
public:
	Bmphandler();
	Bmphandler(const Bmphandler& object);
	~Bmphandler();

	float* CopyWork();
	float* CopyBmp();
	tissues_size_t* CopyTissue(tissuelayers_size_t idx);
	void CopyWork(float* output);
	void CopyBmp(float* output);
	void CopyTissue(tissuelayers_size_t idx, tissues_size_t* output);
	float* ReturnBmp();
	const float* ReturnBmp() const;
	float* ReturnWork();
	const float* ReturnWork() const;
	tissues_size_t* ReturnTissues(tissuelayers_size_t idx);
	const tissues_size_t* ReturnTissues(tissuelayers_size_t idx) const;
	float* ReturnHelp();
	float** ReturnBmpfield();
	float** ReturnWorkfield();
	tissues_size_t** ReturnTissuefield(tissuelayers_size_t idx);

	std::vector<Mark>* ReturnMarks();
	void Copy2marks(std::vector<Mark>* marks1);
	void GetAddLabels(std::vector<Mark>* labels);
	void GetLabels(std::vector<Mark>* labels);
	void ClearTissue(tissuelayers_size_t idx);
	bool HasTissue(tissuelayers_size_t idx, tissues_size_t tissuetype);
	void Add2tissue(tissuelayers_size_t idx, tissues_size_t tissuetype, Point p, bool override);
	void Add2tissueConnected(tissuelayers_size_t idx, tissues_size_t tissuetype, Point p, bool override);
	void Add2tissue(tissuelayers_size_t idx, tissues_size_t tissuetype, float f, bool override);
	void Add2tissue(tissuelayers_size_t idx, tissues_size_t tissuetype, bool* mask, bool override);
	void Add2tissueThresh(tissuelayers_size_t idx, tissues_size_t tissuetype, Point p);
	void SubtractTissue(tissuelayers_size_t idx, tissues_size_t tissuetype, Point p);
	void SubtractTissueConnected(tissuelayers_size_t idx, tissues_size_t tissuetype, Point p);
	void SubtractTissue(tissuelayers_size_t idx, tissues_size_t tissuetype, float f);
	void Change2maskConnectedtissue(tissuelayers_size_t idx, bool* mask, Point p, bool addorsub);
	void Change2maskConnectedwork(bool* mask, Point p, bool addorsub);
	void Tissue2work(tissuelayers_size_t idx, const std::vector<float>& mask);
	void Tissue2work(tissuelayers_size_t idx);
	void Work2tissue(tissuelayers_size_t idx);
	void Cleartissue(tissuelayers_size_t idx, tissues_size_t tissuetype);
	void Cleartissues(tissuelayers_size_t idx);
	void Cleartissuesall();
	void CapTissue(tissues_size_t maxval);
	void SetBmp(float* bits, unsigned char mode);
	void SetWork(float* bits, unsigned char mode);
	void SetTissue(tissuelayers_size_t idx, tissues_size_t* bits);
	float* SwapBmpPointer(float* bits);
	float* SwapWorkPointer(float* bits);
	tissues_size_t* SwapTissuesPointer(tissuelayers_size_t idx, tissues_size_t* bits);
	void Copy2bmp(float* bits, unsigned char mode);
	void Copy2work(float* bits, unsigned char mode);
	void Copy2work(float* bits, bool* mask, unsigned char mode);
	void Copy2tissue(tissuelayers_size_t idx, tissues_size_t* bits);
	void Copy2tissue(tissuelayers_size_t idx, tissues_size_t* bits, bool* mask);
	void Copyfrombmp(float* bits);
	void Copyfromwork(float* bits);
	void Copyfromtissue(tissuelayers_size_t idx, tissues_size_t* bits);
#ifdef TISSUES_SIZE_TYPEDEF
	void Copyfromtissue(tissuelayers_size_t idx, unsigned char* bits);
#endif // TISSUES_SIZE_TYPEDEF
	void Copyfromtissuepadded(tissuelayers_size_t idx, tissues_size_t* bits, unsigned short padding);
	void ClearBmp();
	void ClearWork();
	void BmpSum();
	void BmpAdd(float f);
	void BmpDiff();
	void BmpMult();
	void BmpMult(float f);
	void BmpAbs();
	void BmpNeg();
	void BmpOverlay(float alpha);
	void TransparentAdd(float* pict2);
	void Newbmp(unsigned short width1, unsigned short height1, bool init = true);
	void Newbmp(unsigned short width1, unsigned short height1, float* bits);
	void Freebmp();
	static int CheckBMPDepth(const char* filename);
	void SetConverterFactors(int redFactor, int greenFactor, int blueFactor);
	int LoadDIBitmap(const char* filename);
	int LoadDIBitmap(const char* filename, Point p, unsigned short dx, unsigned short dy);
	int ReloadDIBitmap(const char* filename);
	int ReloadDIBitmap(const char* filename, Point p);
	static int CheckPNGDepth(const char* filename);
	int LoadPNGBitmap(const char* filename);
	bool LoadArray(float* bits, unsigned short w1, unsigned short h1);
	bool LoadArray(float* bits, unsigned short w1, unsigned short h1, Point p, unsigned short dx, unsigned short dy);
	bool ReloadArray(float* bits);
	bool ReloadArray(float* bits, unsigned short w1, unsigned short h1, Point p);
	bool LoadDICOM(const char* filename);
	bool LoadDICOM(const char* filename, Point p, unsigned short dx, unsigned short dy);
	bool ReloadDICOM(const char* filename);
	bool ReloadDICOM(const char* filename, Point p);
	FILE* SaveProj(FILE* fp, bool inclpics = true);
	FILE* SaveStack(FILE* fp) const;
	FILE* LoadProj(FILE* fp, int tissuesVersion, bool inclpics = true, bool init = true);
	FILE* LoadStack(FILE* fp);
	int SaveDIBitmap(const char* filename);
	int SaveWorkBitmap(const char* filename);
	int SaveTissueBitmap(tissuelayers_size_t idx, const char* filename);
	int ReadRaw(const char* filename, short unsigned w, short unsigned h, unsigned bitdepth, unsigned short slicenr);
	int ReadRaw(const char* filename, short unsigned w, short unsigned h, unsigned bitdepth, unsigned short slicenr, Point p, unsigned short dx, unsigned short dy);
	int ReloadRaw(const char* filename, unsigned bitdepth, unsigned slicenr);
	int ReloadRaw(const char* filename, short unsigned w, short unsigned h, unsigned bitdepth, unsigned slicenr, Point p);
	int ReadRawFloat(const char* filename, short unsigned w, short unsigned h, unsigned short slicenr);
	int ReadRawFloat(const char* filename, short unsigned w, short unsigned h, unsigned short slicenr, Point p, unsigned short dx, unsigned short dy);
	int ReloadRawFloat(const char* filename, unsigned slicenr);
	static float* ReadRawFloat(const char* filename, unsigned slicenr, unsigned int area);
	int ReloadRawFloat(const char* filename, short unsigned w, short unsigned h, unsigned slicenr, Point p);
	int ReloadRawTissues(const char* filename, unsigned bitdepth, unsigned slicenr);
	int ReloadRawTissues(const char* filename, short unsigned w, short unsigned h, unsigned bitdepth, unsigned slicenr, Point p);
	int SaveBmpRaw(const char* filename);
	int SaveWorkRaw(const char* filename);
	int SaveTissueRaw(tissuelayers_size_t idx, const char* filename);
	int ReadAvw(const char* filename, short unsigned i);
	void Work2bmp();
	void Bmp2work();
	void SwapBmpwork();
	void SwapBmphelp();
	void SwapWorkhelp();
	unsigned int MakeHistogram(bool includeoutofrange);
	unsigned int MakeHistogram(float* mask, float f, bool includeoutofrange);
	unsigned int MakeHistogram(Point p, unsigned short dx, unsigned short dy, bool includeoutofrange);
	unsigned int* ReturnHistogram();
	float* FindModal(unsigned int thresh1, float thresh2);
	void PrintInfo() const;
	void PrintHistogram();
	unsigned int ReturnArea() const;
	unsigned int ReturnWidth() const;
	unsigned int ReturnHeight() const;
	void Threshold(float* thresholds);
	void Threshold(float* thresholds, Point p, unsigned short dx, unsigned short dy);
	void Convolute(float* mask, unsigned short direction); // x: 0, y: 1, xy: 2
	void ScaleColors(Pair p);
	void CropColors();
	void GetRange(Pair* pp);
	void GetRangetissue(tissuelayers_size_t idx, tissues_size_t* pp);
	void GetBmprange(Pair* pp);
	void Gaussian(float sigma);
	void Average(unsigned short n);
	void Laplacian();
	void Laplacian1();
	void Sobel();
	void SobelFiner();
	void Sobelxy(float** sobelx, float** sobely);
	void MedianInterquartile(bool median);
	void MedianInterquartile(float* median, float* iq);
	void Sigmafilter(float sigma, unsigned short nx, unsigned short ny);
	void Compacthist();
	void MomentLine();
	void GaussLine(float sigma);
	void GaussSharpen(float sigma);
	void CannyLine(float sigma, float thresh_low, float thresh_high);
	void Subthreshold(int n1, int n2, unsigned int thresh1, float thresh2, float sigma);
	void ConvoluteHist(float* mask);
	void GaussianHist(float sigma);
	float* DirectionMap(float* sobelx, float* sobely);
	void NonmaximumSupr(float* direct_map);
	void Hysteretic(float thresh_low, float thresh_high, bool connectivity, float set_to);
	void Hysteretic(float thresh_low, float thresh_high, bool connectivity, float* mask, float f, float set_to);
	void DoubleHysteretic(float thresh_low_l, float thresh_low_h, float thresh_high_l, float thresh_high_h, bool connectivity, float set_to);
	void DoubleHysteretic(float thresh_low_l, float thresh_low_h, float thresh_high_l, float thresh_high_h, bool connectivity, float* mask, float f, float set_to);
	void ThresholdedGrowing(Point p, float thresh_low, float thresh_high, bool connectivity, float set_to);
	void ThresholdedGrowing(Point p, float thresh_low, float thresh_high, bool connectivity, float set_to, std::vector<Point>* limits1);
	void ThresholdedGrowinglimit(Point p, float thresh_low, float thresh_high, bool connectivity, float set_to);
	void ThresholdedGrowing(Point p, float threshfactor_low, float threshfactor_high, bool connectivity, float set_to, Pair* tp);
	void ThresholdedGrowing(float thresh_low, float thresh_high, bool connectivity, float* mask, float f, float set_to);
	void DistanceMap(bool connectivity);
	void DistanceMap(bool connectivity, float f, short unsigned levlset); //0:outside,1:inside,2:both
	void DeadReckoning();
	unsigned* DeadReckoning(float f);
	unsigned* DeadReckoningSquared(float f);
	void IftDistance1(float f);
	void Erosion(int n, bool connectivity);
	void Erosion1(int n, bool connectivity);
	void Dilation(int n, bool connectivity);
	void Closure(int n, bool connectivity);
	void Open(int n, bool connectivity);
	void Erasework(bool* mask);
	void Erasetissue(tissuelayers_size_t idx, bool* mask);
	void Floodwork(bool* mask);
	void Floodtissue(tissuelayers_size_t idx, bool* mask);
	void MarkBorder(bool connectivity);
	void AnisoDiff(float dt, int n, float (*f)(float, float), float k, float restraint);
	void ContAnisodiff(float dt, int n, float (*f)(float, float), float k, float restraint);
	unsigned* Watershed(bool connectivity);
	unsigned* WatershedSobel(bool connectivity);
	void ConstructRegions(unsigned h, unsigned* wshed);
	void AddMark(Point p, unsigned label, std::string str = "");
	bool RemoveMark(Point p, unsigned radius);
	void ClearMarks();
	void NMoment(short unsigned n, short unsigned p);
	void NMomentSigma(short unsigned n, short unsigned p, float sigma);
	void ZeroCrossings(bool connectivity);
	void ZeroCrossings(float thresh, bool connectivity);
	void LaplacianZero(float sigma, float thresh, bool connectivity);
	void ToBmpgrey(float* p_bits) const;
	void ConnectedComponents(bool connectivity);
	void ConnectedComponents(bool connectivity, std::set<float>& components);
	void FillGaps(short unsigned n, bool connectivity);
	void FillGapstissue(tissuelayers_size_t idx, short unsigned n, bool connectivity);
	void Wshed2work(unsigned* Y);
	void Labels2work(unsigned* Y, unsigned lnr);
	//		change_island, remove_island s. 3D-slicer
	//		registration: manual, mutual information s. 3D-slicer
	void GetTissuecontours(tissuelayers_size_t idx, tissues_size_t f, std::vector<std::vector<Point>>* outer_line, std::vector<std::vector<Point>>* inner_line, int minsize);
	void GetTissuecontours2Xmirrored(tissuelayers_size_t idx, tissues_size_t f, std::vector<std::vector<Point>>* outer_line, std::vector<std::vector<Point>>* inner_line, int minsize);
	void GetTissuecontours2Xmirrored(tissuelayers_size_t idx, tissues_size_t f, std::vector<std::vector<Point>>* outer_line, std::vector<std::vector<Point>>* inner_line, int minsize, float disttol);
	void GetContours(float f, std::vector<std::vector<Point>>* outer_line, std::vector<std::vector<Point>>* inner_line, int minsize);
	void GetContours(Point p, std::vector<std::vector<Point>>* outer_line, std::vector<std::vector<Point>>* inner_line, int minsize);
	void LoadLine(std::vector<Point>* vec_pt);
	void PresimplifyLine(float d);
	void DougpeuckLine(float epsilon);
	void PlotLine();
	float BmpPt(Point p);
	float WorkPt(Point p);
	tissues_size_t TissuesPt(tissuelayers_size_t idx, Point p);
	void SetWorkPt(Point p, float f);
	void SetBmpPt(Point p, float f);
	void SetTissuePt(tissuelayers_size_t idx, Point p, tissues_size_t f);
	void RgIft(float* lb_map, float thresh);
	ImageForestingTransformRegionGrowing* IfTrgInit(float* lb_map);
	ImageForestingTransformLivewire* Livewireinit(Point p);
	void FillContour(std::vector<Point>* vp, bool continuous);
	void AdaptiveFuzzy(Point p, float m1, float s1, float s2, float thresh);
	void FastMarching(Point p, float sigma, float thresh);
	ImageForestingTransformFastMarching* FastmarchingInit(Point p, float sigma, float thresh);
	void Classify(short nrclasses, short dim, float** bits, float* weights, float* centers, float maxdist);
	void Em(short nrtissues, short dim, float** bits, float* weights, unsigned int iternr, unsigned int converge);
	void Kmeans(short nrtissues, short dim, float** bits, float* weights, unsigned int iternr, unsigned int converge);
	void KmeansMhd(short nrtissues, short dim, std::vector<std::string> mhdfiles, unsigned short slicenr, float* weights, unsigned int iternr, unsigned int converge);
	void KmeansPng(short nrtissues, short dim, std::vector<std::string> pngfiles, std::vector<int> exctractChannel, unsigned short slicenr, float* weights, unsigned int iternr, unsigned int converge, const std::string initCentersFile = "");
	void GammaMhd(short nrtissues, short dim, std::vector<std::string> mhdfiles, unsigned short slicenr, float* weights, float** centers, float* tol_f, float* tol_d, Pair pixelsize);
	void Cannylevelset(float* initlev, float f, float sigma, float thresh_low, float thresh_high, float epsilon, float stepsize, unsigned nrsteps, unsigned reinitfreq);
	void Threshlevelset(float thresh_low, float thresh_high, float epsilon, float stepsize, unsigned nrsteps, unsigned reinitfreq);
	float ExtractFeature(Point p1, Point p2);
	float ExtractFeaturework(Point p1, Point p2);
	float ReturnStdev();
	Pair ReturnExtrema();
	unsigned PushstackBmp();
	unsigned PushstackWork();
	unsigned PushstackHelp();
	unsigned PushstackTissue(tissuelayers_size_t idx, tissues_size_t tissuenr);
	bool Savestack(unsigned i, const char* filename);
	unsigned Loadstack(const char* filename);
	void Removestack(unsigned i);
	float* Getstack(unsigned i, unsigned char& mode);
	void GetstackBmp(unsigned i);
	void GetstackWork(unsigned i);
	void GetstackHelp(unsigned i);
	void GetstackTissue(tissuelayers_size_t idx, unsigned i, tissues_size_t tissuenr, bool override);
	void PopstackBmp();
	void PopstackWork();
	void PopstackHelp();
	void ClearStack();
	bool Isloaded() const;
	void CorrectOutline(float f, std::vector<Point>* newline);
	void CorrectOutlinetissue(tissuelayers_size_t idx, tissues_size_t f1, std::vector<Point>* newline);
	void Brush(float f, Point p, int radius, bool draw);
	void Brush(float f, Point p, float radius, float dx, float dy, bool draw);
	void Brushtissue(tissuelayers_size_t idx, tissues_size_t f, Point p, int radius, bool draw, tissues_size_t f1);
	void Brushtissue(tissuelayers_size_t idx, tissues_size_t f, Point p, float radius, float dx, float dy, bool draw, tissues_size_t f1);
	void FillHoles(float f, int minsize);
	void FillHolestissue(tissuelayers_size_t idx, tissues_size_t f, int minsize);
	void RemoveIslands(float f, int minsize);
	void RemoveIslandstissue(tissuelayers_size_t idx, tissues_size_t f, int minsize);
	float AddSkin(unsigned i);
	void AddSkin(unsigned i, float setto);
	void AddSkintissue(tissuelayers_size_t idx, unsigned i4, tissues_size_t setto);
	float AddSkinOutside(unsigned i);
	void AddSkinOutside(unsigned i, float setto);
	void AddSkintissueOutside(tissuelayers_size_t idx, unsigned i4, tissues_size_t setto);
	bool ValueAtBoundary(float value);
	bool TissuevalueAtBoundary(tissuelayers_size_t idx, tissues_size_t value);
	void FillSkin(int thicknessX, int thicknessY, tissues_size_t backgroundID, tissues_size_t skinID);
	void FloodExterior(float setto);
	void FloodExteriortissue(tissuelayers_size_t idx, tissues_size_t setto);
	void FillUnassigned(float setto);
	void FillUnassigned();
	void FillUnassignedtissue(tissuelayers_size_t idx, tissues_size_t setto);
	void AddVm(std::vector<Mark>* vm1);
	void ClearVvm();
	bool DelVm(Point p, short radius);
	std::vector<std::vector<Mark>>* ReturnVvm();
	unsigned ReturnVvmmaxim() const;
	void Copy2vvm(std::vector<std::vector<Mark>>* vvm1);
	void AddLimit(std::vector<Point>* vp1);
	void ClearLimits();
	bool DelLimit(Point p, short radius);
	std::vector<std::vector<Point>>* ReturnLimits();
	void Copy2limits(std::vector<std::vector<Point>>* limits1);
	void MapTissueIndices(const std::vector<tissues_size_t>& indexMap);
	void RemoveTissue(tissues_size_t tissuenr);
	void GroupTissues(tissuelayers_size_t idx, std::vector<tissues_size_t>& olds, std::vector<tissues_size_t>& news);
	unsigned char ReturnMode(bool bmporwork) const;
	void SetMode(unsigned char mode, bool bmporwork);
	bool PrintAmasciiSlice(tissuelayers_size_t idx, std::ofstream& streamname);
	bool PrintVtkasciiSlice(tissuelayers_size_t idx, std::ofstream& streamname);
	bool PrintVtkbinarySlice(tissuelayers_size_t idx, std::ofstream& streamname);
	void Shifttissue();
	void Shiftbmp();
	unsigned long ReturnWorkpixelcount(float f);
	unsigned long ReturnTissuepixelcount(tissuelayers_size_t idx, tissues_size_t c);
	void Swap(Bmphandler& bmph);
	bool GetExtent(tissuelayers_size_t idx, tissues_size_t tissuenr, unsigned short extent[2][2]);
	bool Unwrap(float jumpratio, float range = 0, float shift = 0);

	int ConvertImageTo8BitBMP(const char* filename, unsigned char*& bits_tmp) const;
	int ConvertPNGImageTo8BitBMP(const char* filename, unsigned char*& bits_tmp) const;
	void SetRGBtoGrayScaleFactors(double newRedFactor, double newGreenFactor, double newBlueFactor);
	void Mergetissue(tissues_size_t tissuetype, tissuelayers_size_t idx);

protected:
	static std::list<unsigned> stackindex;
	static unsigned stackcounter;
	Contour m_Contour;
	std::vector<Mark> m_Marks;
	short unsigned m_Width;
	short unsigned m_Height;
	unsigned int m_Area;
	inline unsigned int Pt2coord(Point p) const;
	float* MakeGaussfilter(float sigma, int n);
	float* MakeLaplacianfilter();
	unsigned DeepestConBas(unsigned k);
	void CondMerge(unsigned m, unsigned h);
	unsigned LabelLookup(unsigned i, unsigned* wshed);
	int SaveDIBitmap(const char* filename, float* p_bits) const;
	int SaveRaw(const char* filename, float* p_bits) const;
	void Bucketsort(std::vector<unsigned int>* sorted, float* p_bits) const;
	void SetMarker(unsigned* wshed);
	float BaseConnection(float c, std::vector<float>* maps);
	void HystereticGrowth(float* pict, std::vector<int>* s, unsigned short w, unsigned short h, bool connectivity, float set_to);
	void HystereticGrowth(float* pict, std::vector<int>* s, unsigned short w, unsigned short h, bool connectivity, float set_to, int nr);
	template<typename T, typename F>
	void Brush(T* data, T f, Point p, int radius, bool draw, T f1, F);
	template<typename T, typename F>
	void Brush(T* data, T f, Point p, float radius, float dx, float dy, bool draw, T f1, F);

private:
	unsigned int m_Histogram[256];
	float* m_BmpBits;
	float* m_WorkBits;
	float* m_HelpBits;
	std::vector<tissues_size_t*> m_Tissuelayers;
	WshedObj m_Wshedobj;
	bool m_BmpIsGrey;
	bool m_WorkIsGrey;
	bool m_Loaded;
	bool m_Ownsliceprovider;
	FeatureExtractor m_Fextract;
	static std::list<float*> bits_stack;
	static std::list<unsigned char> mode_stack;
	SliceProvider* m_Sliceprovide;
	SliceProviderInstaller* m_SliceprovideInstaller;
	std::vector<std::vector<Mark>> m_Vvm;
	unsigned m_MaximStore;
	std::vector<std::vector<Point>> m_Limits;
	unsigned char m_Mode1;
	unsigned char m_Mode2;

	double m_RedFactor;
	double m_GreenFactor;
	double m_BlueFactor;
};

} // namespace iseg
