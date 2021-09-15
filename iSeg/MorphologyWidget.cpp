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

#include "MorphologyWidget.h"
#include "SlicesHandler.h"
#include "bmp_read_1.h"

#include "Data/Logger.h"
#include "Data/Point.h"

#include "Core/Morpho.h"
#include "Core/TopologyEditing.h"

#include "Interface/ProgressDialog.h"

#include <QFormLayout>

#include <algorithm>

namespace iseg {

MorphologyWidget::MorphologyWidget(SlicesHandler* hand3D)
		: m_Handler3D(hand3D)
{
	setToolTip(Format("Apply morphological operations to the Target image. "
										"Morphological operations are "
										"based on expanding or shrinking (Dilate/Erode) regions by "
										"a given number of pixel layers (n)."
										"<br>"
										"The functions act on the Target image (to modify a tissue "
										"use Get Tissue and Adder)."));

	m_Activeslice = m_Handler3D->ActiveSlice();
	m_Bmphand = m_Handler3D->GetActivebmphandler();

	// methods
	auto modegroup = new QButtonGroup(this);
	modegroup->insert(m_RbOpen = new QRadioButton(QString("Open")));
	modegroup->insert(m_RbClose = new QRadioButton(QString("Close")));
	modegroup->insert(m_RbErode = new QRadioButton(QString("Erode")));
	modegroup->insert(m_RbDilate = new QRadioButton(QString("Dilate")));
	modegroup->insert(m_RbFillGaps = new QRadioButton(QString("Fill Gaps")));
	m_RbOpen->setChecked(true);

	m_RbOpen->setToolTip(Format("First shrinking before growing is called Open and results in the "
															"deletion of small islands and thin links between structures."));
	m_RbClose->setToolTip(Format("Growing followed by shrinking results in the "
															 "closing of small (< 2n) gaps and holes."));
	m_RbErode->setToolTip(Format("Erode or shrink the boundaries of regions of foreground pixels."));
	m_RbDilate->setToolTip(Format("Enlarge the boundaries of regions of foreground pixels."));
	m_RbFillGaps->setToolTip(Format("Dilate with specified radius to get skeleton surface. Add skeleton to current FG."));

	// method layout
	auto method_vbox = new QVBoxLayout;
	const auto buttons = modegroup->buttons();
	for (auto child : buttons)
		method_vbox->addWidget(child);
	method_vbox->setMargin(5);

	auto method_area = new QFrame(this);
	method_area->setLayout(method_vbox);
	method_area->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
	method_area->setLineWidth(1);

	// params
	m_PixelUnits = new QCheckBox("Pixel");
	m_PixelUnits->setChecked(true);

	m_OperationRadius = new QLineEdit(QString::number(1));
	m_OperationRadius->setValidator(new QIntValidator(0, 100, m_OperationRadius));
	m_OperationRadius->setToolTip(Format("Radius of operation, either number of pixel layers or radius in length units."));

	m_AllSlices = new QCheckBox("Apply to all slices");
	m_AllSlices->setToolTip(Format("Apply to active slices in 3D or to current slice"));

	m_True3d = new QCheckBox("3D");
	m_True3d->setChecked(true);
	m_True3d->setToolTip(Format("Run morphological operations in 3D or per-slice."));

	m_NodeConnectivity = new QCheckBox;
	m_NodeConnectivity->setToolTip(Format("Use chess-board (8 neighbors) or city-block (4 neighbors) neighborhood."));

	m_ExecuteButton = new QPushButton("Execute");

	auto unit_box = new QHBoxLayout;
	unit_box->addWidget(m_OperationRadius);
	unit_box->addWidget(m_PixelUnits);

	// params layout
	auto params_layout = new QFormLayout;
	params_layout->addRow(m_AllSlices, m_True3d);
	params_layout->addRow("Radius", unit_box);
	params_layout->addRow("Full connectivity", m_NodeConnectivity);
	params_layout->addRow(m_ExecuteButton);
	auto params_area = new QWidget(this);
	params_area->setLayout(params_layout);

	// top-level layot
	auto top_layout = new QHBoxLayout;
	top_layout->addWidget(method_area);
	top_layout->addWidget(params_area);
	setLayout(top_layout);

	// init
	AllSlicesChanged();

	// connect signal-slots
	QObject_connect(m_PixelUnits, SIGNAL(stateChanged(int)), this, SLOT(UnitsChanged()));
	QObject_connect(m_AllSlices, SIGNAL(stateChanged(int)), this, SLOT(AllSlicesChanged()));
	QObject_connect(m_ExecuteButton, SIGNAL(clicked()), this, SLOT(Execute()));
}

void MorphologyWidget::Execute()
{
	bool connect8 = m_NodeConnectivity->isChecked();
	bool true3d = m_True3d->isChecked();

	DataSelection data_selection;
	data_selection.work = true;

	if (m_AllSlices->isChecked())
	{
		boost::variant<int, float> radius;
		if (m_PixelUnits->isChecked())
		{
			radius = static_cast<int>(m_OperationRadius->text().toFloat());
		}
		else
		{
			radius = m_OperationRadius->text().toFloat();
		}

		data_selection.allSlices = true;
		emit BeginDatachange(data_selection, this);

		if (m_RbOpen->isChecked())
		{
			ProgressDialog progress("Morphological opening ...", this);
			MorphologicalOperation(m_Handler3D, radius, iseg::kOpen, true3d, &progress);
		}
		else if (m_RbClose->isChecked())
		{
			ProgressDialog progress("Morphological closing ...", this);
			MorphologicalOperation(m_Handler3D, radius, iseg::kClose, true3d, &progress);
		}
		else if (m_RbErode->isChecked())
		{
			ProgressDialog progress("Morphological erosion ...", this);
			MorphologicalOperation(m_Handler3D, radius, iseg::kErode, true3d, &progress);
		}
		else if (m_RbDilate->isChecked())
		{
			ProgressDialog progress("Morphological dilation ...", this);
			MorphologicalOperation(m_Handler3D, radius, iseg::kDilate, true3d, &progress);
		}
		else
		{
			ProgressDialog progress("Morphological editing ...", this);
			FillLoopsAndGaps(m_Handler3D, radius);
		}
	}
	else
	{
		auto radius = static_cast<int>(m_OperationRadius->text().toFloat());

		data_selection.sliceNr = m_Handler3D->ActiveSlice();
		emit BeginDatachange(data_selection, this);

		if (m_RbOpen->isChecked())
		{
			m_Bmphand->Open(radius, connect8);
		}
		else if (m_RbClose->isChecked())
		{
			m_Bmphand->Closure(radius, connect8);
		}
		else if (m_RbErode->isChecked())
		{
			m_Bmphand->Erosion(radius, connect8);
		}
		else
		{
			m_Bmphand->Dilation(radius, connect8);
		}
	}
	emit EndDatachange(this);
}

void MorphologyWidget::OnSlicenrChanged()
{
	m_Activeslice = m_Handler3D->ActiveSlice();
	BmphandChanged(m_Handler3D->GetActivebmphandler());
}

void MorphologyWidget::BmphandChanged(Bmphandler* bmph)
{
	m_Bmphand = bmph;
}

void MorphologyWidget::Init()
{
	if (m_Activeslice != m_Handler3D->ActiveSlice())
	{
		m_Activeslice = m_Handler3D->ActiveSlice();
		BmphandChanged(m_Handler3D->GetActivebmphandler());
	}
	HideParamsChanged();
}

void MorphologyWidget::NewLoaded()
{
	m_Activeslice = m_Handler3D->ActiveSlice();
	m_Bmphand = m_Handler3D->GetActivebmphandler();
}

FILE* MorphologyWidget::SaveParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		bool const connect8 = m_NodeConnectivity->isChecked();
		int radius = m_OperationRadius->text().toFloat();

