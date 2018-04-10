/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef WIDGETCOLLECTION
#define WIDGETCOLLECTION

#include "Plugin/DataSelection.h"
#include "Plugin/Point.h"

#include "Core/Types.h"

#include <Q3HBoxLayout>
#include <Q3VBoxLayout>
#include <QDir>
#include <QPaintEvent>
#include <QTableWidget>
#include <QXmlDefaultHandler>
#include <QXmlSimpleReader>
#include <QXmlStreamWriter>
#include <q3hbox.h>
#include <q3listbox.h>
#include <q3vbox.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qdialog.h>
#include <qgroupbox.h>
#include <qimage.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qlist.h>
#include <qlistwidget.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qslider.h>
#include <qspinbox.h>
#include <qtreewidget.h>
#include <qwidget.h>

#include <map>
#include <set>

namespace iseg {

class SlicesHandler;
class TissueHiearchy;
class TissueHierarchyItem;
class bmphandler;

//class ScaleWork : public QWidget
class ScaleWork : public QDialog
{
	Q_OBJECT
public:
	ScaleWork(SlicesHandler *hand3D, QDir picpath, QWidget *parent = 0,
			const char *name = 0, Qt::WindowFlags wFlags = 0);
	~ScaleWork();

private:
	bmphandler *bmphand;
	SlicesHandler *handler3D;
	unsigned short activeslice;
	Q3HBox *hbox1;
	Q3HBox *hbox2;
	Q3HBox *hbox3;
	Q3HBox *hbox4;
	Q3VBox *vbox1;
	QPushButton *getRange;
	QPushButton *doScale;
	QPushButton *doCrop;
	QSlider *sl_brighness;
	QSlider *sl_contrast;
	QLineEdit *limitLow;
	QLineEdit *limitHigh;
	QLabel *lL;
	QLabel *lH;
	QLabel *lb_brightness;
	QLabel *lb_contrast;
	QCheckBox *allslices;
	float minval, maxval;
	float minval1, maxval1;
	bool dontundo;
	QPushButton *closeButton;

signals:
	void begin_datachange(iseg::DataSelection &dataSelection,
			QWidget *sender = NULL, bool beginUndo = true);
	void end_datachange(QWidget *sender = NULL,
			iseg::EndUndoAction undoAction = iseg::EndUndo);

public slots:
	void slicenr_changed();

private slots:
	void bmphand_changed(bmphandler *bmph);
	void getrange_pushed();
	void scale_pushed();
	void crop_pushed();
	void slider_changed(int newval);
	void slider_pressed();
	void slider_released();
};

class ImageMath : public QDialog
{
	Q_OBJECT
public:
	ImageMath(SlicesHandler *hand3D, QWidget *parent = 0, const char *name = 0,
			Qt::WindowFlags wFlags = 0);
	~ImageMath();

private:
	bmphandler *bmphand;
	SlicesHandler *handler3D;
	unsigned short activeslice;
	Q3HBox *hbox1;
	Q3HBox *hbox2;
	Q3HBox *hbox3;
	Q3VBox *vbox1;
	QPushButton *doAdd;
	QPushButton *doSub;
	QPushButton *doMult;
	QPushButton *doNeg;
	QButtonGroup *imgorval;
	QRadioButton *rb_img;
	QRadioButton *rb_val;
	float val;
	QLineEdit *le_val;
	QLabel *lb_val;
	QCheckBox *allslices;
	QPushButton *closeButton;

signals:
	void begin_datachange(iseg::DataSelection &dataSelection,
			QWidget *sender = NULL, bool beginUndo = true);
	void end_datachange(QWidget *sender = NULL,
			iseg::EndUndoAction undoAction = iseg::EndUndo);

public slots:
	void slicenr_changed();

private slots:
	void bmphand_changed(bmphandler *bmph);
	void add_pushed();
	void sub_pushed();
	void mult_pushed();
	void neg_pushed();
	void value_changed();
	void imgorval_changed(int);
};

class ImageOverlay : public QDialog
{
	Q_OBJECT
public:
	ImageOverlay(SlicesHandler *hand3D, QWidget *parent = 0,
			const char *name = 0, Qt::WindowFlags wFlags = 0);
	~ImageOverlay();

protected:
	virtual void closeEvent(QCloseEvent *e);

private:
	bmphandler *bmphand;
	SlicesHandler *handler3D;
	unsigned short activeslice;
	Q3VBox *vbox1;
	Q3HBox *hbox1;
	Q3HBox *hbox2;
	Q3HBox *hbox3;
	QLabel *lbAlpha;
	QSlider *slAlpha;
	QLineEdit *leAlpha;
	QCheckBox *allslices;
	QPushButton *closeButton;
	QPushButton *applyButton;
	float alpha;
	int sliderMax;
	unsigned short sliderPrecision;
	float *bkpWork;

signals:
	void begin_datachange(iseg::DataSelection &dataSelection,
			QWidget *sender = NULL, bool beginUndo = true);
	void end_datachange(QWidget *sender = NULL,
			iseg::EndUndoAction undoAction = iseg::EndUndo);

public slots:
	void newloaded();
	void slicenr_changed();

private slots:
	void bmphand_changed(bmphandler *bmph);
	void apply_pushed();
	void alpha_changed();
	void slider_changed(int newval);
};

class HistoWin : public QWidget
{
	Q_OBJECT
public:
	HistoWin(unsigned int *histo1, QWidget *parent = 0, const char *name = 0,
			Qt::WindowFlags wFlags = 0);
	void update();
	void histo_changed(unsigned int *histo1);

protected:
	void paintEvent(QPaintEvent *e);

private:
	QImage image;
	unsigned int *histo;
};

class ShowHisto : public QDialog
{
	Q_OBJECT
public:
	ShowHisto(SlicesHandler *hand3D, QWidget *parent = 0, const char *name = 0,
			Qt::WindowFlags wFlags = 0);
	~ShowHisto();

private:
	bmphandler *bmphand;
	SlicesHandler *handler3D;
	unsigned short activeslice;
	Q3HBox *hbox1;
	Q3HBox *hbox2;
	Q3HBox *hbox3;
	Q3HBox *hbox4;
	HistoWin *histwindow;
	Q3VBox *vbox1;
	Q3VBox *vbox2;
	QPushButton *closeButton;
	QPushButton *updateSubsect;
	QSpinBox *xoffset;
	QSpinBox *yoffset;
	QSpinBox *xlength;
	QSpinBox *ylength;
	QCheckBox *subsect;
	QLabel *xoffs;
	QLabel *yoffs;
	QLabel *xl;
	QLabel *yl;
	QButtonGroup *pictselect;
	QRadioButton *workpict;
	QRadioButton *bmppict;

public slots:
	void slicenr_changed();
	void newloaded();

private slots:
	void bmphand_changed(bmphandler *bmph);
	void draw_histo();
	void subsect_toggled();
	void pict_toggled(bool on);
	void subsect_update();
};

class colorshower : public QWidget
{
	Q_OBJECT
public:
	colorshower(int lx1, int ly1, QWidget *parent = 0, const char *name = 0,
			Qt::WindowFlags wFlags = 0);

protected:
	void paintEvent(QPaintEvent *e);

private:
	int lx;
	int ly;
	float fr;
	float fg;
	float fb;
	float opac;

private slots:
	void color_changed(float fr1, float fg1, float fb1, float opac1);
};

class TissueTreeWidget : public QTreeWidget
{
	Q_OBJECT
public:
	TissueTreeWidget(TissueHiearchy *hierarchy, QDir picpath,
			QWidget *parent = 0);
	~TissueTreeWidget();

public:
	void initialize();

