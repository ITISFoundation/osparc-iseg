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

#include "ImageGraphCut3DFilter.h"

#include "Interface/addLine.h"

#include <itkConnectedComponentImageFilter.h>
#include <itkFlatStructuringElement.h>
#include <itkGrayscaleErodeImageFilter.h>
#include <itkImage.h>
#include <itkLabelShapeKeepNObjectsImageFilter.h>
#include <itksys/SystemTools.hxx>

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
	execute_button = new QPushButton("Execute");

	// setup layout
	auto top_layout = new QFormLayout;
	// add options ?
	top_layout->addRow(QString("Apply to all slices"), all_slices);
	top_layout->addRow(QString("Use source"), use_source);
	top_layout->addRow(QString("Sigma"), sigma_edit);
	top_layout->addRow(execute_button);

	setLayout(top_layout);

	// connect signals
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
	dataSelection.work = true;
	dataSelection.vvm = true;
	emit begin_datachange(dataSelection, this);
	emit vpdyn_changed(&vpdyn);
	emit vm_changed(&vm);

	// here run algorithm

	emit end_datachange(this);
}

void TissueSeparatorWidget::do_work() //Code for special GC for division
{
	// get source [optional]

	// get segmentation

	// create mask from selected tissue [how to select tissue?]
	// -> BG_VALUE = 0
	// -> MASK_VALUE = 1
	// -> OBJECT1_VALUE = 127
	// -> OBJECT2_VALUE = 255

	// build "speed-function" from gradient magnitude [scale?]

	// build graph, ignoring connections outside mask

	// sparse graph or grid-based?

	// label nodes in graph, based on marks [graph->SetTerminalArcCap]

	// solve graph-cut

	// copy result to target [tissue?], use e.g. 255 and 127
}
