//
//  AutoTubePanel.cpp
//  iSegData
//
//  Created by Benjamin Fuhrer on 13.10.18.
//

#include "AutoTubePanel.h"

#include "Data/ItkUtils.h"
#include "Data/Logger.h"
#include "Data/SliceHandlerItkWrapper.h"

#include "Core/Morpho.h"

#include <QFileDialog>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QScrollArea>
#include <QVBoxLayout>

#include "itkBinaryThinningImageFilter3D.h"
#include "itkNonMaxSuppressionImageFilter.h"

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
#include <boost/lexical_cast.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include <Eigen/Dense>

#include <algorithm>
#include <fstream>

using namespace iseg;
using boost::adaptors::transformed;
using boost::algorithm::join;

template<typename TInputImage, typename TOutputImage, unsigned int Dimension>
class BinaryThinningImageFilter : public itk::BinaryThinningImageFilter<TInputImage, TOutputImage>
{
};

template<typename TInputImage, typename TOutputImage>
class BinaryThinningImageFilter<TInputImage, TOutputImage, 3> : public itk::BinaryThinningImageFilter3D<TInputImage, TOutputImage>
{
};

typedef unsigned long LabelType;
typedef itk::ShapeLabelObject<LabelType, 2> LabelObjectType;
typedef itk::LabelMap<LabelObjectType> LabelMapType;

AutoTubePanel::AutoTubePanel(iseg::SliceHandlerInterface* hand3D, QWidget* parent,
		const char* name, Qt::WindowFlags wFlags)
		: WidgetInterface(parent, name, wFlags), _handler3D(hand3D)
{
	setToolTip(Format("Kalman filter based root tracer"));

	_execute_button = new QPushButton("Execute");
	_execute_button->setMaximumSize(_execute_button->minimumSizeHint());
	_remove_button = new QPushButton("Remove Object");
	_remove_button->setMaximumSize(_remove_button->minimumSizeHint());
	_extrapolate_button = new QPushButton("Extrapolate");
	_extrapolate_button->setMaximumSize(_extrapolate_button->minimumSizeHint());
	_merge_button = new QPushButton("Merge Selected List Items");
	_merge_button->setMaximumSize(_merge_button->minimumSizeHint());
	_select_objects_button = new QPushButton("Select Mask");
	_select_objects_button->setMaximumSize(_select_objects_button->minimumSizeHint());
	_visualize_button = new QPushButton("See Label Map");
	_update_kfilter_button = new QPushButton("Update Kalman Filters");
	_remove_k_filter = new QPushButton("Remove Kalman Filter");
	_save = new QPushButton("Save Parameters");
	_load = new QPushButton("Load Parameters");
	_remove_non_selected = new QPushButton("Remove Non Selected");
	_add_to_tissues = new QPushButton("Add To Tissues");
	_export_lines = new QPushButton("Export lines");
	_k_filter_predict = new QPushButton("Predict Root Positions");

	int width(80);

	QLabel* _min = new QLabel("Sigma Min");
	QLabel* _max = new QLabel("Sigma Max");
	QLabel* _num = new QLabel("Number of Sigmas");
	QLabel* _feature_th = new QLabel("Feature Threshold");
	QLabel* _non_max = new QLabel("Non-maximum Suppression");
	QLabel* _add_l = new QLabel("Add To Existing Label Map");
	QLabel* _centerlines = new QLabel("Centerlines");
	QLabel* _min_obj_size = new QLabel("Min Object Size");
	QLabel* _slice_l = new QLabel("Slice Object List");
	QLabel* _obj_prob_l = new QLabel("Probability List");
	QLabel* _min_p = new QLabel("Minimum Probability");
	QLabel* _w_d = new QLabel("Weight: Distance");
	QLabel* _w_pa = new QLabel("Weight: Parameters");
	QLabel* _w_pr = new QLabel("Weight: Prediction");
	QLabel* _limit_s = new QLabel("Extrapolate To Slice");
	QLabel* _k_filters_list = new QLabel("Kalman Filter List");
	QLabel* _r_k_filter = new QLabel("Restart Kalman Filter");
	QLabel* _connect_d = new QLabel("Connect Dots");
	QLabel* _extra_only = new QLabel("Keep Only Matches");
	QLabel* _addPix = new QLabel("Add Pixel");
	QLabel* _line_radius_l = new QLabel("Line Radius");

	_sigma_low = new QLineEdit(QString::number(0.3));
	_sigma_low->setValidator(new QDoubleValidator);
	_sigma_low->setFixedWidth(width);

	_min_probability = new QLineEdit(QString::number(0.5));
	_min_probability->setValidator(new QDoubleValidator);
	_min_probability->setFixedWidth(width);

	_w_distance = new QLineEdit(QString::number(0.3));
	_w_distance->setValidator(new QDoubleValidator);
	_w_distance->setFixedWidth(width);

	_w_params = new QLineEdit(QString::number(0.3));
	_w_params->setValidator(new QDoubleValidator);
	_w_params->setFixedWidth(width);

	_w_pred = new QLineEdit(QString::number(0.3));
	_w_pred->setValidator(new QDoubleValidator);
	_w_pred->setFixedWidth(width);

	_sigma_hi = new QLineEdit(QString::number(0.6));
	_sigma_hi->setValidator(new QDoubleValidator);
	_sigma_hi->setFixedWidth(width);

	_number_sigma_levels = new QLineEdit(QString::number(2));
	_number_sigma_levels->setValidator(new QIntValidator);
	_number_sigma_levels->setFixedWidth(width);

	_threshold = new QLineEdit(QString::number(3));
	_threshold->setValidator(new QDoubleValidator);
	_threshold->setFixedWidth(width);

	_max_radius = new QLineEdit(QString::number(1));
	_max_radius->setValidator(new QDoubleValidator);
	_max_radius->setFixedWidth(width);

	_min_object_size = new QLineEdit(QString::number(1));
	_min_object_size->setValidator(new QIntValidator);
	_min_object_size->setFixedWidth(width);

	_selected_objects = new QLineEdit;
	_selected_objects->setReadOnly(true);
	_selected_objects->setFixedWidth(width);

	_limit_slice = new QLineEdit();
	_limit_slice->setValidator(new QIntValidator);
	_limit_slice->setFixedWidth(width);

	_line_radius = new QLineEdit();
	_line_radius->setValidator(new QDoubleValidator);
	_line_radius->setFixedWidth(width);

	_skeletonize = new QCheckBox;
	_skeletonize->setChecked(true);
	_skeletonize->setToolTip(Format("Compute 1-pixel wide centerlines (skeleton)."));

	_connect_dots = new QCheckBox;
	_connect_dots->setChecked(false);

	_non_max_suppression = new QCheckBox;
	_non_max_suppression->setChecked(true);
	_non_max_suppression->setToolTip(Format("Extract approx. one pixel wide paths based on non-maximum suppression."));

	_add = new QCheckBox;
	_add->setChecked(false);
	_add->setToolTip(Format("Add New Objects To Label Map"));

	_restart_k_filter = new QCheckBox;
	_restart_k_filter->setChecked(false);

	_extrapolate_only_matches = new QCheckBox;
	_extrapolate_only_matches->setChecked(true);

	_add_pixel = new QCheckBox;
	_add_pixel->setChecked(false);

	object_list = new QListWidget();
	object_list->setSelectionMode(QAbstractItemView::ExtendedSelection);

	object_probability_list = new QListWidget();
	object_probability_list->setSelectionMode(QAbstractItemView::ExtendedSelection);

	k_filters_list = new QListWidget();
	k_filters_list->setSelectionMode(QAbstractItemView::ExtendedSelection);

	QVBoxLayout* vbox1 = new QVBoxLayout;
	vbox1->addWidget(_slice_l);
	vbox1->addWidget(object_list);

	QVBoxLayout* vbox2 = new QVBoxLayout;
	vbox2->addWidget(_limit_s);
	vbox2->addWidget(_limit_slice);
	vbox2->addWidget(_select_objects_button);
	vbox2->addWidget(_selected_objects);
	vbox2->addWidget(_add_l);
	vbox2->addWidget(_add);
	vbox2->addWidget(_r_k_filter);
	vbox2->addWidget(_restart_k_filter);
	vbox2->addWidget(_extra_only);
	vbox2->addWidget(_extrapolate_only_matches);
	vbox2->addWidget(_addPix);
	vbox2->addWidget(_add_pixel);

	QVBoxLayout* vbox3 = new QVBoxLayout;
	vbox3->addWidget(_k_filters_list);
	vbox3->addWidget(k_filters_list);

	QVBoxLayout* vbox4 = new QVBoxLayout;
	vbox4->addWidget(_remove_non_selected);
	vbox4->addWidget(_update_kfilter_button);
	vbox4->addWidget(_visualize_button);
	vbox4->addWidget(_save);
	vbox4->addWidget(_load);
	vbox4->addWidget(_add_to_tissues);
	vbox4->addWidget(_export_lines);

	QHBoxLayout* hbox1 = new QHBoxLayout;
	hbox1->addLayout(vbox4);
	hbox1->addLayout(vbox1);
	hbox1->addLayout(vbox2);
	hbox1->addLayout(vbox3);

	QVBoxLayout* vbox5 = new QVBoxLayout;
	vbox5->addWidget(_min);
	vbox5->addWidget(_sigma_low);
	vbox5->addWidget(_max);
	vbox5->addWidget(_sigma_hi);
	vbox5->addWidget(_num);
	vbox5->addWidget(_number_sigma_levels);
	vbox5->addWidget(_feature_th);
	vbox5->addWidget(_threshold);

	QVBoxLayout* vbox6 = new QVBoxLayout;
	vbox6->addWidget(_non_max);
	vbox6->addWidget(_non_max_suppression);
	vbox6->addWidget(_centerlines);
	vbox6->addWidget(_skeletonize);
	vbox6->addWidget(_min_obj_size);
	vbox6->addWidget(_min_object_size);
	vbox6->addWidget(_connect_d);
	vbox6->addWidget(_connect_dots);
	vbox6->addWidget(_line_radius_l);
	vbox6->addWidget(_line_radius);

	QVBoxLayout* vbox8 = new QVBoxLayout;
	vbox8->addWidget(_min_p);
	vbox8->addWidget(_min_probability);
	vbox8->addWidget(_w_d);
	vbox8->addWidget(_w_distance);
	vbox8->addWidget(_w_pa);
	vbox8->addWidget(_w_params);
	vbox8->addWidget(_w_pr);
	vbox8->addWidget(_w_pred);

	QVBoxLayout* vbox9 = new QVBoxLayout;
	vbox9->addWidget(_obj_prob_l);
	vbox9->addWidget(object_probability_list);

	QHBoxLayout* hbox2 = new QHBoxLayout;
	hbox2->addLayout(vbox5);
	hbox2->addLayout(vbox6);
	hbox2->addLayout(vbox9);
	hbox2->addLayout(vbox8);

	QHBoxLayout* hbox3 = new QHBoxLayout;
	hbox3->addWidget(_merge_button);
	hbox3->addWidget(_remove_button);
	hbox3->addWidget(_remove_k_filter);
	hbox3->addWidget(_extrapolate_button);
	hbox3->addWidget(_k_filter_predict);
	hbox3->addWidget(_execute_button);

	auto layout = new QFormLayout;
	layout->addRow(hbox3);
	layout->addRow(hbox1);
	layout->addRow(hbox2);

	auto big_view = new QWidget;
	big_view->setLayout(layout);

	auto scroll_area = new QScrollArea(this);
	scroll_area->setWidget(big_view);

	auto top_layout = new QGridLayout(1, 1);
	top_layout->addWidget(scroll_area, 0, 0);
	setLayout(top_layout);

	QObject::connect(_select_objects_button, SIGNAL(clicked()), this, SLOT(select_objects()));
	QObject::connect(_execute_button, SIGNAL(clicked()), this, SLOT(do_work()));
	QObject::connect(object_list, SIGNAL(itemSelectionChanged()), this, SLOT(item_selected()));
	QObject::connect(object_list, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(item_double_clicked(QListWidgetItem*)));
	QObject::connect(_merge_button, SIGNAL(clicked()), this, SLOT(merge_selected_items()));
	QObject::connect(_extrapolate_button, SIGNAL(clicked()), this, SLOT(extrapolate_results()));
	QObject::connect(_visualize_button, SIGNAL(clicked()), this, SLOT(visualize()));
	QObject::connect(_update_kfilter_button, SIGNAL(clicked()), this, SLOT(update_kalman_filters()));
	QObject::connect(_remove_button, SIGNAL(clicked()), this, SLOT(remove_object()));
	QObject::connect(_remove_k_filter, SIGNAL(clicked()), this, SLOT(remove_k_filter()));
	QObject::connect(k_filters_list, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(k_filter_double_clicked(QListWidgetItem*)));
	QObject::connect(_save, SIGNAL(clicked()), this, SLOT(save()));
	QObject::connect(_load, SIGNAL(clicked()), this, SLOT(load()));
	QObject::connect(_remove_non_selected, SIGNAL(clicked()), this, SLOT(remove_non_selected()));
	QObject::connect(_add_to_tissues, SIGNAL(clicked()), this, SLOT(add_to_tissues()));
	QObject::connect(_export_lines, SIGNAL(clicked()), this, SLOT(export_lines()));
	QObject::connect(_k_filter_predict, SIGNAL(clicked()), this, SLOT(predict_k_filter()));
}

