/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#pragma once

#include "AtlasViewer.h"

#include "Data/Point.h"

#include "Core/Pair.h"

#include <QScrollArea>
#include <QWidget>
#include <QDir>

#include <vector>

class QHBoxLayout;
class QVBoxLayout;
class QLabel;
class QButtonGroup;
class QRadioButton;
class QSlider;

namespace iseg {

class ZoomWidget;

class AtlasWidget : public QWidget
{
	Q_OBJECT
public:
	AtlasWidget(const char* filename, QDir picpath, QWidget* parent = 0,
			const char* name = 0, Qt::WindowFlags wFlags = 0);
	~AtlasWidget();
	bool isOK;

private:
	bool loadfile(const char* filename);

private:
	QLabel* lb_contrast;
	QLabel* lb_brightness;
	QLabel* lb_transp;
	QLabel* lb_name;
	QSlider* sl_contrast;
	QSlider* sl_brightness;
	QSlider* sl_transp;
	AtlasViewer* atlasViewer;
	QScrollArea* sa_viewer;
	ZoomWidget* zoomer;
	QScrollBar* scb_slicenr;
	QButtonGroup* bg_orient;
	QRadioButton* rb_x;
	QRadioButton* rb_y;
	QRadioButton* rb_z;
	QHBoxLayout* hbox1;
	QHBoxLayout* hbox2;
	QHBoxLayout* hbox3;
	QVBoxLayout* vbox1;

	float* image;
	tissues_size_t* tissue;
	float minval, maxval;
	float dx, dy, dz;
	unsigned short dimx, dimy, dimz;
	std::vector<float> color_r;
	std::vector<float> color_g;
	std::vector<float> color_b;
	std::vector<QString> tissue_names;

	QDir m_picpath;

private slots:
	void scb_slicenr_changed();
	void sl_transp_changed();
	void xyz_changed();
	void pt_moved(tissues_size_t val);
	void sl_brightcontr_moved();
};

} // namespace iseg
