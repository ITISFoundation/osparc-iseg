/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef LOADERWIDGET
#define LOADERWIDGET

#include "SlicesHandler.h"
#include "bmp_read_1.h"

#include "Data/Point.h"

#include <q3boxlayout.h>
#include <q3hbox.h>
#include <q3vbox.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qdialog.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qspinbox.h>
#include <qstringlist.h>
#include <qwidget.h>

#include <array>

namespace iseg {

class LoaderDicom : public QDialog
{
	Q_OBJECT
public:
	LoaderDicom(SlicesHandler* hand3D, QStringList* lname, bool breload, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~LoaderDicom() override;

private:
	SlicesHandler* m_Handler3D;
	QStringList* m_Lnames;
	bool m_Reload;
	Point m_P;
	unsigned short m_Dx;
	unsigned short m_Dy;
	Q3HBox* m_Hbox1;
	Q3HBox* m_Hbox2;
	Q3HBox* m_Hbox3;
	Q3HBox* m_Hbox4;
	Q3HBox* m_Hbox5;
	Q3HBox* m_Hbox6;
	Q3VBox* m_Vbox1;
	Q3VBox* m_Vbox2;
	Q3VBox* m_Vbox3;
	Q3VBox* m_Vbox4;
	Q3VBox* m_Vbox5;
	Q3VBox* m_Vbox6;
	QPushButton* m_LoadFile;
	QPushButton* m_CancelBut;
	QSpinBox* m_Xoffset;
	QSpinBox* m_Yoffset;
	QSpinBox* m_Xlength;
	QSpinBox* m_Ylength;
	QCheckBox* m_CbSubsect;
	QCheckBox* m_CbCt;
	QCheckBox* m_CbCrop;
	QButtonGroup* m_BgWeight;
	QRadioButton* m_RbMuscle;
	QRadioButton* m_RbBone;
	QLabel* m_Xoffs;
	QLabel* m_Yoffs;
	QLabel* m_Xl;
	QLabel* m_Yl;
	QLabel* m_LbTitle;
	QComboBox* m_Seriesnrselection;
	std::vector<unsigned> m_Dicomseriesnr;
	std::vector<unsigned> m_Dicomseriesnrlist;

private slots:
	void SubsectToggled();
	void CtToggled();
	void LoadPushed();
};

class LoaderColorImages : public QDialog
{
	Q_OBJECT
public:
	enum eImageType {
		kPNG,
		kBMP,
		kJPG,
		kTIF
	};

