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

#include "SlicesHandler.h"
#include "bmp_read_1.h"

#include "Interface/WidgetInterface.h"

class QRadioButton;
class QButtonGroup;
class QSpinBox;
class QCheckBox;
class QLineEdit;
class QPushButton;

namespace iseg {

class MorphologyWidget : public WidgetInterface
{
	Q_OBJECT
public:
	MorphologyWidget(SlicesHandler* hand3D, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~MorphologyWidget() override = default;
	void Init() override;
	void NewLoaded() override;
	FILE* SaveParams(FILE* fp, int version) override;
	FILE* LoadParams(FILE* fp, int version) override;
	void HideParamsChanged() override;
	std::string GetName() override { return std::string("Morpho"); }
	QIcon GetIcon(QDir picdir) override { return QIcon(picdir.absFilePath(QString("morphology.png"))); }

private:
	void OnSlicenrChanged() override;

	Bmphandler* m_Bmphand;
	SlicesHandler* m_Handler3D;
	unsigned short m_Activeslice;

	QRadioButton* m_RbOpen;
	QRadioButton* m_RbClose;
	QRadioButton* m_RbErode;
	QRadioButton* m_RbDilate;
	QCheckBox* m_NodeConnectivity;
	QCheckBox* m_True3d;
	QLineEdit* m_OperationRadius;
	QCheckBox* m_PixelUnits;
	QCheckBox* m_AllSlices;
	QPushButton* m_ExecuteButton;

private slots:
	void BmphandChanged(Bmphandler* bmph);
	void Execute();
	void UnitsChanged();
	void AllSlicesChanged();
};

} // namespace iseg
