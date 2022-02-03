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

#include "Data/SlicesHandlerInterface.h"
#include "Interface/WidgetInterface.h"

#include <itkImage.h>

#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

#include <map>

namespace iseg {

class TissueSeparatorWidget : public WidgetInterface
{
	Q_OBJECT
public:
	TissueSeparatorWidget(SlicesHandlerInterface* hand3D, QWidget* parent = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~TissueSeparatorWidget() override = default;
	void Init() override;
	void NewLoaded() override;
	void Cleanup() override;
	std::string GetName() override { return std::string("Separate Tissue"); }
	QIcon GetIcon(QDir picdir) override { return QIcon(picdir.absoluteFilePath(QString("graphcut.png"))); }

private:
	void OnTissuenrChanged(int i) override;
	void OnSlicenrChanged() override;

	void OnMouseClicked(iseg::Point p) override;
	void OnMouseReleased(iseg::Point p) override;
	void OnMouseMoved(iseg::Point p) override;

private slots:
	void Execute();
	void Clearmarks();

private:
	void DoWorkAllSlices();
	void DoWorkCurrentSlice();

	template<unsigned int Dim, typename TInput>
	typename itk::Image<unsigned char, Dim>::Pointer DoWork(TInput* source, TInput* target, const typename itk::Image<unsigned char, Dim>::RegionType& requested_region);

	iseg::SlicesHandlerInterface* m_SliceHandler;
	unsigned m_CurrentSlice;
	unsigned m_Tissuenr;
	iseg::Point m_LastPt;
	std::vector<iseg::Point> m_Vpdyn;
	std::map<unsigned, std::vector<iseg::Mark>> m_Vm;

	QCheckBox* m_AllSlices;
	QCheckBox* m_UseSource;
	QLineEdit* m_SigmaEdit;
	QPushButton* m_ClearLines;
	QPushButton* m_ExecuteButton;
};

} // namespace iseg
