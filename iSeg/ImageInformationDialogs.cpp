/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "Precompiled.h"

#include "ImageInformationDialogs.h"
#include "SlicesHandler.h"

#include <vtkMath.h>

#include <q3hbox.h>
#include <q3vbox.h>
#include <qapplication.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qstring.h>
#include <qwidget.h>

using namespace std;
using namespace iseg;

namespace {
bool orthonormalize(float* vecA, float* vecB)
{
	// Check whether input valid
	bool ok = true;
	float precision = 1.0e-07;

	// Input vectors non-zero length
	float lenA = std::sqrtf(vtkMath::Dot(vecA, vecA));
	float lenB = std::sqrtf(vtkMath::Dot(vecB, vecB));
	ok &= lenA >= precision;
	ok &= lenB >= precision;

	// Input vectors not collinear
	float tmp = std::abs(vtkMath::Dot(vecA, vecB));
	ok &= std::abs(tmp - lenA * lenB) >= precision;
	if (!ok)
	{
		return false;
	}

	float cross[3];
	vtkMath::Cross(vecA, vecB, cross);

	float A[3][3];
	for (unsigned short i = 0; i < 3; ++i)
	{
		A[0][i] = vecA[i];
		A[1][i] = vecB[i];
		A[2][i] = cross[i];
	}

	float B[3][3];
	vtkMath::Orthogonalize3x3(A, B);
	for (unsigned short i = 0; i < 3; ++i)
	{
		vecA[i] = B[0][i];
		vecB[i] = B[1][i];
	}

	vtkMath::Normalize(vecA);
	vtkMath::Normalize(vecB);

	return true;
}
} // namespace

PixelResize::PixelResize(SlicesHandler* hand3D, QWidget* parent,
						 const char* name, Qt::WindowFlags wFlags)
	: QDialog(parent, name, TRUE, wFlags), handler3D(hand3D)
{
	vbox1 = new Q3VBox(this);
	hbox1 = new Q3HBox(vbox1);
	vbox2 = new Q3VBox(hbox1);
	vbox3 = new Q3VBox(hbox1);
	hbox2 = new Q3HBox(vbox1);

	lb_dx = new QLabel(QString("dx (mm): "), vbox2);
	lb_dy = new QLabel(QString("dy (mm): "), vbox2);
	lb_lx = new QLabel(QString("lx (mm): "), vbox2);
	lb_ly = new QLabel(QString("ly (mm): "), vbox2);

	Pair p1 = handler3D->get_pixelsize();

	dx = p1.high;
	dy = p1.low;
	le_dx = new QLineEdit(QString::number(p1.high), vbox3);
	le_dy = new QLineEdit(QString::number(p1.low), vbox3);
	le_lx = new QLineEdit(QString::number(p1.high * handler3D->return_width()),
						  vbox3);
	le_ly = new QLineEdit(QString::number(p1.low * handler3D->return_height()),
						  vbox3);

	pb_resize = new QPushButton("Resize", hbox2);
	pb_close = new QPushButton("Close", hbox2);

	hbox2->show();
	vbox2->show();
	vbox3->show();
	hbox1->show();
	vbox1->show();
	//setSizePolicy(QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed));

	vbox1->setFixedSize(vbox1->sizeHint());

	QObject::connect(pb_resize, SIGNAL(clicked()), this,
					 SLOT(resize_pressed()));
	QObject::connect(pb_close, SIGNAL(clicked()), this, SLOT(reject()));
	QObject::connect(le_dx, SIGNAL(textChanged(const QString&)), this,
					 SLOT(dx_changed()));
	QObject::connect(le_dy, SIGNAL(textChanged(const QString&)), this,
					 SLOT(dy_changed()));
	QObject::connect(le_lx, SIGNAL(textChanged(const QString&)), this,
					 SLOT(lx_changed()));
	QObject::connect(le_ly, SIGNAL(textChanged(const QString&)), this,
					 SLOT(ly_changed()));

	return;
}

PixelResize::~PixelResize() { delete vbox1; }

