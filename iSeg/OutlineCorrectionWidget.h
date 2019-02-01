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

	void select_background(QString tissueName, tissues_size_t nr);
	void select_skin(QString tissueName, tissues_size_t nr);

private:
	void on_tissuenr_changed(int i) override;
	void on_slicenr_changed() override;

	void on_mouse_clicked(Point p) override;
	void on_mouse_released(Point p) override;
	void on_mouse_moved(Point p) override;

	void draw_circle(Point p);

	float get_object_value() const;

	tissues_size_t tissuenr;
	tissues_size_t tissuenrnew;
	bool draw;
	bool selectobj;
	bmphandler* bmphand;
	SlicesHandler* handler3D;
	unsigned short activeslice;
	Vec3 spacing;
	Point last_pt;

	QListWidget* methods;
	QListWidgetItem* olcorr;
	QListWidgetItem* brush;
	QListWidgetItem* holefill;
	QListWidgetItem* removeislands;
	QListWidgetItem* gapfill;
	QListWidgetItem* addskin;
	QListWidgetItem* fillskin;
	QListWidgetItem* allfill;
	QListWidgetItem* adapt;
	QListWidgetItem* smooth_tissues;

	QWidget* parameter_area;
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

#if 0
	Q3HBox* hbox1;
	Q3HBox* hbox2;
	Q3HBox* hbox2a;
	Q3HBox* hbox3a;
	Q3HBox* hbox4;
	Q3HBox* hbox5o;
	Q3HBox* hbox5;
	Q3HBox* hbox6;
	Q3HBox* hboxpixormm;

	QRadioButton* tissue;
	QRadioButton* work;
	QButtonGroup* target;

	QButtonGroup* brushtype;
	QRadioButton* modifybrush;
	QRadioButton* drawbrush;
	QRadioButton* erasebrush;

	QButtonGroup* in_or_out;
	QRadioButton* inside;
	QRadioButton* outside;

	QButtonGroup* pixelormm;
	QRadioButton* pixel;
	QRadioButton* mm;
	QLabel* txt_unit;
	QLabel* txt_radius;
	QSpinBox* sb_radius;
	QLineEdit* mm_radius;
	QLabel* txt_holesize;
	QLabel* txt_gapsize;
	QSpinBox* sb_holesize;
	QSpinBox* sb_gapsize;
	QPushButton* pb_execute;
	QPushButton* pb_selectobj;
	QCheckBox* allslices;
#endif
	std::vector<Point> vpdyn;
	bool dontundo;
#if 0
	Q3HBox* tissuesListBackground;
	Q3HBox* tissuesListSkin;
	QPushButton* getCurrentTissueBackground;
	QPushButton* getCurrentTissueSkin;
	QLineEdit* backgroundText;
	QLineEdit* skinText;

	QLineEdit* object_value;

	Q3HBox* hbox_prev_slice;
	QCheckBox* cb_show_guide;
	QSpinBox* sb_guide_offset;
	QPushButton* pb_copy_guide;
	QPushButton* pb_copy_pick_guide;
#endif
	bool copy_mode = false;

public slots:
	void pixmm_changed();
	void workbits_changed();

	void on_select_background();
	void on_select_skin();

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
