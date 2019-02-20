/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
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
	LoaderDicom(SlicesHandler* hand3D, QStringList* lname, bool breload,
			QWidget* parent = 0, const char* name = 0,
			Qt::WindowFlags wFlags = 0);
	~LoaderDicom();

private:
	SlicesHandler* handler3D;
	QStringList* lnames;
	bool reload;
	Point p;
	unsigned short dx;
	unsigned short dy;
	Q3HBox* hbox1;
	Q3HBox* hbox2;
	Q3HBox* hbox3;
	Q3HBox* hbox4;
	Q3HBox* hbox5;
	Q3HBox* hbox6;
	Q3VBox* vbox1;
	Q3VBox* vbox2;
	Q3VBox* vbox3;
	Q3VBox* vbox4;
	Q3VBox* vbox5;
	Q3VBox* vbox6;
	QPushButton* loadFile;
	QPushButton* cancelBut;
	QSpinBox* xoffset;
	QSpinBox* yoffset;
	QSpinBox* xlength;
	QSpinBox* ylength;
	QCheckBox* cb_subsect;
	QCheckBox* cb_ct;
	QCheckBox* cb_crop;
	QButtonGroup* bg_weight;
	QRadioButton* rb_muscle;
	QRadioButton* rb_bone;
	QLabel* xoffs;
	QLabel* yoffs;
	QLabel* xl;
	QLabel* yl;
	QLabel* lb_title;
	QComboBox* seriesnrselection;
	std::vector<unsigned> dicomseriesnr;
	std::vector<unsigned> dicomseriesnrlist;

private slots:
	void subsect_toggled();
	void ct_toggled();
	void load_pushed();
};

class LoaderColorImages : public QDialog
{
	Q_OBJECT
public:
	enum eImageType {
		kPNG,
		kBMP,
		kJPG
	};

	LoaderColorImages(SlicesHandler* hand3D, eImageType typ, std::vector<const char*> filenames,
			QWidget* parent = 0, const char* name = 0,
			Qt::WindowFlags wFlags = 0);
	~LoaderColorImages();

	eImageType type = kPNG;

private:
	void load_mixer();
	void load_quantize();

	std::vector<const char*> m_filenames;
	SlicesHandler* handler3D;

	QCheckBox* map_to_lut;
	QCheckBox* subsect;
	QSpinBox* xoffset;
	QSpinBox* yoffset;
	QSpinBox* xlength;
	QSpinBox* ylength;
	QPushButton* loadFile;
	QPushButton* cancelBut;

private slots:
	void map_to_lut_toggled();
	void load_pushed();
};

class ClickableLabel : public QLabel
{
	Q_OBJECT

public:
	ClickableLabel(QWidget* parent = 0, Qt::WindowFlags f = 0);
	ClickableLabel(const QString& text, QWidget* parent = 0,
			Qt::WindowFlags f = 0);

	void SetSquareWidth(int width);
	void SetSquareHeight(int height);
	void SetCenter(QPoint newCenter);

private:
	int centerX;
	int centerY;
	int squareWidth;
	int squareHeight;

signals:
	void newCenterPreview(QPoint point);

private slots:
	void mousePressEvent(QMouseEvent* ev);
	void paintEvent(QPaintEvent* e);
};

class ChannelMixer : public QDialog
{
	Q_OBJECT
public:
	ChannelMixer(std::vector<const char*> filenames, QWidget* parent = 0,
			const char* name = 0, Qt::WindowFlags wFlags = 0);
	~ChannelMixer();

	int GetRedFactor();
	int GetGreenFactor();
	int GetBlueFactor();

private:
	std::vector<const char*> m_filenames;
	SlicesHandler* handler3D;

	Q3VBox* hboxChannelOptions;

	Q3VBox* vboxMain;

	Q3HBox* hboxImageAndControl;

	Q3VBox* hboxControl;
	Q3HBox* hboxButtons;

	Q3HBox* hboxImageSource;
	ClickableLabel* imageSourceLabel;

	Q3HBox* hboxImage;
	QLabel* imageLabel;

	Q3HBox* vboxRed;
	Q3HBox* vboxBlue;
	Q3HBox* vboxGreen;
	Q3HBox* vboxSlice;

	QLabel* labelRed;
	QSlider* sliderRed;
	QLineEdit* labelRedValue;
	QRadioButton* buttonRed;

	QLabel* labelGreen;
	QSlider* sliderGreen;
	QLineEdit* labelGreenValue;
	QRadioButton* buttonGreen;