void AutoTubePanel::predict_k_filter()
{
	if (k_filters.empty())
	{
		QMessageBox mBox;
		mBox.setWindowTitle("Error");
		mBox.setText("Cannot predict without any Kalman filters!");
		mBox.exec();
		return;
	}

	_cached_data.get(label_maps, objects, label_to_text, k_filters, _probabilities, max_active_slice_reached);
	typedef float PixelType;
	using ImageType = itk::Image<PixelType, 2>;

	for (auto filter : k_filters)
	{

		if (!label_maps[_handler3D->active_slice()])
		{
			typedef float PixelType;
			using ImageType = itk::Image<PixelType, 2>;
			ImageType::SizeType size{{384, 384}};
			LabelMapType::Pointer map = LabelMapType::New();
			itk::Index<2> index{{0, 0}};

			ImageType::RegionType region;
			region.SetSize(size);
			region.SetIndex(index);

			map->SetRegions(region);
			map->Allocate(true);
			map->Update();
			label_maps[_handler3D->active_slice()] = map;

			std::vector<std::string> list;
			objects[_handler3D->active_slice()] = list;
		}

		auto labelMap = label_maps[_handler3D->active_slice()];

		typedef itk::LabelMapToLabelImageFilter<LabelMapType, ImageType> Label2ImageType;
		auto label2image = Label2ImageType::New();
		label2image->SetInput(labelMap);
		label2image->Update();
		auto image = label2image->GetOutput();

		KalmanFilter copy = filter;
		copy.measurement_prediction();
		std::vector<double> prediction = copy.get_prediction();

		itk::Index<2> index;
		index[0] = prediction[0];
		index[1] = prediction[1];

		LabelType last_label = 0;
		if (!labelMap->GetLabels().empty())
			last_label = labelMap->GetLabels().back();

		if (image->GetBufferedRegion().IsInside(index))
		{
			LabelType found_label = labelMap->GetPixel(index);

			if (found_label == 0)
			{
				image->SetPixel(index, last_label + 1);

				image->Update();

				typedef itk::LabelImageToShapeLabelMapFilter<ImageType, LabelMapType> Image2LabelType;
				auto image2label = Image2LabelType::New();
				image2label->SetInput(image);
				image2label->Update();

				label_maps[_handler3D->active_slice()] = image2label->GetOutput();

				objects[_handler3D->active_slice()].push_back(filter.get_label());
			}
		}
	}

	auto labelObjects = label_maps[_handler3D->active_slice()]->GetLabelObjects();
	label_maps[_handler3D->active_slice()]->ClearLabels();
	for (auto& object : labelObjects)
		label_maps[_handler3D->active_slice()]->PushLabelObject(object);

	refresh_object_list();
	visualize_label_map(label_maps[_handler3D->active_slice()]);
	_cached_data.store(label_maps, objects, label_to_text, k_filters, _probabilities, max_active_slice_reached);
}

