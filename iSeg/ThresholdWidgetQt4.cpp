/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */

#include "ThresholdWidgetQt4.h"

#include "SlicesHandler.h"
#include "bmp_read_1.h"

#include "Data/ItkProgressObserver.h"
#include "Data/ItkUtils.h"
#include "Data/SlicesHandlerITKInterface.h"

#include <itkBinaryThresholdImageFilter.h>
#include <itkMeanImageFilter.h>
#include <itkSliceBySliceImageFilter.h>

#include <QDoubleValidator>
#include <QFileDialog>
#include <QSignalMapper>

namespace iseg {

ThresholdWidgetQt4::ThresholdWidgetQt4(SlicesHandler* hand3D, QWidget* parent, const char* name, Qt::WindowFlags wFlags)
		: WidgetInterface(parent, name, wFlags), m_Handler3D(hand3D)
{
	initUi();
	updateUi();
}

ThresholdWidgetQt4::~ThresholdWidgetQt4()
= default;

void ThresholdWidgetQt4::initUi()
{
	m_Ui.setupUi(this);

	m_Threshs[0] = float(m_Ui.mManualNrTissuesSpinBox->value() - 1);
	for (unsigned i = 0; i < 20; ++i)
	{
		m_Bits1[i] = 0;
		m_Threshs[i + 1] = 255;
		m_Weights[i] = 1.0f;
	}

	auto mode_signal_mapping = new QSignalMapper(this);
	mode_signal_mapping->setMapping(m_Ui.mManualModeRadioButton, m_Ui.mManualWidget);
	mode_signal_mapping->setMapping(m_Ui.mDensityModeRadioButton, m_Ui.mDensityThresholdWidget);
	mode_signal_mapping->setMapping(m_Ui.mHistoModeRadioButton, m_Ui.mHistoWidget);
	mode_signal_mapping->setMapping(m_Ui.mKMeansRadioButton, m_Ui.mKmeansEMWidget);
	mode_signal_mapping->setMapping(m_Ui.mEMModeRadioButton, m_Ui.mKmeansEMWidget);
	QObject_connect(m_Ui.mManualModeRadioButton, SIGNAL(clicked()), mode_signal_mapping, SLOT(map()));
	QObject_connect(m_Ui.mDensityModeRadioButton, SIGNAL(clicked()), mode_signal_mapping, SLOT(map()));
	QObject_connect(m_Ui.mHistoModeRadioButton, SIGNAL(clicked()), mode_signal_mapping, SLOT(map()));
	QObject_connect(m_Ui.mKMeansRadioButton, SIGNAL(clicked()), mode_signal_mapping, SLOT(map()));
	QObject_connect(m_Ui.mEMModeRadioButton, SIGNAL(clicked()), mode_signal_mapping, SLOT(map()));
	QObject_connect(mode_signal_mapping, SIGNAL(mapped(QWidget*)), this, SLOT(on_ModeChanged(QWidget*)));

	// init with manual mode
	m_Ui.mManualModeRadioButton->setChecked(true);
	on_ModeChanged(m_Ui.mManualWidget);
}

void ThresholdWidgetQt4::on_ModeChanged(QWidget* current_widget)
{
	bool enable_init_centers = (current_widget == m_Ui.mKmeansEMWidget);

	m_Ui.mUseCenterFileCheckBox->setEnabled(enable_init_centers);
	m_Ui.mCenterFilenameLineEdit->setEnabled(enable_init_centers && m_Ui.mUseCenterFileCheckBox->isChecked());
	m_Ui.mSelectCenterFilenamePushButton->setEnabled(enable_init_centers && m_Ui.mUseCenterFileCheckBox->isChecked());

	m_Ui.mModeStackedWidget->setCurrentWidget(current_widget);
}

void ThresholdWidgetQt4::updateUi()
{
	auto range = get_range();
	// manual
	m_Ui.mLoadBordersPushButton->setDisabled(hideparams);
	m_Ui.mSaveBordersPushButton->setDisabled(hideparams);
	m_Ui.mThresholdLowerLabel->setText(QString::number(range.first, 'g', 3));
	m_Ui.mThresholdUpperLabel->setText(QString::number(range.second, 'g', 3));
	m_Ui.mDensityIntensityMin->setText(QString::number(range.first, 'g', 3));
	m_Ui.mDensityIntensityMax->setText(QString::number(range.second, 'g', 3));
	if (range.second != range.first)
	{
		const int slider_value = static_cast<int>((m_Threshs[m_Ui.mManualLimitNrSpinBox->value()] - range.first) * 200 / (range.second - range.first) + .5f);
		auto slider_block = m_Ui.mThresholdHorizontalSlider->blockSignals(true);
		m_Ui.mThresholdHorizontalSlider->setValue(slider_value);
		m_Ui.mThresholdHorizontalSlider->blockSignals(slider_block);

		auto lineedit_block = m_Ui.mThresholdBorderLineEdit->blockSignals(true);
		m_Ui.mThresholdBorderLineEdit->setText(QString::number(m_Threshs[m_Ui.mManualLimitNrSpinBox->value()], 'g', 3));
		m_Ui.mThresholdBorderLineEdit->blockSignals(lineedit_block);
	}

	if (m_Ui.mDensityIntensityThresholdLineEdit->text().isEmpty())
	{
		auto lineedit_block = m_Ui.mDensityIntensityThresholdLineEdit->blockSignals(true);
		m_Ui.mDensityIntensityThresholdLineEdit->setText(QString::number(range.first));
		m_Ui.mDensityIntensityThresholdLineEdit->blockSignals(lineedit_block);
	}

	//ui.mWeightBorderLineEdit->setText(QString::number(threshs[sb_tissuenr->value()], 'g', 3));
	m_Ui.mManualLimitNrLabel->setEnabled(m_Ui.mManualNrTissuesSpinBox->value() != 2);
	m_Ui.mManualLimitNrSpinBox->setEnabled(m_Ui.mManualNrTissuesSpinBox->value() != 2);

	// histo
	m_Ui.mSubsectionGroupBox->setDisabled(hideparams);
	m_Ui.mMinPixelsFrame->setDisabled(hideparams);

	// kmeans
	m_Ui.mKMeansDimsLabel->setDisabled(hideparams);
	m_Ui.mKMeansDimsSpinBox->setDisabled(hideparams);
	m_Ui.mKMeansIterationsFrame->setDisabled(hideparams);
	m_Ui.mKMeansImageNrLabel->setEnabled(m_Ui.mKMeansDimsSpinBox->value() != 1);
	m_Ui.mKMeansImageNrSpinBox->setEnabled(m_Ui.mKMeansDimsSpinBox->value() != 1);
	m_Ui.mKMeansWeightLabel->setEnabled(m_Ui.mKMeansDimsSpinBox->value() != 1);
	m_Ui.mKMeansWeightHorizontalSlider->setEnabled(m_Ui.mKMeansDimsSpinBox->value() != 1);
	m_Ui.mKMeansWeightHorizontalSlider->setValue(int(200 * m_Weights[m_Ui.mKMeansImageNrSpinBox->value() - 1]));
	m_Ui.mKMeansFilenameFrame->setEnabled(m_Ui.mKMeansImageNrSpinBox->value() > 1);
	m_Ui.mKMeansFilenameLineEdit->setText(m_Ui.mKMeansImageNrSpinBox->value() > 1 ? m_Filenames[m_Ui.mKMeansImageNrSpinBox->value() - 2] : "");

	// general
	m_Ui.mCenterFilenameLineEdit->setEnabled(m_Ui.mUseCenterFileCheckBox->isChecked());
	m_Ui.mSelectCenterFilenamePushButton->setEnabled(m_Ui.mUseCenterFileCheckBox->isChecked());
}

std::pair<float, float> ThresholdWidgetQt4::get_range() const
{
	std::pair<float, float> range(0.0f, 0.0f);
	if (m_Handler3D->GetActivebmphandler())
	{
		Pair p;
		m_Handler3D->GetActivebmphandler()->GetBmprange(&p);
		range.first = p.low;
		range.second = p.high;
	}
	return range;
}

void ThresholdWidgetQt4::Init()
{
	auto range = get_range();
	auto range_validator = new QDoubleValidator(range.first, range.second, 1000, m_Ui.mThresholdBorderLineEdit);
	m_Ui.mThresholdBorderLineEdit->setValidator(range_validator);

	// same for density Threshold widget
	m_Ui.mDensityIntensityThresholdLineEdit->setValidator(range_validator);

	resetThresholds();

	updateUi();
}

void ThresholdWidgetQt4::NewLoaded()
{
	Init();
}

void ThresholdWidgetQt4::HideParamsChanged()
{
	Init();
}

FILE* ThresholdWidgetQt4::SaveParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		int dummy;
		auto slider_block = m_Ui.mThresholdHorizontalSlider->blockSignals(true);
		dummy = m_Ui.mThresholdHorizontalSlider->value();
		m_Ui.mThresholdHorizontalSlider->blockSignals(slider_block);
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = m_Ui.mHistoMinPixelsRatioHorizontalSlider->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = m_Ui.mManualNrTissuesSpinBox->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = m_Ui.mKMeansDimsSpinBox->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = m_Ui.mManualLimitNrSpinBox->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = m_Ui.mHistoPxSpinBox->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = m_Ui.mHistoPySpinBox->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = m_Ui.mHistoLxSpinBox->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = m_Ui.mHistoLySpinBox->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = m_Ui.mKMeansIterationsSpinBox->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = m_Ui.mKMeansConvergeSpinBox->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = m_Ui.mHistoMinPixelsSpinBox->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_Ui.mSubsectionGroupBox->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_Ui.mManualModeRadioButton->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_Ui.mHistoModeRadioButton->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_Ui.mKMeansRadioButton->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_Ui.mEMModeRadioButton->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_Ui.mAllSlicesCheckBox->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);

		auto range = get_range();
		fwrite(&range.second, 1, sizeof(float), fp);
		fwrite(&range.first, 1, sizeof(float), fp);
		fwrite(m_Threshs, 21, sizeof(float), fp);
		fwrite(m_Weights, 20, sizeof(float), fp);
	}

	return fp;
}

