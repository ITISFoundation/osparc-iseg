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

#include "Core/Types.h"
#include "Core/Mark.h"
#include "Core/Pair.h"
#include "Core/feature_extractor.h"
#include "Core/contour_class.h"

#include <set>
#include <list>
#include <vector>

class IFT_regiongrowing;
class IFT_livewire;
class IFT_fastmarch;
class sliceprovider;
class sliceprovider_installer;

const unsigned int unvisited=222222;

typedef struct 
{
	unsigned g;
	unsigned r;
	unsigned l;
} basin;

typedef struct
{
	unsigned k;
	unsigned a;
	unsigned g;
} merge_event;

typedef struct
{
	std::vector<basin> B;
	std::vector<merge_event> M;
	std::vector<Point> marks;
} wshed_obj;

const unsigned int ptnr=8192;
const float f_tol=0.00001f;
const float very_bit=1E10;

class bmphandler
{
public:
	bmphandler();
	bmphandler(const bmphandler &object);
	~bmphandler();

	float *copy_work();
	float *copy_bmp();
	tissues_size_t *copy_tissue(tissuelayers_size_t idx);
	void copy_work(float *output);
	void copy_bmp(float *output);
	void copy_tissue(tissuelayers_size_t idx, tissues_size_t *output);
	float *return_bmp();
	const float *return_bmp() const;
	float *return_work();
	const float *return_work() const;
	tissues_size_t *return_tissues(tissuelayers_size_t idx);
	const tissues_size_t *return_tissues(tissuelayers_size_t idx) const;
	float *return_help();
	float **return_bmpfield();
	float **return_workfield();
	tissues_size_t **return_tissuefield(tissuelayers_size_t idx);
	
