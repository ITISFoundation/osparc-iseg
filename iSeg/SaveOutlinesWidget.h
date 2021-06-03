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

#include "Data/Property.h"

#include <QDialog>

class QListWidget;

namespace iseg {

class SaveOutlinesWidget : public QDialog
{
	Q_OBJECT
public:
	SaveOutlinesWidget(SlicesHandler* hand3D, QWidget* parent = nullptr, Qt::WindowFlags wFlags = Qt::Widget);

private:
	void ModeChanged();
	void SimplifyChanged();
	void ExtrusionChanged();
	void SavePushed();

	SlicesHandler* m_Handler3D;

	QListWidget* m_LboTissues;

	enum eOutputType {
		kLine,
		kTriang
	};
	PropertyEnum_ptr m_Filetype;

	enum eSimplifyLines {
		kNone,
		kDougpeuck,
		kDist
	};
	PropertyEnum_ptr m_SimplifyLines;

	PropertyInt_ptr m_SbDist;
	PropertyInt_ptr m_SbMinsize;
	PropertyInt_ptr m_SbBetween;
	PropertySlider_ptr m_SlF;
	PropertyBool_ptr m_CbExtrusion;
	PropertyInt_ptr m_SbTopextrusion;
	PropertyInt_ptr m_SbBottomextrusion;

	PropertyBool_ptr m_CbSmooth;
	PropertyBool_ptr m_CbSimplify;
	PropertyBool_ptr m_CbMarchingcubes;

	PropertyButton_ptr m_PbExec;
	PropertyButton_ptr m_PbClose;
};

} // namespace iseg
