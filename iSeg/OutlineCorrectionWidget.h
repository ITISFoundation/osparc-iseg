/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef OLC_4APRIL05
#define OLC_4APRIL05

#include "SlicesHandler.h"
#include "bmp_read_1.h"

#include "Interface/WidgetInterface.h"

#include <q3hbox.h>
#include <q3mimefactory.h>
#include <q3vbox.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlistwidget.h>
#include <qpixmap.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qslider.h>
#include <qspinbox.h>

class QStackedLayout;

namespace iseg {

class OLCorrParamView;
class BrushParamView;
class FillHolesParamView;
class AddSkinParamView;
class FillSkinParamView;
class FillAllParamView;
class AdaptParamView;
class SmoothTissuesParamView;

class ParamViewBase : public QWidget
{
public:
	ParamViewBase(QWidget* parent = nullptr) : QWidget(parent) {}

	virtual bool work() const { return _work; }
	virtual void set_work(bool v) { _work = v; }

	virtual float object_value() const { return _object_value; }
	virtual void set_object_value(float v) { _object_value = v; }

private:
	// cache setting
	bool _work = true;
	float _object_value = 0.f;
};

/** \brief Class which contains different methods to modify/correct target or tissues
*/
class OutlineCorrectionWidget : public WidgetInterface
{
	Q_OBJECT
public:
	OutlineCorrectionWidget(SlicesHandler* hand3D, QWidget* parent = 0,
			const char* name = 0, Qt::WindowFlags wFlags = 0);
	~OutlineCorrectionWidget();
	void cleanup() override;

	void init() override;
	void newloaded() override;
	FILE* SaveParams(FILE* fp, int version) override;
	FILE* LoadParams(FILE* fp, int version) override;
	void hideparams_changed() override;
	std::string GetName() override { return std::string("OLC"); }
	QIcon GetIcon(QDir picdir) override { return QIcon(picdir.absFilePath(QString("olc.png"))); }

private:
	void on_tissuenr_changed(int i) override;
	void on_slicenr_changed() override;

	void on_mouse_clicked(Point p) override;
	void on_mouse_released(Point p) override;
	void on_mouse_moved(Point p) override;

	void draw_circle(Point p);

	// \todo move to method class?
	void select_background(QString tissueName, tissues_size_t nr);
	void select_skin(QString tissueName, tissues_size_t nr);

	float get_object_value() const;

	// parameter view
	QButtonGroup* methods;
	QRadioButton* olcorr;
	QRadioButton* brush;
	QRadioButton* holefill;
	QRadioButton* removeislands;
	QRadioButton* gapfill;
	QRadioButton* addskin;
	QRadioButton* fillskin;
	QRadioButton* allfill;
	QRadioButton* adapt;
	QRadioButton* smooth_tissues;

	QWidget* parameter_area;
	ParamViewBase* current_params = nullptr;
	QStackedLayout* stacked_param_layout;
	OLCorrParamView* olc_params;
	BrushParamView* brush_params;
	FillHolesParamView* fill_holes_params;
	FillHolesParamView* remove_islands_params;
	FillHolesParamView* fill_gaps_params;
	AddSkinParamView* add_skin_params;
	FillSkinParamView* fill_skin_params;
	FillAllParamView* fill_all_params;
	AdaptParamView* adapt_params;
	SmoothTissuesParamView* smooth_tissues_params;

	// member/state variables
	tissues_size_t tissuenr;
	tissues_size_t tissuenrnew;
	bool draw;
	bool selectobj;
	bmphandler* bmphand;
	SlicesHandler* handler3D;
	unsigned short activeslice;
	Vec3 spacing;
	Point last_pt;

	std::vector<Point> vpdyn;
	bool dontundo;
	bool copy_mode = false;

public slots:
	void workbits_changed();

private slots:
	void bmphand_changed(bmphandler* bmph);
	void method_changed();
	void execute_pushed();
	void selectobj_pushed();
	void draw_guide();
	void copy_guide(Point* p = nullptr);
	void copy_pick_pushed();
	void smooth_tissues_pushed();
};

} // namespace iseg

#endif
