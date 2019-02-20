/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "Precompiled.h"

#include "MultiDatasetWidget.h"
#include "SlicesHandler.h"
#include "LoaderWidgets.h"

#include <itkImageFileReader.h>
#include <itkImageSeriesReader.h>
#include <itkRawImageIO.h>

#include <Q3HBoxLayout>
#include <Q3VBoxLayout>
#include <QGroupBox>
#include <QFileDialog>

using namespace iseg;

MultiDatasetWidget::MultiDatasetWidget(SlicesHandler* hand3D, QWidget* parent,
		const char* name, Qt::WindowFlags wFlags)
		: QWidget(parent, name, wFlags), m_Handler3D(hand3D)
{
	hboxOverall = new Q3HBoxLayout(this);
	vboxOverall = new Q3VBoxLayout();
	hboxOverall->addLayout(vboxOverall);

	// Add dataset button
	m_AddDatasetButton = new QPushButton("Add Dataset...", this);
	vboxOverall->addWidget(m_AddDatasetButton);

	// Dataset selection group box
	m_DatasetsGroupBox = new QGroupBox("- Available datasets -");
	m_VboxDatasets = new Q3VBoxLayout(m_DatasetsGroupBox);
	m_VboxDatasets->addStretch(1);
	m_DatasetsGroupBox->setLayout(m_VboxDatasets);
	vboxOverall->addWidget(m_DatasetsGroupBox);

	//Buttons
	QHBoxLayout* buttonsGrid = new QHBoxLayout();
	vboxOverall->addLayout(buttonsGrid);

	// Add dataset button
	m_LoadDatasetButton = new QPushButton("Load Dataset", this);
	buttonsGrid->addWidget(m_LoadDatasetButton);

	m_ChangeNameButton = new QPushButton("Change Name", this);
	buttonsGrid->addWidget(m_ChangeNameButton);

	m_RemoveDatasetButton = new QPushButton("Remove Dataset", this);
	buttonsGrid->addWidget(m_RemoveDatasetButton);
	m_RemoveDatasetButton->setEnabled(false);

	m_DatasetsGroupBox->setMinimumHeight(200);
	setFixedHeight(hboxOverall->sizeHint().height());

	connect(m_AddDatasetButton, SIGNAL(clicked()), this,
			SLOT(AddDatasetPressed()));
	connect(m_LoadDatasetButton, SIGNAL(clicked()), this,
			SLOT(SwitchDataset()));
	connect(m_ChangeNameButton, SIGNAL(clicked()), this,
			SLOT(ChangeDatasetName()));
	connect(m_RemoveDatasetButton, SIGNAL(clicked()), this,
			SLOT(RemoveDataset()));
	connect(m_DatasetsGroupBox, SIGNAL(clicked()), this,
			SLOT(DatasetSelectionChanged()));

	Initialize();
	InitializeMap();
}

MultiDatasetWidget::~MultiDatasetWidget() { m_RadioButtons.clear(); }

void MultiDatasetWidget::InitializeMap()
{
	m_MapDatasetValues["DCM"] = DatasetTypeEnum::DCM;
	m_MapDatasetValues["DICOM"] = DatasetTypeEnum::DCM;
	m_MapDatasetValues["BMP"] = DatasetTypeEnum::BMP;
	m_MapDatasetValues["PNG"] = DatasetTypeEnum::PNG;
	m_MapDatasetValues["RAW"] = DatasetTypeEnum::RAW;
	m_MapDatasetValues["MHD"] = DatasetTypeEnum::MHD;
	m_MapDatasetValues["AVW"] = DatasetTypeEnum::AVW;
	m_MapDatasetValues["VTI"] = DatasetTypeEnum::VTK;
	m_MapDatasetValues["VTK"] = DatasetTypeEnum::VTK;
	m_MapDatasetValues["XMF"] = DatasetTypeEnum::XDMF;
	m_MapDatasetValues["NII"] = DatasetTypeEnum::NIFTI;
	m_MapDatasetValues["HDR"] = DatasetTypeEnum::NIFTI;
	m_MapDatasetValues["IMG"] = DatasetTypeEnum::NIFTI;
	m_MapDatasetValues["NIA"] = DatasetTypeEnum::NIFTI;
	m_MapDatasetValues["RTDOSE"] = DatasetTypeEnum::RTDOSE;
}

