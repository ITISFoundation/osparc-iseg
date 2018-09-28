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

#include <qcheckbox.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qspinbox.h>

#include <itkImage.h>

class AutoTubeWidget : public iseg::WidgetInterface
{
	Q_OBJECT
public:
	AutoTubeWidget(iseg::SliceHandlerInterface* hand3D, QWidget* parent = 0,
			const char* name = 0, Qt::WindowFlags wFlags = 0);
	~AutoTubeWidget() {}
	void init() override;
	void newloaded() override;
	void cleanup() override;
	std::string GetName() override { return std::string("Auto-Tubes"); };
	QIcon GetIcon(QDir picdir) override { return QIcon(picdir.absFilePath(QString("LevelSet.png"))); };

private:
	void on_slicenr_changed() override;
	void on_mouse_clicked(iseg::Point p) override;

	template<class TInput, class TTissue, class TTarget>
	void do_work_nd(TInput* source, TTissue* tissues, TTarget* target);

	template<class TInput, class TImage>
	typename TImage::Pointer compute_feature_image(TInput* source) const;

	template<class TInput, class TImage>
	typename TImage::Pointer compute_feature_image_2d(TInput* source) const;

	iseg::SliceHandlerInterface* _handler3D;

	QLineEdit* _sigma_low;
	QLineEdit* _sigma_hi;
	QLineEdit* _number_sigma_levels;
	QLineEdit* _threshold;
	QCheckBox* _metric2d;
	QCheckBox* _non_max_suppression;
	QCheckBox* _skeletonize;
	QLineEdit* _max_radius;
	QLineEdit* _min_object_size;
	QPushButton* _select_objects_button;
	QLineEdit* _selected_objects;
	QPushButton* _execute_button;

	template<typename T>
	struct Cache
	{
		using params_type = std::vector<double>;

		void store(typename itk::Image<T, 2>::Pointer&, const params_type& p = params_type()) {}
		void store(typename itk::Image<T, 3>::Pointer& i, const params_type& p = params_type())
		{
			params = p;
			img = i;
		}

		bool get(typename itk::Image<T, 2>::Pointer&, const params_type& p = params_type()) const { return false; }
		bool get(typename itk::Image<T, 3>::Pointer& r, const params_type& p = params_type()) const
		{
			if (p == params)
			{
				r = img;
			}
			return r != nullptr;
		}

		params_type params;
		typename itk::Image<T, 3>::Pointer img;
	};

	Cache<float> _cached_feature_image;
	Cache<unsigned char> _cached_skeleton;

private slots:
	void select_objects();
	void do_work();
};
