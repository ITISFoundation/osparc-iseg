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

#include "SlicesHandler.h"
#include "bmp_read_1.h"

#include "Data/Point.h"

#include <QDialog>
#include <QLabel>

#include <array>

class QButtonGroup;
class QCheckBox;
class QComboBox;
class QDialog;
class QLineEdit;
class QPushButton;
class QRadioButton;
class QSlider;
class QSpinBox;
class QStringList;

namespace iseg {

class LoaderDicom : public QDialog
{
	Q_OBJECT
public:
	LoaderDicom(SlicesHandler* hand3D, QStringList* lname, bool breload, QWidget* parent = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~LoaderDicom() override;

private:
	SlicesHandler* m_Handler3D;
	QStringList* m_Lnames;
	bool m_Reload;
	Point m_P;
	unsigned short m_Dx;
	unsigned short m_Dy;

	QCheckBox* m_CbSubsect;
	QWidget* m_SubsectArea;
	QSpinBox* m_Xoffset;
	QSpinBox* m_Yoffset;
	QSpinBox* m_Xlength;
	QSpinBox* m_Ylength;

	QCheckBox* m_CbCt;
	QWidget* m_CtWeightArea;
	QCheckBox* m_CbCrop;
	QButtonGroup* m_BgWeight;
	QRadioButton* m_RbMuscle;
	QRadioButton* m_RbBone;
	QComboBox* m_Seriesnrselection;

	QPushButton* m_LoadFile;
	QPushButton* m_CancelBut;

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

	LoaderColorImages(SlicesHandler* hand3D, eImageType typ, std::vector<const char*> filenames, QWidget* parent = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
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
	ChannelMixer(std::vector<const char*> filenames, QWidget* parent = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~ChannelMixer() override;

	int GetRedFactor() const;
	int GetGreenFactor() const;
	int GetBlueFactor() const;

private:
	std::vector<const char*> m_MFilenames;
	SlicesHandler* m_Handler3D;

	ClickableLabel* m_ImageSourceLabel;
	QLabel* m_ImageLabel;
	QLabel* m_LabelPreviewAlgorithm;

	QSlider* m_SliderRed;
	QLineEdit* m_LabelRedValue;

	QSlider* m_SliderGreen;
	QLineEdit* m_LabelGreenValue;

	QSlider* m_SliderBlue;
	QLineEdit* m_LabelBlueValue;

	QSpinBox* m_SpinSlice;

	QPushButton* m_LoadFile;
	QPushButton* m_CancelBut;

	int m_RedFactorPv = 30;
	int m_GreenFactorPv = 59;
	int m_BlueFactorPv = 11;

	int m_RedFactor = 30;
	int m_GreenFactor = 59;
	int m_BlueFactor = 11;

	double m_ScaleX = 400;
	double m_ScaleY = 500;

	int m_SelectedSlice = 0;

	QImage m_SourceImage;

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

	void SliceValueChanged(int value);

	void CancelToggled();
	void LoadPushed();
};

class LoaderRaw : public QDialog
{
	Q_OBJECT
public:
	LoaderRaw(SlicesHandler* nullptrand3D, const QString& file_path, QWidget* parent = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~LoaderRaw() override;

	QString GetFileName() const;

	void SetSkipReading(bool skip) { m_SkipReading = skip; }

	std::array<unsigned int, 2> GetDimensions() const;
	std::array<unsigned int, 3> GetSubregionStart() const;
	std::array<unsigned int, 3> GetSubregionSize() const;
	int GetBits() const;

private:
	SlicesHandler* m_Handler3D;
	QString m_FileName;
	bool m_SkipReading = true;

	QButtonGroup* m_Bitselect;
	QRadioButton* m_Bit8;
	QRadioButton* m_Bit16;

	QCheckBox* m_Subsect;
	QWidget* m_SubsectArea;
	QSpinBox* m_Xoffset;
	QSpinBox* m_Yoffset;
	QSpinBox* m_Xlength;
	QSpinBox* m_Ylength;
	QSpinBox* m_Xlength1;
	QSpinBox* m_Ylength1;

	QSpinBox* m_Slicenrbox;
	QSpinBox* m_SbNrslices;

	QPushButton* m_LoadFile;
	QPushButton* m_CancelBut;

private slots:
	void SubsectToggled();
	void LoadPushed();
};

class ExportImg : public QDialog
{
	Q_OBJECT
public:
	ExportImg(SlicesHandler* hand3D, QWidget* parent = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
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
	ReloaderBmp2(SlicesHandler* hand3D, std::vector<const char*> filenames, QWidget* parent = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~ReloaderBmp2() override;

private:
	std::vector<const char*> m_MFilenames;
	SlicesHandler* m_Handler3D;
	
	QPushButton* m_LoadFile;
	QPushButton* m_CancelBut;
	QCheckBox* m_Subsect;
	QWidget* m_SubsectArea;
	QSpinBox* m_Xoffset;
	QSpinBox* m_Yoffset;

private slots:
	void SubsectToggled();
	void LoadPushed();
};

class ReloaderRaw : public QDialog
{
	Q_OBJECT
public:
	ReloaderRaw(SlicesHandler* hand3D, const QString& file_path, QWidget* parent = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~ReloaderRaw() override;

private:
	SlicesHandler* m_Handler3D;
	QString m_FileName;

	QButtonGroup* m_Bitselect;
	QRadioButton* m_Bit8;
	QRadioButton* m_Bit16;

	QCheckBox* m_Subsect;
	QWidget* m_SubsectArea;
	QSpinBox* m_Xlength1;
	QSpinBox* m_Ylength1;
	QSpinBox* m_Xoffset;
	QSpinBox* m_Yoffset;
	QSpinBox* m_Slicenrbox;

	QPushButton* m_LoadFile;
	QPushButton* m_CancelBut;

private slots:
	void SubsectToggled();
	void LoadPushed();
};

class NewImg : public QDialog
{
	Q_OBJECT
public:
	NewImg(SlicesHandler* hand3D, QWidget* parent = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~NewImg() override;

	bool NewPressed() const;

private:
	SlicesHandler* m_Handler3D;
	bool m_NewPressed;

	QPushButton* m_NewFile;
	QPushButton* m_CancelBut;
	QSpinBox* m_Xlength;
	QSpinBox* m_Ylength;
	QSpinBox* m_SbNrslices;

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

	EditText(QWidget* parent = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~EditText() override;

private:
	QPushButton* m_SaveBut;
	QPushButton* m_CancelBut;

	QLineEdit* m_TextEdit;
};

class SupportedMultiDatasetTypes : public QDialog
{
	Q_OBJECT
public:
	enum eSupportedTypes { kBMP,
		kDicom,
		kNifti,
		kMeta,
		kRaw,
		kVTK,
		keSupportedTypesSize };

	inline QString ToQString(eSupportedTypes v) const
	{
		switch (v)
		{
		case kBMP: return "BMP";
		case kDicom: return "DICOM";
		case kNifti: return "NIfTI";
		case kMeta: return "UNC Meta";
		case kRaw: return "RAW";
		case kVTK: return "VTK";
		default: return "[unsupported]";
		}
	}

	int GetSelectedType();

	SupportedMultiDatasetTypes(QWidget* parent = nullptr, Qt::WindowFlags wFlags = Qt::Widget);

private:
	std::vector<QRadioButton*> m_RadioButs;

	QPushButton* m_SelectBut;
	QPushButton* m_CancelBut;
};

} // namespace iseg