void MultiDatasetWidget::Initialize()
{
	m_ItIsBeingLoaded = false;
	m_RadioButtons.clear();
}

void MultiDatasetWidget::ClearRadioButtons()
{
	if (m_VboxDatasets->layout() != nullptr)
	{
		QLayoutItem* item;
		while ((item = m_VboxDatasets->layout()->takeAt(0)) != nullptr)
		{
			delete item->widget();
			delete item;
		}
	}

	m_VboxDatasets->update();
	m_VboxDatasets->layout()->update();

	m_RadioButtons.clear();
}

void MultiDatasetWidget::NewLoaded()
{
	if (m_ItIsBeingLoaded)
	{
		m_ItIsBeingLoaded = false;
		return;
	}

	ClearRadioButtons();

	Initialize();

	const unsigned short w_loaded = m_Handler3D->width();
	const unsigned short h_loaded = m_Handler3D->height();
	const unsigned short nrofslices_loaded = m_Handler3D->num_slices();

	const bool checkMatch = true;
	if (checkMatch)
	{
		if (w_loaded == 512 && h_loaded == 512 && nrofslices_loaded == 1)
		{
			// empty default image
			return;
		}
	}

	SDatasetInfo newRadioButton;
	// Create the copy of the main dataset only when adding a second dataset
	//CopyImagesSlices(m_Handler3D, newRadioButton);

	QStringList paths;
	paths.append("main_Dataset");
	AddDatasetToList(newRadioButton, paths);

	QFont font(newRadioButton.m_RadioButton->font());
	font.setBold(true);
	newRadioButton.m_RadioButton->setFont(font);
	newRadioButton.m_RadioButton->setChecked(true);
	newRadioButton.m_IsActive = true;

	m_RadioButtons.push_back(newRadioButton);
}