	std::vector<mark> *return_marks();
	void copy2marks(std::vector<mark> *marks1);
	void get_add_labels(std::vector<mark> *labels);
	void get_labels(std::vector<mark> *labels);
	void clear_tissue(tissuelayers_size_t idx);
	bool has_tissue(tissuelayers_size_t idx, tissues_size_t tissuetype);
	void add2tissue(tissuelayers_size_t idx, tissues_size_t tissuetype, Point p, bool override);
	void add2tissue_connected(tissuelayers_size_t idx, tissues_size_t tissuetype, Point p, bool override);
	void add2tissue(tissuelayers_size_t idx, tissues_size_t tissuetype, float f, bool override);
	void add2tissue(tissuelayers_size_t idx, tissues_size_t tissuetype, bool *mask, bool override);
	void add2tissue_thresh(tissuelayers_size_t idx, tissues_size_t tissuetype, Point p);
	void subtract_tissue(tissuelayers_size_t idx, tissues_size_t tissuetype, Point p);
	void subtract_tissue_connected(tissuelayers_size_t idx, tissues_size_t tissuetype, Point p);
	void subtract_tissue(tissuelayers_size_t idx, tissues_size_t tissuetype, float f);
	void change2mask_connectedtissue(tissuelayers_size_t idx, bool *mask, Point p, bool addorsub);
	void change2mask_connectedwork(bool *mask, Point p, bool addorsub);
	void tissue2work(tissuelayers_size_t idx, tissues_size_t tissuetype);
	void setissue2work(tissuelayers_size_t idx, tissues_size_t tissuetype);
	void tissue2work(tissuelayers_size_t idx);
	void work2tissue(tissuelayers_size_t idx);
	void cleartissue(tissuelayers_size_t idx, tissues_size_t tissuetype);
	void cleartissues(tissuelayers_size_t idx);
	void cleartissuesall();
	void cap_tissue(tissues_size_t maxval);
	void set_bmp(float *bits, unsigned char mode);
	void set_work(float *bits, unsigned char mode);
	void set_tissue(tissuelayers_size_t idx, tissues_size_t *bits);
	float *swap_bmp_pointer(float *bits);
	float *swap_work_pointer(float *bits);
	tissues_size_t *swap_tissues_pointer(tissuelayers_size_t idx, tissues_size_t *bits);
	void copy2bmp(float *bits, unsigned char mode);
	void copy2work(float *bits, unsigned char mode);
	void copy2work(float *bits, bool *mask, unsigned char mode);
	void copy2tissue(tissuelayers_size_t idx, tissues_size_t *bits);
	void copy2tissue(tissuelayers_size_t idx, tissues_size_t *bits, bool *mask);
	void copyfrombmp(float *bits);
	void copyfromwork(float *bits);
	void copyfromtissue(tissuelayers_size_t idx, tissues_size_t *bits);
#ifdef TISSUES_SIZE_TYPEDEF
	void copyfromtissue(tissuelayers_size_t idx, unsigned char *bits);
#endif // TISSUES_SIZE_TYPEDEF
	void copyfromtissuepadded(tissuelayers_size_t idx, tissues_size_t *bits, unsigned short padding);
	void clear_bmp();
	void clear_work();
	void bmp_sum();
	void bmp_add(float f);
	void bmp_diff();
	void bmp_mult();
	void bmp_mult(float f);
	void bmp_abs();
	void bmp_neg();
	void bmp_overlay(float alpha);
	void transparent_add(float *pict2);
	void newbmp(unsigned short width1, unsigned short  height1);
	void newbmp(unsigned short width1, unsigned short  height1, float *bits);
	void freebmp();
	int CheckBMPDepth(const char *filename);
	void SetConverterFactors(int redFactor, int greenFactor, int blueFactor);
	int LoadDIBitmap(const char *filename);
	int LoadDIBitmap(const char *filename, Point p, unsigned short dx, unsigned short dy);
	int ReloadDIBitmap(const char *filename);
	int ReloadDIBitmap(const char *filename, Point p);
	int CheckPNGDepth(const char *filename);
	int LoadPNGBitmap(const char *filename);
	bool LoadArray(float *bits, unsigned short w1, unsigned short h1);
	bool LoadArray(float *bits, unsigned short w1, unsigned short h1, Point p, unsigned short dx, unsigned short dy);
	bool ReloadArray(float *bits);
	bool ReloadArray(float *bits, unsigned short w1, unsigned short h1, Point p);
	bool LoadDICOM(const char *filename);
	bool LoadDICOM(const char *filename, Point p, unsigned short dx, unsigned short dy);
	bool ReloadDICOM(const char *filename);
	bool ReloadDICOM(const char *filename, Point p);
	FILE *save_proj(FILE *fp, bool inclpics = true);
	FILE *save_stack(FILE *fp);
	FILE *load_proj(FILE *fp, int tissuesVersion, bool inclpics = true);
	FILE *load_stack(FILE *fp);
	int SaveDIBitmap(const char *filename);
	int SaveWorkBitmap(const char *filename);
	int SaveTissueBitmap(tissuelayers_size_t idx, const char *filename);
	int ReadRaw(const char *filename, short unsigned w, short unsigned h, unsigned bitdepth, unsigned short slicenr);
	int ReadRaw(const char *filename, short unsigned w, short unsigned h, unsigned bitdepth, unsigned short slicenr, Point p, unsigned short dx, unsigned short dy);
	int ReloadRaw(const char *filename, unsigned bitdepth, unsigned slicenr);
	int ReloadRaw(const char *filename, short unsigned w, short unsigned h, unsigned bitdepth, unsigned slicenr, Point p);
	int ReadRawFloat(const char *filename, short unsigned w, short unsigned h, unsigned short slicenr);
	int ReadRawFloat(const char *filename, short unsigned w, short unsigned h, unsigned short slicenr, Point p, unsigned short dx, unsigned short dy);
	int ReloadRawFloat(const char *filename, unsigned slicenr);
	static float* ReadRawFloat(const char *filename, unsigned slicenr, unsigned int area);
	int ReloadRawFloat(const char *filename, short unsigned w, short unsigned h, unsigned slicenr, Point p);
	int ReloadRawTissues(const char *filename, unsigned bitdepth, unsigned slicenr);
	int ReloadRawTissues(const char *filename, short unsigned w, short unsigned h, unsigned bitdepth, unsigned slicenr, Point p);
	int SaveBmpRaw(const char *filename);
	int SaveWorkRaw(const char *filename);
	int SaveTissueRaw(tissuelayers_size_t idx, const char *filename);
	int ReadAvw(const char *filename, short unsigned i);
	void work2bmp();
	void bmp2work();
	void swap_bmpwork();
	void swap_bmphelp();
	void swap_workhelp();
	unsigned int make_histogram(bool includeoutofrange);
	unsigned int make_histogram(float *mask, float f, bool includeoutofrange);
	unsigned int make_histogram(Point p, unsigned short dx, unsigned short dy, bool includeoutofrange);
	unsigned int *return_histogram();
	float *find_modal(unsigned int thresh1, float thresh2);
	void print_info();
	void print_histogram();
	unsigned int return_area();
	unsigned int return_width();
	unsigned int return_height();
	void threshold(float *thresholds);
	void threshold(float *thresholds, Point p, unsigned short dx, unsigned short dy);
	void convolute(float *mask, unsigned short direction); // x: 0, y: 1, xy: 2
	void scale_colors(Pair p);
	void crop_colors();
	void get_range(Pair *pp);
	void get_rangetissue(tissuelayers_size_t idx, tissues_size_t *pp);
	void get_bmprange(Pair *pp);
	void gaussian(float sigma);
	void average(unsigned short n);
	void laplacian();
	void laplacian1();
	void sobel();
	void sobel_finer();
	void sobelxy(float **sobelx, float **sobely);
	void median_interquartile(bool median);
	void median_interquartile(float *median, float *iq);
	void sigmafilter(float sigma, unsigned short nx, unsigned short ny);
	//		void erase_text();
	void compacthist();
	void moment_line();
	void gauss_line(float sigma);
	void gauss_sharpen(float sigma);
	void canny_line(float sigma, float thresh_low, float thresh_high);
	//		void canny_line1(float sigma, float thresh_low, float thresh_high);
	void subthreshold(int n1, int n2, unsigned int thresh1, float thresh2, float sigma);
	void convolute_hist(float *mask);
	void gaussian_hist(float sigma);
	float *direction_map(float *sobelx, float *sobely);
	void nonmaximum_supr(float *direct_map);
	void hysteretic(float thresh_low, float thresh_high, bool connectivity, float set_to);
	void hysteretic(float thresh_low, float thresh_high, bool connectivity, float *mask, float f, float set_to);
	void double_hysteretic(float thresh_low_l, float thresh_low_h, float thresh_high_l, float thresh_high_h, bool connectivity, float set_to);
	void double_hysteretic(float thresh_low_l, float thresh_low_h, float thresh_high_l, float thresh_high_h, bool connectivity, float *mask, float f, float set_to);
	void thresholded_growing(Point p, float thresh_low, float thresh_high, bool connectivity, float set_to);
	void thresholded_growing(Point p, float thresh_low, float thresh_high, bool connectivity, float set_to, std::vector<Point> *limits1);
	void thresholded_growinglimit(Point p, float thresh_low, float thresh_high, bool connectivity, float set_to);
	void thresholded_growing(Point p, float threshfactor_low, float threshfactor_high, bool connectivity, float set_to, Pair *tp);
	void thresholded_growing(float thresh_low, float thresh_high, bool connectivity, float *mask, float f, float set_to);
	void distance_map(bool connectivity);
	void distance_map(bool connectivity, float f, short unsigned levlset);//0:outside,1:inside,2:both
	void dead_reckoning();
	unsigned *dead_reckoning(float f);
	unsigned *dead_reckoning_squared(float f);
	void IFT_distance1(float f);
	void erosion(int n, bool connectivity);
	void erosion1(int n, bool connectivity);
	void dilation(int n, bool connectivity);
	void closure(int n, bool connectivity);
	void open(int n, bool connectivity);
	void erasework(bool *mask);
	void erasetissue(tissuelayers_size_t idx, bool *mask);
	void floodwork(bool *mask);
	void floodtissue(tissuelayers_size_t idx, bool *mask);
	void mark_border(bool connectivity);
	void aniso_diff(float dt, int n, float(*f)(float, float), float k, float restraint);
	void cont_anisodiff(float dt, int n, float(*f)(float, float), float k, float restraint);
	unsigned *watershed(bool connectivity);
	unsigned *watershed_sobel(bool connectivity);
	void construct_regions(unsigned h, unsigned *wshed);
	void add_mark(Point p, unsigned label, std::string str = "");
	bool remove_mark(Point p, unsigned radius);
	void clear_marks();
	//		void snake();
	void n_moment(short unsigned n, short unsigned p);
	void n_moment_sigma(short unsigned n, short unsigned p, float sigma);
	//		void region_growing();
	void zero_crossings(bool connectivity);
	void zero_crossings(float thresh, bool connectivity);
	void laplacian_zero(float sigma, float thresh, bool connectivity);
	void to_bmpgrey(float *p_bits);
	void connected_components(bool connectivity);
	void connected_components(bool connectivity, std::set<float> &components);
	void fill_gaps(short unsigned n, bool connectivity);
	void fill_gapstissue(tissuelayers_size_t idx, short unsigned n, bool connectivity);
	void wshed2work(unsigned *Y);
	void labels2work(unsigned *Y, unsigned lnr);
	//		change_island, remove_island s. 3D-slicer
	//		registration: manual, mutual information s. 3D-slicer
	void get_tissuecontours(tissuelayers_size_t idx, tissues_size_t f, std::vector<std::vector<Point> > *outer_line, std::vector<std::vector<Point> > *inner_line, int minsize);
	void get_tissuecontours2_xmirrored(tissuelayers_size_t idx, tissues_size_t f, std::vector<std::vector<Point> > *outer_line, std::vector<std::vector<Point> > *inner_line, int minsize);
	void get_tissuecontours2_xmirrored(tissuelayers_size_t idx, tissues_size_t f, std::vector<std::vector<Point> > *outer_line, std::vector<std::vector<Point> > *inner_line, int minsize, float disttol);
	void get_contours(float f, std::vector<std::vector<Point> > *outer_line, std::vector<std::vector<Point> > *inner_line, int minsize);
	void get_contours(Point p, std::vector<std::vector<Point> > *outer_line, std::vector<std::vector<Point> > *inner_line, int minsize);
	void load_line(std::vector<Point> *vec_pt);
	void presimplify_line(float d);
	void dougpeuck_line(float epsilon);
	void plot_line();
	float bmp_pt(Point p);
	float work_pt(Point p);
	tissues_size_t tissues_pt(tissuelayers_size_t idx, Point p);
	//		float get_work_pt(Point p);
	void set_work_pt(Point p, float f);
	void set_bmp_pt(Point p, float f);
	void set_tissue_pt(tissuelayers_size_t idx, Point p, tissues_size_t f);
	void stupidDT();
	void rgIFT(float *lb_map, float thresh);
	IFT_regiongrowing *IFTrg_init(float *lb_map);
	//		void rgIFT(std::vector<mark> *vm,float thresh);
	void livewire_test();
	IFT_livewire *livewireinit(Point p);
	void fill_contour(std::vector<Point> *vp, bool continuous);
	void adaptive_fuzzy(Point p, float m1, float s1, float s2, float thresh);
	void fast_marching(Point p, float sigma, float thresh);
	IFT_fastmarch *fastmarching_init(Point p, float sigma, float thresh);
	void classify(short nrclasses, short dim, float **bits, float *weights, float *centers, float maxdist);
	void classifytest();
	void EMtest();
	void em(short nrtissues, short dim, float **bits, float *weights, unsigned int iternr, unsigned int converge);
	float *classifytest1();
	void kmeans(short nrtissues, short dim, float **bits, float *weights, unsigned int iternr, unsigned int converge);
	void kmeans_mhd(short nrtissues, short dim, std::vector<std::string> mhdfiles, unsigned short slicenr, float *weights, unsigned int iternr, unsigned int converge);
	void kmeans_png(short nrtissues, short dim, std::vector<std::string> pngfiles, std::vector<int> exctractChannel, unsigned short slicenr, float *weights, unsigned int iternr, unsigned int converge, const std::string initCentersFile = "");
	void gamma_mhd(short nrtissues, short dim, std::vector<std::string> mhdfiles, unsigned short slicenr, float *weights, float **centers, float *tol_f, float *tol_d, Pair pixelsize);
	void levelsettest(float sigma, float epsilon, float alpha, float beta, float stepsize, unsigned nrsteps, unsigned reinitfreq);
	void levelsettest1(float sigma, float epsilon, float alpha, float beta, float stepsize, unsigned nrsteps, unsigned reinitfreq);
	void cannylevelset(float *initlev, float f, float sigma, float thresh_low, float thresh_high, float epsilon, float stepsize, unsigned nrsteps, unsigned reinitfreq);
	void cannylevelsettest(float sigma, float thresh_low, float thresh_high, float epsilon, float stepsize, unsigned nrsteps, unsigned reinitfreq);
	void threshlevelset(float thresh_low, float thresh_high, float epsilon, float stepsize, unsigned nrsteps, unsigned reinitfreq);
	float extract_feature(Point p1, Point p2);
	float extract_featurework(Point p1, Point p2);
	float return_stdev();
	Pair return_extrema();
	unsigned pushstack_bmp();
	unsigned pushstack_work();
	unsigned pushstack_help();
	unsigned pushstack_tissue(tissuelayers_size_t idx, tissues_size_t tissuenr);
	bool savestack(unsigned i, const char *filename);
	unsigned loadstack(const char *filename);
	void removestack(unsigned i);
	float *getstack(unsigned i, unsigned char &mode);
	void getstack_bmp(unsigned i);
	void getstack_work(unsigned i);
	void getstack_help(unsigned i);
	void getstack_tissue(tissuelayers_size_t idx, unsigned i, tissues_size_t tissuenr, bool override);
	void popstack_bmp();
	void popstack_work();
	void popstack_help();
	void clear_stack();
	bool isloaded();
	void correct_outline(float f, std::vector<Point> *newline);
	void correct_outlinetissue(tissuelayers_size_t idx, tissues_size_t f1, std::vector<Point> *newline);
	void brush(float f, Point p, int radius, bool draw);
	void brush(float f, Point p, float radius, float dx, float dy, bool draw);
	void brushtissue(tissuelayers_size_t idx, tissues_size_t f, Point p, int radius, bool draw, tissues_size_t f1);
	void brushtissue(tissuelayers_size_t idx, tissues_size_t f, Point p, float radius, float dx, float dy, bool draw, tissues_size_t f1);
	void fill_holes(float f, int minsize);
	void fill_holestissue(tissuelayers_size_t idx, tissues_size_t f, int minsize);
	void remove_islands(float f, int minsize);
	void remove_islandstissue(tissuelayers_size_t idx, tissues_size_t f, int minsize);
	float add_skin(unsigned i);
	void add_skin(unsigned i, float setto);
	void add_skintissue(tissuelayers_size_t idx, unsigned i4, tissues_size_t setto);
	float add_skin_outside(unsigned i);
	void add_skin_outside(unsigned i, float setto);
	void add_skintissue_outside(tissuelayers_size_t idx, unsigned i4, tissues_size_t setto);
	bool value_at_boundary(float value);
	bool tissuevalue_at_boundary(tissuelayers_size_t idx, tissues_size_t value);
	void fill_skin(int thicknessX, int thicknessY, tissues_size_t backgroundID, tissues_size_t skinID);
	void flood_exterior(float setto);
	void flood_exteriortissue(tissuelayers_size_t idx, tissues_size_t setto);
	void fill_unassigned(float setto);
	void fill_unassigned();
	void fill_unassignedtissue(tissuelayers_size_t idx, tissues_size_t setto);
	void add_vm(std::vector<mark> *vm1);
	void clear_vvm();
	bool del_vm(Point p, short radius);
	std::vector<std::vector<mark> > *return_vvm();
	unsigned return_vvmmaxim();
	void copy2vvm(std::vector<std::vector<mark> > *vvm1);
	void add_limit(std::vector<Point> *vp1);
	void clear_limits();
	bool del_limit(Point p, short radius);
	std::vector<std::vector<Point> > *return_limits();
	void copy2limits(std::vector<std::vector<Point> > *limits1);
	void permute_tissue_indices(tissues_size_t *indexMap);
	void remove_tissue(tissues_size_t tissuenr, tissues_size_t tissuecount1);
	void group_tissues(tissuelayers_size_t idx, std::vector<tissues_size_t> &olds, std::vector<tissues_size_t> &news);
	/*		void locktissue(tissues_size_t nr);
			void unlocktissue(tissues_size_t nr);
			bool islockedtissue(tissues_size_t nr);
			void set_tissuelock(tissues_size_t nr, bool setval);*/
	unsigned char return_mode(bool bmporwork);
	void set_mode(unsigned char mode, bool bmporwork);
	bool print_amascii_slice(tissuelayers_size_t idx, std::ofstream &streamname);
	bool print_vtkascii_slice(tissuelayers_size_t idx, std::ofstream &streamname);
	bool print_vtkbinary_slice(tissuelayers_size_t idx, std::ofstream &streamname);
	void adaptwork2bmp(float f);
	void shifttissue();
	void shiftbmp();
	unsigned long return_workpixelcount(float f);
	unsigned long return_tissuepixelcount(tissuelayers_size_t idx, tissues_size_t c);
	void swap(bmphandler &bmph);
	bool get_extent(tissuelayers_size_t idx, tissues_size_t tissuenr, unsigned short extent[2][2]);
	bool unwrap(float jumpratio, float range = 0, float shift = 0);

