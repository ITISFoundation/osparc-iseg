/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "TissueSeparatorWidget.h"

#include "GraphCutAlgorithms.h"

#include "Data/LogApi.h"
#include "Data/ItkUtils.h"
#include "Data/addLine.h"
#include "Data/SliceHandlerItkWrapper.h"

#include <itkBinaryThresholdImageFilter.h>

#include <QApplication>
#include <QFormLayout>
#include <QProgressDialog>
#include <QStyleFactory>

#include <algorithm>
#include <sstream>

using namespace iseg;

TissueSeparatorWidget::TissueSeparatorWidget(
		iseg::SliceHandlerInterface* hand3D, QWidget* parent, const char* name,
		Qt::WindowFlags wFlags)
		: WidgetInterface(parent, name, wFlags), slice_handler(hand3D)
{
	current_slice = slice_handler->active_slice();

	// create options
	all_slices = new QCheckBox;
	use_source = new QCheckBox;
	use_source->setToolTip(QString("Use information from Source image, or split purely based on minimum cut through segmentation."));
	sigma_edit = new QLineEdit(QString::number(1.0));
	clear_lines = new QPushButton("Clear lines");
	execute_button = new QPushButton("Execute");

	// setup layout
	auto top_layout = new QFormLayout;
	// add options ?
	top_layout->addRow(QString("Apply to all slices"), all_slices);
	top_layout->addRow(QString("Use source"), use_source);
	top_layout->addRow(QString("Sigma"), sigma_edit);
	top_layout->addRow(clear_lines);
	top_layout->addRow(execute_button);

	setLayout(top_layout);

	// connect signals
	QObject::connect(clear_lines, SIGNAL(clicked()), this, SLOT(clearmarks()));
	QObject::connect(execute_button, SIGNAL(clicked()), this, SLOT(execute()));
}

void TissueSeparatorWidget::init()
{
	on_slicenr_changed();
	hideparams_changed();
}

void TissueSeparatorWidget::newloaded()
{
	current_slice = slice_handler->active_slice();
}

void TissueSeparatorWidget::cleanup()
{
	vpdyn.clear();
	emit vpdyn_changed(&vpdyn);

	std::vector<iseg::Mark> vmempty;
	emit vm_changed(&vmempty);
}

void TissueSeparatorWidget::clearmarks()
{
	vpdyn.clear();
	vm.clear();
	//bmphand->clear_vvm();  \todo BL is this needed?
	emit vpdyn_changed(&vpdyn);

	std::vector<Mark> dummy;
	emit vm_changed(&dummy);
}

void TissueSeparatorWidget::on_tissuenr_changed(int i)
{
	tissuenr = (unsigned)i + 1;
}

void TissueSeparatorWidget::on_slicenr_changed()
{
	current_slice = slice_handler->active_slice();

	auto& vm_slice = vm[current_slice];
	emit vm_changed(&vm_slice);
}

void TissueSeparatorWidget::on_mouse_clicked(Point p)
{
	last_pt = p;
}

void TissueSeparatorWidget::on_mouse_moved(Point p)
{
	addLine(&vpdyn, last_pt, p);
	last_pt = p;
	emit vpdyn_changed(&vpdyn);
}

void TissueSeparatorWidget::on_mouse_released(Point p)
{
	addLine(&vpdyn, last_pt, p);
	Mark m(tissuenr);
	std::vector<Mark> vmdummy;
	for (auto& p : vpdyn)
	{
		m.p = p;
		vmdummy.push_back(m);

		// \note I could write directly into image, e.g. for automatic update
	}
	auto& vm_slice = vm[current_slice];
	vm_slice.insert(vm_slice.end(), vmdummy.begin(), vmdummy.end());
	vpdyn.clear();

	std::set<unsigned> labels;
	for (auto& m : vm_slice)
	{
		labels.insert(m.mark);
	}
	bool const auto_update = (!all_slices->isChecked()) && labels.size() >= 2;

	iseg::DataSelection dataSelection;
	dataSelection.sliceNr = slice_handler->active_slice();
	dataSelection.work = auto_update;
	dataSelection.vvm = true;
	emit begin_datachange(dataSelection, this);
	emit vpdyn_changed(&vpdyn);
	emit vm_changed(&vm_slice);

	// run 2d (current slice) algorithm
	if (auto_update)
	{
		do_work_current_slice();
	}

	emit end_datachange(this);
}

