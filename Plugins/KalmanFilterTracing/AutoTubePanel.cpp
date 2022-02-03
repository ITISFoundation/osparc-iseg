//
//  AutoTubePanel.cpp
//  iSegData
//
//  Created by Benjamin Fuhrer on 13.10.18.
//

#include "AutoTubePanel.h"

#include "Data/ItkUtils.h"
#include "Data/Logger.h"
#include "Data/SlicesHandlerITKInterface.h"

#include "Core/Morpho.h"

#include <QFileDialog>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QScrollArea>
#include <QVBoxLayout>

#include "Thirdparty/IJ/BinaryThinningImageFilter3D/itkBinaryThinningImageFilter3D.h"
#include "Thirdparty/IJ/NonMaxSuppression/itkNonMaxSuppressionImageFilter.h"

#include <itkBinaryThinningImageFilter.h>
#include <itkBinaryThresholdImageFilter.h>
#include <itkConnectedComponentImageFilter.h>
#include <itkHessianToObjectnessMeasureImageFilter.h>
#include <itkLabelImageToShapeLabelMapFilter.h>
#include <itkMultiScaleHessianBasedMeasureImageFilter.h>
#include <itkRelabelComponentImageFilter.h>
#include <itkRescaleIntensityImageFilter.h>
#include <itkThresholdImageFilter.h>

#include <itkBinaryImageToLabelMapFilter.h>
#include <itkChangeLabelLabelMapFilter.h>
#include <itkLabelMapToBinaryImageFilter.h>
#include <itkMergeLabelMapFilter.h>
#include <itkNeighborhoodAlgorithm.h>
#include <itkShapedNeighborhoodIterator.h>

#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include <Eigen/Dense>

#include <algorithm>
#include <cmath>
#include <fstream>

using boost::adaptors::transformed;
using boost::algorithm::join;

