/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef GLWIDGET_H
#define GLWIDGET_H

#include "window.h"
#include <QGLFunctions>
#include <QGLShaderProgram>
#include <QGLWidget>

class GPUMC;
//class Window;

class GLWidget : public QGLWidget, protected QGLFunctions
{
	Q_OBJECT
public:
	GLWidget(Window *mainwin, QWidget *parent = 0);
	~GLWidget();

	bool loadData(const char *filename, int dims[3], float spacing[3]);

	QSize minimumSizeHint() const;
	QSize sizeHint() const;
	void initializeGL();

public slots:
	void setXRotation(int angle);
	void setYRotation(int angle);
	void setZRotation(int angle);
	void setOpacity(int);
	void setSlider(int);

signals:
	void xRotationChanged(int angle);
	void yRotationChanged(int angle);
	void zRotationChanged(int angle);

protected:
	void paintGL();
	void resizeGL(int width, int height);
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void wheelEvent(QWheelEvent *event);

private:
	int xRot;
	int yRot;
	int zRot;
	float zoom;
	QPoint lastPos;
	QColor qtGreen;
	QColor qtPurple;
	Window *mainwin;

	std::vector<int> opacityvalues;
	std::vector<unsigned char *> m_Voxels;
	int m_VoxelsSize;
	int m_Dims[3];
	float m_Spacing[3];
	//GPUMC* gpumc;
	std::vector<GPUMC *> gpumc;
};

#endif