	QLabel* labelBlue;
	QSlider* sliderBlue;
	QLineEdit* labelBlueValue;
	QRadioButton* buttonBlue;

	QLabel* labelPreviewAlgorithm;

	QSpinBox* spinSlice;
	QLabel* labelSliceValue;

	QPushButton* loadFile;
	QPushButton* cancelBut;

	int redFactor;
	int greenFactor;
	int blueFactor;

	int redFactorPV;
	int greenFactorPV;
	int blueFactorPV;
	int selectedSlice;

	QImage sourceImage;
	double scaleX;
	double scaleY;

	QPoint previewCenter;

	bool firstTime;
	int widthPV;
	int heightPV;

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
	void sliderRedValueChanged(int value);
	void sliderGreenValueChanged(int value);
	void sliderBlueValueChanged(int value);

	void labelRedValueChanged(QString text);
	void labelGreenValueChanged(QString text);
	void labelBlueValueChanged(QString text);

	void buttonRedPushed(bool checked);
	void buttonGreenPushed(bool checked);
	void buttonBluePushed(bool checked);

	void sliceValueChanged(int value);

	void cancel_toggled();
	void load_pushed();
};

class LoaderRaw : public QDialog
{
	Q_OBJECT
public:
	LoaderRaw(SlicesHandler* hand3D, QWidget* parent = 0, const char* name = 0,
			Qt::WindowFlags wFlags = 0);
	~LoaderRaw();

	QString GetFileName() const;

	void setSkipReading(bool skip) { skip_reading = skip; }

	std::array<unsigned int, 2> getDimensions() const;
	std::array<unsigned int, 3> getSubregionStart() const;
	std::array<unsigned int, 3> getSubregionSize() const;

private:
	SlicesHandler* handler3D;
	bool skip_reading = true;
	char* filename;
	short unsigned w;
	short unsigned h;
	unsigned bitdepth;
	unsigned short slicenr;
	Point p;
	unsigned short dx;
	unsigned short dy;
	Q3HBox* hbox1;
	Q3HBox* hbox2;
	Q3HBox* hbox3;
	Q3HBox* hbox4;
	Q3HBox* hbox5;
	Q3HBox* hbox6;
	Q3HBox* hbox7;
	Q3HBox* hbox8;
	Q3VBox* vbox1;
	Q3VBox* vbox2;
	QPushButton* selectFile;
	QPushButton* loadFile;
	QPushButton* cancelBut;
	QSpinBox* xoffset;
	QSpinBox* yoffset;
	QSpinBox* xlength;
	QSpinBox* ylength;
	QSpinBox* xlength1;
	QSpinBox* ylength1;
	QSpinBox* slicenrbox;
	QSpinBox* sb_nrslices;
	QCheckBox* subsect;
	QLineEdit* nameEdit;
	QLabel* fileName;
	QButtonGroup* bitselect;
	QRadioButton* bit8;
	QRadioButton* bit16;
	QLabel* nrslice;
	QLabel* xoffs;
	QLabel* yoffs;
	QLabel* xl;
	QLabel* yl;
	QLabel* xl1;
	QLabel* yl1;
	QLabel* lb_nrslices;

private slots:
	void subsect_toggled();
	void load_pushed();
	void select_pushed();
};

class SaverImg : public QDialog
{
	Q_OBJECT
public:
	SaverImg(SlicesHandler* hand3D, QWidget* parent = 0, const char* name = 0,
			Qt::WindowFlags wFlags = 0);
	~SaverImg();
	//	void update();
	//protected:

private:
	SlicesHandler* handler3D;
	char* filename;
	Q3HBox* hbox1;
	Q3HBox* hbox2;
	Q3HBox* hbox3;
	Q3HBox* hbox4;
	Q3VBox* vbox1;
	Q3VBox* vbox2;
	Q3VBox* vbox3;
	Q3VBox* vbox4;
	Q3VBox* vbox5;
	QPushButton* selectFile;
	QPushButton* saveFile;
	QPushButton* cancelBut;
	QLineEdit* nameEdit;
	QLabel* fileName;
	QButtonGroup* typeselect;
	QRadioButton* typeraw;
	QRadioButton* typebmp;
	QRadioButton* typevti;
	QRadioButton* typevtk;
	QRadioButton* typemat;
	QRadioButton* typenii;
	QButtonGroup* pictselect;
	QRadioButton* pictbmp;
	QRadioButton* pictwork;
	QRadioButton* picttissue;
	QButtonGroup* singleimg;
	QRadioButton* single1;
	QRadioButton* all1;

private slots:
	void save_pushed();
	void select_pushed();
	void type_changed(int);
};

