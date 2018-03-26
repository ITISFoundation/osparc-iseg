/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>

class QSlider;

class GLWidget;

class Window : public QWidget
{
	Q_OBJECT

public:
	Window();
	~Window();
	QComboBox *tissuebox;
	void setDim(int* );
	void setSpace(float*);
	void setVoxel(std::vector<unsigned char*>);//(uchar ** voxels);
	void setisovalues(std::vector<int>);
	void setColours(std::vector<unsigned char>);
	int* dimension;
	float* spacing;
	std::vector<unsigned char*> voxels;
	std::vector<int> isovalues;
	std::vector<int> opacityvalues;
	std::vector<unsigned char> colours;
	QSlider *opacityslider;

protected:
	void keyPressEvent(QKeyEvent *event);

private:
	QSlider *createSlider();
	GLWidget *glWidget;
	QSlider *xSlider;
	QSlider *ySlider;
	QSlider *zSlider;
	QGroupBox *horizontalGroupBox;
	QLabel *txt_h;
};

#endif