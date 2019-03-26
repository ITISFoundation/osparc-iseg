//
//  AutoTubePanel.hpp
//  iSegData
//
//  Created by Benjamin Fuhrer on 13.10.18.
//
#pragma once

#include "KalmanFilter.h"

#include "Data/SlicesHandlerInterface.h"

#include "Interface/WidgetInterface.h"

#include "Core/Pair.h"

#include <QCheckBox>
#include <QListWidget>
#include <QPushButton>

#include <itkImage.h>
#include <itkShapeLabelObject.h>

#include <iostream>
#include <vector>

typedef unsigned long LabelType;
typedef itk::ShapeLabelObject<LabelType, 2> LabelObjectType;
typedef itk::LabelMap<LabelObjectType> LabelMapType;

class AutoTubePanel : public iseg::WidgetInterface
{

	struct Cache
	{

		void store(std::vector<LabelMapType::Pointer> l, std::vector<std::vector<std::string>> o, std::vector<std::map<LabelType, std::string>> t, std::vector<KalmanFilter> k, std::vector<std::vector<std::string>> p, int m)
		{
			objects = o;
			label_maps = l;
			label_to_text = t;
			k_filters = k;
			_probabilities = p;
			max_active_slice = m;
		}

		void get(std::vector<LabelMapType::Pointer>& l, std::vector<std::vector<std::string>>& o, std::vector<std::map<LabelType, std::string>>& t, std::vector<KalmanFilter>& k, std::vector<std::vector<std::string>>& p, int& m)
		{
			if (!label_maps.empty())
			{
				l = label_maps;
				o = objects;
				t = label_to_text;
				k = k_filters;
				p = _probabilities;
				m = max_active_slice;
			}
		}

		std::vector<std::vector<std::string>> objects;
		std::vector<LabelMapType::Pointer> label_maps;
		std::vector<std::map<LabelType, std::string>> label_to_text;
		std::vector<KalmanFilter> k_filters;
		std::vector<std::vector<std::string>> _probabilities;
		int max_active_slice;
	};

	Q_OBJECT
public:
	AutoTubePanel(iseg::SliceHandlerInterface* hand3D, QWidget* parent = 0,
			const char* name = 0, Qt::WindowFlags wFlags = 0);
	~AutoTubePanel() {}

	void init() override;
	void newloaded() override;
	void cleanup() override;
	std::string GetName() override { return std::string("Root Tracer"); };
	QIcon GetIcon(QDir picdir) override { return QIcon(picdir.absFilePath(QString("LevelSet.png"))); };

	void save_label_map(FILE* fp, LabelMapType::Pointer map);
	LabelMapType::Pointer load_label_map(FILE* fi);

	void save_l_to_t(FILE* fp, std::map<LabelType, std::string> l_to_t);
	std::map<LabelType, std::string> load_l_to_t(FILE* fi);

	void save_k_filter(FILE* fp, std::vector<KalmanFilter> k_filters);
	std::vector<KalmanFilter> load_k_filters(FILE* fi);

	std::vector<std::string> modify_probability(std::vector<std::string> probs, std::string label, bool remove, bool change, std::string value = "");

	template<class TInput, class TTissue, class TTarget>
	void do_work_nd(TInput* source, TTissue* tissues, TTarget* target);

	template<class TInput, class TImage>
	typename TImage::Pointer compute_feature_image(TInput* source) const;

	void refresh_object_list();
	void refresh_probability_list();
	void refresh_k_filter_list();

	char get_random_char();

	std::vector<std::string> split_string(std::string text);

	bool is_label(const std::string& s);
	bool label_in_list(const std::string label);

	std::map<LabelType, std::vector<double>> get_label_map_params(LabelMapType::Pointer labelMap);
	LabelMapType::Pointer calculate_label_map_params(LabelMapType::Pointer labelMap);

	double calculate_distance(std::vector<double> params_1, std::vector<double> params_2);
	std::vector<double> softmax(std::vector<double> distances, std::vector<double> diff_in_pred, std::vector<double> diff_in_data);

	void visualize_label_map(LabelMapType::Pointer labelMap, std::vector<itk::Index<2>>* pixels = nullptr);

	bool isInteger(const std::string& s);

private:
	void on_slicenr_changed() override;
	void on_mouse_clicked(iseg::Point p) override;

	iseg::SliceHandlerInterface* _handler3D;

	QListWidget* object_list;
	QListWidget* object_probability_list;
	QListWidget* k_filters_list;

	QPushButton* _execute_button;
	QPushButton* _select_objects_button;
	QPushButton* _remove_button;
	QPushButton* _merge_button;
	QPushButton* _extrapolate_button;
	QPushButton* _visualize_button;
	QPushButton* _update_kfilter_button;
	QPushButton* _remove_k_filter;
	QPushButton* _save;
	QPushButton* _load;
	QPushButton* _remove_non_selected;
	QPushButton* _add_to_tissues;
	QPushButton* _export_lines;
	QPushButton* _k_filter_predict;

	QLineEdit* _selected_objects;
	QLineEdit* _sigma_low;
	QLineEdit* _sigma_hi;
	QLineEdit* _number_sigma_levels;
	QLineEdit* _threshold;
	QLineEdit* _max_radius;
	QLineEdit* _min_object_size;
	QLineEdit* _min_probability;
	QLineEdit* _w_distance;
	QLineEdit* _w_params;
	QLineEdit* _w_pred;
	QLineEdit* _limit_slice;
	QLineEdit* _line_radius;

	QCheckBox* _non_max_suppression;
	QCheckBox* _skeletonize;
	QCheckBox* _add;
	QCheckBox* _restart_k_filter;
	QCheckBox* _connect_dots;
	QCheckBox* _extrapolate_only_matches;
	QCheckBox* _add_pixel;

	int max_active_slice_reached = 0;
	int active_slice = 0;

	std::vector<std::vector<std::string>> objects;
	std::vector<std::vector<std::string>> _probabilities;

	std::vector<LabelMapType::Pointer> label_maps;

	itk::Image<unsigned char, 2>::Pointer mask_image;

	std::vector<std::map<LabelType, std::string>> label_to_text;
	std::vector<KalmanFilter> k_filters;
	std::vector<int> selected;

	Cache _cached_data;

private slots:

	void select_objects();
	void do_work();
	void bmp_changed(){};
	void tissues_changed(){};
	void work_changed(){};
	void marks_changed(){};
	void object_selected(QList<QListWidgetItem*> items);
	void item_double_clicked(QListWidgetItem* item);
	void item_edited(QListWidgetItem* item);
	void item_selected();
	void merge_selected_items();
	void extrapolate_results();
	void visualize();
	void remove_object();
	void remove_k_filter();
	void update_kalman_filters();
	void k_filter_double_clicked(QListWidgetItem* item);
	void save();
	void load();
	void remove_non_selected();
	void add_to_tissues();
	void export_lines();
	void predict_k_filter();
};
