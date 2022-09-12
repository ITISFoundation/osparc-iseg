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

#include "Atlas.h"
#include "Project.h"

#include "Data/DataSelection.h"
#include "Data/Point.h"

#include <QMainWindow>
#include <QMenu>
#include <QDir>

class QAction;
class QCheckBox;
class QCloseEvent;
class QDockWidget;
class QLabel;
class QTextEdit;
class QLineEdit;
class QMenuBar;
class QHBoxLayout;
class QPushButton;
class QStackedWidget;
class QSpinBox;
class QSlider;
class QTreeWidgetItem;
class QSignalMapper;
class QVBoxLayout;
class QWidget;

class QScrollBar;
class Q3ScrollView;

class QLogTable;

namespace iseg {

class SlicesHandler;
class WidgetInterface;
class TissueTreeWidget;
class TissueHierarchyWidget;
class BitsStack;
class ExtoverlayWidget;
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
class ThresholdWidgetQt4;
class MeasurementWidget;
class ImageViewerWidget;
class SurfaceViewerWidget;
class VolumeViewerWidget;

class MenuWTT : public QMenu
{
	Q_OBJECT
public:
	MenuWTT() = default;
	bool event(QEvent* e) override;
};

class MainWindow : public QMainWindow
{
	Q_OBJECT
public:
	MainWindow(SlicesHandler* hand3D, const QString& locationstring, const QDir& picpath, const QDir& tmppath, bool editingmode = false, const std::vector<std::string>& plugin_search_dirs = std::vector<std::string>());
	~MainWindow() override = default;

	friend class Settings;

	void LoadLoadProj(const QString& path1);
	void LoadAtlas(const QDir& path1);
	void AddLoadProj(const QString& path1);
	void SaveLoadProj(const QString& latestprojpath) const;
	void SaveSettings();
	void LoadSettings(const std::string& loadfilename);
	void Loadproj(const QString& loadfilename);
	void LoadAny(const QString& loadfilename);
	void LoadS4Llink(const QString& loadfilename);
	void LoadTissuelist(const QString& file_path, bool append, bool no_popup);
	void ReloadMedicalImage(const QString& loadfilename);
	void Work2Tissue() { DoWork2tissue(); }

protected:
	void StartSurfaceviewer(int mode);
	void closeEvent(QCloseEvent*) override;
	bool MaybeSafe();
	bool Modified() const;
	void ModifTissue();
	void ModifFolder();
	void EndUndoHelper(eEndUndoAction undoAction);
	void CancelTransformHelper();
	void UpdateRangesHelper();
	void PixelsizeChanged();
	void DoUndostepdone();
	void DoClearundo();
	void ResetBrightnesscontrast();
	void UpdateBrightnesscontrast(bool bmporwork, bool paint = true);
	FILE* SaveNotes(FILE* fp, unsigned short version);
	FILE* LoadNotes(FILE* fp, unsigned short version);

signals:
	void BmpChanged();
	void WorkChanged();
	void MarksChanged();
	void TissuesChanged();
	void DrawingChanged();
	void BeginDatachange(DataSelection& dataSelection, QWidget* sender = nullptr, bool beginUndo = true);
	void EndDatachange(QWidget* sender = nullptr, eEndUndoAction undoAction = iseg::EndUndo);
	void BeginDataexport(DataSelection& dataSelection, QWidget* sender = nullptr);
	void EndDataexport(QWidget* sender = nullptr);

private:
	bool m_MEditingmode;
	bool m_Modified;

