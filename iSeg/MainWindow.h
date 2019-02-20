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

#include "Atlas.h"
#include "Project.h"

#include "Data/DataSelection.h"
#include "Data/Point.h"

#include <qdir.h>
#include <qmainwindow.h>
#include <qmenu.h>

class QLabel;
class QTextEdit;
class QLineEdit;
class QMenuBar;
class QWidget;
class QVBoxLayout;
class QHBoxLayout;
class QPushButton;
class QCheckBox;
class QSpinBox;
class QSlider;
class QTreeWidgetItem;
class QSignalMapper;
class QCloseEvent;
class QDockWidget;

class QStackedWidget;
class QScrollBar;
class Q3ScrollView;
class Q3PopupMenu;
class Q3Action;
class Q3Accel;

namespace iseg {

class SlicesHandler;
class WidgetInterface;
class TissueTreeWidget;
class TissueHierarchyWidget;
class bits_stack;
class extoverlay_widget;
class MultiDatasetWidget;
class ScaleWork;
class ImageMath;
class ImageOverlay;
class ZoomWidget;
class CheckBoneConnectivityDialog;
class SliceViewerWidget;
class PickerWidget;
class InterpolationWidget;
class FeatureWidget;
class OutlineCorrectionWidget;
class ImageForestingTransformRegionGrowingWidget;
class TransformWidget;
class FastmarchingFuzzyWidget;
class LivewireWidget;
class EdgeWidget;
class MorphologyWidget;
class HystereticGrowingWidget;
class VesselWidget;
class WatershedWidget;
class SmoothingWidget;
class ThresholdWidget;
class MeasurementWidget;
class ImageViewerWidget;
class SurfaceViewerWidget;
class VolumeViewerWidget;

class MenuWTT : public QMenu
{
	Q_OBJECT
public:
	MenuWTT() {}
	bool event(QEvent* e) override;
};

class MainWindow : public QMainWindow
{
	Q_OBJECT
public:
	MainWindow(SlicesHandler* hand3D, const QString& locationstring, const QDir& picpath,
			const QDir& tmppath, bool editingmode = false, QWidget* parent = nullptr,
			const char* name = nullptr, Qt::WindowFlags wFlags = 0,
			const std::vector<std::string>& plugin_search_dirs = std::vector<std::string>());
	~MainWindow() {}

	friend class Settings;

	void LoadLoadProj(const QString& path1);
	void LoadAtlas(const QDir& path1);
	void AddLoadProj(const QString& path1);
	void SaveLoadProj(const QString& latestprojpath);
	void SaveSettings();
	void LoadSettings(const char* loadfilename);
	void loadproj(const QString& loadfilename);
	void loadS4Llink(const QString& loadfilename);

protected:
	void start_surfaceviewer(int mode);
	void closeEvent(QCloseEvent*);
	bool maybeSafe();
	bool modified();
	void modifTissue();
	void modifFolder();
	void end_undo_helper(iseg::EndUndoAction undoAction);
	void cancel_transform_helper();
	void update_ranges_helper();
	void pixelsize_changed();
	void do_undostepdone();
	void do_clearundo();
	void reset_brightnesscontrast();
	void update_brightnesscontrast(bool bmporwork, bool paint = true);
	FILE* save_notes(FILE* fp, unsigned short version);
	FILE* load_notes(FILE* fp, unsigned short version);

signals:
	void bmp_changed();
	void work_changed();
	void marks_changed();
	void tissues_changed();
	void drawing_changed();
	void begin_datachange(iseg::DataSelection& dataSelection,
			QWidget* sender = nullptr, bool beginUndo = true);
	void end_datachange(QWidget* sender = nullptr,
			iseg::EndUndoAction undoAction = iseg::EndUndo);
	void begin_dataexport(iseg::DataSelection& dataSelection,
			QWidget* sender = nullptr);
	void end_dataexport(QWidget* sender = nullptr);

private:
	bool m_editingmode;
	bool m_Modified;