void PixelResize::dx_changed()
{
	bool b1;
	float dx1 = le_dx->text().toFloat(&b1);
	if (b1)
	{
		unsigned w = handler3D->return_width();
		le_lx->setText(QString::number(w * dx1));
		dx = dx1;
	}
	else
	{
		if (le_dx->text() != QString("."))
			QApplication::beep();
	}
}

void PixelResize::dy_changed()
{
	bool b1;
	float dy1 = le_dy->text().toFloat(&b1);
	if (b1)
	{
		unsigned h = handler3D->return_height();
		le_ly->setText(QString::number(h * dy1));
		dy = dy1;
	}
	else
	{
		if (le_dy->text() != QString("."))
			QApplication::beep();
	}
}

void PixelResize::lx_changed()
{
	bool b1;
	float lx1 = le_lx->text().toFloat(&b1);
	if (b1)
	{
		unsigned w = handler3D->return_width();
		le_dx->setText(QString::number(lx1 / w));
		dx = lx1 / w;
	}
	else
	{
		if (le_lx->text() != QString("."))
			QApplication::beep();
	}
}

void PixelResize::ly_changed()
{
	bool b1;
	float ly1 = le_ly->text().toFloat(&b1);
	if (b1)
	{
		unsigned h = handler3D->return_height();
		le_dy->setText(QString::number(ly1 / h));
		dy = ly1 / h;
	}
	else
	{
		if (le_ly->text() != QString("."))
			QApplication::beep();
	}
}

void PixelResize::resize_pressed()
{
	bool bx, by;
	float dx1 = le_dx->text().toFloat(&bx);
	float dy1 = le_dy->text().toFloat(&by);
	if (bx && by)
	{
		accept();
	}
	else
	{
		QApplication::beep();
	}
}

Pair PixelResize::return_pixelsize()
{
	Pair p1;
	p1.high = dx;
	p1.low = dy;
	return p1;
}

DisplacementDialog::DisplacementDialog(SlicesHandler* hand3D, QWidget* parent,
									   const char* name, Qt::WindowFlags wFlags)
	: QDialog(parent, name, TRUE, wFlags), handler3D(hand3D)
{
	vbox1 = new Q3VBox(this);
	hbox1 = new Q3HBox(vbox1);
	vbox2 = new Q3VBox(hbox1);
	vbox3 = new Q3VBox(hbox1);
	hbox2 = new Q3HBox(vbox1);

	lb_dispx = new QLabel(QString("displacement x (mm): "), vbox2);
	lb_dispy = new QLabel(QString("displacement y (mm): "), vbox2);
	lb_dispz = new QLabel(QString("displacement z (mm): "), vbox2);

	handler3D->get_displacement(displacement);
	le_dispx = new QLineEdit(QString::number(displacement[0]), vbox3);
	le_dispy = new QLineEdit(QString::number(displacement[1]), vbox3);
	le_dispz = new QLineEdit(QString::number(displacement[2]), vbox3);

	pb_set = new QPushButton("Set", hbox2);
	pb_close = new QPushButton("Close", hbox2);

	hbox2->show();
	vbox2->show();
	vbox3->show();
	hbox1->show();
	vbox1->show();
	setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));

	vbox1->setFixedSize(vbox1->sizeHint());

	QObject::connect(pb_set, SIGNAL(clicked()), this, SLOT(set_pressed()));
	QObject::connect(pb_close, SIGNAL(clicked()), this, SLOT(reject()));
	QObject::connect(le_dispx, SIGNAL(textChanged(const QString&)), this,
					 SLOT(dispx_changed()));
	QObject::connect(le_dispy, SIGNAL(textChanged(const QString&)), this,
					 SLOT(dispy_changed()));
	QObject::connect(le_dispz, SIGNAL(textChanged(const QString&)), this,
					 SLOT(dispz_changed()));

	return;
}

DisplacementDialog::~DisplacementDialog() { delete vbox1; }

void DisplacementDialog::dispx_changed()
{
	bool b1;
	float dx1 = le_dispx->text().toFloat(&b1);
	if (b1)
	{
		displacement[0] = dx1;
	}
	else
	{
		if (le_dispx->text() != QString(".") &&
			le_dispx->text() != QString("-"))
			QApplication::beep();
	}
}

