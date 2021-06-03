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

#include "AtlasViewer.h"

#include "Data/Point.h"

#include "Core/Pair.h"

#include <QDir>
#include <QScrollArea>
#include <QWidget>

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
	AtlasWidget(const char* filename, QDir picpath, QWidget* parent = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~AtlasWidget() override;
	bool m_IsOk;

private:
	bool Loadfile(const char* filename);

private:
	QLabel* m_LbContrast;
	QLabel* m_LbBrightness;
	QLabel* m_LbTransp;
	QLabel* m_LbName;
	QSlider* m_SlContrast;
	QSlider* m_SlBrightness;
	QSlider* m_SlTransp;
	AtlasViewer* m_AtlasViewer;
	QScrollArea* m_SaViewer;
	ZoomWidget* m_Zoomer;
	QScrollBar* m_ScbSlicenr;
	QButtonGroup* m_BgOrient;
	QRadioButton* m_RbX;
	QRadioButton* m_RbY;
	QRadioButton* m_RbZ;
	QHBoxLayout* m_Hbox1;
	QHBoxLayout* m_Hbox2;
	QHBoxLayout* m_Hbox3;
	QVBoxLayout* m_Vbox1;

	float* m_Image;
	tissues_size_t* m_Tissue;
	float m_Minval, m_Maxval;
	float m_Dx, m_Dy, m_Dz;
	unsigned short m_Dimx, m_Dimy, m_Dimz;
	std::vector<float> m_ColorR;
	std::vector<float> m_ColorG;
	std::vector<float> m_ColorB;
	std::vector<QString> m_TissueNames;

	QDir m_MPicpath;

private slots:
	void ScbSlicenrChanged();
	void SlTranspChanged();
	void XyzChanged();
	void PtMoved(tissues_size_t val);
	void SlBrightcontrMoved();
};

} // namespace iseg