	unsigned getfirststackindexxxxxxxxx();

	int ConvertImageTo8BitBMP(const char *filename, unsigned char *&bits_tmp);
	int ConvertPNGImageTo8BitBMP(const char *filename, unsigned char *&bits_tmp);
	void SetRGBtoGrayScaleFactors(double newRedFactor, double newGreenFactor, double newBlueFactor);
	void mergetissue(tissues_size_t tissuetype, tissuelayers_size_t idx);
	void tissue2mc(tissuelayers_size_t idx, tissues_size_t tissuetype, unsigned char ** voxels, int k);

protected:
	//		static bool lockedtissues[TISSUES_SIZE_MAX+1];
	static std::list<unsigned> stackindex;
	static unsigned stackcounter;
	contour_class contour;
	std::vector<mark> marks;
	short unsigned width;
	short unsigned height;
	unsigned int area;
	inline unsigned int pt2coord(Point p);
	float *make_gaussfilter(float sigma, int n);
	float *make_laplacianfilter();
	unsigned deepest_con_bas(unsigned k);
	void cond_merge(unsigned m, unsigned h);
	unsigned label_lookup(unsigned i, unsigned *wshed);
	int SaveDIBitmap(const char *filename, float *p_bits);
	int SaveRaw(const char *filename, float *p_bits);
	void bucketsort(std::vector<unsigned int> *sorted, float *p_bits);
	void set_marker(unsigned *wshed);
	float base_connection(float c, std::vector<float> *maps);
	void hysteretic_growth(float *pict, std::vector<int> *s, unsigned short w, unsigned short h, bool connectivity, float set_to);
	void hysteretic_growth(float *pict, std::vector<int> *s, unsigned short w, unsigned short h, bool connectivity, float set_to, int nr);
	template<typename T, typename F> void _brush(T* data, T f, Point p, int radius, bool draw, T f1, F);
	template<typename T, typename F> void _brush(T* data, T f, Point p, float radius, float dx, float dy, bool draw, T f1, F);

	//		int infosize;
	//		BITMAPINFO *bmpinfo;
	unsigned int histogram[256];
	float *bmp_bits;
	float *work_bits;
	float *help_bits;
	std::vector<tissues_size_t *> tissuelayers;
	wshed_obj wshedobj;
	bool bmp_is_grey;
	bool work_is_grey;
	bool loaded;
	bool ownsliceprovider;
	feature_extractor fextract;
	static std::list<float *> bits_stack;
	static std::list<unsigned char> mode_stack;
	//		sliceprovider<float> *sliceprovide;
	//		sliceprovider_installer<float> *sliceprovide_installer;
	//		FloatSliceProviderInstaller *sliceprovide_installer;
	sliceprovider *sliceprovide;
	sliceprovider_installer *sliceprovide_installer;
	std::vector<std::vector<mark> > vvm;
	unsigned maxim_store;
	std::vector<std::vector<Point> > limits;
	unsigned char mode1;
	unsigned char mode2;

	double redFactor;
	double greenFactor;
	double blueFactor;
};

float f1(float dI, float k);
float f2(float dI, float k);