void DisplacementDialog::dispy_changed()
{
	bool b1;
	float dy1 = le_dispy->text().toFloat(&b1);
	if (b1)
	{
		displacement[1] = dy1;
	}
	else
	{
		if (le_dispy->text() != QString(".") &&
			le_dispy->text() != QString("-"))
			QApplication::beep();
	}
}

void DisplacementDialog::dispz_changed()
{
	bool b1;
	float dz1 = le_dispz->text().toFloat(&b1);
	if (b1)
	{
		displacement[2] = dz1;
	}
	else
	{
		if (le_dispz->text() != QString(".") &&
			le_dispz->text() != QString("-"))
			QApplication::beep();
	}
}

void DisplacementDialog::set_pressed()
{
	bool bx, by, bz;
	float dx1 = le_dispx->text().toFloat(&bx);
	float dy1 = le_dispy->text().toFloat(&by);
	float dz1 = le_dispz->text().toFloat(&bz);
	if (bx && by && bz)
	{
		accept();
	}
	else
	{
		QApplication::beep();
	}
}

void DisplacementDialog::return_displacement(float disp[3])
{
	disp[0] = displacement[0];
	disp[1] = displacement[1];
	disp[2] = displacement[2];
	return;
}

RotationDialog::RotationDialog(SlicesHandler* hand3D, QWidget* parent,
							   const char* name, Qt::WindowFlags wFlags)
	: QDialog(parent, name, TRUE, wFlags), handler3D(hand3D)
{
	handler3D->get_direction_cosines(directionCosines);

	vbox1 = new Q3VBox(this);
	vbox1->setMargin(5);

	// Description
	hbox1 = new Q3HBox(vbox1);
	hbox1->setMargin(5);
	lb_descr = new QLabel(
		QString("Direction cosines of the first row and first column\nof the "
				"data set w.r.t. the world coordinate system."),
		hbox1);

	// Values
	hbox2 = new Q3HBox(vbox1);
	hbox2->setMargin(5);

	vbox2 = new Q3VBox(hbox2);
	lb_row = new QLabel(QString("Row"), vbox2);
	le_dca = new QLineEdit(QString::number(directionCosines[0]), vbox2);
	le_dcb = new QLineEdit(QString::number(directionCosines[1]), vbox2);
	le_dcc = new QLineEdit(QString::number(directionCosines[2]), vbox2);

	vbox3 = new Q3VBox(hbox2);
	lb_col = new QLabel(QString("Column"), vbox3);
	le_dcx = new QLineEdit(QString::number(directionCosines[3]), vbox3);
	le_dcy = new QLineEdit(QString::number(directionCosines[4]), vbox3);
	le_dcz = new QLineEdit(QString::number(directionCosines[5]), vbox3);

	// Buttons
	hbox3 = new Q3HBox(vbox1);
	hbox3->setMargin(5);
	pb_orthonorm = new QPushButton("Orthonormalize", hbox3);
	pb_set = new QPushButton("Set", hbox3);
	pb_close = new QPushButton("Close", hbox3);

	setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));

	vbox1->setFixedSize(vbox1->sizeHint());

	QObject::connect(pb_orthonorm, SIGNAL(clicked()), this,
					 SLOT(orthonorm_pressed()));
	QObject::connect(pb_set, SIGNAL(clicked()), this, SLOT(set_pressed()));
	QObject::connect(pb_close, SIGNAL(clicked()), this, SLOT(reject()));
	QObject::connect(le_dca, SIGNAL(textChanged(const QString&)), this,
					 SLOT(dca_changed()));
	QObject::connect(le_dcb, SIGNAL(textChanged(const QString&)), this,
					 SLOT(dcb_changed()));
	QObject::connect(le_dcc, SIGNAL(textChanged(const QString&)), this,
					 SLOT(dcc_changed()));
	QObject::connect(le_dcx, SIGNAL(textChanged(const QString&)), this,
					 SLOT(dcx_changed()));
	QObject::connect(le_dcy, SIGNAL(textChanged(const QString&)), this,
					 SLOT(dcy_changed()));
	QObject::connect(le_dcz, SIGNAL(textChanged(const QString&)), this,
					 SLOT(dcz_changed()));

	return;
}

RotationDialog::~RotationDialog() { delete vbox1; }

