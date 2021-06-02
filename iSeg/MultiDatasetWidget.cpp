/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "Precompiled.h"

#include "LoaderWidgets.h"
#include "MultiDatasetWidget.h"
#include "SlicesHandler.h"

#include "../Interface/RecentPlaces.h"

#include <itkImageFileReader.h>
#include <itkImageSeriesReader.h>
#include <itkRawImageIO.h>
#include <itkResampleImageFilter.h>

#include <Q3HBoxLayout>
#include <Q3VBoxLayout>
#include <QFileDialog>
#include <QGroupBox>

namespace iseg {

MultiDatasetWidget::MultiDatasetWidget(SlicesHandler* hand3D, QWidget* parent, Qt::WindowFlags wFlags)
		: QWidget(parent, wFlags), m_Handler3D(hand3D)
{
	m_HboxOverall = new Q3HBoxLayout(this);
	m_VboxOverall = new Q3VBoxLayout();
	m_HboxOverall->addLayout(m_VboxOverall);

	// Add dataset button
	m_AddDatasetButton = new QPushButton("Add Dataset...", this);
	m_VboxOverall->addWidget(m_AddDatasetButton);

	// Dataset selection group box
	m_DatasetsGroupBox = new QGroupBox("- Available datasets -");
	m_VboxDatasets = new Q3VBoxLayout(m_DatasetsGroupBox);
	m_VboxDatasets->addStretch(1);
	m_DatasetsGroupBox->setLayout(m_VboxDatasets);
	m_VboxOverall->addWidget(m_DatasetsGroupBox);

	//Buttons
	QHBoxLayout* buttons_grid = new QHBoxLayout();
	m_VboxOverall->addLayout(buttons_grid);

	// Add dataset button
	m_LoadDatasetButton = new QPushButton("Load Dataset", this);
	buttons_grid->addWidget(m_LoadDatasetButton);

	m_ChangeNameButton = new QPushButton("Change Name", this);
	buttons_grid->addWidget(m_ChangeNameButton);

	m_RemoveDatasetButton = new QPushButton("Remove Dataset", this);
	buttons_grid->addWidget(m_RemoveDatasetButton);
	m_RemoveDatasetButton->setEnabled(false);

	m_DatasetsGroupBox->setMinimumHeight(200);
	setFixedHeight(m_HboxOverall->sizeHint().height());

	QObject_connect(m_AddDatasetButton, SIGNAL(clicked()), this, SLOT(AddDatasetPressed()));
	QObject_connect(m_LoadDatasetButton, SIGNAL(clicked()), this, SLOT(SwitchDataset()));
	QObject_connect(m_ChangeNameButton, SIGNAL(clicked()), this, SLOT(ChangeDatasetName()));
	QObject_connect(m_RemoveDatasetButton, SIGNAL(clicked()), this, SLOT(RemoveDataset()));
	QObject_connect(m_DatasetsGroupBox, SIGNAL(clicked()), this, SLOT(DatasetSelectionChanged()));

	Initialize();
}

MultiDatasetWidget::~MultiDatasetWidget() { m_RadioButtons.clear(); }

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

	const unsigned short w_loaded = m_Handler3D->Width();
	const unsigned short h_loaded = m_Handler3D->Height();
	const unsigned short nrofslices_loaded = m_Handler3D->NumSlices();

	const bool check_match = true;
	if (check_match)
	{
		if (w_loaded == 512 && h_loaded == 512 && nrofslices_loaded == 1)
		{
			// empty default image
			return;
		}
	}

	SDatasetInfo new_radio_button;
	// Create the copy of the main dataset only when adding a second dataset
	//CopyImagesSlices(m_Handler3D, newRadioButton);

	QStringList paths;
	paths.append("main_Dataset");
	AddDatasetToList(new_radio_button, paths);

	QFont font(new_radio_button.m_RadioButton->font());
	font.setBold(true);
	new_radio_button.m_RadioButton->setFont(font);
	new_radio_button.m_RadioButton->setChecked(true);
	new_radio_button.m_IsActive = true;

	m_RadioButtons.push_back(new_radio_button);
}