FILE* ThresholdWidgetQt4::LoadParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		int dummy;
		fread(&dummy, sizeof(int), 1, fp);
		auto slider_block = m_Ui.mThresholdHorizontalSlider->blockSignals(true);
		m_Ui.mThresholdHorizontalSlider->setValue(dummy);
		m_Ui.mThresholdHorizontalSlider->blockSignals(slider_block);
		fread(&dummy, sizeof(int), 1, fp);
		m_Ui.mHistoMinPixelsRatioHorizontalSlider->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		m_Ui.mManualNrTissuesSpinBox->setValue(dummy);
		m_Ui.mManualLimitNrSpinBox->setMaxValue(dummy - 1);
		fread(&dummy, sizeof(int), 1, fp);
		m_Ui.mKMeansDimsSpinBox->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		m_Ui.mManualLimitNrSpinBox->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		m_Ui.mHistoPxSpinBox->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		m_Ui.mHistoPySpinBox->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		m_Ui.mHistoLxSpinBox->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		m_Ui.mHistoLySpinBox->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		m_Ui.mKMeansIterationsSpinBox->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		m_Ui.mKMeansConvergeSpinBox->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		m_Ui.mHistoMinPixelsSpinBox->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		m_Ui.mSubsectionGroupBox->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		m_Ui.mManualModeRadioButton->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		m_Ui.mHistoModeRadioButton->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		m_Ui.mKMeansRadioButton->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		m_Ui.mEMModeRadioButton->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		m_Ui.mAllSlicesCheckBox->setChecked(dummy > 0);

		float upper = 0, lower = 0;
		fread(&upper, sizeof(float), 1, fp);
		fread(&lower, sizeof(float), 1, fp);
		fread(m_Threshs, sizeof(float), 21, fp);
		fread(m_Weights, sizeof(float), 20, fp);

		dummy = m_Ui.mManualNrTissuesSpinBox->value();
		OnTissuenrChanged(dummy);

		if (m_Ui.mManualModeRadioButton->isChecked())
			m_Ui.mModeStackedWidget->setCurrentWidget(m_Ui.mManualWidget);
		else if (m_Ui.mHistoModeRadioButton->isChecked())
			m_Ui.mModeStackedWidget->setCurrentWidget(m_Ui.mHistoWidget);
		else
			m_Ui.mModeStackedWidget->setCurrentWidget(m_Ui.mKmeansEMWidget);
	}
	return fp;
}

