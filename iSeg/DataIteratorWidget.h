/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in
 Society (IT'IS).
 *
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 *
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#pragma once

#include "../Data/DataSelection.h"
#include "../Data/TimeStamp.h"

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>

#include <map>

namespace iseg {

class SlicesHandler;

class DataIteratorWidget : public QWidget
{
	Q_OBJECT
public:
	DataIteratorWidget(iseg::SlicesHandler* hand3D, QWidget* parent = nullptr,
			Qt::WindowFlags wFlags = Qt::WindowFlags());
	~DataIteratorWidget() override = default;

private:
	void LoadNext(bool skip_processed);
	void Load(int pair_idx);

private:
	iseg::SlicesHandler* m_Handler3D;

	QLineEdit* m_ImageDir;
	QLineEdit* m_LabelsDir;
	QLineEdit* m_OutputDir;
	QLineEdit* m_TargetDir;
	QLineEdit* m_CurrentPairEdit;
	QCheckBox* m_LoadProcessed;
	QLabel* m_Status;

	QPushButton* m_SaveButton;
	QPushButton* m_SaveMarksButton;
	QPushButton* m_NextButton;
	QPushButton* m_NextUnprocessedButton;

	QComboBox* m_Orientation;

	std::string m_CachedImageDir;
	std::string m_CachedLabelsDir;
	int m_CurrentPair = -1;
	TimeStamp m_FileTime;
	std::vector<std::pair<std::string, std::string>> m_ImageLabelPairs;

signals:
	void BeginDatachange(iseg::DataSelection& dataSelection,
			QWidget* sender = nullptr, bool beginUndo = true);
	void EndDatachange(QWidget* sender = nullptr,
			iseg::eEndUndoAction undoAction = iseg::EndUndo);

private slots:
	void Save();
	void SaveMarks();
	void Next();
	void NextUnprocessed();
	void CurrentPairChanged();
	void Reload();
	void ReloadOrientation();
};

} // namespace iseg