void MultiDatasetWidget::AddDatasetPressed()
{
	// BL todo: 
	// a) detect if correct info without loading full data, abort if incorrect and user does not want to resample
	// b) load via itk instead of SliceHandler, 
	// c) resample to current shape/position if requested

	SupportedMultiDatasetTypes dlg;
	dlg.exec();

	int selectedType = dlg.GetSelectedType();

	if (selectedType != -1)
	{
		using image_type = itk::Image<float, 3>;
		image_type::Pointer image;

		QStringList loadfilenames;
		unsigned short width, height, nrofslices;
		MultiDatasetWidget::SDatasetInfo dataInfo;
		dataInfo.m_IsActive = false;
		bool success = false;

		switch (SupportedMultiDatasetTypes::supportedTypes(selectedType))
		{
		case SupportedMultiDatasetTypes::supportedTypes::bmp:
		case SupportedMultiDatasetTypes::supportedTypes::dcm:
		{
			loadfilenames = QFileDialog::getOpenFileNames(
					"Images (*.dcm *.dicom *.bmp)\n"
					"All (*)",
					QString::null, this, "Open Files",
					"Select one or more files to open");
			
			std::vector< std::string > files;
			for (const auto& f: loadfilenames)
			{
				files.push_back(f.toStdString());
			}
			auto reader = itk::ImageSeriesReader<image_type>::New();
			reader->SetFileNames(files);
			try
			{
				reader->Update();

				image = reader->GetOutput();

				auto dims = image->GetBufferedRegion().GetSize();
				width = static_cast<unsigned>(dims[0]);
				height = static_cast<unsigned>(dims[1]);
				nrofslices = static_cast<unsigned>(dims[2]);

				success = CheckInfoAndAddToList(dataInfo, loadfilenames, width, height, nrofslices);
			}
			catch (itk::ExceptionObject&)
			{}
		}
		break;

		case SupportedMultiDatasetTypes::supportedTypes::raw:
		{

			LoaderRaw LR(nullptr, this);
			LR.setSkipReading(true);
			LR.move(QCursor::pos());
			LR.exec();

			QString loadfilename = LR.GetFileName();
			auto dims = LR.getDimensions();
			auto start = LR.getSubregionStart();
			auto size = LR.getSubregionSize();

			loadfilenames.append(loadfilename);

			auto region = itk::ImageIORegion(3);
			region.SetIndex(std::vector<itk::IndexValueType>(start.begin(), start.end()));
			region.SetSize(std::vector<itk::SizeValueType>(size.begin(), size.end()));

			auto rawio = itk::RawImageIO<float, 3>::New();
			rawio->SetDimensions(0, dims[0]);
			rawio->SetDimensions(1, dims[1]);
			rawio->SetDimensions(2, start[2] + size[2]); // may be more, but at least this many
			rawio->SetIORegion(region);

			auto reader = itk::ImageFileReader<image_type>::New();
			reader->SetImageIO(rawio);
			reader->SetFileName(loadfilename.ascii());
			try
			{
				reader->Update();
				
				image = reader->GetOutput();

				loadfilenames.append(loadfilename);

				auto dims = image->GetBufferedRegion().GetSize();
				width = static_cast<unsigned>(dims[0]);
				height = static_cast<unsigned>(dims[1]);
				nrofslices = static_cast<unsigned>(dims[2]);

				success = CheckInfoAndAddToList(dataInfo, loadfilenames, width, height, nrofslices);
			}
			catch(itk::ExceptionObject& e)
			{
				// todo
			}
		}
		break;

		case SupportedMultiDatasetTypes::supportedTypes::nifti:
		case SupportedMultiDatasetTypes::supportedTypes::vtk:
		{
			bool res = true;
			auto loadfilename = QFileDialog::getOpenFileName(this, tr("Open File"),
					QString::null, tr("Images (*.vti *.vtk *.nii *.hdr *.img *.nia)"));
			if (!loadfilename.isEmpty())
			{
				auto reader = itk::ImageFileReader<image_type>::New();
				reader->SetFileName(loadfilename.ascii());
				try
				{
					reader->Update();

					image = reader->GetOutput();

					loadfilenames.append(loadfilename);

					auto dims = image->GetBufferedRegion().GetSize();
					width = static_cast<unsigned>(dims[0]);
					height = static_cast<unsigned>(dims[1]);
					nrofslices = static_cast<unsigned>(dims[2]);

					success = CheckInfoAndAddToList(dataInfo, loadfilenames, width, height, nrofslices);
				}
				catch(itk::ExceptionObject& e)
				{
					// todo
				}
			}
		}
		break;

		default: break;
		}

		std::cerr << "Success: " << success << std::endl;

		if (success && false)
		{
			std::array<size_t,3> dims = {width, height, nrofslices};

			// Create the copy of the main dataset only when adding a second dataset
			if (m_RadioButtons.size() == 1)
			{
				const SlicesHandler* chandler = m_Handler3D;
				CopyImagesSlices(chandler->source_slices(), chandler->target_slices(), 
					dims, m_RadioButtons.at(0));
			}

			auto buffer = image->GetBufferPointer();
			auto slice_size = static_cast<size_t>(width*height);
			std::vector<const float*> bmp_slices(nrofslices, nullptr);
			for (unsigned i=0; i<nrofslices; ++i)
			{
				bmp_slices[i] = buffer + i * slice_size;
			}

			CopyImagesSlices(bmp_slices, bmp_slices, dims, dataInfo);
			m_RadioButtons.push_back(dataInfo);
		}
	}
}

bool MultiDatasetWidget::CheckInfoAndAddToList(
		MultiDatasetWidget::SDatasetInfo& newRadioButton,
		QStringList loadfilenames, unsigned short width, unsigned short height,
		unsigned short nrofslices)
{
	// check whether the new dataset matches the dataset loaded
	const unsigned short w_loaded = m_Handler3D->width();
	const unsigned short h_loaded = m_Handler3D->height();
	const unsigned short nrofslices_loaded = m_Handler3D->num_slices();

	if (w_loaded == width && h_loaded == height && nrofslices_loaded == nrofslices)
	{
		return AddDatasetToList(newRadioButton, loadfilenames);
	}
	else
	{
		QMessageBox mb(
				"Loading failed",
				"The resolution of the dataset must match the one loaded.",
				QMessageBox::Critical, QMessageBox::Ok | QMessageBox::Default,
				QMessageBox::NoButton, QMessageBox::NoButton);
		mb.exec();
	}

	return false;
}

