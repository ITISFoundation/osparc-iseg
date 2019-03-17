/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
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
class vtkStructuredPoints;

namespace iseg {

class Transform;
class TissueHiearchy;
class ColorLookupTable;
class bmphandler;
class ProgressInfo;

class SlicesHandler : public SliceHandlerInterface
{
public:
	SlicesHandler();
	~SlicesHandler();

	void newbmp(unsigned short width1, unsigned short height1, unsigned short nrofslices, const std::function<void(float**)>& init_callback = std::function<void(float**)>());
	void freebmp();
	void clear_bmp();
	void clear_work();
	void clear_overlay();
	void new_overlay();
	void set_bmp(unsigned short slicenr, float* bits, unsigned char mode);
	void set_work(unsigned short slicenr, float* bits, unsigned char mode);
	void set_tissue(unsigned short slicenr, tissues_size_t* bits);
	void copy2bmp(unsigned short slicenr, float* bits, unsigned char mode);
	void copy2work(unsigned short slicenr, float* bits, unsigned char mode);
	void copy2tissue(unsigned short slicenr, tissues_size_t* bits);
	void copyfrombmp(unsigned short slicenr, float* bits);
	void copyfromwork(unsigned short slicenr, float* bits);
	void copyfromtissue(unsigned short slicenr, tissues_size_t* bits);
#ifdef TISSUES_SIZE_TYPEDEF
	void copyfromtissue(unsigned short slicenr, unsigned char* bits);
#endif // TISSUES_SIZE_TYPEDEF
	void copyfromtissuepadded(unsigned short slicenr, tissues_size_t* bits, unsigned short padding);
	void set_rgb_factors(int redFactor, int greenFactor, int blueFactor);
	int LoadDIBitmap(std::vector<const char*> filenames);
	int LoadDIBitmap(std::vector<const char*> filenames, Point p, unsigned short dx, unsigned short dy);
	int LoadPng(std::vector<const char*> filenames);
	int LoadPng(std::vector<const char*> filenames, Point p, unsigned short dx, unsigned short dy);
	int LoadDIJpg(std::vector<const char*> filenames);
	int LoadDIJpg(std::vector<const char*> filenames, Point p, unsigned short dx, unsigned short dy);
	int LoadDICOM(std::vector<const char*> lfilename); //Assumption Filenames: fnxxx.bmp xxx: 3 digit number
	int LoadDICOM(std::vector<const char*> lfilename, Point p,
			unsigned short dx, unsigned short dy);
	int ReadImage(const char* filename);
	int ReadOverlay(const char* filename, unsigned short slicenr);
	int ReadAvw(const char* filename);
	int ReadRTdose(const char* filename);
	bool LoadSurface(const std::string& filename, bool overwrite_working, bool intersect);

	int LoadAllXdmf(const char* filename);
	int LoadAllHDF(const char* filename);

	void UpdateColorLookupTable(std::shared_ptr<ColorLookupTable> new_lut = nullptr);
	std::shared_ptr<ColorLookupTable> GetColorLookupTable() { return _color_lookup_table; }

