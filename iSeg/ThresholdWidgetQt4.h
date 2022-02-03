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

#include "Interface/WidgetInterface.h"

#include "ui_ThresholdWidgetQt4.h"

namespace iseg {

class SlicesHandler;
class Bmphandler;

class ThresholdWidgetQt4 : public WidgetInterface
{
	Q_OBJECT
public:
	explicit ThresholdWidgetQt4(SlicesHandler* hand3D);
	~ThresholdWidgetQt4() override;

	void Init() override;
	void NewLoaded() override;
	void HideParamsChanged() override;
	// should be const'ed
	std::string GetName() override { return std::string("Threshold"); }
	QIcon GetIcon(QDir picdir) override { return QIcon(picdir.absoluteFilePath("thresholding.png")); }
	FILE* SaveParams(FILE* fp, int version) override;
	FILE* LoadParams(FILE* fp, int version) override;

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

	void on_ModeChanged(QWidget*);

	void BmpChanged() override;

private:
	void OnTissuenrChanged(int i) override;
	void OnSlicenrChanged() override;
	void initUi();
	void updateUi();
	void resetThresholds();
	void RGBA_changed();

	void doDensityThreshold(bool user_update = false);

	std::pair<float, float> get_range() const;

	SlicesHandler* m_Handler3D = nullptr;
	float m_Threshs[21];	// ugly
	float m_Weights[20];	// ugly
	float* m_Bits[20];		// ugly
	unsigned m_Bits1[20]; // ugly
	std::vector<QString> m_Filenames;
	Ui::ThresholdWidgetQt4 m_Ui;
};

} // namespace iseg
