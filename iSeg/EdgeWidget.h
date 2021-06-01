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
	EdgeWidget(SlicesHandler* hand3D, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~EdgeWidget() override;
	void Init() override;
	void NewLoaded() override;
	FILE* SaveParams(FILE* fp, int version) override;
	FILE* LoadParams(FILE* fp, int version) override;
	std::string GetName() override { return std::string("Edge"); }
	QIcon GetIcon(QDir picdir) override { return QIcon(picdir.absoluteFilePath(QString("edge.png"))); }

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
	std::shared_ptr<PropertyEnum> m_Modegroup;

	std::shared_ptr<PropertyInt> m_SlSigma; // scaled to [0,5]
	std::shared_ptr<PropertyInt> m_SlThresh1; // scaled to [0,150]
	std::shared_ptr<PropertyInt> m_SlThresh2; // scaled to [0,150]
	std::shared_ptr<PropertyBool> m_Cb3d;

	std::shared_ptr<PropertyButton> m_BtnExportCenterlines;
	std::shared_ptr<PropertyButton> m_BtnExec;
};

} // namespace iseg

#endif