	// Description: write project data into an Xdmf file
	int SaveAllXdmf(const char* filename, int compression, bool naked = false);
	bool SaveMarkersHDF(const char* filename, bool naked, unsigned short version);
	int SaveMergeAllXdmf(const char* filename, std::vector<QString>& mergeImagefilenames, unsigned short nrslicesTotal, int compression);
	int ReadRaw(const char* filename, short unsigned w, short unsigned h,
			unsigned bitdepth, unsigned short slicenr,
			unsigned short nrofslices);
	int ReadRaw(const char* filename, short unsigned w, short unsigned h,
			unsigned bitdepth, unsigned short slicenr,
			unsigned short nrofslices, Point p, unsigned short dx,
			unsigned short dy);
	int ReadRawFloat(const char* filename, short unsigned w, short unsigned h,
			unsigned short slicenr, unsigned short nrofslices);
	int ReadRawFloat(const char* filename, short unsigned w, short unsigned h,
			unsigned short slicenr, unsigned short nrofslices, Point p,
			unsigned short dx, unsigned short dy);
	int ReadRawOverlay(const char* filename, unsigned bitdepth, unsigned short slicenr);
	int SaveRaw_resized(const char* filename, int dxm, int dxp, int dym, int dyp, int dzm, int dzp, bool work);
	int SaveTissuesRaw_resized(const char* filename, int dxm, int dxp, int dym, int dyp, int dzm, int dzp);
	bool SwapXY();
	bool SwapYZ();
	bool SwapXZ();
	int SaveRaw_xy_swapped(const char* filename, bool work);
	static int SaveRaw_xy_swapped(const char* filename,
			std::vector<float*> bits_to_swap,
			short unsigned width, short unsigned height,
			short unsigned nrslices);
	int SaveRaw_xz_swapped(const char* filename, bool work);
	static int SaveRaw_xz_swapped(const char* filename,
			std::vector<float*> bits_to_swap,
			short unsigned width, short unsigned height,
			short unsigned nrslices);
	int SaveRaw_yz_swapped(const char* filename, bool work);
	static int SaveRaw_yz_swapped(const char* filename,
			std::vector<float*> bits_to_swap,
			short unsigned width, short unsigned height,
			short unsigned nrslices);
	int SaveTissuesRaw(const char* filename);
	int SaveTissuesRaw_xy_swapped(const char* filename);
	int SaveTissuesRaw_xz_swapped(const char* filename);
	int SaveTissuesRaw_yz_swapped(const char* filename);
	int ReloadDIBitmap(std::vector<const char*> filenames);
	int ReloadDIBitmap(std::vector<const char*> filenames, Point p);
	int ReloadDICOM(std::vector<const char*> lfilename);
	int ReloadDICOM(std::vector<const char*> lfilename, Point p);
	int ReloadRaw(const char* filename, unsigned bitdepth,
			unsigned short slicenr);
	int ReloadRaw(const char* filename, short unsigned w, short unsigned h,
			unsigned bitdepth, unsigned short slicenr, Point p);
	int ReloadRawFloat(const char* filename, unsigned short slicenr);
	int ReloadRawFloat(const char* filename, short unsigned w, short unsigned h,
			unsigned short slicenr, Point p);
	static std::vector<float*> LoadRawFloat(const char* filename,
			unsigned short startslice,
			unsigned short endslice,
			unsigned short slicenr,
			unsigned int area);
	int ReloadRawTissues(const char* filename, unsigned bitdepth,
			unsigned short slicenr);
	int ReloadRawTissues(const char* filename, short unsigned w,
			short unsigned h, unsigned bitdepth,
			unsigned short slicenr, Point p);
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
	void save_contours(const char* filename);
	void shift_contours(int dx, int dy);
	void setextrusion_contours(int top, int bottom);
	void resetextrusion_contours();
	FILE* save_contourprologue(const char* filename, unsigned nr_slices);
	FILE* save_contoursection(FILE* fp, unsigned startslice1, unsigned endslice1, unsigned offset);
	FILE* save_tissuenamescolors(FILE* fp);
	void work2bmp();
	void bmp2work();
	void swap_bmpwork();
	void work2bmpall();
	void bmp2workall();
	void work2tissueall();
	void swap_bmpworkall();

	std::vector<const float*> source_slices() const override;
	std::vector<float*> source_slices() override;
	std::vector<const float*> target_slices() const override;
	std::vector<float*> target_slices() override;
	std::vector<const tissues_size_t*> tissue_slices(tissuelayers_size_t layeridx) const override;
	std::vector<tissues_size_t*> tissue_slices(tissuelayers_size_t layeridx) override;

	std::vector<std::string> tissue_names() const override;
	std::vector<bool> tissue_locks() const override;
	std::vector<tissues_size_t> tissue_selection() const override;

	boost::signals2::signal<void(const std::vector<tissues_size_t>& sel)> on_tissue_selection_changed;
	void set_tissue_selection(const std::vector<tissues_size_t>& sel) override;

	bool has_colors() const override { return _color_lookup_table != 0; }
	size_t number_of_colors() const override;
	void get_color(size_t, unsigned char& r, unsigned char& g, unsigned char& b) const override;

	void set_target_fixed_range(bool on) override { set_modeall(on ? 2 : 1, false); }