void TissueSeparatorWidget::execute()
{
	if (all_slices->isChecked())
	{
		do_work_all_slices();
	}
	else
	{
		do_work_current_slice();
	}
}

void TissueSeparatorWidget::do_work_all_slices()
{
	QProgressDialog progress("Running Graph Cut...", "Abort", 0, 0, this);
	progress.setCancelButton(0);
	progress.setMinimum(0);
	progress.setMaximum(0);
	progress.setWindowModality(Qt::WindowModal);
	//progress.setStyle(QStyleFactory::create("Fusion"));
	progress.show();

	QApplication::processEvents();

	using source_type = itk::SliceContiguousImage<float>;
	using mask_type = itk::Image<unsigned char, 3>;

	SliceHandlerItkWrapper wrapper(slice_handler);
	auto source = wrapper.GetSource(false);
	auto target = wrapper.GetTarget(false);

	auto start = target->GetLargestPossibleRegion().GetIndex();
	start[2] = slice_handler->start_slice();

	auto size = target->GetLargestPossibleRegion().GetSize();
	size[2] = slice_handler->end_slice() - start[2];

	auto output = do_work<3, source_type>(source, target, mask_type::RegionType(start, size));
	if (output)
	{
		iseg::DataSelection dataSelection;
		dataSelection.allSlices = true;
		dataSelection.work = true;
		emit begin_datachange(dataSelection, this);

		iseg::Paste<unsigned char, float>(output, target, slice_handler->start_slice(), slice_handler->end_slice());

		emit end_datachange(this);
	}
	else
	{
		std::cerr << "No result. GC failed.\n";
	}

	progress.setMaximum(1);
	progress.setValue(1);
}

void TissueSeparatorWidget::do_work_current_slice()
{
	using source_type = itk::Image<float, 2>;

	SliceHandlerItkWrapper wrapper(slice_handler);
	auto source = wrapper.GetSourceSlice();
	auto target = wrapper.GetTargetSlice();

	auto output = do_work<2, source_type>(source, target, target->GetLargestPossibleRegion());
	if (output)
	{
		auto buffer_size = target->GetPixelContainer()->Size();
		auto target_buffer = target->GetPixelContainer()->GetImportPointer();
		auto output_buffer = output->GetPixelContainer()->GetImportPointer();
		std::transform(output_buffer, output_buffer + buffer_size, target_buffer,
				[](unsigned char v) { return static_cast<float>(v); });

		std::cerr << "Computed GC in 2D\n";
	}
}

template<unsigned int Dim, typename TInput>
typename itk::Image<unsigned char, Dim>::Pointer
		TissueSeparatorWidget::do_work(TInput* source, TInput* target, const typename itk::Image<unsigned char, Dim>::RegionType& requested_region)
{
	using tissue_value_type = SliceHandlerInterface::tissue_type;
	tissue_value_type const OBJECT_1 = 127;
	tissue_value_type const OBJECT_2 = 255;
	bool has_sigma;
	auto sigma = sigma_edit->text().toDouble(&has_sigma);
	bool use_gradient_magnitude = use_source->isChecked();
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
	size_t const width = slice_handler->width();
	size_t const height = slice_handler->width();

	unsigned first_mark = -1;
	bool found_other_mark = false;
	for (auto slice_marks : vm)
	{
		auto slice = slice_marks.first;
		if (slice == current_slice || Dim == 3)
		{
			size_t const zoffset = (Dim == 3) ? slice * width * height : 0;
			for (auto& m : slice_marks.second)
			{
				if (first_mark == -1)
				{
					first_mark = m.mark;
				}
				mask_buffer[m.p.px + m.p.py * width + zoffset] = (m.mark == first_mark) ? OBJECT_1 : OBJECT_2;
				found_other_mark = found_other_mark || (m.mark != first_mark);
			}
		}
	}
	std::cerr << "Found other mark: " << found_other_mark << "\n";

	auto cutter = gc_filter_type::New();
	cutter->SetBackgroundValue(0);
	cutter->SetObject1Value(OBJECT_1);
	cutter->SetObject2Value(OBJECT_2);
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
		iseg::Log::error(e.what());
	}
	catch (std::exception e)
	{
		iseg::Log::error(e.what());
	}
	return nullptr;
}