	std::string m_Settingsfile;
	QString m_MLocationstring;
	QDir m_MPicpath;
	QDir m_MTmppath;
	SlicesHandler* m_Handler3D;
	ImageViewerWidget* m_BmpShow;
	ImageViewerWidget* m_WorkShow;
	QVBoxLayout* m_Vboxtotal;
	QHBoxLayout* m_Hbox1;
	QVBoxLayout* m_Vbox1;
	QVBoxLayout* m_Vboxbmp;
	QVBoxLayout* m_Vboxwork;
	QHBoxLayout* m_Hboxbmp;
	QHBoxLayout* m_Hboxwork;
	QHBoxLayout* m_Hboxbmp1;
	QHBoxLayout* m_Hboxwork1;
	QHBoxLayout* m_Hbox2;
	QVBoxLayout* m_Vbox2;
	QVBoxLayout* m_Vboxnotes;
	QVBoxLayout* m_Rightvbox;
	QHBoxLayout* m_Hboxlock;
	QHBoxLayout* m_Hboxtissueadder;
	QVBoxLayout* m_Vboxtissueadder1;
	QVBoxLayout* m_Vboxtissueadder1b;
	QVBoxLayout* m_Vboxtissueadder2;
	QCheckBox* m_CbAddsub3d;
	QCheckBox* m_CbAddsubconn;
	QCheckBox* m_CbAddsuboverride;
	QPushButton* m_PbAdd;
	QPushButton* m_PbSub;
	QPushButton* m_PbAddhold;
	QPushButton* m_PbSubhold;
	QPushButton* m_ToworkBtn;
	QPushButton* m_TobmpBtn;
	QPushButton* m_SwapBtn;
	QPushButton* m_SwapAllBtn;
	QPushButton* m_PbWork2tissue;
	QCheckBox* m_CbMask3d;
	QLineEdit* m_EditMaskValue;
	QPushButton* m_PbMask;
	QMenuBar* m_Menubar;
	MenuWTT* m_File;
	QMenu* m_Imagemenu;
	QMenu* m_Viewmenu;
	QMenu* m_Toolmenu;
	QMenu* m_Atlasmenu;
	QMenu* m_Helpmenu;
	QMenu* m_Editmenu;
	QMenu* m_Loadmenu;
	QMenu* m_Reloadmenu;
	QMenu* m_Exportmenu;
	QMenu* m_Saveprojasmenu;
	QMenu* m_Saveprojmenu;
	QMenu* m_Saveactiveslicesmenu;
	QMenu* m_Hidemenu;
	QMenu* m_Hidesubmenu;
	QDockWidget* m_TissuesDock;
	TissueTreeWidget* m_TissueTreeWidget; // Widget visualizing the tissue hierarchy
	QLineEdit* m_TissueFilter;
	TissueHierarchyWidget* m_TissueHierarchyWidget; // Widget for selecting the tissue hierarchy
	QLogTable* m_LogWindow = nullptr;
	QCheckBox* m_CbTissuelock;
	QPushButton* m_LockTissues;
	QPushButton* m_AddTissue;
	QPushButton* m_AddFolder;
	QPushButton* m_ModifyTissueFolder;
	QPushButton* m_RemoveTissueFolder;
	QPushButton* m_RemoveTissueFolderAll;
	QPushButton* m_GetTissue;
	QPushButton* m_GetTissueAll;
	QPushButton* m_ClearTissue;
	QPushButton* m_ClearTissues;
	QWidget* m_Menubarspacer;
	QAction* m_Hideparameters;
	QAction* m_Hidezoom;
	QAction* m_Hidecontrastbright;
	QAction* m_Hidecopyswap;
	QAction* m_Hidestack;
	QAction* m_Hidenotes;
	QAction* m_Hidesource;
	QAction* m_Hidetarget;
	std::vector<QAction*> m_ShowtabAction;
	QCheckBox* m_Tissue3Dopt;
	QStackedWidget* m_MethodTab;
	ThresholdWidgetQt4* m_ThresholdWidget;
	MeasurementWidget* m_MeasurementWidget;
	VesselWidget* m_VesselextrWidget;
	SmoothingWidget* m_SmoothingWidget;
	EdgeWidget* m_EdgeWidget;
	MorphologyWidget* m_MorphologyWidget;
	WatershedWidget* m_WatershedWidget;
	HystereticGrowingWidget* m_HystWidget;
	LivewireWidget* m_LivewireWidget;
	ImageForestingTransformRegionGrowingWidget* m_IftrgWidget;
	FastmarchingFuzzyWidget* m_FastmarchingWidget;
	OutlineCorrectionWidget* m_OlcWidget;
	FeatureWidget* m_FeatureWidget;
	InterpolationWidget* m_InterpolationWidget;
	PickerWidget* m_PickerWidget;
	TransformWidget* m_TransformWidget;
	WidgetInterface* m_TabOld;
	ScaleWork* m_ScaleDialog;
	ImageMath* m_ImagemathDialog;
	ImageOverlay* m_ImageoverlayDialog;
	BitsStack* m_BitstackWidget;
	ExtoverlayWidget* m_OverlayWidget;
	MultiDatasetWidget* m_MultidatasetWidget;
	QCheckBox* m_CbBmptissuevisible;
	QCheckBox* m_CbBmpcrosshairvisible;
	QCheckBox* m_CbBmpoutlinevisible;
	QCheckBox* m_CbWorktissuevisible;
	QCheckBox* m_CbWorkcrosshairvisible;
	QCheckBox* m_CbWorkpicturevisible;
	void ClearStack();
	void SliceChanged();
	unsigned short m_Nrslices;
	void Slices3dChanged(bool new_bitstack);
	QLabel* m_LbSlicenr;
	QLabel* m_LbInactivewarning;
	QSpinBox* m_SbSlicenr;
	QScrollBar* m_ScbSlicenr;
	QPushButton* m_PbFirst;
	QPushButton* m_PbLast;
	QLabel* m_LbStride;
	QSpinBox* m_SbStride;