	std::string settingsfile;
	QString m_locationstring;
	QDir m_picpath;
	QDir m_tmppath;
	SlicesHandler* handler3D;
	ImageViewerWidget* bmp_show;
	ImageViewerWidget* work_show;
	QVBoxLayout* vboxtotal;
	QHBoxLayout* hbox1;
	QVBoxLayout* vbox1;
	QVBoxLayout* vboxbmp;
	QVBoxLayout* vboxwork;
	QHBoxLayout* hboxbmp;
	QHBoxLayout* hboxwork;
	QHBoxLayout* hboxbmp1;
	QHBoxLayout* hboxwork1;
	QHBoxLayout* hbox2;
	QVBoxLayout* vbox2;
	QVBoxLayout* vboxnotes;
	QVBoxLayout* rightvbox;
	QHBoxLayout* hboxlock;
	QHBoxLayout* hboxtissueadder;
	QVBoxLayout* vboxtissueadder1;
	QVBoxLayout* vboxtissueadder1b;
	QVBoxLayout* vboxtissueadder2;
	QCheckBox* cb_addsub3d;
	QCheckBox* cb_addsubconn;
	QCheckBox* cb_addsuboverride;
	QPushButton* pb_add;
	QPushButton* pb_sub;
	QPushButton* pb_addhold;
	QPushButton* pb_subhold;
	QPushButton* toworkBtn;
	QPushButton* tobmpBtn;
	QPushButton* swapBtn;
	QPushButton* swapAllBtn;
	QPushButton* pb_work2tissue;
	QMenuBar* menubar;
	MenuWTT* file;
	QMenu* imagemenu;
	QMenu* viewmenu;
	QMenu* toolmenu;
	QMenu* atlasmenu;
	QMenu* helpmenu;
	QMenu* editmenu;
	Q3PopupMenu* loadmenu;
	Q3PopupMenu* reloadmenu;
	Q3PopupMenu* exportmenu;
	Q3PopupMenu* saveprojasmenu;
	Q3PopupMenu* saveprojmenu;
	Q3PopupMenu* saveactiveslicesmenu;
	Q3PopupMenu* hidemenu;
	Q3PopupMenu* hidesubmenu;
	QDockWidget* tissuesDock;
	TissueTreeWidget* tissueTreeWidget; // Widget visualizing the tissue hierarchy
	QLineEdit* tissueFilter;
	TissueHierarchyWidget* tissueHierarchyWidget; // Widget for selecting the tissue hierarchy
	QCheckBox* cb_tissuelock;
	QPushButton* lockTissues;
	QPushButton* addTissue;
	QPushButton* addFolder;
	QPushButton* modifyTissueFolder;
	QPushButton* removeTissueFolder;
	QPushButton* removeTissueFolderAll;
	QPushButton* getTissue;
	QPushButton* getTissueAll;
	QPushButton* clearTissue;
	QPushButton* clearTissues;
	QWidget* menubarspacer;
	Q3Action* hideparameters;
	Q3Action* hidezoom;
	Q3Action* hidecontrastbright;
	Q3Action* hidecopyswap;
	Q3Action* hidestack;
	Q3Action* hidenotes;
	Q3Action* hidesource;
	Q3Action* hidetarget;
	std::vector<Q3Action*> showtab_action;
	QCheckBox* tissue3Dopt;
	QStackedWidget* methodTab;
	ThresholdWidget* threshold_widget;
	MeasurementWidget* measurement_widget;
	VesselWidget* vesselextr_widget;
	SmoothingWidget* smoothing_widget;
	EdgeWidget* edge_widget;
	MorphologyWidget* morphology_widget;
	WatershedWidget* watershed_widget;
	HystereticGrowingWidget* hyst_widget;
	LivewireWidget* livewire_widget;
	ImageForestingTransformRegionGrowingWidget* iftrg_widget;
	FastmarchingFuzzyWidget* fastmarching_widget;
	OutlineCorrectionWidget* olc_widget;
	FeatureWidget* feature_widget;
	InterpolationWidget* interpolation_widget;
	PickerWidget* picker_widget;
	TransformWidget* transform_widget;
	WidgetInterface* tab_old;
	ScaleWork* scale_dialog;
	ImageMath* imagemath_dialog;
	ImageOverlay* imageoverlay_dialog;
	bits_stack* bitstack_widget;
	extoverlay_widget* overlay_widget;
	MultiDatasetWidget* multidataset_widget;
	QCheckBox* cb_bmptissuevisible;
	QCheckBox* cb_bmpcrosshairvisible;
	QCheckBox* cb_bmpoutlinevisible;
	QCheckBox* cb_worktissuevisible;
	QCheckBox* cb_workcrosshairvisible;
	QCheckBox* cb_workpicturevisible;
	void clear_stack();
	void slice_changed();
	unsigned short nrslices;
	void slices3d_changed(bool new_bitstack);
	QLabel* lb_slicenr;
	QLabel* lb_inactivewarning;
	QSpinBox* sb_slicenr;
	QScrollBar* scb_slicenr;
	QPushButton* pb_first;
	QPushButton* pb_last;
	QLabel* lb_stride;
	QSpinBox* sb_stride;