	void set_tissue_filter(const QString &filter);
	void update_visibility();
	void update_visibility_recursive(QTreeWidgetItem *item);

	// Add tree items
	void insert_item(bool isFolder, const QString &name);

	// Delete tree items
	void remove_tissue(const QString &name);
	void remove_current_item(bool removeChildren = false);
	void remove_all_folders(bool removeChildren = false);

	// Update items
	void update_tissue_name(const QString &oldName, const QString &newName);
	void update_tissue_icons(QTreeWidgetItem *parent = 0);
	void update_folder_icons(QTreeWidgetItem *parent = 0);
	void set_current_folder_name(const QString &name);

	// Current item
	void set_current_item(QTreeWidgetItem *item);
	void set_current_tissue(tissues_size_t type);
	bool get_current_is_folder();
	tissues_size_t get_current_type();
	QString get_current_name();
	bool get_current_has_children();
	void get_current_child_tissues(
			std::map<tissues_size_t, unsigned short> &types);

	// Hierarchy
	void
			update_tree_widget(); // Updates QTreeWidget from internal representation
	void update_hierarchy();	// Updates internal representation from QTreeWidget
	unsigned short get_selected_hierarchy();
	unsigned short get_hierarchy_count();
	std::vector<QString> *get_hierarchy_names_ptr();
	QString get_current_hierarchy_name();
	void reset_default_hierarchy();
	void set_hierarchy(unsigned short index);
	void add_new_hierarchy(const QString &name);
	void remove_current_hierarchy();
	bool get_hierarchy_modified();
	void set_hierarchy_modified(bool val);
	unsigned short get_tissue_instance_count(tissues_size_t type);
	void get_sublevel_child_tissues(
			std::map<tissues_size_t, unsigned short> &types);

