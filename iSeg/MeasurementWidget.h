/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef MEASUREWIDGET_12DEZ07
#define MEASUREWIDGET_12DEZ07

#include "SlicesHandler.h"
#include "bmp_read_1.h"

#include "Interface/WidgetInterface.h"

#include <QComboBox>
#include <QRadioButton>

class QStackedWidget;
class QLabel;

namespace iseg {

class MeasurementWidget : public WidgetInterface
{
	Q_OBJECT
public:
	MeasurementWidget(SlicesHandler* hand3D);
	~MeasurementWidget() override = default;
	FILE* SaveParams(FILE* fp, int version) override;
	FILE* LoadParams(FILE* fp, int version) override;
	void Init() override;
	void Cleanup() override;
	void NewLoaded() override;
	float Calculatevec(unsigned short orient);
	std::string GetName() override { return std::string("Measurement"); }
	QIcon GetIcon(QDir picdir) override { return QIcon(picdir.absoluteFilePath("measurement.png")); }

private:
	void OnMouseClicked(Point p) override;
	void OnMouseMoved(Point p) override;
	void MarksChanged() override;

	void Getlabels();
	float Calculate();
	void SetCoord(unsigned short Posit, Point p, unsigned short slicenr);

	enum eActiveLabels { kP1,
		kP2,
		kP3,
		kP4 };
	void SetActiveLabels(eActiveLabels labels);

	Bmphandler* m_Bmphand;
	SlicesHandler* m_Handler3D;
	std::vector<AugmentedMark> m_Labels;
	unsigned short m_Activeslice;

	QRadioButton* m_RbVector;
	QRadioButton* m_RbDist;
	QRadioButton* m_RbThick;
	QRadioButton* m_RbAngle;
	QRadioButton* m_Rb4ptangle;
	QRadioButton* m_RbVol;
	QWidget* m_InputArea;
	QRadioButton* m_RbPts;
	QRadioButton* m_RbLbls;

	QStackedWidget* m_StackedWidget;
	QLabel* m_TxtDisplayer;
	QWidget* m_LabelsArea;
	QComboBox* m_CbbLb1;
	QComboBox* m_CbbLb2;
	QComboBox* m_CbbLb3;
	QComboBox* m_CbbLb4;

	int m_State;
	int m_Pt[4][3];
	bool m_Drawing;
	std::vector<Point> m_Established;
	std::vector<Point> m_Dynamic;
	Point m_P1;

signals:
	void Vp1Changed(std::vector<Point>* vp1);
	void Vp1dynChanged(std::vector<Point>* vp1, std::vector<Point>* vpdyn, bool also_points = false);

private slots:
	void BmphandChanged(Bmphandler* bmph);
	void CbbChanged(int);
	void MethodChanged(int);
	void InputtypeChanged(int);
	void UpdateVisualization();
};

} // namespace iseg

#endif
