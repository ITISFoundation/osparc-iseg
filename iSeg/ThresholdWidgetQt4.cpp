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
#include <QSignalMapper>
#include <QFileDialog>
#include <QDoubleValidator>

using namespace iseg;


ThresholdWidgetQt4::ThresholdWidgetQt4(SlicesHandler* hand3D, 
										QWidget *parent, 
										const char* name, 
										Qt::WindowFlags wFlags):
WidgetInterface(parent, name, wFlags), 
handler3D(hand3D)
{
	activeslice = handler3D->active_slice();
	bmphand = handler3D->get_activebmphandler();

	initUi();	
	updateUi();
}

ThresholdWidgetQt4::~ThresholdWidgetQt4()
{

}

void ThresholdWidgetQt4::init()
{
	if (activeslice != handler3D->active_slice())
	{
		activeslice = handler3D->active_slice();
		bmphand = handler3D->get_activebmphandler();
		auto range = get_range();
		auto range_validator = new QDoubleValidator(range.first, range.second, 1000, ui.mThresholdBorderLineEdit);
		ui.mThresholdBorderLineEdit->setValidator(range_validator);

		threshs[0] = float(ui.mManualNrTissuesSpinBox->value() - 1);
		for (unsigned i = 0; i < 20; ++i)
		{
			bits1[i] = 0;
			threshs[i + 1] = range.second;
			weights[i] = 1.0f;
		}
	}
	else {
		bmphand = handler3D->get_activebmphandler();
		auto range = get_range();
		for (unsigned i = 0; i < 20; i++)
		{
			if (threshs[i + 1] > range.second)
				threshs[i + 1] = range.second;
			if (threshs[i + 1] < range.first)
				threshs[i + 1] = range.first;
		}
	}

	
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
		dummy = slider->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = ratio->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = sb_nrtissues->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = sb_dim->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = sb_tissuenr->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = sb_px->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = sb_py->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = sb_lx->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = sb_ly->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = sb_iternr->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = sb_converge->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = sb_minpix->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(subsect->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_manual->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_histo->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_kmeans->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_EM->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(allslices->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);

		fwrite(&upper, 1, sizeof(float), fp);
		fwrite(&lower, 1, sizeof(float), fp);
		fwrite(threshs, 21, sizeof(float), fp);
		fwrite(weights, 20, sizeof(float), fp);
	}

	return fp;
}

FILE* ThresholdWidgetQt4::LoadParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		QObject::disconnect(sb_nrtissues, SIGNAL(valueChanged(int)), this,
			SLOT(nrtissues_changed(int)));
		QObject::disconnect(sb_dim, SIGNAL(valueChanged(int)), this,
			SLOT(dim_changed(int)));
		QObject::disconnect(sb_tissuenr, SIGNAL(valueChanged(int)), this,
			SLOT(tissuenr_changed(int)));
		QObject::disconnect(slider, SIGNAL(sliderMoved(int)), this,
			SLOT(slider_changed(int)));
		QObject::disconnect(le_borderval, SIGNAL(returnPressed()), this,
			SLOT(le_borderval_returnpressed()));
		QObject::disconnect(subsect, SIGNAL(clicked()), this,
			SLOT(subsect_toggled()));
		QObject::disconnect(modegroup, SIGNAL(buttonClicked(int)), this,
			SLOT(method_changed(int)));

		int dummy;
		fread(&dummy, sizeof(int), 1, fp);
		slider->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		ratio->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		sb_nrtissues->setValue(dummy);
		sb_tissuenr->setMaxValue(dummy - 1);
		fread(&dummy, sizeof(int), 1, fp);
		sb_dim->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		sb_tissuenr->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		sb_px->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		sb_py->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		sb_lx->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		sb_ly->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		sb_iternr->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		sb_converge->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		sb_minpix->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		subsect->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		rb_manual->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		rb_histo->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		rb_kmeans->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		rb_EM->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		allslices->setChecked(dummy > 0);

		fread(&upper, sizeof(float), 1, fp);
		fread(&lower, sizeof(float), 1, fp);
		fread(threshs, sizeof(float), 21, fp);
		fread(weights, sizeof(float), 20, fp);

		dummy = sb_tissuenr->value();
		method_changed(0);
		subsect_toggled();
		nrtissues_changed(sb_nrtissues->value());
		dim_changed(sb_dim->value());
		sb_tissuenr->setValue(dummy);
		on_tissuenr_changed(dummy);

		QObject::connect(subsect, SIGNAL(clicked()), this,
			SLOT(subsect_toggled()));
		QObject::connect(modegroup, SIGNAL(buttonClicked(int)), this,
			SLOT(method_changed(int)));
		QObject::connect(sb_nrtissues, SIGNAL(valueChanged(int)), this,
			SLOT(nrtissues_changed(int)));
		QObject::connect(sb_dim, SIGNAL(valueChanged(int)), this,
			SLOT(dim_changed(int)));
		QObject::connect(sb_tissuenr, SIGNAL(valueChanged(int)), this,
			SLOT(tissuenr_changed(int)));
		QObject::connect(slider, SIGNAL(sliderMoved(int)), this,
			SLOT(slider_changed(int)));
		QObject::connect(le_borderval, SIGNAL(returnPressed()), this,
			SLOT(le_borderval_returnpressed()));
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

	if (loadfilename.isEmpty()) {
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
		bmphand->threshold(threshs);

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
				bmphand->threshold(threshs);

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
}

