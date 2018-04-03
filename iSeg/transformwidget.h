/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef TRANSFORM_17NOV11
#define TRANSFORM_17NOV11

#include "SliceTransform.h"

#include "Core/DataSelection.h"
#include "Core/Point.h"

#include "Plugin/WidgetInterface.h"

#include <q3mimefactory.h>
#include <q3vbox.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qpixmap.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qsize.h>
#include <qslider.h>
#include <qspinbox.h>
#include <qwidget.h>

namespace iseg {

class TransformWidget : public WidgetInterface
{
	Q_OBJECT

public:
	TransformWidget(SlicesHandler* hand3D, QWidget* parent = 0,
					const char* name = 0, Qt::WindowFlags wFlags = 0);
	~TransformWidget();

	void init();
	void cleanup();
	void newloaded();
	void hideparams_changed();

	bool GetIsIdentityTransform();
	void GetDataSelection(bool& source, bool& target, bool& tissues);

	QSize sizeHint() const;
	std::string GetName() { return std::string("Transform"); };
	virtual QIcon GetIcon(QDir picdir)
	{
		return QIcon(picdir.absFilePath(QString("transform.png")).ascii());
	};

private:
	struct TransformParametersStruct
	{
		int translationOffset[2];
		double rotationAngle;
		double scalingFactor[2];
		double shearingAngle;
		int transformCenter[2];
	};

	void UpdatePreview();
	void SetCenter(int x, int y);
	void SetCenterDefault();
	void ResetWidgets();
	void BitsChanged();

private:
	// Image data
	SlicesHandler* handler3D;

	// Slice transform
	SliceTransform* sliceTransform;

	// Widgets
	Q3HBox* hBoxOverall;
	Q3VBox* vBoxTransforms;
	Q3VBox* vBoxParams;
	Q3HBox* hBoxSelectData;
	Q3HBox* hBoxSlider1;
	Q3HBox* hBoxSlider2;
	Q3HBox* hBoxFlip;
	Q3HBox* hBoxAxisSelection;
	Q3HBox* hBoxCenter;
	Q3HBox* hBoxExecute;

	QCheckBox* transformSourceCheckBox;
	QCheckBox* transformTargetCheckBox;
	QCheckBox* transformTissuesCheckBox;

	QCheckBox* allSlicesCheckBox;
	QPushButton* executePushButton;
	QPushButton* cancelPushButton;

	QRadioButton* translateRadioButton;
	QRadioButton* rotateRadioButton;
	QRadioButton* scaleRadioButton;
	QRadioButton* shearRadioButton;
	QRadioButton* flipRadioButton;
	//QRadioButton *matrixRadioButton;
	QButtonGroup* transformButtonGroup;

	QLabel* slider1Label;
	QLabel* slider2Label;
	QSlider* slider1;
	QSlider* slider2;
	QLineEdit* lineEdit1;
	QLineEdit* lineEdit2;

	QRadioButton* xAxisRadioButton;
	QRadioButton* yAxisRadioButton;
	QButtonGroup* axisButtonGroup;

	QPushButton* flipPushButton;

	QLabel* centerLabel;
	QLabel* centerCoordsLabel;
	QPushButton* centerSelectPushButton;

	// Transform parameters
	bool disableUpdatePreview;
	double updateParameter1;
	double updateParameter2;
	TransformParametersStruct transformParameters;

signals:
	void begin_datachange(iseg::DataSelection& dataSelection,
						  QWidget* sender = NULL, bool beginUndo = true);
	void end_datachange(QWidget* sender = NULL,
						iseg::EndUndoAction undoAction = iseg::EndUndo);

public slots:
	void slicenr_changed();
	void pt_clicked(Point p);
	void bmp_changed();
	void work_changed();
	void tissues_changed();
	void CancelPushButtonClicked();
	void ExecutePushButtonClicked();

private slots:
	void Slider1Changed(int value);
	void Slider2Changed(int value);
	void LineEdit1Edited();
	void LineEdit2Edited();
	void TransformChanged(int index);
	void SelectSourceChanged(int state);
	void SelectTargetChanged(int state);
	void SelectTissuesChanged(int state);
	void FlipPushButtonClicked();
};

} // namespace iseg

#endif
