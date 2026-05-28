/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in
 Society (IT'IS).
 *
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 *
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "Precompiled.h"

#include "DataIteratorWidget.h"
#include "SlicesHandler.h"

#include "../Interface/FormatTooltip.h"
#include "../Interface/LayoutTools.h"

#include "../Core/ImageReader.h"
#include "../Core/itkDICOMOrientImageFilter.h"

#include "../Data/ItkUtils.h"
#include "../Data/SlicesHandlerITKInterface.h"

#include <itkImageFileReader.h>

#include <QCompleter>
#include <QFileSystemModel>
#include <QFormLayout>
#include <QMessageBox>

#include <boost/filesystem.hpp>

#include <algorithm>
#include <sstream>
#include <utility>

namespace iseg {

namespace fs = boost::filesystem;

namespace {
struct Orientation
{
	std::string name;
	eOrientation tag;
};

const std::array<Orientation, 4> _OrientationNames = {
		Orientation{"no change", eOrientation::noChange},
		Orientation{"Axial", eOrientation::RAS},
		Orientation{"Sagittal", eOrientation::PSL},
		Orientation{"Coronal", eOrientation::RSP}};

class FileSystemModel : public QFileSystemModel
{
public:
	FileSystemModel(QObject* parent = nullptr) : QFileSystemModel(parent) {}

	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
	{
		if (role == Qt::DisplayRole && index.column() == 0)
		{
			QString path = QDir::toNativeSeparators(filePath(index));
			if (path.endsWith(QDir::separator()))
				path.chop(1);
			return path;
		}

		return QFileSystemModel::data(index, role);
	}
};
} // namespace

DataIteratorWidget::DataIteratorWidget(iseg::SlicesHandler* hand3D, QWidget* parent,
		Qt::WindowFlags wFlags)
		: QWidget(parent, wFlags), m_Handler3D(hand3D)
{
	setToolTip(Format("Dataset Iterator. Quickly review and edit large dataset."));

	QCompleter* completer = new QCompleter(this);
	completer->setMaxVisibleItems(10);
	auto fs_model = new FileSystemModel(completer);
	fs_model->setRootPath("");
	completer->setModel(fs_model);

	m_ImageDir = new QLineEdit;
	m_ImageDir->setCompleter(completer);

	m_LabelsDir = new QLineEdit;
	m_LabelsDir->setCompleter(completer);

	m_OutputDir = new QLineEdit;
	m_OutputDir->setCompleter(completer);

	m_TargetDir = new QLineEdit;
	m_TargetDir->setCompleter(completer);

	m_Status = new QLabel("none");
	m_CurrentPairEdit = new QLineEdit(QString::number(m_CurrentPair + 1));
	m_CurrentPairEdit->setValidator(new QIntValidator(0, std::numeric_limits<int>::max()));

	m_SaveButton = new QPushButton("Save");
	m_SaveButton->setToolTip("Save local changes to the labels to the 'Processed Output' folder.");
	m_SaveMarksButton = new QPushButton("Save Marks");
	m_SaveMarksButton->setToolTip("Save local changes to the labels to the 'Processed Output' folder.");
	m_NextButton = new QPushButton("Next");
	m_NextButton->setToolTip("Load next image/label pair.");
	m_NextUnprocessedButton = new QPushButton("Next Unprocessed");
	m_NextUnprocessedButton->setToolTip("Load next unprocessed image/label pair.");
	m_LoadProcessed = new QCheckBox("Load Processed");
	m_LoadProcessed->setChecked(true);
	m_LoadProcessed->setToolTip("Load labels from 'Processed Output' if it exists.");

	m_Orientation = new QComboBox;
	for (auto& o : _OrientationNames)
	{
		m_Orientation->addItem(QString::fromStdString(o.name));
	}
	m_Orientation->setCurrentIndex(1);
	m_Orientation->setToolTip("Reload the image/labels in the selected orientation.");

	auto layout = new QFormLayout;
	layout->addRow("Images", m_ImageDir);
	layout->addRow("Labels", m_LabelsDir);
	layout->addRow("Processed Output", m_OutputDir);
	layout->addRow("Target [Optional]", m_TargetDir);
	layout->addRow("Current Dataset", m_CurrentPairEdit);
	layout->addRow("Status", m_Status);
	layout->addRow("Orientation", m_Orientation);
	layout->addRow(m_SaveButton, make_hbox({m_SaveMarksButton, m_NextButton}));
	layout->addRow(m_NextUnprocessedButton, m_LoadProcessed);
	setLayout(layout);

	QObject::connect(m_SaveButton, SIGNAL(clicked()), this, SLOT(Save()));
	QObject::connect(m_SaveMarksButton, SIGNAL(clicked()), this, SLOT(SaveMarks()));
	QObject::connect(m_NextButton, SIGNAL(clicked()), this, SLOT(Next()));
	QObject::connect(m_NextUnprocessedButton, SIGNAL(clicked()), this, SLOT(NextUnprocessed()));
	QObject::connect(m_CurrentPairEdit, SIGNAL(editingFinished()), this, SLOT(CurrentPairChanged()));
	QObject::connect(m_LoadProcessed, SIGNAL(stateChanged(int)), this, SLOT(Reload()));
	QObject::connect(m_Orientation, SIGNAL(currentIndexChanged(int)), this, SLOT(ReloadOrientation()));
}

void DataIteratorWidget::LoadNext(bool skip_processed)
{
	int current_pair = m_CurrentPair;
	// check if folders changed
	if (m_CachedImageDir != m_ImageDir->text().toStdString() || m_CachedLabelsDir != m_LabelsDir->text().toStdString())
	{
		m_CachedImageDir = m_ImageDir->text().toStdString();
		m_CachedLabelsDir = m_LabelsDir->text().toStdString();
		m_ImageLabelPairs.clear();
		m_FileTime.Modified();
		current_pair = -1;

		fs::directory_iterator dir_itr(m_CachedImageDir);
		fs::directory_iterator end_iter;
		for (; dir_itr != end_iter; ++dir_itr)
		{
			fs::path image_path(dir_itr->path());
			fs::path labels_path = fs::path(m_CachedLabelsDir) / image_path.filename();

			const std::vector<std::string> valid_extensions = {".mhd", ".mha", ".nii", ".hdr", ".img", ".gz", ".nrrd"};
			const auto ext = image_path.extension().string();
			boost::system::error_code ec;

			if (std::count(valid_extensions.begin(), valid_extensions.end(), ext) != 0)
			{
				if (fs::exists(labels_path, ec))
				{
					m_ImageLabelPairs.emplace_back(image_path.string(), labels_path.string());
				}
			}
		}
	}

	if (m_ImageLabelPairs.empty())
	{
		m_Status->setText("[0/0] No image/label pairs");
	}
	else if (current_pair + 1 < static_cast<int>(m_ImageLabelPairs.size()))
	{
		current_pair++;

		if (skip_processed)
		{
			const auto output_dir = fs::path(m_OutputDir->text().toStdString());
			auto exist_output = [this, output_dir, &current_pair]() {
				const auto file_name = fs::path(m_ImageLabelPairs.at(current_pair).second).filename();
				const fs::path processed_path = output_dir / file_name;
				boost::system::error_code ec;
				return fs::exists(processed_path, ec);
			};

			while (exist_output() && current_pair + 1 < static_cast<int>(m_ImageLabelPairs.size()))
			{
				current_pair++;
			}
		}
	}

	if (current_pair < static_cast<int>(m_ImageLabelPairs.size()))
	{
		Load(current_pair);
	}
	else
	{
		if (!m_Status->text().contains("(The END)"))
		{
			m_Status->setText(m_Status->text() + " (The END)");
		}
	}
}

void DataIteratorWidget::Load(int pair_idx)
{
	if (m_FileTime < m_Handler3D->ModifyTime())
	{
		const int ret = QMessageBox::warning(
				this, "iSeg", "Detected unsaved modifications. Do you want to save your changes?",
				QMessageBox::Yes | QMessageBox::Default, QMessageBox::No,
				QMessageBox::Cancel | QMessageBox::Escape);

		if (ret == QMessageBox::Yes)
		{
			Save();
		}
		else if (ret == QMessageBox::Cancel)
		{
			return;
		}
	}

	if (pair_idx < static_cast<int>(m_ImageLabelPairs.size()))
	{
		const auto image_path = m_ImageLabelPairs[pair_idx].first;
		const auto labels_path = m_ImageLabelPairs[pair_idx].second;
		const auto processed_path = fs::path(m_OutputDir->text().toStdString()) / fs::path(labels_path).filename();
		const auto target_path = fs::path(m_TargetDir->text().toStdString()) / fs::path(labels_path).filename();
		const auto slice = m_Handler3D->ActiveSlice();

		boost::system::error_code ec;
		if (!fs::exists(image_path) || !fs::exists(labels_path))
		{
			QMessageBox msgBox;
			msgBox.setText("ERROR: image/label files could not be found.");
			msgBox.exec();
			return;
		}

		//
		const auto orientation = static_cast<int>(_OrientationNames.at(m_Orientation->currentIndex()).tag);

		iseg::DataSelection dataSelection;
		dataSelection.allSlices = true;
		dataSelection.bmp = true;
		dataSelection.work = true;
		dataSelection.tissues = true;
		emit BeginDatachange(dataSelection, this, false);

		// read labels
		const auto load_processed = m_LoadProcessed->isChecked() && fs::exists(processed_path, ec);
		m_Handler3D->ReadVolumeOrient(load_processed ? processed_path.string() : labels_path, true, orientation);

		// read target
		if (!m_TargetDir->text().isEmpty() && fs::exists(target_path, ec))
		{
			m_Handler3D->ReadVolumeOrient(target_path.string(), false, orientation);
			m_Handler3D->Bmp2workall();
		}

		// read image
		m_Handler3D->ReadVolumeOrient(image_path, false, orientation);

		emit EndDatachange(this, iseg::ClearUndo);

		// update GUI
		m_Handler3D->SignalVolumeChange(true);

		// try to stay at similar position
		m_Handler3D->SetActiveSlice(slice, true);

		// update gui
		auto file_name = fs::path(labels_path).filename().string();
		auto status = "[" + std::to_string(pair_idx + 1) + "/" + std::to_string(m_ImageLabelPairs.size()) + "] ";
		m_Status->setText((status + file_name).c_str());

		if (!load_processed && fs::exists(processed_path, ec))
		{
			m_Status->setText(m_Status->text() + " Already Processed");
			m_Status->setStyleSheet("QLabel  { color: red; }");
		}
		else
		{
			m_Status->setStyleSheet("");
		}

		m_FileTime.Modified();
		m_CurrentPair = pair_idx; // 'pair_idx' was loaded -> save its index
		m_CurrentPairEdit->setText(QString::number(m_CurrentPair + 1));
	}
}

void DataIteratorWidget::Save()
{
	if (m_CurrentPair >= 0 && m_CurrentPair < static_cast<int>(m_ImageLabelPairs.size()))
	{
		const fs::path output_dir(m_OutputDir->text().toStdString());
		boost::system::error_code ec;
		if (!fs::exists(output_dir, ec))
		{
			fs::create_directory(output_dir, ec);
		}

		const fs::path image_path(m_ImageLabelPairs[m_CurrentPair].first);
		const fs::path output_file_path = output_dir / image_path.filename();

		if (!m_LoadProcessed->isChecked() && fs::exists(output_file_path))
		{
			const int ret = QMessageBox::warning(
					this, "iSeg", "The output file exists. Do you want to overwrite it with your changes?",
					QMessageBox::Yes, QMessageBox::Cancel | QMessageBox::Escape | QMessageBox::Default);

			if (ret == QMessageBox::Cancel)
			{
				ISEG_WARNING("Cancelled saving " << m_CurrentPair + 1);
				return;
			}
		}

		using tissue_image_t = itk::Image<iseg::SlicesHandlerITKInterface::tissue_type, 3>;
		iseg::SlicesHandlerITKInterface itk_handler(m_Handler3D);

		tissue_image_t::Pointer tissues = itk_handler.GetTissuesDeprecated(false);

		const auto orientation = _OrientationNames.at(m_Orientation->currentIndex()).tag;
		if (orientation != eOrientation::noChange)
		{
			// load input orientation
			auto reader = itk::ImageFileReader<tissue_image_t>::New();
			reader->SetFileName(image_path.string());
			reader->UpdateOutputInformation();

			auto image = reader->GetOutput();
			const itk::DICOMOrientation input_orientation(image->GetDirection());

			auto orient = itk::DICOMOrientImageFilter<tissue_image_t>::New();
			orient->SetInput(tissues);
			orient->SetDesiredCoordinateOrientation(input_orientation);
			orient->Update();
			tissues = orient->GetOutput();
		}

		auto writer = itk::ImageFileWriter<tissue_image_t>::New();
		writer->SetInput(tissues);
		writer->SetFileName(output_file_path.string());
		writer->SetUseCompression(true);
		writer->Update();

		m_FileTime.Modified();
		ISEG_INFO("Saved " << output_file_path.string());
	}
	else
	{
		ISEG_ERROR("Invalid data index " << m_CurrentPair + 1);
	}
}

void DataIteratorWidget::SaveMarks()
{
	if (m_CurrentPair >= 0 && m_CurrentPair < static_cast<int>(m_ImageLabelPairs.size()))
	{
		const fs::path output_dir(m_OutputDir->text().toStdString());
		boost::system::error_code ec;
		if (!fs::exists(output_dir, ec))
		{
			fs::create_directory(output_dir, ec);
		}

		const fs::path image_path(m_ImageLabelPairs[m_CurrentPair].first);
		const fs::path marks_file_path = output_dir / image_path.stem().concat(".csv");

		m_Handler3D->ExportMarkers(marks_file_path.string().c_str());

		ISEG_INFO("Saved marks " << marks_file_path.string());
	}
	else
	{
		ISEG_ERROR("Invalid data index " << m_CurrentPair + 1);
	}
}

void DataIteratorWidget::Next()
{
	LoadNext(false);
}

void DataIteratorWidget::NextUnprocessed()
{
	LoadNext(true);
}

void DataIteratorWidget::CurrentPairChanged()
{
	bool ok = false;
	const int new_idx = m_CurrentPairEdit->text().toInt(&ok) - 1;
	if (ok && static_cast<size_t>(new_idx) < m_ImageLabelPairs.size())
	{
		Load(new_idx);
	}
	else
	{
		ISEG_ERROR("Invalid index: " << m_CurrentPairEdit->text().toStdString());
		m_CurrentPairEdit->setText(QString::number(m_CurrentPair + 1));
	}
}

void DataIteratorWidget::Reload()
{
	Load(m_CurrentPair);
}

void DataIteratorWidget::ReloadOrientation()
{
	const auto temp_dir = fs::temp_directory_path();
	auto pts_file_path = temp_dir / "marks.pts";
	m_Handler3D->ExportMarkers(pts_file_path.string().c_str());

	Load(m_CurrentPair);

	m_Handler3D->ImportMarkers(pts_file_path.string().c_str());
	fs::remove(pts_file_path);
}

} // namespace iseg
