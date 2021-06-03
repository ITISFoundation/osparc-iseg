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

namespace iseg {
using label_type = unsigned long;
using label_object_type = itk::ShapeLabelObject<label_type, 2>;
using label_map_type = itk::LabelMap<label_object_type>;

class AutoTubePanel : public iseg::WidgetInterface
{
	struct Cache
	{
		void Store(std::vector<label_map_type::Pointer> l, std::vector<std::vector<std::string>> o, std::vector<std::map<label_type, std::string>> t, std::vector<KalmanFilter> k, std::vector<std::vector<std::string>> p, int m)
		{
			objects = o;
			label_maps = l;
			label_to_text = t;
			k_filters = k;
			_probabilities = p;
			max_active_slice = m;
		}

		void Get(std::vector<label_map_type::Pointer>& l, std::vector<std::vector<std::string>>& o, std::vector<std::map<label_type, std::string>>& t, std::vector<KalmanFilter>& k, std::vector<std::vector<std::string>>& p, int& m) const
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
		std::vector<label_map_type::Pointer> label_maps;
		std::vector<std::map<label_type, std::string>> label_to_text;
		std::vector<KalmanFilter> k_filters;
		std::vector<std::vector<std::string>> _probabilities;
		int max_active_slice;
	};

	Q_OBJECT
public:
	AutoTubePanel(iseg::SlicesHandlerInterface* hand3D);
	~AutoTubePanel() override = default;

	void Init() override;
	void NewLoaded() override;
	void Cleanup() override;
	std::string GetName() override { return std::string("Root Tracer"); };
	QIcon GetIcon(QDir picdir) override { return QIcon(picdir.absFilePath(QString("LevelSet.png"))); };

	void SaveLabelMap(FILE* fp, label_map_type::Pointer map);
	label_map_type::Pointer LoadLabelMap(FILE* fi);

	void SaveLToT(FILE* fp, std::map<label_type, std::string> l_to_t);
	std::map<label_type, std::string> LoadLToT(FILE* fi);

	void SaveKFilter(FILE* fp, std::vector<KalmanFilter> k_filters);
	std::vector<KalmanFilter> LoadKFilters(FILE* fi);

	std::vector<std::string> ModifyProbability(std::vector<std::string> probs, std::string label, bool remove, bool change, std::string value = "");

	template<class TInput, class TTissue, class TTarget>
	void DoWorkNd(TInput* source, TTissue* tissues, TTarget* target);

	template<class TInput, class TImage>
	typename TImage::Pointer ComputeFeatureImage(TInput* source) const;

	void RefreshObjectList();
	void RefreshProbabilityList();
	void RefreshKFilterList();

	char GetRandomChar();

	std::vector<std::string> SplitString(std::string text);

	bool IsLabel(const std::string& s);
	bool LabelInList(const std::string label);

	std::map<label_type, std::vector<double>> GetLabelMapParams(label_map_type::Pointer labelMap);
	label_map_type::Pointer CalculateLabelMapParams(label_map_type::Pointer labelMap);

	double CalculateDistance(std::vector<double> params_1, std::vector<double> params_2);
	std::vector<double> Softmax(std::vector<double> distances, std::vector<double> diff_in_pred, std::vector<double> diff_in_data);

	void VisualizeLabelMap(label_map_type::Pointer labelMap, std::vector<itk::Index<2>>* pixels = nullptr);

	bool IsInteger(const std::string& s);

private:
	void OnSlicenrChanged() override;
	void OnMouseClicked(iseg::Point p) override;

	void ObjectSelected(QList<QListWidgetItem*> items);

	iseg::SlicesHandlerInterface* m_Handler3D;

	QListWidget* m_ObjectList;
	QListWidget* m_ObjectProbabilityList;
	QListWidget* m_KFiltersList;

	QPushButton* m_ExecuteButton;
	QPushButton* m_SelectObjectsButton;
	QPushButton* m_RemoveButton;
	QPushButton* m_MergeButton;
	QPushButton* m_ExtrapolateButton;
	QPushButton* m_VisualizeButton;
	QPushButton* m_UpdateKfilterButton;
	QPushButton* m_RemoveKFilter;
	QPushButton* m_Save;
	QPushButton* m_Load;
	QPushButton* m_RemoveNonSelected;
	QPushButton* m_AddToTissues;
	QPushButton* m_ExportLines;
	QPushButton* m_KFilterPredict;

	QLineEdit* m_SelectedObjects;
	QLineEdit* m_SigmaLow;
	QLineEdit* m_SigmaHi;
	QLineEdit* m_NumberSigmaLevels;
	QLineEdit* m_Threshold;
	QLineEdit* m_MaxRadius;
	QLineEdit* m_MinObjectSize;
	QLineEdit* m_MinProbability;
	QLineEdit* m_WDistance;
	QLineEdit* m_WParams;
	QLineEdit* m_WPred;
	QLineEdit* m_LimitSlice;
	QLineEdit* m_LineRadius;

	QCheckBox* m_NonMaxSuppression;
	QCheckBox* m_Skeletonize;
	QCheckBox* m_Add;
	QCheckBox* m_RestartKFilter;
	QCheckBox* m_ConnectDots;
	QCheckBox* m_ExtrapolateOnlyMatches;
	QCheckBox* m_AddPixel;

	int m_MaxActiveSliceReached = 0;
	int m_ActiveSlice = 0;

	std::vector<std::vector<std::string>> m_Objects;
	std::vector<std::vector<std::string>> m_Probabilities;

	std::vector<label_map_type::Pointer> m_LabelMaps;

	itk::Image<unsigned char, 2>::Pointer m_MaskImage;

	std::vector<std::map<label_type, std::string>> m_LabelToText;
	std::vector<KalmanFilter> m_KFilters;
	std::vector<int> m_Selected;

	Cache m_CachedData;

private slots:
	void SelectObjects();
	void DoWork();
	void ItemDoubleClicked(QListWidgetItem* item);
	void ItemEdited(QListWidgetItem* item);
	void ItemSelected();
	void MergeSelectedItems();
	void ExtrapolateResults();
	void Visualize();
	void RemoveObject();
	void RemoveKFilter();
	void UpdateKalmanFilters();
	void KFilterDoubleClicked(QListWidgetItem* item);
	void Save();
	void Load();
	void RemoveNonSelected();
	void AddToTissues();
	void ExportLines();
	void PredictKFilter();
};

} // namespace iseg