void RotationDialog::ParseValue(QLineEdit* le, unsigned short dcIndex)
{
	bool b1;
	float val = le->text().toFloat(&b1);
	if (b1)
	{
		directionCosines[dcIndex] = val;
	}
	else
	{
		if (le->text() != QString(".") && le->text() != QString("-"))
			QApplication::beep();
	}
}

void RotationDialog::dca_changed()
{
	ParseValue(le_dca, 0);
	pb_set->setEnabled(false);
}

void RotationDialog::dcb_changed()
{
	ParseValue(le_dcb, 1);
	pb_set->setEnabled(false);
}

void RotationDialog::dcc_changed()
{
	ParseValue(le_dcc, 2);
	pb_set->setEnabled(false);
}

void RotationDialog::dcx_changed()
{
	ParseValue(le_dcx, 3);
	pb_set->setEnabled(false);
}

void RotationDialog::dcy_changed()
{
	ParseValue(le_dcy, 4);
	pb_set->setEnabled(false);
}

void RotationDialog::dcz_changed()
{
	ParseValue(le_dcz, 5);
	pb_set->setEnabled(false);
}

void RotationDialog::set_pressed()
{
	bool ok[6];
	le_dca->text().toFloat(&ok[0]);
	le_dcb->text().toFloat(&ok[1]);
	le_dcc->text().toFloat(&ok[2]);
	le_dcx->text().toFloat(&ok[3]);
	le_dcy->text().toFloat(&ok[4]);
	le_dcz->text().toFloat(&ok[5]);
	bool tmp = true;
	for (unsigned short i = 0; i < 6; ++i)
	{
		tmp &= ok[i];
	}
	if (tmp)
	{
		accept();
	}
	else
	{
		QApplication::beep();
	}
}

void RotationDialog::orthonorm_pressed()
{
	if (orthonormalize(&directionCosines[0], &directionCosines[3]))
	{
		le_dca->setText(QString::number(directionCosines[0]));
		le_dcb->setText(QString::number(directionCosines[1]));
		le_dcc->setText(QString::number(directionCosines[2]));
		le_dcx->setText(QString::number(directionCosines[3]));
		le_dcy->setText(QString::number(directionCosines[4]));
		le_dcz->setText(QString::number(directionCosines[5]));
		pb_set->setEnabled(true);
	}
	else
	{
		QApplication::beep();
	}
}

void RotationDialog::return_direction_cosines(float dc[6])
{
	for (unsigned short i = 0; i < 6; ++i)
	{
		dc[i] = directionCosines[i];
	}
	return;
}