	float* return_bmp(unsigned short slicenr1);
	float* return_work(unsigned short slicenr1);
	tissues_size_t* return_tissues(tissuelayers_size_t layeridx, unsigned short slicenr1);
	float* return_overlay();
	float get_work_pt(Point p, unsigned short slicenr);
	void set_work_pt(Point p, unsigned short slicenr, float f);
	float get_bmp_pt(Point p, unsigned short slicenr);
	void set_bmp_pt(Point p, unsigned short slicenr, float f);
	tissues_size_t get_tissue_pt(Point p, unsigned short slicenr);
	void set_tissue_pt(Point p, unsigned short slicenr, tissues_size_t f);
	unsigned int make_histogram(bool includeoutofrange);
	unsigned int return_area();
	unsigned short width() const override;
	unsigned short height() const override;
	unsigned short num_slices() const override;
	unsigned short start_slice() const override;
	unsigned short end_slice() const override;
	void set_startslice(unsigned short startslice1);
	void set_endslice(unsigned short endslice1);
	bool isloaded();
	void threshold(float* thresholds);
	void bmp_sum();
	void bmp_add(float f);
	void bmp_diff();
	void bmp_mult();
	void bmp_mult(float f);
	void bmp_overlay(float alpha);
	void bmp_abs();
	void bmp_neg();
	void scale_colors(Pair p);
	void crop_colors();
	void get_range(Pair* pp);
	void compute_range_mode1(Pair* pp);
	void compute_range_mode1(unsigned short updateSlicenr, Pair* pp);
	void get_bmprange(Pair* pp);
	void compute_bmprange_mode1(Pair* pp);
	void compute_bmprange_mode1(unsigned short updateSlicenr, Pair* pp);
	void get_rangetissue(tissues_size_t* pp);
	void gaussian(float sigma);
	void average(unsigned short n);
	void median_interquartile(bool median);
	void aniso_diff(float dt, int n, float (*f)(float, float), float k,
			float restraint);
	void cont_anisodiff(float dt, int n, float (*f)(float, float), float k,
			float restraint);
	void stepsmooth_z(unsigned short n);
	void smooth_tissues(unsigned short n);
	void sigmafilter(float sigma, unsigned short nx, unsigned short ny);
	void hysteretic(float thresh_low, float thresh_high, bool connectivity,
			unsigned short nrpasses);
	void double_hysteretic(float thresh_low_l, float thresh_low_h,
			float thresh_high_l, float thresh_high_h,
			bool connectivity, unsigned short nrpasses);
	void double_hysteretic_allslices(float thresh_low_l, float thresh_low_h,
			float thresh_high_l, float thresh_high_h,
			bool connectivity, float set_to);
	void thresholded_growing(short unsigned slicenr, Point p,
			float threshfactor_low, float threshfactor_high,
			bool connectivity, unsigned short nrpasses);
	void thresholded_growing(short unsigned slicenr, Point p, float thresh_low,
			float thresh_high,
			float set_to); //bool connectivity,float set_to);
	void erosion(boost::variant<int, float> radius, bool true3d);
	void dilation(boost::variant<int, float> radius, bool true3d);
	void closure(boost::variant<int, float> radius, bool true3d);
	void open(boost::variant<int, float> radius, bool true3d);
	void add_mark(Point p, unsigned label);
	void add_mark(Point p, unsigned label, std::string str);
	void clear_marks();
	bool remove_mark(Point p, unsigned radius);
	void add_mark(Point p, unsigned label, unsigned short slicenr);
	void add_mark(Point p, unsigned label, std::string str,
			unsigned short slicenr);
	bool remove_mark(Point p, unsigned radius, unsigned short slicenr);
	void get_labels(std::vector<augmentedmark>* labels);
	void add_vm(std::vector<Mark>* vm1);
	bool remove_vm(Point p, unsigned radius);
	void add_vm(std::vector<Mark>* vm1, unsigned short slicenr);
	bool remove_vm(Point p, unsigned radius, unsigned short slicenr);
	void add_limit(std::vector<Point>* vp1);
	bool remove_limit(Point p, unsigned radius);
	void add_limit(std::vector<Point>* vp1, unsigned short slicenr);
	bool remove_limit(Point p, unsigned radius, unsigned short slicenr);
	void zero_crossings(bool connectivity);
	void dougpeuck_line(float epsilon);
	void kmeans(unsigned short slicenr, short nrtissues, unsigned int iternr,
			unsigned int converge);
	void kmeans_mhd(unsigned short slicenr, short nrtissues, short dim,
			std::vector<std::string> mhdfiles, float* weights,
			unsigned int iternr, unsigned int converge);
	void kmeans_png(unsigned short slicenr, short nrtissues, short dim,
			std::vector<std::string> pngfiles,
			std::vector<int> exctractChannel, float* weights,
			unsigned int iternr, unsigned int converge,
			const std::string initCentersFile = "");
	void em(unsigned short slicenr, short nrtissues, unsigned int iternr,
			unsigned int converge);
	void extract_contours(int minsize, std::vector<tissues_size_t>& tissuevec);
	void extract_contours2_xmirrored(int minsize,
			std::vector<tissues_size_t>& tissuevec);
	void extract_contours2_xmirrored(int minsize,
			std::vector<tissues_size_t>& tissuevec,
			float epsilon);
	void extractinterpolatesave_contours(int minsize,
			std::vector<tissues_size_t>& tissuevec,
			unsigned short between, bool dp,
			float epsilon, const char* filename);
	void extractinterpolatesave_contours2_xmirrored(
			int minsize, std::vector<tissues_size_t>& tissuevec,
			unsigned short between, bool dp, float epsilon, const char* filename);
	void add2tissue(tissues_size_t tissuetype, Point p, bool override);
	void add2tissueall(tissues_size_t tissuetype, Point p, bool override);
	void add2tissueall(tissues_size_t tissuetype, Point p,
			unsigned short slicenr, bool override);
	void add2tissueall(tissues_size_t tissuetype, float f, bool override);
	void add2tissue_connected(tissues_size_t tissuetype, Point p,
			bool override);
	void add2tissueall_connected(tissues_size_t tissuetype, Point p,
			bool override);
	void add2tissue_thresh(tissues_size_t tissuetype, Point p);
	void add2tissue(tissues_size_t tissuetype, bool* mask,
			unsigned short slicenr, bool override);
	void subtract_tissue(tissues_size_t tissuetype, Point p);
	void subtract_tissueall(tissues_size_t tissuetype, Point p);
	void subtract_tissueall(tissues_size_t tissuetype, Point p,
			unsigned short slicenr);
	void subtract_tissueall(tissues_size_t tissuetype, float f);
	void subtract_tissue_connected(tissues_size_t tissuetype, Point p);
	void subtract_tissueall_connected(tissues_size_t tissuetype, Point p);
	void tissue2workall3D();
	void tissue2workall();
	void selectedtissue2work(const std::vector<tissues_size_t>& tissuetype);
	void selectedtissue2work3D(const std::vector<tissues_size_t>& tissuetype);
	void cleartissue(tissues_size_t tissuetype);
	void cleartissue3D(tissues_size_t tissuetype);
	void cleartissues();
	void cleartissues3D();
	void interpolatetissue(unsigned short slice1, unsigned short slice2, tissues_size_t tissuetype, bool connected);
	void interpolatetissue_medianset(unsigned short slice1,
			unsigned short slice2,
			tissues_size_t tissuetype,
			bool connectivity,
			bool handleVanishingComp = true);
	void extrapolatetissue(unsigned short origin1, unsigned short origin2, unsigned short target, tissues_size_t tissuetype);
	void interpolatework(unsigned short slice1, unsigned short slice2);
	void extrapolatework(unsigned short origin1, unsigned short origin2, unsigned short target);
	void interpolatetissuegrey(unsigned short slice1, unsigned short slice2);
	void interpolatetissuegrey_medianset(unsigned short slice1,
			unsigned short slice2,
			bool connectivity,
			bool handleVanishingComp = true);
	void interpolateworkgrey(unsigned short slice1, unsigned short slice2, bool connected);
	void interpolateworkgrey_medianset(unsigned short slice1,
			unsigned short slice2, bool connectivity,
			bool handleVanishingComp = true);
	void interpolate(unsigned short slice1, unsigned short slice2);
	void extrapolate(unsigned short origin1, unsigned short origin2, unsigned short target);
	void interpolate(unsigned short slice1, unsigned short slice2, float* bmp1, float* bmp2);

