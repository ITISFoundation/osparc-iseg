/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "BoneSegmentationWidget.h"

#include "ImageGraphCut3DFilter.h"

#include "Data/ItkUtils.h"
#include "Data/SlicesHandlerITKInterface.h"

#include "Interface/PropertyWidget.h"

#include <itkConnectedComponentImageFilter.h>
#include <itkFlatStructuringElement.h>
#include <itkGrayscaleErodeImageFilter.h>
#include <itkImage.h>
#include <itkLabelShapeKeepNObjectsImageFilter.h>
#include <itksys/SystemTools.hxx>

#include <QHBoxLayout>
#include <QProgressDialog>
#include <QVBoxLayout>

#include <algorithm>
#include <sstream>

namespace {
template<class TFilter>
class CommandProgressUpdate : public itk::Command
{
public:
	using Self = CommandProgressUpdate; // NOLINT
	using Superclass = itk::Command;		// NOLINT
	using Pointer = itk::SmartPointer<Self>;
	itkNewMacro(Self);

protected:
	CommandProgressUpdate() = default;

public:
	void SetProgressObject(QProgressDialog* progress) { m_Progress = progress; }

	void Execute(itk::Object* caller, const itk::EventObject& event) override
	{
		Execute((const itk::Object*)caller, event);
	}

	void Execute(const itk::Object* object, const itk::EventObject& event) override
	{
		auto filter = dynamic_cast<const TFilter*>(object);

		if (typeid(event) != typeid(itk::ProgressEvent) || filter == nullptr)
		{
			return;
		}

		double percent = filter->GetProgress() * 100.0;
		m_Progress->setValue(std::min<int>(percent, 100));
	}

private:
	QProgressDialog* m_Progress;
};

} // namespace

BoneSegmentationWidget::BoneSegmentationWidget(iseg::SlicesHandlerInterface* hand3D)
		: m_Handler3D(hand3D), m_CurrentFilter(nullptr)
{
	using namespace iseg;

	setToolTip(Format(
			"A fully-automatic method for segmenting the femur from 3D CT "
			"scans, based on the graph-cut segmentation framework and bone "
			"enhancement filters."
			"<br>"
			"Krcah et al., 'Fully automatic and fast segmentation of the "
			"femur bone from 3D-CT images with no shape prior', IEEE, 2011"));

	m_CurrentSlice = m_Handler3D->ActiveSlice();

	// add properties
	auto group = PropertyGroup::Create("Settings");

	auto pi = group->Add("Iterations", PropertyInt::Create(5));
	pi->SetDescription("Number of iterations");

	m_MaxFlowAlgorithm = group->Add("MaxFlowAlgorithm", PropertyEnum::Create({"Kohli", "PushLabel-Fifo", "PushLabel-H_PRF"}, 0));
	m_MaxFlowAlgorithm->SetDescription("Max-Flow Algorithm");
	m_MaxFlowAlgorithm->SetToolTip("Choose Max-Flow algorithm used to perform Graph-Cut.");

	m_M6Connectivity = group->Add("6-Connectivity", PropertyBool::Create(false));

	m_UseSliceRange = group->Add("UseSliceRange", PropertyBool::Create(false));
	m_UseSliceRange->SetDescription("Use Slice Range");

	m_Start = group->Add("Start Slice", PropertyInt::Create(m_Handler3D->StartSlice()));
	m_End = group->Add("End Slice", PropertyInt::Create(m_Handler3D->EndSlice()));

	// setup callbacks
	auto pbtn = group->Add("Execute", PropertyButton::Create([this]() { DoWork(); }));

	m_UseSliceRange->onModified.connect([this](Property_ptr, Property::eChangeType type) {
		if (type == Property::kValueChanged)
			Showsliders();
	});

	// add widget and layout
	auto property_view = new PropertyWidget(group);
	property_view->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

	auto hbox = new QHBoxLayout;
	hbox->addWidget(property_view);
	hbox->addStretch();

	auto main_layout = new QVBoxLayout;
	main_layout->addLayout(hbox);
	main_layout->addStretch();

	setLayout(main_layout);
}

