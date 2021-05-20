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

#include <itkConnectedComponentImageFilter.h>
#include <itkFlatStructuringElement.h>
#include <itkGrayscaleErodeImageFilter.h>
#include <itkImage.h>
#include <itkLabelShapeKeepNObjectsImageFilter.h>
#include <itksys/SystemTools.hxx>

#include <qprogressdialog.h>

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

BoneSegmentationWidget::BoneSegmentationWidget(iseg::SlicesHandlerInterface* hand3D, QWidget* parent, const char* name, Qt::WindowFlags wFlags)
		: WidgetInterface(parent, name, wFlags), m_Handler3D(hand3D), m_CurrentFilter(nullptr)
{
	setToolTip(Format("A fully-automatic method for segmenting the femur from 3D CT "
										"scans, based on the graph-cut segmentation framework and bone "
										"enhancement filters."
										"<br>"
										"Krcah et al., 'Fully automatic and fast segmentation of the "
										"femur bone from 3D-CT images with no shape prior', IEEE, 2011"));

	m_CurrentSlice = m_Handler3D->ActiveSlice();

	m_VGrid = new Q3VBox(this);

	m_HGrid1 = new Q3HBox(m_VGrid);
	m_LabelMaxFlowAlgorithm = new QLabel("Max-Flow Algorithm: ", m_HGrid1);
	m_MaxFlowAlgorithm = new QComboBox(m_HGrid1);
	m_MaxFlowAlgorithm->setToolTip(QString("Choose Max-Flow algorithm used to perform Graph-Cut."));
	m_MaxFlowAlgorithm->insertItem(QString("Kohli"));
	m_MaxFlowAlgorithm->insertItem(QString("PushLabel-Fifo"));
	m_MaxFlowAlgorithm->insertItem(QString("PushLabel-H_PRF"));
	m_MaxFlowAlgorithm->setCurrentItem(0);

	m_M6Connectivity = new QCheckBox(QString("6-Connectivity"), m_VGrid);
	m_M6Connectivity->setToolTip(QString("Use fully connected neighborhood or "
																			 "only city-block neighbors (26 vs 6)."));

	// TODO: this should re-use active-slices
	m_UseSliceRange = new QCheckBox(QString("Use Slice Range"), m_VGrid);
	m_HGrid2 = new Q3HBox(m_VGrid);
	m_LabelStart = new QLabel("Start-Slice: ", m_HGrid2);
	m_Start = new QSpinBox(1, 100000, 1, m_HGrid2);
	m_Start->setValue(1);
	m_Start->setEnabled(false);

	m_HGrid3 = new Q3HBox(m_VGrid);
	m_LabelEnd = new QLabel("End-Slice: ", m_HGrid3);
	m_End = new QSpinBox(1, 100000, 1, m_HGrid3);
	m_End->setValue(1);
	m_End->setEnabled(false);

	m_Execute = new QPushButton("Execute", m_VGrid);

	m_VGrid->setMinimumWidth(std::max(300, m_VGrid->sizeHint().width()));

	QObject_connect(m_Execute, SIGNAL(clicked()), this, SLOT(DoWork()));
	QObject_connect(m_UseSliceRange, SIGNAL(clicked()), this, SLOT(Showsliders()));
}

void BoneSegmentationWidget::Showsliders()
{
	if (m_UseSliceRange->isChecked() == true)
	{
		m_Start->setMaximum(m_Handler3D->EndSlice());
		m_Start->setEnabled(true);
		m_End->setMaximum(m_Handler3D->EndSlice());
		m_End->setValue(m_Handler3D->EndSlice());
		m_End->setEnabled(true);
	}
	else
	{
		m_Start->setEnabled(false);
		m_End->setEnabled(false);
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
	auto input = itk_wrapper.GetImageDeprecated(iseg::SlicesHandlerITKInterface::kSource, m_UseSliceRange->isChecked());

	assert(m_MaxFlowAlgorithm->currentItem() > 0);

	// setup algorithm
	auto graph_cut_filter = graph_cut_filter_type::New();
	graph_cut_filter->SetNumberOfRequiredInputs(1);
	graph_cut_filter->SetInputImage(input);
	graph_cut_filter->SetMaxFlowAlgorithm(static_cast<graph_cut_filter_type::eMaxFlowAlgorithm>(m_MaxFlowAlgorithm->currentItem()));
	graph_cut_filter->SetForegroundPixelValue(255);
	graph_cut_filter->SetBackgroundPixelValue(0);
	graph_cut_filter->SetSigma(0.2);

	if (m_M6Connectivity->isChecked())
		graph_cut_filter->SetConnectivity(true);

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

			auto target = itk_wrapper.GetTarget(m_UseSliceRange->isChecked());

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

QSize BoneSegmentationWidget::sizeHint() const
{
	return m_VGrid->sizeHint();
}

BoneSegmentationWidget::~BoneSegmentationWidget() { delete m_VGrid; }

void BoneSegmentationWidget::OnSlicenrChanged()
{
	m_CurrentSlice = m_Handler3D->ActiveSlice();
}

void BoneSegmentationWidget::Init()
{
	OnSlicenrChanged();
	HideParamsChanged();
}

void BoneSegmentationWidget::NewLoaded()
{
	m_CurrentSlice = m_Handler3D->ActiveSlice();
}
