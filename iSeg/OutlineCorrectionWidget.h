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

#include "Plugin/WidgetInterface.h"

#include <q3hbox.h>
#include <q3mimefactory.h>
#include <q3vbox.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qdialog.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpixmap.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qslider.h>
#include <qspinbox.h>
#include <qwidget.h>

namespace iseg {

class OutlineCorrectionWidget : public WidgetInterface
{
	Q_OBJECT
public:
	OutlineCorrectionWidget(SlicesHandler *hand3D, QWidget *parent = 0,
			const char *name = 0, Qt::WindowFlags wFlags = 0);
	~OutlineCorrectionWidget();
	QSize sizeHint() const override;
	void cleanup() override;

	void init() override;
	void newloaded() override;
	FILE *SaveParams(FILE *fp, int version) override;
	FILE *LoadParams(FILE *fp, int version) override;
	void hideparams_changed() override;
	std::string GetName() override { return std::string("OLC"); }
	QIcon GetIcon(QDir picdir) override { return QIcon(picdir.absFilePath(QString("olc.png"))); }

	void Select_selected_tissue_BG(QString tissueName, tissues_size_t nr);
	void Select_selected_tissue_TS(QString tissueName, tissues_size_t nr);

protected:
	void on_tissuenr_changed(int i) override;
	void on_slicenr_changed() override;

	void on_mouse_clicked(Point p) override;
	void on_mouse_released(Point p) override;
	void on_mouse_moved(Point p) override;

private:
	tissues_size_t tissuenr;
	tissues_size_t tissuenrnew;
	float f;
	bool draw;
	bool selectobj;
	bmphandler *bmphand;
	SlicesHandler *handler3D;
	unsigned short activeslice;
	float spacing[3];
	Point last_pt;
	Q3VBox *vbox1;
	Q3HBox *hbox1;
	Q3HBox *hbox2;
	Q3HBox *hbox2a;
	Q3HBox *hbox3a;
	Q3HBox *hboxoverall;
	Q3VBox *vboxmethods;
	//	Q3HBox *hbox3;
	//	Q3HBox *hbox3cont;
	Q3HBox *hbox4;
	Q3HBox *hbox5o;
	Q3HBox *hbox5;
	Q3HBox *hbox6;
	Q3HBox *hboxpixormm;
	QRadioButton *brush;
	QRadioButton *olcorr;
	QRadioButton *holefill;
	QRadioButton *removeislands;
	QRadioButton *gapfill;
	QRadioButton *allfill;
	QRadioButton *addskin;
	QRadioButton *fillskin;
	QButtonGroup *method;
	QRadioButton *tissue;
	QRadioButton *work;
	QButtonGroup *target;
	QRadioButton *erasebrush;
	QRadioButton *drawbrush;
	QRadioButton *modifybrush;
	QRadioButton *adapt;
	QButtonGroup *brushtype;
	QRadioButton *inside;
	QRadioButton *outside;
	QButtonGroup *in_or_out;
	QRadioButton *pixel;
	QRadioButton *mm;
	QButtonGroup *pixelormm;
	QLabel *txt_unit;
	QLabel *txt_radius;
	QSpinBox *sb_radius;
	QLineEdit *mm_radius;
	QLabel *txt_holesize;
	QLabel *txt_gapsize;
	QSpinBox *sb_holesize;
	QSpinBox *sb_gapsize;
	QPushButton *pb_removeholes;
	QPushButton *pb_selectobj;
	QCheckBox *allslices;
	void draw_circle(Point p);
	std::vector<Point> vpdyn;
	bool dontundo;

	Q3HBox *tissuesListBackground;
	Q3HBox *tissuesListSkin;
	QPushButton *getCurrentTissueBackground;
	QPushButton *getCurrentTissueSkin;
	QLineEdit *backgroundText;
	QLineEdit *skinText;
	tissues_size_t selectedBacgroundID;
	tissues_size_t selectedSkinID;
	bool backgroundSelected;
	bool skinSelected;

public:
	QCheckBox *fb;
	QCheckBox *bg;

signals:
	void vpdyn_changed(std::vector<Point> *vpdyn_arg);

	void signal_request_selected_tissue_TS(); // TODO BL hack
	void signal_request_selected_tissue_BG();

public slots:
	void pixmm_changed();
	void workbits_changed();

	void request_selected_tissue_BG();
	void request_selected_tissue_TS();

private slots:
	void bmphand_changed(bmphandler *bmph);
	void method_changed();
	void removeholes_pushed();
	void selectobj_pushed();
};

} // namespace iseg

#endif