	// File IO
	FILE *SaveParams(FILE *fp, int version);
	FILE *LoadParams(FILE *fp, int version);
	bool load_hierarchy(const QString &path);
	bool save_hierarchy_as(const QString &name, const QString &path);

	// Display
	bool get_tissue_indices_hidden();

	QList<QTreeWidgetItem *> get_all_items();

public slots:
	void toggle_show_tissue_indices();
	void sort_by_tissue_name();
	void sort_by_tissue_index();
	tissues_size_t get_type(QTreeWidgetItem *item);
	QString get_name(QTreeWidgetItem *item);

protected:
	// Drag & drop
	void dropEvent(QDropEvent *de);

private:
	void resize_columns_to_contents();
	bool get_is_folder(QTreeWidgetItem *item);

	QTreeWidgetItem *find_tissue_item(tissues_size_t type,
			QTreeWidgetItem *parent = 0);
	void take_children_recursively(QTreeWidgetItem *parent,
			QList<QTreeWidgetItem *> &appendTo);
	void get_child_tissues_recursively(
			QTreeWidgetItem *parent,
			std::map<tissues_size_t, unsigned short> &types);
	unsigned short
			get_tissue_instance_count_recursively(QTreeWidgetItem *parent,
					tissues_size_t type);
	void insert_item(bool isFolder, const QString &name,
			QTreeWidgetItem *insertAbove);
	void insert_item(bool isFolder, const QString &name,
			QTreeWidgetItem *parent, unsigned int index);
	void remove_tissue_recursively(QTreeWidgetItem *parent,
			const QString &name);
	void update_tissue_name_widget(const QString &oldName,
			const QString &newName,
			QTreeWidgetItem *parent = 0);
	short get_child_lockstates(QTreeWidgetItem *folder);
	void pad_tissue_indices();
	void pad_tissue_indices_recursively(QTreeWidgetItem *parent,
			unsigned short digits);
	void update_tissue_indices();
	void update_tissue_indices_recursively(QTreeWidgetItem *parent);

	// Convert QTreeWidget to internal representation
	TissueHierarchyItem *create_current_hierarchy();
	void create_hierarchy_recursively(QTreeWidgetItem *parentIn,
			TissueHierarchyItem *parentOut);
	TissueHierarchyItem *create_hierarchy_item(QTreeWidgetItem *item);

	// Convert internal representation to QTreeWidget
	void build_tree_widget(TissueHierarchyItem *root);
	void build_tree_widget_recursively(TissueHierarchyItem *parentIn,
			QTreeWidgetItem *parentOut,
			std::set<tissues_size_t> *tissueTypes);
	QTreeWidgetItem *create_hierarchy_item(bool isFolder, const QString &name);