namespace iseg {

template<typename TInputImage, typename TOutputImage, unsigned int Dimension>
class BinaryThinningImageFilter : public itk::BinaryThinningImageFilter<TInputImage, TOutputImage>
{
};

template<typename TInputImage, typename TOutputImage>
class BinaryThinningImageFilter<TInputImage, TOutputImage, 3> : public itk::BinaryThinningImageFilter3D<TInputImage, TOutputImage>
{
};

using label_type = unsigned long;
using label_object_type = itk::ShapeLabelObject<label_type, 2>;
using label_map_type = itk::LabelMap<label_object_type>;

AutoTubePanel::AutoTubePanel(iseg::SlicesHandlerInterface* hand3D)
		: m_Handler3D(hand3D)
{
	setToolTip(Format("Kalman filter based root tracer"));

	m_ExecuteButton = new QPushButton("Execute");
	m_ExecuteButton->setMaximumSize(m_ExecuteButton->minimumSizeHint());
	m_RemoveButton = new QPushButton("Remove Object");
	m_RemoveButton->setMaximumSize(m_RemoveButton->minimumSizeHint());
	m_ExtrapolateButton = new QPushButton("Extrapolate");
	m_ExtrapolateButton->setMaximumSize(m_ExtrapolateButton->minimumSizeHint());
	m_MergeButton = new QPushButton("Merge Selected List Items");
	m_MergeButton->setMaximumSize(m_MergeButton->minimumSizeHint());
	m_SelectObjectsButton = new QPushButton("Select Mask");
	m_SelectObjectsButton->setMaximumSize(m_SelectObjectsButton->minimumSizeHint());
	m_VisualizeButton = new QPushButton("See Label Map");
	m_UpdateKfilterButton = new QPushButton("Update Kalman Filters");
	m_RemoveKFilter = new QPushButton("Remove Kalman Filter");
	m_Save = new QPushButton("Save Parameters");
	m_Load = new QPushButton("Load Parameters");
	m_RemoveNonSelected = new QPushButton("Remove Non Selected");
	m_AddToTissues = new QPushButton("Add To Tissues");
	m_ExportLines = new QPushButton("Export lines");
	m_KFilterPredict = new QPushButton("Predict Root Positions");

	int width(80);

	QLabel* min = new QLabel("Sigma Min");
	QLabel* max = new QLabel("Sigma Max");
	QLabel* num = new QLabel("Number of Sigmas");
	QLabel* feature_th = new QLabel("Feature Threshold");
	QLabel* non_max = new QLabel("Non-maximum Suppression");
	QLabel* add_l = new QLabel("Add To Existing Label Map");
	QLabel* centerlines = new QLabel("Centerlines");
	QLabel* min_obj_size = new QLabel("Min Object Size");
	QLabel* slice_l = new QLabel("Slice Object List");
	QLabel* obj_prob_l = new QLabel("Probability List");
	QLabel* min_p = new QLabel("Minimum Probability");
	QLabel* w_d = new QLabel("Weight: Distance");
	QLabel* w_pa = new QLabel("Weight: Parameters");
	QLabel* w_pr = new QLabel("Weight: Prediction");
	QLabel* limit_s = new QLabel("Extrapolate To Slice");
	QLabel* k_filters_list = new QLabel("Kalman Filter List");
	QLabel* r_k_filter = new QLabel("Restart Kalman Filter");
	QLabel* connect_d = new QLabel("Connect Dots");
	QLabel* extra_only = new QLabel("Keep Only Matches");
	QLabel* add_pix = new QLabel("Add Pixel");
	QLabel* line_radius_l = new QLabel("Line Radius");

	m_SigmaLow = new QLineEdit(QString::number(0.3));
	m_SigmaLow->setValidator(new QDoubleValidator);
	m_SigmaLow->setFixedWidth(width);

	m_MinProbability = new QLineEdit(QString::number(0.5));
	m_MinProbability->setValidator(new QDoubleValidator);
	m_MinProbability->setFixedWidth(width);

	m_WDistance = new QLineEdit(QString::number(0.3));
	m_WDistance->setValidator(new QDoubleValidator);
	m_WDistance->setFixedWidth(width);

	m_WParams = new QLineEdit(QString::number(0.3));
	m_WParams->setValidator(new QDoubleValidator);
	m_WParams->setFixedWidth(width);

	m_WPred = new QLineEdit(QString::number(0.3));
	m_WPred->setValidator(new QDoubleValidator);
	m_WPred->setFixedWidth(width);

	m_SigmaHi = new QLineEdit(QString::number(0.6));
	m_SigmaHi->setValidator(new QDoubleValidator);
	m_SigmaHi->setFixedWidth(width);

	m_NumberSigmaLevels = new QLineEdit(QString::number(2));
	m_NumberSigmaLevels->setValidator(new QIntValidator);
	m_NumberSigmaLevels->setFixedWidth(width);

	m_Threshold = new QLineEdit(QString::number(3));
	m_Threshold->setValidator(new QDoubleValidator);
	m_Threshold->setFixedWidth(width);

	m_MaxRadius = new QLineEdit(QString::number(1));
	m_MaxRadius->setValidator(new QDoubleValidator);
	m_MaxRadius->setFixedWidth(width);

	m_MinObjectSize = new QLineEdit(QString::number(1));
	m_MinObjectSize->setValidator(new QIntValidator);
	m_MinObjectSize->setFixedWidth(width);

	m_SelectedObjects = new QLineEdit;
	m_SelectedObjects->setReadOnly(true);
	m_SelectedObjects->setFixedWidth(width);

	m_LimitSlice = new QLineEdit();
	m_LimitSlice->setValidator(new QIntValidator);
	m_LimitSlice->setFixedWidth(width);

	m_LineRadius = new QLineEdit();
	m_LineRadius->setValidator(new QDoubleValidator);
	m_LineRadius->setFixedWidth(width);

	m_Skeletonize = new QCheckBox;
	m_Skeletonize->setChecked(true);
	m_Skeletonize->setToolTip(Format("Compute 1-pixel wide centerlines (skeleton)."));

	m_ConnectDots = new QCheckBox;
	m_ConnectDots->setChecked(false);

	m_NonMaxSuppression = new QCheckBox;
	m_NonMaxSuppression->setChecked(true);
	m_NonMaxSuppression->setToolTip(Format("Extract approx. one pixel wide paths based on non-maximum suppression."));

	m_Add = new QCheckBox;
	m_Add->setChecked(false);
	m_Add->setToolTip(Format("Add New Objects To Label Map"));

	m_RestartKFilter = new QCheckBox;
	m_RestartKFilter->setChecked(false);

	m_ExtrapolateOnlyMatches = new QCheckBox;
	m_ExtrapolateOnlyMatches->setChecked(true);

	m_AddPixel = new QCheckBox;
	m_AddPixel->setChecked(false);

	m_ObjectList = new QListWidget();
	m_ObjectList->setSelectionMode(QAbstractItemView::ExtendedSelection);

	m_ObjectProbabilityList = new QListWidget();
	m_ObjectProbabilityList->setSelectionMode(QAbstractItemView::ExtendedSelection);

	m_KFiltersList = new QListWidget();
	m_KFiltersList->setSelectionMode(QAbstractItemView::ExtendedSelection);

	QVBoxLayout* vbox1 = new QVBoxLayout;
	vbox1->addWidget(slice_l);
	vbox1->addWidget(m_ObjectList);

	QVBoxLayout* vbox2 = new QVBoxLayout;
	vbox2->addWidget(limit_s);
	vbox2->addWidget(m_LimitSlice);
	vbox2->addWidget(m_SelectObjectsButton);
	vbox2->addWidget(m_SelectedObjects);
	vbox2->addWidget(add_l);
	vbox2->addWidget(m_Add);
	vbox2->addWidget(r_k_filter);
	vbox2->addWidget(m_RestartKFilter);
	vbox2->addWidget(extra_only);
	vbox2->addWidget(m_ExtrapolateOnlyMatches);
	vbox2->addWidget(add_pix);
	vbox2->addWidget(m_AddPixel);

	QVBoxLayout* vbox3 = new QVBoxLayout;
	vbox3->addWidget(k_filters_list);
	vbox3->addWidget(m_KFiltersList);

	QVBoxLayout* vbox4 = new QVBoxLayout;
	vbox4->addWidget(m_RemoveNonSelected);
	vbox4->addWidget(m_UpdateKfilterButton);
	vbox4->addWidget(m_VisualizeButton);
	vbox4->addWidget(m_Save);
	vbox4->addWidget(m_Load);
	vbox4->addWidget(m_AddToTissues);
	vbox4->addWidget(m_ExportLines);

	QHBoxLayout* hbox1 = new QHBoxLayout;
	hbox1->addLayout(vbox4);
	hbox1->addLayout(vbox1);
	hbox1->addLayout(vbox2);
	hbox1->addLayout(vbox3);

	QVBoxLayout* vbox5 = new QVBoxLayout;
	vbox5->addWidget(min);
	vbox5->addWidget(m_SigmaLow);
	vbox5->addWidget(max);
	vbox5->addWidget(m_SigmaHi);
	vbox5->addWidget(num);
	vbox5->addWidget(m_NumberSigmaLevels);
	vbox5->addWidget(feature_th);
	vbox5->addWidget(m_Threshold);

	QVBoxLayout* vbox6 = new QVBoxLayout;
	vbox6->addWidget(non_max);
	vbox6->addWidget(m_NonMaxSuppression);
	vbox6->addWidget(centerlines);
	vbox6->addWidget(m_Skeletonize);
	vbox6->addWidget(min_obj_size);
	vbox6->addWidget(m_MinObjectSize);
	vbox6->addWidget(connect_d);
	vbox6->addWidget(m_ConnectDots);
	vbox6->addWidget(line_radius_l);
	vbox6->addWidget(m_LineRadius);

	QVBoxLayout* vbox8 = new QVBoxLayout;
	vbox8->addWidget(min_p);
	vbox8->addWidget(m_MinProbability);
	vbox8->addWidget(w_d);
	vbox8->addWidget(m_WDistance);
	vbox8->addWidget(w_pa);
	vbox8->addWidget(m_WParams);
	vbox8->addWidget(w_pr);
	vbox8->addWidget(m_WPred);

	QVBoxLayout* vbox9 = new QVBoxLayout;
	vbox9->addWidget(obj_prob_l);
	vbox9->addWidget(m_ObjectProbabilityList);

	QHBoxLayout* hbox2 = new QHBoxLayout;
	hbox2->addLayout(vbox5);
	hbox2->addLayout(vbox6);
	hbox2->addLayout(vbox9);
	hbox2->addLayout(vbox8);

	QHBoxLayout* hbox3 = new QHBoxLayout;
	hbox3->addWidget(m_MergeButton);
	hbox3->addWidget(m_RemoveButton);
	hbox3->addWidget(m_RemoveKFilter);
	hbox3->addWidget(m_ExtrapolateButton);
	hbox3->addWidget(m_KFilterPredict);
	hbox3->addWidget(m_ExecuteButton);

	auto layout = new QFormLayout;
	layout->addRow(hbox3);
	layout->addRow(hbox1);
	layout->addRow(hbox2);

	auto big_view = new QWidget;
	big_view->setLayout(layout);

	auto scroll_area = new QScrollArea(this);
	scroll_area->setWidget(big_view);

	auto top_layout = new QGridLayout;
	top_layout->addWidget(scroll_area, 0, 0);
	setLayout(top_layout);

	QObject_connect(m_SelectObjectsButton, SIGNAL(clicked()), this, SLOT(SelectObjects()));
	QObject_connect(m_ExecuteButton, SIGNAL(clicked()), this, SLOT(DoWork()));
	QObject_connect(m_ObjectList, SIGNAL(itemSelectionChanged()), this, SLOT(ItemSelected()));
	QObject_connect(m_ObjectList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(ItemDoubleClicked(QListWidgetItem*)));
	QObject_connect(m_MergeButton, SIGNAL(clicked()), this, SLOT(MergeSelectedItems()));
	QObject_connect(m_ExtrapolateButton, SIGNAL(clicked()), this, SLOT(ExtrapolateResults()));
	QObject_connect(m_VisualizeButton, SIGNAL(clicked()), this, SLOT(Visualize()));
	QObject_connect(m_UpdateKfilterButton, SIGNAL(clicked()), this, SLOT(UpdateKalmanFilters()));
	QObject_connect(m_RemoveButton, SIGNAL(clicked()), this, SLOT(RemoveObject()));
	QObject_connect(m_RemoveKFilter, SIGNAL(clicked()), this, SLOT(RemoveKFilter()));
	QObject_connect(m_KFiltersList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(KFilterDoubleClicked(QListWidgetItem*)));
	QObject_connect(m_Save, SIGNAL(clicked()), this, SLOT(Save()));
	QObject_connect(m_Load, SIGNAL(clicked()), this, SLOT(Load()));
	QObject_connect(m_RemoveNonSelected, SIGNAL(clicked()), this, SLOT(RemoveNonSelected()));
	QObject_connect(m_AddToTissues, SIGNAL(clicked()), this, SLOT(AddToTissues()));
	QObject_connect(m_ExportLines, SIGNAL(clicked()), this, SLOT(ExportLines()));
	QObject_connect(m_KFilterPredict, SIGNAL(clicked()), this, SLOT(PredictKFilter()));
}

void AutoTubePanel::PredictKFilter()
{
	if (m_KFilters.empty())
	{
		QMessageBox m_box;
		m_box.setWindowTitle("Error");
		m_box.setText("Cannot predict without any Kalman filters!");
		m_box.exec();
		return;
	}

	m_CachedData.Get(m_LabelMaps, m_Objects, m_LabelToText, m_KFilters, m_Probabilities, m_MaxActiveSliceReached);
	using pixel_type = float;
	using image_type = itk::Image<pixel_type, 2>;

	for (const auto& filter : m_KFilters)
	{

		if (!m_LabelMaps[m_Handler3D->ActiveSlice()])
		{
			image_type::SizeType size{{384, 384}};
			label_map_type::Pointer map = label_map_type::New();
			itk::Index<2> index{{0, 0}};

			image_type::RegionType region;
			region.SetSize(size);
			region.SetIndex(index);

			map->SetRegions(region);
			map->Allocate(true);
			map->Update();
			m_LabelMaps[m_Handler3D->ActiveSlice()] = map;

			std::vector<std::string> list;
			m_Objects[m_Handler3D->ActiveSlice()] = list;
		}

		auto label_map = m_LabelMaps[m_Handler3D->ActiveSlice()];

		using Label2ImageType = itk::LabelMapToLabelImageFilter<label_map_type, image_type>;
		auto label2image = Label2ImageType::New();
		label2image->SetInput(label_map);
		label2image->Update();
		auto image = label2image->GetOutput();

		KalmanFilter copy = filter;
		copy.MeasurementPrediction();
		std::vector<double> prediction = copy.GetPrediction();

		itk::Index<2> index;
		index[0] = prediction[0];
		index[1] = prediction[1];

		label_type last_label = 0;
		if (!label_map->GetLabels().empty())
			last_label = label_map->GetLabels().back();

		if (image->GetBufferedRegion().IsInside(index))
		{
			label_type found_label = label_map->GetPixel(index);

			if (found_label == 0)
			{
				image->SetPixel(index, last_label + 1);

				image->Update();

				using Image2LabelType = itk::LabelImageToShapeLabelMapFilter<image_type, label_map_type>;
				auto image2label = Image2LabelType::New();
				image2label->SetInput(image);
				image2label->Update();

				m_LabelMaps[m_Handler3D->ActiveSlice()] = image2label->GetOutput();

				m_Objects[m_Handler3D->ActiveSlice()].push_back(filter.GetLabel());
			}
		}
	}

	auto label_objects = m_LabelMaps[m_Handler3D->ActiveSlice()]->GetLabelObjects();
	m_LabelMaps[m_Handler3D->ActiveSlice()]->ClearLabels();
	for (auto& object : label_objects)
		m_LabelMaps[m_Handler3D->ActiveSlice()]->PushLabelObject(object);

	RefreshObjectList();
	VisualizeLabelMap(m_LabelMaps[m_Handler3D->ActiveSlice()]);
	m_CachedData.Store(m_LabelMaps, m_Objects, m_LabelToText, m_KFilters, m_Probabilities, m_MaxActiveSliceReached);
}

void AutoTubePanel::AddToTissues()
{
	m_CachedData.Get(m_LabelMaps, m_Objects, m_LabelToText, m_KFilters, m_Probabilities, m_MaxActiveSliceReached);
	QMessageBox m_box;
	m_box.setWindowTitle("");
	m_box.setText("Before continuing make sure that all the roots were found and labeled in all slices and that they all have kalman filters. In addition, make sure that all of these roots have tissues with their name before continuing");
	m_box.addButton(QMessageBox::No);
	m_box.addButton(QMessageBox::Yes);
	m_box.setDefaultButton(QMessageBox::No);
	if (m_box.exec() == QMessageBox::Yes)
	{
		if (!m_KFilters.empty() && m_MaxActiveSliceReached == m_Handler3D->NumSlices() - 1)
		{
			// get Kalman filter labels
			std::vector<std::string> labels;
			for (const auto& filter : m_KFilters)
				labels.push_back(filter.GetLabel());

			iseg::SlicesHandlerITKInterface itk_handler(m_Handler3D);
			auto tissue_names = m_Handler3D->TissueNames();

			// shallow copy - used to transform index to world coordinates
			auto image_3d = itk_handler.GetSource(false);

			using pixel_type = float;
			using tissue_type = SlicesHandlerInterface::tissue_type;
			using image_type = itk::Image<pixel_type, 2>;
			using const_iterator_type = itk::ImageRegionConstIterator<image_type>;
			using iterator_type = itk::ImageRegionIterator<itk::Image<tissue_type, 2>>;

			std::ofstream myfile;
			for (const auto& label : labels)
			{
				std::string filename = label + ".txt";
				myfile.open(filename);
				std::vector<std::string>::iterator it = std::find(tissue_names.begin(), tissue_names.end(), label);
				if (it == labels.end())
				{
					QMessageBox m_box;
					m_box.setWindowTitle("Error");
					m_box.setText("Roots have no tissues! First create a tissue in the tissue list!");
					m_box.exec();
					myfile.close();
					break;
				}

				bool draw_circle;
				double circle_radius = m_LineRadius->text().toDouble(&draw_circle);
				auto spacing = itk_handler.GetTissuesSlice(0)->GetSpacing();
				auto ball = iseg::details::MakeBall<2>(spacing, draw_circle ? circle_radius : 1.0);

				tissues_size_t tissue_number = std::distance(tissue_names.begin(), it);
				for (int i(0); i < m_Handler3D->NumSlices(); i++)
				{
					itk::Image<tissue_type, 2>::Pointer tissue = itk_handler.GetTissuesSlice(i);
					auto label_map = m_LabelMaps[i];

					label_map = CalculateLabelMapParams(label_map);

					it = std::find(m_Objects[i].begin(), m_Objects[i].end(), label);
					if (it != m_Objects[i].end())
					{
						int row = std::distance(m_Objects[i].begin(), it);
						auto label_object = label_map->GetLabelObject(row + 1);

						using Label2ImageType = itk::LabelMapToLabelImageFilter<label_map_type, image_type>;
						auto label2image = Label2ImageType::New();
						label2image->SetInput(label_map);
						label2image->Update();

						const_iterator_type in(label2image->GetOutput(), label2image->GetOutput()->GetRequestedRegion());
						iterator_type out(tissue, label2image->GetOutput()->GetRequestedRegion());
						double x(0);
						double y(0);
						int size(0);
						for (in.GoToBegin(), out.GoToBegin(); !in.IsAtEnd(); ++in, ++out)
						{
							if (label_map->GetPixel(in.GetIndex()) == label_object->GetLabel())
							{
								if (!draw_circle) // copy to tissues
									out.Set(tissue_number);

								x += in.GetIndex()[0];
								y += in.GetIndex()[1];
								size += 1;
							}
						}

						// calculate centroid
						double x_centroid = x / size;
						double y_centroid = y / size;

						itk::Point<double, 3> point;
						itk::Index<3> ind;
						ind[0] = x_centroid;
						ind[1] = y_centroid;
						ind[2] = i;

						if (draw_circle) // draw perfect circle
						{
							using ShapedNeighborhoodIterator = itk::ShapedNeighborhoodIterator<itk::Image<tissue_type, 2>>;
							ShapedNeighborhoodIterator siterator(ball.GetRadius(), tissue, tissue->GetLargestPossibleRegion());

							//siterator.CreateActiveListFromNeighborhood(ball); // does not work because ball has incorrect neighborhood type
							size_t idx = 0;
							for (auto nit = ball.Begin(); nit != ball.End(); ++nit, ++idx)
							{
								if (*nit)
									siterator.ActivateOffset(siterator.GetOffset(idx));
								else
									siterator.DeactivateOffset(siterator.GetOffset(idx));
							}

							siterator.NeedToUseBoundaryConditionOn();

							ShapedNeighborhoodIterator::IndexType location;
							location[0] = x_centroid;
							location[1] = y_centroid;
							siterator.SetLocation(location);
							for (auto i = siterator.Begin(); !i.IsAtEnd(); ++i)
							{
								i.Set(tissue_number);
							}
						}

						image_3d->TransformIndexToPhysicalPoint(ind, point);
						myfile << point[0] << "\t" << point[1] << "\t" << point[2] << "\n";
					}
				}
				myfile.close();
			}

			iseg::DataSelection data_selection;
			data_selection.allSlices = true;
			data_selection.tissues = true;

			emit BeginDatachange(data_selection, this);
			emit EndDatachange(this);

			QMessageBox m_box;
			m_box.setWindowTitle("");
			m_box.setText("Done");
			m_box.exec();
		}
		else
		{
			QMessageBox m_box;
			m_box.setWindowTitle("Error");
			m_box.setText("Only Click When All Roots are found in all slices and all have kalman filters in the kalman filter list!");
			m_box.exec();
		}
	}
}

void AutoTubePanel::ExportLines()
{
	auto directory = QFileDialog::getExistingDirectory(this, "Open directory");
	if (directory.isEmpty())
		return;

	// get Kalman filter labels
	std::vector<std::string> labels;
	for (const auto& filter : m_KFilters)
		labels.push_back(filter.GetLabel());

	iseg::SlicesHandlerITKInterface itk_handler(m_Handler3D);
	auto tissue_names = m_Handler3D->TissueNames();

	// shallow copy - used to transform index to world coordinates
	auto image_3d = itk_handler.GetSource(false);

	using pixel_type = float;
	using image_type = itk::Image<pixel_type, 2>;
	using const_iterator_type = itk::ImageRegionConstIterator<image_type>;

	std::ofstream myfile;
	for (const auto& label : labels)
	{
		std::string filename = directory.toStdString() + label + ".txt";
		myfile.open(filename);
		auto it = std::find(tissue_names.begin(), tissue_names.end(), label);
		if (it == labels.end())
		{
			QMessageBox m_box;
			m_box.setWindowTitle("Error");
			m_box.setText("Roots have no tissues! First create a tissue in the tissue list!");
			m_box.exec();
			myfile.close();
			continue;
		}

		for (int i(0); i < m_Handler3D->NumSlices(); i++)
		{
			auto tissue = itk_handler.GetTissuesSlice(i);
			auto label_map = m_LabelMaps[i];

			label_map = CalculateLabelMapParams(label_map);

			it = std::find(m_Objects[i].begin(), m_Objects[i].end(), label);
			if (it != m_Objects[i].end())
			{
				int row = std::distance(m_Objects[i].begin(), it);
				auto label_object = label_map->GetLabelObject(row + 1);

				using Label2ImageType = itk::LabelMapToLabelImageFilter<label_map_type, image_type>;
				auto label2image = Label2ImageType::New();
				label2image->SetInput(label_map);
				label2image->Update();

				const_iterator_type in(label2image->GetOutput(), label2image->GetOutput()->GetRequestedRegion());
				double x(0);
				double y(0);
				int size(0);
				for (in.GoToBegin(); !in.IsAtEnd(); ++in)
				{
					// why do we go via iterator to labelMap?
					if (label_map->GetPixel(in.GetIndex()) == label_object->GetLabel())
					{
						x += in.GetIndex()[0];
						y += in.GetIndex()[1];
						size += 1;
					}
				}

				// calculate centroid
				double x_centroid = x / size;
				double y_centroid = y / size;

				itk::Point<double, 3> point;
				itk::Index<3> ind;
				ind[0] = x_centroid;
				ind[1] = y_centroid;
				ind[2] = i;

				image_3d->TransformIndexToPhysicalPoint(ind, point);
				myfile << point[0] << "\t" << point[1] << "\t" << point[2] << "\n";
			}
		}
		myfile.close();
	}
}

// function for the Remove Non Selected button
void AutoTubePanel::RemoveNonSelected()
{
	// get cached information
	m_CachedData.Get(m_LabelMaps, m_Objects, m_LabelToText, m_KFilters, m_Probabilities, m_MaxActiveSliceReached);

	// making sure that they are selected objects
	if (!m_Selected.empty())
	{
		std::vector<int> rows_to_delete;

		// find all rows that are not selected
		for (int i(0); i < m_Objects[m_Handler3D->ActiveSlice()].size(); i++)
		{
			std::vector<int>::iterator it = std::find(m_Selected.begin(), m_Selected.end(), i);
			if (it == m_Selected.end())
				rows_to_delete.push_back(i);
		}

		// delete rows to delete from current slice's object list
		// delete in descending order, i.e. from back of vector
		std::sort(rows_to_delete.begin(), rows_to_delete.end(), std::greater<int>());
		for (auto row : rows_to_delete)
		{
			m_LabelMaps[m_Handler3D->ActiveSlice()]->RemoveLabel(row + 1);
			m_Objects[m_Handler3D->ActiveSlice()].erase(m_Objects[m_Handler3D->ActiveSlice()].begin() + row);
		}

		// delete rows to delete from current slice's probabilities list
		if (m_Handler3D->ActiveSlice() != 0)
		{
			for (auto row : rows_to_delete)
			{
				m_Probabilities[m_Handler3D->ActiveSlice()] = ModifyProbability(m_Probabilities[m_Handler3D->ActiveSlice()], std::to_string(row + 1), true, false);
			}
		}

		auto label_objects = m_LabelMaps[m_Handler3D->ActiveSlice()]->GetLabelObjects();

		// rename elements in probabilities list to correspond to the new indexes of remaining objects
		if (m_Handler3D->ActiveSlice() != 0)
		{
			for (unsigned int i(0); i < m_LabelMaps[m_Handler3D->ActiveSlice()]->GetNumberOfLabelObjects(); i++)
			{
				m_Probabilities[m_Handler3D->ActiveSlice()] = ModifyProbability(m_Probabilities[m_Handler3D->ActiveSlice()], std::to_string(m_LabelMaps[m_Handler3D->ActiveSlice()]->GetNthLabelObject(i)->GetLabel()), false, true, std::to_string(i + 1));
			}
		}
		// rename remaining labels of label map to 1 until number of label objects
		m_LabelMaps[m_Handler3D->ActiveSlice()]->ClearLabels();
		for (auto& object : label_objects)
			m_LabelMaps[m_Handler3D->ActiveSlice()]->PushLabelObject(object);

		RefreshObjectList();
		RefreshProbabilityList();

		VisualizeLabelMap(m_LabelMaps[m_Handler3D->ActiveSlice()]);

		m_CachedData.Store(m_LabelMaps, m_Objects, m_LabelToText, m_KFilters, m_Probabilities, m_MaxActiveSliceReached);

		m_Selected.clear();
	}
}

void AutoTubePanel::SaveLabelMap(FILE* fp, label_map_type::Pointer map)
{

	using pixel_type = float;
	using image_type = itk::Image<pixel_type, 2>;

	using label2image_type = itk::LabelMapToLabelImageFilter<label_map_type, image_type>;
	auto label2image = label2image_type::New();
	label2image->SetInput(map);

	using const_iterator_type = itk::ImageRegionConstIterator<image_type>;
	SAFE_UPDATE(label2image, return );
	int dummy;

	// save size
	dummy = label2image->GetOutput()->GetRequestedRegion().GetSize()[0];
	fwrite(&dummy, sizeof(int), 1, fp);

	dummy = label2image->GetOutput()->GetRequestedRegion().GetSize()[1];
	fwrite(&dummy, sizeof(int), 1, fp);
	// GetPixel() returns the label for the given index -> we save the labels for each pixel
	const_iterator_type in(label2image->GetOutput(), label2image->GetOutput()->GetRequestedRegion());
	for (in.GoToBegin(); !in.IsAtEnd(); ++in)
	{
		dummy = map->GetPixel(in.GetIndex());
		fwrite(&dummy, sizeof(int), 1, fp);
	}

	std::map<label_type, std::vector<double>> params = GetLabelMapParams(map);
	// saving paramters of label objects
	for (const auto& it : params)
	{
		for (auto parameter : it.second)
		{
			double temp = parameter;
			fwrite(&temp, sizeof(double), 1, fp);
		}
	}
}

void AutoTubePanel::SaveLToT(FILE* fp, std::map<label_type, std::string> l_to_t)
{
	// save labelto text mapping

	int size = l_to_t.size();
	fwrite(&size, sizeof(int), 1, fp);

	for (auto const& element : l_to_t)
	{
		fwrite(&(element.first), sizeof(label_type), 1, fp);
		fwrite(&(element.second), sizeof(std::string), 1, fp);
	}
}
std::map<label_type, std::string> AutoTubePanel::LoadLToT(FILE* fi)
{

	// load label to text mapping

	std::map<label_type, std::string> mapping;
	int size;
	fread(&size, sizeof(int), 1, fi);

	for (unsigned int j(0); j < size; j++)
	{
		label_type label;
		fread(&label, sizeof(label_type), 1, fi);
		std::string text;
		fread(&text, sizeof(std::string), 1, fi);
		mapping[label] = text;
	}
	return mapping;
}

label_map_type::Pointer AutoTubePanel::LoadLabelMap(FILE* fi)
{
	label_map_type::Pointer map = label_map_type::New();

	int x;
	int y;
	// load size
	fread(&x, sizeof(int), 1, fi);
	fread(&y, sizeof(int), 1, fi);

	int value;
	itk::Index<2> index{{0, 0}};

	using pixel_type = float;
	using image_type = itk::Image<pixel_type, 2>;
	image_type::SizeType size{{384, 384}};

	size[0] = x;
	size[1] = y;

	image_type::RegionType region;
	region.SetSize(size);
	region.SetIndex(index);

	map->SetRegions(region);
	map->Allocate(true);
	map->Update();

	using label2image_type = itk::LabelMapToLabelImageFilter<label_map_type, image_type>;
	auto label2image = label2image_type::New();
	label2image->SetInput(map);
	label2image->Update();

	using const_iterator_type = itk::ImageRegionConstIterator<image_type>;
	const_iterator_type in(label2image->GetOutput(), label2image->GetOutput()->GetRequestedRegion());
	for (in.GoToBegin(); !in.IsAtEnd(); ++in)
	{
		fread(&value, sizeof(int), 1, fi);
		map->SetPixel(in.GetIndex(), value);
	}

	map = CalculateLabelMapParams(map);

	double param;
	for (auto& label_object : map->GetLabelObjects())
	{
		double centroid_x;
		double centroid_y;
		fread(&centroid_x, sizeof(double), 1, fi);
		fread(&centroid_y, sizeof(double), 1, fi);
		itk::Point<double, 2> centroid;
		centroid[0] = centroid_x;
		centroid[1] = centroid_y;
		label_object->SetCentroid(centroid);
		fread(&param, sizeof(double), 1, fi);
		label_object->SetEquivalentSphericalPerimeter(param);
		fread(&param, sizeof(double), 1, fi);
		label_object->SetEquivalentSphericalRadius(param);
		fread(&param, sizeof(double), 1, fi);
		label_object->SetFeretDiameter(param);
		fread(&param, sizeof(double), 1, fi);
		label_object->SetFlatness(param);
		fread(&param, sizeof(double), 1, fi);
		label_object->SetNumberOfPixels(param);
		fread(&param, sizeof(double), 1, fi);
		label_object->SetNumberOfPixelsOnBorder(param);
		fread(&param, sizeof(double), 1, fi);
		label_object->SetPerimeter(param);
		fread(&param, sizeof(double), 1, fi);
		label_object->SetPerimeterOnBorder(param);
		fread(&param, sizeof(double), 1, fi);
		label_object->SetPerimeterOnBorderRatio(param);
		fread(&param, sizeof(double), 1, fi);
		label_object->SetPhysicalSize(param);
		fread(&param, sizeof(double), 1, fi);
		label_object->SetRoundness(param);
	}

	map->Update();

	return map;
}

void AutoTubePanel::SaveKFilter(FILE* fp, std::vector<KalmanFilter> filters)
{
	int size = filters.size();
	fwrite(&size, sizeof(int), 1, fp);
	for (auto const& element : filters)
	{

		Eigen::VectorXd z = element.GetZ();
		for (unsigned int i(0); i < z.size(); i++)
		{
			fwrite(&z(i), sizeof(double), 1, fp);
		}
		Eigen::VectorXd z_hat = element.GetZHat();
		for (unsigned int i(0); i < z_hat.size(); i++)
		{
			fwrite(&z_hat(i), sizeof(double), 1, fp);
		}

		Eigen::VectorXd x = element.GetX();
		for (unsigned int i(0); i < x.size(); i++)
		{
			fwrite(&x(i), sizeof(double), 1, fp);
		}

		Eigen::VectorXd v = element.GetV();
		for (unsigned int i(0); i < v.size(); i++)
		{
			fwrite(&v(i), sizeof(double), 1, fp);
		}

		Eigen::VectorXd n = element.GetN();
		for (unsigned int i(0); i < n.size(); i++)
		{
			fwrite(&n(i), sizeof(double), 1, fp);
		}

		Eigen::VectorXd m = element.GetM();
		for (unsigned int i(0); i < m.size(); i++)
		{
			fwrite(&m(i), sizeof(double), 1, fp);
		}

		Eigen::MatrixXd f = element.GetF();
		for (unsigned int i(0); i < f.rows(); i++)
		{
			for (unsigned int j(0); j < f.cols(); j++)
			{
				fwrite(&f(i, j), sizeof(double), 1, fp);
			}
		}

		Eigen::MatrixXd h = element.GetH();
		for (unsigned int i(0); i < h.rows(); i++)
		{
			for (unsigned int j(0); j < h.cols(); j++)
			{
				fwrite(&h(i, j), sizeof(double), 1, fp);
			}
		}

		Eigen::MatrixXd p = element.GetP();
		for (unsigned int i(0); i < p.rows(); i++)
		{
			for (unsigned int j(0); j < p.cols(); j++)
			{
				fwrite(&p(i, j), sizeof(double), 1, fp);
			}
		}

		Eigen::MatrixXd q = element.GetQ();
		for (unsigned int i(0); i < q.rows(); i++)
		{
			for (unsigned int j(0); j < q.cols(); j++)
			{
				fwrite(&q(i, j), sizeof(double), 1, fp);
			}
		}

		Eigen::MatrixXd r = element.GetR();
		for (unsigned int i(0); i < r.rows(); i++)
		{
			for (unsigned int j(0); j < r.cols(); j++)
			{
				fwrite(&r(i, j), sizeof(double), 1, fp);
			}
		}

		Eigen::MatrixXd w = element.GetW();
		for (unsigned int i(0); i < w.rows(); i++)
		{
			for (unsigned int j(0); j < w.cols(); j++)
			{
				fwrite(&w(i, j), sizeof(double), 1, fp);
			}
		}

		Eigen::MatrixXd s = element.GetS();
		for (unsigned int i(0); i < s.rows(); i++)
		{
			for (unsigned int j(0); j < s.cols(); j++)
			{
				fwrite(&s(i, j), sizeof(double), 1, fp);
			}
		}
		int slice = element.GetSlice();
		int iteration = element.GetIteration();
		int last_slice = element.GetLastSlice();
		std::string label = element.GetLabel();

		fwrite(&slice, sizeof(int), 1, fp);
		fwrite(&iteration, sizeof(int), 1, fp);
		fwrite(&last_slice, sizeof(int), 1, fp);
		int nstring = label.size();
		fwrite(&nstring, sizeof(int), 1, fp);
		fwrite(label.c_str(), 1, nstring, fp);
	}
}

std::vector<KalmanFilter> AutoTubePanel::LoadKFilters(FILE* fi)
{
	std::vector<KalmanFilter> filters;

	int size;
	fread(&size, sizeof(int), 1, fi);
	for (unsigned int ind(0); ind < size; ind++)
	{

		KalmanFilter k;

		int num = k.N;

		Eigen::VectorXd z(num);
		for (unsigned int i(0); i < num; i++)
		{
			double dummy;
			fread(&dummy, sizeof(double), 1, fi);
			z(i) = dummy;
		}

		Eigen::VectorXd z_hat(num);
		for (unsigned int i(0); i < num; i++)
		{
			double dummy;
			fread(&dummy, sizeof(double), 1, fi);
			z_hat(i) = dummy;
		}

		Eigen::VectorXd x(num);
		for (unsigned int i(0); i < num; i++)
		{
			double dummy;
			fread(&dummy, sizeof(double), 1, fi);
			x(i) = dummy;
		}

		Eigen::VectorXd v(num);
		for (unsigned int i(0); i < num; i++)
		{
			double dummy;
			fread(&dummy, sizeof(double), 1, fi);
			v(i) = dummy;
		}

		Eigen::VectorXd n(num);
		for (unsigned int i(0); i < num; i++)
		{
			double dummy;
			fread(&dummy, sizeof(double), 1, fi);
			n(i) = dummy;
		}

		Eigen::VectorXd m(num);
		for (unsigned int i(0); i < num; i++)
		{
			double dummy;
			fread(&dummy, sizeof(double), 1, fi);
			m(i) = dummy;
		}

		Eigen::MatrixXd f(num, num);
		for (unsigned int i(0); i < f.rows(); i++)
		{
			for (unsigned int j(0); j < f.cols(); j++)
			{
				double dummy;
				fread(&dummy, sizeof(double), 1, fi);
				f(i, j) = dummy;
			}
		}

		Eigen::MatrixXd h(num, num);
		for (unsigned int i(0); i < h.rows(); i++)
		{
			for (unsigned int j(0); j < h.cols(); j++)
			{
				double dummy;
				fread(&dummy, sizeof(double), 1, fi);
				h(i, j) = dummy;
			}
		}

		Eigen::MatrixXd p(num, num);
		for (unsigned int i(0); i < p.rows(); i++)
		{
			for (unsigned int j(0); j < p.cols(); j++)
			{
				double dummy;
				fread(&dummy, sizeof(double), 1, fi);
				p(i, j) = dummy;
			}
		}

		Eigen::MatrixXd q(num, num);
		for (unsigned int i(0); i < q.rows(); i++)
		{
			for (unsigned int j(0); j < q.cols(); j++)
			{
				double dummy;
				fread(&dummy, sizeof(double), 1, fi);
				q(i, j) = dummy;
			}
		}

		Eigen::MatrixXd r(num, num);
		for (unsigned int i(0); i < r.rows(); i++)
		{
			for (unsigned int j(0); j < r.cols(); j++)
			{
				double dummy;
				fread(&dummy, sizeof(double), 1, fi);
				r(i, j) = dummy;
			}
		}

		Eigen::MatrixXd w(num, num);
		for (unsigned int i(0); i < w.rows(); i++)
		{
			for (unsigned int j(0); j < w.cols(); j++)
			{
				double dummy;
				fread(&dummy, sizeof(double), 1, fi);
				w(i, j) = dummy;
			}
		}

		Eigen::MatrixXd s(num, num);
		for (unsigned int i(0); i < s.rows(); i++)
		{
			for (unsigned int j(0); j < s.cols(); j++)
			{
				double dummy;
				fread(&dummy, sizeof(double), 1, fi);
				s(i, j) = dummy;
			}
		}

		k.SetZ(z);
		k.SetZHat(z_hat);
		k.SetX(x);
		k.SetV(v);
		k.SetN(n);
		k.SetM(m);
		k.SetF(f);
		k.SetP(p);
		k.SetH(h);
		k.SetQ(q);
		k.SetR(r);
		k.SetW(w);
		k.SetS(s);

		int slice;
		fread(&slice, sizeof(int), 1, fi);
		int iteration;
		fread(&iteration, sizeof(int), 1, fi);
		int last_slice;
		fread(&last_slice, sizeof(int), 1, fi);
		int nstring;
		fread(&nstring, sizeof(int), 1, fi);
		std::vector<char> content(nstring, 0);
		fread(&content[0], 1, nstring, fi);
		std::string label(content.begin(), content.end());

		k.SetSlice(slice);
		k.SetIteration(iteration);
		k.SetLastSlice(last_slice);
		k.SetLabel(label);
		filters.push_back(k);
	}
	return filters;
}

void AutoTubePanel::Save()
{
	m_CachedData.Get(m_LabelMaps, m_Objects, m_LabelToText, m_KFilters, m_Probabilities, m_MaxActiveSliceReached);
	QString savefilename = QFileDialog::getSaveFileName(this, "Save As", QString::null, "Projects (*.prj)\n");

	if (!savefilename.isEmpty())
	{
		if (savefilename.length() <= 4 || !savefilename.endsWith(QString(".prj")))
			savefilename.append(".prj");
		FILE* fo;
		fo = fopen(savefilename.toAscii(), "wb");

		int dummy;
		dummy = m_Handler3D->NumSlices();
		fwrite(&dummy, sizeof(int), 1, fo);

		dummy = m_MaxActiveSliceReached;
		fwrite(&dummy, sizeof(int), 1, fo);

		for (unsigned int i(0); i <= m_MaxActiveSliceReached; i++)
		{

			dummy = m_CachedData.objects[i].size();
			fwrite(&dummy, sizeof(int), 1, fo);
			for (unsigned int j(0); j < m_CachedData.objects[i].size(); j++)
			{
				int n = m_CachedData.objects[i][j].size();
				fwrite(&n, sizeof(int), 1, fo);
				fwrite(m_CachedData.objects[i][j].c_str(), 1, n, fo);
			}
		}

		for (unsigned int i(0); i <= m_MaxActiveSliceReached; i++)
		{
			SaveLabelMap(fo, m_CachedData.label_maps[i]);
		}
		for (unsigned int i(0); i <= m_MaxActiveSliceReached; i++)
		{
			SaveLToT(fo, m_CachedData.label_to_text[i]);
		}

		SaveKFilter(fo, m_CachedData.k_filters);

		for (unsigned int i(0); i <= m_MaxActiveSliceReached; i++)
		{
			dummy = m_CachedData._probabilities[i].size();
			fwrite(&dummy, sizeof(int), 1, fo);
			for (auto const& prob : m_CachedData._probabilities[i])
			{
				int n = prob.size();
				fwrite(&n, sizeof(int), 1, fo);
				fwrite(prob.c_str(), 1, n, fo);
			}
		}

		fclose(fo);
		QMessageBox m_box;
		m_box.setWindowTitle("Saving");
		m_box.setText(QString::fromStdString("Saving Finished"));
		m_box.exec();
	}
}

void AutoTubePanel::Load()
{
	m_CachedData.Get(m_LabelMaps, m_Objects, m_LabelToText, m_KFilters, m_Probabilities, m_MaxActiveSliceReached);
	QString loadfilename = QFileDialog::getOpenFileName(this, "Open", QString::null, "Projects (*.prj)\nAll (*.*)");
	if (!loadfilename.isEmpty())
	{
		FILE* fi;
		fi = fopen(loadfilename.toAscii(), "rb");

		int max_num_slices;
		fread(&max_num_slices, sizeof(int), 1, fi);

		int num_slices;
		fread(&num_slices, sizeof(int), 1, fi);

		m_MaxActiveSliceReached = num_slices;

		std::vector<std::vector<std::string>> objs(max_num_slices);
		std::vector<label_map_type::Pointer> l_maps(max_num_slices);
		std::vector<std::map<label_type, std::string>> l_to_t(max_num_slices);
		std::vector<std::vector<std::string>> probabilities(max_num_slices);
		std::vector<KalmanFilter> k_fs;

		for (unsigned int i(0); i <= num_slices; i++)
		{
			int size;
			fread(&size, sizeof(int), 1, fi);

			std::vector<std::string> vec;
			for (unsigned int j(0); j < size; j++)
			{
				int n;
				fread(&n, sizeof(int), 1, fi);
				std::vector<char> content(n, 0);
				fread(&content[0], 1, n, fi);
				std::string str(content.begin(), content.end());
				vec.push_back(str);
			}
			objs[i] = vec;
		}

		for (unsigned int i(0); i <= num_slices; i++)
		{

			label_map_type::Pointer label_map = LoadLabelMap(fi);
			VisualizeLabelMap(label_map);
			l_maps[i] = label_map;
		}

		for (unsigned int i(0); i <= num_slices; i++)
		{

			l_to_t[i] = LoadLToT(fi);
		}

		k_fs = LoadKFilters(fi);

		for (unsigned int i(0); i <= num_slices; i++)
		{

			std::vector<std::string> vec;
			int size;
			fread(&size, sizeof(int), 1, fi);
			for (unsigned int j(0); j < size; j++)
			{
				int n;
				fread(&n, sizeof(int), 1, fi);
				std::vector<char> content(n, 0);
				fread(&content[0], 1, n, fi);
				// Construct the string (skip this, if you read into the string directly)
				std::string str(content.begin(), content.end());

				vec.push_back(str);
			}
			probabilities[i] = vec;
		}

		fclose(fi);

		m_CachedData.Store(l_maps, objs, l_to_t, k_fs, probabilities, m_MaxActiveSliceReached);
		m_Objects = objs;
		m_Probabilities = probabilities;
		m_LabelMaps = l_maps;
		m_KFilters = k_fs;

		RefreshObjectList();
		RefreshProbabilityList();
		RefreshKFilterList();
		QMessageBox m_box;
		m_box.setWindowTitle("Loading");
		m_box.setText(QString::fromStdString("Loading Finished"));
		m_box.exec();
	}
}

void AutoTubePanel::KFilterDoubleClicked(QListWidgetItem* item)
{
	// get cached data
	m_CachedData.Get(m_LabelMaps, m_Objects, m_LabelToText, m_KFilters, m_Probabilities, m_MaxActiveSliceReached);
	std::vector<std::string> labels;
	for (const auto& filter : m_KFilters)
		labels.push_back(filter.GetLabel());

	QMessageBox m_box;
	std::stringstream buffer;
	//auto split= split_string(item->text().toStdString());

	std::string label = item->text().toStdString();
	const std::vector<std::string>::iterator it = std::find(labels.begin(), labels.end(), label);
	int index = std::distance(labels.begin(), it);
	buffer << m_KFilters[index] << std::endl;
	if (m_RestartKFilter->isChecked())
	{
		KalmanFilter k;
		k.SetLabel(m_KFilters[index].GetLabel());
		k.SetSlice(m_KFilters[index].GetSlice());
		m_KFilters[index] = k;
	}
	else
	{
		m_box.setWindowTitle("Filters");
		m_box.setText(QString::fromStdString(buffer.str()));
		m_box.exec();
	}

	// store cached data
	m_CachedData.Store(m_LabelMaps, m_Objects, m_LabelToText, m_KFilters, m_Probabilities, m_MaxActiveSliceReached);
}

void AutoTubePanel::RemoveKFilter()
{
	QList<QListWidgetItem*> items = m_KFiltersList->selectedItems();
	// get all rows from the selected items
	std::vector<int> rows;
	for (auto item : items)
	{
		int row = m_KFiltersList->row(item);
		rows.push_back(row);
	}

	// delete in descending order, i.e. from back of vector
	std::sort(rows.begin(), rows.end(), std::greater<int>());
	for (auto row : rows)
	{
		m_KFilters.erase(m_KFilters.begin() + row);
	}

	RefreshKFilterList();
	m_CachedData.Store(m_LabelMaps, m_Objects, m_LabelToText, m_KFilters, m_Probabilities, m_MaxActiveSliceReached);
}

void AutoTubePanel::UpdateKalmanFilters()
{

	if (!m_KFilters.empty() && m_LabelMaps[m_Handler3D->ActiveSlice()])
	{
		// get cached data
		m_CachedData.Get(m_LabelMaps, m_Objects, m_LabelToText, m_KFilters, m_Probabilities, m_MaxActiveSliceReached);
		// get Kalman filter laels
		std::vector<std::string> labels;
		for (const auto& filter : m_KFilters)
			labels.push_back(filter.GetLabel());
		bool not_in_k_filter_list(false);
		std::vector<std::string> objects_not_in_list;
		// for each slice
		for (int i(0); i <= m_Handler3D->ActiveSlice(); i++) // \bug this always starts at slice 0 => it should store start slice, i.e. where user pressed execute first time
		{
			auto label_map = m_LabelMaps[i];
			if (!label_map)
			{
				QMessageBox m_box;
				m_box.setWindowTitle("Error");
				m_box.setText(QString::fromStdString("No label map for slice " + std::to_string(i) + ". Tool expects you to start at slice 0."));
				m_box.exec();
				return;
			}

			using LabelToParams = std::map<label_type, std::vector<double>>;
			LabelToParams label_to_params = GetLabelMapParams(label_map);
			// for each object in slice object list
			for (unsigned int row(0); row < m_Objects[i].size(); row++)
			{
				std::string label = m_Objects[i][row];
				auto label_object = label_map->GetNthLabelObject(row);

				std::vector<std::string>::iterator it = std::find(labels.begin(), labels.end(), label);
				if (it != labels.end())
				{
					int index = std::distance(labels.begin(), it);
					if (m_KFilters[index].GetSlice() <= i + 1 && m_KFilters[index].GetLastSlice() < i + 1)
					{
						std::vector<double> params = label_to_params[label_object->GetLabel()];
						std::vector<double> needed_params = {params[0], params[1]};
						m_KFilters[index].SetMeasurement(needed_params);
						m_KFilters[index].Work();
						m_KFilters[index].SetLastSlice(i + 1);
					}
				}
				else
				{
					not_in_k_filter_list = true;
					objects_not_in_list.push_back(label);
				}
			}
		}

		if (not_in_k_filter_list)
		{
			std::string message;
			for (const auto& object : objects_not_in_list)
				message += object + "\n";

			QMessageBox m_box;
			m_box.setWindowTitle("Error");
			m_box.setText(QString::fromStdString(message) + "Don't Have Kalman Filters!");
			m_box.exec();
		}
		// store to cached data
		m_CachedData.Store(m_LabelMaps, m_Objects, m_LabelToText, m_KFilters, m_Probabilities, m_MaxActiveSliceReached);
	}
	else
	{

		QMessageBox m_box;
		m_box.setWindowTitle("Error");
		m_box.setText("No Kalman Filters Found! or No label map exists for the current slice");
		m_box.exec();
	}
}

label_map_type::Pointer AutoTubePanel::CalculateLabelMapParams(label_map_type::Pointer labelMap)
{
	using ShapeLabelMapFilter = itk::ShapeLabelMapFilter<label_map_type>;

	ShapeLabelMapFilter::Pointer shape_filter = ShapeLabelMapFilter::New();
	shape_filter->SetInput(labelMap);
	shape_filter->SetComputeFeretDiameter(true);
	shape_filter->SetComputePerimeter(true);
	shape_filter->Update();
	return shape_filter->GetOutput();
}

std::map<label_type, std::vector<double>> AutoTubePanel::GetLabelMapParams(label_map_type::Pointer labelMap)
{
	std::map<label_type, std::vector<double>> label_to_params;
	using PixelType = float;
	using ImageType = itk::Image<PixelType, 2>;
	using ConstIteratorType = itk::ImageRegionConstIterator<ImageType>;

	std::map<label_type, double> label_2_x;
	std::map<label_type, double> label_2_y;
	std::map<label_type, double> label_2_nb_pixels;

	using Label2ImageType = itk::LabelMapToLabelImageFilter<label_map_type, ImageType>;
	auto label2image = Label2ImageType::New();
	label2image->SetInput(labelMap);
	label2image->Update();

	//centroid calculation
	for (auto label : labelMap->GetLabels())
	{
		label_2_x[label] = 0;
		label_2_y[label] = 0;
		label_2_nb_pixels[label] = 0;
	}

	ConstIteratorType in(label2image->GetOutput(), label2image->GetOutput()->GetRequestedRegion());

	for (in.GoToBegin(); !in.IsAtEnd(); ++in)
	{
		if (labelMap->GetPixel(in.GetIndex()) != 0)
		{
			label_2_x[labelMap->GetPixel(in.GetIndex())] += in.GetIndex()[0];
			label_2_y[labelMap->GetPixel(in.GetIndex())] += in.GetIndex()[1];
			label_2_nb_pixels[labelMap->GetPixel(in.GetIndex())] += 1;
		}
	}

	for (auto label : labelMap->GetLabels())
	{
		label_2_x[label] = label_2_x[label] / label_2_nb_pixels[label];
		label_2_y[label] = label_2_y[label] / label_2_nb_pixels[label];
	}

	for (unsigned int i = 0; i < labelMap->GetNumberOfLabelObjects(); i++)
	{
		auto label_object = labelMap->GetNthLabelObject(i);
		std::vector<double> params;

		params.push_back(label_2_x[label_object->GetLabel()]);
		params.push_back(label_2_y[label_object->GetLabel()]);
		params.push_back(label_object->GetEquivalentSphericalPerimeter());
		params.push_back(label_object->GetEquivalentSphericalRadius());
		params.push_back(label_object->GetFeretDiameter());
		params.push_back(label_object->GetFlatness());
		params.push_back(label_object->GetNumberOfPixels());
		params.push_back(label_object->GetNumberOfPixelsOnBorder());
		params.push_back(label_object->GetPerimeter());
		params.push_back(label_object->GetPerimeterOnBorder());
		params.push_back(label_object->GetPerimeterOnBorderRatio());
		params.push_back(label_object->GetPhysicalSize());
		params.push_back(label_object->GetRoundness());
		label_to_params.insert(std::make_pair(label_object->GetLabel(), params));
	}
	return label_to_params;
}

std::vector<double> AutoTubePanel::Softmax(std::vector<double> distances, std::vector<double> diff_in_pred, std::vector<double> diff_in_data)
{
	std::vector<double> probabilities;

	double sum(0);

	for (unsigned int i(0); i < distances.size(); i++)
	{

		double x = exp(-(m_WDistance->text().toDouble() * distances[i] + m_WPred->text().toDouble() * diff_in_pred[i] + m_WParams->text().toDouble() * diff_in_data[i]));
		probabilities.push_back(x);
		sum += x;
	}
	for (auto& probability : probabilities)
	{
		if (probabilities.size() > 1)
			probability = probability / sum;

		if (std::isnan(probability))
			probability = 0;
	}
	return probabilities;
}

std::vector<std::string> AutoTubePanel::SplitString(std::string text)
{
	// splits a string only via " " (the space character)
	std::istringstream iss(text);
	std::vector<std::string> results(std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>());
	return results;
}

void AutoTubePanel::RemoveObject()
{
	// removes object from the Slice Object List

	// Get Chached data
	m_CachedData.Get(m_LabelMaps, m_Objects, m_LabelToText, m_KFilters, m_Probabilities, m_MaxActiveSliceReached);

	QList<QListWidgetItem*> items = m_ObjectList->selectedItems();
	label_map_type::Pointer label_map = m_LabelMaps[m_Handler3D->ActiveSlice()];

	std::vector<label_object_type*> objects_to_remove;
	std::vector<int> rows;
	std::vector<std::string> labels;

	for (auto item : items)
	{
		int row = m_ObjectList->row(item);
		rows.push_back(row);

		auto label_object = label_map->GetNthLabelObject(row);
		labels.push_back(std::to_string(label_object->GetLabel()));
		objects_to_remove.push_back(label_object);

		m_LabelToText[m_Handler3D->ActiveSlice()].erase(label_object->GetLabel());
	}

	// Remove from label objects  from label map
	for (auto object : objects_to_remove)
		label_map->RemoveLabelObject(object);

	int offset(0);

	// sort rows in an ascending order so offset works
	std::sort(rows.begin(), rows.end());

	for (auto row : rows)
	{
		auto it = m_Objects[m_Handler3D->ActiveSlice()].begin();
		std::advance(it, row - offset);
		m_Objects[m_Handler3D->ActiveSlice()].erase(it);
		offset += 1;
	}

	if (m_Handler3D->ActiveSlice() != 0)
	{
		for (const auto& label : labels)
		{
			m_Probabilities[m_Handler3D->ActiveSlice()] = ModifyProbability(m_Probabilities[m_Handler3D->ActiveSlice()], label, true, false);
		}
	}

	auto label_objects = label_map->GetLabelObjects();

	if (m_Handler3D->ActiveSlice() != 0)
	{
		for (unsigned int i(0); i < label_map->GetNumberOfLabelObjects(); i++)
		{
			m_Probabilities[m_Handler3D->ActiveSlice()] = ModifyProbability(m_Probabilities[m_Handler3D->ActiveSlice()], std::to_string(label_map->GetNthLabelObject(i)->GetLabel()), false, true, std::to_string(i + 1));
		}
	}

	// clear labels from label map and re-initialize to have from 1 up to number of label objects
	label_map->ClearLabels();
	for (auto& object : label_objects)
		label_map->PushLabelObject(object);

	SAFE_UPDATE(label_map, return );
	m_LabelMaps[m_Handler3D->ActiveSlice()] = label_map;

	VisualizeLabelMap(label_map);
	RefreshObjectList();
	RefreshProbabilityList();
	m_CachedData.Store(m_LabelMaps, m_Objects, m_LabelToText, m_KFilters, m_Probabilities, m_MaxActiveSliceReached);
}

char AutoTubePanel::GetRandomChar()
{
	unsigned seed = time(nullptr);
	srand(seed);
	return char(rand() % 26 + 'a');
}

void AutoTubePanel::Visualize()
{
	if (!m_LabelMaps.empty() && m_LabelMaps[m_Handler3D->ActiveSlice()])
		VisualizeLabelMap(m_LabelMaps[m_Handler3D->ActiveSlice()]);
	else
	{
		QMessageBox m_box;
		m_box.setWindowTitle("Error");
		m_box.setText("No label map to show");
		m_box.exec();
	}
}
void AutoTubePanel::MergeSelectedItems()
{
	QList<QListWidgetItem*> items = m_ObjectList->selectedItems();
	if (items.size() > 1)
	{
		using mask_type = itk::Image<unsigned char, 2>;

		m_CachedData.Get(m_LabelMaps, m_Objects, m_LabelToText, m_KFilters, m_Probabilities, m_MaxActiveSliceReached);

		auto label_map = m_LabelMaps[m_Handler3D->ActiveSlice()];

		std::vector<int> rows;

		for (auto item : items)
			rows.push_back(m_ObjectList->row(item));

		std::sort(rows.begin(), rows.end());

		std::vector<std::string> text_of_labels;

		std::string target_name = items[0]->text().toStdString();

		label_type target_label(label_map->GetNthLabelObject(rows[0])->GetLabel());

		std::vector<unsigned int> pixel_ids;

		// need to map labels to target label in the new merged label map
		std::map<label_type, label_type> objects_label_mapping;
		for (unsigned int i = 0; i < label_map->GetNumberOfLabelObjects(); i++)
		{
			// Get the ith region

			auto label_object = label_map->GetNthLabelObject(i);

			std::pair<label_type, label_type> label_pair;
			if (std::find(rows.begin(), rows.end(), label_object->GetLabel() - 1) != rows.end())
			{
				// map label to a different label in new label map
				label_pair = std::make_pair(label_object->GetLabel(), target_label);
				objects_label_mapping.insert(label_pair);
			}
			else
			{
				// map label to the same label
				label_pair = std::make_pair(label_object->GetLabel(), label_object->GetLabel());
				objects_label_mapping.insert(label_pair);
			}
		}

		auto merge_label_map = itk::ChangeLabelLabelMapFilter<label_map_type>::New();
		merge_label_map->SetInput(label_map);
		merge_label_map->SetChangeMap(objects_label_mapping);
		SAFE_UPDATE(merge_label_map, return );
		label_map = merge_label_map->GetOutput();

		auto binary_image = itk::LabelMapToBinaryImageFilter<label_map_type, mask_type>::New();
		binary_image->SetInput(label_map);
		SAFE_UPDATE(binary_image, return );
		m_MaskImage = binary_image->GetOutput();

		std::vector<label_type> labels = label_map->GetLabels();
		std::vector<std::string> objects_list;

		std::map<label_type, std::string> l_to_t;
		for (unsigned int i(0); i < labels.size(); i++)
		{
			if (target_label != labels[i])
			{
				objects_list.push_back(m_LabelToText[m_Handler3D->ActiveSlice()][labels[i]]);
			}
			else
			{
				objects_list.push_back(target_name);
			}
		}

		if (m_Handler3D->ActiveSlice() != 0)
		{
			for (unsigned int i(1); i < rows.size(); i++)
			{
				m_Probabilities[m_Handler3D->ActiveSlice()] = ModifyProbability(m_Probabilities[m_Handler3D->ActiveSlice()], std::to_string(rows[i] + 1), true, false);
			}
		}

		auto label_objects = label_map->GetLabelObjects();

		if (m_Handler3D->ActiveSlice() != 0)
		{
			for (unsigned int i(0); i < label_map->GetNumberOfLabelObjects(); i++)
			{
				m_Probabilities[m_Handler3D->ActiveSlice()] = ModifyProbability(m_Probabilities[m_Handler3D->ActiveSlice()], std::to_string(label_map->GetNthLabelObject(i)->GetLabel()), false, true, std::to_string(i + 1));
			}
		}

		// clear labels from label map and re-initialize to have from 1 up to number of label objects
		label_map->ClearLabels();
		for (auto& object : label_objects)
			label_map->PushLabelObject(object);

		m_LabelMaps[m_Handler3D->ActiveSlice()] = label_map;
		m_Objects[m_Handler3D->ActiveSlice()] = objects_list;
		RefreshObjectList();
		RefreshProbabilityList();

		m_CachedData.Store(m_LabelMaps, m_Objects, m_LabelToText, m_KFilters, m_Probabilities, m_MaxActiveSliceReached);
	}
}

void AutoTubePanel::OnMouseClicked(iseg::Point p)
{
	// if point click is a label object it adds it to the current slice's selected objects

	m_CachedData.Get(m_LabelMaps, m_Objects, m_LabelToText, m_KFilters, m_Probabilities, m_MaxActiveSliceReached);
	if (m_LabelMaps[m_Handler3D->ActiveSlice()])
	{
		using PixelType = float;
		using ImageType = itk::Image<PixelType, 2>;
		if (!m_LabelMaps[m_Handler3D->ActiveSlice()])
		{
			ImageType::SizeType size{{384, 384}};
			label_map_type::Pointer map = label_map_type::New();
			itk::Index<2> index{{0, 0}};

			ImageType::RegionType region;
			region.SetSize(size);
			region.SetIndex(index);

			map->SetRegions(region);
			map->Allocate(true);
			map->Update();
			m_LabelMaps[m_Handler3D->ActiveSlice()] = map;

			std::vector<std::string> list;
			m_Objects[m_Handler3D->ActiveSlice()] = list;
		}
		auto label_map = m_LabelMaps[m_Handler3D->ActiveSlice()];

		itk::Index<2> index;
		index[0] = p.px;
		index[1] = p.py;

		label_type label = label_map->GetPixel(index);

		// 0 is background
		if (label != 0 && !m_AddPixel->isChecked())
		{
			auto item = m_ObjectList->item(label - 1); // rows start from 0 and labels in label map from 1
			m_ObjectList->setCurrentItem(item);

			int row = m_ObjectList->row(item);

			std::vector<int>::iterator it = std::find(m_Selected.begin(), m_Selected.end(), row);
			if (it == m_Selected.end())
				m_Selected.push_back(row);
		}

		else if (m_AddPixel->isChecked() && label == 0)
		{

			using Label2ImageType = itk::LabelMapToLabelImageFilter<label_map_type, ImageType>;
			auto label2image = Label2ImageType::New();
			label2image->SetInput(label_map);
			label2image->Update();
			auto image = label2image->GetOutput();

			if (label_map->GetPixel(index) == 0)
			{
				label_type last_label = 0;
				if (!label_map->GetLabels().empty())
					last_label = label_map->GetLabels().back();

				if (image->GetBufferedRegion().IsInside(index))
				{

					image->SetPixel(index, last_label + 1);

					image->Update();

					using Image2LabelType = itk::LabelImageToShapeLabelMapFilter<ImageType, label_map_type>;
					auto image2label = Image2LabelType::New();
					image2label->SetInput(image);
					image2label->Update();

					m_LabelMaps[m_Handler3D->ActiveSlice()] = image2label->GetOutput();

					m_Objects[m_Handler3D->ActiveSlice()].push_back(std::to_string(last_label + 1));
				}

				auto label_objects = m_LabelMaps[m_Handler3D->ActiveSlice()]->GetLabelObjects();
				m_LabelMaps[m_Handler3D->ActiveSlice()]->ClearLabels();
				for (auto& object : label_objects)
					m_LabelMaps[m_Handler3D->ActiveSlice()]->PushLabelObject(object);
			}

			RefreshObjectList();
			VisualizeLabelMap(m_LabelMaps[m_Handler3D->ActiveSlice()]);
			m_CachedData.Store(m_LabelMaps, m_Objects, m_LabelToText, m_KFilters, m_Probabilities, m_MaxActiveSliceReached);
		}
	}
}
double AutoTubePanel::CalculateDistance(std::vector<double> params_1, std::vector<double> params_2)
{
	// calculate distance
	itk::Point<double, 2> centroid1;
	centroid1[0] = params_1[0];
	centroid1[1] = params_1[1];

	itk::Point<double, 2> centroid2;
	centroid2[0] = params_2[0];
	centroid2[1] = params_2[1];

	return centroid1.EuclideanDistanceTo(centroid2);
}

bool AutoTubePanel::LabelInList(const std::string label)
{
	bool in_list(false);
	for (unsigned int i(0); i < m_Objects.size(); i++)
	{
		if (!(m_Objects[i]).empty())
		{
			for (const auto& o_label : m_Objects[i])
			{
				if (o_label == label)
				{
					in_list = true;
					break;
				}
			}
		}
		if (in_list)
			break;
	}
	return in_list;
}

void AutoTubePanel::ExtrapolateResults()
{
	// Function of Extrapolate button

	// To make sure that it is not checked as we do not want to add to existing label maps when there aren't any
	m_Add->setChecked(false);

	// boolean in case no label objects are found
	bool quit(false);

	// quite in case a Kalman filter does not exist for an object
	bool quit_k_filters(false);

	int limit_slice = m_LimitSlice->text().toInt();
	if (limit_slice <= m_Handler3D->ActiveSlice() || limit_slice >= m_Handler3D->NumSlices())
		limit_slice = m_Handler3D->NumSlices();

	if (!m_KFilters.empty())
	{

		for (unsigned int index(m_Handler3D->ActiveSlice()); index < limit_slice - 1; index++)
		{

			m_CachedData.Get(m_LabelMaps, m_Objects, m_LabelToText, m_KFilters, m_Probabilities, m_MaxActiveSliceReached);

			int slice_to_infer_from = index; // starting slice

			m_Handler3D->SetActiveSlice(slice_to_infer_from + 1, true);

			if (m_MaxActiveSliceReached < m_Handler3D->ActiveSlice())
				m_MaxActiveSliceReached = m_Handler3D->ActiveSlice();

			// preparing data for do_work_nd -> function of Execute button which will save a new label map in label_maps[_handler3D->active_slice()]
			iseg::SlicesHandlerITKInterface itk_handler(m_Handler3D);

			using input_type = itk::Image<float, 2>;
			using tissues_type = itk::Image<iseg::tissues_size_t, 2>;
			auto source = itk_handler.GetSourceSlice();
			auto target = itk_handler.GetTargetSlice();
			auto tissues = itk_handler.GetTissuesSlice();
			DoWorkNd<input_type, tissues_type, input_type>(source, tissues, target);

			// label map from slice to infer from
			auto label_map = m_LabelMaps[slice_to_infer_from];

			// new label map
			auto extrapolated_label_map = m_LabelMaps[m_Handler3D->ActiveSlice()];

			std::vector<itk::Point<double, 2>>* centroids = new std::vector<itk::Point<double, 2>>;

			for (unsigned int i = 0; i < label_map->GetNumberOfLabelObjects(); i++)
			{
				// Get the ith region
				auto label_object = label_map->GetNthLabelObject(i);
				centroids->push_back(label_object->GetCentroid());
			}

			using LabelToParams = std::map<label_type, std::vector<double>>;

			LabelToParams label_to_params;
			label_to_params = GetLabelMapParams(label_map);

			LabelToParams extrapolated_label_to_params;
			extrapolated_label_to_params = GetLabelMapParams(extrapolated_label_map);

			std::map<label_type, label_type> objects_label_mapping;

			std::map<label_type, std::string> l_to_t; // local variable label to text mapping
			std::vector<std::string> probs;						// probabilities

			if (centroids->empty() || extrapolated_label_map->GetNumberOfLabelObjects() == 0)
			{
				quit = true;
				m_Handler3D->SetActiveSlice(slice_to_infer_from, true);
				break;
			}
			std::vector<std::string> objects_list;

			std::vector<std::string> labels;
			for (const auto& filter : m_KFilters)
			{
				labels.push_back(filter.GetLabel());
			}

			std::vector<std::string>::iterator it; // iterator used to find the index of the given kalman filter from the label map

			double threshold_probability = m_MinProbability->text().toDouble();

			for (unsigned int i = 0; i < extrapolated_label_map->GetNumberOfLabelObjects(); i++)
			{
				std::vector<double> distances;
				std::vector<double> diff_from_predictions;
				std::vector<double> diff_in_data;
				for (unsigned int j = 0; j < label_map->GetNumberOfLabelObjects(); j++)
				{

					std::vector<double> par = label_to_params[label_map->GetNthLabelObject(j)->GetLabel()];

					it = std::find(labels.begin(), labels.end(), m_LabelToText[slice_to_infer_from][label_map->GetNthLabelObject(j)->GetLabel()]);
					if (it != labels.end())
					{
						int index = std::distance(labels.begin(), it);

						// calculates the difference in position prediction
						double diff_from_pred = m_KFilters[index].DiffBtwPredicatedObject(extrapolated_label_to_params[extrapolated_label_map->GetNthLabelObject(i)->GetLabel()]);
						diff_from_predictions.push_back(diff_from_pred);

						// calculate the distance
						double distance = CalculateDistance(label_to_params[label_map->GetNthLabelObject(j)->GetLabel()], extrapolated_label_to_params[extrapolated_label_map->GetNthLabelObject(i)->GetLabel()]);
						distances.push_back(distance);

						unsigned int size(label_to_params[label_map->GetNthLabelObject(j)->GetLabel()].size());
						// calculate the difference between the two given object's parameters
						double sum(0);
						for (unsigned int k(2); k < size; k++)
						{
							sum += abs(label_to_params[label_map->GetNthLabelObject(j)->GetLabel()][k] - extrapolated_label_to_params[extrapolated_label_map->GetNthLabelObject(i)->GetLabel()][k]);
						}
						diff_in_data.push_back(sum); // two first elements are centroids
					}
					else
					{
						quit_k_filters = true;
						break;
					}
				}
				if (quit_k_filters)
					break;
				std::vector<double> probabilities;
				// calculates the probabilities of label object from the extrapolated label map to being any label object from the previous label map
				probabilities = Softmax(distances, diff_from_predictions, diff_in_data);

				// index of highest probability
				int max_index = std::distance(probabilities.begin(), std::max_element(probabilities.begin(), probabilities.end()));

				auto new_label_object = extrapolated_label_map->GetNthLabelObject(i);
				bool set(false); // was the name of the new label object already set?

				if (probabilities[max_index] > threshold_probability)
				{
					auto old_label_object = label_map->GetNthLabelObject(max_index);

					// if the new label object corresponds to an old label object that is a digit
					// we add a random charecter to it so the user can see to which object it is linked too

					if (isdigit(m_LabelToText[slice_to_infer_from][old_label_object->GetLabel()][0]))
					{
						char letter_to_add;
						if (isalpha(m_LabelToText[slice_to_infer_from][old_label_object->GetLabel()].back()))
							letter_to_add = m_LabelToText[slice_to_infer_from][old_label_object->GetLabel()].back();
						else
							letter_to_add = GetRandomChar();
						while (LabelInList(m_LabelToText[slice_to_infer_from][old_label_object->GetLabel()] + letter_to_add))
						{
							letter_to_add = GetRandomChar();
						}
						l_to_t[new_label_object->GetLabel()] = m_LabelToText[slice_to_infer_from][old_label_object->GetLabel()] + letter_to_add;
					}
					else
					{
						l_to_t[new_label_object->GetLabel()] = m_LabelToText[slice_to_infer_from][old_label_object->GetLabel()];
					}

					set = true;
				}

				for (unsigned int j = 0; j < label_map->GetNumberOfLabelObjects(); j++)
				{
					auto old_label_object = label_map->GetNthLabelObject(j);

					// if Extrapolate Only Matches is not checked and the new object was not set then add it to the mapping
					if (!m_ExtrapolateOnlyMatches->isChecked() && !set)
						l_to_t[new_label_object->GetLabel()] = std::to_string(new_label_object->GetLabel());

					std::string str = "Probability of " + std::to_string(new_label_object->GetLabel()) + " -> " + m_LabelToText[slice_to_infer_from][old_label_object->GetLabel()] + " = " + std::to_string(probabilities[j]);
					probs.push_back(str);
				}
			}

			if (m_ExtrapolateOnlyMatches->isChecked())
			{
				bool in;

				for (auto label : extrapolated_label_map->GetLabels())
				{
					in = false;
					for (auto& element : l_to_t)
					{
						if (element.first == label)
						{
							in = true;
							break;
						}
					}
					if (!in)
					{
						// if label not in the mapping -> remove the probability from the probability list and remove its label from the label map
						probs = ModifyProbability(probs, std::to_string(label), true, false);
						extrapolated_label_map->RemoveLabel(label);
					}
				}

				// modify the probability to write the correct index in the object list
				for (unsigned int i(0); i < extrapolated_label_map->GetNumberOfLabelObjects(); i++)
				{
					probs = ModifyProbability(probs, std::to_string(extrapolated_label_map->GetNthLabelObject(i)->GetLabel()), false, true, std::to_string(i + 1));
				}

				// Change label maps labels from 1 up to number of labele objects
				auto label_objects = extrapolated_label_map->GetLabelObjects();
				extrapolated_label_map->ClearLabels();
				for (auto& object : label_objects)
					extrapolated_label_map->PushLabelObject(object);
			}

			if (quit_k_filters)
				break;

			labels.clear();
			for (const auto& filter : m_KFilters)
			{
				labels.push_back(filter.GetLabel());
			}

			for (auto& element : l_to_t)
			{
				// refresh object list to correspond to the mapping
				objects_list.push_back(element.second);
				/*
                // if new object has no kalman filter than create one
                if ( std::find(labels.begin(), labels.end(), element.second) == labels.end())
                {
                    KalmanFilter k;
                    k.set_label(element.second);
                    k.set_slice(_handler3D->active_slice() + 1);
                    k_filters.push_back(k);
                }
                 
                 */
			}

			m_LabelMaps[m_Handler3D->ActiveSlice()] = extrapolated_label_map;
			m_Objects[m_Handler3D->ActiveSlice()] = objects_list;
			m_Probabilities[m_Handler3D->ActiveSlice()] = probs;

			RefreshObjectList();
			RefreshProbabilityList();
			RefreshKFilterList();
			VisualizeLabelMap(extrapolated_label_map);

			m_CachedData.Store(m_LabelMaps, m_Objects, m_LabelToText, m_KFilters, m_Probabilities, m_MaxActiveSliceReached);
		}

		if (quit)
		{
			QMessageBox m_box;
			m_box.setWindowTitle("Error");
			m_box.setText("Stopped at slice:" + QString::number(m_Handler3D->ActiveSlice() + 1) + " due to object found not meeting criteria. Change filtering settings and try again.");
			m_box.exec();
		}

		if (quit_k_filters)
		{
			QMessageBox m_box;
			m_box.setWindowTitle("Error");
			m_box.setText("Stopped due to there being objects having no Kalman Filters! Make sure all objects have Kalman Filters!");
			m_box.exec();
		}
	}
	else
	{
		QMessageBox m_box;
		m_box.setWindowTitle("Error");
		m_box.setText("Cannot Extrapolate When There Are No kalman Filters! Extrapolate only when all objects have Kalman Filters!");
		m_box.exec();
	}
}

bool AutoTubePanel::IsInteger(const std::string& s)
{
	if (s.empty() || ((!isdigit(s[0])) && (s[0] != '-') && (s[0] != '+')))
		return false;

	char* p;
	strtol(s.c_str(), &p, 10);

	return (*p == 0);
}

void AutoTubePanel::ItemSelected()
{
	QList<QListWidgetItem*> items = m_ObjectList->selectedItems();
	ObjectSelected(items);
}

void AutoTubePanel::ItemDoubleClicked(QListWidgetItem* item)
{
	item->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
	m_ObjectList->editItem(item);
	QObject_connect(m_ObjectList, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(ItemEdited(QListWidgetItem*)));
}

bool AutoTubePanel::IsLabel(const std::string& s)
{
	auto str = QString::fromStdString(s);
	bool b = false;
	str.toInt(&b);
	return b;
}

void AutoTubePanel::ItemEdited(QListWidgetItem* Item)
{
	m_CachedData.Get(m_LabelMaps, m_Objects, m_LabelToText, m_KFilters, m_Probabilities, m_MaxActiveSliceReached);

	std::string new_name = Item->text().toStdString();
	boost::algorithm::trim(new_name);

	std::string old_name = m_Objects[m_Handler3D->ActiveSlice()][m_ObjectList->currentRow()];

	if (!new_name.empty() && !IsLabel(new_name))
	{

		bool in_k_filters(true);

		std::vector<std::string> labels;
		for (const auto& filter : m_KFilters)
			labels.push_back(filter.GetLabel());

		if (std::find(labels.begin(), labels.end(), new_name) == labels.end())
			in_k_filters = false;

		// if Kalman Filter doesnt exist for new name create one
		if (!in_k_filters)
		{
			KalmanFilter k;
			k.SetLabel(new_name);
			k.SetSlice(m_Handler3D->ActiveSlice() + 1);
			m_KFilters.push_back(k);
		}

		// update objects
		m_Objects[m_Handler3D->ActiveSlice()][m_ObjectList->currentRow()] = new_name;

		// update mapping
		std::map<label_type, std::string> l_to_t;
		for (unsigned int i(0); i < m_Objects[m_Handler3D->ActiveSlice()].size(); i++)
		{
			l_to_t[m_LabelMaps[m_Handler3D->ActiveSlice()]->GetNthLabelObject(i)->GetLabel()] = m_Objects[m_Handler3D->ActiveSlice()][i];
		}

		m_LabelToText[m_Handler3D->ActiveSlice()] = l_to_t;
		RefreshKFilterList();

		m_CachedData.Store(m_LabelMaps, m_Objects, m_LabelToText, m_KFilters, m_Probabilities, m_MaxActiveSliceReached);
	}
}

void AutoTubePanel::ObjectSelected(QList<QListWidgetItem*> items)
{
	if (!items.empty())
	{
		auto label_map = m_LabelMaps[m_Handler3D->ActiveSlice()];

		std::vector<int> labels_indexes;
		for (unsigned int i(0); i < items.size(); i++)
			labels_indexes.push_back(m_ObjectList->row(items[i]));

		// select pixels to highlight
		std::vector<itk::Index<2>>* item_pixels = new std::vector<itk::Index<2>>;
		for (auto label : labels_indexes)
		{
			auto label_object = label_map->GetNthLabelObject(label);
			for (unsigned int pixel_id = 0; pixel_id < label_object->Size(); pixel_id++)
			{
				item_pixels->push_back(label_object->GetIndex(pixel_id));
			}
		}

		VisualizeLabelMap(m_LabelMaps[m_Handler3D->ActiveSlice()], item_pixels);
	}
}

std::vector<std::string> AutoTubePanel::ModifyProbability(std::vector<std::string> probs, std::string label, bool remove, bool change, std::string value)
{
	// each label object has multiple values in the probability list as it has a probability for all the objects in the previous slice
	std::vector<unsigned int> indices;
	for (unsigned int ind(0); ind < probs.size(); ind++)
	{
		std::string prob = probs[ind];
		std::vector<std::string> str = SplitString(prob);
		std::string label_text = str[2]; // position of object labels name
		if (label == label_text)
		{
			if (change) // change name
			{
				str[2] = value;
				prob = "";
				for (const auto& word : str)
					prob += word + " ";
				probs[ind] = prob;
			}
			else
				indices.push_back(ind);
		}
	}
	if (remove) // if remove probability
	{
		int offset = 0;
		for (auto ind : indices)
		{
			probs.erase(probs.begin() + ind - offset);
			offset += 1;
		}
	}
	return probs;
}

void AutoTubePanel::RefreshKFilterList()
{

	m_KFiltersList->clear();

	for (const auto& filter : m_KFilters)
	{
		m_KFiltersList->addItem(QString::fromStdString(filter.GetLabel()));
	}
	m_KFiltersList->show();
}

void AutoTubePanel::RefreshObjectList()
{
	m_ObjectList->clear();
	std::map<label_type, std::string> l_to_t;
	if (!m_Objects.empty() && !(m_Objects[m_Handler3D->ActiveSlice()]).empty())
	{
		for (unsigned int i(0); i < m_Objects[m_Handler3D->ActiveSlice()].size(); i++)
		{
			m_ObjectList->addItem(QString::fromStdString(m_Objects[m_Handler3D->ActiveSlice()][i]));
			l_to_t[i + 1] = m_Objects[m_Handler3D->ActiveSlice()][i];
		}
	}
	m_ObjectList->show();
	m_LabelToText[m_Handler3D->ActiveSlice()] = l_to_t; // update mapping to reflect the refreshed objects
}
void AutoTubePanel::RefreshProbabilityList()
{
	m_ObjectProbabilityList->clear();
	if (m_Handler3D->ActiveSlice() != 0)
	{
		if (!m_Probabilities.empty() && !(m_Probabilities[m_Handler3D->ActiveSlice()]).empty())
		{
			for (unsigned int i(0); i < m_Probabilities[m_Handler3D->ActiveSlice()].size(); i++)
			{
				m_ObjectProbabilityList->addItem(QString::fromStdString(m_Probabilities[m_Handler3D->ActiveSlice()][i]));
			}
		}
		m_ObjectProbabilityList->show();
	}
}

void AutoTubePanel::SelectObjects()
{
	auto sel = m_Handler3D->TissueSelection();

	std::string text = join(sel | transformed([](int d) { return std::to_string(d); }), ", ");
	m_SelectedObjects->setText(QString::fromStdString(text));
}

void AutoTubePanel::Init()
{
	OnSlicenrChanged();
	HideParamsChanged();
}

void AutoTubePanel::NewLoaded()
{
	OnSlicenrChanged();
}

void AutoTubePanel::OnSlicenrChanged()
{
	m_CachedData.Get(m_LabelMaps, m_Objects, m_LabelToText, m_KFilters, m_Probabilities, m_MaxActiveSliceReached);
	// initialize object list for all slices
	if (m_Objects.empty() || m_Objects.size() != m_Handler3D->NumSlices())

		m_Objects = std::vector<std::vector<std::string>>(m_Handler3D->NumSlices());

	// initiating label maps
	if (m_LabelMaps.empty() || m_LabelMaps.size() != m_Handler3D->NumSlices())
		m_LabelMaps = std::vector<label_map_type::Pointer>(m_Handler3D->NumSlices());

	if (m_LabelToText.empty() || m_LabelToText.size() != m_Handler3D->NumSlices())
		m_LabelToText = std::vector<std::map<label_type, std::string>>(m_Handler3D->NumSlices());

	if (m_Probabilities.empty() || m_Probabilities.size() != m_Handler3D->NumSlices())
	{
		m_Probabilities = std::vector<std::vector<std::string>>(m_Handler3D->NumSlices());
		std::vector<std::string> init;
		init.push_back(" ");
		m_Probabilities[0] = init;
	}

	RefreshObjectList();
	RefreshProbabilityList();
	RefreshKFilterList();
	m_Selected.clear();

	if (!m_LabelMaps.empty() && m_LabelMaps[m_Handler3D->ActiveSlice()])
		VisualizeLabelMap(m_LabelMaps[m_Handler3D->ActiveSlice()]);
}

void AutoTubePanel::Cleanup()
{
}

void AutoTubePanel::DoWork()
{
	// function for Execute Button

	iseg::SlicesHandlerITKInterface itk_handler(m_Handler3D);
	using input_type = itk::Image<float, 2>;
	using tissues_type = itk::Image<iseg::tissues_size_t, 2>;
	auto source = itk_handler.GetSourceSlice();
	auto target = itk_handler.GetTargetSlice();
	auto tissues = itk_handler.GetTissuesSlice();
	DoWorkNd<input_type, tissues_type, input_type>(source, tissues, target);
}

template<class TInput, class TImage>
typename TImage::Pointer AutoTubePanel::ComputeFeatureImage(TInput* source) const
{
	itkStaticConstMacro(ImageDimension, size_t, TInput::ImageDimension);
	using hessian_pixel_type = itk::SymmetricSecondRankTensor<float, ImageDimension>;
	using hessian_image_type = itk::Image<hessian_pixel_type, ImageDimension>;
	using feature_filter_type = itk::HessianToObjectnessMeasureImageFilter<hessian_image_type, TImage>;
	using multiscale_hessian_filter_type = itk::MultiScaleHessianBasedMeasureImageFilter<TInput, hessian_image_type, TImage>;

	double sigm_min = m_SigmaLow->text().toDouble();
	double sigm_max = m_SigmaHi->text().toDouble();
	int num_levels = m_NumberSigmaLevels->text().toInt();

	auto objectness_filter = feature_filter_type::New();
	objectness_filter->SetBrightObject(false);
	objectness_filter->SetObjectDimension(1);
	objectness_filter->SetScaleObjectnessMeasure(true);
	objectness_filter->SetAlpha(0.5);
	objectness_filter->SetBeta(0.5);
	objectness_filter->SetGamma(5.0);

	auto multi_scale_enhancement_filter = multiscale_hessian_filter_type::New();
	multi_scale_enhancement_filter->SetInput(source);
	multi_scale_enhancement_filter->SetHessianToMeasureFilter(objectness_filter);
	multi_scale_enhancement_filter->SetSigmaStepMethodToEquispaced();
	multi_scale_enhancement_filter->SetSigmaMinimum(std::min(sigm_min, sigm_max));
	multi_scale_enhancement_filter->SetSigmaMaximum(std::max(sigm_min, sigm_max));
	multi_scale_enhancement_filter->SetNumberOfSigmaSteps(std::min(1, num_levels));
	multi_scale_enhancement_filter->Update();
	return multi_scale_enhancement_filter->GetOutput();
}

void AutoTubePanel::VisualizeLabelMap(label_map_type::Pointer labelMap, std::vector<itk::Index<2>>* pixels)
{
	using PixelType = float;
	using ImageType = itk::Image<PixelType, 2>;

	iseg::SlicesHandlerITKInterface itk_handler(m_Handler3D);
	auto target = itk_handler.GetTargetSlice();

	using Label2ImageType = itk::LabelMapToLabelImageFilter<label_map_type, ImageType>;
	auto label2image = Label2ImageType::New();
	label2image->SetInput(labelMap);
	SAFE_UPDATE(label2image, return );

	using tissue_type = SlicesHandlerInterface::tissue_type;
	iseg::DataSelection data_selection;
	data_selection.allSlices = false; // all_slices->isChecked();
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.work = true;
	data_selection.tissues = false;
	data_selection.bmp = false;

	using ConstIteratorType = itk::ImageRegionConstIterator<ImageType>;
	using IteratorType = itk::ImageRegionIterator<ImageType>;

	ConstIteratorType in(label2image->GetOutput(), label2image->GetOutput()->GetRequestedRegion());
	IteratorType out(target, target->GetRequestedRegion());

	for (in.GoToBegin(), out.GoToBegin(); !in.IsAtEnd(); ++in, ++out)
	{

		auto pixel_index = in.GetIndex();
		// if there are specified pixels to show else show all label objects
		if (pixels != nullptr)
		{
			if (std::find((*pixels).begin(), (*pixels).end(), pixel_index) != (*pixels).end())
			{
				out.Set(std::numeric_limits<tissue_type>::max());
			}
			else
			{
				if (out.Get() != 0)
					out.Set(std::numeric_limits<tissue_type>::max() / 2);
			}
		}
		else
		{
			if (in.Get() != 0)
				out.Set(std::numeric_limits<tissue_type>::max());
			else
				out.Set(in.Get());
		}
	}

	emit BeginDatachange(data_selection, this);

	emit EndDatachange(this);
}
template<class TInput, class TTissue, class TTarget>
void AutoTubePanel::DoWorkNd(TInput* source, TTissue* tissues, TTarget* target)
{
	itkStaticConstMacro(ImageDimension, size_t, TInput::ImageDimension);
	using input_type = TInput;

	using mask_type = itk::Image<unsigned char, ImageDimension>;
	using real_type = itk::Image<float, ImageDimension>;

	using labelfield_type = itk::Image<unsigned short, ImageDimension>;

	using threshold_filter_type = itk::BinaryThresholdImageFilter<real_type, mask_type>;
	using nonmax_filter_type = itk::NonMaxSuppressionImageFilter<real_type>;
	using thinnning_filter_type = BinaryThinningImageFilter<mask_type, mask_type, ImageDimension>;

	std::vector<double> feature_params;
	feature_params.push_back(m_SigmaLow->text().toDouble());
	feature_params.push_back(m_SigmaHi->text().toDouble());
	feature_params.push_back(m_NumberSigmaLevels->text().toInt());

	// extract IDs if any were set
	std::vector<int> object_ids;
	if (!m_SelectedObjects->text().isEmpty())
	{
		std::vector<std::string> tokens;
		std::string selected_objects_text = m_SelectedObjects->text().toStdString();
		boost::algorithm::split(tokens, selected_objects_text, boost::algorithm::is_any_of(","));
		std::transform(tokens.begin(), tokens.end(), std::back_inserter(object_ids), [](std::string s) {
			boost::algorithm::trim(s);
			return stoi(s);
		});
	}

	typename real_type::Pointer feature_image;
	feature_image = ComputeFeatureImage<input_type, real_type>(source);
	// mask feature image before skeletonization
	if (!object_ids.empty())
	{
		using map_functor = iseg::Functor::MapLabels<unsigned short, unsigned char>;
		map_functor map;
		map.m_Map.assign(m_Handler3D->TissueNames().size() + 1, 0);
		for (size_t i = 0; i < object_ids.size(); i++)
		{
			map.m_Map.at(object_ids[i]) = 1;
		}

		auto map_filter = itk::UnaryFunctorImageFilter<TTissue, mask_type, map_functor>::New();
		map_filter->SetFunctor(map);
		map_filter->SetInput(tissues);

		auto masker = itk::MaskImageFilter<real_type, mask_type>::New();
		masker->SetInput(feature_image);
		masker->SetMaskImage(map_filter->GetOutput());
		SAFE_UPDATE(masker, return );
		feature_image = masker->GetOutput();
	}

	typename mask_type::Pointer skeleton;
	std::vector<double> skeleton_params(object_ids.begin(), object_ids.end());
	float lower(30);

	if (m_NonMaxSuppression->isChecked())
	{
		auto masking = itk::ThresholdImageFilter<real_type>::New();
		masking->SetInput(feature_image);
		masking->ThresholdBelow(lower);
		masking->SetOutsideValue(std::min(lower, 0.f));

		// do thinning to get brightest pixels ("one" pixel wide)
		auto nonmax_filter = nonmax_filter_type::New();
		nonmax_filter->SetInput(masking->GetOutput());

		auto threshold = threshold_filter_type::New();
		threshold->SetInput(nonmax_filter->GetOutput());
		threshold->SetLowerThreshold(std::nextafter(std::min(lower, 0.f), 1.f));
		SAFE_UPDATE(threshold, return );
		skeleton = threshold->GetOutput();
	}
	else
	{
		auto threshold = threshold_filter_type::New();
		threshold->SetInput(feature_image);
		threshold->SetLowerThreshold(lower);
		SAFE_UPDATE(threshold, return );
		skeleton = threshold->GetOutput();
	}

	if (m_Skeletonize->isChecked())
	{
		// get centerline: either thresholding or non-max suppression must be done before this
		auto thinning = thinnning_filter_type::New();
		thinning->SetInput(skeleton);

		auto rescale = itk::RescaleIntensityImageFilter<mask_type>::New();
		rescale->SetInput(thinning->GetOutput());
		rescale->SetOutputMinimum(0);
		rescale->SetOutputMaximum(255);
		rescale->InPlaceOn();

		SAFE_UPDATE(rescale, return );
		skeleton = rescale->GetOutput();
	}
	using BinaryImageToLabelMapFilterType = itk::BinaryImageToLabelMapFilter<mask_type, label_map_type>;

	typename mask_type::Pointer output;
	if (skeleton)
	{
		auto connectivity = itk::ConnectedComponentImageFilter<mask_type, labelfield_type>::New();
		connectivity->SetInput(skeleton);
		connectivity->FullyConnectedOn();

		auto relabel = itk::RelabelComponentImageFilter<labelfield_type, labelfield_type>::New();
		relabel->SetInput(connectivity->GetOutput());
		relabel->SetMinimumObjectSize(m_MinObjectSize->text().toInt());
		SAFE_UPDATE(relabel, return );

		auto threshold = itk::BinaryThresholdImageFilter<labelfield_type, mask_type>::New();
		threshold->SetInput(relabel->GetOutput());
		threshold->SetLowerThreshold(m_Threshold->text().toInt());
		SAFE_UPDATE(threshold, return );

		output = threshold->GetOutput();

		//converting output to label map
		auto binary_image_to_label_map_filter = BinaryImageToLabelMapFilterType::New();
		binary_image_to_label_map_filter->SetInput(output);
		if (m_ConnectDots->isChecked())
			binary_image_to_label_map_filter->SetFullyConnected(true);
		else
			binary_image_to_label_map_filter->SetFullyConnected(false);
		SAFE_UPDATE(binary_image_to_label_map_filter, return );

		iseg::SlicesHandlerITKInterface itk_handler(m_Handler3D);
		std::vector<std::string> label_object_list;

		auto label_map = binary_image_to_label_map_filter->GetOutput();
		auto map = CalculateLabelMapParams(label_map);
		using LabelToParams = std::map<label_type, std::vector<double>>;

		LabelToParams label_to_params = GetLabelMapParams(map);

		std::vector<std::string> labels;
		for (auto& filter : m_KFilters)
			labels.push_back(filter.GetLabel());

		if (!m_Add->isChecked())
		{
			for (unsigned int i = 0; i < map->GetNumberOfLabelObjects(); i++)
			{

				// Get the ith region
				auto label_object = map->GetNthLabelObject(i);
				label_object_list.push_back(std::to_string(label_object->GetLabel()));
			}
			m_Objects[m_Handler3D->ActiveSlice()] = label_object_list;
			m_LabelMaps[m_Handler3D->ActiveSlice()] = map;
		}
		else
		{
			auto old_map = m_LabelMaps[m_Handler3D->ActiveSlice()];
			label_type last_label = old_map->GetLabels().back();
			for (unsigned int i(0); i < map->GetNumberOfLabelObjects(); i++)
			{
				map->GetNthLabelObject(i)->SetLabel(map->GetNthLabelObject(i)->GetLabel() + last_label);
				SAFE_UPDATE(map, return );
				m_Objects[m_Handler3D->ActiveSlice()].push_back(std::to_string(map->GetNthLabelObject(i)->GetLabel()));
			}
			using MergeLabelFilterType = itk::MergeLabelMapFilter<label_map_type>;
			auto merger = MergeLabelFilterType::New();
			merger->SetInput(0, old_map);
			merger->SetInput(1, map);
			merger->SetMethod(MergeLabelFilterType::MethodChoice::KEEP);
			SAFE_UPDATE(merger, return );

			auto calculated_map = CalculateLabelMapParams(merger->GetOutput());

			m_LabelMaps[m_Handler3D->ActiveSlice()] = calculated_map;
		}
	}

	if (m_MaxActiveSliceReached < m_Handler3D->ActiveSlice())
		m_MaxActiveSliceReached = m_Handler3D->ActiveSlice();

	iseg::DataSelection data_selection;
	data_selection.allSlices = false; // all_slices->isChecked();
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.work = true;

	if (output && iseg::Paste<mask_type, input_type>(output, target))
	{

		// good, else maybe output is not defined
	}

	else if (!iseg::Paste<mask_type, input_type>(skeleton, target))
	{
		std::cerr << "Error: could not set output because image regions don't match.\n";
	}

	emit BeginDatachange(data_selection, this);
	emit EndDatachange(this);
	m_MaskImage = output;

	RefreshObjectList();
	RefreshKFilterList();
	VisualizeLabelMap(m_LabelMaps[m_Handler3D->ActiveSlice()]);
	m_CachedData.Store(m_LabelMaps, m_Objects, m_LabelToText, m_KFilters, m_Probabilities, m_MaxActiveSliceReached);
}

} // namespace iseg