void ThresholdWidgetQt4::on_mManualNrTissuesSpinBox_valueChanged(int newValue)
{
	m_Threshs[0] = float(newValue - 1);
	m_Ui.mManualLimitNrSpinBox->setMaxValue(newValue - 1);

	updateUi();
}

void ThresholdWidgetQt4::on_mManualLimitNrSpinBox_valueChanged(int newValue)
{
	updateUi();
}

void ThresholdWidgetQt4::on_mLoadBordersPushButton_clicked()
{
	auto loadfilename = QFileDialog::getOpenFileName(this, "Select Boarder file", QString::null, "Boarders (*.txt)\n"
																																															 "All (*.*)");

	if (loadfilename.isEmpty())
	{
		return;
	}

	std::vector<float> fvec;
	// hope it's only american english... ;)
	FILE* fp = fopen(loadfilename.ascii(), "r");
	float f;
	if (fp != nullptr)
	{
		while (fscanf(fp, "%f", &f) == 1)
		{
			fvec.push_back(f);
		}
	}
	fclose(fp);

	if (!fvec.empty() && fvec.size() < 20)
	{
		m_Ui.mManualNrTissuesSpinBox->setValue(static_cast<int>(fvec.size() + 1));
		m_Threshs[0] = float(fvec.size());
		m_Ui.mManualLimitNrSpinBox->setMaxValue(static_cast<int>(fvec.size()));

		auto range = get_range();
		for (unsigned i = 0; i < (unsigned)fvec.size(); i++)
		{
			if (fvec[i] > range.second)
				f = range.second;
			else if (fvec[i] < range.first)
				f = range.first;
			else
				f = fvec[i];
			m_Threshs[i + 1] = f;
		}
		m_Ui.mManualLimitNrSpinBox->setValue(1);
	}

	updateUi();
}