	// SaveParams
	FILE *save_hierarchy(FILE *fp, unsigned short idx);
	FILE *load_hierarchy(FILE *fp);

private:
	TissueHiearchy *hierarchies;
	std::string tissue_filter;
	QDir picturePath;
	bool modified;
	bool sortByNameAscending;
	bool sortByTypeAscending;

signals:
	void hierarchy_list_changed(void);

private slots:
	void resize_columns_to_contents(QTreeWidgetItem *item);
};

class TissueAdder : public QDialog
{
	Q_OBJECT
public:
	TissueAdder(bool modifyTissue, TissueTreeWidget *tissueTree,
			QWidget *parent = 0, const char *name = 0,
			Qt::WindowFlags wFlags = 0);
	~TissueAdder();
	//	FILE *save_proj(FILE *fp);

private:
	bool modify;

	Q3HBoxLayout *hbox1;
	Q3HBoxLayout *hbox2;
	Q3HBoxLayout *hbox3;
	/*	QHBoxLayout *hboxr;
			QHBoxLayout *hboxg;
			QHBoxLayout *hboxb;*/
	colorshower *cs;
	Q3VBoxLayout *vbox1;
	Q3VBoxLayout *vbox2;
	Q3VBoxLayout *vbox3;
	Q3VBoxLayout *vbox4;
	QPushButton *closeButton;
	//	QPushButton *colorButton;
	QPushButton *addTissue;
	QLabel *red;
	QLabel *green;
	QLabel *blue;
	QLabel *opac;
	QLabel *tissuename;
	QLineEdit *nameField;
	QSlider *r;
	QSlider *g;
	QSlider *b;
	QSlider *sl_transp;
	QSpinBox *sb_r;
	QSpinBox *sb_g;
	QSpinBox *sb_b;
	QSpinBox *sb_transp;
	float fr1;
	float fg1;
	float fb1;
	float transp1;
	TissueTreeWidget *tissueTreeWidget;
	//	float **tc;

signals:
	void color_changed(float fr, float fg, float fb, float opac2);

private slots:
	void update_color_r(int v);
	void update_color_g(int v);
	void update_color_b(int v);
	void update_opac(int v);
	void update_color_rsb(int v);
	void update_color_gsb(int v);
	void update_color_bsb(int v);
	void update_opacsb(int v);
	void add_pressed();
};

class TissueFolderAdder : public QDialog
{
	Q_OBJECT
public:
	TissueFolderAdder(TissueTreeWidget *tissueTree, QWidget *parent = 0,
			const char *name = 0, Qt::WindowFlags wFlags = 0);
	~TissueFolderAdder();

private:
	Q3HBoxLayout *hboxOverall;
	Q3VBoxLayout *vboxOverall;
	Q3HBoxLayout *hboxFolderName;
	Q3HBoxLayout *hboxPushButtons;
	QLabel *nameLabel;
	QLineEdit *nameLineEdit;
	QPushButton *addButton;
	QPushButton *closeButton;
	TissueTreeWidget *tissueTreeWidget;

private slots:
	void add_pressed();
};

class TissueHierarchyWidget : public QWidget
{
	Q_OBJECT
public:
	TissueHierarchyWidget(TissueTreeWidget *tissueTree, QWidget *parent = 0,
			Qt::WindowFlags wFlags = 0);
	~TissueHierarchyWidget();

