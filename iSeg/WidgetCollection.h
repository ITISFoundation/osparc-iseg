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

#include "Data/DataSelection.h"
#include "Data/Point.h"
#include "Data/Types.h"

#include <QDialog>
#include <QDir>
#include <QMap>
#include <QWidget>

#include <map>
#include <string>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QGroupBox;
class QSpinBox;
class QButtonGroup;
class QRadioButton;
class QListWidget;
class QTableWidget;
class QSlider;
// Qt3
class Q3HBoxLayout;
class Q3VBoxLayout;
class Q3HBox;
class Q3VBox;

namespace iseg {

class SlicesHandler;
class TissueTreeWidget;
class Bmphandler;

//class ScaleWork : public QWidget
class ScaleWork : public QDialog
{
	Q_OBJECT
public:
	ScaleWork(SlicesHandler* hand3D, QDir picpath, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~ScaleWork() override;

private:
	Bmphandler* m_Bmphand;
	SlicesHandler* m_Handler3D;
	unsigned short m_Activeslice;
	Q3HBox* m_Hbox1;
	Q3HBox* m_Hbox2;
	Q3HBox* m_Hbox3;
	Q3HBox* m_Hbox4;
	Q3VBox* m_Vbox1;
	QPushButton* m_GetRange;
	QPushButton* m_DoScale;
	QPushButton* m_DoCrop;
	QSlider* m_SlBrighness;
	QSlider* m_SlContrast;
	QLineEdit* m_LimitLow;
	QLineEdit* m_LimitHigh;
	QLabel* m_LL;
	QLabel* m_LH;
	QLabel* m_LbBrightness;
	QLabel* m_LbContrast;
	QCheckBox* m_Allslices;
	float m_Minval, m_Maxval;
	float m_Minval1, m_Maxval1;
	bool m_Dontundo;
	QPushButton* m_CloseButton;

signals:
	void BeginDatachange(DataSelection& dataSelection, QWidget* sender = nullptr, bool beginUndo = true);
	void EndDatachange(QWidget* sender = nullptr, eEndUndoAction undoAction = iseg::EndUndo);

public slots:
	void SlicenrChanged();

private slots:
	void BmphandChanged(Bmphandler* bmph);
	void GetrangePushed();
	void ScalePushed();
	void CropPushed();
	void SliderChanged(int newval);
	void SliderPressed();
	void SliderReleased();
};

class ImageMath : public QDialog
{
	Q_OBJECT
public:
	ImageMath(SlicesHandler* hand3D, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~ImageMath() override;

private:
	Bmphandler* m_Bmphand;
	SlicesHandler* m_Handler3D;
	unsigned short m_Activeslice;
	Q3HBox* m_Hbox1;
	Q3HBox* m_Hbox2;
	Q3HBox* m_Hbox3;
	Q3VBox* m_Vbox1;
	QPushButton* m_DoAdd;
	QPushButton* m_DoSub;
	QPushButton* m_DoMult;
	QPushButton* m_DoNeg;
	QButtonGroup* m_Imgorval;
	QRadioButton* m_RbImg;
	QRadioButton* m_RbVal;
	float m_Val;
	QLineEdit* m_LeVal;
	QLabel* m_LbVal;
	QCheckBox* m_Allslices;
	QPushButton* m_CloseButton;

signals:
	void BeginDatachange(DataSelection& dataSelection, QWidget* sender = nullptr, bool beginUndo = true);
	void EndDatachange(QWidget* sender = nullptr, eEndUndoAction undoAction = iseg::EndUndo);

public slots:
	void SlicenrChanged();

private slots:
	void BmphandChanged(Bmphandler* bmph);
	void AddPushed();
	void SubPushed();
	void MultPushed();
	void NegPushed();
	void ValueChanged();
	void ImgorvalChanged(int);
};

class ImageOverlay : public QDialog
{
	Q_OBJECT
public:
	ImageOverlay(SlicesHandler* hand3D, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~ImageOverlay() override;

protected:
	void closeEvent(QCloseEvent* e) override;

private:
	Bmphandler* m_Bmphand;
	SlicesHandler* m_Handler3D;
	unsigned short m_Activeslice;
	Q3VBox* m_Vbox1;
	Q3HBox* m_Hbox1;
	Q3HBox* m_Hbox2;
	Q3HBox* m_Hbox3;
	QLabel* m_LbAlpha;
	QSlider* m_SlAlpha;
	QLineEdit* m_LeAlpha;
	QCheckBox* m_Allslices;
	QPushButton* m_CloseButton;
	QPushButton* m_ApplyButton;
	float m_Alpha;
	int m_SliderMax;
	unsigned short m_SliderPrecision;
	float* m_BkpWork;

signals:
	void BeginDatachange(DataSelection& dataSelection, QWidget* sender = nullptr, bool beginUndo = true);
	void EndDatachange(QWidget* sender = nullptr, eEndUndoAction undoAction = iseg::EndUndo);

public slots:
	void Newloaded();
	void SlicenrChanged();

private slots:
	void BmphandChanged(Bmphandler* bmph);
	void ApplyPushed();
	void AlphaChanged();
	void SliderChanged(int newval);
};

class HistoWin : public QWidget
{
	Q_OBJECT
public:
	HistoWin(unsigned int* histo1, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	void update();
	void HistoChanged(unsigned int* histo1);

protected:
	void paintEvent(QPaintEvent* e) override;

private:
	QImage m_Image;
	unsigned int* m_Histo;
};

class ShowHisto : public QDialog
{
	Q_OBJECT
public:
	ShowHisto(SlicesHandler* hand3D, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~ShowHisto() override;

private:
	Bmphandler* m_Bmphand;
	SlicesHandler* m_Handler3D;
	unsigned short m_Activeslice;
	Q3HBox* m_Hbox1;
	Q3HBox* m_Hbox2;
	Q3HBox* m_Hbox3;
	Q3HBox* m_Hbox4;
	HistoWin* m_Histwindow;
	Q3VBox* m_Vbox1;
	Q3VBox* m_Vbox2;
	QPushButton* m_CloseButton;
	QPushButton* m_UpdateSubsect;
	QSpinBox* m_Xoffset;
	QSpinBox* m_Yoffset;
	QSpinBox* m_Xlength;
	QSpinBox* m_Ylength;
	QCheckBox* m_Subsect;
	QLabel* m_Xoffs;
	QLabel* m_Yoffs;
	QLabel* m_Xl;
	QLabel* m_Yl;
	QButtonGroup* m_Pictselect;
	QRadioButton* m_Workpict;
	QRadioButton* m_Bmppict;

public slots:
	void SlicenrChanged();
	void Newloaded();

private slots:
	void BmphandChanged(Bmphandler* bmph); //TODO is this a slot?
	void DrawHisto();
	void SubsectToggled();
	void PictToggled(bool on);
	void SubsectUpdate();
};

class Colorshower : public QWidget
{
	Q_OBJECT
public:
	Colorshower(int lx1, int ly1, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);

protected:
	void paintEvent(QPaintEvent* e) override;

private:
	int m_Lx;
	int m_Ly;
	float m_Fr;
	float m_Fg;
	float m_Fb;
	float m_Opac;

private slots:
	void ColorChanged(float fr1, float fg1, float fb1, float opac1);
};

class TissueAdder : public QDialog
{
	Q_OBJECT
public:
	TissueAdder(bool modifyTissue, TissueTreeWidget* tissueTree, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~TissueAdder() override;
	//	FILE *save_proj(FILE *fp);

private:
	bool m_Modify;

	Q3HBoxLayout* m_Hbox1;
	Q3HBoxLayout* m_Hbox2;
	Q3HBoxLayout* m_Hbox3;
	/*	QHBoxLayout *hboxr;
			QHBoxLayout *hboxg;
			QHBoxLayout *hboxb;*/
	Colorshower* m_Cs;
	Q3VBoxLayout* m_Vbox1;
	Q3VBoxLayout* m_Vbox2;
	Q3VBoxLayout* m_Vbox3;
	Q3VBoxLayout* m_Vbox4;
	QPushButton* m_CloseButton;
	//	QPushButton *colorButton;
	QPushButton* m_AddTissue;
	QLabel* m_Red;
	QLabel* m_Green;
	QLabel* m_Blue;
	QLabel* m_Opac;
	QLabel* m_Tissuename;
	QLineEdit* m_NameField;
	QSlider* m_R;
	QSlider* m_G;
	QSlider* m_B;
	QSlider* m_SlTransp;
	QSpinBox* m_SbR;
	QSpinBox* m_SbG;
	QSpinBox* m_SbB;
	QSpinBox* m_SbTransp;
	float m_Fr1;
	float m_Fg1;
	float m_Fb1;
	float m_Transp1;
	TissueTreeWidget* m_TissueTreeWidget;
	//	float **tc;

signals:
	void ColorChanged(float fr, float fg, float fb, float opac2);

private slots:
	void UpdateColorR(int v);
	void UpdateColorG(int v);
	void UpdateColorB(int v);
	void UpdateOpac(int v);
	void UpdateColorRsb(int v);
	void UpdateColorGsb(int v);
	void UpdateColorBsb(int v);
	void UpdateOpacsb(int v);
	void AddPressed();
};

class TissueFolderAdder : public QDialog
{
	Q_OBJECT
public:
	TissueFolderAdder(TissueTreeWidget* tissueTree, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~TissueFolderAdder() override;

private:
	Q3HBoxLayout* m_HboxOverall;
	Q3VBoxLayout* m_VboxOverall;
	Q3HBoxLayout* m_HboxFolderName;
	Q3HBoxLayout* m_HboxPushButtons;
	QLabel* m_NameLabel;
	QLineEdit* m_NameLineEdit;
	QPushButton* m_AddButton;
	QPushButton* m_CloseButton;
	TissueTreeWidget* m_TissueTreeWidget;

private slots:
	void AddPressed();
};

class TissueHierarchyWidget : public QWidget
{
	Q_OBJECT
public:
	TissueHierarchyWidget(TissueTreeWidget* tissueTree, QWidget* parent = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~TissueHierarchyWidget() override;

	bool HandleChangedHierarchy();

private:
	QDir m_PicturePath;
	Q3HBoxLayout* m_HboxOverall;
	Q3VBoxLayout* m_VboxOverall;
	Q3VBoxLayout* m_VboxHierarchyButtons;
	QComboBox* m_HierarchyComboBox;
	QPushButton* m_NewHierarchyButton;
	QPushButton* m_LoadHierarchyButton;
	QPushButton* m_SaveHierarchyAsButton;
	QPushButton* m_RemoveHierarchyButton;
	TissueTreeWidget* m_TissueTreeWidget;

public slots:
	void UpdateHierarchyComboBox();
	bool SaveHierarchyAsPressed();

private slots:
	void HierarchyChanged(int index);
	void NewHierarchyPressed();
	void LoadHierarchyPressed();
	void RemoveHierarchyPressed();
};

class BitsStack : public QWidget
{
	Q_OBJECT
public:
	BitsStack(SlicesHandler* hand3D, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~BitsStack() override;
	QMap<QString, unsigned int>* ReturnBitsnr();
	void Newloaded();
	FILE* SaveProj(FILE* fp);
	FILE* LoadProj(FILE* fp);
	void ClearStack();

protected:
	void PushHelper(bool source, bool target, bool tissue);
	void PopHelper(bool source, bool target, bool tissue);

private:
	unsigned short m_Oldw, m_Oldh;
	SlicesHandler* m_Handler3D;
	QListWidget* m_BitsNames;
	Q3HBoxLayout* m_Hbox1;
	Q3VBoxLayout* m_Vbox1;
	QPushButton* m_Pushwork;
	QPushButton* m_Pushbmp;
	QPushButton* m_Pushtissue;
	QPushButton* m_Popwork;
	QPushButton* m_Popbmp;
	QPushButton* m_Loaditem;
	QPushButton* m_Saveitem;
	QPushButton* m_Poptissue;
	QPushButton* m_Deletebtn;
	QMap<QString, unsigned int> m_BitsNr;
	tissues_size_t m_Tissuenr;

signals:
	void StackChanged();
	void BeginDatachange(DataSelection& dataSelection, QWidget* sender = nullptr, bool beginUndo = true);
	void EndDatachange(QWidget* sender = nullptr, eEndUndoAction undoAction = iseg::EndUndo);
	void BeginDataexport(DataSelection& dataSelection, QWidget* sender = nullptr);
	void EndDataexport(QWidget* sender = nullptr);

public slots:
	void TissuenrChanged(unsigned short i);

private slots:
	void PushworkPressed();
	void PushbmpPressed();
	void PushtissuePressed();
	void PopworkPressed();
	void PopbmpPressed();
	void PoptissuePressed();
	void SaveitemPressed();
	void LoaditemPressed();
	void DeletePressed();
};

class BitsStackPushdialog : public QDialog
{
	Q_OBJECT
public:
	BitsStackPushdialog(QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~BitsStackPushdialog() override;
	bool GetPushcurrentslice();
	unsigned int GetStartslice(bool* ok);
	unsigned int GetEndslice(bool* ok);

private:
	Q3VBox* m_Vboxoverall;
	Q3HBox* m_Hboxparams;
	Q3VBox* m_Vboxsliceselection;
	Q3VBox* m_Vboxdelimiter;
	Q3HBox* m_Hboxslicerange;
	Q3VBox* m_Vboxslicerangelabels;
	Q3VBox* m_Vboxslicerangelineedits;
	Q3HBox* m_Hboxpushbuttons;
	QRadioButton* m_RbCurrentslice;
	QRadioButton* m_RbMultislices;
	QButtonGroup* m_Slicegroup;
	QLabel* m_LbStartslice;
	QLabel* m_LbEndslice;
	QLineEdit* m_LeStartslice;
	QLineEdit* m_LeEndslice;
	QPushButton* m_Pushexec;
	QPushButton* m_Pushcancel;

private slots:
	void SliceselectionChanged();
};

class ExtoverlayWidget : public QWidget
{
	Q_OBJECT
public:
	ExtoverlayWidget(SlicesHandler* hand3D, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~ExtoverlayWidget() override;
	void Newloaded();

private:
	Bmphandler* m_Bmphand;
	SlicesHandler* m_Handler3D;
	unsigned short m_Activeslice;

	unsigned short m_SelectedDataset;
	std::vector<QString> m_DatasetNames;
	std::vector<QString> m_DatasetFilepaths;

	float m_Alpha;
	int m_SliderMax;
	unsigned short m_SliderPrecision;

	Q3HBoxLayout* m_HboxOverall;
	Q3VBoxLayout* m_VboxOverall;
	Q3HBoxLayout* m_HboxAlpha;
	Q3HBoxLayout* m_HboxDisplaySrcTgt;
	QComboBox* m_DatasetComboBox;
	QPushButton* m_LoadDatasetButton;
	QLabel* m_LbAlpha;
	QSlider* m_SlAlpha;
	QLineEdit* m_LeAlpha;
	QCheckBox* m_SrcCheckBox;
	QCheckBox* m_TgtCheckBox;

protected:
	void Initialize();
	void AddDataset(const QString& path);
	void RemoveDataset(unsigned short idx);
	void ReloadOverlay();

signals:
	void OverlayChanged();
	void OverlayalphaChanged(float newValue);
	void BmpoverlayvisibleChanged(bool newValue);
	void WorkoverlayvisibleChanged(bool newValue);

public slots:
	void SlicenrChanged();

private slots:
	void DatasetChanged(int index);
	void LoadDatasetPressed();
	void AlphaChanged();
	void SliderChanged(int newval);
	void SourceToggled();
	void TargetToggled();
};

class ZoomWidget : public QWidget
{
	Q_OBJECT
public:
	ZoomWidget(double zoom1, QDir picpath, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~ZoomWidget() override;
	double GetZoom() const;

public slots:
	void ZoomChanged(double);

private:
	double m_Zoom;
	Q3VBoxLayout* m_Vbox1;
	Q3HBoxLayout* m_Hbox1;
	QPushButton* m_Pushzoomin;
	QPushButton* m_Pushzoomout;
	QPushButton* m_Pushunzoom;
	QLineEdit* m_LeZoomF;
	QLabel* m_ZoomF;

signals:
	void ZoomIn();
	void ZoomOut();
	void Unzoom();
	void SetZoom(double);

private slots:
	//	void zoom_changed(double);
	void ZoominPushed();
	void ZoomoutPushed();
	void UnzoomPushed();
	void LeZoomChanged();
};

class CleanerParams : public QDialog
{
	Q_OBJECT
public:
	CleanerParams(int* rate1, int* minsize1, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~CleanerParams() override;

private:
	int* m_Rate;
	int* m_Minsize;
	Q3HBox* m_Hbox1;
	Q3VBox* m_Vbox1;
	Q3VBox* m_Vbox2;
	QPushButton* m_PbDoit;
	QPushButton* m_PbDontdoit;
	QLabel* m_LbRate;
	QLabel* m_LbMinsize;
	QSpinBox* m_SbRate;
	QSpinBox* m_SbMinsize;

private slots:
	void DoitPressed();
	void DontdoitPressed();
};

class MergeProjectsDialog : public QDialog
{
	Q_OBJECT
public:
	MergeProjectsDialog(QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~MergeProjectsDialog() override;

	void GetFilenames(std::vector<QString>& filenames);

private:
	Q3HBoxLayout* m_HboxOverall;
	Q3VBoxLayout* m_VboxFileList;
	Q3VBoxLayout* m_VboxButtons;
	Q3VBoxLayout* m_VboxEditButtons;
	Q3VBoxLayout* m_VboxExecuteButtons;
	QListWidget* m_FileListWidget;
	QPushButton* m_AddButton;
	QPushButton* m_RemoveButton;
	QPushButton* m_MoveUpButton;
	QPushButton* m_MoveDownButton;
	QPushButton* m_ExecuteButton;
	QPushButton* m_CancelButton;

private slots:
	void AddPressed();
	void RemovePressed();
	void MoveUpPressed();
	void MoveDownPressed();
};

class CheckBoneConnectivityDialog : public QWidget
{
	Q_OBJECT