void ThresholdWidgetQt4::on_mSaveBordersPushButton_clicked()
{
	auto savefilename = QFileDialog::getSaveFileName(this, "Save file", QString::null, "Boarders (*.txt)\n");

	if (savefilename.length() > 4 && !savefilename.endsWith(QString(".txt")))
		savefilename.append(".txt");

	if (!savefilename.isEmpty())
	{
		FILE* fp = fopen(savefilename.ascii(), "w");
		for (int i = 1; i <= m_Ui.mManualLimitNrSpinBox->maxValue(); i++)
			fprintf(fp, "%f\n", m_Threshs[i]);
		fclose(fp);
	}
}

void ThresholdWidgetQt4::on_mThresholdHorizontalSlider_valueChanged(int newValue)
{
	auto range = get_range();
	m_Threshs[m_Ui.mManualLimitNrSpinBox->value()] =
			newValue * 0.005f * (range.second - range.first) + range.first;

	if (m_Ui.mAllSlicesCheckBox->isChecked())
		m_Handler3D->Threshold(m_Threshs);
	else
		m_Handler3D->GetActivebmphandler()->Threshold(m_Threshs);

	emit EndDatachange(this, iseg::NoUndo);

	updateUi();
}

void ThresholdWidgetQt4::on_mThresholdBorderLineEdit_editingFinished()
{
	bool b1;
	float val = m_Ui.mThresholdBorderLineEdit->text().toFloat(&b1);
	if (b1)
	{
		m_Threshs[m_Ui.mManualLimitNrSpinBox->value()] = val;
		{
			DataSelection data_selection;
			data_selection.allSlices = m_Ui.mAllSlicesCheckBox->isChecked();
			data_selection.sliceNr = m_Handler3D->ActiveSlice();
			data_selection.work = true;
			emit BeginDatachange(data_selection, this);

			if (m_Ui.mAllSlicesCheckBox->isChecked())
				m_Handler3D->Threshold(m_Threshs);
			else
				m_Handler3D->GetActivebmphandler()->Threshold(m_Threshs);

			emit EndDatachange(this);
		}
	}

	updateUi();
}

void ThresholdWidgetQt4::on_mDensityIntensityThresholdLineEdit_editingFinished()
{
	bool b1;
	float val = m_Ui.mDensityIntensityThresholdLineEdit->text().toFloat(&b1);
	if (b1)
	{
		auto range = get_range();
		int slider_value = static_cast<int>(100.f * (val - range.first) / (range.second - range.first));

		auto slider_block = m_Ui.mDensityIntensityThresholdSlider->blockSignals(true);
		m_Ui.mDensityIntensityThresholdSlider->setValue(slider_value);
		m_Ui.mDensityIntensityThresholdSlider->blockSignals(slider_block);

		doDensityThreshold();
	}
}

void ThresholdWidgetQt4::on_mDensityIntensityThresholdSlider_valueChanged(int newValue)
{
	auto range = get_range();
	int slider_value = m_Ui.mDensityIntensityThresholdSlider->value();
	float val = (range.second - range.first) * slider_value / 100.f + range.first;

	auto slider_block = m_Ui.mDensityIntensityThresholdLineEdit->blockSignals(true);
	m_Ui.mDensityIntensityThresholdLineEdit->setText(QString::number(val));
	m_Ui.mDensityIntensityThresholdLineEdit->blockSignals(slider_block);

	doDensityThreshold();
}

void ThresholdWidgetQt4::on_mDensityRadiusSpinBox_valueChanged(int newValue)
{
	doDensityThreshold();
}

void ThresholdWidgetQt4::on_mDensityThresholdSpinBox_valueChanged(int newValue)
{
	doDensityThreshold();
}