	bool handle_changed_hierarchy();

private:
	QDir picturePath;
	Q3HBoxLayout *hboxOverall;
	Q3VBoxLayout *vboxOverall;
	Q3VBoxLayout *vboxHierarchyButtons;
	QComboBox *hierarchyComboBox;
	QPushButton *newHierarchyButton;
	QPushButton *loadHierarchyButton;
	QPushButton *saveHierarchyAsButton;
	QPushButton *removeHierarchyButton;
	TissueTreeWidget *tissueTreeWidget;

public slots:
	void update_hierarchy_combo_box();
	bool save_hierarchy_as_pressed();

private slots:
	void hierarchy_changed(int index);
	void new_hierarchy_pressed();
	void load_hierarchy_pressed();
	void remove_hierarchy_pressed();
};

class bits_stack : public QWidget
{
	Q_OBJECT
public:
	bits_stack(SlicesHandler *hand3D, QWidget *parent = 0, const char *name = 0,
			Qt::WindowFlags wFlags = 0);
	~bits_stack();
	QMap<QString, unsigned int> *return_bitsnr();
	void newloaded();
	FILE *save_proj(FILE *fp);
	FILE *load_proj(FILE *fp);
	void clear_stack();

protected:
	void push_helper(bool source, bool target, bool tissue);
	void pop_helper(bool source, bool target, bool tissue);

private:
	unsigned short oldw, oldh;
	SlicesHandler *handler3D;
	QListWidget *bits_names;
	Q3HBoxLayout *hbox1;
	Q3VBoxLayout *vbox1;
	QPushButton *pushwork;
	QPushButton *pushbmp;
	QPushButton *pushtissue;
	QPushButton *popwork;
	QPushButton *popbmp;
	QPushButton *loaditem;
	QPushButton *saveitem;
	QPushButton *poptissue;
	QPushButton *deletebtn;
	QMap<QString, unsigned int> bits_nr;
	tissues_size_t tissuenr;

signals:
	void stack_changed();
	void begin_datachange(iseg::DataSelection &dataSelection,
			QWidget *sender = NULL, bool beginUndo = true);
	void end_datachange(QWidget *sender = NULL,
			iseg::EndUndoAction undoAction = iseg::EndUndo);
	void begin_dataexport(iseg::DataSelection &dataSelection,
			QWidget *sender = NULL);
	void end_dataexport(QWidget *sender = NULL);

public slots:
	void tissuenr_changed(unsigned short i);

private slots:
	void pushwork_pressed();
	void pushbmp_pressed();
	void pushtissue_pressed();
	void popwork_pressed();
	void popbmp_pressed();
	void poptissue_pressed();
	void saveitem_pressed();
	void loaditem_pressed();
	void delete_pressed();
};

class bits_stack_pushdialog : public QDialog
{
	Q_OBJECT
public:
	bits_stack_pushdialog(QWidget *parent = 0, const char *name = 0,
			Qt::WindowFlags wFlags = 0);
	~bits_stack_pushdialog();
	bool get_pushcurrentslice();
	unsigned int get_startslice(bool *ok);
	unsigned int get_endslice(bool *ok);

private:
	Q3VBox *vboxoverall;
	Q3HBox *hboxparams;
	Q3VBox *vboxsliceselection;
	Q3VBox *vboxdelimiter;
	Q3HBox *hboxslicerange;
	Q3VBox *vboxslicerangelabels;
	Q3VBox *vboxslicerangelineedits;
	Q3HBox *hboxpushbuttons;
	QRadioButton *rb_currentslice;
	QRadioButton *rb_multislices;
	QButtonGroup *slicegroup;
	QLabel *lb_startslice;
	QLabel *lb_endslice;
	QLineEdit *le_startslice;
	QLineEdit *le_endslice;
	QPushButton *pushexec;
	QPushButton *pushcancel;

private slots:
	void sliceselection_changed();
};

class extoverlay_widget : public QWidget
{
	Q_OBJECT
public:
	extoverlay_widget(SlicesHandler *hand3D, QWidget *parent = 0,
			const char *name = 0, Qt::WindowFlags wFlags = 0);
	~extoverlay_widget();
	void newloaded();

private:
	bmphandler *bmphand;
	SlicesHandler *handler3D;
	unsigned short activeslice;

	unsigned short selectedDataset;
	std::vector<QString> datasetNames;
	std::vector<QString> datasetFilepaths;

	float alpha;
	int sliderMax;
	unsigned short sliderPrecision;

	Q3HBoxLayout *hboxOverall;
	Q3VBoxLayout *vboxOverall;
	Q3HBoxLayout *hboxAlpha;
	Q3HBoxLayout *hboxDisplaySrcTgt;
	QComboBox *datasetComboBox;
	QPushButton *loadDatasetButton;
	QLabel *lbAlpha;
	QSlider *slAlpha;
	QLineEdit *leAlpha;
	QCheckBox *srcCheckBox;
	QCheckBox *tgtCheckBox;

protected:
	void initialize();
	void add_dataset(const QString &path);
	void remove_dataset(unsigned short idx);
	void reload_overlay();

signals:
	void overlay_changed();
	void overlayalpha_changed(float newValue);
	void bmpoverlayvisible_changed(bool newValue);
	void workoverlayvisible_changed(bool newValue);

public slots:
	void slicenr_changed();

private slots:
	void dataset_changed(int index);
	void load_dataset_pressed();
	void alpha_changed();
	void slider_changed(int newval);
	void source_toggled();
	void target_toggled();
};

class MultiDataset_widget : public QWidget
{
	Q_OBJECT

public:
	struct SDatasetInfo
	{
		QRadioButton *m_RadioButton;
		QString m_RadioButtonText;
		QStringList m_DatasetFilepath;
		unsigned m_Width;
		unsigned m_Height;
		std::vector<float *> m_BmpSlices;
		std::vector<float *> m_WorkSlices;
		bool m_IsActive;
	};