	bool compute_target_connectivity(ProgressInfo* progress = nullptr);
	bool compute_split_tissues(tissues_size_t tissue, ProgressInfo* progress = nullptr);

	void set_slicethickness(float t);
	float get_slicethickness();
	void set_pixelsize(float dx1, float dy1);
	Pair get_pixelsize();
	void slicebmp_x(float* return_bits, unsigned short xcoord);
	float* slicebmp_x(unsigned short xcoord);
	void slicebmp_y(float* return_bits, unsigned short ycoord);
	float* slicebmp_y(unsigned short ycoord);
	void slicework_x(float* return_bits, unsigned short xcoord);
	float* slicework_x(unsigned short xcoord);
	void slicework_y(float* return_bits, unsigned short ycoord);
	float* slicework_y(unsigned short ycoord);
	void slicetissue_x(tissues_size_t* return_bits, unsigned short xcoord);
	tissues_size_t* slicetissue_x(unsigned short xcoord);
	void slicetissue_y(tissues_size_t* return_bits, unsigned short ycoord);
	tissues_size_t* slicetissue_y(unsigned short ycoord);
	void slicework_z(unsigned short slicenr);
	int extract_tissue_surfaces(const QString& filename,
			std::vector<tissues_size_t>& tissuevec,
			bool usediscretemc = false, float ratio = 1.0f,
			unsigned smoothingiterations = 15,
			float passBand = 0.1f,
			float featureAngle = 180);
	void next_slice();
	void prev_slice();
	unsigned short get_next_featuring_slice(tissues_size_t type, bool& found);

