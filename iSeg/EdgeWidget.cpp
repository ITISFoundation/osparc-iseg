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

#include "EdgeWidget.h"
#include "SlicesHandler.h"
#include "bmp_read_1.h"

#include "Interface/PropertyWidget.h"

#include "Core/BinaryThinningImageFilter.h"
#include "Core/ImageConnectivtyGraph.h"

#include "Data/ItkUtils.h"
#include "Data/Point.h"
#include "Data/ScopedTimer.h"
#include "Data/SlicesHandlerITKInterface.h"

#include <vtkCellArray.h>
#include <vtkDataSetWriter.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>

#include <QFileDialog>
#include <QVBoxLayout>

namespace iseg {

EdgeWidget::EdgeWidget(SlicesHandler* hand3D)
		: m_Handler3D(hand3D)
{
	setToolTip(Format(
			"Various edge extraction routines. These are mostly useful "
			"as part of other segmentation techniques."));

	m_Activeslice = m_Handler3D->ActiveSlice();
	m_Bmphand = m_Handler3D->GetActivebmphandler();

	auto group = PropertyGroup::Create("Settings");

	m_Modegroup = group->Add("Method", PropertyEnum::Create({"Sobel", "Laplacian", "Interquartile", "Moment", "Gauss", "Canny", "Laplacian Zero", "Centerlines"}, kSobel));
	m_Modegroup->SetDescription("Method");
	m_Modegroup->SetToolTip("Select a method to detect edge features");

	m_SlSigma = group->Add("Sigma", PropertySlider::Create(30, 1, 100));
	m_SlSigma->SetDescription("Sigma");

	m_SlThresh1 = group->Add("ThresholdLo", PropertySlider::Create(20, 1, 100));
	m_SlThresh1->SetDescription("Lower Threshold");

	m_SlThresh2 = group->Add("ThresholdHi", PropertySlider::Create(80, 1, 100));
	m_SlThresh2->SetDescription("Upper Threshold");

	m_Cb3d = group->Add("Skeletonize3D", PropertyBool::Create(true));
	m_Cb3d->SetDescription("Apply in 3D");

	m_BtnExec = group->Add("Execute", PropertyButton::Create([this]() { Execute(); }));
	m_BtnExportCenterlines = group->Add("Export", PropertyButton::Create([this]() { ExportCenterlines(); }));

	// initialize
	MethodChanged();

	// create signal-slot connections
	m_Modegroup->onModified.connect([this](Property_ptr, Property::eChangeType type) {
		MethodChanged();
	});

	m_SlSigma->onReleased.connect([this](int) {
		SliderChanged();
	});

	m_SlThresh1->onReleased.connect([this](int) {
		SliderChanged();
	});

	m_SlThresh2->onReleased.connect([this](int) {
		SliderChanged();
	});

	// add widget and layout
	auto property_view = new PropertyWidget(group);

	auto layout = new QHBoxLayout;
	layout->addWidget(property_view, 2);
	layout->addStretch(1);
	setLayout(layout);
}

EdgeWidget::~EdgeWidget() = default;

void EdgeWidget::NewLoaded()
{
	m_Activeslice = m_Handler3D->ActiveSlice();
	m_Bmphand = m_Handler3D->GetActivebmphandler();
}

void EdgeWidget::Init()
{
	OnSlicenrChanged();
	HideParamsChanged();
}

FILE* EdgeWidget::SaveParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		int dummy;
		dummy = m_SlSigma->Value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = m_SlThresh1->Value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = m_SlThresh2->Value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_Modegroup->Value() == kSobel);
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_Modegroup->Value() == kLaplacian);
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_Modegroup->Value() == kInterquartile);
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_Modegroup->Value() == kMomentline);
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_Modegroup->Value() == kGaussline);
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_Modegroup->Value() == kCanny);
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_Modegroup->Value() == kLaplacianzero);
		fwrite(&(dummy), 1, sizeof(int), fp);
	}

	return fp;
}

FILE* EdgeWidget::LoadParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		int sigma, thr1, thr2;
		std::array<int, eModeType::eModeTypeSize> mode{};
		fread(&sigma, sizeof(int), 1, fp);
		fread(&thr1, sizeof(int), 1, fp);
		fread(&thr2, sizeof(int), 1, fp);

		fread(&mode[kSobel], sizeof(int), 1, fp);
		fread(&mode[kLaplacian], sizeof(int), 1, fp);
		fread(&mode[kInterquartile], sizeof(int), 1, fp);
		fread(&mode[kMomentline], sizeof(int), 1, fp);
		fread(&mode[kGaussline], sizeof(int), 1, fp);
		fread(&mode[kCanny], sizeof(int), 1, fp);
		fread(&mode[kLaplacianzero], sizeof(int), 1, fp);

		m_BlockExecute = true;

		m_SlSigma->SetValue(sigma);
		m_SlThresh1->SetValue(thr1);
		m_SlThresh2->SetValue(thr2);
		for (unsigned int i = 0; i < eModeType::eModeTypeSize; ++i)
		{
			if (mode[i] != 0)
			{
				m_Modegroup->SetValue(i);
				break;
			}
		}

		MethodChanged();

		m_BlockExecute = false;
	}
	return fp;
}

void EdgeWidget::OnSlicenrChanged()
{
	m_Activeslice = m_Handler3D->ActiveSlice();
	m_Bmphand = m_Handler3D->GetActivebmphandler();
}

