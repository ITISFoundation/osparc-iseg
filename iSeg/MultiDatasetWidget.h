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

#include "Data/DataSelection.h"

#include <QWidget>

#include <vector>
#include <map>

class QGroupBox;
class QRadioButton;
class QPushButton;
// Qt3
class Q3HBoxLayout;
class Q3VBoxLayout;

namespace iseg {

class SlicesHandler;

class MultiDatasetWidget : public QWidget
{
	Q_OBJECT
public:
	struct SDatasetInfo
	{
		QRadioButton* m_RadioButton;
		QString m_RadioButtonText;
		QStringList m_DatasetFilepath;
		unsigned m_Width;
		unsigned m_Height;
		std::vector<float*> m_BmpSlices;
		std::vector<float*> m_WorkSlices;
		bool m_IsActive;
	};

	enum DatasetTypeEnum {
		DCM,
		BMP,
		PNG,
		RAW,
		MHD,
		AVW,
		VTK,
		XDMF,
		NIFTI,
		RTDOSE
	};

	MultiDatasetWidget(SlicesHandler* hand3D, QWidget* parent = 0,
			const char* name = 0, Qt::WindowFlags wFlags = 0);
	~MultiDatasetWidget();
	void NewLoaded();
	int GetNumberOfDatasets();
	bool IsActive(const int multiDS_index);
	bool IsChecked(const int multiDS_index);

	std::vector<float*> GetBmpData(const int multiDS_index);
	void SetBmpData(const int multiDS_index, std::vector<float*> bmp_bits_vc);

	std::vector<float*> GetWorkingData(const int multiDS_index);
	void SetWorkingData(const int multiDS_index,
			std::vector<float*> work_bits_vc);

protected:
	void Initialize();
	void InitializeMap();
	void ClearRadioButtons();

	bool CheckInfoAndAddToList(SDatasetInfo& newRadioButton,
					QStringList loadfilenames, unsigned short width,
					unsigned short height, unsigned short nrofslices);
	bool AddDatasetToList(SDatasetInfo& newRadioButton,
			QStringList loadfilenames);
	void CopyImagesSlices(
			const std::vector<const float*>& bmp_slices, 
			const std::vector<const float*>& work_slices,
			const std::array<size_t,3>& dims, 
			SDatasetInfo& newRadioButton);

signals:
	void begin_datachange(iseg::DataSelection& dataSelection,
			QWidget* sender = nullptr, bool beginUndo = true);
	void end_datachange(QWidget* sender = nullptr,
			iseg::EndUndoAction undoAction = iseg::EndUndo);
	void dataset_changed();

private slots:
	void AddDatasetPressed();
	void SwitchDataset();
	void ChangeDatasetName();
	void RemoveDataset();
	void DatasetSelectionChanged();

private:
	SlicesHandler* m_Handler3D;
	std::vector<SDatasetInfo> m_RadioButtons;

	Q3HBoxLayout* hboxOverall;
	Q3VBoxLayout* vboxOverall;
	Q3VBoxLayout* m_VboxOverall;
	Q3VBoxLayout* m_VboxDatasets;
	QGroupBox* m_DatasetsGroupBox;
	QPushButton* m_AddDatasetButton;
	QPushButton* m_LoadDatasetButton;
	QPushButton* m_ChangeNameButton;
	QPushButton* m_RemoveDatasetButton;

	bool m_ItIsBeingLoaded;
	std::map<std::string, DatasetTypeEnum> m_MapDatasetValues;
};

}