ResizeDialog::ResizeDialog(SlicesHandler* hand3D, eResizeType type1,
						   QWidget* parent, const char* name,
						   Qt::WindowFlags wFlags)
	: QDialog(parent, name, TRUE, wFlags), handler3D(hand3D), resizetype(type1)
{
	mainBox = new Q3HBox(this);
	vbox1 = new Q3VBox(mainBox);
	hbox1 = new Q3HBox(vbox1);
	vbox2 = new Q3VBox(hbox1);
	vbox3 = new Q3VBox(hbox1);
	vbox4 = new Q3VBox(hbox1);
	vbox5 = new Q3VBox(hbox1);
	hbox2 = new Q3HBox(vbox1);

	vImagebox1 = new Q3VBox(mainBox);
	vImagebox1->setFixedSize(QSize(400, 400));

	m_ImageSourceLabel = new ImagePVLabel(vImagebox1);
	m_ImageSourceLabel->setFixedSize(vImagebox1->size());

	unsigned short activeSlice = hand3D->get_activeslice();
	unsigned short sourceWidth = hand3D->return_width();
	unsigned short sourceHeight = hand3D->return_height();
	float* data = hand3D->return_bmp(activeSlice);

	QImage smallImage(sourceWidth, sourceHeight, 32);
	smallImage.fill(QColor(Qt::white).rgb());
	int pos = 0;
	int f;
	for (int j = sourceHeight - 1; j >= 0; j--)
	{
		for (int i = 0; i < sourceWidth; i++)
		{
			f = (int)max(0.0f, min(255.0f, (data)[pos]));
			smallImage.setPixel(i, j, qRgb(f, f, f));
			pos++;
		}
	}

	m_ImageSourceLabel->clear();

	m_ImageSourceLabel->SetWidth(smallImage.width());
	m_ImageSourceLabel->SetHeight(smallImage.height());

	if (smallImage.width() > smallImage.height())
		m_ImageSourceLabel->SetScaledValue(smallImage.width() / 400.0);
	else
		m_ImageSourceLabel->SetScaledValue(smallImage.height() / 400.0);

	QImage scaledImage = smallImage.scaled(400, 400, Qt::KeepAspectRatio);
	//m_ImageSourceLabel->setScaledContents(true);
	m_ImageSourceLabel->setPixmap(QPixmap::fromImage(scaledImage));
	m_ImageSourceLabel->setAlignment(Qt::AlignCenter);

	/*
	if( sourceWidth<400 )
		vImagebox1->setFixedWidth(sourceWidth);
	if( sourceHeight<400 )
		vImagebox1->setFixedHeight(sourceHeight);
	vImagebox1->update();
	*/

	if (resizetype != kCrop)
	{
		lb_dxm = new QLabel(QString("padding x- (pixels): "), vbox2);
		lb_dym = new QLabel(QString("padding y- (pixels): "), vbox2);
		lb_dzm = new QLabel(QString("padding z- (pixels): "), vbox2);

		lb_dxp = new QLabel(QString("padding x+ (pixels): "), vbox4);
		lb_dyp = new QLabel(QString("padding y+ (pixels): "), vbox4);
		lb_dzp = new QLabel(QString("padding z+ (pixels): "), vbox4);

		if (resizetype == kOther) // BL TODO what is this for?
		{
			sb_dxm = new QSpinBox(-(int)hand3D->return_width(), 1000, 1, vbox3);
			sb_dxm->setValue(0);
			sb_dym =
				new QSpinBox(-(int)hand3D->return_height(), 1000, 1, vbox3);
			sb_dym->setValue(0);
			sb_dzm =
				new QSpinBox(-(int)hand3D->return_nrslices(), 1000, 1, vbox3);
			sb_dzm->setValue(0);

			sb_dxp = new QSpinBox(-(int)hand3D->return_width(), 1000, 1, vbox5);
			sb_dxp->setValue(0);
			sb_dyp =
				new QSpinBox(-(int)hand3D->return_height(), 1000, 1, vbox5);
			sb_dyp->setValue(0);
			sb_dzp =
				new QSpinBox(-(int)hand3D->return_nrslices(), 1000, 1, vbox5);
			sb_dzp->setValue(0);
		}
		else
		{
			sb_dxm = new QSpinBox(0, 1000, 1, vbox3);
			sb_dxm->setValue(0);
			sb_dym = new QSpinBox(0, 1000, 1, vbox3);
			sb_dym->setValue(0);
			sb_dzm = new QSpinBox(0, 1000, 1, vbox3);
			sb_dzm->setValue(0);

			sb_dxp = new QSpinBox(0, 1000, 1, vbox5);
			sb_dxp->setValue(0);
			sb_dyp = new QSpinBox(0, 1000, 1, vbox5);
			sb_dyp->setValue(0);
			sb_dzp = new QSpinBox(0, 1000, 1, vbox5);
			sb_dzp->setValue(0);
		}
	}
	else
	{
		lb_dxm = new QLabel(QString("crop x- (pixels): "), vbox2);
		lb_dym = new QLabel(QString("crop y- (pixels): "), vbox2);
		lb_dzm = new QLabel(QString("crop z- (pixels): "), vbox2);

		lb_dxp = new QLabel(QString("crop x+ (pixels): "), vbox4);
		lb_dyp = new QLabel(QString("crop y+ (pixels): "), vbox4);
		lb_dzp = new QLabel(QString("crop z+ (pixels): "), vbox4);

		sb_dxm = new QSpinBox(0, (int)hand3D->return_width(), 1, vbox3);
		sb_dxm->setValue(0);
		sb_dym = new QSpinBox(0, (int)hand3D->return_height(), 1, vbox3);
		sb_dym->setValue(0);
		sb_dzm = new QSpinBox(0, (int)hand3D->return_nrslices(), 1, vbox3);
		sb_dzm->setValue(0);

		sb_dxp = new QSpinBox(0, (int)hand3D->return_width(), 1, vbox5);
		sb_dxp->setValue(0);
		sb_dyp = new QSpinBox(0, (int)hand3D->return_height(), 1, vbox5);
		sb_dyp->setValue(0);
		sb_dzp = new QSpinBox(0, (int)hand3D->return_nrslices(), 1, vbox5);
		sb_dzp->setValue(0);
	}

	pb_set = new QPushButton("Set", hbox2);
	pb_close = new QPushButton("Close", hbox2);

	hbox2->show();
	vbox2->show();
	vbox3->show();
	vbox4->show();
	vbox5->show();
	hbox1->show();
	vbox1->show();

	mainBox->show();

	setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));

	vbox1->setFixedSize(vbox1->sizeHint());

	QObject::connect(sb_dxm, SIGNAL(valueChanged(int)), m_ImageSourceLabel,
					 SLOT(SetXMin(int)));
	QObject::connect(sb_dxp, SIGNAL(valueChanged(int)), m_ImageSourceLabel,
					 SLOT(SetXMax(int)));
	QObject::connect(sb_dym, SIGNAL(valueChanged(int)), m_ImageSourceLabel,
					 SLOT(SetYMax(int)));
	QObject::connect(sb_dyp, SIGNAL(valueChanged(int)), m_ImageSourceLabel,
					 SLOT(SetYMin(int)));

	QObject::connect(pb_set, SIGNAL(clicked()), this, SLOT(set_pressed()));
	QObject::connect(pb_close, SIGNAL(clicked()), this, SLOT(reject()));

	return;
}