void ThresholdWidgetQt4::on_mKMeansFilenameLineEdit_editingFinished()
{

}

void ThresholdWidgetQt4::on_mKMeansFilenamePushButton_clicked()
{
	QString loadfilename = QFileDialog::getOpenFileName(this, "Select an image"
		QString::null,
		"Images (*.png)\n"
		"Images (*.mhd)\n"
		"All (*.*)" //"Images (*.bmp)\n" "All (*.*)", QString::null,
		);			//, filename);
	ui.mKMeansFilenameLineEdit_2->setText(loadfilename);
	filenames[ui.mKMeansImageNrSpinBox->value() - 2] = loadfilename;

	ui.mKMeansRRadioButton_2->setEnabled(false);
	ui.mKMeansGRadioButton_2->setEnabled(false);
	ui.mKMeansBRadioButton_2->setEnabled(false);
	ui.mKMeansARadioButton_2->setEnabled(false);

	QFileInfo fi(loadfilename);
	QString ext = fi.extension();
	if (ext == "png")
	{
		QImage image(loadfilename);

		if (image.depth() > 8)
		{
			ui.mKMeansRRadioButton_2->setEnabled(true);
			ui.mKMeansGRadioButton_2->setEnabled(true);
			ui.mKMeansBRadioButton_2->setEnabled(true);
			if (image.hasAlphaChannel())
				ui.mKMeansARadioButton_2->setEnabled(true);
		}
	}
}

void ThresholdWidgetQt4::on_mKMeansRRadioButton_clicked()
{

}

void ThresholdWidgetQt4::on_mKMeansGRadioButton_clicked()
{

}

void ThresholdWidgetQt4::on_mKMeansBRadioButton_clicked()
{

}

void ThresholdWidgetQt4::on_mKMeansARadioButton_clicked()
{

}

void ThresholdWidgetQt4::on_mKMeansIterationsSpinBox_valueChanged(int newValue)
{

}

void ThresholdWidgetQt4::on_mKMeansConvergeSpinBox_valueChanged(int newValue)
{

}

