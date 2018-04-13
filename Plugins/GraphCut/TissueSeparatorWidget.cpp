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

#include "Interface/ItkUtils.h"
#include "Interface/addLine.h"

#include <itkBinaryThresholdImageFilter.h>
#include <itkImageFileWriter.h>

#include <QFormLayout>

#include <algorithm>
#include <sstream>

using namespace iseg;

TissueSeparatorWidget::TissueSeparatorWidget(
		iseg::SliceHandlerInterface* hand3D, QWidget* parent, const char* name,
		Qt::WindowFlags wFlags)
		: WidgetInterface(parent, name, wFlags), slice_handler(hand3D)
{
	current_slice = slice_handler->get_activeslice();

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
	QObject::connect(execute_button, SIGNAL(clicked()), this, SLOT(do_work()));
}

void TissueSeparatorWidget::init()
{
	on_slicenr_changed();
	hideparams_changed();
}

void TissueSeparatorWidget::newloaded()
{
	current_slice = slice_handler->get_activeslice();
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
	emit vm_changed(&vm);
}

void TissueSeparatorWidget::on_tissuenr_changed(int i)
{
	tissuenr = (unsigned)i + 1;
}

void TissueSeparatorWidget::on_slicenr_changed()
{
	current_slice = slice_handler->get_activeslice();
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

		// \write directly into image, e.g. for automatic update
		//lbmap[p.px + width * p.py] = tissuenr;
	}
	vm.insert(vm.end(), vmdummy.begin(), vmdummy.end());
	vpdyn.clear();

	iseg::DataSelection dataSelection;
	dataSelection.sliceNr = slice_handler->get_activeslice();
	dataSelection.work = false;
	dataSelection.vvm = true;
	emit begin_datachange(dataSelection, this);
	emit vpdyn_changed(&vpdyn);
	emit vm_changed(&vm);

	// here run algorithm

	emit end_datachange(this);
}

namespace {

template<typename TImage>
void dump_image(TImage* img, const std::string& file_path)
{
#if 0
	auto writer = itk::ImageFileWriter<TImage>::New();
	writer->SetInput(img);
	writer->SetFileName(file_path);
	writer->Update();
#endif
}

} // namespace

void TissueSeparatorWidget::do_work() //Code for special GC for division
{
	using tissue_value_type = SliceHandlerInterface::tissue_type;
	using tissue_type = itk::SliceContiguousImage<tissue_value_type>;
	using mask_type = itk::Image<unsigned char, 3>;
	using source_type = itk::SliceContiguousImage<float>;
	using gc_filter_type = itk::GraphCutLabelSeparator<mask_type, mask_type, source_type>;

	tissue_value_type const OBJECT_1 = 127;
	tissue_value_type const OBJECT_2 = 255;
	bool has_sigma;
	auto sigma = sigma_edit->text().toDouble(&has_sigma);
	bool use_gradient_magnitude = use_source->isChecked();
	bool use_full_neighborhood = true;

	auto tissues = slice_handler->GetTissues(false);
	auto source = slice_handler->GetImage(SliceHandlerInterface::kSource, false);
	auto target = slice_handler->GetImage(SliceHandlerInterface::kTarget, false);

	// create mask from selected tissue [use current selection, or initial selection?]
	auto threshold = itk::BinaryThresholdImageFilter<tissue_type, mask_type>::New();
	threshold->SetInput(tissues);
	threshold->SetLowerThreshold(tissuenr);
	threshold->SetUpperThreshold(tissuenr);
	threshold->SetInsideValue(1);
	threshold->SetOutsideValue(0);
	threshold->Update();

	auto mask = threshold->GetOutput();
	{
		auto mask_buffer = mask->GetPixelContainer()->GetImportPointer();
		unsigned const first_mark = vm.front().mark;
		size_t const width = slice_handler->return_width();
		size_t const height = slice_handler->return_width();
		size_t const zoffset = current_slice * width * height;
		bool found_other_mark = false;
		for (auto& m : vm)
		{
			mask_buffer[m.p.px + m.p.py * width + zoffset] = (m.mark == first_mark) ? OBJECT_1 : OBJECT_2;
			found_other_mark = found_other_mark || (m.mark != first_mark);
		}

		std::cerr << "Found other mark: " << found_other_mark << "\n";
	}
	dump_image(mask, "E:/temp/_gc_input.nii.gz");

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
	cutter->SetConnectivity(use_full_neighborhood ? gc_filter_type::k26 : gc_filter_type::k6);
	cutter->SetMaskInput(mask);
	if (use_gradient_magnitude)
	{
		cutter->SetIntensityInput(source);
	}

	try
	{
		cutter->Update();
		std::cerr << "Success TissueSeparator finished\n";
	}
	catch (itk::ExceptionObject e)
	{
		std::cerr << "Error: " << e.what() << "\n";
	}
	catch (std::exception e)
	{
		std::cerr << "Error: " << e.what() << "\n";
	}

	// \todo copy result to target [tissue?], use e.g. 255 and 127
	dump_image(cutter->GetOutput(), "E:/temp/_gc_output.nii.gz");

	iseg::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.work = true;
	emit begin_datachange(dataSelection, this);

	iseg::Paste<unsigned char, float>(cutter->GetOutput(), target, slice_handler->return_startslice(), slice_handler->return_endslice());

	emit end_datachange(this);
}