	unsigned short active_slice() const override;
	boost::signals2::signal<void(unsigned short)> on_active_slice_changed;
	void set_active_slice(unsigned short slice, bool signal_change = false) override;

	bmphandler* get_activebmphandler();
	tissuelayers_size_t active_tissuelayer() const override;
	void set_active_tissuelayer(tissuelayers_size_t idx);
	unsigned pushstack_bmp();
	unsigned pushstack_bmp(unsigned int slice);
	unsigned pushstack_work();
	unsigned pushstack_work(unsigned int slice);
	unsigned pushstack_tissue(tissues_size_t i);
	unsigned pushstack_tissue(tissues_size_t i, unsigned int slice);
	unsigned pushstack_help();
	void removestack(unsigned i);
	void clear_stack();
	float* getstack(unsigned i, unsigned char& mode);
	void getstack_bmp(unsigned i);
	void getstack_bmp(unsigned int slice, unsigned i);
	void getstack_work(unsigned i);
	void getstack_work(unsigned int slice, unsigned i);
	void getstack_help(unsigned i);
	void getstack_tissue(unsigned i, tissues_size_t tissuenr, bool override);
	void getstack_tissue(unsigned int slice, unsigned i, tissues_size_t tissuenr, bool override);
	void popstack_bmp();
	void popstack_work();
	void popstack_help();
	unsigned loadstack(const char* filename);
	void savestack(unsigned i, const char* filename);
	void fill_holes(float f, int minsize);
	void remove_islands(float f, int minsize);
	void fill_gaps(int minsize, bool connectivity);
	void fill_holestissue(tissues_size_t f, int minsize);
	void remove_islandstissue(tissues_size_t f, int minsize);
	void fill_gapstissue(int minsize, bool connectivity);
	float add_skin(int i);
	void add_skintissue(int i1, tissues_size_t f);
	float add_skin3D(int i);
	float add_skin3D(int ix, int iy, int iz);
	void add_skin3D(int ix, int iy, int iz, float setto);
	void add_skintissue3D(int ix, int iy, int iz, tissues_size_t f);
	float add_skin_outside(int i);
	void add_skintissue_outside(int i1, tissues_size_t f);
	float add_skin3D_outside(int i);
	void add_skin3D_outside(int ix, int iy, int iz, float setto);
	float add_skin3D_outside2(int ix, int iy, int iz);
	void add_skin3D_outside2(int ix, int iy, int iz, float setto);
	void add_skintissue3D_outside(int ixyz, tissues_size_t f);
	void add_skintissue3D_outside2(int ix, int iy, int iz, tissues_size_t f);
	bool value_at_boundary3D(float value);
	bool tissuevalue_at_boundary3D(tissues_size_t value);
	void fill_skin_3d(int thicknessX, int thicknessY, int thicknessZ,
			tissues_size_t backgroundID, tissues_size_t skinID);
	void gamma_mhd(unsigned short slicenr, short nrtissues, short dim,
			std::vector<std::string> mhdfiles, float* weights,
			float** centers, float* tol_f, float* tol_d);
	void fill_unassigned();
	void fill_unassignedtissue(tissues_size_t f);
	void start_undo(DataSelection& dataSelection);
	bool start_undoall(DataSelection& dataSelection);
	bool start_undo(DataSelection& dataSelection, std::vector<unsigned> vslicenr1);
	void abort_undo();
	void end_undo();
	void merge_undo();
	DataSelection undo();
	DataSelection redo();
	void clear_undo();
	void reverse_undosliceorder();
	unsigned return_nrundo();
	unsigned return_nrredo();
	bool return_undo3D();
	unsigned return_nrundosteps();
	unsigned return_nrundoarrays();
	void set_undo3D(bool undo3D1);
	void set_undonr(unsigned nr);
	void set_undoarraynr(unsigned nr);
	void map_tissue_indices(const std::vector<tissues_size_t>& indexMap);
	void remove_tissue(tissues_size_t tissuenr);
	void remove_tissues(const std::set<tissues_size_t>& tissuenrs);
	void remove_tissueall();
	std::vector<tissues_size_t> find_unused_tissues();
	void cap_tissue(tissues_size_t maxval);
	void buildmissingtissues(tissues_size_t j);
	void group_tissues(std::vector<tissues_size_t>& olds, std::vector<tissues_size_t>& news);
	void GetDICOMseriesnr(std::vector<const char*>* vnames,
			std::vector<unsigned>* dicomseriesnr,
			std::vector<unsigned>* dicomseriesnrlist);
	void set_modeall(unsigned char mode, bool bmporwork);
	bool print_amascii(const char* filename);
	bool print_tissuemat(const char* filename);
	bool print_bmpmat(const char* filename);
	bool print_workmat(const char* filename);

