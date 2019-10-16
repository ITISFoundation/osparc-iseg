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

#include "Interface/WidgetInterface.h"

#include "ui_ThresholdWidgetQt4.h"



namespace iseg {

class SlicesHandler;
class bmphandler;

class ThresholdWidgetQt4 : public WidgetInterface
{
	Q_OBJECT
public:
	explicit ThresholdWidgetQt4(SlicesHandler *hand3D, QWidget *parent,
								const char* name, Qt::WindowFlags wFlags);
	virtual ~ThresholdWidgetQt4();

	virtual void init() override;
	virtual void newloaded() override;
	virtual void hideparams_changed() override;
	// should be const'ed
	virtual std::string GetName() override { return std::string("Threshold"); }
	virtual QIcon GetIcon(QDir picdir) override { return QIcon(picdir.absFilePath(QString("thresholding.png"))); }
	virtual FILE *SaveParams(FILE *fp, int version) override;
	virtual FILE *LoadParams(FILE *fp, int version) override;

private slots:
	// manual
	void on_mManualNrTissuesSpinBox_valueChanged(int newValue);
	void on_mManualLimitNrSpinBox_valueChanged(int newValue);
	void on_mLoadBordersPushButton_clicked();
	void on_mSaveBordersPushButton_clicked();
	void on_mThresholdHorizontalSlider_valueChanged(int newValue);
	void on_mThresholdBorderLineEdit_editingFinished();

	// manual 'density' threshold
	void on_mDensityIntensityThresholdLineEdit_editingFinished();
	void on_mDensityIntensityThresholdSlider_valueChanged(int newValue);
	void on_mDensityRadiusSpinBox_valueChanged(int newValue);
	void on_mDensityThresholdSpinBox_valueChanged(int newValue);

	// histo
	void on_mHistoPxSpinBox_valueChanged(int newValue);
	void on_mHistoLxSpinBox_valueChanged(int newValue);
	void on_mHistoPySpinBox_valueChanged(int newValue);
	void on_mHistoLySpinBox_valueChanged(int newValue);
	void on_mHistoMinPixelsSpinBox_valueChanged(int newValue);
	void on_mHistoMinPixelsRatioHorizontalSlider_valueChanged(int newValue);

	// kmeans
	void on_mKMeansNrTissuesSpinBox_valueChanged(int newValue);
	void on_mKMeansDimsSpinBox_valueChanged(int newValue);
	void on_mKMeansImageNrSpinBox_valueChanged(int newValue);
	void on_mKMeansWeightHorizontalSlider_valueChanged(int newValue);
	void on_mKMeansFilenameLineEdit_editingFinished();
	void on_mKMeansFilenamePushButton_clicked();
	void on_mKMeansRRadioButton_clicked();
	void on_mKMeansGRadioButton_clicked();
	void on_mKMeansBRadioButton_clicked();
	void on_mKMeansARadioButton_clicked();
	void on_mKMeansIterationsSpinBox_valueChanged(int newValue);
	void on_mKMeansConvergeSpinBox_valueChanged(int newValue);


	// general
	void on_mUseCenterFileCheckBox_toggled(bool newValue);
	void on_mCenterFilenameLineEdit_editingFinished();
	void on_mSelectCenterFilenamePushButton_clicked();

	void on_mAllSlicesCheckBox_toggled(bool newValue);
	void on_mExecutePushButton_clicked();

	void on_ModeChanged(QWidget* current_widget);

	// override
	void bmp_changed();

private:
	void on_tissuenr_changed(int i) override;
	void on_slicenr_changed() override;
	void initUi();
	void updateUi();
	void resetThresholds();
	void RGBA_changed();

	void doDensityThreshold(bool user_update = false);

	std::pair<float, float> get_range() const;

	SlicesHandler *handler3D = nullptr;
	float threshs[21]; // ugly
	float weights[20]; // ugly
	float *bits[20]; // ugly
	unsigned bits1[20]; // ugly
	std::vector<QString> filenames;
	Ui::ThresholdWidgetQt4 ui;

};

} // namespace iseg