class ReloaderBmp2 : public QDialog
{
	Q_OBJECT
public:
	ReloaderBmp2(SlicesHandler* hand3D, std::vector<const char*> filenames,
			QWidget* parent = 0, const char* name = 0,
			Qt::WindowFlags wFlags = 0);
	~ReloaderBmp2();
	//	void update();
	//protected:

private:
	std::vector<const char*> m_filenames;
	SlicesHandler* handler3D;
	char* filename;
	Q3HBox* hbox1;
	Q3HBox* hbox2;
	Q3HBox* hbox3;
	Q3HBox* hbox4;
	Q3VBox* vbox1;
	QPushButton* loadFile;
	QPushButton* cancelBut;
	QSpinBox* xoffset;
	QSpinBox* yoffset;
	QCheckBox* subsect;
	QLabel* xoffs;
	QLabel* yoffs;

private slots:
	void subsect_toggled();
	void load_pushed();
};

class ReloaderRaw : public QDialog
{
	Q_OBJECT
public:
	ReloaderRaw(SlicesHandler* hand3D, QWidget* parent = 0,
			const char* name = 0, Qt::WindowFlags wFlags = 0);
	~ReloaderRaw();
	//	void update();
	//protected:

private:
	SlicesHandler* handler3D;
	char* filename;
	Q3HBox* hbox1;
	Q3HBox* hbox2;
	Q3HBox* hbox3;
	Q3HBox* hbox4;
	Q3HBox* hbox5;
	Q3HBox* hbox6;
	//	QHBox *hbox7;
	Q3VBox* vbox1;
	Q3VBox* vbox2;
	QPushButton* selectFile;
	QPushButton* loadFile;
	QPushButton* cancelBut;
	QSpinBox* xoffset;
	QSpinBox* yoffset;
	QSpinBox* xlength1;
	QSpinBox* ylength1;
	QSpinBox* slicenrbox;
	//	QSpinBox *sb_startnr;
	QCheckBox* subsect;
	QLineEdit* nameEdit;
	QLabel* fileName;
	QButtonGroup* bitselect;
	QRadioButton* bit8;
	QRadioButton* bit16;
	QLabel* nrslice;
	QLabel* xoffs;
	QLabel* yoffs;
	QLabel* xl1;
	QLabel* yl1;
	//	QLabel *lb_startnr;

private slots:
	void subsect_toggled();
	void load_pushed();
	void select_pushed();
};

class NewImg : public QDialog
{
	Q_OBJECT
public:
	NewImg(SlicesHandler* hand3D, QWidget* parent = 0, const char* name = 0,
			Qt::WindowFlags wFlags = 0);
	~NewImg();

	bool new_pressed() const;
	//	void update();
	//protected:

private:
	SlicesHandler* handler3D;
	Q3HBox* hbox1;
	Q3HBox* hbox2;
	Q3HBox* hbox3;
	Q3VBox* vbox1;
	QPushButton* newFile;
	QPushButton* cancelBut;
	QSpinBox* xlength;
	QSpinBox* ylength;
	QSpinBox* sb_nrslices;
	QLabel* xl;
	QLabel* yl;
	QLabel* lb_nrslices;
	bool newPressed;

private slots:
	void new_pushed();
	void on_close();
};

class EditText : public QDialog
{
	Q_OBJECT
public:
	void set_editable_text(QString editable_text);
	QString get_editable_text();

	EditText(QWidget* parent = 0, const char* name = 0,
			Qt::WindowFlags wFlags = 0);
	~EditText();

private:
	Q3HBox* hbox1;
	Q3HBox* hbox2;
	Q3VBox* vbox1;

	QPushButton* saveBut;
	QPushButton* cancelBut;

	QLineEdit* text_edit;
};

class SupportedMultiDatasetTypes : public QDialog
{
	Q_OBJECT
public:
	enum supportedTypes { bmp,
		dcm,
		nifti,
		raw,
		vtk,
		nrSupportedTypes };

	inline QString ToQString(supportedTypes v)
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

	SupportedMultiDatasetTypes(QWidget* parent = 0, const char* name = 0,
			Qt::WindowFlags wFlags = 0);
	~SupportedMultiDatasetTypes();

private:
	Q3HBoxLayout* hboxoverall;
	Q3VBoxLayout* vboxoverall;

	std::vector<QRadioButton*> m_RadioButs;

	QPushButton* selectBut;
	QPushButton* cancelBut;
};

} // namespace iseg

#endif