	bool export_tissue(const char* filename, bool binary) const;
	bool export_bmp(const char* filename, bool binary) const;
	bool export_work(const char* filename, bool binary) const;

	bool print_xmlregionextent(const char* filename, bool onlyactiveslices, const char* projname = nullptr);
	bool print_tissueindex(const char* filename, bool onlyactiveslices, const char* projname = nullptr);
	bool print_atlas(const char* filename);
	vtkImageData* make_vtktissueimage();
	float calculate_volume(Point p, unsigned short slicenr);
	float calculate_tissuevolume(Point p, unsigned short slicenr);
	void inversesliceorder();

	Vec3 spacing() const override;
	Transform transform() const override;
	Transform get_transform_active_slices() const;
	void set_transform(const Transform& tr);
	void get_displacement(float disp[3]) const;
	void set_displacement(const float disp[3]);
	void get_direction_cosines(float dc[6]) const;
	void set_direction_cosines(const float dc[6]);

	bool get_extent(tissues_size_t tissuenr, bool onlyactiveslices, unsigned short extent[3][2]);
	void regrow(unsigned short sourceslicenr, unsigned short targetslicenr, int n);
	bool unwrap(float jumpratio, float shift = 0);
	unsigned GetNumberOfUndoSteps();
	void SetNumberOfUndoSteps(unsigned);
	unsigned GetNumberOfUndoArrays();
	void SetNumberOfUndoArrays(unsigned);
	int GetCompression() const { return this->_hdf5_compression; }
	void SetCompression(int c) { this->_hdf5_compression = c; }
	bool GetContiguousMemory() const { return _contiguous_memory_io; }
	void SetContiguousMemory(bool v) { _contiguous_memory_io = v; }

	int SaveRaw(const char* filename, bool work);
	float DICOMsort(std::vector<const char*>* lfilename);

	TissueHiearchy* get_tissue_hierachy() { return _tissue_hierachy; }

	void mergetissues(tissues_size_t tissuetype);

private:
	unsigned short _activeslice;
	std::vector<bmphandler> _image_slices;
	short unsigned _width;
	short unsigned _height;
	short unsigned _startslice;
	short unsigned _endslice;
	short unsigned _nrslices;
	unsigned int _area;
	float _thickness;
	float _dx;
	float _dy;
	Transform _transform;
	tissuelayers_size_t _active_tissuelayer;
	std::shared_ptr<ColorLookupTable> _color_lookup_table;
	TissueHiearchy* _tissue_hierachy;
	float* _overlay;
	std::vector<Pair> _slice_ranges;
	std::vector<Pair> _slice_bmpranges;
	OutlineSlices _os;

	bool _loaded;
	UndoElem* _uelem;
	UndoQueue _undoQueue;
	bool _undo3D;
	int _hdf5_compression;
	bool _contiguous_memory_io;
};

} // namespace iseg