	enum DatasetTypeEnum {
		DCM,
		BMP,
		PNG,
		RAW,
		MHD,
		AVW,
		VTK,
		XDMF,
		NIFTI,
		RTDOSE
	};

	MultiDataset_widget(SlicesHandler *hand3D, QWidget *parent = 0,
			const char *name = 0, Qt::WindowFlags wFlags = 0);
	~MultiDataset_widget();
	void NewLoaded();
	int GetNumberOfDatasets();
	bool IsActive(const int multiDS_index);
	bool IsChecked(const int multiDS_index);

	std::vector<float *> GetBmpData(const int multiDS_index);
	void SetBmpData(const int multiDS_index, std::vector<float *> bmp_bits_vc);

	std::vector<float *> GetWorkingData(const int multiDS_index);
	void SetWorkingData(const int multiDS_index,
			std::vector<float *> work_bits_vc);

protected:
	void Initialize();
	void InitializeMap();
	void ClearRadioButtons();

	bool
			CheckInfoAndAddToList(MultiDataset_widget::SDatasetInfo &newRadioButton,
					QStringList loadfilenames, unsigned short width,
					unsigned short height, unsigned short nrofslices);
	bool AddDatasetToList(MultiDataset_widget::SDatasetInfo &newRadioButton,
			QStringList loadfilenames);
	void CopyImagesSlices(SlicesHandler *handler3D,
			MultiDataset_widget::SDatasetInfo &newRadioButton,
			const bool saveOnlyWorkingBits = false);

signals:
	void begin_datachange(iseg::DataSelection &dataSelection,
			QWidget *sender = NULL, bool beginUndo = true);
	void end_datachange(QWidget *sender = NULL,
			iseg::EndUndoAction undoAction = iseg::EndUndo);
	void dataset_changed();

private slots:
	void AddDatasetPressed();
	void SwitchDataset();
	void ChangeDatasetName();
	void RemoveDataset();
	void DatasetSelectionChanged();

private:
	SlicesHandler *m_Handler3D;
	std::vector<SDatasetInfo> m_RadioButtons;

	Q3HBoxLayout *hboxOverall;
	Q3VBoxLayout *vboxOverall;
	Q3VBoxLayout *m_VboxOverall;
	Q3VBoxLayout *m_VboxDatasets;
	QGroupBox *m_DatasetsGroupBox;
	QPushButton *m_AddDatasetButton;
	QPushButton *m_LoadDatasetButton;
	QPushButton *m_ChangeNameButton;
	QPushButton *m_RemoveDatasetButton;

	bool m_ItIsBeingLoaded;
	std::map<std::string, DatasetTypeEnum> m_MapDatasetValues;
};

class ZoomWidget : public QWidget
{
	Q_OBJECT
public:
	ZoomWidget(double zoom1, QDir picpath, QWidget *parent = 0,
			const char *name = 0, Qt::WindowFlags wFlags = 0);
	~ZoomWidget();
	double get_zoom();

public slots:
	void zoom_changed(double);

private:
	double zoom;
	Q3VBoxLayout *vbox1;
	Q3HBoxLayout *hbox1;
	QPushButton *pushzoomin;
	QPushButton *pushzoomout;
	QPushButton *pushunzoom;
	QLineEdit *le_zoom_f;
	QLabel *zoom_f;

signals:
	void zoom_in();
	void zoom_out();
	void unzoom();
	void set_zoom(double);

private slots:
	//	void zoom_changed(double);
	void zoomin_pushed();
	void zoomout_pushed();
	void unzoom_pushed();
	void le_zoom_changed();
};

class CleanerParams : public QDialog
{
	Q_OBJECT
public:
	CleanerParams(int *rate1, int *minsize1, QWidget *parent = 0,
			const char *name = 0, Qt::WindowFlags wFlags = 0);
	~CleanerParams();

private:
	int *rate;
	int *minsize;
	Q3HBox *hbox1;
	Q3VBox *vbox1;
	Q3VBox *vbox2;
	QPushButton *pb_doit;
	QPushButton *pb_dontdoit;
	QLabel *lb_rate;
	QLabel *lb_minsize;
	QSpinBox *sb_rate;
	QSpinBox *sb_minsize;

private slots:
	void doit_pressed();
	void dontdoit_pressed();
};

class MergeProjectsDialog : public QDialog
{
	Q_OBJECT
public:
	MergeProjectsDialog(QWidget *parent = 0, const char *name = 0,
			Qt::WindowFlags wFlags = 0);
	~MergeProjectsDialog();

