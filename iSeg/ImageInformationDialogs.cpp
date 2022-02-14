/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
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

#include "Interface/QtConnect.h"

#include <vtkMath.h>

#include <QDoubleValidator>
#include <QFormLayout>
#include <QApplication>
#include <QLabel>
#include <QPushButton>
#include <QString>
#include <QWidget>

namespace iseg {

namespace {
bool orthonormalize(float* vecA, float* vecB)
{
	// Check whether input valid
	bool ok = true;
	float precision = 1.0e-07;

	// Input vectors non-zero length
	float len_a = std::sqrt(vtkMath::Dot(vecA, vecA));
	float len_b = std::sqrt(vtkMath::Dot(vecB, vecB));
	ok &= len_a >= precision;
	ok &= len_b >= precision;

	// Input vectors not collinear
	float tmp = std::abs(vtkMath::Dot(vecA, vecB));
	ok &= std::abs(tmp - len_a * len_b) >= precision;
	if (!ok)
	{
		return false;
	}

	float cross[3];
	vtkMath::Cross(vecA, vecB, cross);

	float a[3][3];
	for (unsigned short i = 0; i < 3; ++i)
	{
		a[0][i] = vecA[i];
		a[1][i] = vecB[i];
		a[2][i] = cross[i];
	}

	float b[3][3];
	vtkMath::Orthogonalize3x3(a, b);
	for (unsigned short i = 0; i < 3; ++i)
	{
		vecA[i] = b[0][i];
		vecB[i] = b[1][i];
	}

	vtkMath::Normalize(vecA);
	vtkMath::Normalize(vecB);

	return true;
}
} // namespace

PixelResize::PixelResize(SlicesHandler* hand3D, QWidget* parent, Qt::WindowFlags wFlags)
		: QDialog(parent, wFlags), m_Handler3D(hand3D)
{
	setModal(true);

	auto dx = m_Handler3D->Spacing();

	m_LeDx = new QLineEdit(QString::number(dx[0]));
	m_LeDx->setValidator(new QDoubleValidator);
	m_LeDy = new QLineEdit(QString::number(dx[1]));
	m_LeDy->setValidator(new QDoubleValidator);
	m_LeDz = new QLineEdit(QString::number(dx[2]));
	m_LeDz->setValidator(new QDoubleValidator);

	m_LeLx = new QLineEdit(QString::number(dx[0] * m_Handler3D->Width()));
	m_LeLx->setValidator(new QDoubleValidator);
	m_LeLy = new QLineEdit(QString::number(dx[1] * m_Handler3D->Height()));
	m_LeLy->setValidator(new QDoubleValidator);
	m_LeLz = new QLineEdit(QString::number(dx[2] * m_Handler3D->NumSlices()));
	m_LeLz->setValidator(new QDoubleValidator);

	m_PbResize = new QPushButton("Resize");
	m_PbClose = new QPushButton("Close");

	auto layout = new QFormLayout;
	layout->addRow("dx (mm)", m_LeDx);
	layout->addRow("dy (mm)", m_LeDy);
	layout->addRow("dz (mm)", m_LeDz);
	layout->addRow("lx (mm)", m_LeLx);
	layout->addRow("ly (mm)", m_LeLy);
	layout->addRow("lz (mm)", m_LeLz);
	layout->addRow(m_PbResize, m_PbClose);

	setLayout(layout);
	setMinimumHeight(std::max(200, layout->sizeHint().height()));
	setMinimumWidth(std::max(200, layout->sizeHint().width()));

	QObject_connect(m_PbResize, SIGNAL(clicked()), this, SLOT(ResizePressed()));
	QObject_connect(m_PbClose, SIGNAL(clicked()), this, SLOT(reject()));
	QObject_connect(m_LeDx, SIGNAL(textChanged(QString)), this, SLOT(DxChanged()));
	QObject_connect(m_LeDy, SIGNAL(textChanged(QString)), this, SLOT(DyChanged()));
	QObject_connect(m_LeDz, SIGNAL(textChanged(QString)), this, SLOT(DzChanged()));
	QObject_connect(m_LeLx, SIGNAL(textChanged(QString)), this, SLOT(LxChanged()));
	QObject_connect(m_LeLy, SIGNAL(textChanged(QString)), this, SLOT(LyChanged()));
	QObject_connect(m_LeLz, SIGNAL(textChanged(QString)), this, SLOT(LzChanged()));
}

void PixelResize::DxChanged()
{
	float dx1 = m_LeDx->text().toFloat();
	unsigned w = m_Handler3D->Width();
	m_LeLx->setText(QString::number(w * dx1));
}

void PixelResize::DyChanged()
{
	float dy1 = m_LeDy->text().toFloat();
	unsigned h = m_Handler3D->Height();
	m_LeLy->setText(QString::number(h * dy1));
}

void PixelResize::DzChanged()
{
	float dz1 = m_LeDz->text().toFloat();
	unsigned n = m_Handler3D->NumSlices();
	m_LeLz->setText(QString::number(n * dz1));
}

void PixelResize::LxChanged()
{
	float lx1 = m_LeLx->text().toFloat();
	unsigned w = m_Handler3D->Width();
	m_LeDx->setText(QString::number(lx1 / w));
}

void PixelResize::LyChanged()
{
	float ly1 = m_LeLy->text().toFloat();
	unsigned h = m_Handler3D->Height();
	m_LeDy->setText(QString::number(ly1 / h));
}

void PixelResize::LzChanged()
{
	float lz1 = m_LeLz->text().toFloat();
	unsigned n = m_Handler3D->NumSlices();
	m_LeDz->setText(QString::number(lz1 / n));
}

void PixelResize::ResizePressed()
{
	bool bx, by, bz;
	m_LeDx->text().toFloat(&bx);
	m_LeDy->text().toFloat(&by);
	m_LeDz->text().toFloat(&bz);
	if (bx && by && bz)
	{
		accept();
	}
	else
	{
		QApplication::beep();
	}
}

Vec3 PixelResize::GetPixelsize()
{
	Vec3 dx;
	dx[0] = m_LeDx->text().toFloat();
	dx[1] = m_LeDy->text().toFloat();
	dx[2] = m_LeDz->text().toFloat();
	return dx;
}

DisplacementDialog::DisplacementDialog(SlicesHandler* hand3D, QWidget* parent, Qt::WindowFlags wFlags)
		: QDialog(parent, wFlags), m_Handler3D(hand3D)
{
	setModal(true);

	m_Handler3D->GetDisplacement(m_Displacement);
	m_LeDispx = new QLineEdit(QString::number(m_Displacement[0]));
	m_LeDispx->setValidator(new QDoubleValidator);
	m_LeDispy = new QLineEdit(QString::number(m_Displacement[1]));
	m_LeDispy->setValidator(new QDoubleValidator);
	m_LeDispz = new QLineEdit(QString::number(m_Displacement[2]));
	m_LeDispz->setValidator(new QDoubleValidator);

	m_PbSet = new QPushButton("Set");
	m_PbClose = new QPushButton("Close");

	auto layout = new QFormLayout;
	layout->addRow("Offset x (mm)", m_LeDispx);
	layout->addRow("Offset y (mm)", m_LeDispy);
	layout->addRow("Offset z (mm)", m_LeDispz);
	layout->addRow(m_PbSet, m_PbClose);

	setLayout(layout);
	setMinimumHeight(layout->sizeHint().height());
	setMinimumWidth(std::max(200, layout->sizeHint().width()));

	QObject_connect(m_PbSet, SIGNAL(clicked()), this, SLOT(SetPressed()));
	QObject_connect(m_PbClose, SIGNAL(clicked()), this, SLOT(reject()));
	QObject_connect(m_LeDispx, SIGNAL(textChanged(QString)), this, SLOT(DispxChanged()));
	QObject_connect(m_LeDispy, SIGNAL(textChanged(QString)), this, SLOT(DispyChanged()));
	QObject_connect(m_LeDispz, SIGNAL(textChanged(QString)), this, SLOT(DispzChanged()));
}

void DisplacementDialog::DispxChanged()
{
	m_Displacement[0] = m_LeDispx->text().toFloat();
}

void DisplacementDialog::DispyChanged()
{
	m_Displacement[1] = m_LeDispy->text().toFloat();
}

void DisplacementDialog::DispzChanged()
{
	m_Displacement[2] = m_LeDispz->text().toFloat();
}

void DisplacementDialog::SetPressed()
{
	// this code just validates the values
	bool bx, by, bz;
	m_LeDispx->text().toFloat(&bx);
	m_LeDispy->text().toFloat(&by);
	m_LeDispz->text().toFloat(&bz);
	if (bx && by && bz)
	{
		accept();
	}
	else
	{
		QApplication::beep();
	}
}

void DisplacementDialog::ReturnDisplacement(float disp[3])
{
	disp[0] = m_Displacement[0];
	disp[1] = m_Displacement[1];
	disp[2] = m_Displacement[2];
}

RotationDialog::RotationDialog(SlicesHandler* hand3D, QWidget* parent, Qt::WindowFlags wFlags)
		: QDialog(parent, wFlags), m_Handler3D(hand3D)
{
	setModal(true);

	m_Handler3D->ImageTransform().GetRotation(m_Rotation);

	m_LeR11 = new QLineEdit(QString::number(m_Rotation[0][0]));
	m_LeR21 = new QLineEdit(QString::number(m_Rotation[1][0]));
	m_LeR31 = new QLineEdit(QString::number(m_Rotation[2][0]));

	m_LeR12 = new QLineEdit(QString::number(m_Rotation[0][1]));
	m_LeR22 = new QLineEdit(QString::number(m_Rotation[1][1]));
	m_LeR32 = new QLineEdit(QString::number(m_Rotation[2][1]));

	m_LeR13 = new QLineEdit(QString::number(m_Rotation[0][2]));
	m_LeR23 = new QLineEdit(QString::number(m_Rotation[1][2]));
	m_LeR33 = new QLineEdit(QString::number(m_Rotation[2][2]));

	QLineEdit* rot[] = {m_LeR11, m_LeR12, m_LeR13, m_LeR21, m_LeR22, m_LeR23, m_LeR31, m_LeR32, m_LeR33};
	for (auto le : rot)
	{
		le->setValidator(new QDoubleValidator);
	}

	// buttons
	m_PbOrthonorm = new QPushButton("Orthonormalize");
	m_PbSet = new QPushButton("Set");
	m_PbClose = new QPushButton("Cancel");

	// layout
	auto vbox1 = new QVBoxLayout;
	vbox1->addWidget(new QLabel(QString("Column 1")));
	vbox1->addWidget(m_LeR11);
	vbox1->addWidget(m_LeR21);
	vbox1->addWidget(m_LeR31);

	auto vbox2 = new QVBoxLayout;
	vbox2->addWidget(new QLabel(QString("Column 2")));
	vbox2->addWidget(m_LeR12);
	vbox2->addWidget(m_LeR22);
	vbox2->addWidget(m_LeR32);

	auto vbox3 = new QVBoxLayout;
	vbox3->addWidget(new QLabel(QString("Column 3")));
	vbox3->addWidget(m_LeR13);
	vbox3->addWidget(m_LeR23);
	vbox3->addWidget(m_LeR33);

	auto hbox_rot = new QHBoxLayout;
	hbox_rot->addLayout(vbox1);
	hbox_rot->addLayout(vbox2);
	hbox_rot->addLayout(vbox3);

	auto hbox_buttons = new QHBoxLayout;
	hbox_buttons->addWidget(m_PbOrthonorm);
	hbox_buttons->addWidget(m_PbSet);
	hbox_buttons->addWidget(m_PbClose);

	auto main_layout = new QVBoxLayout;
	main_layout->addWidget(new QLabel(QString("The rotation matrix of the image data")));
	main_layout->addLayout(hbox_rot);
	main_layout->addLayout(hbox_buttons);
	setLayout(main_layout);

	QObject_connect(m_PbOrthonorm, SIGNAL(clicked()), this, SLOT(OrthonormPressed()));
	QObject_connect(m_PbSet, SIGNAL(clicked()), this, SLOT(SetPressed()));
	QObject_connect(m_PbClose, SIGNAL(clicked()), this, SLOT(reject()));

	QObject_connect(m_LeR11, SIGNAL(textChanged(QString)), this, SLOT(RotationChanged()));
	QObject_connect(m_LeR12, SIGNAL(textChanged(QString)), this, SLOT(RotationChanged()));
	QObject_connect(m_LeR13, SIGNAL(textChanged(QString)), this, SLOT(RotationChanged()));
	QObject_connect(m_LeR21, SIGNAL(textChanged(QString)), this, SLOT(RotationChanged()));
	QObject_connect(m_LeR22, SIGNAL(textChanged(QString)), this, SLOT(RotationChanged()));
	QObject_connect(m_LeR23, SIGNAL(textChanged(QString)), this, SLOT(RotationChanged()));
	QObject_connect(m_LeR31, SIGNAL(textChanged(QString)), this, SLOT(RotationChanged()));
	QObject_connect(m_LeR32, SIGNAL(textChanged(QString)), this, SLOT(RotationChanged()));
	QObject_connect(m_LeR33, SIGNAL(textChanged(QString)), this, SLOT(RotationChanged()));
}

RotationDialog::~RotationDialog() = default;

void RotationDialog::RotationChanged()
{
	m_PbSet->setEnabled(true);
}

void RotationDialog::SetPressed()
{
	m_Rotation[0][0] = m_LeR11->text().toFloat();
	m_Rotation[0][1] = m_LeR12->text().toFloat();
	m_Rotation[0][2] = m_LeR13->text().toFloat();

	m_Rotation[1][0] = m_LeR21->text().toFloat();
	m_Rotation[1][1] = m_LeR22->text().toFloat();
	m_Rotation[1][2] = m_LeR23->text().toFloat();

	m_Rotation[2][0] = m_LeR31->text().toFloat();
	m_Rotation[2][1] = m_LeR32->text().toFloat();
	m_Rotation[2][2] = m_LeR33->text().toFloat();

	accept();
}

void RotationDialog::OrthonormPressed()
{
	float direction_cosines[6];
	direction_cosines[0] = m_LeR11->text().toFloat();
	direction_cosines[1] = m_LeR21->text().toFloat();
	direction_cosines[2] = m_LeR31->text().toFloat();

	direction_cosines[3] = m_LeR12->text().toFloat();
	direction_cosines[4] = m_LeR22->text().toFloat();
	direction_cosines[5] = m_LeR32->text().toFloat();

	if (orthonormalize(&direction_cosines[0], &direction_cosines[3]))
	{
		float offset[3] = {0, 0, 0};
		Transform tr(offset, direction_cosines);

		float rot[3][3];
		tr.GetRotation(rot);

		m_LeR11->setText(QString::number(rot[0][0]));
		m_LeR12->setText(QString::number(rot[0][1]));
		m_LeR13->setText(QString::number(rot[0][2]));

		m_LeR21->setText(QString::number(rot[1][0]));
		m_LeR22->setText(QString::number(rot[1][1]));
		m_LeR23->setText(QString::number(rot[1][2]));

		m_LeR31->setText(QString::number(rot[2][0]));
		m_LeR32->setText(QString::number(rot[2][1]));
		m_LeR33->setText(QString::number(rot[2][2]));

		m_PbSet->setEnabled(true);
	}
	else
	{
		QApplication::beep();
	}
}

void RotationDialog::GetRotation(float r[3][3])
{
	for (unsigned i = 0; i < 3; ++i)
	{
		for (unsigned j = 0; j < 3; ++j)
		{
			r[i][j] = m_Rotation[i][j];
		}
	}
}

ResizeDialog::ResizeDialog(SlicesHandler* hand3D, eResizeType type1, QWidget* parent, Qt::WindowFlags wFlags)
		: QDialog(parent, wFlags), m_Handler3D(hand3D), m_Resizetype(type1)
{
	setModal(true);

	m_ImageSourceLabel = new ImagePVLabel;
	// TODO BL: m_ImageSourceLabel->setFixedSize(m_VImagebox1->size());

	unsigned short active_slice = hand3D->ActiveSlice();
	unsigned short source_width = hand3D->Width();
	unsigned short source_height = hand3D->Height();
	float* data = hand3D->ReturnBmp(active_slice);

	QImage small_image(source_width, source_height, 32);
	small_image.fill(QColor(Qt::white).rgb());
	int pos = 0;
	int f;
	for (int j = source_height - 1; j >= 0; j--)
	{
		for (int i = 0; i < source_width; i++)
		{
			f = (int)std::max(0.0f, std::min(255.0f, (data)[pos]));
			small_image.setPixel(i, j, qRgb(f, f, f));
			pos++;
		}
	}

	m_ImageSourceLabel->clear();

	m_ImageSourceLabel->SetWidth(small_image.width());
	m_ImageSourceLabel->SetHeight(small_image.height());

	if (small_image.width() > small_image.height())
		m_ImageSourceLabel->SetScaledValue(small_image.width() / 400.0);
	else
		m_ImageSourceLabel->SetScaledValue(small_image.height() / 400.0);

	QImage scaled_image = small_image.scaled(400, 400, Qt::KeepAspectRatio);
	m_ImageSourceLabel->setPixmap(QPixmap::fromImage(scaled_image));
	m_ImageSourceLabel->setAlignment(Qt::AlignCenter);

	m_SbDxm = new QSpinBox;
	m_SbDxm->setValue(0);
	m_SbDym = new QSpinBox;
	m_SbDym->setValue(0);
	m_SbDzm = new QSpinBox;
	m_SbDzm->setValue(0);

	m_SbDxp = new QSpinBox;
	m_SbDxp->setValue(0);
	m_SbDyp = new QSpinBox;
	m_SbDyp->setValue(0);
	m_SbDzp = new QSpinBox;
	m_SbDzp->setValue(0);

	QString op;
	if (m_Resizetype != kCrop)
	{
		op = "Padding";

		if (m_Resizetype == kOther) // BL TODO what is this for?
		{
			m_SbDxm->setRange(-(int)hand3D->Width(), 1000);
			m_SbDym->setRange(-(int)hand3D->Width(), 1000);
			m_SbDzm->setRange(-(int)hand3D->NumSlices(), 1000);
			m_SbDxp->setRange(-(int)hand3D->Width(), 1000);
			m_SbDyp->setRange(-(int)hand3D->Width(), 1000);
			m_SbDzp->setRange(-(int)hand3D->NumSlices(), 1000);
		}
		else
		{
			m_SbDxm->setRange(0, 1000);
			m_SbDym->setRange(0, 1000);
			m_SbDzm->setRange(0, 1000);
			m_SbDxp->setRange(0, 1000);
			m_SbDyp->setRange(0, 1000);
			m_SbDzp->setRange(0, 1000);
		}
	}
	else
	{
		op = "Crop";

		m_SbDxm->setRange(0, (int)hand3D->Width());
		m_SbDym->setRange(0, (int)hand3D->Width());
		m_SbDzm->setRange(0, (int)hand3D->NumSlices());
		m_SbDxp->setRange(0, (int)hand3D->Width());
		m_SbDyp->setRange(0, (int)hand3D->Width());
		m_SbDzp->setRange(0, (int)hand3D->NumSlices());
	}

	m_PbSet = new QPushButton("Set");
	m_PbClose = new QPushButton("Close");

	// layout
	auto minus = new QFormLayout;
	minus->addRow(op + " x-", m_SbDxm);
	minus->addRow(op + " y-", m_SbDym);
	minus->addRow(op + " z-", m_SbDzm);

	auto plus = new QFormLayout;
	plus->addRow(op + " x+", m_SbDxp);
	plus->addRow(op + " y+", m_SbDyp);
	plus->addRow(op + " z+", m_SbDzp);

	auto options_layout = new QHBoxLayout;
	options_layout->addLayout(minus);
	options_layout->addLayout(plus);

	auto buttons_layout = new QHBoxLayout;
	buttons_layout->addWidget(m_PbSet);
	buttons_layout->addWidget(m_PbClose);

	auto left_layout = new QVBoxLayout;
	left_layout->addLayout(options_layout);
	left_layout->addLayout(buttons_layout);

	auto main_layout = new QHBoxLayout;
	main_layout->addLayout(left_layout);
	main_layout->addWidget(m_ImageSourceLabel);
	setLayout(main_layout);

	// connections
	QObject_connect(m_SbDxm, SIGNAL(valueChanged(int)), m_ImageSourceLabel, SLOT(SetXMin(int)));
	QObject_connect(m_SbDxp, SIGNAL(valueChanged(int)), m_ImageSourceLabel, SLOT(SetXMax(int)));
	QObject_connect(m_SbDym, SIGNAL(valueChanged(int)), m_ImageSourceLabel, SLOT(SetYMax(int)));
	QObject_connect(m_SbDyp, SIGNAL(valueChanged(int)), m_ImageSourceLabel, SLOT(SetYMin(int)));

	QObject_connect(m_PbSet, SIGNAL(clicked()), this, SLOT(SetPressed()));
	QObject_connect(m_PbClose, SIGNAL(clicked()), this, SLOT(reject()));
}

ResizeDialog::~ResizeDialog() = default;

void ResizeDialog::SetPressed()
{
	m_D[0] = m_SbDxm->value();
	m_D[1] = m_SbDxp->value();
	m_D[2] = m_SbDym->value();
	m_D[3] = m_SbDyp->value();
	m_D[4] = m_SbDzm->value();
	m_D[5] = m_SbDzp->value();
	accept();
}

void ResizeDialog::ReturnPadding(int& dxm, int& dxp, int& dym, int& dyp, int& dzm, int& dzp)
{
	if (m_Resizetype == kCrop)
	{
		dxm = -m_D[0];
		dxp = -m_D[1];
		dym = -m_D[2];
		dyp = -m_D[3];
		dzm = -m_D[4];
		dzp = -m_D[5];
		return;
	}

	dxm = m_D[0];
	dxp = m_D[1];
	dym = m_D[2];
	dyp = m_D[3];
	dzm = m_D[4];
	dzp = m_D[5];
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

	int width_offset = 0;
	int height_offset = 0;

	if (m_Width > m_Height)
		height_offset = (m_Width - m_Height) / 2;
	else
		width_offset = (m_Height - m_Width) / 2;

	QPainterPath square;
	square.addRect((m_XMin + width_offset) / m_Scale, (m_YMin + height_offset) / m_Scale, (m_Width - 1 - m_XMax - m_XMin) / m_Scale, (m_Height - 1 - m_YMax - m_YMin) / m_Scale);
	painter.drawPath(square);
}

} // namespace iseg
