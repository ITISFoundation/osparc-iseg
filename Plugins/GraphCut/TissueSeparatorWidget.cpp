/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "TissueSeparatorWidget.h"

#include "GraphCutAlgorithms.h"

#include "Data/ItkUtils.h"
#include "Data/LogApi.h"
#include "Data/SlicesHandlerITKInterface.h"
#include "Data/addLine.h"

#include <itkBinaryThresholdImageFilter.h>

#include <QApplication>
#include <QFormLayout>
#include <QProgressDialog>
#include <QStyleFactory>

#include <algorithm>
#include <sstream>

namespace iseg {

TissueSeparatorWidget::TissueSeparatorWidget(SlicesHandlerInterface* hand3D, QWidget* parent, Qt::WindowFlags wFlags)
		: m_SliceHandler(hand3D)
{
	m_CurrentSlice = m_SliceHandler->ActiveSlice();

	// create options
	m_AllSlices = new QCheckBox;
	m_UseSource = new QCheckBox;
	m_UseSource->setToolTip(QString("Use information from Source image, or split purely based on minimum cut through segmentation."));
	m_SigmaEdit = new QLineEdit(QString::number(1.0));
	m_ClearLines = new QPushButton("Clear lines");
	m_ExecuteButton = new QPushButton("Execute");

	// setup layout
	auto top_layout = new QFormLayout;
	// add options ?
	top_layout->addRow(QString("Apply to all slices"), m_AllSlices);
	top_layout->addRow(QString("Use source"), m_UseSource);
	top_layout->addRow(QString("Sigma"), m_SigmaEdit);
	top_layout->addRow(m_ClearLines);
	top_layout->addRow(m_ExecuteButton);

	setLayout(top_layout);

	// connect signals
	QObject_connect(m_ClearLines, SIGNAL(clicked()), this, SLOT(Clearmarks()));
	QObject_connect(m_ExecuteButton, SIGNAL(clicked()), this, SLOT(Execute()));
}

void TissueSeparatorWidget::Init()
{
	OnSlicenrChanged();
	HideParamsChanged();
}

void TissueSeparatorWidget::NewLoaded()
{
	m_CurrentSlice = m_SliceHandler->ActiveSlice();
}

void TissueSeparatorWidget::Cleanup()
{
	m_Vpdyn.clear();
	emit VpdynChanged(&m_Vpdyn);

	std::vector<iseg::Mark> vmempty;
	emit VmChanged(&vmempty);
}

void TissueSeparatorWidget::Clearmarks()
{
	m_Vpdyn.clear();
	m_Vm.clear();
	//bmphand->clear_vvm();  \todo BL is this needed?
	emit VpdynChanged(&m_Vpdyn);

	std::vector<Mark> dummy;
	emit VmChanged(&dummy);
}

void TissueSeparatorWidget::OnTissuenrChanged(int i)
{
	m_Tissuenr = (unsigned)i + 1;
}

void TissueSeparatorWidget::OnSlicenrChanged()
{
	m_CurrentSlice = m_SliceHandler->ActiveSlice();

	auto& vm_slice = m_Vm[m_CurrentSlice];
	emit VmChanged(&vm_slice);
}

void TissueSeparatorWidget::OnMouseClicked(Point p)
{
	m_LastPt = p;
}

void TissueSeparatorWidget::OnMouseMoved(Point p)
{
	addLine(&m_Vpdyn, m_LastPt, p);
	m_LastPt = p;
	emit VpdynChanged(&m_Vpdyn);
}

void TissueSeparatorWidget::OnMouseReleased(Point p)
{
	addLine(&m_Vpdyn, m_LastPt, p);
	Mark m(m_Tissuenr);
	std::vector<Mark> vmdummy;
	for (auto& p : m_Vpdyn)
	{
		m.p = p;
		vmdummy.push_back(m);

		// \note I could write directly into image, e.g. for automatic update
	}
	auto& vm_slice = m_Vm[m_CurrentSlice];
	vm_slice.insert(vm_slice.end(), vmdummy.begin(), vmdummy.end());
	m_Vpdyn.clear();

	std::set<unsigned> labels;
	for (auto& m : vm_slice)
	{
		labels.insert(m.mark);
	}
	bool const auto_update = (!m_AllSlices->isChecked()) && labels.size() >= 2;

	iseg::DataSelection data_selection;
	data_selection.sliceNr = m_SliceHandler->ActiveSlice();
	data_selection.work = auto_update;
	data_selection.vvm = true;
	emit BeginDatachange(data_selection, this);
	emit VpdynChanged(&m_Vpdyn);
	emit VmChanged(&vm_slice);

	// run 2d (current slice) algorithm
	if (auto_update)
	{
		DoWorkCurrentSlice();
	}

	emit EndDatachange(this);
}

void TissueSeparatorWidget::Execute()
{
	if (m_AllSlices->isChecked())
	{
		DoWorkAllSlices();
	}
	else
	{
		DoWorkCurrentSlice();
	}
}

void TissueSeparatorWidget::DoWorkAllSlices()
{
	QProgressDialog progress("Running Graph Cut...", "Abort", 0, 0, this);
	progress.setCancelButton(nullptr);
	progress.setMinimum(0);
	progress.setMaximum(0);
	progress.setWindowModality(Qt::WindowModal);
	//progress.setStyle(QStyleFactory::create("Fusion"));
	progress.show();

	QApplication::processEvents();

	using source_type = itk::SliceContiguousImage<float>;
	using mask_type = itk::Image<unsigned char, 3>;

	SlicesHandlerITKInterface wrapper(m_SliceHandler);
	auto source = wrapper.GetSource(false);
	auto target = wrapper.GetTarget(false);

	auto start = target->GetLargestPossibleRegion().GetIndex();
	start[2] = m_SliceHandler->StartSlice();

	auto size = target->GetLargestPossibleRegion().GetSize();
	size[2] = m_SliceHandler->EndSlice() - start[2];

	auto output = DoWork<3, source_type>(source, target, mask_type::RegionType(start, size));
	if (output)
	{
		iseg::DataSelection data_selection;
		data_selection.allSlices = true;
		data_selection.work = true;
		emit BeginDatachange(data_selection, this);

		iseg::Paste<unsigned char, float>(output, target, m_SliceHandler->StartSlice(), m_SliceHandler->EndSlice());

		emit EndDatachange(this);
	}
	else
	{
		std::cerr << "No result. GC failed.\n";
	}

	progress.setMaximum(1);
	progress.setValue(1);
}

void TissueSeparatorWidget::DoWorkCurrentSlice()
{
	using source_type = itk::Image<float, 2>;

	SlicesHandlerITKInterface wrapper(m_SliceHandler);
	auto source = wrapper.GetSourceSlice();
	auto target = wrapper.GetTargetSlice();

	auto output = DoWork<2, source_type>(source, target, target->GetLargestPossibleRegion());
	if (output)
	{
		auto buffer_size = target->GetPixelContainer()->Size();
		auto target_buffer = target->GetPixelContainer()->GetImportPointer();
		auto output_buffer = output->GetPixelContainer()->GetImportPointer();
		std::transform(output_buffer, output_buffer + buffer_size, target_buffer, [](unsigned char v) { return static_cast<float>(v); });

		std::cerr << "Computed GC in 2D\n";
	}
}

template<unsigned int Dim, typename TInput>
typename itk::Image<unsigned char, Dim>::Pointer
		TissueSeparatorWidget::DoWork(TInput* source, TInput* target, const typename itk::Image<unsigned char, Dim>::RegionType& requested_region)
{
	using tissue_value_type = SlicesHandlerInterface::tissue_type;
	tissue_value_type const object_1 = 127;
	tissue_value_type const object_2 = 255;
	bool has_sigma;
	auto sigma = m_SigmaEdit->text().toDouble(&has_sigma);
	bool use_gradient_magnitude = m_UseSource->isChecked();
	bool use_full_neighborhood = true;

	using source_type = TInput;
	using mask_type = itk::Image<unsigned char, Dim>;
	using gc_filter_type = itk::GraphCutLabelSeparator<mask_type, mask_type, source_type>;

	// create mask from selected tissue [use current selection, or initial selection?]
	auto threshold = itk::BinaryThresholdImageFilter<source_type, mask_type>::New();
	threshold->SetInput(target);
	threshold->SetLowerThreshold(0.001f);
	threshold->SetInsideValue(1);
	threshold->SetOutsideValue(0);
	threshold->Update();

	auto mask = threshold->GetOutput();
	auto mask_buffer = mask->GetPixelContainer()->GetImportPointer();
	size_t const width = m_SliceHandler->Width();
	size_t const height = m_SliceHandler->Width();

	unsigned first_mark = -1;
	bool found_other_mark = false;
	for (const auto& slice_marks : m_Vm)
	{
		auto slice = slice_marks.first;
		if (slice == m_CurrentSlice || Dim == 3)
		{
			size_t const zoffset = (Dim == 3) ? slice * width * height : 0;
			for (auto& m : slice_marks.second)
			{
				if (first_mark == -1)
				{
					first_mark = m.mark;
				}
				mask_buffer[m.p.px + m.p.py * width + zoffset] = (m.mark == first_mark) ? object_1 : object_2;
				found_other_mark = found_other_mark || (m.mark != first_mark);
			}
		}
	}
	std::cerr << "Found other mark: " << found_other_mark << "\n";

	auto cutter = gc_filter_type::New();
	cutter->SetBackgroundValue(0);
	cutter->SetObject1Value(object_1);
	cutter->SetObject2Value(object_2);
	cutter->SetVerboseOutput(true);
	cutter->SetUseGradientMagnitude(use_gradient_magnitude);
	if (has_sigma)
	{
		cutter->SetSigma(sigma);
	}
	cutter->SetConnectivity(use_full_neighborhood ? itk::eGcConnectivity::kNodeNeighbors : itk::eGcConnectivity::kFaceNeighbors);
	cutter->SetMaskInput(mask);
	if (use_gradient_magnitude)
	{
		cutter->SetIntensityInput(source);
	}

	try
	{
		cutter->GetOutput()->SetRequestedRegion(requested_region);
		cutter->Update();
		std::cerr << "TissueSeparator finished\n";
		return cutter->GetOutput();
	}
	catch (itk::ExceptionObject e)
	{
		iseg::Log::Error(e.what());
	}
	catch (std::exception e)
	{
		iseg::Log::Error(e.what());
	}
	return nullptr;
}

} // namespace iseg
