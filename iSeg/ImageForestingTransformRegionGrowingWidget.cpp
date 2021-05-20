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

#include "ImageForestingTransformRegionGrowingWidget.h"

#include "Data/addLine.h"

#include "Core/ImageForestingTransform.h"

#include <QFormLayout>

namespace iseg {

ImageForestingTransformRegionGrowingWidget::ImageForestingTransformRegionGrowingWidget(SlicesHandler* hand3D, QWidget* parent, const char* name, Qt::WindowFlags wFlags)
		: WidgetInterface(parent, name, wFlags), m_Handler3D(hand3D)
{
	setToolTip(Format("Segment multiple tissues by drawing lines in the current slice based "
										"on "
										"the Image Foresting Transform. "
										"These lines are drawn with the color of the currently selected "
										"tissue. "
										"Multiple lines of different colours can be drawn "
										"and they are subsequently used as seeds to grow regions based on a "
										"local homogeneity criterion. Through competitive growing the best "
										"boundaries "
										"between regions grown from lines with different colours are "
										"identified."
										"<br>"
										"The result is stored in the Target. To assign a segmented region to a "
										"tissue the 'Adder' must be used."));

	m_Activeslice = m_Handler3D->ActiveSlice();
	m_Bmphand = m_Handler3D->GetActivebmphandler();

	m_Area = 0;
	m_IfTrg = nullptr;
	m_Lbmap = nullptr;
	m_Thresh = 0;

	m_Pushclear = new QPushButton("Clear Lines");
	m_Pushremove = new QPushButton("Remove Line");
	m_Pushremove->setToggleButton(true);
	m_Pushremove->setToolTip(Format("Remove Line followed by a click on a line deletes "
																	"this line and automatically updates the segmentation. If Remove "
																	"Line has "
																	"been pressed accidentally, a second press will deactivate the "
																	"function again."));

	m_SlThresh = new QSlider(Qt::Horizontal, nullptr);
	m_SlThresh->setRange(0, 100);
	m_SlThresh->setValue(60);
	m_SlThresh->setEnabled(false);
	m_SlThresh->setFixedWidth(400);

	// layout
	auto layout = new QFormLayout;
	layout->addRow(m_Pushremove, m_Pushclear);
	layout->addRow(m_SlThresh);

	setLayout(layout);

	// connections
	QObject_connect(m_Pushclear, SIGNAL(clicked()), this, SLOT(Clearmarks()));
	QObject_connect(m_SlThresh, SIGNAL(sliderMoved(int)), this, SLOT(SliderChanged(int)));
	QObject_connect(m_SlThresh, SIGNAL(sliderPressed()), this, SLOT(SliderPressed()));
	QObject_connect(m_SlThresh, SIGNAL(sliderReleased()), this, SLOT(SliderReleased()));
}

ImageForestingTransformRegionGrowingWidget::~ImageForestingTransformRegionGrowingWidget()
{
	if (m_IfTrg != nullptr)
		delete m_IfTrg;
	if (m_Lbmap != nullptr)
		delete m_Lbmap;
}

void ImageForestingTransformRegionGrowingWidget::Init()
{
	if (m_Activeslice != m_Handler3D->ActiveSlice())
	{
		m_Activeslice = m_Handler3D->ActiveSlice();
		m_Bmphand = m_Handler3D->GetActivebmphandler();
		Init1();
		if (m_SlThresh->isEnabled())
		{
			Getrange();
		}
	}
	else
	{
		Init1();
	}

	HideParamsChanged();
}

void ImageForestingTransformRegionGrowingWidget::NewLoaded()
{
	m_Activeslice = m_Handler3D->ActiveSlice();
	m_Bmphand = m_Handler3D->GetActivebmphandler();
}

void ImageForestingTransformRegionGrowingWidget::Init1()
{
	std::vector<std::vector<Mark>>* vvm = m_Bmphand->ReturnVvm();
	m_Vm.clear();
	for (auto it = vvm->begin(); it != vvm->end(); it++)
	{
		m_Vm.insert(m_Vm.end(), it->begin(), it->end());
	}
	emit VmChanged(&m_Vm);
	m_Area = m_Bmphand->ReturnHeight() * (unsigned)m_Bmphand->ReturnWidth();
	if (m_Lbmap != nullptr)
		free(m_Lbmap);
	m_Lbmap = (float*)malloc(sizeof(float) * m_Area);
	for (unsigned i = 0; i < m_Area; i++)
		m_Lbmap[i] = 0;
	unsigned width = (unsigned)m_Bmphand->ReturnWidth();
	for (auto it = m_Vm.begin(); it != m_Vm.end(); it++)
	{
		m_Lbmap[width * it->p.py + it->p.px] = (float)it->mark;
	}
	for (auto it = m_Vmdyn.begin(); it != m_Vmdyn.end(); it++)
	{
		m_Lbmap[width * it->py + it->px] = (float)m_Tissuenr;
	}

	if (m_IfTrg != nullptr)
		delete (m_IfTrg);
	m_IfTrg = m_Bmphand->IfTrgInit(m_Lbmap);

	m_Thresh = 0;

	if (!m_Vm.empty())
		m_SlThresh->setEnabled(true);
}

void ImageForestingTransformRegionGrowingWidget::Cleanup()
{
	m_Vmdyn.clear();
	if (m_IfTrg != nullptr)
		delete m_IfTrg;
	if (m_Lbmap != nullptr)
		delete m_Lbmap;
	m_IfTrg = nullptr;
	m_Lbmap = nullptr;
	m_SlThresh->setEnabled(false);
	emit VpdynChanged(&m_Vmdyn);
	emit VmChanged(&m_Vmempty);
}

void ImageForestingTransformRegionGrowingWidget::OnTissuenrChanged(int i)
{
	m_Tissuenr = (unsigned)i + 1;
}

void ImageForestingTransformRegionGrowingWidget::OnMouseClicked(Point p)
{
	m_LastPt = p;
	if (m_Pushremove->isChecked())
	{
		Removemarks(p);
	}
}

void ImageForestingTransformRegionGrowingWidget::OnMouseMoved(Point p)
{
	if (!m_Pushremove->isChecked())
	{
		addLine(&m_Vmdyn, m_LastPt, p);
		m_LastPt = p;
		emit VpdynChanged(&m_Vmdyn);
	}
}

void ImageForestingTransformRegionGrowingWidget::OnMouseReleased(Point p)
{
	if (!m_Pushremove->isChecked())
	{
		addLine(&m_Vmdyn, m_LastPt, p);
		Mark m;
		m.mark = m_Tissuenr;
		unsigned width = (unsigned)m_Bmphand->ReturnWidth();
		std::vector<Mark> vmdummy;
		vmdummy.clear();
		for (auto it = m_Vmdyn.begin(); it != m_Vmdyn.end();
				 it++)
		{
			m.p = *it;
			vmdummy.push_back(m);
			m_Lbmap[it->px + width * it->py] = m_Tissuenr;
		}
		m_Vm.insert(m_Vm.end(), vmdummy.begin(), vmdummy.end());

		DataSelection data_selection;
		data_selection.sliceNr = m_Handler3D->ActiveSlice();
		data_selection.work = true;
		data_selection.vvm = true;
		emit BeginDatachange(data_selection, this);

		m_Bmphand->AddVm(&vmdummy);

		m_Vmdyn.clear();
		emit VpdynChanged(&m_Vmdyn);
		emit VmChanged(&m_Vm);
		Execute();

		emit EndDatachange(this);
	}
	else
	{
		m_Pushremove->setChecked(false);
	}
}

void ImageForestingTransformRegionGrowingWidget::Execute()
{
	m_IfTrg->Reinit(m_Lbmap, false);
	if (hideparams)
		m_Thresh = 0;
	Getrange();
	float* f1 = m_IfTrg->ReturnLb();
	float* f2 = m_IfTrg->ReturnPf();
	float* work_bits = m_Bmphand->ReturnWork();

	float d = 255.0f / m_Bmphand->ReturnVvmmaxim();
	for (unsigned i = 0; i < m_Area; i++)
	{
		if (f2[i] < m_Thresh)
			work_bits[i] = f1[i] * d;
		else
			work_bits[i] = 0;
	}
	m_SlThresh->setEnabled(true);

	m_Bmphand->SetMode(2, false);
}

void ImageForestingTransformRegionGrowingWidget::Clearmarks()
{
	for (unsigned i = 0; i < m_Area; i++)
		m_Lbmap[i] = 0;

	m_Vm.clear();
	m_Vmdyn.clear();
	m_Bmphand->ClearVvm();
	emit VpdynChanged(&m_Vmdyn);
	emit VmChanged(&m_Vm);
}

void ImageForestingTransformRegionGrowingWidget::SliderChanged(int i)
{
	m_Thresh = i * 0.01f * m_Maxthresh;
	if (m_IfTrg != nullptr)
	{
		float* f1 = m_IfTrg->ReturnLb();
		float* f2 = m_IfTrg->ReturnPf();
		float* work_bits = m_Bmphand->ReturnWork();

		float d = 255.0f / m_Bmphand->ReturnVvmmaxim();
		for (unsigned i = 0; i < m_Area; i++)
		{
			if (f2[i] < m_Thresh)
				work_bits[i] = f1[i] * d;
			else
				work_bits[i] = 0;
		}
		m_Bmphand->SetMode(2, false);
		emit EndDatachange(this, iseg::NoUndo);
	}
}

void ImageForestingTransformRegionGrowingWidget::BmpChanged()
{
	m_Bmphand = m_Handler3D->GetActivebmphandler();
	m_SlThresh->setEnabled(false);
	Init1();
}

void ImageForestingTransformRegionGrowingWidget::OnSlicenrChanged()
{
	m_Activeslice = m_Handler3D->ActiveSlice();
	BmphandChanged(m_Handler3D->GetActivebmphandler());
}

void ImageForestingTransformRegionGrowingWidget::BmphandChanged(Bmphandler* bmph)
{
	m_Bmphand = bmph;

	unsigned width = (unsigned)m_Bmphand->ReturnWidth();

	std::vector<std::vector<Mark>>* vvm = m_Bmphand->ReturnVvm();
	m_Vm.clear();
	for (auto& line : *vvm)
	{
		m_Vm.insert(m_Vm.end(), line.begin(), line.end());
	}

	for (unsigned i = 0; i < m_Area; i++)
		m_Lbmap[i] = 0;
	for (auto& m : m_Vm)
	{
		m_Lbmap[width * m.p.py + m.p.px] = static_cast<float>(m.mark);
	}

	if (m_IfTrg != nullptr)
		delete (m_IfTrg);
	m_IfTrg = m_Bmphand->IfTrgInit(m_Lbmap);

	//	thresh=0;

	if (m_SlThresh->isEnabled())
	{
		Getrange();
	}

	emit VmChanged(&m_Vm);
}

void ImageForestingTransformRegionGrowingWidget::Getrange()
{
	float* pf = m_IfTrg->ReturnPf();
	m_Maxthresh = 0;
	for (unsigned i = 0; i < m_Area; i++)
	{
		if (m_Maxthresh < pf[i])
		{
			m_Maxthresh = pf[i];
		}
	}
	if (m_Thresh > m_Maxthresh || m_Thresh == 0)
		m_Thresh = m_Maxthresh;
	if (m_Maxthresh == 0)
		m_Maxthresh = m_Thresh = 1;
	m_SlThresh->setValue(std::min(int(m_Thresh * 100 / m_Maxthresh), 100));
}

void ImageForestingTransformRegionGrowingWidget::Removemarks(Point p)
{
	if (m_Bmphand->DelVm(p, 3))
	{
		DataSelection data_selection;
		data_selection.sliceNr = m_Handler3D->ActiveSlice();
		data_selection.work = true;
		data_selection.vvm = true;
		emit BeginDatachange(data_selection, this);

		std::vector<std::vector<Mark>>* vvm = m_Bmphand->ReturnVvm();
		m_Vm.clear();
		for (auto it = vvm->begin(); it != vvm->end(); it++)
		{
			m_Vm.insert(m_Vm.end(), it->begin(), it->end());
		}

		unsigned width = (unsigned)m_Bmphand->ReturnWidth();
		for (unsigned i = 0; i < m_Area; i++)
			m_Lbmap[i] = 0;
		for (auto it = m_Vm.begin(); it != m_Vm.end(); it++)
		{
			m_Lbmap[width * it->p.py + it->p.px] = (float)it->mark;
		}

		emit VmChanged(&m_Vm);
		Execute();

		emit EndDatachange(this);
	}
}

void ImageForestingTransformRegionGrowingWidget::SliderPressed()
{
	DataSelection data_selection;
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.work = true;
	emit BeginDatachange(data_selection, this);
}

void ImageForestingTransformRegionGrowingWidget::SliderReleased() { emit EndDatachange(this); }

FILE* ImageForestingTransformRegionGrowingWidget::SaveParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		int dummy;
		dummy = m_SlThresh->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		fwrite(&m_Thresh, 1, sizeof(float), fp);
		fwrite(&m_Maxthresh, 1, sizeof(float), fp);
	}

	return fp;
}

FILE* ImageForestingTransformRegionGrowingWidget::LoadParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		QObject_disconnect(m_SlThresh, SIGNAL(sliderMoved(int)), this, SLOT(SliderChanged(int)));

		int dummy;
		fread(&dummy, sizeof(int), 1, fp);
		m_SlThresh->setValue(dummy);
		fread(&m_Thresh, sizeof(float), 1, fp);
		fread(&m_Maxthresh, sizeof(float), 1, fp);

		QObject_connect(m_SlThresh, SIGNAL(sliderMoved(int)), this, SLOT(SliderChanged(int)));
	}
	return fp;
}

void ImageForestingTransformRegionGrowingWidget::HideParamsChanged()
{
	if (hideparams)
	{
		m_SlThresh->hide();
	}
	else
	{
		m_SlThresh->show();
	}
}

} // namespace iseg
