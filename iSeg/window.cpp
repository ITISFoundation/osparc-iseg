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
#if QT_VERSION >= 0x050000
# include <QtWidgets>
#else
# include <QtGui>
#endif

#include "glwidget.h"
#include "window.h"

Window::Window()
{
	glWidget = new GLWidget(this);
	dimension=new int[3];
	dimension[1]=0;
	dimension[2]=0;
	spacing=new float[3];
	spacing[0]=0;
	spacing[1]=0;
	spacing[2]=0;

	xSlider = createSlider();
	ySlider = createSlider();
	zSlider = createSlider();

	QSlider *slider = new QSlider(Qt::Horizontal);
	slider->setRange(0, 10);
	slider->setSingleStep(1);
	slider->setPageStep(1);
	slider->setTickInterval(1);
	slider->setTickPosition(QSlider::TicksBelow);
	opacityslider=slider;

	tissuebox=new QComboBox();

	connect(opacityslider, SIGNAL(valueChanged(int)),       glWidget, SLOT(setOpacity(int)));
	connect(tissuebox,	   SIGNAL(currentIndexChanged(int)),glWidget, SLOT(setSlider(int)));
	connect(xSlider,       SIGNAL(valueChanged(int)),   glWidget, SLOT(setXRotation(int)));
	connect(glWidget,      SIGNAL(xRotationChanged(int)),xSlider, SLOT(setValue(int)));
	connect(ySlider,       SIGNAL(valueChanged(int)),   glWidget, SLOT(setYRotation(int)));
	connect(glWidget,      SIGNAL(yRotationChanged(int)),ySlider, SLOT(setValue(int)));
	connect(zSlider,       SIGNAL(valueChanged(int)),   glWidget, SLOT(setZRotation(int)));
	connect(glWidget,      SIGNAL(zRotationChanged(int)),zSlider, SLOT(setValue(int)));

	QVBoxLayout *mainLayout = new QVBoxLayout;
	horizontalGroupBox = new QGroupBox(tr(""));
	QHBoxLayout *layout = new QHBoxLayout;
	layout->addWidget(glWidget);
	layout->addWidget(xSlider);
	layout->addWidget(ySlider);
	layout->addWidget(zSlider);
	horizontalGroupBox->setLayout(layout);
	mainLayout->addWidget(horizontalGroupBox);
	mainLayout->addWidget(tissuebox);
	txt_h=new QLabel("Opacity-Value (0 to 1) : ");
	mainLayout->addWidget(txt_h);
	mainLayout->addWidget(opacityslider);
	setLayout(mainLayout);

	xSlider->setValue(0);
	ySlider->setValue(0);
	zSlider->setValue(0);
	opacityslider->setValue(10);
	setWindowTitle(tr("Tissue 3D View"));
}

QSlider *Window::createSlider()
{
	QSlider *slider = new QSlider(Qt::Vertical);
	slider->setRange(0, 360 * 16);
	slider->setSingleStep(16);
	slider->setPageStep(15 * 16);
	slider->setTickInterval(15 * 16);
	slider->setTickPosition(QSlider::TicksRight);
	return slider;
}

void Window::keyPressEvent(QKeyEvent *e)
{
	if (e->key() == Qt::Key_Escape)
		close();
	else
		QWidget::keyPressEvent(e);
}

void Window:: setDim(int dims[3])
{
	dimension=dims;
}

void Window:: setSpace(float space[3])
{
	spacing=space;
}

void Window:: setVoxel(std::vector<unsigned char*> Voxelinput)
{
	voxels=Voxelinput;

	xSlider->setValue(0);
	ySlider->setValue(0);
	zSlider->setValue(0);
	opacityslider->setValue(10);
	
}

Window::~Window(){
	delete glWidget ;
	delete xSlider;
	delete ySlider;
	delete zSlider;
	delete opacityslider;
}

void Window:: setisovalues(std::vector<int> iso)
{
	isovalues=iso;

}

void Window:: setColours(std::vector<unsigned char> color)
{
	colours=color;

}