void EdgeWidget::MethodChanged()
{
	m_SlSigma->SetVisible(
			m_Modegroup->Value() == kGaussline ||
			m_Modegroup->Value() == kCanny ||
			m_Modegroup->Value() == kLaplacianzero);

	m_SlThresh1->SetVisible(
			m_Modegroup->Value() == kCanny ||
			m_Modegroup->Value() == kLaplacianzero);

	m_SlThresh2->SetVisible(m_Modegroup->Value() == kCanny);

	m_Cb3d->SetVisible(m_Modegroup->Value() == kCenterlines);
	m_BtnExportCenterlines->SetVisible(m_Modegroup->Value() == kCenterlines);
}

void EdgeWidget::SliderChanged()
{
	if (m_Modegroup->Value() != kCenterlines)
	{
		Execute();
	}
}

void EdgeWidget::Execute()
{
	if (m_BlockExecute)
	{
		return;
	}

	DataSelection data_selection;
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.work = true;
	emit BeginDatachange(data_selection, this);

	if (m_Modegroup->Value() == kSobel)
	{
		m_Bmphand->Sobel();
	}
	else if (m_Modegroup->Value() == kLaplacian)
	{
		m_Bmphand->Laplacian1();
	}
	else if (m_Modegroup->Value() == kInterquartile)
	{
		m_Bmphand->MedianInterquartile(false);
	}
	else if (m_Modegroup->Value() == kMomentline)
	{
		m_Bmphand->MomentLine();
	}
	else if (m_Modegroup->Value() == kGaussline)
	{
		m_Bmphand->GaussLine(m_SlSigma->Value() * 0.05f);
	}
	else if (m_Modegroup->Value() == kCanny)
	{
		m_Bmphand->CannyLine(m_SlSigma->Value() * 0.05f, m_SlThresh1->Value() * 1.5f, m_SlThresh2->Value() * 1.5f);
	}
	else if (m_Modegroup->Value() == kCenterlines)
	{
		try
		{
			ScopedTimer timer("Skeletonization");
			if (m_Cb3d->Value())
			{
				using input_type = itk::SliceContiguousImage<float>;
				using output_type = itk::Image<unsigned char, 3>;

				SlicesHandlerITKInterface wrapper(m_Handler3D);
				auto target = wrapper.GetTarget(true);
				auto skeleton = BinaryThinning<input_type, output_type>(target, 0.001f);

				DataSelection data_selection;
				data_selection.allSlices = true;
				data_selection.work = true;
				emit BeginDatachange(data_selection, this);

				iseg::Paste<output_type, input_type>(skeleton, target);

				emit EndDatachange(this);
			}
			else
			{
				using input_type = itk::Image<float, 2>;
				using output_type = itk::Image<unsigned char, 2>;

				SlicesHandlerITKInterface wrapper(m_Handler3D);
				auto target = wrapper.GetTargetSlice();
				auto skeleton = BinaryThinning<input_type, output_type>(target, 0.001f);

				DataSelection data_selection;
				data_selection.sliceNr = m_Handler3D->ActiveSlice();
				data_selection.work = true;
				emit BeginDatachange(data_selection, this);

				iseg::Paste<output_type, input_type>(skeleton, target);

				emit EndDatachange(this);
			}
		}
		catch (itk::ExceptionObject& e)
		{
			ISEG_ERROR("Thinning failed: " << e.what());
		}
		catch (std::exception& e)
		{
			ISEG_ERROR("Thinning failed: " << e.what());
		}
	}
	else
	{
		m_Bmphand->LaplacianZero(m_SlSigma->Value() * 0.05f, m_SlThresh1->Value() * 0.5f, false);
	}

	emit EndDatachange(this);
}

void EdgeWidget::ExportCenterlines()
{
	QString savefilename = QFileDialog::getSaveFileName(QString::null, "VTK legacy file (*.vtk)\n", this);
	if (!savefilename.isEmpty())
	{
		SlicesHandlerITKInterface wrapper(m_Handler3D);
		auto target = wrapper.GetTarget(true);

		using input_type = itk::SliceContiguousImage<float>;
		auto edges = ImageConnectivityGraph<input_type>(target, target->GetBufferedRegion());

		auto points = vtkSmartPointer<vtkPoints>::New();
		auto lines = vtkSmartPointer<vtkCellArray>::New();

		std::vector<vtkIdType> node_map(target->GetBufferedRegion().GetNumberOfPixels(), -1);
		itk::Point<double, 3> p;
		for (auto e : edges)
		{
			if (node_map[e[0]] == -1)
			{
				target->TransformIndexToPhysicalPoint(target->ComputeIndex(e[0]), p);
				node_map[e[0]] = points->InsertNextPoint(p[0], p[1], p[2]);
			}
			if (node_map[e[1]] == -1)
			{
				target->TransformIndexToPhysicalPoint(target->ComputeIndex(e[1]), p);
				node_map[e[1]] = points->InsertNextPoint(p[0], p[1], p[2]);
			}
			lines->InsertNextCell(2);
			lines->InsertCellPoint(node_map[e[0]]);
			lines->InsertCellPoint(node_map[e[1]]);
		}

		auto pd = vtkSmartPointer<vtkPolyData>::New();
		pd->SetLines(lines);
		pd->SetPoints(points);

		auto writer = vtkSmartPointer<vtkDataSetWriter>::New();
		writer->SetInputData(pd);
		writer->SetFileTypeToBinary();
		writer->SetFileName(savefilename.toStdString().c_str());
		writer->Write();
	}
}

} // namespace iseg