ResizeDialog::~ResizeDialog() { delete vbox1; }

void ResizeDialog::set_pressed()
{
	d[0] = sb_dxm->value();
	d[1] = sb_dxp->value();
	d[2] = sb_dym->value();
	d[3] = sb_dyp->value();
	d[4] = sb_dzm->value();
	d[5] = sb_dzp->value();
	accept();
}

void ResizeDialog::return_padding(int& dxm, int& dxp, int& dym, int& dyp,
								  int& dzm, int& dzp)
{
	if (resizetype == kCrop)
	{
		dxm = -d[0];
		dxp = -d[1];
		dym = -d[2];
		dyp = -d[3];
		dzm = -d[4];
		dzp = -d[5];
		return;
	}

	dxm = d[0];
	dxp = d[1];
	dym = d[2];
	dyp = d[3];
	dzm = d[4];
	dzp = d[5];

	return;
}

ImagePVLabel::ImagePVLabel(QWidget* parent, Qt::WindowFlags f)
	: QLabel(parent, f)
{
	m_XMin = 0;
	m_XMax = 0;
	m_YMin = 0;
	m_YMax = 0;

	m_Width = 0;
	m_Height = 0;

	m_Scale = 1;
}

void ImagePVLabel::SetXMin(int value)
{
	m_XMin = value;
	this->update();
}

void ImagePVLabel::SetXMax(int value)
{
	m_XMax = value;
	this->update();
}

void ImagePVLabel::SetYMin(int value)
{
	m_YMin = value;
	this->update();
}

void ImagePVLabel::SetYMax(int value)
{
	m_YMax = value;
	this->update();
}

void ImagePVLabel::paintEvent(QPaintEvent* e)
{
	QLabel::paintEvent(e);

	QPainter painter(this);

	QPen paintpen(Qt::yellow);
	paintpen.setWidth(1);
	painter.setPen(paintpen);

	int widthOffset = 0;
	int heightOffset = 0;

	if (m_Width > m_Height)
		heightOffset = (m_Width - m_Height) / 2;
	else
		widthOffset = (m_Height - m_Width) / 2;

	QPainterPath square;
	square.addRect((m_XMin + widthOffset) / m_Scale,
				   (m_YMin + heightOffset) / m_Scale,
				   (m_Width - 1 - m_XMax - m_XMin) / m_Scale,
				   (m_Height - 1 - m_YMax - m_YMin) / m_Scale);
	painter.drawPath(square);
}