	QLabel* lb_source;
	QLabel* lb_target;
	Q3ScrollView* bmp_scroller;
	Q3ScrollView* work_scroller;
	bool tomove_scroller;
	ZoomWidget* zoom_widget;
	//	float thickness;
	SliceViewerWidget* xsliceshower;
	SliceViewerWidget* ysliceshower;
	SurfaceViewerWidget* surface_viewer;
	VolumeViewerWidget* VV3D;
	VolumeViewerWidget* VV3Dbmp;
	int undonr;
	int redonr;
	QString m_saveprojfilename;
	QString m_S4Lcommunicationfilename;
	Project m_loadprojfilename;
	Atlas m_atlasfilename;
	QLabel* lb_contrastbmp;
	QLabel* lb_brightnessbmp;
	QLabel* lb_contrastwork;
	QLabel* lb_brightnesswork;
	QLabel* lb_contrastbmp_val;
	QLabel* lb_brightnessbmp_val;
	QLineEdit* le_contrastbmp_val;
	QLineEdit* le_brightnessbmp_val;
	QLabel* lb_contrastwork_val;
	QLabel* lb_brightnesswork_val;
	QLineEdit* le_contrastwork_val;
	QLineEdit* le_brightnesswork_val;
	QSlider* sl_contrastbmp;
	QSlider* sl_contrastwork;
	QSlider* sl_brightnessbmp;
	QSlider* sl_brightnesswork;
	Q3Accel* m_acc_undo;
	Q3Accel* m_acc_undo2;
	Q3Accel* m_acc_redo;
	Q3Accel* m_acc_sub;
	Q3Accel* m_acc_add;
	Q3Accel* m_acc_zoomout;
	Q3Accel* m_acc_zoomin;
	Q3Accel* m_acc_slicedown1;
	Q3Accel* m_acc_sliceup1;
	Q3Accel* m_acc_slicedown;
	Q3Accel* m_acc_sliceup;
	QTextEdit* m_notes;
	std::vector<QPushButton*> pb_tab;
	QSignalMapper* m_widget_signal_mapper;
	std::vector<bool> showpb_tab;
	void updateMethodButtonsPressed(WidgetInterface*);
	void updateTabvisibility();
	std::vector<WidgetInterface*> tabwidgets;
	void disconnect_mouseclick();
	void connect_mouseclick();
	QWidget* vboxbmpw;
	QWidget* vboxworkw;

	CheckBoneConnectivityDialog* boneConnectivityDialog;

