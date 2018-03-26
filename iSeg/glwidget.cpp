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
#include <QtGlobal>
#if QT_VERSION >= 0x050000
# include <QtWidgets>
#else
# include <QtGui>
#endif
#include <QtOpenGL>

#include <math.h>

#include "glwidget.h"
#include "window.h"

#include "rawUtilities.hpp"
#include "gpu-mc.hpp"

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE  0x809D
#endif

GLWidget::GLWidget(Window *mainwin,QWidget *parent)
	: QGLWidget(QGLFormat(QGL::SampleBuffers), parent)
	//, gpumc(new GPUMC)
	, m_VoxelsSize(0)
	, mainwin(mainwin)
{
	xRot = 0;
	yRot = 0;
	zRot = 0;
	zoom=10;
	opacityvalues.clear();
	
	qtGreen = QColor::fromCmykF(0.40, 0.0, 1.0, 0.0);
	qtPurple = QColor::fromCmykF(0.39, 0.39, 0.0, 0.0);
}

GLWidget::~GLWidget()
{
	for(int i=gpumc.size()-1;i>=0;i--){
		delete gpumc[i];}
	m_Voxels.clear();
}

bool GLWidget::loadData(const char* filename, int dims[3], float spacing[3])
{
	std::copy(dims, dims+3, m_Dims);
	std::copy(spacing, spacing+3, m_Spacing);
	int size = dims[0]*dims[1]*dims[2];
	//m_Voxels = new unsigned char[size];
	//m_Voxels = mainwin->voxels[i];
	
	//if (m_Voxels)
	//{
	for(int i=0; i<mainwin->isovalues.size();++i){
		m_Voxels.push_back( new unsigned char[size]);
		m_Voxels[i] = mainwin->voxels[i];
		m_VoxelsSize = gpumc[i]->prepareDataset(&m_Voxels[i], dims[0], dims[1], dims[2]);}

	//	return (m_Voxels != 0);
	//}
	return true;// false;
}

QSize GLWidget::minimumSizeHint() const
{
	return QSize(50, 50);
}

QSize GLWidget::sizeHint() const
{
	return QSize(400, 400);
}

static void qNormalizeAngle(int &angle)
{
	while (angle < 0)
		angle += 360 * 16;
	while (angle > 360 * 16)
		angle -= 360 * 16;
}

void GLWidget::setXRotation(int angle)
{
	qNormalizeAngle(angle);
	if (angle != xRot)
	{
		xRot = angle;
		emit xRotationChanged(angle);
		updateGL();
	}
}

void GLWidget::setYRotation(int angle)
{
	qNormalizeAngle(angle);
	if (angle != yRot)
	{
		yRot = angle;
		emit yRotationChanged(angle);
		updateGL();
	}
}

void GLWidget::setZRotation(int angle)
{
	qNormalizeAngle(angle);
	if (angle != zRot)
	{
		zRot = angle;
		emit zRotationChanged(angle);
		updateGL();
	}
}


void GLWidget::initializeGL()
{
	initializeGLFunctions(QGLContext::currentContext());
	qglClearColor((1.0, 1.0, 1.0, 1.0));//qtPurple.dark());

	for(int i=0; i<mainwin->isovalues.size();++i){
	gpumc.push_back(new GPUMC);
	gpumc[i]->setIso(mainwin->isovalues[i]);
	}

	this->loadData("", mainwin->dimension, mainwin->spacing);
	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_LIGHTING);
	glEnable(GL_MULTISAMPLE);
	glEnable(GL_LIGHT0);
	//Light Version 1
	/*static GLfloat lightPosition[4] = {  m_VoxelsSize/2, m_VoxelsSize/2,  m_VoxelsSize/2, 0.0 };
	glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
    GLfloat light_ambient[] = { 1.0, 1.0, 1.0, 0.5 };
	//glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);*/
	
	//Light Version 2
	GLfloat ambientLight[] = { 0.5f, 0.5f, 0.5f,0.5f };
	GLfloat diffuseLight[] = { 0.3f, 0.3f, 0.3f, 1.0f };
	GLfloat specularLight[] = { 0.2f, 0.2f, 0.2f, 1.0f };
	GLfloat position[] = { -0.0f, 4.0f, 1.0f, 1.0f };
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
	glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight);
	glLightfv(GL_LIGHT0, GL_POSITION, position);
	
    
	for(int i=0; i<mainwin->isovalues.size();++i){
	if (gpumc[i])
	{
		try
		{
			
			gpumc[i]->setupOpenGL(m_VoxelsSize, m_Dims[0], m_Dims[1], m_Dims[2], m_Spacing[0], m_Spacing[1], m_Spacing[2]);
			if (!gpumc[i]->setupOpenCL(m_Voxels[i], m_VoxelsSize))
			{
				delete gpumc[i];
				gpumc[i] = 0;
			}
		}
		catch (...)
		{
		}
	}
	}
}