void ThresholdWidgetQt4::doDensityThreshold(bool user_update)
{
	DataSelection data_selection;
	data_selection.allSlices = m_Ui.mAllSlicesCheckBox->isChecked();
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.work = true;
	emit BeginDatachange(data_selection, this);

	using slice_image_type = itk::Image<float, 2>;
	using threshold_filter_type = itk::BinaryThresholdImageFilter<slice_image_type, slice_image_type>;
	using mean_filter_type = itk::MeanImageFilter<slice_image_type, slice_image_type>;

	float val = 0.f;
	val = m_Ui.mDensityIntensityThresholdLineEdit->text().toFloat();

	auto Threshold = threshold_filter_type::New();
	Threshold->SetLowerThreshold(val); // intensity Threshold
	Threshold->SetInsideValue(1);
	Threshold->SetOutsideValue(0);

	slice_image_type::SizeType radius;
	radius[0] = m_Ui.mDensityRadiusSpinBox->value(); // radius along x
	radius[1] = m_Ui.mDensityRadiusSpinBox->value(); // radius along y

	auto mean = mean_filter_type::New();
	mean->SetInput(Threshold->GetOutput());
	mean->SetRadius(radius);

	auto threshold2 = threshold_filter_type::New();
	threshold2->SetInput(mean->GetOutput());
	threshold2->SetLowerThreshold(m_Ui.mDensityThresholdSpinBox->value() / 100.f); // % above Threshold
	threshold2->SetInsideValue(255);
	threshold2->SetOutsideValue(0);

	SlicesHandlerITKInterface wrapper(m_Handler3D);
	if (!m_Ui.mAllSlicesCheckBox->isChecked())
	{
		auto source = wrapper.GetSourceSlice();
		auto target = wrapper.GetTargetSlice();

		Threshold->SetInput(source);

		threshold2->Update();
		iseg::Paste<slice_image_type, slice_image_type>(threshold2->GetOutput(), target);

		m_Handler3D->GetActivebmphandler()->SetMode(eScaleMode::kFixedRange, true);
	}
	else if (user_update) // only update 3D when explicitly requested via Execute button
	{
		using input_image_type = itk::SliceContiguousImage<float>;
		using image_type = itk::Image<float, 3>;
		using slice_filter_type = itk::SliceBySliceImageFilter<input_image_type, image_type, threshold_filter_type, threshold_filter_type>;

		auto source = wrapper.GetSource(true);
		auto target = wrapper.GetTarget(true);

		auto slice_executor = slice_filter_type::New();
		slice_executor->SetInput(source);
		slice_executor->SetInputFilter(Threshold);
		slice_executor->SetOutputFilter(threshold2);

		slice_executor->Update();
		iseg::Paste<image_type, input_image_type>(slice_executor->GetOutput(), target);

		m_Handler3D->SetModeall(eScaleMode::kFixedRange, true);
	}
	emit EndDatachange(this);
}

void ThresholdWidgetQt4::on_mHistoPxSpinBox_valueChanged(int newValue)
{
	// nothing done
}

void ThresholdWidgetQt4::on_mHistoLxSpinBox_valueChanged(int newValue)
{
	// nothing done
}

void ThresholdWidgetQt4::on_mHistoPySpinBox_valueChanged(int newValue)
{
	// nothing done
}

void ThresholdWidgetQt4::on_mHistoLySpinBox_valueChanged(int newValue)
{
	// nothing done
}

void ThresholdWidgetQt4::on_mHistoMinPixelsSpinBox_valueChanged(int newValue)
{
	// nothing done
}

void ThresholdWidgetQt4::on_mHistoMinPixelsRatioHorizontalSlider_valueChanged(int newValue)
{
	// nothing done
}

void ThresholdWidgetQt4::on_mKMeansNrTissuesSpinBox_valueChanged(int newValue)
{
}

void ThresholdWidgetQt4::on_mKMeansDimsSpinBox_valueChanged(int newValue)
{
	const size_t cursize = m_Filenames.size();
	if (newValue > cursize + 1)
	{
		m_Filenames.resize(newValue - 1);
		for (size_t i = cursize; i + 1 < newValue; ++i)
			m_Filenames[i] = QString("");
	}

	m_Ui.mKMeansImageNrSpinBox->setMaxValue(newValue);
	m_Ui.mKMeansImageNrSpinBox->setValue(1);

	updateUi();
}

void ThresholdWidgetQt4::on_mKMeansImageNrSpinBox_valueChanged(int newValue)
{
	updateUi();
}

void ThresholdWidgetQt4::on_mKMeansWeightHorizontalSlider_valueChanged(int newValue)
{
	m_Weights[m_Ui.mKMeansImageNrSpinBox->value() - 1] = newValue * 0.005f;
}

