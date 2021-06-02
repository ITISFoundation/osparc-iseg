/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef PIXELSIZE_28June05
#define PIXELSIZE_28June05

#include "Data/Vec3.h"

#include <q3hbox.h>
#include <q3vbox.h>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QSpinBox>
#include <QWidget>

namespace iseg {

class SlicesHandler;

class PixelResize : public QDialog
{
	Q_OBJECT
public:
	PixelResize(SlicesHandler* hand3D, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~PixelResize() override = default;
	Vec3 GetPixelsize();

private:
	SlicesHandler* m_Handler3D;

	QLineEdit* m_LeDx;
	QLineEdit* m_LeDy;
	QLineEdit* m_LeDz;
	QLineEdit* m_LeLx;
	QLineEdit* m_LeLy;
	QLineEdit* m_LeLz;

	QPushButton* m_PbResize;
	QPushButton* m_PbClose;

private slots:
	void DxChanged();
	void DyChanged();
	void DzChanged();
	void LxChanged();
	void LyChanged();
	void LzChanged();
	void ResizePressed();
};

class DisplacementDialog : public QDialog
{
	Q_OBJECT
public:
	DisplacementDialog(SlicesHandler* hand3D, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~DisplacementDialog() override = default;
	void ReturnDisplacement(float disp[3]);

private:
	float m_Displacement[3];
	SlicesHandler* m_Handler3D;

	QLineEdit* m_LeDispx;
	QLineEdit* m_LeDispy;
	QLineEdit* m_LeDispz;
	QPushButton* m_PbSet;
	QPushButton* m_PbClose;

private slots:
	void DispxChanged();
	void DispyChanged();
	void DispzChanged();
	void SetPressed();
};

class RotationDialog : public QDialog
{
	Q_OBJECT
public:
	RotationDialog(SlicesHandler* hand3D, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~RotationDialog() override;
	void GetRotation(float r[3][3]);

private:
	float m_Rotation[3][3];
	SlicesHandler* m_Handler3D;
	Q3HBox* m_Hbox1;
	Q3HBox* m_Hbox2;
	Q3HBox* m_Hbox3;
	Q3VBox* m_Vbox1;
	Q3VBox* m_Vbox2;
	Q3VBox* m_Vbox3;
	Q3VBox* m_Vbox4;
	QLabel* m_LbDescr;
	QLabel* m_LbC1;
	QLabel* m_LbC2;
	QLabel* m_LbC3;
	QLineEdit* m_LeR11;
	QLineEdit* m_LeR12;
	QLineEdit* m_LeR13;
	QLineEdit* m_LeR21;
	QLineEdit* m_LeR22;
	QLineEdit* m_LeR23;
	QLineEdit* m_LeR31;
	QLineEdit* m_LeR32;
	QLineEdit* m_LeR33;
	QPushButton* m_PbOrthonorm;
	QPushButton* m_PbSet;
	QPushButton* m_PbClose;

private slots:
	void RotationChanged();
	void SetPressed();
	void OrthonormPressed();
};

class ImagePVLabel : public QLabel
{
	Q_OBJECT

public:
	ImagePVLabel(QWidget* parent = nullptr, Qt::WindowFlags wFlags = Qt::Widget);

	void SetWidth(int value) { m_Width = value; }
	void SetHeight(int value) { m_Height = value; }
	void SetScaledValue(float value) { m_Scale = value; }

public slots:
	void SetXMin(int value);
	void SetXMax(int value);
	void SetYMin(int value);
	void SetYMax(int value);

private slots:
	void paintEvent(QPaintEvent* e) override;

protected:
	int m_XMin;
	int m_XMax;
	int m_YMin;
	int m_YMax;

	int m_Width;
	int m_Height;

	float m_Scale;
};

class ResizeDialog : public QDialog
{
	Q_OBJECT
public:
	enum eResizeType { kOther = 0,
		kPad = 1,
		kCrop = 2 };

	ResizeDialog(SlicesHandler* hand3D, eResizeType type1 = kOther, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~ResizeDialog() override;
	void ReturnPadding(int& dxm, int& dxp, int& dym, int& dyp, int& dzm, int& dzp);

private:
	int m_D[6];
	SlicesHandler* m_Handler3D;
	Q3HBox* m_Hbox1;
	Q3HBox* m_Hbox2;
	Q3VBox* m_Vbox1;
	Q3VBox* m_Vbox2;
	Q3VBox* m_Vbox3;
	Q3VBox* m_Vbox4;
	Q3VBox* m_Vbox5;
	QLabel* m_LbDxm;
	QLabel* m_LbDym;
	QLabel* m_LbDzm;
	QLabel* m_LbDxp;
	QLabel* m_LbDyp;
	QLabel* m_LbDzp;
	QSpinBox* m_SbDxm;
	QSpinBox* m_SbDym;
	QSpinBox* m_SbDzm;
	QSpinBox* m_SbDxp;
	QSpinBox* m_SbDyp;
	QSpinBox* m_SbDzp;
	QPushButton* m_PbSet;
	QPushButton* m_PbClose;
	Q3HBox* m_MainBox;
	Q3VBox* m_VImagebox1;
	ImagePVLabel* m_ImageSourceLabel;
	eResizeType m_Resizetype;

private slots:
	void SetPressed();
};

} // namespace iseg

#endif