		int dummy;
		dummy = static_cast<int>(radius);
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(connect8); // rb_8connect->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(!connect8); // rb_4connect->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_RbOpen->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_RbClose->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_RbErode->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_RbDilate->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_AllSlices->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
	}

	return fp;
}

FILE* MorphologyWidget::LoadParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		int dummy;
		fread(&dummy, sizeof(int), 1, fp);
		m_OperationRadius->setText(QString::number(dummy));
		fread(&dummy, sizeof(int), 1, fp);
		m_NodeConnectivity->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		// skip, is used to set a radio button state
		fread(&dummy, sizeof(int), 1, fp);
		m_RbOpen->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		m_RbClose->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		m_RbErode->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		m_RbDilate->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		m_AllSlices->setChecked(dummy > 0);
	}
	return fp;
}

namespace {

int hide_row(QFormLayout* layout, QWidget* widget)
{
	int row = -1;
	if (auto label = layout->labelForField(widget))
	{
		QFormLayout::ItemRole role;
		layout->getWidgetPosition(widget, &row, &role);
		widget->hide();
		label->hide();
		layout->removeWidget(widget);
		layout->removeWidget(label);
	}
	return row;
}

} // namespace

void MorphologyWidget::HideParamsChanged()
{
	// \note this is the horrible side of using QFormLayout, show/hide row does not exist
	auto top_layout = dynamic_cast<QFormLayout*>(layout());
	if (top_layout)
	{
		if (hideparams)
		{
			hide_row(top_layout, m_OperationRadius);
			hide_row(top_layout, m_NodeConnectivity);
		}
		else if (m_NodeConnectivity->isHidden())
		{
			m_OperationRadius->show();
			m_NodeConnectivity->show();
			top_layout->insertRow(1, QString("Pixel Layers (n)"), m_OperationRadius);
			top_layout->insertRow(2, QString("Full connectivity"), m_NodeConnectivity);
		}
	}
}

void iseg::MorphologyWidget::UnitsChanged()
{
	if (m_PixelUnits->isChecked())
	{
		auto radius = m_OperationRadius->text().toFloat();
		m_OperationRadius->setValidator(new QIntValidator(0, 100, m_OperationRadius));
		m_OperationRadius->setText(QString::number(static_cast<int>(radius)));
	}
	else
	{
		m_OperationRadius->setValidator(new QDoubleValidator(0, 100, 2, m_OperationRadius));
	}
}

void iseg::MorphologyWidget::AllSlicesChanged()
{
	m_NodeConnectivity->setEnabled(!m_AllSlices->isChecked());
	m_True3d->setEnabled(m_AllSlices->isChecked());
}

} // namespace iseg