	QLabel* m_LbSource;
	QLabel* m_LbTarget;
	Q3ScrollView* m_BmpScroller = nullptr;
	Q3ScrollView* m_WorkScroller = nullptr;
	bool m_TomoveScroller;
	ZoomWidget* m_ZoomWidget = nullptr;
	//	float thickness;
	SliceViewerWidget* m_Xsliceshower = nullptr;
	SliceViewerWidget* m_Ysliceshower = nullptr;
	SurfaceViewerWidget* m_SurfaceViewer = nullptr;
	VolumeViewerWidget* m_VV3D = nullptr;
	VolumeViewerWidget* m_VV3Dbmp = nullptr;
	QAction* m_Undonr = nullptr;
	QAction* m_Redonr = nullptr;
	QString m_MSaveprojfilename;
	QString m_S4Lcommunicationfilename;
	Project m_MLoadprojfilename;
	Atlas m_MAtlasfilename;
	QLabel* m_LbContrastbmp;
	QLabel* m_LbBrightnessbmp;
	QLabel* m_LbContrastwork;
	QLabel* m_LbBrightnesswork;
	QLabel* m_LbContrastbmpVal;
	QLabel* m_LbBrightnessbmpVal;
	QLineEdit* m_LeContrastbmpVal;
	QLineEdit* m_LeBrightnessbmpVal;
	QLabel* m_LbContrastworkVal;
	QLabel* m_LbBrightnessworkVal;
	QLineEdit* m_LeContrastworkVal;
	QLineEdit* m_LeBrightnessworkVal;
	QSlider* m_SlContrastbmp;
	QSlider* m_SlContrastwork;
	QSlider* m_SlBrightnessbmp;
	QSlider* m_SlBrightnesswork;

	QTextEdit* m_MNotes;
	std::vector<QPushButton*> m_PbTab;
	QSignalMapper* m_MWidgetSignalMapper;
	std::vector<bool> m_ShowpbTab;
	void UpdateMethodButtonsPressed(WidgetInterface*);
	void UpdateTabvisibility();
	std::vector<WidgetInterface*> m_Tabwidgets;
	void DisconnectMouseclick();
	void ConnectMouseclick();
	QWidget* m_Vboxbmpw;
	QWidget* m_Vboxworkw;

	CheckBoneConnectivityDialog* m_BoneConnectivityDialog;

	bool m_UndoStarted;
	bool m_CanUndo3D;
	DataSelection m_ChangeData;
	bool m_NewDataAfterSwap;

private slots:
	void UpdateBmp();
	void UpdateWork();
	void UpdateTissue();
	void ExecuteBmp2work();
	void ExecuteWork2bmp();
	void ExecuteSwapBmpwork();
	void ExecuteSwapBmpworkall();
	void ExecuteNew();
	void ExecuteSaveContours();
	void ExecuteLoaddicom();
	void ExecuteLoadImageSeries();
	void ExecuteLoadraw(const QString& file_name=QString());
	void ExecuteLoadavw();
	void ExecuteLoadMedicalImage(const QString& file_name=QString());