	void get_filenames(std::vector<QString> &filenames);

private:
	Q3HBoxLayout *hboxOverall;
	Q3VBoxLayout *vboxFileList;
	Q3VBoxLayout *vboxButtons;
	Q3VBoxLayout *vboxEditButtons;
	Q3VBoxLayout *vboxExecuteButtons;
	QListWidget *fileListWidget;
	QPushButton *addButton;
	QPushButton *removeButton;
	QPushButton *moveUpButton;
	QPushButton *moveDownButton;
	QPushButton *executeButton;
	QPushButton *cancelButton;

private slots:
	void add_pressed();
	void remove_pressed();
	void move_up_pressed();
	void move_down_pressed();
};

class CheckBoneConnectivityDialog : public QWidget
{
	Q_OBJECT

	enum BoneConnectionColumn {
		kTissue1,
		kTissue2,
		kSliceNumber,
		kColumnNumber
	};

	struct BoneConnectionInfo
	{
		BoneConnectionInfo(const tissues_size_t tis1, const tissues_size_t tis2,
				const int slice)
				: TissueID1(tis1), TissueID2(tis2), SliceNumber(slice)
		{
		}

		tissues_size_t TissueID1;
		tissues_size_t TissueID2;
		int SliceNumber;

		bool operator<(const BoneConnectionInfo &a) const
		{
			if (SliceNumber < a.SliceNumber)
			{
				return true;
			}
			else if (SliceNumber > a.SliceNumber)
			{
				return false;
			}
			else
			{
				return TissueID1 < a.TissueID1;
			}
		}

		bool operator==(const BoneConnectionInfo &a) const
		{
			return (SliceNumber == a.SliceNumber &&
							((TissueID1 == a.TissueID1 && TissueID2 == a.TissueID2) ||
									(TissueID1 == a.TissueID2 && TissueID2 == a.TissueID1)));
		}
	};

public:
	CheckBoneConnectivityDialog(SlicesHandler *hand3D, const char *name,
			QWidget *parent = 0,
			Qt::WindowFlags wFlags = 0);
	~CheckBoneConnectivityDialog();

signals:
	void slice_changed();

private:
	void ShowText(const std::string &text);
	void CheckBoneExist();
	void LookForConnections();
	void FillConnectionsTable();

	bool IsBone(const std::string &label_name) const;

private:
	SlicesHandler *handler3D;

	Q3HBox *mainBox;
	Q3VBox *vbox1;
	Q3HBox *hbox1;
	Q3HBox *hbox2;
	Q3HBox *hbox3;
	Q3HBox *hbox4;

	QCheckBox *bonesFoundCB;

	QPushButton *executeButton;
	QPushButton *cancelButton;
	QPushButton *exportButton;

	QTableWidget *foundConnectionsTable;

	QLabel *progressText;

	std::vector<BoneConnectionInfo> foundConnections;

	Q3HBoxLayout *hboxOverall;
	Q3VBoxLayout *vboxFileList;
	Q3VBoxLayout *vboxButtons;
	Q3VBoxLayout *vboxEditButtons;
	Q3VBoxLayout *vboxExecuteButtons;
	QListWidget *fileListWidget;

private slots:
	void cellClicked(int row, int col);
	void execute_pressed();
	void cancel_pressed();
	void export_pressed();
};

} // namespace iseg

#endif