void MultiDatasetWidget::AddDatasetPressed()
{
	SupportedMultiDatasetTypes dlg;
	dlg.exec();

	int selected_type = dlg.GetSelectedType();

	if (selected_type != -1)
	{
		using image_type = itk::Image<float, 3>;
		image_type::Pointer image;

		QStringList loadfilenames;
		MultiDatasetWidget::SDatasetInfo data_info;
		data_info.m_IsActive = false;

		switch (SupportedMultiDatasetTypes::eSupportedTypes(selected_type))
		{
		case SupportedMultiDatasetTypes::eSupportedTypes::bmp:
		case SupportedMultiDatasetTypes::eSupportedTypes::dcm: {
			loadfilenames = QFileDialog::getOpenFileNames(
					"Images (*.dcm *.dicom *.bmp)\n"
					"All (*)",
					QString::null, this, "Open Files", "Select one or more files to open");

			std::vector<std::string> files;
			for (auto it=loadfilenames.begin(); it!=loadfilenames.end(); ++it)
			{
				files.push_back(it->toStdString());
			}
			auto reader = itk::ImageSeriesReader<image_type>::New();
			reader->SetFileNames(files);
			try
			{
				reader->Update();

				image = reader->GetOutput();
			}
			catch (itk::ExceptionObject& e)
			{
				ISEG_ERROR("Failed to load file series: " << e.what());
			}
		}
		break;

		case SupportedMultiDatasetTypes::eSupportedTypes::raw: {

			LoaderRaw lr(nullptr, this);
			lr.SetSkipReading(true);
			lr.move(QCursor::pos());
			lr.exec();

			QString loadfilename = lr.GetFileName();
			auto dims = lr.GetDimensions();
			auto start = lr.GetSubregionStart();
			auto size = lr.GetSubregionSize();
			int bits = lr.GetBits();

			loadfilenames.append(loadfilename);

			auto region = itk::ImageIORegion(3);
			region.SetIndex(std::vector<itk::IndexValueType>(start.begin(), start.end()));
			region.SetSize(std::vector<itk::SizeValueType>(size.begin(), size.end()));

			itk::ImageIOBase::Pointer rawio;
			if (bits == 8)
			{
				auto uchar_rawio = itk::RawImageIO<unsigned char, 3>::New();
				uchar_rawio->SetHeaderSize(0);
				rawio = uchar_rawio;
			}
			else //if (bits == 16)
			{
				auto ushort_rawio = itk::RawImageIO<unsigned short, 3>::New();
				ushort_rawio->SetHeaderSize(0);
				rawio = ushort_rawio;
			}
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
			}
			catch (itk::ExceptionObject& e)
			{
				ISEG_ERROR("Failed to load raw file: " << e.what());
			}
		}
		break;

		case SupportedMultiDatasetTypes::eSupportedTypes::nifti:
		case SupportedMultiDatasetTypes::eSupportedTypes::vtk: {
			auto loadfilename = RecentPlaces::GetOpenFileName(this, "Open File", "", "Images (*.vti *.vtk *.nii *.nii.gz *.hdr *.img *.nia)");
			if (!loadfilename.isEmpty())
			{
				auto reader = itk::ImageFileReader<image_type>::New();
				reader->SetFileName(loadfilename.ascii());
				try
				{
					reader->Update();

					image = reader->GetOutput();

					loadfilenames.append(loadfilename);
				}
				catch (itk::ExceptionObject& e)
				{
					ISEG_ERROR("Failed to load file " << loadfilename.toStdString() << " :" << e.what());
				}
			}
		}
		break;

		default: break;
		}

		bool success = false;
		if (image)
		{
			auto dims = image->GetBufferedRegion().GetSize();

			success = (m_Handler3D->Width() == dims[0] && m_Handler3D->Height() == dims[1] && m_Handler3D->NumSlices() == dims[2]);
			if (!success)
			{
				QMessageBox msg_box(QMessageBox::Warning, "", "The image dimensions do not match. Do you want to resample the new dataset?", QMessageBox::Yes | QMessageBox::No);
				msg_box.setDefaultButton(QMessageBox::No);
				int ret = msg_box.exec();

				if (ret == QMessageBox::Yes)
				{
					itk::Size<3> size = {m_Handler3D->Width(), m_Handler3D->Height(), m_Handler3D->NumSlices()};
					double spacing[3] = {m_Handler3D->Spacing().x, m_Handler3D->Spacing().y, m_Handler3D->Spacing().z};
					auto transform = m_Handler3D->ImageTransform();

					itk::Point<itk::SpacePrecisionType, 3> origin;
					transform.GetOffset(origin);

					itk::Matrix<itk::SpacePrecisionType, 3, 3> direction;
					transform.GetRotation(direction);

					try
					{
						auto resample = itk::ResampleImageFilter<image_type, image_type>::New();
						resample->SetInput(image);
						resample->SetSize(size);
						resample->SetOutputOrigin(origin);
						resample->SetOutputSpacing(spacing);
						resample->SetOutputDirection(direction);
						resample->SetDefaultPixelValue(0);
						resample->Update();

						image = resample->GetOutput();

						success = true;
					}
					catch (itk::ExceptionObject)
					{
					}
				}
			}
		}

		if (success)
		{
			// add to available datasets
			AddDatasetToList(data_info, loadfilenames);

			std::array<size_t, 3> dims = {m_Handler3D->Width(), m_Handler3D->Height(), m_Handler3D->NumSlices()};

			// create the copy of the main dataset only when adding a second dataset
			if (m_RadioButtons.size() == 1)
			{
				const SlicesHandler* chandler = m_Handler3D;
				CopyImagesSlices(chandler->SourceSlices(), dims, m_RadioButtons.at(0));
			}

			// copy image to slices
			auto buffer = image->GetBufferPointer();
			auto slice_size = dims[0] * dims[1];
			std::vector<const float*> bmp_slices(dims[2], nullptr);
			for (unsigned i = 0; i < dims[2]; ++i)
			{
				bmp_slices[i] = buffer + i * slice_size;
			}

			CopyImagesSlices(bmp_slices, dims, data_info);
			m_RadioButtons.push_back(data_info);
		}
	}
}