	void ExecuteLoadvtk(const QString& file_name=QString());
	void ExecuteReloaddicom();
	void ExecuteReloadbmp();
	void ExecuteReloadraw();
	void ExecuteReloadMedicalImage();
	void ExecuteReloadavw();
	void ExecuteReloadvtk();
	void ExecuteLoadsurface();
	void ExecuteLoadrtstruct();
	void ExecuteLoadrtdose();
	void ExecuteReloadrtdose();
	void ExecuteLoads4llivelink();

	void ExecuteSaveimg();
	void ExecuteSaveprojas();
	void ExecuteSavecopyas();
	void ExecuteSaveactiveslicesas();
	void ExecuteSaveproj();
	void ExecuteMergeprojects();
	void ExecuteBoneconnectivity();
	void ExecuteLoadproj();
	void ExecuteLoadproj1();
	void ExecuteLoadproj2();
	void ExecuteLoadproj3();
	void ExecuteLoadproj4();
	void ExecuteLoadatlas(int i);
	void ExecuteLoadatlas0();
	void ExecuteLoadatlas1();
	void ExecuteLoadatlas2();
	void ExecuteLoadatlas3();
	void ExecuteLoadatlas4();
	void ExecuteLoadatlas5();
	void ExecuteLoadatlas6();
	void ExecuteLoadatlas7();
	void ExecuteLoadatlas8();
	void ExecuteLoadatlas9();
	void ExecuteLoadatlas10();
	void ExecuteLoadatlas11();
	void ExecuteLoadatlas12();
	void ExecuteLoadatlas13();
	void ExecuteLoadatlas14();
	void ExecuteLoadatlas15();
	void ExecuteLoadatlas16();
	void ExecuteLoadatlas17();
	void ExecuteLoadatlas18();
	void ExecuteLoadatlas19();
	void ExecuteCreateatlas();
	void ExecuteReloadatlases();
	void ExecuteSavetissues();
	void ExecuteExportlabelfield();
	void ExecuteExportmat();
	void ExecuteExporthdf();
	void ExecuteExportvtkascii();
	void ExecuteExportvtkbinary();
	void ExecuteExportvtkcompressedascii();
	void ExecuteExportvtkcompressedbinary();
	void ExecuteExportxmlregionextent();
	void ExecuteExporttissueindex();
	void ExecuteLoadtissues();
	void ExecuteSavecolorlookup();
	void ExecuteSettissuesasdef();
	void ExecuteRemovedeftissues();
	void ExecuteScale();
	void ExecuteImagemath();
	void ExecuteUnwrap();
	void ExecuteOverlay();
	void ExecuteHisto();
	void ExecutePixelsize();
	void ExecuteTissueSurfaceviewer();
	void ExecuteSourceSurfaceviewer();
	void ExecuteTargetSurfaceviewer();
	void ExecuteSelectedtissueSurfaceviewer();
	void Execute3Dvolumeviewerbmp();
	void Execute3Dvolumeviewertissue();
	void ExecuteDisplacement();
	void ExecuteRotation();
	void ExecuteUndoconf();
	void ExecuteActiveslicesconf();
	void ExecuteHideparameters(bool);
	void ExecuteHidezoom(bool);
	void ExecuteHidecontrastbright(bool);
	void ExecuteHidecopyswap(bool);
	void ExecuteHidestack(bool);
	void ExecuteHideoverlay(bool);
	void ExecuteMultidataset(bool);
	void ExecuteHidenotes(bool);
	void ExecuteHidesource(bool);
	void ExecuteHidetarget(bool);
	void ExecuteShowtabtoggled(bool);
	void ExecuteXslice();
	void ExecuteYslice();
	void ExecuteGrouptissues();
	void ExecuteRemovetissues();
	void ExecuteAbout();
	void ExecuteSettings();

