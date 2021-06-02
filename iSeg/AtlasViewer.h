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

#include "Data/Point.h"

#include "Data/Types.h"

#include "Core/Pair.h"

#include <QImage>
#include <QWidget>

#include <vector>

namespace iseg {

class AtlasViewer : public QWidget
{
	Q_OBJECT
public:
	AtlasViewer(float* bmpbits1, tissues_size_t* tissue1, unsigned char orient1, unsigned short dimx1, unsigned short dimy1, unsigned short dimz1, float dx1, float dy1, float dz1, std::vector<float>* r, std::vector<float>* g, std::vector<float>* b, QWidget* parent = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~AtlasViewer() override;
	void Init();
	void update();
	void update(QRect rect);

signals:
	void MousemovedSign(tissues_size_t t);

protected:
	void paintEvent(QPaintEvent* e) override;
	void mouseMoveEvent(QMouseEvent* e) override;

private:
	QPainter* m_Painter;
	float m_Scalefactor;
	float m_Scaleoffset;
	double m_Zoom;
	Pair m_Pixelsize;
	void ReloadBits();
	void GetSlice();
	QImage m_Image;
	unsigned char m_Orient;

	unsigned short m_Width, m_Height, m_Slicenr;
	unsigned short m_Dimx, m_Dimy, m_Dimz;
	float m_Dx, m_Dy, m_Dz;
	float m_Tissueopac;
	float* m_Bmpbits;
	tissues_size_t* m_Tissue;
	float* m_CurrentBmpbits;
	tissues_size_t* m_CurrentTissue;
	std::vector<float>* m_ColorR;
	std::vector<float>* m_ColorG;
	std::vector<float>* m_ColorB;

public slots:
	void SetScale(float offset1, float factor1);
	void SetScaleoffset(float offset1);
	void SetScalefactor(float factor1);
	void ZoomIn();
	void ZoomOut();
	void Unzoom();
	double ReturnZoom() const;
	void SetZoom(double z);
	void SlicenrChanged(unsigned short slicenr1);
	void OrientChanged(unsigned char orient1);
	void SetTissueopac(float tissueopac1);
	void PixelsizeChanged(Pair pixelsize1);
};

} // namespace iseg