void AutoTubePanel::add_to_tissues()
{
	_cached_data.get(label_maps, objects, label_to_text, k_filters, _probabilities, max_active_slice_reached);
	QMessageBox mBox;
	mBox.setWindowTitle("");
	mBox.setText("Before continuing make sure that all the roots were found and labeled in all slices and that they all have kalman filters. In addition, make sure that all of these roots have tissues with their name before continuing");
	mBox.addButton(QMessageBox::No);
	mBox.addButton(QMessageBox::Yes);
	mBox.setDefaultButton(QMessageBox::No);
	if (mBox.exec() == QMessageBox::Yes)
	{
		if (k_filters.size() > 0 && max_active_slice_reached == _handler3D->num_slices() - 1)
		{
			// get Kalman filter labels
			std::vector<std::string> labels;
			for (auto filter : k_filters)
				labels.push_back(filter.get_label());

			iseg::SliceHandlerItkWrapper itk_handler(_handler3D);
			auto tissue_names = _handler3D->tissue_names();

			// shallow copy - used to transform index to world coordinates
			auto image_3d = itk_handler.GetSource(false);

			typedef float PixelType;
			using tissue_type = SliceHandlerInterface::tissue_type;
			using ImageType = itk::Image<PixelType, 2>;
			using ConstIteratorType = itk::ImageRegionConstIterator<ImageType>;
			using IteratorType = itk::ImageRegionIterator<itk::Image<tissue_type, 2>>;

			std::ofstream myfile;
			for (auto label : labels)
			{
				std::string filename = label + ".txt";
				myfile.open(filename);
				std::vector<std::string>::iterator it = std::find(tissue_names.begin(), tissue_names.end(), label);
				if (it == labels.end())
				{
					QMessageBox mBox;
					mBox.setWindowTitle("Error");
					mBox.setText("Roots have no tissues! First create a tissue in the tissue list!");
					mBox.exec();
					myfile.close();
					break;
				}

				bool draw_circle;
				double circle_radius = _line_radius->text().toDouble(&draw_circle);
				auto spacing = itk_handler.GetTissuesSlice(0)->GetSpacing();
				auto ball = morpho::MakeBall<2>(spacing, draw_circle ? circle_radius : 1.0);

				tissues_size_t tissue_number = std::distance(tissue_names.begin(), it);
				for (int i(0); i < _handler3D->num_slices(); i++)
				{
					itk::Image<tissue_type, 2>::Pointer tissue = itk_handler.GetTissuesSlice(i);
					auto labelMap = label_maps[i];

					labelMap = calculate_label_map_params(labelMap);

					it = std::find(objects[i].begin(), objects[i].end(), label);
					if (it != objects[i].end())
					{
						int row = std::distance(objects[i].begin(), it);
						auto labelObject = labelMap->GetLabelObject(row + 1);

						using Label2ImageType = itk::LabelMapToLabelImageFilter<LabelMapType, ImageType>;
						auto label2image = Label2ImageType::New();
						label2image->SetInput(labelMap);
						label2image->Update();

						ConstIteratorType in(label2image->GetOutput(), label2image->GetOutput()->GetRequestedRegion());
						IteratorType out(tissue, label2image->GetOutput()->GetRequestedRegion());
						double x(0);
						double y(0);
						int size(0);
						for (in.GoToBegin(), out.GoToBegin(); !in.IsAtEnd(); ++in, ++out)
						{
							if (labelMap->GetPixel(in.GetIndex()) == labelObject->GetLabel())
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
							ShapedNeighborhoodIterator siterator(ball.GetRadius(),
									tissue, tissue->GetLargestPossibleRegion());

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

			iseg::DataSelection dataSelection;
			dataSelection.allSlices = true;
			dataSelection.tissues = true;

			emit begin_datachange(dataSelection, this);
			emit end_datachange(this);

			QMessageBox mBox;
			mBox.setWindowTitle("");
			mBox.setText("Done");
			mBox.exec();
		}
		else
		{
			QMessageBox mBox;
			mBox.setWindowTitle("Error");
			mBox.setText("Only Click When All Roots are found in all slices and all have kalman filters in the kalman filter list!");
			mBox.exec();
		}
	}
}

void AutoTubePanel::export_lines()
{
	auto directory = QFileDialog::getExistingDirectory(this, "Open directory");
	if (directory.isEmpty())
		return;

	// get Kalman filter labels
	std::vector<std::string> labels;
	for (auto filter : k_filters)
		labels.push_back(filter.get_label());

	iseg::SliceHandlerItkWrapper itk_handler(_handler3D);
	auto tissue_names = _handler3D->tissue_names();

	// shallow copy - used to transform index to world coordinates
	auto image_3d = itk_handler.GetSource(false);

	typedef float PixelType;
	using tissue_type = SliceHandlerInterface::tissue_type;
	using ImageType = itk::Image<PixelType, 2>;
	using ConstIteratorType = itk::ImageRegionConstIterator<ImageType>;

	std::ofstream myfile;
	for (auto label : labels)
	{
		std::string filename = directory.toStdString() + label + ".txt";
		myfile.open(filename);
		auto it = std::find(tissue_names.begin(), tissue_names.end(), label);
		if (it == labels.end())
		{
			QMessageBox mBox;
			mBox.setWindowTitle("Error");
			mBox.setText("Roots have no tissues! First create a tissue in the tissue list!");
			mBox.exec();
			myfile.close();
			continue;
		}

		tissues_size_t tissue_number = std::distance(tissue_names.begin(), it);
		for (int i(0); i < _handler3D->num_slices(); i++)
		{
			auto tissue = itk_handler.GetTissuesSlice(i);
			auto labelMap = label_maps[i];

			labelMap = calculate_label_map_params(labelMap);

			it = std::find(objects[i].begin(), objects[i].end(), label);
			if (it != objects[i].end())
			{
				int row = std::distance(objects[i].begin(), it);
				auto labelObject = labelMap->GetLabelObject(row + 1);

				using Label2ImageType = itk::LabelMapToLabelImageFilter<LabelMapType, ImageType>;
				auto label2image = Label2ImageType::New();
				label2image->SetInput(labelMap);
				label2image->Update();

				ConstIteratorType in(label2image->GetOutput(), label2image->GetOutput()->GetRequestedRegion());
				double x(0);
				double y(0);
				int size(0);
				for (in.GoToBegin(); !in.IsAtEnd(); ++in)
				{
					// why do we go via iterator to labelMap?
					if (labelMap->GetPixel(in.GetIndex()) == labelObject->GetLabel())
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
void AutoTubePanel::remove_non_selected()
{
	// get cached information
	_cached_data.get(label_maps, objects, label_to_text, k_filters, _probabilities, max_active_slice_reached);

	// making sure that they are selected objects
	if (selected.size() > 0)
	{
		std::vector<int> rows_to_delete;

		// find all rows that are not selected
		for (int i(0); i < objects[_handler3D->active_slice()].size(); i++)
		{
			std::vector<int>::iterator it = std::find(selected.begin(), selected.end(), i);
			if (it == selected.end())
				rows_to_delete.push_back(i);
		}

		// delete rows to delete from current slice's object list
		// delete in descending order, i.e. from back of vector
		std::sort(rows_to_delete.begin(), rows_to_delete.end(), std::greater<int>());
		for (auto row : rows_to_delete)
		{
			label_maps[_handler3D->active_slice()]->RemoveLabel(row + 1);
			objects[_handler3D->active_slice()].erase(objects[_handler3D->active_slice()].begin() + row);
		}

		// delete rows to delete from current slice's probabilities list
		if (_handler3D->active_slice() != 0)
		{
			for (auto row : rows_to_delete)
			{
				_probabilities[_handler3D->active_slice()] = modify_probability(_probabilities[_handler3D->active_slice()], std::to_string(row + 1), true, false);
			}
		}

		auto labelObjects = label_maps[_handler3D->active_slice()]->GetLabelObjects();

		// rename elements in probabilities list to correspond to the new indexes of remaining objects
		if (_handler3D->active_slice() != 0)
		{
			for (unsigned int i(0); i < label_maps[_handler3D->active_slice()]->GetNumberOfLabelObjects(); i++)
			{
				_probabilities[_handler3D->active_slice()] = modify_probability(_probabilities[_handler3D->active_slice()], std::to_string(label_maps[_handler3D->active_slice()]->GetNthLabelObject(i)->GetLabel()), false, true, std::to_string(i + 1));
			}
		}
		// rename remaining labels of label map to 1 until number of label objects
		label_maps[_handler3D->active_slice()]->ClearLabels();
		for (auto& object : labelObjects)
			label_maps[_handler3D->active_slice()]->PushLabelObject(object);

		refresh_object_list();
		refresh_probability_list();

		visualize_label_map(label_maps[_handler3D->active_slice()]);

		_cached_data.store(label_maps, objects, label_to_text, k_filters, _probabilities, max_active_slice_reached);

		selected.clear();
	}
}

void AutoTubePanel::save_label_map(FILE* fp, LabelMapType::Pointer map)
{

	typedef float PixelType;
	using ImageType = itk::Image<PixelType, 2>;

	typedef itk::LabelMapToLabelImageFilter<LabelMapType, ImageType> Label2ImageType;
	auto label2image = Label2ImageType::New();
	label2image->SetInput(map);

	using ConstIteratorType = itk::ImageRegionConstIterator<ImageType>;
	SAFE_UPDATE(label2image, return );
	int dummy;

	// save size
	dummy = label2image->GetOutput()->GetRequestedRegion().GetSize()[0];
	fwrite(&dummy, sizeof(int), 1, fp);

	dummy = label2image->GetOutput()->GetRequestedRegion().GetSize()[1];
	fwrite(&dummy, sizeof(int), 1, fp);
	// GetPixel() returns the label for the given index -> we save the labels for each pixel
	ConstIteratorType in(label2image->GetOutput(), label2image->GetOutput()->GetRequestedRegion());
	for (in.GoToBegin(); !in.IsAtEnd(); ++in)
	{
		dummy = map->GetPixel(in.GetIndex());
		fwrite(&dummy, sizeof(int), 1, fp);
	}

	std::map<LabelType, std::vector<double>> params = get_label_map_params(map);
	// saving paramters of label objects
	for (auto it : params)
	{
		for (auto parameter : it.second)
		{
			double temp = parameter;
			fwrite(&temp, sizeof(double), 1, fp);
		}
	}
}

void AutoTubePanel::save_l_to_t(FILE* fp, std::map<LabelType, std::string> l_to_t)
{
	// save labelto text mapping

	int size = l_to_t.size();
	fwrite(&size, sizeof(int), 1, fp);

	for (auto const& element : l_to_t)
	{
		fwrite(&(element.first), sizeof(LabelType), 1, fp);
		fwrite(&(element.second), sizeof(std::string), 1, fp);
	}
}
std::map<LabelType, std::string> AutoTubePanel::load_l_to_t(FILE* fi)
{

	// load label to text mapping

	std::map<LabelType, std::string> mapping;
	int size;
	fread(&size, sizeof(int), 1, fi);

	for (unsigned int j(0); j < size; j++)
	{
		LabelType label;
		fread(&label, sizeof(LabelType), 1, fi);
		std::string text;
		fread(&text, sizeof(std::string), 1, fi);
		mapping[label] = text;
	}
	return mapping;
}

LabelMapType::Pointer AutoTubePanel::load_label_map(FILE* fi)
{
	LabelMapType::Pointer map = LabelMapType::New();

	int x;
	int y;
	// load size
	fread(&x, sizeof(int), 1, fi);
	fread(&y, sizeof(int), 1, fi);

	int value;
	itk::Index<2> index{{0, 0}};

	typedef float PixelType;
	using ImageType = itk::Image<PixelType, 2>;
	ImageType::SizeType size{{384, 384}};

	size[0] = x;
	size[1] = y;

	ImageType::RegionType region;
	region.SetSize(size);
	region.SetIndex(index);

	map->SetRegions(region);
	map->Allocate(true);
	map->Update();

	typedef itk::LabelMapToLabelImageFilter<LabelMapType, ImageType> Label2ImageType;
	auto label2image = Label2ImageType::New();
	label2image->SetInput(map);
	label2image->Update();

	using ConstIteratorType = itk::ImageRegionConstIterator<ImageType>;
	ConstIteratorType in(label2image->GetOutput(), label2image->GetOutput()->GetRequestedRegion());
	for (in.GoToBegin(); !in.IsAtEnd(); ++in)
	{
		fread(&value, sizeof(int), 1, fi);
		map->SetPixel(in.GetIndex(), value);
	}

	map = calculate_label_map_params(map);

	double param;
	for (auto& labelObject : map->GetLabelObjects())
	{
		double centroid_x;
		double centroid_y;
		fread(&centroid_x, sizeof(double), 1, fi);
		fread(&centroid_y, sizeof(double), 1, fi);
		itk::Point<double, 2> centroid;
		centroid[0] = centroid_x;
		centroid[1] = centroid_y;
		labelObject->SetCentroid(centroid);
		fread(&param, sizeof(double), 1, fi);
		labelObject->SetEquivalentSphericalPerimeter(param);
		fread(&param, sizeof(double), 1, fi);
		labelObject->SetEquivalentSphericalRadius(param);
		fread(&param, sizeof(double), 1, fi);
		labelObject->SetFeretDiameter(param);
		fread(&param, sizeof(double), 1, fi);
		labelObject->SetFlatness(param);
		fread(&param, sizeof(double), 1, fi);
		labelObject->SetNumberOfPixels(param);
		fread(&param, sizeof(double), 1, fi);
		labelObject->SetNumberOfPixelsOnBorder(param);
		fread(&param, sizeof(double), 1, fi);
		labelObject->SetPerimeter(param);
		fread(&param, sizeof(double), 1, fi);
		labelObject->SetPerimeterOnBorder(param);
		fread(&param, sizeof(double), 1, fi);
		labelObject->SetPerimeterOnBorderRatio(param);
		fread(&param, sizeof(double), 1, fi);
		labelObject->SetPhysicalSize(param);
		fread(&param, sizeof(double), 1, fi);
		labelObject->SetRoundness(param);
	}

	map->Update();

	return map;
}

void AutoTubePanel::save_k_filter(FILE* fp, std::vector<KalmanFilter> filters)
{
	int size = filters.size();
	fwrite(&size, sizeof(int), 1, fp);
	for (auto const& element : filters)
	{

		Eigen::VectorXd z = element.get_z();
		for (unsigned int i(0); i < z.size(); i++)
		{
			fwrite(&z(i), sizeof(double), 1, fp);
		}
		Eigen::VectorXd z_hat = element.get_z_hat();
		for (unsigned int i(0); i < z_hat.size(); i++)
		{
			fwrite(&z_hat(i), sizeof(double), 1, fp);
		}

		Eigen::VectorXd x = element.get_x();
		for (unsigned int i(0); i < x.size(); i++)
		{
			fwrite(&x(i), sizeof(double), 1, fp);
		}

		Eigen::VectorXd v = element.get_v();
		for (unsigned int i(0); i < v.size(); i++)
		{
			fwrite(&v(i), sizeof(double), 1, fp);
		}

		Eigen::VectorXd n = element.get_n();
		for (unsigned int i(0); i < n.size(); i++)
		{
			fwrite(&n(i), sizeof(double), 1, fp);
		}

		Eigen::VectorXd m = element.get_m();
		for (unsigned int i(0); i < m.size(); i++)
		{
			fwrite(&m(i), sizeof(double), 1, fp);
		}

		Eigen::MatrixXd F = element.get_F();
		for (unsigned int i(0); i < F.rows(); i++)
		{
			for (unsigned int j(0); j < F.cols(); j++)
			{
				fwrite(&F(i, j), sizeof(double), 1, fp);
			}
		}

		Eigen::MatrixXd H = element.get_H();
		for (unsigned int i(0); i < H.rows(); i++)
		{
			for (unsigned int j(0); j < H.cols(); j++)
			{
				fwrite(&H(i, j), sizeof(double), 1, fp);
			}
		}

		Eigen::MatrixXd P = element.get_P();
		for (unsigned int i(0); i < P.rows(); i++)
		{
			for (unsigned int j(0); j < P.cols(); j++)
			{
				fwrite(&P(i, j), sizeof(double), 1, fp);
			}
		}

		Eigen::MatrixXd Q = element.get_Q();
		for (unsigned int i(0); i < Q.rows(); i++)
		{
			for (unsigned int j(0); j < Q.cols(); j++)
			{
				fwrite(&Q(i, j), sizeof(double), 1, fp);
			}
		}

		Eigen::MatrixXd R = element.get_R();
		for (unsigned int i(0); i < R.rows(); i++)
		{
			for (unsigned int j(0); j < R.cols(); j++)
			{
				fwrite(&R(i, j), sizeof(double), 1, fp);
			}
		}

		Eigen::MatrixXd W = element.get_W();
		for (unsigned int i(0); i < W.rows(); i++)
		{
			for (unsigned int j(0); j < W.cols(); j++)
			{
				fwrite(&W(i, j), sizeof(double), 1, fp);
			}
		}

		Eigen::MatrixXd S = element.get_S();
		for (unsigned int i(0); i < S.rows(); i++)
		{
			for (unsigned int j(0); j < S.cols(); j++)
			{
				fwrite(&S(i, j), sizeof(double), 1, fp);
			}
		}
		int slice = element.get_slice();
		int iteration = element.get_iteration();
		int last_slice = element.get_last_slice();
		std::string label = element.get_label();

		fwrite(&slice, sizeof(int), 1, fp);
		fwrite(&iteration, sizeof(int), 1, fp);
		fwrite(&last_slice, sizeof(int), 1, fp);
		int Nstring = label.size();
		fwrite(&Nstring, sizeof(int), 1, fp);
		fwrite(label.c_str(), 1, Nstring, fp);
	}
}

std::vector<KalmanFilter> AutoTubePanel::load_k_filters(FILE* fi)
{
	std::vector<KalmanFilter> filters;

	int size;
	fread(&size, sizeof(int), 1, fi);
	for (unsigned int ind(0); ind < size; ind++)
	{

		KalmanFilter k;

		int N = k.N;

		Eigen::VectorXd z(N);
		for (unsigned int i(0); i < N; i++)
		{
			double dummy;
			fread(&dummy, sizeof(double), 1, fi);
			z(i) = dummy;
		}

		Eigen::VectorXd z_hat(N);
		for (unsigned int i(0); i < N; i++)
		{
			double dummy;
			fread(&dummy, sizeof(double), 1, fi);
			z_hat(i) = dummy;
		}

		Eigen::VectorXd x(N);
		for (unsigned int i(0); i < N; i++)
		{
			double dummy;
			fread(&dummy, sizeof(double), 1, fi);
			x(i) = dummy;
		}

		Eigen::VectorXd v(N);
		for (unsigned int i(0); i < N; i++)
		{
			double dummy;
			fread(&dummy, sizeof(double), 1, fi);
			v(i) = dummy;
		}

		Eigen::VectorXd n(N);
		for (unsigned int i(0); i < N; i++)
		{
			double dummy;
			fread(&dummy, sizeof(double), 1, fi);
			n(i) = dummy;
		}

		Eigen::VectorXd m(N);
		for (unsigned int i(0); i < N; i++)
		{
			double dummy;
			fread(&dummy, sizeof(double), 1, fi);
			m(i) = dummy;
		}

		Eigen::MatrixXd F(N, N);
		for (unsigned int i(0); i < F.rows(); i++)
		{
			for (unsigned int j(0); j < F.cols(); j++)
			{
				double dummy;
				fread(&dummy, sizeof(double), 1, fi);
				F(i, j) = dummy;
			}
		}

		Eigen::MatrixXd H(N, N);
		for (unsigned int i(0); i < H.rows(); i++)
		{
			for (unsigned int j(0); j < H.cols(); j++)
			{
				double dummy;
				fread(&dummy, sizeof(double), 1, fi);
				H(i, j) = dummy;
			}
		}

		Eigen::MatrixXd P(N, N);
		for (unsigned int i(0); i < P.rows(); i++)
		{
			for (unsigned int j(0); j < P.cols(); j++)
			{
				double dummy;
				fread(&dummy, sizeof(double), 1, fi);
				P(i, j) = dummy;
			}
		}

		Eigen::MatrixXd Q(N, N);
		for (unsigned int i(0); i < Q.rows(); i++)
		{
			for (unsigned int j(0); j < Q.cols(); j++)
			{
				double dummy;
				fread(&dummy, sizeof(double), 1, fi);
				Q(i, j) = dummy;
			}
		}

		Eigen::MatrixXd R(N, N);
		for (unsigned int i(0); i < R.rows(); i++)
		{
			for (unsigned int j(0); j < R.cols(); j++)
			{
				double dummy;
				fread(&dummy, sizeof(double), 1, fi);
				R(i, j) = dummy;
			}
		}

		Eigen::MatrixXd W(N, N);
		for (unsigned int i(0); i < W.rows(); i++)
		{
			for (unsigned int j(0); j < W.cols(); j++)
			{
				double dummy;
				fread(&dummy, sizeof(double), 1, fi);
				W(i, j) = dummy;
			}
		}

		Eigen::MatrixXd S(N, N);
		for (unsigned int i(0); i < S.rows(); i++)
		{
			for (unsigned int j(0); j < S.cols(); j++)
			{
				double dummy;
				fread(&dummy, sizeof(double), 1, fi);
				S(i, j) = dummy;
			}
		}

		k.set_z(z);
		k.set_z_hat(z_hat);
		k.set_x(x);
		k.set_v(v);
		k.set_n(n);
		k.set_m(m);
		k.set_F(F);
		k.set_P(P);
		k.set_H(H);
		k.set_Q(Q);
		k.set_R(R);
		k.set_W(W);
		k.set_S(S);

		int slice;
		fread(&slice, sizeof(int), 1, fi);
		int iteration;
		fread(&iteration, sizeof(int), 1, fi);
		int last_slice;
		fread(&last_slice, sizeof(int), 1, fi);
		int Nstring;
		fread(&Nstring, sizeof(int), 1, fi);
		std::vector<char> content(Nstring, 0);
		fread(&content[0], 1, Nstring, fi);
		std::string label(content.begin(), content.end());

		k.set_slice(slice);
		k.set_iteration(iteration);
		k.set_last_slice(last_slice);
		k.set_label(label);
		filters.push_back(k);
	}
	return filters;
}

void AutoTubePanel::save()
{
	_cached_data.get(label_maps, objects, label_to_text, k_filters, _probabilities, max_active_slice_reached);
	QString savefilename = QFileDialog::getSaveFileName(
			QString::null, "Projects (*.prj)\n", this); //, filename);

	if (!savefilename.isEmpty())
	{
		if (savefilename.length() <= 4 || !savefilename.endsWith(QString(".prj")))
			savefilename.append(".prj");
		FILE* fo;
		fo = fopen(savefilename.ascii(), "wb");

		int dummy;
		dummy = _handler3D->num_slices();
		fwrite(&dummy, sizeof(int), 1, fo);

		dummy = max_active_slice_reached;
		fwrite(&dummy, sizeof(int), 1, fo);

		for (unsigned int i(0); i <= max_active_slice_reached; i++)
		{

			dummy = _cached_data.objects[i].size();
			fwrite(&dummy, sizeof(int), 1, fo);
			for (unsigned int j(0); j < _cached_data.objects[i].size(); j++)
			{
				int N = _cached_data.objects[i][j].size();
				fwrite(&N, sizeof(int), 1, fo);
				fwrite(_cached_data.objects[i][j].c_str(), 1, N, fo);
			}
		}

		for (unsigned int i(0); i <= max_active_slice_reached; i++)
		{
			save_label_map(fo, _cached_data.label_maps[i]);
		}
		for (unsigned int i(0); i <= max_active_slice_reached; i++)
		{
			save_l_to_t(fo, _cached_data.label_to_text[i]);
		}

		save_k_filter(fo, _cached_data.k_filters);

		for (unsigned int i(0); i <= max_active_slice_reached; i++)
		{
			dummy = _cached_data._probabilities[i].size();
			fwrite(&dummy, sizeof(int), 1, fo);
			for (auto const& prob : _cached_data._probabilities[i])
			{
				int N = prob.size();
				fwrite(&N, sizeof(int), 1, fo);
				fwrite(prob.c_str(), 1, N, fo);
			}
		}

		fclose(fo);
		QMessageBox mBox;
		mBox.setWindowTitle("Saving");
		mBox.setText(QString::fromStdString("Saving Finished"));
		mBox.exec();
	}
}

void AutoTubePanel::load()
{
	_cached_data.get(label_maps, objects, label_to_text, k_filters, _probabilities, max_active_slice_reached);
	QString loadfilename = QFileDialog::getOpenFileName(QString::null,
			"Projects (*.prj)\n"
			"All (*.*)",
			this); //, filename);
	if (!loadfilename.isEmpty())
	{
		FILE* fi;
		fi = fopen(loadfilename.ascii(), "rb");

		int max_num_slices;
		fread(&max_num_slices, sizeof(int), 1, fi);

		int num_slices;
		fread(&num_slices, sizeof(int), 1, fi);

		max_active_slice_reached = num_slices;

		std::vector<std::vector<std::string>> objs(max_num_slices);
		std::vector<LabelMapType::Pointer> l_maps(max_num_slices);
		std::vector<std::map<LabelType, std::string>> l_to_t(max_num_slices);
		std::vector<std::vector<std::string>> probabilities(max_num_slices);
		std::vector<KalmanFilter> k_fs;

		for (unsigned int i(0); i <= num_slices; i++)
		{
			int size;
			fread(&size, sizeof(int), 1, fi);

			std::vector<std::string> vec;
			for (unsigned int j(0); j < size; j++)
			{
				int N;
				fread(&N, sizeof(int), 1, fi);
				std::vector<char> content(N, 0);
				fread(&content[0], 1, N, fi);
				std::string str(content.begin(), content.end());
				vec.push_back(str);
			}
			objs[i] = vec;
		}

		for (unsigned int i(0); i <= num_slices; i++)
		{

			LabelMapType::Pointer label_map = load_label_map(fi);
			visualize_label_map(label_map);
			l_maps[i] = label_map;
		}

		for (unsigned int i(0); i <= num_slices; i++)
		{

			l_to_t[i] = load_l_to_t(fi);
		}

		k_fs = load_k_filters(fi);

		for (unsigned int i(0); i <= num_slices; i++)
		{

			std::vector<std::string> vec;
			int size;
			fread(&size, sizeof(int), 1, fi);
			for (unsigned int j(0); j < size; j++)
			{
				int N;
				fread(&N, sizeof(int), 1, fi);
				std::vector<char> content(N, 0);
				fread(&content[0], 1, N, fi);
				// Construct the string (skip this, if you read into the string directly)
				std::string str(content.begin(), content.end());

				vec.push_back(str);
			}
			probabilities[i] = vec;
		}

		fclose(fi);

		_cached_data.store(l_maps, objs, l_to_t, k_fs, probabilities, max_active_slice_reached);
		objects = objs;
		_probabilities = probabilities;
		label_maps = l_maps;
		k_filters = k_fs;

		refresh_object_list();
		refresh_probability_list();
		refresh_k_filter_list();
		QMessageBox mBox;
		mBox.setWindowTitle("Loading");
		mBox.setText(QString::fromStdString("Loading Finished"));
		mBox.exec();
	}
}

void AutoTubePanel::k_filter_double_clicked(QListWidgetItem* item)
{
	// get cached data
	_cached_data.get(label_maps, objects, label_to_text, k_filters, _probabilities, max_active_slice_reached);
	std::vector<std::string> labels;
	for (auto filter : k_filters)
		labels.push_back(filter.get_label());

	QMessageBox mBox;
	std::stringstream buffer;
	//auto split= split_string(item->text().toStdString());

	std::string label = item->text().toStdString();
	const std::vector<std::string>::iterator it = std::find(labels.begin(), labels.end(), label);
	int index = std::distance(labels.begin(), it);
	buffer << k_filters[index] << std::endl;
	if (_restart_k_filter->isChecked())
	{
		KalmanFilter k;
		k.set_label(k_filters[index].get_label());
		k.set_slice(k_filters[index].get_slice());
		k_filters[index] = k;
	}
	else
	{
		mBox.setWindowTitle("Filters");
		mBox.setText(QString::fromStdString(buffer.str()));
		mBox.exec();
	}

	// store cached data
	_cached_data.store(label_maps, objects, label_to_text, k_filters, _probabilities, max_active_slice_reached);
}

void AutoTubePanel::remove_k_filter()
{
	QList<QListWidgetItem*> items = k_filters_list->selectedItems();
	// get all rows from the selected items
	std::vector<int> rows;
	for (auto item : items)
	{
		int row = k_filters_list->row(item);
		rows.push_back(row);
	}

	// delete in descending order, i.e. from back of vector
	std::sort(rows.begin(), rows.end(), std::greater<int>());
	for (auto row : rows)
	{
		k_filters.erase(k_filters.begin() + row);
	}

	refresh_k_filter_list();
	_cached_data.store(label_maps, objects, label_to_text, k_filters, _probabilities, max_active_slice_reached);
}

void AutoTubePanel::update_kalman_filters()
{

	if (k_filters.size() > 0 && label_maps[_handler3D->active_slice()])
	{
		// get cached data
		_cached_data.get(label_maps, objects, label_to_text, k_filters, _probabilities, max_active_slice_reached);
		// get Kalman filter laels
		std::vector<std::string> labels;
		for (auto filter : k_filters)
			labels.push_back(filter.get_label());
		bool not_in_k_filter_list(false);
		std::vector<std::string> objects_not_in_list;
		// for each slice
		for (int i(0); i <= _handler3D->active_slice(); i++) // \bug this always starts at slice 0 => it should store start slice, i.e. where user pressed execute first time
		{
			auto labelMap = label_maps[i];
			if (!labelMap)
			{
				QMessageBox mBox;
				mBox.setWindowTitle("Error");
				mBox.setText(QString::fromStdString("No label map for slice " + boost::lexical_cast<std::string>(i) + ". Tool expects you to start at slice 0."));
				mBox.exec();
				return;
			}

			typedef std::map<LabelType, std::vector<double>> LabelToParams;
			LabelToParams label_to_params = get_label_map_params(labelMap);
			// for each object in slice object list
			for (unsigned int row(0); row < objects[i].size(); row++)
			{
				std::string label = objects[i][row];
				auto labelObject = labelMap->GetNthLabelObject(row);

				std::vector<std::string>::iterator it = std::find(labels.begin(), labels.end(), label);
				if (it != labels.end())
				{
					int index = std::distance(labels.begin(), it);
					if (k_filters[index].get_slice() <= i + 1 && k_filters[index].get_last_slice() < i + 1)
					{
						std::vector<double> params = label_to_params[labelObject->GetLabel()];
						std::vector<double> needed_params = {params[0], params[1]};
						k_filters[index].set_measurement(needed_params);
						k_filters[index].work();
						k_filters[index].set_last_slice(i + 1);
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
			for (auto object : objects_not_in_list)
				message += object + "\n";

			QMessageBox mBox;
			mBox.setWindowTitle("Error");
			mBox.setText(QString::fromStdString(message) + "Don't Have Kalman Filters!");
			mBox.exec();
		}
		// store to cached data
		_cached_data.store(label_maps, objects, label_to_text, k_filters, _probabilities, max_active_slice_reached);
	}
	else
	{

		QMessageBox mBox;
		mBox.setWindowTitle("Error");
		mBox.setText("No Kalman Filters Found! or No label map exists for the current slice");
		mBox.exec();
	}
}

LabelMapType::Pointer AutoTubePanel::calculate_label_map_params(LabelMapType::Pointer labelMap)
{
	typedef itk::ShapeLabelMapFilter<LabelMapType> ShapeLabelMapFilter;

	ShapeLabelMapFilter::Pointer shape_filter = ShapeLabelMapFilter::New();
	shape_filter->SetInput(labelMap);
	shape_filter->SetComputeFeretDiameter(true);
	shape_filter->SetComputePerimeter(true);
	shape_filter->Update();
	return shape_filter->GetOutput();
}

std::map<LabelType, std::vector<double>> AutoTubePanel::get_label_map_params(LabelMapType::Pointer labelMap)
{
	std::map<LabelType, std::vector<double>> label_to_params;
	typedef float PixelType;
	using ImageType = itk::Image<PixelType, 2>;
	using ConstIteratorType = itk::ImageRegionConstIterator<ImageType>;

	std::map<LabelType, double> label_2_x;
	std::map<LabelType, double> label_2_y;
	std::map<LabelType, double> label_2_nb_pixels;

	typedef itk::LabelMapToLabelImageFilter<LabelMapType, ImageType> Label2ImageType;
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
		auto labelObject = labelMap->GetNthLabelObject(i);
		std::vector<double> params;

		params.push_back(label_2_x[labelObject->GetLabel()]);
		params.push_back(label_2_y[labelObject->GetLabel()]);
		params.push_back(labelObject->GetEquivalentSphericalPerimeter());
		params.push_back(labelObject->GetEquivalentSphericalRadius());
		params.push_back(labelObject->GetFeretDiameter());
		params.push_back(labelObject->GetFlatness());
		params.push_back(labelObject->GetNumberOfPixels());
		params.push_back(labelObject->GetNumberOfPixelsOnBorder());
		params.push_back(labelObject->GetPerimeter());
		params.push_back(labelObject->GetPerimeterOnBorder());
		params.push_back(labelObject->GetPerimeterOnBorderRatio());
		params.push_back(labelObject->GetPhysicalSize());
		params.push_back(labelObject->GetRoundness());
		label_to_params.insert(std::make_pair(labelObject->GetLabel(), params));
	}
	return label_to_params;
}

std::vector<double> AutoTubePanel::softmax(std::vector<double> distances, std::vector<double> diff_in_pred, std::vector<double> diff_in_data)
{
	std::vector<double> probabilities;

	double sum(0);

	for (unsigned int i(0); i < distances.size(); i++)
	{

		double x = exp(-(_w_distance->text().toDouble() * distances[i] + _w_pred->text().toDouble() * diff_in_pred[i] + _w_params->text().toDouble() * diff_in_data[i]));
		probabilities.push_back(x);
		sum += x;
	}
	for (auto& probability : probabilities)
	{
		if (probabilities.size() > 1)
			probability = probability / sum;

		if (isnan(probability))
			probability = 0;
	}
	return probabilities;
}

std::vector<std::string> AutoTubePanel::split_string(std::string text)
{
	// splits a string only via " " (the space character)
	std::istringstream iss(text);
	std::vector<std::string> results(std::istream_iterator<std::string>{iss},
			std::istream_iterator<std::string>());
	return results;
}

void AutoTubePanel::remove_object()
{
	// removes object from the Slice Object List

	// Get Chached data
	_cached_data.get(label_maps, objects, label_to_text, k_filters, _probabilities, max_active_slice_reached);

	QList<QListWidgetItem*> items = object_list->selectedItems();
	LabelMapType::Pointer labelMap = label_maps[_handler3D->active_slice()];

	std::vector<LabelObjectType*> objects_to_remove;
	std::vector<int> rows;
	std::vector<std::string> labels;

	for (auto item : items)
	{
		int row = object_list->row(item);
		rows.push_back(row);

		auto labelObject = labelMap->GetNthLabelObject(row);
		labels.push_back(std::to_string(labelObject->GetLabel()));
		objects_to_remove.push_back(labelObject);

		label_to_text[_handler3D->active_slice()].erase(labelObject->GetLabel());
	}

	// Remove from label objects  from label map
	for (auto object : objects_to_remove)
		labelMap->RemoveLabelObject(object);

	int offset(0);

	// sort rows in an ascending order so offset works
	std::sort(rows.begin(), rows.end());

	for (auto row : rows)
	{
		auto it = objects[_handler3D->active_slice()].begin();
		std::advance(it, row - offset);
		objects[_handler3D->active_slice()].erase(it);
		offset += 1;
	}

	if (_handler3D->active_slice() != 0)
	{
		for (auto label : labels)
		{
			_probabilities[_handler3D->active_slice()] = modify_probability(_probabilities[_handler3D->active_slice()], label, true, false);
		}
	}

	auto labelObjects = labelMap->GetLabelObjects();

	if (_handler3D->active_slice() != 0)
	{
		for (unsigned int i(0); i < labelMap->GetNumberOfLabelObjects(); i++)
		{
			_probabilities[_handler3D->active_slice()] = modify_probability(_probabilities[_handler3D->active_slice()], std::to_string(labelMap->GetNthLabelObject(i)->GetLabel()), false, true, std::to_string(i + 1));
		}
	}

	// clear labels from label map and re-initialize to have from 1 up to number of label objects
	labelMap->ClearLabels();
	for (auto& object : labelObjects)
		labelMap->PushLabelObject(object);

	SAFE_UPDATE(labelMap, return );
	label_maps[_handler3D->active_slice()] = labelMap;

	visualize_label_map(labelMap);
	refresh_object_list();
	refresh_probability_list();
	_cached_data.store(label_maps, objects, label_to_text, k_filters, _probabilities, max_active_slice_reached);
}

char AutoTubePanel::get_random_char()
{
	unsigned seed = time(0);
	srand(seed);
	return char(rand() % 26 + 'a');
}

void AutoTubePanel::visualize()
{
	if (!label_maps.empty() && label_maps[_handler3D->active_slice()])
		visualize_label_map(label_maps[_handler3D->active_slice()]);
	else
	{
		QMessageBox mBox;
		mBox.setWindowTitle("Error");
		mBox.setText("No label map to show");
		mBox.exec();
	}
}
void AutoTubePanel::merge_selected_items()
{
	QList<QListWidgetItem*> items = object_list->selectedItems();
	if (items.size() > 1)
	{
		typedef itk::Image<unsigned char, 2> mask_type;

		_cached_data.get(label_maps, objects, label_to_text, k_filters, _probabilities, max_active_slice_reached);

		auto labelMap = label_maps[_handler3D->active_slice()];

		std::vector<int> rows;

		for (auto item : items)
			rows.push_back(object_list->row(item));

		std::sort(rows.begin(), rows.end());

		std::vector<std::string> text_of_labels;

		std::string target_name = items[0]->text().toStdString();

		LabelType target_label(labelMap->GetNthLabelObject(rows[0])->GetLabel());

		std::vector<unsigned int> pixelIds;

		// need to map labels to target label in the new merged label map
		std::map<LabelType, LabelType> objects_label_mapping;
		for (unsigned int i = 0; i < labelMap->GetNumberOfLabelObjects(); i++)
		{
			// Get the ith region

			auto labelObject = labelMap->GetNthLabelObject(i);

			std::pair<LabelType, LabelType> label_pair;
			if (std::find(rows.begin(), rows.end(), labelObject->GetLabel() - 1) != rows.end())
			{
				// map label to a different label in new label map
				label_pair = std::make_pair(labelObject->GetLabel(), target_label);
				objects_label_mapping.insert(label_pair);
			}
			else
			{
				// map label to the same label
				label_pair = std::make_pair(labelObject->GetLabel(), labelObject->GetLabel());
				objects_label_mapping.insert(label_pair);
			}
		}

		auto merge_label_map = itk::ChangeLabelLabelMapFilter<LabelMapType>::New();
		merge_label_map->SetInput(labelMap);
		merge_label_map->SetChangeMap(objects_label_mapping);
		SAFE_UPDATE(merge_label_map, return );
		labelMap = merge_label_map->GetOutput();

		auto binary_image = itk::LabelMapToBinaryImageFilter<LabelMapType, mask_type>::New();
		binary_image->SetInput(labelMap);
		SAFE_UPDATE(binary_image, return );
		mask_image = binary_image->GetOutput();

		std::vector<LabelType> labels = labelMap->GetLabels();
		std::vector<std::string> objects_list;

		std::map<LabelType, std::string> l_to_t;
		for (unsigned int i(0); i < labels.size(); i++)
		{
			if (target_label != labels[i])
			{
				objects_list.push_back(label_to_text[_handler3D->active_slice()][labels[i]]);
			}
			else
			{
				objects_list.push_back(target_name);
			}
		}

		if (_handler3D->active_slice() != 0)
		{
			for (unsigned int i(1); i < rows.size(); i++)
			{
				_probabilities[_handler3D->active_slice()] = modify_probability(_probabilities[_handler3D->active_slice()], std::to_string(rows[i] + 1), true, false);
			}
		}

		auto labelObjects = labelMap->GetLabelObjects();

		if (_handler3D->active_slice() != 0)
		{
			for (unsigned int i(0); i < labelMap->GetNumberOfLabelObjects(); i++)
			{
				_probabilities[_handler3D->active_slice()] = modify_probability(_probabilities[_handler3D->active_slice()], std::to_string(labelMap->GetNthLabelObject(i)->GetLabel()), false, true, std::to_string(i + 1));
			}
		}

		// clear labels from label map and re-initialize to have from 1 up to number of label objects
		labelMap->ClearLabels();
		for (auto& object : labelObjects)
			labelMap->PushLabelObject(object);

		label_maps[_handler3D->active_slice()] = labelMap;
		objects[_handler3D->active_slice()] = objects_list;
		refresh_object_list();
		refresh_probability_list();

		_cached_data.store(label_maps, objects, label_to_text, k_filters, _probabilities, max_active_slice_reached);
	}
}

void AutoTubePanel::on_mouse_clicked(iseg::Point p)
{
	// if point click is a label object it adds it to the current slice's selected objects

	_cached_data.get(label_maps, objects, label_to_text, k_filters, _probabilities, max_active_slice_reached);
	if (label_maps[_handler3D->active_slice()])
	{
		typedef float PixelType;
		using ImageType = itk::Image<PixelType, 2>;
		if (!label_maps[_handler3D->active_slice()])
		{
			ImageType::SizeType size{{384, 384}};
			LabelMapType::Pointer map = LabelMapType::New();
			itk::Index<2> index{{0, 0}};

			ImageType::RegionType region;
			region.SetSize(size);
			region.SetIndex(index);

			map->SetRegions(region);
			map->Allocate(true);
			map->Update();
			label_maps[_handler3D->active_slice()] = map;

			std::vector<std::string> list;
			objects[_handler3D->active_slice()] = list;
		}
		auto labelMap = label_maps[_handler3D->active_slice()];

		itk::Index<2> index;
		index[0] = p.px;
		index[1] = p.py;

		LabelType label = labelMap->GetPixel(index);

		// 0 is background
		if (label != 0 && !_add_pixel->isChecked())
		{
			auto item = object_list->item(label - 1); // rows start from 0 and labels in label map from 1
			object_list->setCurrentItem(item);

			int row = object_list->row(item);

			std::vector<int>::iterator it = std::find(selected.begin(), selected.end(), row);
			if (it == selected.end())
				selected.push_back(row);
		}

		else if (_add_pixel->isChecked() && label == 0)
		{

			typedef itk::LabelMapToLabelImageFilter<LabelMapType, ImageType> Label2ImageType;
			auto label2image = Label2ImageType::New();
			label2image->SetInput(labelMap);
			label2image->Update();
			auto image = label2image->GetOutput();

			if (labelMap->GetPixel(index) == 0)
			{
				LabelType last_label = 0;
				if (!labelMap->GetLabels().empty())
					last_label = labelMap->GetLabels().back();

				if (image->GetBufferedRegion().IsInside(index))
				{

					image->SetPixel(index, last_label + 1);

					image->Update();

					typedef itk::LabelImageToShapeLabelMapFilter<ImageType, LabelMapType> Image2LabelType;
					auto image2label = Image2LabelType::New();
					image2label->SetInput(image);
					image2label->Update();

					label_maps[_handler3D->active_slice()] = image2label->GetOutput();

					objects[_handler3D->active_slice()].push_back(std::to_string(last_label + 1));
				}

				auto labelObjects = label_maps[_handler3D->active_slice()]->GetLabelObjects();
				label_maps[_handler3D->active_slice()]->ClearLabels();
				for (auto& object : labelObjects)
					label_maps[_handler3D->active_slice()]->PushLabelObject(object);
			}

			refresh_object_list();
			visualize_label_map(label_maps[_handler3D->active_slice()]);
			_cached_data.store(label_maps, objects, label_to_text, k_filters, _probabilities, max_active_slice_reached);
		}
	}
}
double AutoTubePanel::calculate_distance(std::vector<double> params_1, std::vector<double> params_2)
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

bool AutoTubePanel::label_in_list(const std::string label)
{
	bool in_list(false);
	for (unsigned int i(0); i < objects.size(); i++)
	{
		if (objects[i].size() > 0)
		{
			for (auto o_label : objects[i])
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

void AutoTubePanel::extrapolate_results()
{
	// Function of Extrapolate button

	// To make sure that it is not checked as we do not want to add to existing label maps when there aren't any
	_add->setChecked(false);

	// boolean in case no label objects are found
	bool quit(false);

	// quite in case a Kalman filter does not exist for an object
	bool quit_k_filters(false);

	int limit_slice = _limit_slice->text().toInt();
	if (limit_slice <= _handler3D->active_slice() || limit_slice >= _handler3D->num_slices())
		limit_slice = _handler3D->num_slices();

	if (k_filters.size() > 0)
	{

		for (unsigned int index(_handler3D->active_slice()); index < limit_slice - 1; index++)
		{

			_cached_data.get(label_maps, objects, label_to_text, k_filters, _probabilities, max_active_slice_reached);

			int slice_to_infer_from = index; // starting slice

			_handler3D->set_active_slice(slice_to_infer_from + 1, true);

			if (max_active_slice_reached < _handler3D->active_slice())
				max_active_slice_reached = _handler3D->active_slice();

			// preparing data for do_work_nd -> function of Execute button which will save a new label map in label_maps[_handler3D->active_slice()]
			iseg::SliceHandlerItkWrapper itk_handler(_handler3D);

			using input_type = itk::Image<float, 2>;
			using tissues_type = itk::Image<iseg::tissues_size_t, 2>;
			auto source = itk_handler.GetSourceSlice();
			auto target = itk_handler.GetTargetSlice();
			auto tissues = itk_handler.GetTissuesSlice();
			do_work_nd<input_type, tissues_type, input_type>(source, tissues, target);

			// label map from slice to infer from
			auto labelMap = label_maps[slice_to_infer_from];

			// new label map
			auto extrapolated_labelMap = label_maps[_handler3D->active_slice()];

			std::vector<itk::Point<double, 2>>* centroids = new std::vector<itk::Point<double, 2>>;

			for (unsigned int i = 0; i < labelMap->GetNumberOfLabelObjects(); i++)
			{
				// Get the ith region
				auto labelObject = labelMap->GetNthLabelObject(i);
				centroids->push_back(labelObject->GetCentroid());
			}

			typedef std::map<LabelType, std::vector<double>> LabelToParams;

			LabelToParams label_to_params;
			label_to_params = get_label_map_params(labelMap);

			LabelToParams extrapolated_label_to_params;
			extrapolated_label_to_params = get_label_map_params(extrapolated_labelMap);

			std::map<LabelType, LabelType> objects_label_mapping;

			std::map<LabelType, std::string> l_to_t; // local variable label to text mapping
			std::vector<std::string> probs;					 // probabilities

			if (centroids->empty() || extrapolated_labelMap->GetNumberOfLabelObjects() == 0)
			{
				quit = true;
				_handler3D->set_active_slice(slice_to_infer_from, true);
				break;
			}
			std::vector<std::string> objects_list;

			std::vector<std::string> labels;
			for (auto filter : k_filters)
			{
				labels.push_back(filter.get_label());
			}

			std::vector<std::string>::iterator it; // iterator used to find the index of the given kalman filter from the label map

			double threshold_probability = _min_probability->text().toDouble();

			for (unsigned int i = 0; i < extrapolated_labelMap->GetNumberOfLabelObjects(); i++)
			{
				std::vector<double> distances;
				std::vector<double> diff_from_predictions;
				std::vector<double> diff_in_data;
				for (unsigned int j = 0; j < labelMap->GetNumberOfLabelObjects(); j++)
				{

					std::vector<double> par = label_to_params[labelMap->GetNthLabelObject(j)->GetLabel()];

					it = std::find(labels.begin(), labels.end(), label_to_text[slice_to_infer_from][labelMap->GetNthLabelObject(j)->GetLabel()]);
					if (it != labels.end())
					{
						int index = std::distance(labels.begin(), it);

						// calculates the difference in position prediction
						double diff_from_pred = k_filters[index].diff_btw_predicated_object(extrapolated_label_to_params[extrapolated_labelMap->GetNthLabelObject(i)->GetLabel()]);
						diff_from_predictions.push_back(diff_from_pred);

						// calculate the distance
						double distance = calculate_distance(label_to_params[labelMap->GetNthLabelObject(j)->GetLabel()], extrapolated_label_to_params[extrapolated_labelMap->GetNthLabelObject(i)->GetLabel()]);
						distances.push_back(distance);

						unsigned int Size(label_to_params[labelMap->GetNthLabelObject(j)->GetLabel()].size());
						// calculate the difference between the two given object's parameters
						double sum(0);
						for (unsigned int k(2); k < Size; k++)
						{
							sum += abs(label_to_params[labelMap->GetNthLabelObject(j)->GetLabel()][k] - extrapolated_label_to_params[extrapolated_labelMap->GetNthLabelObject(i)->GetLabel()][k]);
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
				probabilities = softmax(distances, diff_from_predictions, diff_in_data);

				// index of highest probability
				int max_index = std::distance(probabilities.begin(), std::max_element(probabilities.begin(), probabilities.end()));

				auto new_labelObject = extrapolated_labelMap->GetNthLabelObject(i);
				bool set(false); // was the name of the new label object already set?

				if (probabilities[max_index] > threshold_probability)
				{
					auto old_labelObject = labelMap->GetNthLabelObject(max_index);

					// if the new label object corresponds to an old label object that is a digit
					// we add a random charecter to it so the user can see to which object it is linked too

					if (isdigit(label_to_text[slice_to_infer_from][old_labelObject->GetLabel()][0]))
					{
						char letter_to_add;
						if (isalpha(label_to_text[slice_to_infer_from][old_labelObject->GetLabel()].back()))
							letter_to_add = label_to_text[slice_to_infer_from][old_labelObject->GetLabel()].back();
						else
							letter_to_add = get_random_char();
						while (label_in_list(label_to_text[slice_to_infer_from][old_labelObject->GetLabel()] + letter_to_add))
						{
							letter_to_add = get_random_char();
						}
						l_to_t[new_labelObject->GetLabel()] = label_to_text[slice_to_infer_from][old_labelObject->GetLabel()] + letter_to_add;
					}
					else
					{
						l_to_t[new_labelObject->GetLabel()] = label_to_text[slice_to_infer_from][old_labelObject->GetLabel()];
					}

					set = true;
				}

				for (unsigned int j = 0; j < labelMap->GetNumberOfLabelObjects(); j++)
				{
					auto old_labelObject = labelMap->GetNthLabelObject(j);

					// if Extrapolate Only Matches is not checked and the new object was not set then add it to the mapping
					if (!_extrapolate_only_matches->isChecked() && !set)
						l_to_t[new_labelObject->GetLabel()] = std::to_string(new_labelObject->GetLabel());

					std::string str = "Probability of " + std::to_string(new_labelObject->GetLabel()) + " -> " + label_to_text[slice_to_infer_from][old_labelObject->GetLabel()] + " = " + std::to_string(probabilities[j]);
					probs.push_back(str);
				}
			}

			if (_extrapolate_only_matches->isChecked())
			{
				bool in;

				for (auto label : extrapolated_labelMap->GetLabels())
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
						probs = modify_probability(probs, std::to_string(label), true, false);
						extrapolated_labelMap->RemoveLabel(label);
					}
				}

				// modify the probability to write the correct index in the object list
				for (unsigned int i(0); i < extrapolated_labelMap->GetNumberOfLabelObjects(); i++)
				{
					probs = modify_probability(probs, std::to_string(extrapolated_labelMap->GetNthLabelObject(i)->GetLabel()), false, true, std::to_string(i + 1));
				}

				// Change label maps labels from 1 up to number of labele objects
				auto labelObjects = extrapolated_labelMap->GetLabelObjects();
				extrapolated_labelMap->ClearLabels();
				for (auto& object : labelObjects)
					extrapolated_labelMap->PushLabelObject(object);
			}

			if (quit_k_filters)
				break;

			labels.clear();
			for (auto filter : k_filters)
			{
				labels.push_back(filter.get_label());
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

			label_maps[_handler3D->active_slice()] = extrapolated_labelMap;
			objects[_handler3D->active_slice()] = objects_list;
			_probabilities[_handler3D->active_slice()] = probs;

			refresh_object_list();
			refresh_probability_list();
			refresh_k_filter_list();
			visualize_label_map(extrapolated_labelMap);

			_cached_data.store(label_maps, objects, label_to_text, k_filters, _probabilities, max_active_slice_reached);
		}

		if (quit)
		{
			QMessageBox mBox;
			mBox.setWindowTitle("Error");
			mBox.setText("Stopped at slice:" + QString::number(_handler3D->active_slice() + 1) + " due to object found not meeting criteria. Change filtering settings and try again.");
			mBox.exec();
		}

		if (quit_k_filters)
		{
			QMessageBox mBox;
			mBox.setWindowTitle("Error");
			mBox.setText("Stopped due to there being objects having no Kalman Filters! Make sure all objects have Kalman Filters!");
			mBox.exec();
		}
	}
	else
	{
		QMessageBox mBox;
		mBox.setWindowTitle("Error");
		mBox.setText("Cannot Extrapolate When There Are No kalman Filters! Extrapolate only when all objects have Kalman Filters!");
		mBox.exec();
	}
}

bool AutoTubePanel::isInteger(const std::string& s)
{
	if (s.empty() || ((!isdigit(s[0])) && (s[0] != '-') && (s[0] != '+')))
		return false;

	char* p;
	strtol(s.c_str(), &p, 10);

	return (*p == 0);
}

void AutoTubePanel::item_selected()
{
	QList<QListWidgetItem*> items = object_list->selectedItems();
	object_selected(items);
}

void AutoTubePanel::item_double_clicked(QListWidgetItem* item)
{
	item->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
	object_list->editItem(item);
	QObject::connect(object_list, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(item_edited(QListWidgetItem*)));
}

bool AutoTubePanel::is_label(const std::string& s)
{
	try
	{
		boost::lexical_cast<int>(s);
		return true;
	}
	catch (boost::bad_lexical_cast&)
	{
	}
	return false;
}

void AutoTubePanel::item_edited(QListWidgetItem* Item)
{
	_cached_data.get(label_maps, objects, label_to_text, k_filters, _probabilities, max_active_slice_reached);

	std::string new_name = Item->text().toStdString();
	boost::algorithm::trim(new_name);

	std::string old_name = objects[_handler3D->active_slice()][object_list->currentRow()];

	if (!new_name.empty() && !is_label(new_name))
	{

		bool in_k_filters(true);

		std::vector<std::string> labels;
		for (auto filter : k_filters)
			labels.push_back(filter.get_label());

		if (std::find(labels.begin(), labels.end(), new_name) == labels.end())
			in_k_filters = false;

		// if Kalman Filter doesnt exist for new name create one
		if (!in_k_filters)
		{
			KalmanFilter k;
			k.set_label(new_name);
			k.set_slice(_handler3D->active_slice() + 1);
			k_filters.push_back(k);
		}

		// update objects
		objects[_handler3D->active_slice()][object_list->currentRow()] = new_name;

		// update mapping
		std::map<LabelType, std::string> l_to_t;
		for (unsigned int i(0); i < objects[_handler3D->active_slice()].size(); i++)
		{
			l_to_t[label_maps[_handler3D->active_slice()]->GetNthLabelObject(i)->GetLabel()] = objects[_handler3D->active_slice()][i];
		}

		label_to_text[_handler3D->active_slice()] = l_to_t;
		refresh_k_filter_list();

		_cached_data.store(label_maps, objects, label_to_text, k_filters, _probabilities, max_active_slice_reached);
	}
}

void AutoTubePanel::object_selected(QList<QListWidgetItem*> items)
{
	if (!items.empty())
	{
		auto labelMap = label_maps[_handler3D->active_slice()];

		std::vector<int> labels_indexes;
		for (unsigned int i(0); i < items.size(); i++)
			labels_indexes.push_back(object_list->row(items[i]));

		// select pixels to highlight
		std::vector<itk::Index<2>>* item_pixels = new std::vector<itk::Index<2>>;
		for (auto label : labels_indexes)
		{
			auto labelObject = labelMap->GetNthLabelObject(label);
			for (unsigned int pixelId = 0; pixelId < labelObject->Size(); pixelId++)
			{
				item_pixels->push_back(labelObject->GetIndex(pixelId));
			}
		}

		visualize_label_map(label_maps[_handler3D->active_slice()], item_pixels);
	}
}

std::vector<std::string> AutoTubePanel::modify_probability(std::vector<std::string> probs, std::string label, bool remove, bool change, std::string value)
{
	// each label object has multiple values in the probability list as it has a probability for all the objects in the previous slice
	std::vector<unsigned int> indices;
	for (unsigned int ind(0); ind < probs.size(); ind++)
	{
		std::string _prob = probs[ind];
		std::vector<std::string> str = split_string(_prob);
		std::string label_text = str[2]; // position of object labels name
		if (label == label_text)
		{
			if (change) // change name
			{
				str[2] = value;
				_prob = "";
				for (auto word : str)
					_prob += word + " ";
				probs[ind] = _prob;
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

void AutoTubePanel::refresh_k_filter_list()
{

	k_filters_list->clear();

	for (auto filter : k_filters)
	{
		k_filters_list->addItem(QString::fromStdString(filter.get_label()));
	}
	k_filters_list->show();
}

void AutoTubePanel::refresh_object_list()
{
	object_list->clear();
	std::map<LabelType, std::string> l_to_t;
	if (!objects.empty() && objects[_handler3D->active_slice()].size() > 0)
	{
		for (unsigned int i(0); i < objects[_handler3D->active_slice()].size(); i++)
		{
			object_list->addItem(QString::fromStdString(objects[_handler3D->active_slice()][i]));
			l_to_t[i + 1] = objects[_handler3D->active_slice()][i];
		}
	}
	object_list->show();
	label_to_text[_handler3D->active_slice()] = l_to_t; // update mapping to reflect the refreshed objects
}
void AutoTubePanel::refresh_probability_list()
{
	object_probability_list->clear();
	if (_handler3D->active_slice() != 0)
	{
		if (!_probabilities.empty() && _probabilities[_handler3D->active_slice()].size() > 0)
		{
			for (unsigned int i(0); i < _probabilities[_handler3D->active_slice()].size(); i++)
			{
				object_probability_list->addItem(QString::fromStdString(_probabilities[_handler3D->active_slice()][i]));
			}
		}
		object_probability_list->show();
	}
}

void AutoTubePanel::select_objects()
{
	auto sel = _handler3D->tissue_selection();

	std::string text = join(sel | transformed([](int d) { return std::to_string(d); }), ", ");
	_selected_objects->setText(QString::fromStdString(text));
}

void AutoTubePanel::init()
{
	on_slicenr_changed();
	hideparams_changed();
}

void AutoTubePanel::newloaded()
{
	on_slicenr_changed();
}

void AutoTubePanel::on_slicenr_changed()
{
	_cached_data.get(label_maps, objects, label_to_text, k_filters, _probabilities, max_active_slice_reached);
	// initialize object list for all slices
	if (objects.empty() || objects.size() != _handler3D->num_slices())

		objects = std::vector<std::vector<std::string>>(_handler3D->num_slices());

	// initiating label maps
	if (label_maps.empty() || label_maps.size() != _handler3D->num_slices())
		label_maps = std::vector<LabelMapType::Pointer>(_handler3D->num_slices());

	if (label_to_text.empty() || label_to_text.size() != _handler3D->num_slices())
		label_to_text = std::vector<std::map<LabelType, std::string>>(_handler3D->num_slices());

	if (_probabilities.empty() || _probabilities.size() != _handler3D->num_slices())
	{
		_probabilities = std::vector<std::vector<std::string>>(_handler3D->num_slices());
		std::vector<std::string> init;
		init.push_back(" ");
		_probabilities[0] = init;
	}

	refresh_object_list();
	refresh_probability_list();
	refresh_k_filter_list();
	selected.clear();

	if (!label_maps.empty() && label_maps[_handler3D->active_slice()])
		visualize_label_map(label_maps[_handler3D->active_slice()]);
}

void AutoTubePanel::cleanup()
{
}

void AutoTubePanel::do_work()
{
	// function for Execute Button

	iseg::SliceHandlerItkWrapper itk_handler(_handler3D);
	using input_type = itk::Image<float, 2>;
	using tissues_type = itk::Image<iseg::tissues_size_t, 2>;
	auto source = itk_handler.GetSourceSlice();
	auto target = itk_handler.GetTargetSlice();
	auto tissues = itk_handler.GetTissuesSlice();
	do_work_nd<input_type, tissues_type, input_type>(source, tissues, target);
}

template<class TInput, class TImage>
typename TImage::Pointer AutoTubePanel::compute_feature_image(TInput* source) const
{
	itkStaticConstMacro(ImageDimension, size_t, TInput::ImageDimension);
	using hessian_pixel_type = itk::SymmetricSecondRankTensor<float, ImageDimension>;
	using hessian_image_type = itk::Image<hessian_pixel_type, ImageDimension>;
	using feature_filter_type = itk::HessianToObjectnessMeasureImageFilter<hessian_image_type, TImage>;
	using multiscale_hessian_filter_type = itk::MultiScaleHessianBasedMeasureImageFilter<TInput, hessian_image_type, TImage>;

	double sigm_min = _sigma_low->text().toDouble();
	double sigm_max = _sigma_hi->text().toDouble();
	int num_levels = _number_sigma_levels->text().toInt();

	auto objectnessFilter = feature_filter_type::New();
	objectnessFilter->SetBrightObject(false);
	objectnessFilter->SetObjectDimension(1);
	objectnessFilter->SetScaleObjectnessMeasure(true);
	objectnessFilter->SetAlpha(0.5);
	objectnessFilter->SetBeta(0.5);
	objectnessFilter->SetGamma(5.0);

	auto multiScaleEnhancementFilter = multiscale_hessian_filter_type::New();
	multiScaleEnhancementFilter->SetInput(source);
	multiScaleEnhancementFilter->SetHessianToMeasureFilter(objectnessFilter);
	multiScaleEnhancementFilter->SetSigmaStepMethodToEquispaced();
	multiScaleEnhancementFilter->SetSigmaMinimum(std::min(sigm_min, sigm_max));
	multiScaleEnhancementFilter->SetSigmaMaximum(std::max(sigm_min, sigm_max));
	multiScaleEnhancementFilter->SetNumberOfSigmaSteps(std::min(1, num_levels));
	multiScaleEnhancementFilter->Update();
	return multiScaleEnhancementFilter->GetOutput();
}

void AutoTubePanel::visualize_label_map(LabelMapType::Pointer labelMap, std::vector<itk::Index<2>>* pixels)
{
	typedef float PixelType;
	using ImageType = itk::Image<PixelType, 2>;

	iseg::SliceHandlerItkWrapper itk_handler(_handler3D);
	auto target = itk_handler.GetTargetSlice();

	typedef itk::LabelMapToLabelImageFilter<LabelMapType, ImageType> Label2ImageType;
	auto label2image = Label2ImageType::New();
	label2image->SetInput(labelMap);
	SAFE_UPDATE(label2image, return );

	using tissue_type = SliceHandlerInterface::tissue_type;
	iseg::DataSelection dataSelection;
	dataSelection.allSlices = false; // all_slices->isChecked();
	dataSelection.sliceNr = _handler3D->active_slice();
	dataSelection.work = true;
	dataSelection.tissues = false;
	dataSelection.bmp = false;

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

	emit begin_datachange(dataSelection, this);

	emit end_datachange(this);
}
template<class TInput, class TTissue, class TTarget>
void AutoTubePanel::do_work_nd(TInput* source, TTissue* tissues, TTarget* target)
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
	feature_params.push_back(_sigma_low->text().toDouble());
	feature_params.push_back(_sigma_hi->text().toDouble());
	feature_params.push_back(_number_sigma_levels->text().toInt());

	// extract IDs if any were set
	std::vector<int> object_ids;
	if (!_selected_objects->text().isEmpty())
	{
		std::vector<std::string> tokens;
		std::string selected_objects_text = _selected_objects->text().toStdString();
		boost::algorithm::split(tokens, selected_objects_text, boost::algorithm::is_any_of(","));
		std::transform(tokens.begin(), tokens.end(), std::back_inserter(object_ids),
				[](std::string s) {
					boost::algorithm::trim(s);
					return stoi(s);
				});
	}

	typename real_type::Pointer feature_image;
	feature_image = compute_feature_image<input_type, real_type>(source);
	// mask feature image before skeletonization
	if (!object_ids.empty())
	{
		using map_functor = iseg::Functor::MapLabels<unsigned short, unsigned char>;
		map_functor map;
		map.m_Map.assign(_handler3D->tissue_names().size() + 1, 0);
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

	if (_non_max_suppression->isChecked())
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

	if (_skeletonize->isChecked())
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
	typedef itk::BinaryImageToLabelMapFilter<mask_type, LabelMapType> BinaryImageToLabelMapFilterType;

	typename mask_type::Pointer output;
	if (skeleton)
	{
		auto connectivity = itk::ConnectedComponentImageFilter<mask_type, labelfield_type>::New();
		connectivity->SetInput(skeleton);
		connectivity->FullyConnectedOn();

		auto relabel = itk::RelabelComponentImageFilter<labelfield_type, labelfield_type>::New();
		relabel->SetInput(connectivity->GetOutput());
		relabel->SetMinimumObjectSize(_min_object_size->text().toInt());
		SAFE_UPDATE(relabel, return );

		auto threshold = itk::BinaryThresholdImageFilter<labelfield_type, mask_type>::New();
		threshold->SetInput(relabel->GetOutput());
		threshold->SetLowerThreshold(_threshold->text().toInt());
		SAFE_UPDATE(threshold, return );

		output = threshold->GetOutput();

		//converting output to label map
		auto binaryImageToLabelMapFilter = BinaryImageToLabelMapFilterType::New();
		binaryImageToLabelMapFilter->SetInput(output);
		if (_connect_dots->isChecked())
			binaryImageToLabelMapFilter->SetFullyConnected(true);
		else
			binaryImageToLabelMapFilter->SetFullyConnected(false);
		SAFE_UPDATE(binaryImageToLabelMapFilter, return );

		iseg::SliceHandlerItkWrapper itk_handler(_handler3D);
		std::vector<std::string> label_object_list;

		auto labelMap = binaryImageToLabelMapFilter->GetOutput();
		auto map = calculate_label_map_params(labelMap);
		typedef std::map<LabelType, std::vector<double>> LabelToParams;

		LabelToParams label_to_params = get_label_map_params(map);

		std::vector<std::string> labels;
		for (auto& filter : k_filters)
			labels.push_back(filter.get_label());

		if (!_add->isChecked())
		{
			for (unsigned int i = 0; i < map->GetNumberOfLabelObjects(); i++)
			{

				// Get the ith region
				auto labelObject = map->GetNthLabelObject(i);
				label_object_list.push_back(std::to_string(labelObject->GetLabel()));
			}
			objects[_handler3D->active_slice()] = label_object_list;
			label_maps[_handler3D->active_slice()] = map;
		}
		else
		{
			auto old_map = label_maps[_handler3D->active_slice()];
			LabelType last_label = old_map->GetLabels().back();
			for (unsigned int i(0); i < map->GetNumberOfLabelObjects(); i++)
			{
				map->GetNthLabelObject(i)->SetLabel(map->GetNthLabelObject(i)->GetLabel() + last_label);
				SAFE_UPDATE(map, return );
				objects[_handler3D->active_slice()].push_back(std::to_string(map->GetNthLabelObject(i)->GetLabel()));
			}
			using MergeLabelFilterType = itk::MergeLabelMapFilter<LabelMapType>;
			auto merger = MergeLabelFilterType::New();
			merger->SetInput(0, old_map);
			merger->SetInput(1, map);
			merger->SetMethod(MergeLabelFilterType::KEEP);
			SAFE_UPDATE(merger, return );

			auto calculated_map = calculate_label_map_params(merger->GetOutput());

			label_maps[_handler3D->active_slice()] = calculated_map;
		}
	}

	if (max_active_slice_reached < _handler3D->active_slice())
		max_active_slice_reached = _handler3D->active_slice();

	iseg::DataSelection dataSelection;
	dataSelection.allSlices = false; // all_slices->isChecked();
	dataSelection.sliceNr = _handler3D->active_slice();
	dataSelection.work = true;

	if (output && iseg::Paste<mask_type, input_type>(output, target))
	{

		// good, else maybe output is not defined
	}

	else if (!iseg::Paste<mask_type, input_type>(skeleton, target))
	{
		std::cerr << "Error: could not set output because image regions don't match.\n";
	}

	emit begin_datachange(dataSelection, this);
	emit end_datachange(this);
	mask_image = output;

	refresh_object_list();
	refresh_k_filter_list();
	visualize_label_map(label_maps[_handler3D->active_slice()]);
	_cached_data.store(label_maps, objects, label_to_text, k_filters, _probabilities, max_active_slice_reached);
}
