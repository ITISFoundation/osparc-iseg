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

#include "Data/Property.h"

#include <algorithm>

namespace iseg {

class SmoothingWidget : public WidgetInterface
{
	Q_OBJECT
public:
	SmoothingWidget(SlicesHandler* hand3D);
	~SmoothingWidget() override;
	void Init() override;
	void NewLoaded() override;
	FILE* SaveParams(FILE* fp, int version) override;
	FILE* LoadParams(FILE* fp, int version) override;
	void HideParamsChanged() override;
	std::string GetName() override { return std::string("Smooth"); }
	QIcon GetIcon(QDir picdir) override { return QIcon(picdir.absoluteFilePath("smoothing.png")); }

private:
	void OnSlicenrChanged() override;

	void Execute();
	void ContinueDiff();
	void MethodChanged();
	void SigmasliderChanged(int v);
	void KsliderChanged(int v);
	void NChanged();
	void KmaxChanged();
	void SliderPressed();
	void SliderReleased();

	Bmphandler* m_Bmphand;
	SlicesHandler* m_Handler3D;
	unsigned short m_Activeslice;

	PropertySlider_ptr m_SlSigma;
	PropertySlider_ptr m_SlK;
	PropertySlider_ptr m_SlRestrain;

	PropertyInt_ptr m_SbN;
	PropertyInt_ptr m_SbIter;
	PropertyInt_ptr m_SbKmax;

	PropertyBool_ptr m_Allslices;

	enum eModeTypes {
		kGaussian,
		kAverage,
		kMedian,
		kSigmafilter,
		kAnisodiff,
		keModeTypesSize
	};
	PropertyEnum_ptr m_Modegroup;

	PropertyButton_ptr m_Pushexec;
	PropertyButton_ptr m_Contdiff;

	bool m_Dontundo;

private slots:
	void BmphandChanged(Bmphandler* bmph); // TODO BL is this a slot?
};

} // namespace iseg