bool MultiDatasetWidget::CheckInfoAndAddToList(MultiDatasetWidget::SDatasetInfo& newRadioButton, QStringList loadfilenames, unsigned short width, unsigned short height, unsigned short nrofslices)
{
	// check whether the new dataset matches the dataset loaded
	const unsigned short w_loaded = m_Handler3D->Width();
	const unsigned short h_loaded = m_Handler3D->Height();
	const unsigned short nrofslices_loaded = m_Handler3D->NumSlices();

	if (w_loaded == width && h_loaded == height && nrofslices_loaded == nrofslices)
	{
		return AddDatasetToList(newRadioButton, loadfilenames);
	}
	else
	{
		QMessageBox mb("Loading failed", "The resolution of the dataset must match the one loaded.", QMessageBox::Critical, QMessageBox::Ok | QMessageBox::Default, QMessageBox::NoButton, QMessageBox::NoButton);
		mb.exec();
	}

	return false;
}

bool MultiDatasetWidget::AddDatasetToList(MultiDatasetWidget::SDatasetInfo& newRadioButton, QStringList loadfilenames)
{
	QString but_text = loadfilenames[0];
	newRadioButton.m_RadioButton = new QRadioButton(QFileInfo(loadfilenames[0]).fileName());
	newRadioButton.m_DatasetFilepath = loadfilenames;
	newRadioButton.m_RadioButtonText = but_text;

	m_VboxDatasets->addWidget(newRadioButton.m_RadioButton, 0, Qt::AlignTop);

	QObject_connect(newRadioButton.m_RadioButton, SIGNAL(clicked()), this, SLOT(DatasetSelectionChanged()));

	return !newRadioButton.m_DatasetFilepath.empty();
}

void MultiDatasetWidget::CopyImagesSlices(const std::vector<const float*>& bmp_slices, const std::array<size_t, 3>& dims, MultiDatasetWidget::SDatasetInfo& newRadioButton)
{
	const size_t nrslices = dims[2];
	const size_t width = dims[0];
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
}

void MultiDatasetWidget::SwitchDataset()
{
	for (auto& radio_button : m_RadioButtons)
	{
		if (radio_button.m_RadioButton->isChecked())
		{
			QFont font(radio_button.m_RadioButton->font());
			font.setBold(true);
			radio_button.m_RadioButton->setFont(font);

			if (!radio_button.m_BmpSlices.empty())
			{
				DataSelection data_selection;
				data_selection.allSlices = true;
				data_selection.bmp = true;
				data_selection.work = false;
				data_selection.tissues = false;
				emit BeginDatachange(data_selection, this, false);

				const int n_slices = radio_button.m_BmpSlices.size();
				assert(radio_button.m_BmpSlices.size() == m_Handler3D->NumSlices());

				for (int i = 0; i < n_slices; i++)
				{
					m_Handler3D->Copy2bmp(i, radio_button.m_BmpSlices.at(i), 1);
				}

				m_ItIsBeingLoaded = true;
				radio_button.m_IsActive = true;

				emit EndDatachange(this, iseg::ClearUndo);
				emit DatasetChanged();
			}
			continue;
		}
		else
		{
			QFont font(radio_button.m_RadioButton->font());
			font.setBold(false);
			radio_button.m_RadioButton->setFont(font);
			radio_button.m_IsActive = false;
		}
	}

	DatasetSelectionChanged();
}

void MultiDatasetWidget::ChangeDatasetName()
{
	for (auto& radio_button : m_RadioButtons)
	{
		if (radio_button.m_RadioButton->isChecked())
		{
			EditText edit_text_dlg(this->parentWidget(), this->windowFlags());
			edit_text_dlg.SetEditableText(radio_button.m_RadioButton->text());
			if (edit_text_dlg.exec())
			{
				QString returning_text = edit_text_dlg.GetEditableText();
				radio_button.m_RadioButtonText = returning_text;
				radio_button.m_RadioButton->setText(returning_text);
			}
			break;
		}
	}
}

void MultiDatasetWidget::RemoveDataset()
{
	int index = 0;
	for (auto& radio_button : m_RadioButtons)
	{
		if (radio_button.m_RadioButton->isChecked())
		{
			m_VboxDatasets->removeWidget(radio_button.m_RadioButton);

			std::for_each(radio_button.m_BmpSlices.begin(), radio_button.m_BmpSlices.end(), [](float* element) { free(element); });
			radio_button.m_BmpSlices.clear();

			delete radio_button.m_RadioButton;

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
			break;
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

void MultiDatasetWidget::SetBmpData(const int multiDS_index, std::vector<float*> bmp_bits_vc)
{
	if (multiDS_index < m_RadioButtons.size())
	{
		m_RadioButtons.at(multiDS_index).m_BmpSlices = bmp_bits_vc;
	}
}

} // namespace iseg