void ThresholdWidgetQt4::on_mUseCenterFileCheckBox_toggled(bool newValue)
{
	if (!newValue) {
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
			bmphand->threshold(threshs);
	}
	else if (ui.mHistoModeRadioButton->isChecked())
	{
		bmphand->swap_bmpwork();

		if (ui.mSubsectionGroupBox->isChecked())
		{
			Point p;
			p.px = (unsigned short)ui.mHistoPxSpinBox->value();
			p.py = (unsigned short)ui.mHistoPySpinBox->value();
			bmphand->make_histogram(p, ui.mHistoLxSpinBox->value(), ui.mHistoLySpinBox->value(), true);
		}
		else
			bmphand->make_histogram(true);

		bmphand->gaussian_hist(1.0f);
		bmphand->swap_bmpwork();

		float* thresh1 = bmphand->find_modal((unsigned)ui.mHistoMinPixelsSpinBox->value(),
			0.005f * ui.mHistoMinPixelsRatioHorizontalSlider->value());
		if (ui.mAllSlicesCheckBox->isChecked())
			handler3D->threshold(thresh1);
		else
			bmphand->threshold(thresh1);
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
				bmphand->gamma_mhd(
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
					if (ui.mKMeansRRadioButton_2->isChecked())
						extractChannels.push_back(0);
					if (ui.mKMeansGRadioButton_2->isChecked())
						extractChannels.push_back(1);
					if (ui.mKMeansBRadioButton_2->isChecked())
						extractChannels.push_back(2);
					if (ui.mKMeansARadioButton_2->isChecked())
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
						bmphand->kmeans_png(
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
						bmphand->kmeans_mhd((short)ui.mKMeansNrTissuesSpinBox->value(),
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
					bmphand->kmeans_mhd((short)ui.mKMeansNrTissuesSpinBox->value(),
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
				bits[i] = bmphand->return_bmp();
			else
				bits[i] = bmphand->getstack(bits1[i], modedummy);
		}

		if (ui.mAllSlicesCheckBox->isChecked())
			handler3D->em(handler3D->active_slice(),
			(short)ui.mKMeansNrTissuesSpinBox->value(),
				(unsigned int)ui.mKMeansIterationsSpinBox->value(),
				(unsigned int)ui.mKMeansConvergeSpinBox->value());
		else
			bmphand->em((short)ui.mKMeansNrTissuesSpinBox->value(), (short)ui.mKMeansDimsSpinBox->value(),
				bits, weights, (unsigned int)ui.mKMeansIterationsSpinBox->value(),
				(unsigned int)ui.mKMeansConvergeSpinBox->value());
	}

	emit end_datachange(this);
}

void ThresholdWidgetQt4::on_tissuenr_changed(int newValue)
{
	updateUi();
}

void ThresholdWidgetQt4::on_slicenr_changed()
{
	init();
}

void ThresholdWidgetQt4::initUi()
{
	ui.setupUi(this);

	activeslice = handler3D->active_slice();
	bmphand = handler3D->get_activebmphandler();


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
	if (range.second != range.first) {
		const int slider_value = static_cast<int>((threshs[ui.mManualLimitNrSpinBox->value()] - range.first) * 200 /
			(range.second - range.first) + .5f);
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
	ui.mKMeansWeightHorizontalSlider->setValue(int(200 * weights[ui.mKMeansImageNrSpinBox->value() - 1]));
	ui.mKMeansFilenameFrame->setEnabled(ui.mKMeansImageNrSpinBox->value() > 1);	
	ui.mKMeansFilenameLineEdit_2->setText(ui.mKMeansImageNrSpinBox->value() > 1 ? filenames[ui.mKMeansImageNrSpinBox->value() - 2] : "");

	// general
	ui.mCenterFilenameLineEdit->setEnabled(ui.mUseCenterFileCheckBox->isChecked());
	ui.mSelectCenterFilenamePushButton->setEnabled(ui.mUseCenterFileCheckBox->isChecked());
}

std::pair<float, float> ThresholdWidgetQt4::get_range() const
{
	std::pair<float, float> range(0, 0);
	if (bmphand) {
		Pair p;
		bmphand->get_bmprange(&p);
		range.first = p.low;
		range.second = p.high;
	}
	return range;
}