	enum eBoneConnectionColumn {
		kTissue1,
		kTissue2,
		kSliceNumber,
		kColumnNumber
	};

	struct BoneConnectionInfo
	{
		BoneConnectionInfo(const tissues_size_t tis1, const tissues_size_t tis2, const int slice)
				: m_TissueID1(tis1), m_TissueID2(tis2), m_SliceNumber(slice)
		{
		}

		bool operator<(const BoneConnectionInfo& a) const
		{
			if (m_SliceNumber < a.m_SliceNumber)
			{
				return true;
			}
			else if (m_SliceNumber > a.m_SliceNumber)
			{
				return false;
			}
			else
			{
				return m_TissueID1 < a.m_TissueID1;
			}
		}

		bool operator==(const BoneConnectionInfo& a) const
		{
			return (m_SliceNumber == a.m_SliceNumber &&
							((m_TissueID1 == a.m_TissueID1 && m_TissueID2 == a.m_TissueID2) ||
									(m_TissueID1 == a.m_TissueID2 && m_TissueID2 == a.m_TissueID1)));
		}

		tissues_size_t m_TissueID1;
		tissues_size_t m_TissueID2;
		int m_SliceNumber;
	};

public:
	CheckBoneConnectivityDialog(SlicesHandler* hand3D, const char* name, QWidget* parent = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~CheckBoneConnectivityDialog() override;

signals:
	void SliceChanged();

private:
	void ShowText(const std::string& text);
	void CheckBoneExist();
	void LookForConnections();
	void FillConnectionsTable();

	bool IsBone(const std::string& label_name) const;

private:
	SlicesHandler* m_Handler3D;

	Q3HBox* m_MainBox;
	Q3VBox* m_Vbox1;
	Q3HBox* m_Hbox1;
	Q3HBox* m_Hbox2;
	Q3HBox* m_Hbox3;
	Q3HBox* m_Hbox4;

	QCheckBox* m_BonesFoundCb;

	QPushButton* m_ExecuteButton;
	QPushButton* m_CancelButton;
	QPushButton* m_ExportButton;

	QTableWidget* m_FoundConnectionsTable;

	QLabel* m_ProgressText;

	std::vector<BoneConnectionInfo> m_FoundConnections;

	Q3HBoxLayout* m_HboxOverall;
	Q3VBoxLayout* m_VboxFileList;
	Q3VBoxLayout* m_VboxButtons;
	Q3VBoxLayout* m_VboxEditButtons;
	Q3VBoxLayout* m_VboxExecuteButtons;
	QListWidget* m_FileListWidget;

private slots:
	void CellClicked(int row, int col);
	void ExecutePressed();
	void CancelPressed();
	void ExportPressed();
};

} // namespace iseg