	void AddMark(Point p);
	void AddLabel(Point p, std::string str);
	void ClearMarks();
	void RemoveMark(Point p);
	void SelectTissue(Point p, bool clear_selection);
	void ViewTissue(Point p);
	void NextFeaturingSlice();
	void AddTissue(Point p);
	void AddTissueConnected(Point p);
	void SubtractTissue(Point p);
	void AddTissue3D(Point p);
	void AddTissuelarger(Point p);
	void AddTissueClicked(Point p);
	void AddholdTissueClicked(Point p);
	//	void add_tissue_connected_clicked(Point p);
	void SubtractTissueClicked(Point p);
	void SubtractholdTissueClicked(Point p);
	//	void add_tissue_3D_clicked(Point p);
	void AddTissuePushed();
	void AddTissueShortkey();
	void AddholdTissuePushed();
	void StopholdTissuePushed();
	//	void add_tissue_connected_pushed();
	void SubtractTissuePushed();
	void SubtractTissueShortkey();
	void SubtractholdTissuePushed();
	void MaskSource();
	//	void add_tissue_3D_pushed();
	void RandomizeColors();
	void TissueFilterChanged(const QString&);
	void NewTissuePressed();
	void NewFolderPressed();
	void LockAllTissues();
	void ModifTissueFolderPressed();
	void RemoveTissueFolderPressed();
	void Removeselected();
	void Removeselected(const std::vector<QTreeWidgetItem*>& sel, bool perform_checks);
	void RemoveTissueFolderAllPressed();
	void Tissue2work();
	void Selectedtissue2work();
	void Tissue2workall();
	void Cleartissue();
	void Cleartissues();
	void Clearselected();
	void TabChanged(int);
	void BmptissuevisibleChanged();
	void BmpoutlinevisibleChanged();
	void SetOutlineColor(QColor);
	void WorktissuevisibleChanged();
	void WorkpicturevisibleChanged();
	void SlicenrUp();
	void SlicenrDown();
	void ZoomIn();
	void ZoomOut();
	void SbSlicenrChanged();
	void ScbSlicenrChanged();
	void SbStrideChanged();
	void PbFirstPressed();
	void PbLastPressed();
	void SlicethicknessChanged();
	void XsliceClosed();
	void YsliceClosed();
	void SurfaceViewerClosed();
	void VV3DClosed();
	void VV3DbmpClosed();
	void XshowerSlicechanged();
	void YshowerSlicechanged();
	void SetBmpContentsPos(int x, int y);
	void SetWorkContentsPos(int x, int y);
	void TissuenrChanged(int);
	void TissueSelectionChanged();
	void TreeWidgetDoubleclicked(QTreeWidgetItem* item, int column);
	void TreeWidgetContextmenu(const QPoint& pos);
	void ExecuteUndo();
	void ExecuteRedo();
	void ExecuteInversesliceorder();
	void ExecuteRemoveUnusedTissues();
	void ExecuteVotingReplaceLabels();
	void ExecuteTargetConnectedComponents();
	void ExecuteSplitTissue();
	void ExecuteCleanup();
	void ExecuteSmoothsteps();
	void ExecuteSmoothtissues();
	void DoTissue2work();
	void DoWork2tissue();
	void DoWork2tissueGrouped();
	void TissuelockToggled();
	void ExecuteSwapxy();
	void ExecuteSwapxz();
	void ExecuteSwapyz();
	void ExecuteResize(int resizetype = 0);
	void ExecutePad();
	void ExecuteCrop();
	void SlContrastbmpMoved(int);
	void SlContrastworkMoved(int);
	void SlBrightnessbmpMoved(int);
	void SlBrightnessworkMoved(int);
	void LeContrastbmpValEdited();
	void LeContrastworkValEdited();
	void LeBrightnessbmpValEdited();
	void LeBrightnessworkValEdited();
	void ReconnectmouseAfterrelease(Point);
	void Merge();
	void Unselectall();

	void PbTabPressed(int nr);
	void BmpcrosshairvisibleChanged();
	void WorkcrosshairvisibleChanged();
	void Wheelrotated(int delta);
	void MousePosZoomChanged(const QPoint& point);

	void HandleBeginDatachange(DataSelection& dataSelection, QWidget* sender, bool beginUndo);
	void HandleEndDatachange(QWidget* sender, eEndUndoAction undoAction);

	void HandleBeginDataexport(DataSelection& dataSelection, QWidget* sender);
	void HandleEndDataexport(QWidget* sender);

	void DatasetChanged();

	void UpdateSlice();

	void EnableActionsAfterPrjLoaded(const bool enable);
};

} // namespace iseg