void BoneSegmentationWidget::Showsliders()
{
	if (m_UseSliceRange->Value())
	{
		m_Start->SetMaximum(m_Handler3D->EndSlice());
		m_Start->SetEnabled(true);
		m_End->SetMaximum(m_Handler3D->EndSlice());
		m_End->SetValue(m_Handler3D->EndSlice());
		m_End->SetEnabled(true);
	}
	else
	{
		m_Start->SetEnabled(false);
		m_End->SetEnabled(false);
	}
}

void BoneSegmentationWidget::DoWork()
{
	QProgressDialog progress("Performing graph cut...", "Cancel", 0, 101, this);
	progress.setWindowModality(Qt::WindowModal);
	progress.setModal(true);
	progress.show();
	progress.setValue(1);

	QObject_connect(&progress, SIGNAL(canceled()), this, SLOT(Cancel()));

	using input_type = itk::Image<float, 3>;
	using output_type = itk::Image<unsigned int, 3>;
	using graph_cut_filter_type = itk::ImageGraphCutFilter<input_type, input_type, input_type, output_type>;

	// get input image
	iseg::SlicesHandlerITKInterface itk_wrapper(m_Handler3D);
	auto input = itk_wrapper.GetImageDeprecated(iseg::SlicesHandlerITKInterface::kSource, m_UseSliceRange->Value());

	// setup algorithm
	auto graph_cut_filter = graph_cut_filter_type::New();
	graph_cut_filter->SetNumberOfRequiredInputs(1);
	graph_cut_filter->SetInputImage(input);
	graph_cut_filter->SetMaxFlowAlgorithm(static_cast<graph_cut_filter_type::eMaxFlowAlgorithm>(m_MaxFlowAlgorithm->Value()));
	graph_cut_filter->SetForegroundPixelValue(255);
	graph_cut_filter->SetBackgroundPixelValue(0);
	graph_cut_filter->SetSigma(0.2);
	graph_cut_filter->SetConnectivity(m_M6Connectivity->Value());

	// assumes input image is 3D
	if (input->GetLargestPossibleRegion().GetSize(2) > 1)
	{
		m_CurrentFilter = graph_cut_filter;

		auto observer = CommandProgressUpdate<graph_cut_filter_type>::New();
		graph_cut_filter->AddObserver(itk::ProgressEvent(), observer);
		observer->SetProgressObject(&progress);

		try
		{
			graph_cut_filter->Update();

			auto output = graph_cut_filter->GetOutput();

			auto target = itk_wrapper.GetTarget(m_UseSliceRange->Value());

			iseg::DataSelection data_selection;
			data_selection.allSlices = true;
			data_selection.work = true;
			emit BeginDatachange(data_selection, this);

			iseg::Paste<output_type, iseg::SlicesHandlerITKInterface::image_ref_type>(output, target);

			emit EndDatachange(this);
		}
		catch (itk::ExceptionObject&)
		{
		}

		m_CurrentFilter = nullptr;
	}
}

void BoneSegmentationWidget::Cancel()
{
	if (m_CurrentFilter)
	{
		m_CurrentFilter->SetAbortGenerateData(true);
		m_CurrentFilter = nullptr;
	}
}

BoneSegmentationWidget::~BoneSegmentationWidget() {}

void BoneSegmentationWidget::OnSlicenrChanged()
{
	m_CurrentSlice = m_Handler3D->ActiveSlice();
}

void BoneSegmentationWidget::Init()
{
	Showsliders();
	OnSlicenrChanged();
	HideParamsChanged();
}

void BoneSegmentationWidget::NewLoaded()
{
	m_CurrentSlice = m_Handler3D->ActiveSlice();
}
