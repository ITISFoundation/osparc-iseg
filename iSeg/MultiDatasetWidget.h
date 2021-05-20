/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
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

#include <map>
#include <vector>

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
		bool m_IsActive;
	};

	MultiDatasetWidget(SlicesHandler* hand3D, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~MultiDatasetWidget() override;
	void NewLoaded();
	int GetNumberOfDatasets();
	bool IsActive(const int multiDS_index);
	bool IsChecked(const int multiDS_index);

	std::vector<float*> GetBmpData(const int multiDS_index);
	void SetBmpData(const int multiDS_index, std::vector<float*> bmp_bits_vc);

protected:
	void Initialize();
	void ClearRadioButtons();

	bool CheckInfoAndAddToList(SDatasetInfo& newRadioButton, QStringList loadfilenames, unsigned short width, unsigned short height, unsigned short nrofslices);
	bool AddDatasetToList(SDatasetInfo& newRadioButton, QStringList loadfilenames);
	void CopyImagesSlices(const std::vector<const float*>& bmp_slices, const std::array<size_t, 3>& dims, SDatasetInfo& newRadioButton);

signals:
	void BeginDatachange(DataSelection& dataSelection, QWidget* sender = nullptr, bool beginUndo = true);
	void EndDatachange(QWidget* sender = nullptr, eEndUndoAction undoAction = iseg::EndUndo);
	void DatasetChanged();

private slots:
	void AddDatasetPressed();
	void SwitchDataset();
	void ChangeDatasetName();
	void RemoveDataset();
	void DatasetSelectionChanged();

private:
	SlicesHandler* m_Handler3D;
	std::vector<SDatasetInfo> m_RadioButtons;

	Q3HBoxLayout* m_HboxOverall;
	Q3VBoxLayout* m_VboxOverall;
	Q3VBoxLayout* m_VboxDatasets;
	QGroupBox* m_DatasetsGroupBox;
	QPushButton* m_AddDatasetButton;
	QPushButton* m_LoadDatasetButton;
	QPushButton* m_ChangeNameButton;
	QPushButton* m_RemoveDatasetButton;

	bool m_ItIsBeingLoaded;
};

} // namespace iseg
