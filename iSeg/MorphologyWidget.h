/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef MORPHO_4MARCH05
#define MORPHO_4MARCH05

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
	MorphologyWidget(SlicesHandler* hand3D, QWidget* parent = 0,
			const char* name = 0, Qt::WindowFlags wFlags = 0);
	~MorphologyWidget() {}
	void init() override;
	void newloaded() override;
	FILE* SaveParams(FILE* fp, int version) override;
	FILE* LoadParams(FILE* fp, int version) override;
	void hideparams_changed() override;
	std::string GetName() override { return std::string("Morpho"); }
	QIcon GetIcon(QDir picdir) override { return QIcon(picdir.absFilePath(QString("morphology.png"))); }

private:
	void on_slicenr_changed() override;

	bmphandler* bmphand;
	SlicesHandler* handler3D;
	unsigned short activeslice;

	QRadioButton* rb_open;
	QRadioButton* rb_close;
	QRadioButton* rb_erode;
	QRadioButton* rb_dilate;
	QCheckBox* node_connectivity;
	QCheckBox* true_3d;
	QLineEdit* operation_radius;
	QCheckBox* pixel_units;
	QCheckBox* all_slices;
	QPushButton* execute_button;

private slots:
	void bmphand_changed(bmphandler* bmph);
	void execute();
	void units_changed();
	void all_slices_changed();
};

} // namespace iseg

#endif