bool MultiDatasetWidget::AddDatasetToList(
		MultiDatasetWidget::SDatasetInfo& newRadioButton,
		QStringList loadfilenames)
{
	QString butText = loadfilenames[0];
	newRadioButton.m_RadioButton = new QRadioButton(QFileInfo(loadfilenames[0]).fileName());
	newRadioButton.m_DatasetFilepath = loadfilenames;
	newRadioButton.m_RadioButtonText = butText;

	m_VboxDatasets->addWidget(newRadioButton.m_RadioButton, 0, Qt::AlignTop);

	connect(newRadioButton.m_RadioButton, SIGNAL(clicked()), this, SLOT(DatasetSelectionChanged()));

	return !newRadioButton.m_DatasetFilepath.empty();
}

void MultiDatasetWidget::CopyImagesSlices(
		const std::vector<const float*>& bmp_slices, 
		const std::vector<const float*>& work_slices,
		const std::array<size_t,3>& dims, 
		MultiDatasetWidget::SDatasetInfo& newRadioButton)
{
	const size_t nrslices = dims[2];
	const size_t width  = dims[0];
	const size_t height = dims[1];
	const size_t size = width * height;

	newRadioButton.m_Width = width;
	newRadioButton.m_Height = height;

	newRadioButton.m_BmpSlices.clear();
	for (int i = 0; i < nrslices; i++)
	{
		float* bmp_data = (float*)malloc(sizeof(float) * size);
		memcpy(bmp_data, bmp_slices.at(i), sizeof(float) * size);
		newRadioButton.m_BmpSlices.push_back(bmp_data);
	}

	// BL TODO why do we need to store work slices?
	newRadioButton.m_WorkSlices.clear();
	for (int i = 0; i < nrslices; i++)
	{
		float* work_data = (float*)malloc(sizeof(float) * size);
		memcpy(work_data, work_slices.at(i), sizeof(float) * size);
		newRadioButton.m_WorkSlices.push_back(work_data);
	}
}

void MultiDatasetWidget::SwitchDataset()
{
	for (auto& radioButton : m_RadioButtons)
	{
		if (radioButton.m_RadioButton->isChecked())
		{
			QFont font(radioButton.m_RadioButton->font());
			font.setBold(true);
			radioButton.m_RadioButton->setFont(font);

			if (!radioButton.m_BmpSlices.empty())
			{
				iseg::DataSelection dataSelection;
				dataSelection.allSlices = true;
				dataSelection.bmp = true;
				dataSelection.work = true;
				dataSelection.tissues = false;
				emit begin_datachange(dataSelection, this, false);

				float size = radioButton.m_Width * radioButton.m_Height;
				const int nSlices = radioButton.m_BmpSlices.size();
				assert(radioButton.m_BmpSlices.size() == m_Handler3D->num_slices());
				assert(radioButton.m_WorkSlices.size() == m_Handler3D->num_slices());

				for (int i = 0; i < nSlices; i++)
				{
					m_Handler3D->copy2bmp(i, radioButton.m_BmpSlices.at(i), 1);
				}

				for (int i = 0; i < nSlices; i++)
				{
					m_Handler3D->copy2work(i, radioButton.m_WorkSlices.at(i), 1);
				}
				m_ItIsBeingLoaded = true;
				radioButton.m_IsActive = true;

				emit end_datachange(this, iseg::ClearUndo);
				emit dataset_changed();
			}
			continue;
		}
		else
		{
			QFont font(radioButton.m_RadioButton->font());
			font.setBold(false);
			radioButton.m_RadioButton->setFont(font);
			radioButton.m_IsActive = false;
		}
	}

	DatasetSelectionChanged();
}

