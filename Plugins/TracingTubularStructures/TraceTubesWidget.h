/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#pragma once

#include "Data/SlicesHandlerInterface.h"

#include "Interface/WidgetInterface.h"

#include <itkImage.h>

#include <vector>

class QCheckBox;
class QLineEdit;
class QComboBox;
class QPushButton;
class QStackedWidget;

namespace itk {
class ProcessObject;
}

namespace iseg {
struct Point3D
{
	union {
		struct
		{
			short px, py;
		};
		Point p;
	};
	short pz;
};
} // namespace iseg

class TraceTubesWidget : public iseg::WidgetInterface
{
	Q_OBJECT
public:
	TraceTubesWidget(iseg::SliceHandlerInterface* hand3D, QWidget* parent = 0,
			const char* name = 0, Qt::WindowFlags wFlags = 0);
	~TraceTubesWidget() {}

	void init() override;
	void newloaded() override;
	void cleanup() override;
	std::string GetName() override;
	QIcon GetIcon(QDir picdir) override;

private:
	void on_slicenr_changed() override;
	void on_mouse_released(iseg::Point p) override;

	void update_points();

	int get_padding() const;

	enum eMetric {
		kIntensity = 0,
		kHessian2D,
		kHessian3D,
		kTarget
	};
	itk::Image<float, 3>::Pointer compute_vesselness(const itk::ImageBase<3>::RegionType& requested_region) const;

	itk::Image<float, 3>::Pointer compute_blobiness(const itk::ImageBase<3>::RegionType& requested_region) const;

	itk::Image<float, 3>::Pointer compute_object_sdf(const itk::ImageBase<3>::RegionType& requested_region) const;

	template<class TSpeedImage>
	void do_work_template(TSpeedImage* speed_image, const itk::ImageBase<3>::RegionType& requested_region);

	iseg::SliceHandlerInterface* _handler;
	std::vector<iseg::Point3D> _points;

	QWidget* _main_options;
	QComboBox* _metric;
	QLineEdit* _intensity_value;
	QLineEdit* _intensity_weight;
	QLineEdit* _angle_weight;
	QLineEdit* _line_radius;
	QLineEdit* _active_region_padding;
	QLineEdit* _debug_metric_file_path;
	QPushButton* _clear_points;
	QPushButton* _estimate_intensity;
	QPushButton* _execute_button;

	QWidget* _empty_options;

	QWidget* _vesselness_options;
	QLineEdit* _sigma;
	QCheckBox* _dark_objects;
	QLineEdit* _alpha;
	QLineEdit* _beta;
	QLineEdit* _gamma;

	QWidget* _target_options;
	QLineEdit* _path_file_name;

	QStackedWidget* _options_stack;

private slots:
	void do_work();
	void clear_points();
	void estimate_intensity();
	void on_metric_changed();
};
