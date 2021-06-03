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

#include "OutlineCorrectionParameterViews.h"

#include "Interface/FormatTooltip.h"
#include "Interface/LayoutTools.h"
#include "Interface/QtConnect.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QScrollArea>

namespace iseg {

OLCorrParamView::OLCorrParamView(QWidget* parent /*= 0*/) : ParamViewBase(parent)
{
	m_Target = new QRadioButton(QString("Target"));
	m_Tissues = new QRadioButton(QString("Tissues"));
	m_Target->setChecked(true);
	auto input_group = make_button_group(this, {m_Target, m_Tissues});

	m_SelectObject = new QPushButton(tr("Select"));
	m_SelectObject->setCheckable(true);
	m_ObjectValue = new QLineEdit(QString::number(255));
	m_ObjectValue->setValidator(new QDoubleValidator);

	auto input_hbox = make_hbox({m_Target, m_Tissues});
	auto object_hbox = make_hbox({m_SelectObject, m_ObjectValue});

	auto layout = new QFormLayout;
	layout->addRow(tr("Input image"), input_hbox);
	layout->addRow(tr("Object value"), object_hbox);
	setLayout(layout);
}

void OLCorrParamView::SetObjectValue(float v)
{
	m_ObjectValue->setText(QString::number(v));
	m_SelectObject->setChecked(false);
}

BrushParamView::BrushParamView(QWidget* parent /*= 0*/) : ParamViewBase(parent)
{
	// parameter fields
	m_Target = new QRadioButton(QString("Target"));
	m_Tissues = new QRadioButton(QString("Tissues"));
	auto input_group = make_button_group(this, {m_Target, m_Tissues});
	m_Target->setChecked(true);

	m_SelectObject = new QPushButton(tr("Select"));
	m_SelectObject->setCheckable(true);
	m_ObjectValue = new QLineEdit(QString::number(255));
	m_ObjectValue->setValidator(new QDoubleValidator);

	m_Modify = new QRadioButton(QString("Modify"));
	m_Draw = new QRadioButton(QString("Draw"));
	m_Erase = new QRadioButton(QString("Erase"));
	auto mode_group = make_button_group(this, {m_Modify, m_Draw, m_Erase});
	m_Modify->setChecked(true);

	m_Radius = new QLineEdit(QString::number(1));
	m_UnitPixel = new QRadioButton(tr("Pixel"));
	m_UnitMm = new QRadioButton(tr("Use spacing"));
	auto unit_group = make_button_group(this, {m_UnitPixel, m_UnitMm});
	m_UnitPixel->setChecked(true);

	m_ShowGuide = new QCheckBox;
	m_ShowGuide->setChecked(false);
	m_GuideOffset = new QSpinBox(-100, 100, 1, nullptr);
	m_GuideOffset->setValue(1);
	m_CopyGuide = new QPushButton(QString("Copy"), nullptr);
	m_CopyPickGuide = new QPushButton(QString("Copy Picked"), nullptr);
	m_CopyPickGuide->setCheckable(true);

	// layout
	auto input_hbox = make_hbox({m_Target, m_Tissues});
	auto object_hbox = make_hbox({m_SelectObject, m_ObjectValue});
	auto mode_hbox = make_hbox({m_Modify, m_Draw, m_Erase});
	auto unit_hbox = make_hbox({m_Radius, m_UnitPixel, m_UnitMm});
	auto guide_hbox = make_hbox({m_ShowGuide, m_GuideOffset, m_CopyGuide, m_CopyPickGuide});

	auto layout = new QFormLayout;
	layout->addRow(tr("Input image"), input_hbox);
	layout->addRow(tr("Object value"), object_hbox);
	layout->addRow(tr("Brush Mode"), mode_hbox);
	layout->addRow(tr("Brush Radius"), unit_hbox);
	layout->addRow(tr("Show guide at offset"), guide_hbox);
	setLayout(layout);

	QObject_connect(unit_group, SIGNAL(buttonClicked(int)), this, SLOT(UnitChanged()));
}

void BrushParamView::UnitChanged()
{
	if (m_UnitMm->isChecked())
	{
		m_Radius->setValidator(new QDoubleValidator);
	}
	else
	{
		m_Radius->setText(QString::number(static_cast<int>(m_Radius->text().toFloat())));
		m_Radius->setValidator(new QIntValidator);
	}
}

void BrushParamView::SetObjectValue(float v)
{
	m_ObjectValue->setText(QString::number(v));
	m_SelectObject->setChecked(false);
}

std::vector<iseg::Point> BrushParamView::DrawCircle(Point p, float spacing_x, float spacing_y, int width, int height) const
{
	Point p1;
	std::vector<Point> vpdyn;
	if (m_UnitMm->isChecked())
	{
		float const radius = m_Radius->text().toFloat();
		float const radius_corrected =
				spacing_x > spacing_y
						? std::floor(radius / spacing_x + 0.5f) * spacing_x
						: std::floor(radius / spacing_y + 0.5f) * spacing_y;
		float const radius_corrected2 = radius_corrected * radius_corrected;

		int const xradius = std::ceil(radius_corrected / spacing_x);
		int const yradius = std::ceil(radius_corrected / spacing_y);
		for (p1.px = std::max(0, p.px - xradius);
				 p1.px <= std::min(width - 1, p.px + xradius);
				 p1.px++)
		{
			for (p1.py = std::max(0, p.py - yradius);
					 p1.py <= std::min(height - 1, p.py + yradius);
					 p1.py++)
			{
				if (std::pow(spacing_x * (p.px - p1.px), 2.f) + std::pow(spacing_y * (p.py - p1.py), 2.f) <= radius_corrected2)
				{
					vpdyn.push_back(p1);
				}
			}
		}
	}
	else
	{
		int const xradius = m_Radius->text().toInt();
		int const yradius = xradius;
		int const radius2 = xradius * xradius;
		for (p1.px = std::max(0, p.px - xradius);
				 p1.px <= std::min(width - 1, p.px + xradius);
				 p1.px++)
		{
			for (p1.py = std::max(0, p.py - yradius);
					 p1.py <= std::min(height - 1, p.py + yradius);
					 p1.py++)
			{
				if ((p.px - p1.px) * (p.px - p1.px) + (p.py - p1.py) * (p.py - p1.py) <= radius2)
				{
					vpdyn.push_back(p1);
				}
			}
		}
	}
	return vpdyn;
}

FillHolesParamView::FillHolesParamView(QWidget* parent /*= 0*/) : ParamViewBase(parent)
{
	m_AllSlices = new QCheckBox;
	m_AllSlices->setChecked(false);

	m_Target = new QRadioButton(QString("Target"));
	m_Tissues = new QRadioButton(QString("Tissues"));
	auto input_group = make_button_group(this, {m_Target, m_Tissues});
	m_Target->setChecked(true);

	m_SelectObject = new QPushButton(tr("Select"));
	m_SelectObject->setCheckable(true);
	m_ObjectValue = new QLineEdit(QString::number(255));
	m_ObjectValue->setValidator(new QDoubleValidator);

	m_ObjectSize = new QSpinBox(1, std::numeric_limits<int>::max(), 1, nullptr);
	m_ObjectSizeLabel = new QLabel(tr("Hole size"));

	m_Execute = new QPushButton(tr("Execute"));

	auto input_hbox = make_hbox({m_Target, m_Tissues});
	auto object_hbox = make_hbox({m_SelectObject, m_ObjectValue});

	auto layout = new QFormLayout;
	layout->addRow(tr("Apply to all slices"), m_AllSlices);
	layout->addRow(tr("Input image"), input_hbox);
	layout->addRow(tr("Object value"), object_hbox);
	layout->addRow(m_ObjectSizeLabel, m_ObjectSize);
	layout->addRow(m_Execute);
	setLayout(layout);
}

void FillHolesParamView::SetObjectValue(float v)
{
	m_ObjectValue->setText(QString::number(v));
	m_SelectObject->setChecked(false);
}

AddSkinParamView::AddSkinParamView(QWidget* parent /*= 0*/) : ParamViewBase(parent)
{
	m_AllSlices = new QCheckBox;
	m_AllSlices->setChecked(false);

	m_Target = new QRadioButton(QString("Target"));
	m_Tissues = new QRadioButton(QString("Tissues"));
	auto input_group = make_button_group(this, {m_Target, m_Tissues});
	m_Target->setChecked(true);

	m_Thickness = new QLineEdit(QString::number(1));
	m_Thickness->setValidator(new QIntValidator);
	m_UnitPixel = new QRadioButton(tr("Pixel"));
	m_UnitMm = new QRadioButton(tr("Use spacing"));
	auto unit_group = make_button_group(this, {m_UnitPixel, m_UnitMm});
	m_UnitPixel->setChecked(true);

	m_Inside = new QRadioButton(tr("Inside"));
	m_Outside = new QRadioButton(tr("Outside"));
	auto mode_group = make_button_group(this, {m_Inside, m_Outside});
	m_Inside->setChecked(true);

	m_Execute = new QPushButton(tr("Execute"));

	auto input_hbox = make_hbox({m_Target, m_Tissues});
	auto thickness_hbox = make_hbox({m_Thickness, m_UnitPixel, m_UnitMm});
	auto mode_hbox = make_hbox({m_Inside, m_Outside});

	auto layout = new QFormLayout;
	layout->addRow(tr("Apply to all slices"), m_AllSlices);
	layout->addRow(tr("Input image"), input_hbox);
	layout->addRow(tr("Thickness"), thickness_hbox);
	layout->addRow(tr("Where"), mode_hbox);
	layout->addRow(m_Execute);
	setLayout(layout);

	QObject_connect(unit_group, SIGNAL(buttonClicked(int)), this, SLOT(UnitChanged()));
}

void AddSkinParamView::UnitChanged()
{
	if (m_UnitMm->isChecked())
	{
		m_Thickness->setValidator(new QDoubleValidator);
	}
	else
	{
		m_Thickness->setText(QString::number(static_cast<int>(m_Thickness->text().toFloat())));
		m_Thickness->setValidator(new QIntValidator);
	}
}

FillSkinParamView::FillSkinParamView(SlicesHandlerInterface* h, QWidget* parent /*= 0*/) : ParamViewBase(parent), m_Handler(h)
{
	// parameter fields
	m_AllSlices = new QCheckBox;
	m_AllSlices->setChecked(false);

	m_Thickness = new QLineEdit(QString::number(1));
	m_UnitPixel = new QRadioButton(tr("Pixel"));
	m_UnitMm = new QRadioButton(tr("Use spacing"));
	auto unit_group = make_button_group(this, {m_UnitPixel, m_UnitMm});
	m_UnitPixel->setChecked(true);

	m_SelectBackground = new QPushButton(tr("Get Selected"));
	m_BackgroundValue = new QLineEdit;

	m_SelectSkin = new QPushButton(tr("Get Selected"));
	m_SkinValue = new QLineEdit;

	m_Execute = new QPushButton(tr("Execute"));

	// layout
	auto thickness_hbox = make_hbox({m_Thickness, m_UnitPixel, m_UnitMm});
	auto bg_hbox = make_hbox({m_SelectBackground, m_BackgroundValue});
	auto fg_hbox = make_hbox({m_SelectSkin, m_SkinValue});

	auto layout = new QFormLayout;
	layout->addRow(tr("Apply to all slices"), m_AllSlices);
	layout->addRow(tr("Thickness"), thickness_hbox);
	layout->addRow(tr("Background"), bg_hbox);
	layout->addRow(tr("Skin"), fg_hbox);
	layout->addRow(m_Execute);
	setLayout(layout);

	QObject_connect(m_SelectBackground, SIGNAL(clicked()), this, SLOT(OnSelectBackground()));
	QObject_connect(m_SelectSkin, SIGNAL(clicked()), this, SLOT(OnSelectSkin()));
	QObject_connect(unit_group, SIGNAL(buttonClicked(int)), this, SLOT(UnitChanged()));
}

void FillSkinParamView::SelectBackground(QString tissueName, tissues_size_t nr)
{
	m_BackgroundValue->setText(tissueName);

	m_SelectedBackgroundId = nr;
	m_BackgroundSelected = true;

	m_Execute->setEnabled(m_BackgroundSelected && m_SkinSelected);
}

void FillSkinParamView::SelectSkin(QString tissueName, tissues_size_t nr)
{
	m_SkinValue->setText(tissueName);

	m_SelectedSkinId = nr;
	m_SkinSelected = true;

	m_Execute->setEnabled(m_BackgroundSelected && m_SkinSelected);
}

void FillSkinParamView::Init()
{
	m_Execute->setEnabled(m_BackgroundSelected && m_SkinSelected);
}

void FillSkinParamView::UnitChanged()
{
	if (m_UnitMm->isChecked())
	{
		m_Thickness->setValidator(new QDoubleValidator);
	}
	else
	{
		m_Thickness->setText(QString::number(static_cast<int>(m_Thickness->text().toFloat())));
		m_Thickness->setValidator(new QIntValidator);
	}
}

void FillSkinParamView::OnSelectBackground()
{
	auto sel = m_Handler->TissueSelection();
	if (sel.size() == 1)
	{
		SelectBackground(m_Handler->TissueNames().at(sel[0]).c_str(), sel[0]);
	}
	else
	{
		ISEG_WARNING("Please select only one tissue");
	}
}

void FillSkinParamView::OnSelectSkin()
{
	auto sel = m_Handler->TissueSelection();
	if (sel.size() == 1)
	{
		SelectSkin(m_Handler->TissueNames().at(sel[0]).c_str(), sel[0]);
	}
	else
	{
		ISEG_WARNING("Please select only one tissue");
	}
}

FillAllParamView::FillAllParamView(QWidget* parent /*= 0*/) : ParamViewBase(parent)
{
	m_AllSlices = new QCheckBox;
	m_AllSlices->setChecked(false);

	m_Target = new QRadioButton(QString("Target"));
	m_Tissues = new QRadioButton(QString("Tissues"));
	auto input_group = make_button_group(this, {m_Target, m_Tissues});
	m_Target->setChecked(true);

	m_Execute = new QPushButton(tr("Execute"));

	auto input_hbox = make_hbox({m_Target, m_Tissues});

	auto layout = new QFormLayout;
	layout->addRow(tr("Apply to all slices"), m_AllSlices);
	layout->addRow(tr("Input image"), input_hbox);
	layout->addRow(m_Execute);
	setLayout(layout);
}

SmoothTissuesParamView::SmoothTissuesParamView(QWidget* parent /*= 0*/) : ParamViewBase(parent)
{
	setToolTip(Format("This tool smooths all non-locked tissues by computing a smoothed signed distance "
										"function for each (non-locked) tissue and re-assigning the voxel to that of the most "
										"negative ('most inside') tissue signed distance."));

	m_ActiveSlice = new QRadioButton(tr("Current slice"));
	m_AllSlices = new QRadioButton(tr("All slices (2D)"));
	m_3D = new QRadioButton(tr("Fully 3D"));
	auto where_group = make_button_group(this, {m_ActiveSlice, m_AllSlices, m_3D});
	m_ActiveSlice->setChecked(true);

	m_Sigma = new QLineEdit(QString::number(0.3));
	m_Sigma->setValidator(new QDoubleValidator);
	m_Sigma->setToolTip(Format("Gaussian Sigma defines the radius of influence "
														 "of the Gaussian smoothing used to smooth the tissues "
														 "in world coordinates, e.g. mm. Sigma is the sqrt(variance)."));

	m_SplitLimit = new QLineEdit(QString::number(500));
	m_SplitLimit->setValidator(new QIntValidator);
	m_SplitLimit->setToolTip(Format("Splits active slices into batches of # slices to reduce memory requirements."
																	"This option is only used for 3D smoothing."));

	m_Execute = new QPushButton(QString("Execute"));

	auto hbox = make_hbox({m_ActiveSlice, m_AllSlices, m_3D});

	auto layout = new QFormLayout;
	layout->addRow(tr("Apply to"), hbox);
	layout->addRow(tr("Sigma"), m_Sigma);
	layout->addRow(tr("Max Slices in 3D"), m_SplitLimit);
	layout->addRow(m_Execute);
	setLayout(layout);
}

SpherizeParamView::SpherizeParamView(QWidget* parent /*= 0*/)
{
	m_Target = new QRadioButton(QString("Target"));
	m_Tissues = new QRadioButton(QString("Tissues"));
	auto input_group = make_button_group(this, {m_Target, m_Tissues});
	m_Target->setChecked(true);

	m_SelectObject = new QPushButton(tr("Select"));
	m_SelectObject->setCheckable(true);
	m_ObjectValue = new QLineEdit(QString::number(255));
	m_ObjectValue->setValidator(new QDoubleValidator);

	m_CarveInside = new QRadioButton(tr("Carve inside"));
	m_CarveInside->setToolTip(Format("Remove voxels from inside the selected object to open handles and enfore a genus '0'."));
	m_CarveOutside = new QRadioButton(tr("Carve from outside"));
	m_CarveOutside->setToolTip(Format("Add voxels around the selected object to close handles and cavities and enfore a genus '0'."));

	auto how_group = make_button_group(this, {m_CarveInside, m_CarveOutside});
	m_CarveInside->setChecked(true);

	auto input_hbox = make_hbox({m_Target, m_Tissues});
	auto object_hbox = make_hbox({m_SelectObject, m_ObjectValue});
	auto carve_hbox = make_hbox({m_CarveInside, m_CarveOutside});

	m_Execute = new QPushButton(QString("Execute"));

	auto layout = new QFormLayout;
	layout->addRow(tr("Input image"), input_hbox);
	layout->addRow(tr("Object value"), object_hbox);
	layout->addRow(tr("Method"), carve_hbox);
	layout->addRow(m_Execute);
	setLayout(layout);
}

void SpherizeParamView::SetObjectValue(float v)
{
	m_ObjectValue->setText(QString::number(v));
	m_SelectObject->setChecked(false);
}

} // namespace iseg