void MultiDatasetWidget::ChangeDatasetName()
{
	for (auto& radioButton : m_RadioButtons)
	{
		if (radioButton.m_RadioButton->isChecked())
		{
			EditText edit_text_dlg(this->parentWidget(), "",
					this->windowFlags());
			edit_text_dlg.set_editable_text(radioButton.m_RadioButton->text());
			if (edit_text_dlg.exec())
			{
				QString returning_text = edit_text_dlg.get_editable_text();
				radioButton.m_RadioButtonText = returning_text;
				radioButton.m_RadioButton->setText(returning_text);
			}
			return;
		}
	}
}

void MultiDatasetWidget::RemoveDataset()
{
	int index = 0;
	for (auto& radioButton : m_RadioButtons)
	{
		if (radioButton.m_RadioButton->isChecked())
		{
			m_VboxDatasets->removeWidget(radioButton.m_RadioButton);

			// TODO BL -> should use free instead of delete since memory was allocated with malloc
			std::for_each(radioButton.m_BmpSlices.begin(),
					radioButton.m_BmpSlices.end(),
					[](float* element) { delete element; });
			radioButton.m_BmpSlices.clear();

			std::for_each(radioButton.m_WorkSlices.begin(),
					radioButton.m_WorkSlices.end(),
					[](float* element) { delete element; });
			radioButton.m_WorkSlices.clear();

			delete radioButton.m_RadioButton;

			m_RadioButtons.erase(m_RadioButtons.begin() + index);

			break;
		}
		index++;
	}

	// Automatically check the active dataset
	for (int i = 0; i < GetNumberOfDatasets(); i++)
	{
		if (IsActive(i))
		{
			m_RadioButtons.at(i).m_RadioButton->setChecked(true);
			return;
		}
	}
}

void MultiDatasetWidget::DatasetSelectionChanged()
{
	// Do not allow removing active dataset
	for (int i = 0; i < GetNumberOfDatasets(); i++)
	{
		if (IsActive(i) && IsChecked(i))
		{
			m_RemoveDatasetButton->setEnabled(false);
			break;
		}
		else
		{
			m_RemoveDatasetButton->setEnabled(true);
		}
	}

	// Do not allow removing main dataset
	if (IsChecked(0))
	{
		m_RemoveDatasetButton->setEnabled(false);
	}
}

bool MultiDatasetWidget::IsChecked(const int multiDS_index)
{
	if (multiDS_index < m_RadioButtons.size())
	{
		return m_RadioButtons.at(multiDS_index).m_RadioButton->isChecked();
	}
	return false;
}

int MultiDatasetWidget::GetNumberOfDatasets() { return m_RadioButtons.size(); }

bool MultiDatasetWidget::IsActive(const int multiDS_index)
{
	if (multiDS_index < m_RadioButtons.size())
	{
		return m_RadioButtons.at(multiDS_index).m_IsActive;
	}
	return false;
}

std::vector<float*> MultiDatasetWidget::GetBmpData(const int multiDS_index)
{
	if (multiDS_index < m_RadioButtons.size())
	{
		return m_RadioButtons.at(multiDS_index).m_BmpSlices;
	}
	return std::vector<float*>();
}

void MultiDatasetWidget::SetBmpData(const int multiDS_index,
		std::vector<float*> bmp_bits_vc)
{
	if (multiDS_index < m_RadioButtons.size())
	{
		m_RadioButtons.at(multiDS_index).m_BmpSlices = bmp_bits_vc;
	}
}

std::vector<float*> MultiDatasetWidget::GetWorkingData(const int multiDS_index)
{
	if (multiDS_index < m_RadioButtons.size())
	{
		return m_RadioButtons.at(multiDS_index).m_WorkSlices;
	}
	return std::vector<float*>();
}

void MultiDatasetWidget::SetWorkingData(const int multiDS_index,
		std::vector<float*> work_bits_vc)
{
	if (multiDS_index < m_RadioButtons.size())
	{
		m_RadioButtons.at(multiDS_index).m_WorkSlices = work_bits_vc;
	}
}