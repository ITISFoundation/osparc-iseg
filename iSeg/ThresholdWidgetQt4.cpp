/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
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

#include <QDoubleValidator>
#include <QFileDialog>
#include <QSignalMapper>

namespace iseg {

ThresholdWidgetQt4::ThresholdWidgetQt4(SlicesHandler* hand3D, QWidget* parent, const char* name, Qt::WindowFlags wFlags)
		: WidgetInterface(parent, name, wFlags), handler3D(hand3D)
{
	initUi();
	updateUi();
}

ThresholdWidgetQt4::~ThresholdWidgetQt4()
{
}

void ThresholdWidgetQt4::initUi()
{
	ui.setupUi(this);

	threshs[0] = float(ui.mManualNrTissuesSpinBox->value() - 1);
	for (unsigned i = 0; i < 20; ++i)
	{
		bits1[i] = 0;
		threshs[i + 1] = 255;
		weights[i] = 1.0f;
	}

	auto mode_signal_mapping = new QSignalMapper(this);
	mode_signal_mapping->setMapping(ui.mManualModeRadioButton, ui.mManualWidget);
	mode_signal_mapping->setMapping(ui.mHistoModeRadioButton, ui.mHistoWidget);
	mode_signal_mapping->setMapping(ui.mKMeansRadioButton, ui.mKmeansEMWidget);
	mode_signal_mapping->setMapping(ui.mEMModeRadioButton, ui.mKmeansEMWidget);
	QObject::connect(ui.mManualModeRadioButton, SIGNAL(clicked()), mode_signal_mapping, SLOT(map()));
	QObject::connect(ui.mHistoModeRadioButton, SIGNAL(clicked()), mode_signal_mapping, SLOT(map()));
	QObject::connect(ui.mKMeansRadioButton, SIGNAL(clicked()), mode_signal_mapping, SLOT(map()));
	QObject::connect(ui.mEMModeRadioButton, SIGNAL(clicked()), mode_signal_mapping, SLOT(map()));
	QObject::connect(mode_signal_mapping, SIGNAL(mapped(QWidget*)), ui.mModeStackedWidget, SLOT(setCurrentWidget(QWidget*)));

	// init with manual mode
	ui.mManualModeRadioButton->setChecked(true);
	ui.mModeStackedWidget->setCurrentWidget(ui.mManualWidget);
}

void ThresholdWidgetQt4::updateUi()
{
	auto range = get_range();
	// manual
	ui.mLoadBordersPushButton->setDisabled(hideparams);
	ui.mSaveBordersPushButton->setDisabled(hideparams);
	ui.mThresholdLowerLabel->setText(QString::number(range.first, 'g', 3));
	ui.mThresholdUpperLabel->setText(QString::number(range.second, 'g', 3));
	if (range.second != range.first)
	{
		const int slider_value = static_cast<int>((threshs[ui.mManualLimitNrSpinBox->value()] - range.first) * 200 / (range.second - range.first) + .5f);
		ui.mThresholdHorizontalSlider->setValue(slider_value);
		ui.mThresholdBorderLineEdit->setText(QString::number(threshs[ui.mManualLimitNrSpinBox->value()], 'g', 3));
	}

	//ui.mWeightBorderLineEdit->setText(QString::number(threshs[sb_tissuenr->value()], 'g', 3));
	ui.mManualLimitNrLabel->setEnabled(ui.mManualNrTissuesSpinBox->value() != 2);
	ui.mManualLimitNrSpinBox->setEnabled(ui.mManualNrTissuesSpinBox->value() != 2);

	// histo
	ui.mSubsectionGroupBox->setDisabled(hideparams);
	ui.mMinPixelsFrame->setDisabled(hideparams);

	// kmeans
	ui.mKMeansDimsLabel->setDisabled(hideparams);
	ui.mKMeansDimsSpinBox->setDisabled(hideparams);
	ui.mKMeansIterationsFrame->setDisabled(hideparams);
	ui.mKMeansImageNrLabel->setEnabled(ui.mKMeansDimsSpinBox->value() != 1);
	ui.mKMeansImageNrSpinBox->setEnabled(ui.mKMeansDimsSpinBox->value() != 1);
	ui.mKMeansWeightLabel->setEnabled(ui.mKMeansDimsSpinBox->value() != 1);
	ui.mKMeansWeightHorizontalSlider->setEnabled(ui.mKMeansDimsSpinBox->value() != 1);
	ui.mKMeansWeightHorizontalSlider->setValue(int(200 * weights[ui.mKMeansImageNrSpinBox->value() - 1]));
	ui.mKMeansFilenameFrame->setEnabled(ui.mKMeansImageNrSpinBox->value() > 1);
	ui.mKMeansFilenameLineEdit->setText(ui.mKMeansImageNrSpinBox->value() > 1 ? filenames[ui.mKMeansImageNrSpinBox->value() - 2] : "");

	// general
	ui.mCenterFilenameLineEdit->setEnabled(ui.mUseCenterFileCheckBox->isChecked());
	ui.mSelectCenterFilenamePushButton->setEnabled(ui.mUseCenterFileCheckBox->isChecked());
}

std::pair<float, float> ThresholdWidgetQt4::get_range() const
{
	std::pair<float, float> range(0.0f, 0.0f);
	if (handler3D->get_activebmphandler())
	{
		Pair p;
		handler3D->get_activebmphandler()->get_bmprange(&p);
		range.first = p.low;
		range.second = p.high;
	}
	return range;
}

void ThresholdWidgetQt4::init()
{
	auto range = get_range();
	auto range_validator = new QDoubleValidator(range.first, range.second, 1000, ui.mThresholdBorderLineEdit);
	ui.mThresholdBorderLineEdit->setValidator(range_validator);

	resetThresholds();

	updateUi();
}

void ThresholdWidgetQt4::newloaded()
{
	init();
}

void ThresholdWidgetQt4::hideparams_changed()
{
	init();
}

FILE* ThresholdWidgetQt4::SaveParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		int dummy;
		dummy = ui.mThresholdHorizontalSlider->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = ui.mHistoMinPixelsRatioHorizontalSlider->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = ui.mManualNrTissuesSpinBox->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = ui.mKMeansDimsSpinBox->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = ui.mManualLimitNrSpinBox->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = ui.mHistoPxSpinBox->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = ui.mHistoPySpinBox->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = ui.mHistoLxSpinBox->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = ui.mHistoLySpinBox->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = ui.mKMeansIterationsSpinBox->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = ui.mKMeansConvergeSpinBox->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = ui.mHistoMinPixelsSpinBox->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(ui.mSubsectionGroupBox->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(ui.mManualModeRadioButton->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(ui.mHistoModeRadioButton->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(ui.mKMeansRadioButton->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(ui.mEMModeRadioButton->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(ui.mAllSlicesCheckBox->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);

		auto range = get_range();
		fwrite(&range.second, 1, sizeof(float), fp);
		fwrite(&range.first, 1, sizeof(float), fp);
		fwrite(threshs, 21, sizeof(float), fp);
		fwrite(weights, 20, sizeof(float), fp);
	}

	return fp;
}

FILE* ThresholdWidgetQt4::LoadParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		int dummy;
		fread(&dummy, sizeof(int), 1, fp);
		ui.mThresholdHorizontalSlider->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		ui.mHistoMinPixelsRatioHorizontalSlider->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		ui.mManualNrTissuesSpinBox->setValue(dummy);
		ui.mManualLimitNrSpinBox->setMaxValue(dummy - 1);
		fread(&dummy, sizeof(int), 1, fp);
		ui.mKMeansDimsSpinBox->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		ui.mManualLimitNrSpinBox->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		ui.mHistoPxSpinBox->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		ui.mHistoPySpinBox->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		ui.mHistoLxSpinBox->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		ui.mHistoLySpinBox->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		ui.mKMeansIterationsSpinBox->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		ui.mKMeansConvergeSpinBox->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		ui.mHistoMinPixelsSpinBox->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		ui.mSubsectionGroupBox->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		ui.mManualModeRadioButton->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		ui.mHistoModeRadioButton->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		ui.mKMeansRadioButton->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		ui.mEMModeRadioButton->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		ui.mAllSlicesCheckBox->setChecked(dummy > 0);

		float upper = 0, lower = 0;
		fread(&upper, sizeof(float), 1, fp);
		fread(&lower, sizeof(float), 1, fp);
		fread(threshs, sizeof(float), 21, fp);
		fread(weights, sizeof(float), 20, fp);

		dummy = ui.mManualNrTissuesSpinBox->value();
		on_tissuenr_changed(dummy);

		if (ui.mManualModeRadioButton->isChecked())
			ui.mModeStackedWidget->setCurrentWidget(ui.mManualWidget);
		else if (ui.mHistoModeRadioButton->isChecked())
			ui.mModeStackedWidget->setCurrentWidget(ui.mHistoWidget);
		else
			ui.mModeStackedWidget->setCurrentWidget(ui.mKmeansEMWidget);
	}
	return fp;
}

void ThresholdWidgetQt4::on_mManualNrTissuesSpinBox_valueChanged(int newValue)
{
	threshs[0] = float(newValue - 1);
	ui.mManualLimitNrSpinBox->setMaxValue(newValue - 1);

	updateUi();
}

void ThresholdWidgetQt4::on_mManualLimitNrSpinBox_valueChanged(int newValue)
{
	updateUi();
}

void ThresholdWidgetQt4::on_mLoadBordersPushButton_clicked()
{
	auto loadfilename =
			QFileDialog::getOpenFileName(this, "Select Boarder file", QString::null,
					"Boarders (*.txt)\n"
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

	if (fvec.size() > 0 && fvec.size() < 20)
	{
		ui.mManualNrTissuesSpinBox->setValue(static_cast<int>(fvec.size() + 1));
		threshs[0] = float(fvec.size());
		ui.mManualLimitNrSpinBox->setMaxValue(static_cast<int>(fvec.size()));

		auto range = get_range();
		for (unsigned i = 0; i < (unsigned)fvec.size(); i++)
		{
			if (fvec[i] > range.second)
				f = range.second;
			else if (fvec[i] < range.first)
				f = range.first;
			else
				f = fvec[i];
			threshs[i + 1] = f;
		}
		ui.mManualLimitNrSpinBox->setValue(1);
	}

	updateUi();
}

void ThresholdWidgetQt4::on_mSaveBordersPushButton_clicked()
{
	auto savefilename = QFileDialog::getSaveFileName(this, "Save file",
			QString::null, "Boarders (*.txt)\n");

	if (savefilename.length() > 4 && !savefilename.endsWith(QString(".txt")))
		savefilename.append(".txt");

	if (!savefilename.isEmpty())
	{
		FILE* fp = fopen(savefilename.ascii(), "w");
		for (int i = 1; i <= ui.mManualLimitNrSpinBox->maxValue(); i++)
			fprintf(fp, "%f\n", threshs[i]);
		fclose(fp);
	}
}

void ThresholdWidgetQt4::on_mThresholdHorizontalSlider_valueChanged(int newValue)
{
	auto range = get_range();
	threshs[ui.mManualLimitNrSpinBox->value()] =
			newValue * 0.005f * (range.second - range.first) + range.first;

	if (ui.mAllSlicesCheckBox->isChecked())
		handler3D->threshold(threshs);
	else
		handler3D->get_activebmphandler()->threshold(threshs);

	emit end_datachange(this, iseg::NoUndo);

	updateUi();
}

void ThresholdWidgetQt4::on_mThresholdBorderLineEdit_editingFinished()
{
	bool b1;
	float val = ui.mThresholdBorderLineEdit->text().toFloat(&b1);
	if (b1)
	{
		threshs[ui.mManualLimitNrSpinBox->value()] = val;
		{
			iseg::DataSelection dataSelection;
			dataSelection.allSlices = ui.mAllSlicesCheckBox->isChecked();
			dataSelection.sliceNr = handler3D->active_slice();
			dataSelection.work = true;
			emit begin_datachange(dataSelection, this);

			if (ui.mAllSlicesCheckBox->isChecked())
				handler3D->threshold(threshs);
			else
				handler3D->get_activebmphandler()->threshold(threshs);

			emit end_datachange(this);
		}
	}

	updateUi();
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
	const size_t cursize = filenames.size();
	if (newValue > cursize + 1)
	{
		filenames.resize(newValue - 1);
		for (size_t i = cursize; i + 1 < newValue; ++i)
			filenames[i] = QString("");
	}

	ui.mKMeansImageNrSpinBox->setMaxValue(newValue);
	ui.mKMeansImageNrSpinBox->setValue(1);

	updateUi();
}

void ThresholdWidgetQt4::on_mKMeansImageNrSpinBox_valueChanged(int newValue)
{
	updateUi();
}

void ThresholdWidgetQt4::on_mKMeansWeightHorizontalSlider_valueChanged(int newValue)
{
	weights[ui.mKMeansImageNrSpinBox->value() - 1] = newValue * 0.005f;
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
			"All (*.*)" //"Images (*.bmp)\n" "All (*.*)", QString::null,
	);							//, filename);
	ui.mKMeansFilenameLineEdit->setText(loadfilename);
	filenames[ui.mKMeansImageNrSpinBox->value() - 2] = loadfilename;

	ui.mKMeansRRadioButton->setEnabled(false);
	ui.mKMeansGRadioButton->setEnabled(false);
	ui.mKMeansBRadioButton->setEnabled(false);
	ui.mKMeansARadioButton->setEnabled(false);

	QFileInfo fi(loadfilename);
	QString ext = fi.extension();
	if (ext == "png")
	{
		QImage image(loadfilename);

		if (image.depth() > 8)
		{
			ui.mKMeansRRadioButton->setEnabled(true);
			ui.mKMeansGRadioButton->setEnabled(true);
			ui.mKMeansBRadioButton->setEnabled(true);
			if (image.hasAlphaChannel())
				ui.mKMeansARadioButton->setEnabled(true);
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
		ui.mCenterFilenameLineEdit->setText("");
	}
	updateUi();
}

void ThresholdWidgetQt4::on_mCenterFilenameLineEdit_editingFinished()
{
	// never happens
}

void ThresholdWidgetQt4::on_mSelectCenterFilenamePushButton_clicked()
{
	auto center_file_name = QFileDialog::getOpenFileName(nullptr,
			"Select center file", QString(), "Text File (*.txt*)");
	ui.mCenterFilenameLineEdit->setText(center_file_name);
}

void ThresholdWidgetQt4::on_mAllSlicesCheckBox_toggled(bool newValue)
{
	// does nothing...
}

void ThresholdWidgetQt4::on_mExecutePushButton_clicked()
{
	unsigned char modedummy;

	iseg::DataSelection dataSelection;
	dataSelection.allSlices = ui.mAllSlicesCheckBox->isChecked();
	dataSelection.sliceNr = handler3D->active_slice();
	dataSelection.work = true;
	emit begin_datachange(dataSelection, this);

	if (ui.mManualModeRadioButton->isChecked())
	{
		if (ui.mAllSlicesCheckBox->isChecked())
			handler3D->threshold(threshs);
		else
			handler3D->get_activebmphandler()->threshold(threshs);
	}
	else if (ui.mHistoModeRadioButton->isChecked())
	{
		handler3D->get_activebmphandler()->swap_bmpwork();

		if (ui.mSubsectionGroupBox->isChecked())
		{
			Point p;
			p.px = (unsigned short)ui.mHistoPxSpinBox->value();
			p.py = (unsigned short)ui.mHistoPySpinBox->value();
			handler3D->get_activebmphandler()->make_histogram(p, ui.mHistoLxSpinBox->value(), ui.mHistoLySpinBox->value(), true);
		}
		else
			handler3D->get_activebmphandler()->make_histogram(true);

		handler3D->get_activebmphandler()->gaussian_hist(1.0f);
		handler3D->get_activebmphandler()->swap_bmpwork();

		float* thresh1 = handler3D->get_activebmphandler()->find_modal((unsigned)ui.mHistoMinPixelsSpinBox->value(),
				0.005f * ui.mHistoMinPixelsRatioHorizontalSlider->value());
		if (ui.mAllSlicesCheckBox->isChecked())
			handler3D->threshold(thresh1);
		else
			handler3D->get_activebmphandler()->threshold(thresh1);
		free(thresh1);
	}
	else if (ui.mKMeansRadioButton->isChecked())
	{
		FILE* fp = fopen("C:\\gamma.txt", "r"); // really????
		if (fp != nullptr)
		{
			float** centers = new float*[ui.mKMeansNrTissuesSpinBox->value()];
			for (int i = 0; i < ui.mKMeansNrTissuesSpinBox->value(); i++)
				centers[i] = new float[ui.mKMeansDimsSpinBox->value()];
			float* tol_f = new float[ui.mKMeansDimsSpinBox->value()];
			float* tol_d = new float[ui.mKMeansDimsSpinBox->value()];
			for (int i = 0; i < ui.mKMeansNrTissuesSpinBox->value(); i++)
			{
				for (int j = 0; j < ui.mKMeansDimsSpinBox->value(); j++)
					fscanf(fp, "%f", &(centers[i][j]));
			}
			for (int j = 0; j < ui.mKMeansDimsSpinBox->value(); j++)
				fscanf(fp, "%f", &(tol_f[j]));
			for (int j = 1; j < ui.mKMeansDimsSpinBox->value(); j++)
				fscanf(fp, "%f", &(tol_d[j]));
			tol_d[0] = 0.0f;
			fclose(fp);
			std::vector<std::string> mhdfiles;
			for (int i = 0; i + 1 < ui.mKMeansDimsSpinBox->value(); i++)
				mhdfiles.push_back(std::string(filenames[i].ascii()));
			if (ui.mAllSlicesCheckBox->isChecked())
				handler3D->gamma_mhd(handler3D->active_slice(),
						(short)ui.mKMeansNrTissuesSpinBox->value(),
						(short)ui.mKMeansDimsSpinBox->value(), mhdfiles, weights,
						centers, tol_f, tol_d);
			else
				handler3D->get_activebmphandler()->gamma_mhd(
						(short)ui.mKMeansNrTissuesSpinBox->value(), (short)ui.mKMeansDimsSpinBox->value(),
						mhdfiles, handler3D->active_slice(), weights, centers,
						tol_f, tol_d, handler3D->get_pixelsize());
			delete[] tol_d;
			delete[] tol_f;
			for (int i = 0; i < ui.mKMeansNrTissuesSpinBox->value(); i++)
				delete[] centers[i];
			delete[] centers;
		}
		else
		{
			std::vector<std::string> kmeansfiles;
			for (int i = 0; i + 1 < ui.mKMeansDimsSpinBox->value(); i++)
				kmeansfiles.push_back(std::string(filenames[i].ascii()));
			if (kmeansfiles.size() > 0)
			{
				if (kmeansfiles[0].substr(kmeansfiles[0].find_last_of(".") +
																	1) == "png")
				{
					std::vector<int> extractChannels;
					if (ui.mKMeansRRadioButton->isChecked())
						extractChannels.push_back(0);
					if (ui.mKMeansGRadioButton->isChecked())
						extractChannels.push_back(1);
					if (ui.mKMeansBRadioButton->isChecked())
						extractChannels.push_back(2);
					if (ui.mKMeansARadioButton->isChecked())
						extractChannels.push_back(3);
					if (ui.mKMeansDimsSpinBox->value() != extractChannels.size() + 1)
						return;
					if (ui.mAllSlicesCheckBox->isChecked())
						handler3D->kmeans_png(
								handler3D->active_slice(),
								(short)ui.mKMeansNrTissuesSpinBox->value(),
								(short)ui.mKMeansDimsSpinBox->value(), kmeansfiles,
								extractChannels, weights,
								(unsigned int)ui.mKMeansIterationsSpinBox->value(),
								(unsigned int)ui.mKMeansConvergeSpinBox->value(),
								ui.mCenterFilenameLineEdit->text().toStdString());
					else
						handler3D->get_activebmphandler()->kmeans_png(
								(short)ui.mKMeansNrTissuesSpinBox->value(),
								(short)ui.mKMeansDimsSpinBox->value(), kmeansfiles,
								extractChannels, handler3D->active_slice(),
								weights, (unsigned int)ui.mKMeansIterationsSpinBox->value(),
								(unsigned int)ui.mKMeansConvergeSpinBox->value(),
								ui.mCenterFilenameLineEdit->text().toStdString());
				}
				else
				{
					if (ui.mAllSlicesCheckBox->isChecked())
						handler3D->kmeans_mhd(
								handler3D->active_slice(),
								(short)ui.mKMeansNrTissuesSpinBox->value(),
								(short)ui.mKMeansDimsSpinBox->value(), kmeansfiles, weights,
								(unsigned int)ui.mKMeansIterationsSpinBox->value(),
								(unsigned int)ui.mKMeansConvergeSpinBox->value());
					else
						handler3D->get_activebmphandler()->kmeans_mhd((short)ui.mKMeansNrTissuesSpinBox->value(),
								(short)ui.mKMeansDimsSpinBox->value(), kmeansfiles,
								handler3D->active_slice(),
								weights,
								(unsigned int)ui.mKMeansIterationsSpinBox->value(),
								(unsigned int)ui.mKMeansConvergeSpinBox->value());
				}
			}
			else
			{
				if (ui.mAllSlicesCheckBox->isChecked())
					handler3D->kmeans_mhd(
							handler3D->active_slice(),
							(short)ui.mKMeansNrTissuesSpinBox->value(), (short)ui.mKMeansDimsSpinBox->value(),
							kmeansfiles, weights, (unsigned int)ui.mKMeansIterationsSpinBox->value(),
							(unsigned int)ui.mKMeansConvergeSpinBox->value());
				else
					handler3D->get_activebmphandler()->kmeans_mhd((short)ui.mKMeansNrTissuesSpinBox->value(),
							(short)ui.mKMeansDimsSpinBox->value(), kmeansfiles,
							handler3D->active_slice(), weights,
							(unsigned int)ui.mKMeansIterationsSpinBox->value(),
							(unsigned int)ui.mKMeansConvergeSpinBox->value());
			}
		}
	}
	else
	{
		//		EM em;
		for (int i = 0; i < ui.mKMeansDimsSpinBox->value(); i++)
		{
			if (bits1[i] == 0)
				bits[i] = handler3D->get_activebmphandler()->return_bmp();
			else
				bits[i] = handler3D->get_activebmphandler()->getstack(bits1[i], modedummy);
		}

		if (ui.mAllSlicesCheckBox->isChecked())
			handler3D->em(handler3D->active_slice(),
					(short)ui.mKMeansNrTissuesSpinBox->value(),
					(unsigned int)ui.mKMeansIterationsSpinBox->value(),
					(unsigned int)ui.mKMeansConvergeSpinBox->value());
		else
			handler3D->get_activebmphandler()->em((short)ui.mKMeansNrTissuesSpinBox->value(), (short)ui.mKMeansDimsSpinBox->value(),
					bits, weights, (unsigned int)ui.mKMeansIterationsSpinBox->value(),
					(unsigned int)ui.mKMeansConvergeSpinBox->value());
	}

	emit end_datachange(this);
}

void ThresholdWidgetQt4::bmp_changed()
{
	resetThresholds();
}

void ThresholdWidgetQt4::on_tissuenr_changed(int newValue)
{
	updateUi();
}

void ThresholdWidgetQt4::on_slicenr_changed()
{
	init();
}

void ThresholdWidgetQt4::resetThresholds()
{
	auto range = get_range();
	for (unsigned i = 0; i < 20; i++)
	{
		if (threshs[i + 1] > range.second)
			threshs[i + 1] = range.second;
		if (threshs[i + 1] < range.first)
			threshs[i + 1] = range.first;
	}
}

void ThresholdWidgetQt4::RGBA_changed()
{
	// amazing code here...
	int buttonsChecked = ui.mKMeansRRadioButton->isChecked() + ui.mKMeansGRadioButton->isChecked() +
											 ui.mKMeansBRadioButton->isChecked() + ui.mKMeansARadioButton->isChecked();
	ui.mKMeansDimsSpinBox->setValue(buttonsChecked + 1);

	if (buttonsChecked > 0)
		ui.mKMeansImageNrSpinBox->setValue(2);
}

}// namespace iseg