void ThresholdWidgetQt4::on_mKMeansFilenameLineEdit_editingFinished()
{
}

void ThresholdWidgetQt4::on_mKMeansFilenamePushButton_clicked()
{
	QString loadfilename = QFileDialog::getOpenFileName(this, "Select an image",
			QString(),
			"Images (*.png)\n"
			"Images (*.mhd)\n"
			"All (*.*)"
	);
	m_Ui.mKMeansFilenameLineEdit->setText(loadfilename);
	m_Filenames[m_Ui.mKMeansImageNrSpinBox->value() - 2] = loadfilename;

	m_Ui.mKMeansRRadioButton->setEnabled(false);
	m_Ui.mKMeansGRadioButton->setEnabled(false);
	m_Ui.mKMeansBRadioButton->setEnabled(false);
	m_Ui.mKMeansARadioButton->setEnabled(false);

	QFileInfo fi(loadfilename);
	QString ext = fi.extension();
	if (ext == "png")
	{
		QImage image(loadfilename);

		if (image.depth() > 8)
		{
			m_Ui.mKMeansRRadioButton->setEnabled(true);
			m_Ui.mKMeansGRadioButton->setEnabled(true);
			m_Ui.mKMeansBRadioButton->setEnabled(true);
			if (image.hasAlphaChannel())
				m_Ui.mKMeansARadioButton->setEnabled(true);
		}
	}
}

void ThresholdWidgetQt4::on_mKMeansRRadioButton_clicked()
{
	RGBA_changed();
}

void ThresholdWidgetQt4::on_mKMeansGRadioButton_clicked()
{
	RGBA_changed();
}

void ThresholdWidgetQt4::on_mKMeansBRadioButton_clicked()
{
	RGBA_changed();
}

void ThresholdWidgetQt4::on_mKMeansARadioButton_clicked()
{
	RGBA_changed();
}

void ThresholdWidgetQt4::on_mKMeansIterationsSpinBox_valueChanged(int newValue)
{
}

void ThresholdWidgetQt4::on_mKMeansConvergeSpinBox_valueChanged(int newValue)
{
}

void ThresholdWidgetQt4::on_mUseCenterFileCheckBox_toggled(bool newValue)
{
	if (!newValue)
	{
		m_Ui.mCenterFilenameLineEdit->setText("");
	}
	updateUi();
}

void ThresholdWidgetQt4::on_mCenterFilenameLineEdit_editingFinished()
{
	// never happens
}

void ThresholdWidgetQt4::on_mSelectCenterFilenamePushButton_clicked()
{
	auto center_file_name = QFileDialog::getOpenFileName(nullptr, "Select center file", QString(), "Text File (*.txt*)");
	m_Ui.mCenterFilenameLineEdit->setText(center_file_name);
}

void ThresholdWidgetQt4::on_mAllSlicesCheckBox_toggled(bool newValue)
{
	// does nothing...
}