void GLWidget::paintGL()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	glTranslatef(0.0, 0.0, -10.0);
	glRotatef(xRot / 16.0, 1.0, 0.0, 0.0);
	glRotatef(yRot / 16.0, 0.0, 1.0, 0.0);
	glRotatef(zRot / 16.0, 0.0, 0.0, 1.0);
	//glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	for(int i=0; i<mainwin->isovalues.size();++i){
	if (gpumc[i])
	{
		int opacitycurrent;
		int k=mainwin->isovalues.size()-i-1;
		if(k>=opacityvalues.size()){
			opacitycurrent=10;
		}
		else{
			opacitycurrent=opacityvalues[k];
		}
		
		gpumc[i]->renderScene(sizeHint().width(), sizeHint().height(), xRot, yRot, zRot, zoom/10, 
			mainwin->colours[3*k], mainwin->colours[3*k+1], mainwin->colours[3*k+2], opacitycurrent);
	}
	}
}

void GLWidget::resizeGL(int width, int height)
{
	int side = qMin(width, height);
	glViewport((width - side) / 2, (height - side) / 2, side, side);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
#ifdef QT_OPENGL_ES_1
	glOrthof(-0.5, +0.5, -0.5, +0.5, 4.0, 15.0);
#else
	glOrtho(-0.5, +0.5, -0.5, +0.5, 4.0, 15.0);
#endif
	glMatrixMode(GL_MODELVIEW);
}

void GLWidget::mousePressEvent(QMouseEvent *event)
{
	lastPos = event->pos();
}

void GLWidget::mouseMoveEvent(QMouseEvent *event)
{
	int dx = event->x() - lastPos.x();
	int dy = event->y() - lastPos.y();

	if (event->buttons() & Qt::LeftButton)
	{
		setXRotation(xRot + 8 * dy);
		setYRotation(yRot + 8 * dx);
	}
	else if (event->buttons() & Qt::RightButton)
	{
		setXRotation(xRot + 8 * dy);
		setZRotation(zRot + 8 * dx);
	}
	lastPos = event->pos();
}

void GLWidget::wheelEvent(QWheelEvent *event)
{
	zoom+=event->delta()/64;
	updateGL();
	paintGL();
}

void GLWidget:: setOpacity(int op)
{
	int index =mainwin->tissuebox->currentIndex();
	if (opacityvalues.size() != mainwin->tissuebox->count()){
		for( int i=opacityvalues.size();i<mainwin->tissuebox->count();++i){
			opacityvalues.push_back(10);
		}
	}

	if(index!=-1){
	int oldop=opacityvalues[index];
	opacityvalues[index]=op;
		if(oldop!=op){
			updateGL();
			paintGL();
		}
	}

}

void GLWidget:: setSlider(int index)
{
	if (opacityvalues.size() != mainwin->tissuebox->count()){
		for( int i=opacityvalues.size();i<mainwin->tissuebox->count();++i){
			opacityvalues.push_back(10);
		}
	}
	mainwin->opacityslider->setValue(opacityvalues[index]);

}