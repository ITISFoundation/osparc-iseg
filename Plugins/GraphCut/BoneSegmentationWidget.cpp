/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
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
#include "Data/SliceHandlerItkWrapper.h"

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
	typedef CommandProgressUpdate Self;
	typedef itk::Command Superclass;
	typedef itk::SmartPointer<Self> Pointer;
	itkNewMacro(Self);

protected:
	CommandProgressUpdate() {}

public:
	void SetProgressObject(QProgressDialog* progress) { m_Progress = progress; }

	void Execute(itk::Object* caller, const itk::EventObject& event) ITK_OVERRIDE
	{
		Execute((const itk::Object*)caller, event);
	}

	void Execute(const itk::Object* object,
			const itk::EventObject& event) ITK_OVERRIDE
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

BoneSegmentationWidget::BoneSegmentationWidget(
		iseg::SliceHandlerInterface* hand3D, QWidget* parent, const char* name,
		Qt::WindowFlags wFlags)
		: WidgetInterface(parent, name, wFlags), m_Handler3D(hand3D),
			m_CurrentFilter(nullptr)
{
	setToolTip(Format(
			"A fully-automatic method for segmenting the femur from 3D CT "
			"scans, based on the graph-cut segmentation framework and bone "
			"enhancement filters."
			"<br>"
			"Krcah et al., 'Fully automatic and fast segmentation of the "
			"femur bone from 3D-CT images with no shape prior', IEEE, 2011"));

	m_CurrentSlice = m_Handler3D->active_slice();

	m_VGrid = new Q3VBox(this);

	m_HGrid1 = new Q3HBox(m_VGrid);
	m_LabelMaxFlowAlgorithm = new QLabel("Max-Flow Algorithm: ", m_HGrid1);
	m_MaxFlowAlgorithm = new QComboBox(m_HGrid1);
	m_MaxFlowAlgorithm->setToolTip(
			QString("Choose Max-Flow algorithm used to perform Graph-Cut."));
	m_MaxFlowAlgorithm->insertItem(QString("Kohli"));
	m_MaxFlowAlgorithm->insertItem(QString("PushLabel-Fifo"));
	m_MaxFlowAlgorithm->insertItem(QString("PushLabel-H_PRF"));
	m_MaxFlowAlgorithm->setCurrentItem(0);

	m_6Connectivity = new QCheckBox(QString("6-Connectivity"), m_VGrid);
	m_6Connectivity->setToolTip(QString("Use fully connected neighborhood or "
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

	QObject::connect(m_Execute, SIGNAL(clicked()), this, SLOT(do_work()));
	QObject::connect(m_UseSliceRange, SIGNAL(clicked()), this,
			SLOT(showsliders()));
}

void BoneSegmentationWidget::showsliders()
{
	if (m_UseSliceRange->isChecked() == true)
	{
		m_Start->setMaximum(m_Handler3D->end_slice());
		m_Start->setEnabled(true);
		m_End->setMaximum(m_Handler3D->end_slice());
		m_End->setValue(m_Handler3D->end_slice());
		m_End->setEnabled(true);
	}
	else
	{
		m_Start->setEnabled(false);
		m_End->setEnabled(false);
	}
}

void BoneSegmentationWidget::do_work()
{
	QProgressDialog progress("Performing graph cut...", "Cancel", 0, 101, this);
	progress.setWindowModality(Qt::WindowModal);
	progress.setModal(true);
	progress.show();
	progress.setValue(1);

	QObject::connect(&progress, SIGNAL(canceled()), this, SLOT(cancel()));

	typedef itk::Image<float, 3> TInput;
	typedef itk::Image<unsigned int, 3> TOutput;
	typedef itk::Image<int, 3> TIntImage;
	typedef itk::ImageGraphCutFilter<TInput, TInput, TInput, TOutput> GraphCutFilterType;

	// get input image
	iseg::SliceHandlerItkWrapper itk_wrapper(m_Handler3D);
	auto input = itk_wrapper.GetImageDeprecated(iseg::SliceHandlerItkWrapper::kSource, m_UseSliceRange->isChecked());

	assert(m_MaxFlowAlgorithm->currentItem() > 0);

	// setup algorithm
	auto graphCutFilter = GraphCutFilterType::New();
	graphCutFilter->SetNumberOfRequiredInputs(1);
	graphCutFilter->SetInputImage(input);
	graphCutFilter->SetMaxFlowAlgorithm(
			static_cast<GraphCutFilterType::eMaxFlowAlgorithm>(
					m_MaxFlowAlgorithm->currentItem()));
	graphCutFilter->SetForegroundPixelValue(255);
	graphCutFilter->SetBackgroundPixelValue(0);
	graphCutFilter->SetSigma(0.2);

	if (m_6Connectivity->isChecked())
		graphCutFilter->SetConnectivity(true);

	// assumes input image is 3D
	if (input->GetLargestPossibleRegion().GetSize(2) > 1)
	{
		m_CurrentFilter = graphCutFilter;

		auto observer = CommandProgressUpdate<GraphCutFilterType>::New();
		auto observer_id = graphCutFilter->AddObserver(itk::ProgressEvent(), observer);
		observer->SetProgressObject(&progress);

		try
		{
			graphCutFilter->Update();

			auto output = graphCutFilter->GetOutput();

			auto target = itk_wrapper.GetTarget(m_UseSliceRange->isChecked());

			iseg::DataSelection dataSelection;
			dataSelection.allSlices = true;
			dataSelection.work = true;
			emit begin_datachange(dataSelection, this);

			iseg::Paste<TOutput, iseg::SliceHandlerItkWrapper::image_ref_type>(output, target);

			emit end_datachange(this);
		}
		catch (itk::ExceptionObject&)
		{
		}

		m_CurrentFilter = nullptr;
	}
}

void BoneSegmentationWidget::cancel()
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

void BoneSegmentationWidget::on_slicenr_changed()
{
	m_CurrentSlice = m_Handler3D->active_slice();
}

void BoneSegmentationWidget::init()
{
	on_slicenr_changed();
	hideparams_changed();
}

void BoneSegmentationWidget::newloaded()
{
	m_CurrentSlice = m_Handler3D->active_slice();
}