void ThresholdWidgetQt4::on_mExecutePushButton_clicked()
{
	unsigned char modedummy;

	DataSelection data_selection;
	data_selection.allSlices = m_Ui.mAllSlicesCheckBox->isChecked();
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.work = true;
	emit BeginDatachange(data_selection, this);

	if (m_Ui.mManualModeRadioButton->isChecked())
	{
		if (m_Ui.mAllSlicesCheckBox->isChecked())
			m_Handler3D->Threshold(m_Threshs);
		else
			m_Handler3D->GetActivebmphandler()->Threshold(m_Threshs);
	}
	else if (m_Ui.mDensityModeRadioButton->isChecked())
	{
		doDensityThreshold(true);
	}
	else if (m_Ui.mHistoModeRadioButton->isChecked())
	{
		m_Handler3D->GetActivebmphandler()->SwapBmpwork();

		if (m_Ui.mSubsectionGroupBox->isChecked())
		{
			Point p;
			p.px = (unsigned short)m_Ui.mHistoPxSpinBox->value();
			p.py = (unsigned short)m_Ui.mHistoPySpinBox->value();
			m_Handler3D->GetActivebmphandler()->MakeHistogram(p, m_Ui.mHistoLxSpinBox->value(), m_Ui.mHistoLySpinBox->value(), true);
		}
		else
			m_Handler3D->GetActivebmphandler()->MakeHistogram(true);

		m_Handler3D->GetActivebmphandler()->GaussianHist(1.0f);
		m_Handler3D->GetActivebmphandler()->SwapBmpwork();

		float* thresh1 = m_Handler3D->GetActivebmphandler()->FindModal((unsigned)m_Ui.mHistoMinPixelsSpinBox->value(), 0.005f * m_Ui.mHistoMinPixelsRatioHorizontalSlider->value());
		if (m_Ui.mAllSlicesCheckBox->isChecked())
			m_Handler3D->Threshold(thresh1);
		else
			m_Handler3D->GetActivebmphandler()->Threshold(thresh1);
		free(thresh1);
	}
	else if (m_Ui.mKMeansRadioButton->isChecked())
	{
		FILE* fp = fopen("C:\\gamma.txt", "r"); // really????
		if (fp != nullptr)
		{
			float** centers = new float*[m_Ui.mKMeansNrTissuesSpinBox->value()];
			for (int i = 0; i < m_Ui.mKMeansNrTissuesSpinBox->value(); i++)
				centers[i] = new float[m_Ui.mKMeansDimsSpinBox->value()];
			float* tol_f = new float[m_Ui.mKMeansDimsSpinBox->value()];
			float* tol_d = new float[m_Ui.mKMeansDimsSpinBox->value()];
			for (int i = 0; i < m_Ui.mKMeansNrTissuesSpinBox->value(); i++)
			{
				for (int j = 0; j < m_Ui.mKMeansDimsSpinBox->value(); j++)
					fscanf(fp, "%f", &(centers[i][j]));
			}
			for (int j = 0; j < m_Ui.mKMeansDimsSpinBox->value(); j++)
				fscanf(fp, "%f", &(tol_f[j]));
			for (int j = 1; j < m_Ui.mKMeansDimsSpinBox->value(); j++)
				fscanf(fp, "%f", &(tol_d[j]));
			tol_d[0] = 0.0f;
			fclose(fp);
			std::vector<std::string> mhdfiles;
			for (int i = 0; i + 1 < m_Ui.mKMeansDimsSpinBox->value(); i++)
				mhdfiles.push_back(std::string(m_Filenames[i].ascii()));
			if (m_Ui.mAllSlicesCheckBox->isChecked())
				m_Handler3D->GammaMhd(m_Handler3D->ActiveSlice(), (short)m_Ui.mKMeansNrTissuesSpinBox->value(), (short)m_Ui.mKMeansDimsSpinBox->value(), mhdfiles, m_Weights, centers, tol_f, tol_d);
			else
				m_Handler3D->GetActivebmphandler()->GammaMhd((short)m_Ui.mKMeansNrTissuesSpinBox->value(), (short)m_Ui.mKMeansDimsSpinBox->value(), mhdfiles, m_Handler3D->ActiveSlice(), m_Weights, centers, tol_f, tol_d, m_Handler3D->GetPixelsize());
			delete[] tol_d;
			delete[] tol_f;
			for (int i = 0; i < m_Ui.mKMeansNrTissuesSpinBox->value(); i++)
				delete[] centers[i];
			delete[] centers;
		}
		else
		{
			std::vector<std::string> kmeansfiles;
			for (int i = 0; i + 1 < m_Ui.mKMeansDimsSpinBox->value(); i++)
				kmeansfiles.push_back(std::string(m_Filenames[i].ascii()));
			if (!kmeansfiles.empty())
			{
				if (kmeansfiles[0].substr(kmeansfiles[0].find_last_of(".") +
																	1) == "png")
				{
					std::vector<int> extract_channels;
					if (m_Ui.mKMeansRRadioButton->isChecked())
						extract_channels.push_back(0);
					if (m_Ui.mKMeansGRadioButton->isChecked())
						extract_channels.push_back(1);
					if (m_Ui.mKMeansBRadioButton->isChecked())
						extract_channels.push_back(2);
					if (m_Ui.mKMeansARadioButton->isChecked())
						extract_channels.push_back(3);
					if (m_Ui.mKMeansDimsSpinBox->value() != extract_channels.size() + 1)
						return;
					if (m_Ui.mAllSlicesCheckBox->isChecked())
						m_Handler3D->KmeansPng(m_Handler3D->ActiveSlice(), (short)m_Ui.mKMeansNrTissuesSpinBox->value(), (short)m_Ui.mKMeansDimsSpinBox->value(), kmeansfiles, extract_channels, m_Weights, (unsigned int)m_Ui.mKMeansIterationsSpinBox->value(), (unsigned int)m_Ui.mKMeansConvergeSpinBox->value(), m_Ui.mCenterFilenameLineEdit->text().toStdString());
					else
						m_Handler3D->GetActivebmphandler()->KmeansPng((short)m_Ui.mKMeansNrTissuesSpinBox->value(), (short)m_Ui.mKMeansDimsSpinBox->value(), kmeansfiles, extract_channels, m_Handler3D->ActiveSlice(), m_Weights, (unsigned int)m_Ui.mKMeansIterationsSpinBox->value(), (unsigned int)m_Ui.mKMeansConvergeSpinBox->value(), m_Ui.mCenterFilenameLineEdit->text().toStdString());
				}
				else
				{
					if (m_Ui.mAllSlicesCheckBox->isChecked())
						m_Handler3D->KmeansMhd(m_Handler3D->ActiveSlice(), (short)m_Ui.mKMeansNrTissuesSpinBox->value(), (short)m_Ui.mKMeansDimsSpinBox->value(), kmeansfiles, m_Weights, (unsigned int)m_Ui.mKMeansIterationsSpinBox->value(), (unsigned int)m_Ui.mKMeansConvergeSpinBox->value());
					else
						m_Handler3D->GetActivebmphandler()->KmeansMhd((short)m_Ui.mKMeansNrTissuesSpinBox->value(), (short)m_Ui.mKMeansDimsSpinBox->value(), kmeansfiles, m_Handler3D->ActiveSlice(), m_Weights, (unsigned int)m_Ui.mKMeansIterationsSpinBox->value(), (unsigned int)m_Ui.mKMeansConvergeSpinBox->value());
				}
			}
			else
			{
				if (m_Ui.mAllSlicesCheckBox->isChecked())
					m_Handler3D->KmeansMhd(m_Handler3D->ActiveSlice(), (short)m_Ui.mKMeansNrTissuesSpinBox->value(), (short)m_Ui.mKMeansDimsSpinBox->value(), kmeansfiles, m_Weights, (unsigned int)m_Ui.mKMeansIterationsSpinBox->value(), (unsigned int)m_Ui.mKMeansConvergeSpinBox->value());
				else
					m_Handler3D->GetActivebmphandler()->KmeansMhd((short)m_Ui.mKMeansNrTissuesSpinBox->value(), (short)m_Ui.mKMeansDimsSpinBox->value(), kmeansfiles, m_Handler3D->ActiveSlice(), m_Weights, (unsigned int)m_Ui.mKMeansIterationsSpinBox->value(), (unsigned int)m_Ui.mKMeansConvergeSpinBox->value());
			}
		}
	}
	else
	{
		//		EM em;
		for (int i = 0; i < m_Ui.mKMeansDimsSpinBox->value(); i++)
		{
			if (m_Bits1[i] == 0)
				m_Bits[i] = m_Handler3D->GetActivebmphandler()->ReturnBmp();
			else
				m_Bits[i] = m_Handler3D->GetActivebmphandler()->Getstack(m_Bits1[i], modedummy);
		}

		if (m_Ui.mAllSlicesCheckBox->isChecked())
			m_Handler3D->Em(m_Handler3D->ActiveSlice(), (short)m_Ui.mKMeansNrTissuesSpinBox->value(), (unsigned int)m_Ui.mKMeansIterationsSpinBox->value(), (unsigned int)m_Ui.mKMeansConvergeSpinBox->value());
		else
			m_Handler3D->GetActivebmphandler()->Em((short)m_Ui.mKMeansNrTissuesSpinBox->value(), (short)m_Ui.mKMeansDimsSpinBox->value(), m_Bits, m_Weights, (unsigned int)m_Ui.mKMeansIterationsSpinBox->value(), (unsigned int)m_Ui.mKMeansConvergeSpinBox->value());
	}

	emit EndDatachange(this);
}

void ThresholdWidgetQt4::BmpChanged()
{
	resetThresholds();
}

void ThresholdWidgetQt4::OnTissuenrChanged(int newValue)
{
	updateUi();
}

void ThresholdWidgetQt4::OnSlicenrChanged()
{
	Init();
}

void ThresholdWidgetQt4::resetThresholds()
{
	auto range = get_range();
	for (unsigned i = 0; i < 20; i++)
	{
		if (m_Threshs[i + 1] > range.second)
			m_Threshs[i + 1] = range.second;
		if (m_Threshs[i + 1] < range.first)
			m_Threshs[i + 1] = range.first;
	}
}

void ThresholdWidgetQt4::RGBA_changed()
{
	// amazing code here...
	int buttons_checked = m_Ui.mKMeansRRadioButton->isChecked() + m_Ui.mKMeansGRadioButton->isChecked() +
												m_Ui.mKMeansBRadioButton->isChecked() + m_Ui.mKMeansARadioButton->isChecked();
	m_Ui.mKMeansDimsSpinBox->setValue(buttons_checked + 1);

	if (buttons_checked > 0)
		m_Ui.mKMeansImageNrSpinBox->setValue(2);
}

} // namespace iseg