	bool undoStarted;
	bool canUndo3D;
	iseg::DataSelection changeData;
	bool m_NewDataAfterSwap;

private slots:
	void update_bmp();
	void update_work();
	void update_tissue();
	void execute_bmp2work();
	void execute_work2bmp();
	void execute_swap_bmpwork();
	void execute_swap_bmpworkall();
	void execute_new();
	void execute_saveContours();
	void execute_loaddicom();
	void execute_loadbmp();
	void execute_loadpng();
	void execute_loadjpg();
	void execute_loadraw();
	void execute_loadavw();
	void execute_loadmhd();
	void execute_loadvtk();
	void execute_loadnifti();
	void execute_reloaddicom();
	void execute_reloadbmp();
	void execute_reloadraw();
	void execute_reloadmhd();
	void execute_reloadavw();
	void execute_reloadvtk();
	void execute_reloadnifti();
	void execute_loadsurface();
	void execute_loadrtstruct();
	void execute_loadrtdose();
	void execute_reloadrtdose();
	void execute_loads4llivelink();

	void execute_saveimg();
	void execute_saveprojas();
	void execute_savecopyas();
	void execute_saveactiveslicesas();
	void execute_saveproj();
	void execute_mergeprojects();
	void execute_boneconnectivity();
	void execute_loadproj();
	void execute_loadproj1();
	void execute_loadproj2();
	void execute_loadproj3();
	void execute_loadproj4();
	void execute_loadatlas(int i);
	void execute_loadatlas0();
	void execute_loadatlas1();
	void execute_loadatlas2();
	void execute_loadatlas3();
	void execute_loadatlas4();
	void execute_loadatlas5();
	void execute_loadatlas6();
	void execute_loadatlas7();
	void execute_loadatlas8();
	void execute_loadatlas9();
	void execute_loadatlas10();
	void execute_loadatlas11();
	void execute_loadatlas12();
	void execute_loadatlas13();
	void execute_loadatlas14();
	void execute_loadatlas15();
	void execute_loadatlas16();
	void execute_loadatlas17();
	void execute_loadatlas18();
	void execute_loadatlas19();
	void execute_createatlas();
	void execute_reloadatlases();
	void execute_savetissues();
	void execute_exportlabelfield();
	void execute_exportmat();
	void execute_exporthdf();
	void execute_exportvtkascii();
	void execute_exportvtkbinary();
	void execute_exportvtkcompressedascii();
	void execute_exportvtkcompressedbinary();
	void execute_exportxmlregionextent();
	void execute_exporttissueindex();
	void execute_loadtissues();
	void execute_savecolorlookup();
	void execute_settissuesasdef();
	void execute_removedeftissues();
	void execute_scale();
	void execute_imagemath();
	void execute_unwrap();
	void execute_overlay();
	void execute_histo();
	void execute_pixelsize();
	void execute_tissue_surfaceviewer();
	void execute_source_surfaceviewer();
	void execute_target_surfaceviewer();
	void execute_selectedtissue_surfaceviewer();
	void execute_3Dvolumeviewerbmp();
	void execute_3Dvolumeviewertissue();
	void execute_displacement();
	void execute_rotation();
	void execute_undoconf();
	void execute_activeslicesconf();
	void execute_hideparameters(bool);
	void execute_hidezoom(bool);
	void execute_hidecontrastbright(bool);
	void execute_hidecopyswap(bool);
	void execute_hidestack(bool);
	void execute_hideoverlay(bool);
	void execute_multidataset(bool);
	void execute_hidenotes(bool);
	void execute_hidesource(bool);
	void execute_hidetarget(bool);
	void execute_showtabtoggled(bool);
	void execute_xslice();
	void execute_yslice();
	void execute_grouptissues();
	void execute_removetissues();
	void execute_about();
	void execute_settings();