	LoaderColorImages(SlicesHandler* hand3D, eImageType typ, std::vector<const char*> filenames, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~LoaderColorImages() override;

	eImageType m_Type = kPNG;

private:
	void LoadMixer();
	void LoadQuantize();

	std::vector<const char*> m_MFilenames;
	SlicesHandler* m_Handler3D;

	QCheckBox* m_MapToLut;
	QCheckBox* m_Subsect;
	QSpinBox* m_Xoffset;
	QSpinBox* m_Yoffset;
	QSpinBox* m_Xlength;
	QSpinBox* m_Ylength;
	QPushButton* m_LoadFile;
	QPushButton* m_CancelBut;

private slots:
	void MapToLutToggled();
	void LoadPushed();
};

class ClickableLabel : public QLabel
{
	Q_OBJECT

public:
	ClickableLabel(QWidget* parent = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	ClickableLabel(const QString& text, QWidget* parent = nullptr, Qt::WindowFlags wFlags = Qt::Widget);

	void SetSquareWidth(int width);
	void SetSquareHeight(int height);
	void SetCenter(QPoint newCenter);

private:
	int m_CenterX;
	int m_CenterY;
	int m_SquareWidth;
	int m_SquareHeight;

signals:
	void NewCenterPreview(QPoint point);

private slots:
	void mousePressEvent(QMouseEvent* ev) override;
	void paintEvent(QPaintEvent* e) override;
};

class ChannelMixer : public QDialog
{
	Q_OBJECT
public:
	ChannelMixer(std::vector<const char*> filenames, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~ChannelMixer() override;

	int GetRedFactor() const;
	int GetGreenFactor() const;
	int GetBlueFactor() const;

private:
	std::vector<const char*> m_MFilenames;
	SlicesHandler* m_Handler3D;

	Q3VBox* m_HboxChannelOptions;

	Q3VBox* m_VboxMain;

	Q3HBox* m_HboxImageAndControl;

	Q3VBox* m_HboxControl;
	Q3HBox* m_HboxButtons;

	Q3HBox* m_HboxImageSource;
	ClickableLabel* m_ImageSourceLabel;

	Q3HBox* m_HboxImage;
	QLabel* m_ImageLabel;

	Q3HBox* m_VboxRed;
	Q3HBox* m_VboxBlue;
	Q3HBox* m_VboxGreen;
	Q3HBox* m_VboxSlice;

	QLabel* m_LabelRed;
	QSlider* m_SliderRed;
	QLineEdit* m_LabelRedValue;
	QRadioButton* m_ButtonRed;

	QLabel* m_LabelGreen;
	QSlider* m_SliderGreen;
	QLineEdit* m_LabelGreenValue;
	QRadioButton* m_ButtonGreen;

	QLabel* m_LabelBlue;
	QSlider* m_SliderBlue;
	QLineEdit* m_LabelBlueValue;
	QRadioButton* m_ButtonBlue;

	QLabel* m_LabelPreviewAlgorithm;

	QSpinBox* m_SpinSlice;
	QLabel* m_LabelSliceValue;

	QPushButton* m_LoadFile;
	QPushButton* m_CancelBut;

	int m_RedFactor;
	int m_GreenFactor;
	int m_BlueFactor;

	int m_RedFactorPv;
	int m_GreenFactorPv;
	int m_BlueFactorPv;
	int m_SelectedSlice;

	QImage m_SourceImage;
	double m_ScaleX;
	double m_ScaleY;

	QPoint m_PreviewCenter;

	bool m_FirstTime;
	int m_WidthPv;
	int m_HeightPv;

public slots:
	void RefreshSourceImage();
	void RefreshImage();

protected:
	QImage ConvertImageTo8BitBMP(QImage image, int width, int height);
	void ChangePreview();
	void UpdateText();

protected slots:
	void NewCenterPreview(QPoint newCenter);

private slots:
	void SliderRedValueChanged(int value);
	void SliderGreenValueChanged(int value);
	void SliderBlueValueChanged(int value);

	void LabelRedValueChanged(QString text);
	void LabelGreenValueChanged(QString text);
	void LabelBlueValueChanged(QString text);

	void ButtonRedPushed(bool checked);
	void ButtonGreenPushed(bool checked);
	void ButtonBluePushed(bool checked);

	void SliceValueChanged(int value);

	void CancelToggled();
	void LoadPushed();
};

class LoaderRaw : public QDialog
{
	Q_OBJECT
public:
	LoaderRaw(SlicesHandler* nullptrand3D, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~LoaderRaw() override;

	QString GetFileName() const;

	void SetSkipReading(bool skip) { m_SkipReading = skip; }

	std::array<unsigned int, 2> GetDimensions() const;
	std::array<unsigned int, 3> GetSubregionStart() const;
	std::array<unsigned int, 3> GetSubregionSize() const;
	int GetBits() const;

private:
	SlicesHandler* m_Handler3D;
	bool m_SkipReading = true;
	char* m_Filename;
	short unsigned m_W;
	short unsigned m_H;
	unsigned m_Bitdepth;
	unsigned short m_Slicenr;
	Point m_P;
	unsigned short m_Dx;
	unsigned short m_Dy;
	Q3HBox* m_Hbox1;
	Q3HBox* m_Hbox2;
	Q3HBox* m_Hbox3;
	Q3HBox* m_Hbox4;
	Q3HBox* m_Hbox5;
	Q3HBox* m_Hbox6;
	Q3HBox* m_Hbox7;
	Q3HBox* m_Hbox8;
	Q3VBox* m_Vbox1;
	Q3VBox* m_Vbox2;
	QPushButton* m_SelectFile;
	QPushButton* m_LoadFile;
	QPushButton* m_CancelBut;
	QSpinBox* m_Xoffset;
	QSpinBox* m_Yoffset;
	QSpinBox* m_Xlength;
	QSpinBox* m_Ylength;
	QSpinBox* m_Xlength1;
	QSpinBox* m_Ylength1;
	QSpinBox* m_Slicenrbox;
	QSpinBox* m_SbNrslices;
	QCheckBox* m_Subsect;
	QLineEdit* m_NameEdit;
	QLabel* m_FileName;
	QButtonGroup* m_Bitselect;
	QRadioButton* m_Bit8;
	QRadioButton* m_Bit16;
	QLabel* m_Nrslice;
	QLabel* m_Xoffs;
	QLabel* m_Yoffs;
	QLabel* m_Xl;
	QLabel* m_Yl;
	QLabel* m_Xl1;
	QLabel* m_Yl1;
	QLabel* m_LbNrslices;

private slots:
	void SubsectToggled();
	void LoadPushed();
	void SelectPushed();
};

class ExportImg : public QDialog
{
	Q_OBJECT
public:
	ExportImg(SlicesHandler* hand3D, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~ExportImg() override = default;

private slots:
	void SavePushed();

private:
	SlicesHandler* m_Handler3D;
	QButtonGroup* m_ImgSelectionGroup;
	QButtonGroup* m_SliceSelectionGroup;
	QPushButton* m_PbSave;
	QPushButton* m_PbCancel;
	QLineEdit* m_EditName;
};

class ReloaderBmp2 : public QDialog
{
	Q_OBJECT
public:
	ReloaderBmp2(SlicesHandler* hand3D, std::vector<const char*> filenames, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~ReloaderBmp2() override;
	//	void update();
	//protected:

private:
	std::vector<const char*> m_MFilenames;
	SlicesHandler* m_Handler3D;
	char* m_Filename;
	Q3HBox* m_Hbox1;
	Q3HBox* m_Hbox2;
	Q3HBox* m_Hbox3;
	Q3HBox* m_Hbox4;
	Q3VBox* m_Vbox1;
	QPushButton* m_LoadFile;
	QPushButton* m_CancelBut;
	QSpinBox* m_Xoffset;
	QSpinBox* m_Yoffset;
	QCheckBox* m_Subsect;
	QLabel* m_Xoffs;
	QLabel* m_Yoffs;

private slots:
	void SubsectToggled();
	void LoadPushed();
};

class ReloaderRaw : public QDialog
{
	Q_OBJECT
public:
	ReloaderRaw(SlicesHandler* hand3D, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~ReloaderRaw() override;
	//	void update();
	//protected:

private:
	SlicesHandler* m_Handler3D;
	char* m_Filename;
	Q3HBox* m_Hbox1;
	Q3HBox* m_Hbox2;
	Q3HBox* m_Hbox3;
	Q3HBox* m_Hbox4;
	Q3HBox* m_Hbox5;
	Q3HBox* m_Hbox6;
	//	QHBox *hbox7;
	Q3VBox* m_Vbox1;
	Q3VBox* m_Vbox2;
	QPushButton* m_SelectFile;
	QPushButton* m_LoadFile;
	QPushButton* m_CancelBut;
	QSpinBox* m_Xoffset;
	QSpinBox* m_Yoffset;
	QSpinBox* m_Xlength1;
	QSpinBox* m_Ylength1;
	QSpinBox* m_Slicenrbox;
	//	QSpinBox *sb_startnr;
	QCheckBox* m_Subsect;
	QLineEdit* m_NameEdit;
	QLabel* m_FileName;
	QButtonGroup* m_Bitselect;
	QRadioButton* m_Bit8;
	QRadioButton* m_Bit16;
	QLabel* m_Nrslice;
	QLabel* m_Xoffs;
	QLabel* m_Yoffs;
	QLabel* m_Xl1;
	QLabel* m_Yl1;
	//	QLabel *lb_startnr;

private slots:
	void SubsectToggled();
	void LoadPushed();
	void SelectPushed();
};

class NewImg : public QDialog
{
	Q_OBJECT
public:
	NewImg(SlicesHandler* hand3D, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~NewImg() override;

	bool NewPressed() const;
	//	void update();
	//protected:

private:
	SlicesHandler* m_Handler3D;
	Q3HBox* m_Hbox1;
	Q3HBox* m_Hbox2;
	Q3HBox* m_Hbox3;
	Q3VBox* m_Vbox1;
	QPushButton* m_NewFile;
	QPushButton* m_CancelBut;
	QSpinBox* m_Xlength;
	QSpinBox* m_Ylength;
	QSpinBox* m_SbNrslices;
	QLabel* m_Xl;
	QLabel* m_Yl;
	QLabel* m_LbNrslices;
	bool m_NewPressed;

private slots:
	void NewPushed();
	void OnClose();
};

class EditText : public QDialog
{
	Q_OBJECT
public:
	void SetEditableText(QString editable_text);
	QString GetEditableText();

	EditText(QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~EditText() override;

private:
	Q3HBox* m_Hbox1;
	Q3HBox* m_Hbox2;
	Q3VBox* m_Vbox1;

	QPushButton* m_SaveBut;
	QPushButton* m_CancelBut;

	QLineEdit* m_TextEdit;
};

class SupportedMultiDatasetTypes : public QDialog
{
	Q_OBJECT
public:
	enum eSupportedTypes { bmp,
		dcm,
		nifti,
		raw,
		vtk,
		nrSupportedTypes };

	inline QString ToQString(eSupportedTypes v) const
	{
		switch (v)
		{
		case bmp: return "BMP";
		case dcm: return "DICOM";
		case nifti: return "NIfTI";
		case raw: return "RAW";
		case vtk: return "VTK";
		default: return "[unsupported]";
		}
	}

	int GetSelectedType();

	SupportedMultiDatasetTypes(QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~SupportedMultiDatasetTypes() override;

private:
	Q3HBoxLayout* m_Hboxoverall;
	Q3VBoxLayout* m_Vboxoverall;

	std::vector<QRadioButton*> m_RadioButs;

	QPushButton* m_SelectBut;
	QPushButton* m_CancelBut;
};

} // namespace iseg

#endif
