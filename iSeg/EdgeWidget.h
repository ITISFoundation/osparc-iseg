/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef EDGEWIDGET_3MARCH05
#define EDGEWIDGET_3MARCH05

#include "SlicesHandler.h"
#include "bmp_read_1.h"

#include "Interface/WidgetInterface.h"

#include "Data/Property.h"

#include <algorithm>

namespace iseg {

class EdgeWidget : public WidgetInterface
{
	Q_OBJECT
public:
	EdgeWidget(SlicesHandler* hand3D);
	~EdgeWidget() override;
	void Init() override;
	void NewLoaded() override;
	FILE* SaveParams(FILE* fp, int version) override;
	FILE* LoadParams(FILE* fp, int version) override;
	std::string GetName() override { return std::string("Edge"); }
	QIcon GetIcon(QDir picdir) override { return QIcon(picdir.absoluteFilePath("edge.png")); }

private:
	void OnSlicenrChanged() override;

	void Execute();
	void ExportCenterlines();
	void MethodChanged();
	void SliderChanged();

	Bmphandler* m_Bmphand = nullptr;
	SlicesHandler* m_Handler3D = nullptr;
	unsigned short m_Activeslice = 0;
	bool m_BlockExecute = false;

	enum eModeType {
		kSobel = 0,
		kLaplacian,
		kInterquartile,
		kMomentline,
		kGaussline,
		kCanny,
		kLaplacianzero,
		kCenterlines,
		eModeTypeSize
	};
	PropertyEnum_ptr m_Modegroup;

	PropertySlider_ptr m_SlSigma; // scaled to [0,5]
	PropertySlider_ptr m_SlThresh1; // scaled to [0,150]
	PropertySlider_ptr m_SlThresh2; // scaled to [0,150]
	PropertyBool_ptr m_Cb3d;

	PropertyButton_ptr m_BtnExportCenterlines;
	PropertyButton_ptr m_BtnExec;
};

} // namespace iseg

#endif