	void add_mark(Point p);
	void add_label(Point p, std::string str);
	void clear_marks();
	void remove_mark(Point p);
	void select_tissue(Point p, bool clear_selection);
	void view_tissue(Point p);
	void next_featuring_slice();
	void add_tissue(Point p);
	void add_tissue_connected(Point p);
	void subtract_tissue(Point p);
	void add_tissue_3D(Point p);
	void add_tissuelarger(Point p);
	void add_tissue_clicked(Point p);
	void addhold_tissue_clicked(Point p);
	//	void add_tissue_connected_clicked(Point p);
	void subtract_tissue_clicked(Point p);
	void subtracthold_tissue_clicked(Point p);
	//	void add_tissue_3D_clicked(Point p);
	void add_tissue_pushed();
	void add_tissue_shortkey();
	void addhold_tissue_pushed();
	void stophold_tissue_pushed();
	//	void add_tissue_connected_pushed();
	void subtract_tissue_pushed();
	void subtract_tissue_shortkey();
	void subtracthold_tissue_pushed();
	//	void add_tissue_3D_pushed();
	void randomize_colors();
	void tissueFilterChanged(const QString&);
	void newTissuePressed();
	void newFolderPressed();
	void lockAllTissues();
	void modifTissueFolderPressed();
	void removeTissueFolderPressed();
	void removeselected();
	void removeselected(const std::vector<QTreeWidgetItem*>& sel, bool perform_checks);
	void removeTissueFolderAllPressed();
	void tissue2work();
	void selectedtissue2work();
	void tissue2workall();
	void cleartissue();
	void cleartissues();
	void clearselected();
	void tab_changed(int);
	void bmptissuevisible_changed();
	void bmpoutlinevisible_changed();
	void worktissuevisible_changed();
	void workpicturevisible_changed();
	void slicenr_up();
	void slicenr_down();
	void zoom_in();
	void zoom_out();
	void sb_slicenr_changed();
	void scb_slicenr_changed();
	void sb_stride_changed();
	void pb_first_pressed();
	void pb_last_pressed();
	void slicethickness_changed();
	void xslice_closed();
	void yslice_closed();
	void surface_viewer_closed();
	void VV3D_closed();
	void VV3Dbmp_closed();
	void xshower_slicechanged();
	void yshower_slicechanged();
	void setBmpContentsPos(int x, int y);
	void setWorkContentsPos(int x, int y);
	void tissuenr_changed(int);
	void tissue_selection_changed();
	void tree_widget_doubleclicked(QTreeWidgetItem* item, int column);
	void tree_widget_contextmenu(const QPoint& pos);
	void execute_undo();
	void execute_redo();
	void execute_inversesliceorder();
	void execute_remove_unused_tissues();
	void execute_voting_replace_labels();
	void execute_target_connected_components();
	void execute_split_tissue();
	void execute_cleanup();
	void execute_smoothsteps();
	void execute_smoothtissues();
	void do_tissue2work();
	void do_work2tissue();
	void do_work2tissue_grouped();
	void tissuelock_toggled();
	void execute_swapxy();
	void execute_swapxz();
	void execute_swapyz();
	void execute_resize(int resizetype = 0);
	void execute_pad();
	void execute_crop();
	void sl_contrastbmp_moved(int);
	void sl_contrastwork_moved(int);
	void sl_brightnessbmp_moved(int);
	void sl_brightnesswork_moved(int);
	void le_contrastbmp_val_edited();
	void le_contrastwork_val_edited();
	void le_brightnessbmp_val_edited();
	void le_brightnesswork_val_edited();
	void reconnectmouse_afterrelease(Point);
	void merge();
	void unselectall();

	void pb_tab_pressed(int nr);
	void bmpcrosshairvisible_changed();
	void workcrosshairvisible_changed();
	void wheelrotated(int delta);
	void mousePosZoom_changed(const QPoint& point);

	void handle_begin_datachange(iseg::DataSelection& dataSelection,
			QWidget* sender = nullptr, bool beginUndo = true);
	void handle_end_datachange(QWidget* sender = nullptr,
			iseg::EndUndoAction undoAction = iseg::EndUndo);

	void handle_begin_dataexport(iseg::DataSelection& dataSelection, QWidget* sender = nullptr);
	void handle_end_dataexport(QWidget* sender = nullptr);

	void DatasetChanged();

	void update_slice();

	void EnableActionsAfterPrjLoaded(const bool enable);
};

} // namespace iseg
