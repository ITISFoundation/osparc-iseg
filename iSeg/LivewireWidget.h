/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef LWWIDGET_17March05
#define LWWIDGET_17March05

#include "SlicesHandler.h"
#include "bmp_read_1.h"

#include "Interface/WidgetInterface.h"

#include "Core/ImageForestingTransform.h"

#include <qcheckbox.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qspinbox.h>

class QFormLayout;

namespace iseg {

class LivewireWidget : public WidgetInterface
{
	Q_OBJECT
public:
	LivewireWidget(SlicesHandler* hand3D, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~LivewireWidget() override;
	void Init() override;
	void NewLoaded() override;
	void Cleanup() override;
	FILE* SaveParams(FILE* fp, int version) override;
	FILE* LoadParams(FILE* fp, int version) override;
	void HideParamsChanged() override;
	std::string GetName() override { return std::string("Contour"); }
	QIcon GetIcon(QDir picdir) override { return QIcon(picdir.absFilePath(QString("contour.png"))); }

private:
	void OnSlicenrChanged() override;

	void OnMouseClicked(Point p) override;
	void OnMouseMoved(Point p) override;
	void OnMouseReleased(Point p) override;
	void BmpChanged() override;

	void Init1();

	ImageForestingTransformLivewire* m_Lw;
	ImageForestingTransformLivewire* m_Lwfirst;

	//	bool isactive;
	bool m_Drawing;
	std::vector<Point> m_Clicks;
	std::vector<Point> m_Established;
	std::vector<Point> m_Dynamic;
	std::vector<Point> m_Dynamic1;
	std::vector<Point> m_Dynamicold;
	std::vector<QTime> m_Times;
	int m_Tlimit1;
	int m_Tlimit2;
	bool m_Cooling;
	Bmphandler* m_Bmphand;
	SlicesHandler* m_Handler3D;
	unsigned short m_Activeslice;

	QRadioButton* m_Autotrace;
	QRadioButton* m_Straight;
	QRadioButton* m_Freedraw;

	QFormLayout* m_ParamsLayout;
	QCheckBox* m_CbFreezing;
	QCheckBox* m_CbClosing;
	QSpinBox* m_SbFreezing;
	QPushButton* m_Pushexec;

	bool m_Straightmode;
	Point m_P1, m_P2;
	std::vector<int> m_Establishedlengths;

signals:
	void Vp1Changed(std::vector<Point>* vp1);
	void Vp1dynChanged(std::vector<Point>* vp1, std::vector<Point>* vpdyn);

private slots:
	void BmphandChanged(Bmphandler* bmph);
	void PtDoubleclicked(Point p);
	void PtMidclicked(Point p);
	void PtDoubleclickedmid(Point p);
	void ModeChanged();
	void FreezingChanged();
	void SbfreezingChanged(int i);
};

} // namespace iseg

#endif
