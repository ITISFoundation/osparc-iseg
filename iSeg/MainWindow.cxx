/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "MainWindow.h"
#include "AtlasWidget.h"
#include "FormatTooltip.h"
#include "InternalUse.h"
#include "Precompiled.h"
#include "SaveOutlinesWidget.h"
#include "bmpshower.h"
#include "loaderwidgets.h"
#include "tissueinfos.h"

#include "../config.h"
#include "IFT_rg.h"
#include "Settings.h"
#include "activeslices_config_widget.h"
#include "edge_widget.h"
#include "fastmarch_fuzzy_widget.h"
#include "featurewidget.h"
#include "hyster_widget.h"
#include "interpolation_widget.h"
#include "livewirewidget.h"
#include "measurementwidget.h"
#include "morphowidget.h"
#include "outlinecorrection.h"
#include "pickerwidget.h"
#include "pixelsize_widget.h"
#include "smoothwidget.h"
#include "threshwidget.h"
#include "tissuecleaner.h"
#include "transformwidget.h"
#include "undoconfig_widget.h"
#include "vessel_widget.h"
#include "watershedwidget.h"
#include "xyslice.h"

#ifdef ISEG_GPU_MC
#	include "window.h"
#endif

#ifndef NORTSTRUCTSUPPORT
#	include "rtstruct_importer.h"
#endif

#include "Addon/Addon.h"

#include "Core/LoadPlugin.h"
#include "Core/Transform.h"

#include <boost/filesystem.hpp>

#include <QDesktopWidget>
#include <QSignalMapper.h>
#include <q3accel.h>
#include <q3filedialog.h>
#include <q3popupmenu.h>
#include <qapplication.h>
#include <qdockwidget.h>
#include <qmenubar.h>
#include <qprogressdialog.h>
#include <qsettings.h>
#include <qtooltip.h>

#include <q3widgetstack.h>
#include <qtextedit.h>

using namespace std;

namespace {
int openS4LLinkPos = -1;
int importSurfacePos = -1;
int importRTstructPos = -1;
} // namespace

int bmpimgnr(QString *s)
{
	int result = 0;
	uint pos;
	if (s->length() > 4 && s->endsWith(QString(".bmp"))) {
		pos = s->length() - 5;
	}
	else {
		pos = s->length() - 1;
	}
	int base = 1;
	QChar cdigit;
	while (pos > 0 && (cdigit = s->at(pos)).isDigit()) {
		result += cdigit.digitValue() * base;
		base *= 10;
		pos--;
	}
	if (pos = 0 && (cdigit = s->at(0)).isDigit())
		result += cdigit.digitValue() * base;

	return result;
}

int pngimgnr(QString *s)
{
	int result = 0;
	uint pos;
	if (s->length() > 4 && s->endsWith(QString(".png"))) {
		pos = s->length() - 5;
	}
	else {
		pos = s->length() - 1;
	}
	int base = 1;
	QChar cdigit;
	while (pos > 0 && (cdigit = s->at(pos)).isDigit()) {
		result += cdigit.digitValue() * base;
		base *= 10;
		pos--;
	}
	if (pos = 0 && (cdigit = s->at(0)).isDigit())
		result += cdigit.digitValue() * base;

	return result;
}

int jpgimgnr(QString *s)
{
	int result = 0;
	uint pos;
	if (s->length() > 4 && s->endsWith(QString(".jpg"))) {
		pos = s->length() - 5;
	}
	else {
		pos = s->length() - 1;
	}
	int base = 1;
	QChar cdigit;
	while (pos > 0 && (cdigit = s->at(pos)).isDigit()) {
		result += cdigit.digitValue() * base;
		base *= 10;
		pos--;
	}
	if (pos = 0 && (cdigit = s->at(0)).isDigit())
		result += cdigit.digitValue() * base;

	return result;
}

QString TruncateFileName(QString str)
{
	if (str != QString("")) {
		int pos = str.findRev('/', -2);
		if (pos != -1 && (int)str.length() > pos + 1) {
			QString name1 = str.right(str.length() - pos - 1);
			return name1;
		}
	}
	return str;
}

bool read_grouptissues(const char *filename, vector<tissues_size_t> &olds,
											 vector<tissues_size_t> &news)
{
	FILE *fp;
	if ((fp = fopen(filename, "r")) == NULL) {
		return false;
	}
	else {
		char name1[1000];
		char name2[1000];
		while (fscanf(fp, "%s %s\n", name1, name2) == 2) {
			olds.push_back(TissueInfos::GetTissueType(QString(name1)));
			news.push_back(TissueInfos::GetTissueType(QString(name2)));
		}
		fclose(fp);
		return true;
	}
}

bool read_grouptissuescapped(const char *filename, vector<tissues_size_t> &olds,
														 vector<tissues_size_t> &news,
														 bool fail_on_unknown_tissue)
{
	FILE *fp;
	if ((fp = fopen(filename, "r")) == NULL) {
		return false;
	}
	else {
		char name1[1000];
		char name2[1000];
		tissues_size_t type1, type2;
		tissues_size_t tissueCount = TissueInfos::GetTissueCount();

		// Try scan first entry to decide between tissue name and index input
		bool readIndices = false;
		if (fscanf(fp, "%s %s\n", name1, name2) == 2) {
			type1 = (tissues_size_t)QString(name1).toUInt(&readIndices);
		}

		bool ok = true;
		if (readIndices) {
			// Read input as tissue indices
			type2 = (tissues_size_t)QString(name2).toUInt(&readIndices);
			if (type1 > 0 && type1 <= tissueCount && type2 > 0 &&
					type2 <= tissueCount) {
				olds.push_back(type1);
				news.push_back(type2);
			}
			else
				ok = false;

			int tmp1, tmp2;
			while (fscanf(fp, "%i %i\n", &tmp1, &tmp2) == 2) {
				type1 = (tissues_size_t)tmp1;
				type2 = (tissues_size_t)tmp2;
				if (type1 > 0 && type1 <= tissueCount && type2 > 0 &&
						type2 <= tissueCount) {
					olds.push_back(type1);
					news.push_back(type2);
				}
				else
					ok = false;
			}
		}
		else {
			// Read input as tissue names
			type1 = TissueInfos::GetTissueType(QString(name1));
			type2 = TissueInfos::GetTissueType(QString(name2));
			if (type1 > 0 && type1 <= tissueCount && type2 > 0 &&
					type2 <= tissueCount) {
				olds.push_back(type1);
				news.push_back(type2);
			}
			else {
				ok = false;
				if (type1 == 0 || type1 > tissueCount)
					std::cerr << "old: " << name1 << " not in tissue list\n";
				if (type2 == 0 || type2 > tissueCount)
					std::cerr << "new: " << name2 << " not in tissue list\n";
			}
			memset(name1, 0, 1000);
			memset(name2, 0, 1000);

			while (fscanf(fp, "%s %s\n", name1, name2) == 2) {
				type1 = TissueInfos::GetTissueType(QString(name1));
				type2 = TissueInfos::GetTissueType(QString(name2));
				if (type1 > 0 && type1 <= tissueCount && type2 > 0 &&
						type2 <= tissueCount) {
					olds.push_back(type1);
					news.push_back(type2);
				}
				else {
					ok = false;
					if (type1 == 0 || type1 > tissueCount)
						std::cerr << "old: " << name1 << " not in tissue list\n";
					if (type2 == 0 || type2 > tissueCount)
						std::cerr << "new: " << name2 << " not in tissue list\n";
				}

				memset(name1, 0, 1000);
				memset(name2, 0, 1000);
			}
		}

		fclose(fp);

		return ok || !fail_on_unknown_tissue;
	}
}

bool read_tissues(const char *filename, std::vector<tissues_size_t> &types)
{
	FILE *fp;
	if ((fp = fopen(filename, "r")) == NULL) {
		return false;
	}

	tissues_size_t const tissueCount = TissueInfos::GetTissueCount();
	char name[1000];
	bool ok = true;

	while (fscanf(fp, "%s\n", name) == 1) {
		tissues_size_t type = TissueInfos::GetTissueType(QString(name));
		if (type > 0 && type <= tissueCount) {
			types.push_back(type);
		}
		else {
			ok = false;
			std::cerr << name << " not in tissue list\n";
		}
	}

	fclose(fp);

	return ok;
}

void style_dockwidget(QDockWidget *dockwidg)
{
	//dockwidg->setStyleSheet("QDockWidget { color: black; } QDockWidget::title { background: gray; padding-left: 5px; padding-top: 3px; color: darkgray} QDockWidget::close-button, QDockWidget::float-button { background: gray; }");
}

bool MenuWTT::event(QEvent *e)
{
	const QHelpEvent *helpEvent = static_cast<QHelpEvent *>(e);
	if (helpEvent->type() == QEvent::ToolTip) {
		// call QToolTip::showText on that QAction's tooltip.
		QPoint gpos = helpEvent->globalPos();
		QPoint pos = helpEvent->pos();
		if (pos.x() > 0 && pos.y() > 0 && pos.x() < 400 && pos.y() < 600) {
			QString textActive = activeAction()->text();
			QString justShow("Import RTstruct...");
			if (textActive == justShow)
				QToolTip::showText(gpos, activeAction()->toolTip());
			else
				QToolTip::hideText();
		}
	}
	else
		QToolTip::hideText();

	return QMenu::event(e);
}

MainWindow::MainWindow(SlicesHandler *hand3D, QString locationstring,
											 QDir picpath, QDir tmppath, bool editingmode,
											 QWidget *parent, const char *name,
											 Qt::WindowFlags wFlags, char **argv)
		: QMainWindow(parent, name, wFlags)
{
	undoStarted = false;
	setContentsMargins(9, 4, 9, 4);
	m_picpath = picpath;
	m_tmppath = tmppath;
	m_editingmode = editingmode;
	iSegVersion = ISEG_VERSION_MAJOR;
	iSegSubversion = ISEG_VERSION_MINOR;
	build = ISEG_VERSION_PATCH;
	InternalUse::SetForInternalUse(true);
	tab_old = NULL;
#ifdef ISEG_GPU_MC
	surfaceview = new Window();
#else
	surfaceview = nullptr;
#endif

	setCaption(QString(" iSeg ") + QString::number(iSegVersion) + QString(".") +
						 QString::number(iSegSubversion) + QString(" B") +
						 QString::number(build) + QString(" - No Filename"));
	QIcon isegicon(m_picpath.absFilePath(QString("isegicon.png")).ascii());
	setWindowIcon(isegicon);
	m_locationstring = locationstring;
	m_saveprojfilename = QString("");
	m_S4Lcommunicationfilename = QString("");

	handler3D = hand3D;

	if (!(handler3D->isloaded())) {
		handler3D->newbmp(512, 512, 10);
	}

	cb_bmptissuevisible = new QCheckBox("Show Tissues", this);
	cb_bmptissuevisible->setChecked(true);
	cb_bmpcrosshairvisible = new QCheckBox("Show Crosshair", this);
	cb_bmpcrosshairvisible->setChecked(false);
	cb_bmpoutlinevisible = new QCheckBox("Show Outlines", this);
	cb_bmpoutlinevisible->setChecked(true);
	cb_worktissuevisible = new QCheckBox("Show Tissues", this);
	cb_worktissuevisible->setChecked(false);
	cb_workcrosshairvisible = new QCheckBox("Show Crosshair", this);
	cb_workcrosshairvisible->setChecked(false);
	cb_workpicturevisible = new QCheckBox("Show Image", this);
	cb_workpicturevisible->setChecked(true);

	bmp_show = new bmptissuemarklineshower(
			this, "new window", Qt::WDestructiveClose | Qt::WResizeNoErase);
	lb_source = new QLabel("Source", this);
	lb_target = new QLabel("Target", this);
	bmp_scroller = new Q3ScrollView(this);
	sl_contrastbmp = new QSlider(Qt::Horizontal, this);
	sl_contrastbmp->setRange(0, 100);
	sl_brightnessbmp = new QSlider(Qt::Horizontal, this);
	sl_brightnessbmp->setRange(0, 100);
	lb_contrastbmp = new QLabel("C:", this);
	lb_contrastbmp->setPixmap(
			QIcon(m_picpath.absFilePath(QString("icon-contrast.png")).ascii())
					.pixmap());
	le_contrastbmp_val = new QLineEdit(this);
	le_contrastbmp_val->setAlignment(Qt::AlignRight);
	le_contrastbmp_val->setText(QString("%1").arg(9999.99, 6, 'f', 2));
	QString text = le_contrastbmp_val->text();
	QFontMetrics fm = le_contrastbmp_val->fontMetrics();
	QRect rect = fm.boundingRect(text);
	le_contrastbmp_val->setFixedSize(rect.width() + 4, rect.height() + 4);
	lb_contrastbmp_val = new QLabel("x", this);
	lb_brightnessbmp = new QLabel("B:", this);
	lb_brightnessbmp->setPixmap(
			QIcon(m_picpath.absFilePath(QString("icon-brightness.png")).ascii())
					.pixmap());
	le_brightnessbmp_val = new QLineEdit(this);
	le_brightnessbmp_val->setAlignment(Qt::AlignRight);
	le_brightnessbmp_val->setText(QString("%1").arg(9999, 3));
	text = le_brightnessbmp_val->text();
	fm = le_brightnessbmp_val->fontMetrics();
	rect = fm.boundingRect(text);
	le_brightnessbmp_val->setFixedSize(rect.width() + 4, rect.height() + 4);
	lb_brightnessbmp_val = new QLabel("%", this);
	bmp_scroller->addChild(bmp_show);
	bmp_show->init(handler3D, TRUE);
	bmp_show->set_workbordervisible(TRUE);
	bmp_show->setIsBmp(true);
	//	work_show->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
	bmp_show->update();

	toworkBtn = new QPushButton(
			QIcon(m_picpath.absFilePath(QString("next.png")).ascii()), "", this);
	toworkBtn->setFixedWidth(50);
	tobmpBtn = new QPushButton(
			QIcon(m_picpath.absFilePath(QString("previous.png")).ascii()), "", this);
	tobmpBtn->setFixedWidth(50);
	swapBtn = new QPushButton(
			QIcon(m_picpath.absFilePath(QString("swap.png")).ascii()), "", this);
	swapBtn->setFixedWidth(50);
	swapAllBtn = new QPushButton(
			QIcon(m_picpath.absFilePath(QString("swap.png")).ascii()), "3D", this);
	swapAllBtn->setFixedWidth(50);

	work_show = new bmptissuemarklineshower(
			this, "new window", Qt::WDestructiveClose | Qt::WResizeNoErase);
	sl_contrastwork = new QSlider(Qt::Horizontal, this);
	sl_contrastwork->setRange(0, 100);
	sl_brightnesswork = new QSlider(Qt::Horizontal, this);
	sl_brightnesswork->setRange(0, 100);
	lb_contrastwork = new QLabel(this);
	lb_contrastwork->setPixmap(
			QIcon(m_picpath.absFilePath(QString("icon-contrast.png")).ascii())
					.pixmap());
	le_contrastwork_val = new QLineEdit(this);
	le_contrastwork_val->setAlignment(Qt::AlignRight);
	le_contrastwork_val->setText(QString("%1").arg(9999.99, 6, 'f', 2));
	text = le_contrastwork_val->text();
	fm = le_contrastwork_val->fontMetrics();
	rect = fm.boundingRect(text);
	le_contrastwork_val->setFixedSize(rect.width() + 4, rect.height() + 4);
	lb_contrastwork_val = new QLabel("x", this);
	lb_brightnesswork = new QLabel(this);
	lb_brightnesswork->setPixmap(
			QIcon(m_picpath.absFilePath(QString("icon-brightness.png")).ascii())
					.pixmap());
	le_brightnesswork_val = new QLineEdit(this);
	le_brightnesswork_val->setAlignment(Qt::AlignRight);
	le_brightnesswork_val->setText(QString("%1").arg(9999, 3));
	text = le_brightnesswork_val->text();
	fm = le_brightnesswork_val->fontMetrics();
	rect = fm.boundingRect(text);
	le_brightnesswork_val->setFixedSize(rect.width() + 4, rect.height() + 4);
	lb_brightnesswork_val = new QLabel("%", this);
	work_scroller = new Q3ScrollView(this);
	work_scroller->addChild(work_show);
	//	work_scroller->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);
	work_show->init(handler3D, FALSE);
	work_show->set_tissuevisible(false); //toggle_tissuevisible();
	work_show->setIsBmp(false);

	reset_brightnesscontrast();

	zoom_widget = new zoomer_widget(1.0, m_picpath, this);

	tissueTreeWidget =
			new TissueTreeWidget(handler3D->get_tissue_hierachy(), m_picpath, this);
	tissueTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
	tissueFilter = new QLineEdit(this);
	tissueFilter->setMargin(1);
	tissueHierarchyWidget = new TissueHierarchyWidget(tissueTreeWidget, this);
	tissueTreeWidget->update_tree_widget(); // Reload hierarchy
	cb_tissuelock = new QCheckBox(this);
	cb_tissuelock->setPixmap(
			QIcon(m_picpath.absFilePath(QString("lock.png")).ascii()).pixmap());
	cb_tissuelock->setChecked(false);
	lockTissues = new QPushButton("All", this);
	lockTissues->setToggleButton(true);
	lockTissues->setFixedWidth(50);
	addTissue = new QPushButton("New Tissue...", this);
	addFolder = new QPushButton("New Folder...", this);
	//xxxb	addTissue->setFixedWidth(110);
	modifyTissueFolder = new QPushButton("Mod. Tissue/Folder", this);
	modifyTissueFolder->setToolTip(
			Format("Edit the selected tissue or folder properties."));
	//xxxb	modifyTissue->setFixedWidth(110);
	removeTissueFolder = new QPushButton("Del. Tissue/Folder", this);
	removeTissueFolder->setToolTip(
			Format("Remove the selected tissues or folders."));
	removeTissueFolderAll = new QPushButton("All", this);
	removeTissueFolderAll->setFixedWidth(30);
	//xxxb	removeTissue->setFixedWidth(110);
	tissue3Dopt = new QCheckBox("3D", this);
	tissue3Dopt->setChecked(false);
	tissue3Dopt->setToolTip(
			Format("'Get Tissue' is applied in 3D or current slice only."));
	getTissue = new QPushButton("Get Tissue", this);
	getTissue->setToolTip(Format("Get tissue creates a mask in the Target from "
															 "the currently selected Tissue."));
	getTissueAll = new QPushButton("All", this);
	getTissueAll->setFixedWidth(30);
	getTissueAll->setToolTip(Format("Get all tissue creates a mask in the Target "
																	"from all Tissue excluding the background."));
	//xxxb	getTissue->setFixedWidth(110);
	clearTissue = new QPushButton("Clear Tissue", this);
	clearTissue->setToolTip(
			Format("Clears the currently selected tissue (use '3D' option to clear "
						 "whole tissue or just the current slice)."));
	//xxxb	clearTissue->setFixedWidth(110);
	clearTissues = new QPushButton("All", this);
	clearTissues->setFixedWidth(30);
	clearTissues->setToolTip(
			Format("Clears all tissues (use '3D' option to clear the entire "
						 "segmentation or just the current slice)."));
	//xxxb	clearTissues->setFixedWidth(110);

	cb_addsub3d = new QCheckBox("3D", this);
	cb_addsub3d->setToolTip(
			Format("Apply add/remove actions in 3D or current slice only."));
	cb_addsuboverride = new QCheckBox("Override", this);
	cb_addsuboverride->setToolTip(
			Format("If override is off, Tissue voxels which are already assigned "
						 "will not be modified. Override allows to change these voxels, "
						 "unless they are locked."));
	cb_addsubconn = new QCheckBox("Connected", this);
	cb_addsubconn->setToolTip(
			Format("Only the connected image region is added/removed."));
	//	vboxtissueadder2->addStretch();
	pb_add = new QPushButton("+", this);
	pb_add->setToggleButton(true);
	pb_add->setToolTip(Format(
			"Adds next selected/picked Target image region to current tissue."));
	pb_sub = new QPushButton("-", this);
	pb_sub->setToggleButton(true);
	pb_sub->setToolTip(Format(
			"Removes next selected/picked Target image region from current tissue."));
	pb_addhold = new QPushButton("++", this);
	pb_addhold->setToggleButton(true);
	pb_addhold->setToolTip(
			Format("Adds selected/picked Target image regions to current tissue."));
	//	pb_stophold=new QPushButton(".",this);
	//	vboxtissueadder2->addWidget(pb_stophold);
	pb_subhold = new QPushButton("--", this);
	pb_subhold->setToggleButton(true);
	pb_subhold->setToolTip(Format(
			"Removes selected/picked Target image regions from current tissue."));
	/*	pb_add3D=new QPushButton("+3D",this);
		vboxtissueadder1->addWidget(pb_add3D);
		pb_addconn=new QPushButton("+Conn",this);
		vboxtissueadder2->addWidget(pb_addconn);*/
	pb_add->setFixedWidth(50);
	pb_sub->setFixedWidth(50);
	pb_addhold->setFixedWidth(50);
	pb_subhold->setFixedWidth(50);
	//	pb_stophold->setFixedWidth(50);
	/*	pb_add3D->setFixedWidth(50);
		pb_addconn->setFixedWidth(50);*/

	//	pb_work2tissue=new QPushButton("Target->Tissue",this);

	unsigned short slicenr = handler3D->get_activeslice() + 1;
	pb_first = new QPushButton("|<<", this);
	scb_slicenr = new QScrollBar(1, (int)handler3D->return_nrslices(), 1, 5, 1,
															 Qt::Horizontal, this);
	scb_slicenr->setFixedWidth(500);
	scb_slicenr->setValue(int(slicenr));
	pb_last = new QPushButton(">>|", this);
	sb_slicenr = new QSpinBox(1, (int)handler3D->return_nrslices(), 1, this);
	sb_slicenr->setValue(slicenr);
	lb_slicenr = new QLabel(
			QString(" of ") + QString::number((int)handler3D->return_nrslices()),
			this);
	//	lb_inactivewarning=new QLabel(QSimpleRichText(QString("  3D Inactive Slice!"),this->font()),this);
	lb_inactivewarning = new QLabel("  3D Inactive Slice!  ", this);
	lb_inactivewarning->setPaletteForegroundColor(QColor(255, 0, 0));
	lb_inactivewarning->setPaletteBackgroundColor(QColor(0, 255, 0));

	lb_slicethick = new QLabel("Slice Thickness (mm): ", this);
	le_slicethick =
			new QLineEdit(QString::number(handler3D->get_slicethickness()), this);
	le_slicethick->setFixedWidth(80);

	threshold_widget =
			new thresh_widget(handler3D, this, "new window",
												Qt::WDestructiveClose | Qt::WResizeNoErase);
	tabwidgets.push_back(threshold_widget);
	hyst_widget = new hyster_widget(handler3D, this, "new window",
																	Qt::WDestructiveClose | Qt::WResizeNoErase);
	tabwidgets.push_back(hyst_widget);
	lw_widget = new livewire_widget(handler3D, this, "new window",
																	Qt::WDestructiveClose | Qt::WResizeNoErase);
	tabwidgets.push_back(lw_widget);
	iftrg_widget = new IFTrg_widget(handler3D, this, "new window",
																	Qt::WDestructiveClose | Qt::WResizeNoErase);
	tabwidgets.push_back(iftrg_widget);
	FMF_widget =
			new FastMarchFuzzy_widget(handler3D, this, "new window",
																Qt::WDestructiveClose | Qt::WResizeNoErase);
	tabwidgets.push_back(FMF_widget);
	wshed_widget =
			new watershed_widget(handler3D, this, "new window",
													 Qt::WDestructiveClose | Qt::WResizeNoErase);
	tabwidgets.push_back(wshed_widget);
	OutlineCorrect_widget =
			new OutlineCorr_widget(handler3D, this, "new window",
														 Qt::WDestructiveClose | Qt::WResizeNoErase);
	tabwidgets.push_back(OutlineCorrect_widget);
	interpolwidget =
			new interpol_widget(handler3D, this, "new window",
													Qt::WDestructiveClose | Qt::WResizeNoErase);
	tabwidgets.push_back(interpolwidget);
	smoothing_widget =
			new smooth_widget(handler3D, this, "new window",
												Qt::WDestructiveClose | Qt::WResizeNoErase);
	tabwidgets.push_back(smoothing_widget);
	morph_widget = new morpho_widget(handler3D, this, "new window",
																	 Qt::WDestructiveClose | Qt::WResizeNoErase);
	tabwidgets.push_back(morph_widget);
	edge_widg = new edge_widget(handler3D, this, "new window",
															Qt::WDestructiveClose | Qt::WResizeNoErase);
	tabwidgets.push_back(edge_widg);
	feature_widget =
			new featurewidget(handler3D, this, "new window",
												Qt::WDestructiveClose | Qt::WResizeNoErase);
	tabwidgets.push_back(feature_widget);
	measurement_widget =
			new measure_widget(handler3D, this, "new window",
												 Qt::WDestructiveClose | Qt::WResizeNoErase);
	tabwidgets.push_back(measurement_widget);
	vesselextr_widget =
			new vessel_widget(handler3D, this, "new window",
												Qt::WDestructiveClose | Qt::WResizeNoErase);
	tabwidgets.push_back(vesselextr_widget);
	pickerwidget = new picker_widget(handler3D, this, "new window",
																	 Qt::WDestructiveClose | Qt::WResizeNoErase);
	tabwidgets.push_back(pickerwidget);
	transfrmWidget =
			new TransformWidget(handler3D, this, "new window",
													Qt::WDestructiveClose | Qt::WResizeNoErase);
	tabwidgets.push_back(transfrmWidget);

	boost::filesystem::path this_exe(argv[0]);

	bool ok = iseg::plugin::LoadPlugins(this_exe.parent_path().string());
	assert(ok == true);

	auto addons = iseg::plugin::CAddonRegistry::GetAllAddons();
	for (auto a : addons) {
		a->SetSliceHandler(handler3D);
		tabwidgets.push_back(a->CreateWidget(
				this, "new window", Qt::WDestructiveClose | Qt::WResizeNoErase));
	}

	nrtabbuttons = (unsigned short)tabwidgets.size();
	pb_tab.resize(nrtabbuttons);
	showpb_tab.resize(nrtabbuttons);
	showtab_action.resize(nrtabbuttons);

	//pb_tab1=new QPushButton(QIconSet(qPixmapFromMimeSource("G:\\docs&settings\\Documents and Settings\\neufeld\\My Documents\\My Pictures\\iSeg icons\\thresholding.png")),"Thresh",this);
	for (unsigned short i = 0; i < nrtabbuttons; i++) {
		showpb_tab[i] = true;

		pb_tab[i] = new QPushButton(this);
		pb_tab[i]->setToggleButton(true);
		pb_tab[i]->setStyleSheet("text-align: left");
	}

	methodTab = new Q3WidgetStack(this);
	methodTab->setFrameStyle(Q3Frame::Box | Q3Frame::Sunken);
	methodTab->setLineWidth(2);

	for (size_t i = 0; i < tabwidgets.size(); i++) {
		methodTab->addWidget(tabwidgets[i]);
		//		methodTab->addTab(tabwidgets[i],tabwidgets[i]->GetName().c_str());
	}

	methodTab->setMargin(10);

	{
		unsigned short posi = 0;
		while (posi < nrtabbuttons && !showpb_tab[posi])
			posi++;
		if (posi < nrtabbuttons)
			methodTab->raiseWidget(tabwidgets[posi]);
	}

	tab_changed((QWidget1 *)methodTab->visibleWidget());

	//	tab_old=(QWidget1 *)methodTab->visibleWidget();
	updateTabvisibility();

	int height_max = 0;
	QSize qs; //,qsmax;
						//	qsmax.setHeight(0);
						//	qsmax.setWidth(0);
	for (size_t i = 0; i < tabwidgets.size(); i++) {
		qs = tabwidgets[i]->sizeHint();
		//		qsmax.setWidth(max(qs.width(),qsmax.width()));
		//		qsmax.setHeight(max(qs.height(),qsmax.height()));F
		height_max = max(height_max, qs.height());
	}
	height_max += 65;
	for (size_t i = 0; i < tabwidgets.size(); i++) {
		//		methodTab->page(i)->setFixedSize(qsmax);
		//		tabwidgets[i]->setFixedHeight(height_max);
		//		tabwidgets[i]->setFixedWidth(500);
	}
	//	methodTab->setFixedSize(qsmax);
	for (unsigned short i = 0; i < nrtabbuttons; i++) {
		//		pb_tab[i]->setFixedHeight(height_max*2/nrtabbuttons);
	}

	scalewidget = new ScaleWork(handler3D, m_picpath, this, "new window",
															Qt::WDestructiveClose | Qt::WResizeNoErase);
	imagemathwidget = new ImageMath(handler3D, this, "new window",
																	Qt::WDestructiveClose | Qt::WResizeNoErase);
	imageoverlaywidget =
			new ImageOverlay(handler3D, this, "new window",
											 Qt::WDestructiveClose | Qt::WResizeNoErase);

	bitstack_widget = new bits_stack(handler3D, this, "new window",
																	 Qt::WDestructiveClose | Qt::WResizeNoErase);
	overlay_widget =
			new extoverlay_widget(handler3D, this, "new window",
														Qt::WDestructiveClose | Qt::WResizeNoErase);
	m_MultiDataset_widget =
			new MultiDataset_widget(handler3D, this, "multi dataset window",
															Qt::WDestructiveClose | Qt::WResizeNoErase);
	int height_max1 = height_max;
	//	methodTab->setFixedHeight(height_max1);
	//	bitstack_widget->setFixedHeight(height_max1);

	m_lb_notes = new QLabel("Notes:", this);
	m_notes = new QTextEdit(this);
	m_notes->clear();

	xsliceshower = ysliceshower = NULL;
	SV3D = NULL;
	SV3Dbmp = NULL;
	VV3D = NULL;
	VV3Dbmp = NULL;

	if (handler3D->return_startslice() >= slicenr ||
			handler3D->return_endslice() + 1 <= slicenr) {
		lb_inactivewarning->setText(QString("   3D Inactive Slice!   "));
		lb_inactivewarning->setPaletteBackgroundColor(QColor(0, 255, 0));
	}
	else {
		lb_inactivewarning->setText(QString(" "));
		lb_inactivewarning->setPaletteBackgroundColor(
				this->paletteBackgroundColor());
	}

	QWidget *hbox2w = new QWidget;
	setCentralWidget(hbox2w);

	QWidget *vbox2w = new QWidget;
	QWidget *rightvboxw = new QWidget;
	QWidget *hbox1w = new QWidget;
	QWidget *hboxslicew = new QWidget;
	QWidget *hboxslicenrw = new QWidget;
	QWidget *hboxslicethickw = new QWidget;
	vboxbmpw = new QWidget;
	vboxworkw = new QWidget;
	QWidget *vbox1w = new QWidget;
	QWidget *hboxbmpw = new QWidget;
	QWidget *hboxworkw = new QWidget;
	QWidget *hboxbmp1w = new QWidget;
	QWidget *hboxwork1w = new QWidget;
	QWidget *vboxtissuew = new QWidget;
	QWidget *hboxlockw = new QWidget;
	QWidget *hboxtissueadderw = new QWidget;
	QWidget *vboxtissueadder1w = new QWidget;
	QWidget *vboxtissueadder2w = new QWidget;
	QWidget *hboxtabsw = new QWidget;
	QWidget *vboxtabs1w = new QWidget;
	QWidget *vboxtabs2w = new QWidget;

	QHBoxLayout *hboxbmp1 = new QHBoxLayout;
	hboxbmp1->setSpacing(0);
	hboxbmp1->setMargin(0);
	hboxbmp1->addWidget(lb_contrastbmp);
	hboxbmp1->addWidget(le_contrastbmp_val);
	hboxbmp1->addWidget(lb_contrastbmp_val);
	hboxbmp1->addWidget(sl_contrastbmp);
	hboxbmp1->addWidget(lb_brightnessbmp);
	hboxbmp1->addWidget(le_brightnessbmp_val);
	hboxbmp1->addWidget(lb_brightnessbmp_val);
	hboxbmp1->addWidget(sl_brightnessbmp);
	hboxbmp1w->setLayout(hboxbmp1);

	QHBoxLayout *hboxbmp = new QHBoxLayout;
	hboxbmp->setSpacing(0);
	hboxbmp->setMargin(0);
	hboxbmp->addWidget(cb_bmptissuevisible);
	hboxbmp->addWidget(cb_bmpcrosshairvisible);
	hboxbmp->addWidget(cb_bmpoutlinevisible);
	hboxbmpw->setLayout(hboxbmp);

	QVBoxLayout *vboxbmp = new QVBoxLayout;
	vboxbmp->setSpacing(0);
	vboxbmp->setMargin(0);
	vboxbmp->addWidget(lb_source);
	vboxbmp->addWidget(hboxbmp1w);
	vboxbmp->addWidget(bmp_scroller);
	vboxbmp->addWidget(hboxbmpw);
	vboxbmpw->setLayout(vboxbmp);

	QVBoxLayout *vbox1 = new QVBoxLayout;
	vbox1->setSpacing(0);
	vbox1->setMargin(0);
	vbox1->addWidget(toworkBtn);
	vbox1->addWidget(tobmpBtn);
	vbox1->addWidget(swapBtn);
	vbox1->addWidget(swapAllBtn);
	vbox1w->setLayout(vbox1);

	QHBoxLayout *hboxwork1 = new QHBoxLayout;
	hboxwork1->setSpacing(0);
	hboxwork1->setMargin(0);
	hboxwork1->addWidget(lb_contrastwork);
	hboxwork1->addWidget(le_contrastwork_val);
	hboxwork1->addWidget(lb_contrastwork_val);
	hboxwork1->addWidget(sl_contrastwork);
	hboxwork1->addWidget(lb_brightnesswork);
	hboxwork1->addWidget(le_brightnesswork_val);
	hboxwork1->addWidget(lb_brightnesswork_val);
	hboxwork1->addWidget(sl_brightnesswork);
	hboxwork1w->setLayout(hboxwork1);

	QHBoxLayout *hboxwork = new QHBoxLayout;
	hboxwork->setSpacing(0);
	hboxwork->setMargin(0);
	hboxwork->addWidget(cb_worktissuevisible);
	hboxwork->addWidget(cb_workcrosshairvisible);
	hboxwork->addWidget(cb_workpicturevisible);
	hboxworkw->setLayout(hboxwork);

	//	vboxworkw->setSizePolicy(QSizePolicy::Ignored);
	QVBoxLayout *vboxwork = new QVBoxLayout;
	vboxwork->setSpacing(0);
	vboxwork->setMargin(0);
	vboxwork->addWidget(lb_target);
	vboxwork->addWidget(hboxwork1w);
	vboxwork->addWidget(work_scroller);
	vboxwork->addWidget(hboxworkw);
	vboxworkw->setLayout(vboxwork);

	//	vboxbmpw->setSizePolicy(QSizePolicy::Ignore,QSizePolicy::Ignore);
	//	vboxworkw->setSizePolicy(QSizePolicy::Ignore,QSizePolicy::Ignore);

	QHBoxLayout *hbox1 = new QHBoxLayout;
	hbox1->setSpacing(0);
	hbox1->setMargin(0);
	hbox1->addWidget(vboxbmpw);
	hbox1->addWidget(vbox1w);
	hbox1->addWidget(vboxworkw);
	hbox1w->setLayout(hbox1);

	QHBoxLayout *hboxslicethick = new QHBoxLayout;
	hboxslicethick->setSpacing(0);
	hboxslicethick->setMargin(0);
	hboxslicethick->addWidget(lb_slicethick);
	hboxslicethick->addWidget(le_slicethick);
	hboxslicethickw->setLayout(hboxslicethick);

	QHBoxLayout *hboxslicenr = new QHBoxLayout;
	hboxslicenr->setSpacing(0);
	hboxslicenr->setMargin(0);
	hboxslicenr->addWidget(pb_first);
	hboxslicenr->addWidget(scb_slicenr);
	hboxslicenr->addWidget(pb_last);
	hboxslicenr->addWidget(sb_slicenr);
	hboxslicenr->addWidget(lb_slicenr);
	hboxslicenr->addWidget(lb_inactivewarning);
	hboxslicenrw->setLayout(hboxslicenr);

	QHBoxLayout *hboxslice = new QHBoxLayout;
	hboxslice->setSpacing(0);
	hboxslice->setMargin(0);
	hboxslice->addWidget(hboxslicenrw);
	hboxslice->addStretch();
	hboxslice->addWidget(hboxslicethickw);
	hboxslicew->setLayout(hboxslice);

	auto add_widget_filter = [this](QVBoxLayout *vbox, QPushButton *pb,
																	unsigned short i) {
#ifdef PLUGIN_VESSEL_WIDGET
		vbox->addWidget(pb);
#else
		if (tabwidgets[i] != vesselextr_widget) {
			vbox->addWidget(pb);
		}
#endif
	};

	QVBoxLayout *vboxtabs1 = new QVBoxLayout;
	vboxtabs1->setSpacing(0);
	vboxtabs1->setMargin(0);
	for (unsigned short i = 0; i < (nrtabbuttons + 1) / 2; i++) {
		add_widget_filter(vboxtabs1, pb_tab[i], i);
	}
	vboxtabs1w->setLayout(vboxtabs1);
	QVBoxLayout *vboxtabs2 = new QVBoxLayout;
	vboxtabs2->setSpacing(0);
	vboxtabs2->setMargin(0);
	for (unsigned short i = (nrtabbuttons + 1) / 2; i < nrtabbuttons; i++) {
		add_widget_filter(vboxtabs2, pb_tab[i], i);
	}
	vboxtabs2w->setLayout(vboxtabs2);

	{
		int widthsuggest = vboxtabs1w->sizeHint().width();
		if (vboxtabs2w->sizeHint().width() > widthsuggest)
			widthsuggest = vboxtabs2w->sizeHint().width();
		vboxtabs1w->setFixedWidth(widthsuggest);
		vboxtabs2w->setFixedWidth(widthsuggest);
		vboxtabs1w->setMaximumHeight(height_max1);
		vboxtabs2w->setMaximumHeight(height_max1);
	}

	QHBoxLayout *hboxtabs = new QHBoxLayout;
	hboxtabs->setSpacing(0);
	hboxtabs->setMargin(0);
	hboxtabs->addWidget(vboxtabs1w);
	hboxtabs->addWidget(vboxtabs2w);
	hboxtabsw->setLayout(hboxtabs);
	hboxtabsw->setFixedWidth(hboxtabs->sizeHint().width());

	QDockWidget *tabswdock = new QDockWidget(tr("Methods"), this);
	style_dockwidget(tabswdock);
	tabswdock->setObjectName("Methods");
	tabswdock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
	tabswdock->setWidget(hboxtabsw);
	addDockWidget(Qt::BottomDockWidgetArea, tabswdock);

	QDockWidget *methodTabdock = new QDockWidget(tr("Parameters"), this);
	style_dockwidget(methodTabdock);
	methodTabdock->setObjectName("Parameters");
	methodTabdock->setAllowedAreas(Qt::TopDockWidgetArea |
																 Qt::BottomDockWidgetArea);
	//	Q3ScrollView *tab_scroller=new Q3ScrollView(this);xxxa
	//	tab_scroller->addChild(methodTab);
	//	methodTabdock->setWidget(tab_scroller);
	methodTabdock->setWidget(methodTab);
	addDockWidget(Qt::BottomDockWidgetArea, methodTabdock);

	QVBoxLayout *vboxnotes = new QVBoxLayout;
	vboxnotes->setSpacing(0);
	vboxnotes->setMargin(0);

	QDockWidget *notesdock = new QDockWidget(tr("Notes"), this);
	style_dockwidget(notesdock);
	notesdock->setObjectName("Notes");
	notesdock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
	notesdock->setWidget(m_notes);
	addDockWidget(Qt::BottomDockWidgetArea, notesdock);

	QDockWidget *bitstackdock = new QDockWidget(tr("Img Clipboard"), this);
	style_dockwidget(bitstackdock);
	bitstackdock->setObjectName("Clipboard");
	bitstackdock->setAllowedAreas(Qt::TopDockWidgetArea |
																Qt::BottomDockWidgetArea);
	bitstackdock->setWidget(bitstack_widget);
	addDockWidget(Qt::BottomDockWidgetArea, bitstackdock);

	QDockWidget *multiDatasetDock = new QDockWidget(tr("Multi Dataset"), this);
	style_dockwidget(multiDatasetDock);
	multiDatasetDock->setObjectName("Multi Dataset");
	multiDatasetDock->setAllowedAreas(Qt::TopDockWidgetArea |
																		Qt::BottomDockWidgetArea);
	multiDatasetDock->setWidget(m_MultiDataset_widget);
	addDockWidget(Qt::BottomDockWidgetArea, multiDatasetDock);

	QDockWidget *overlaydock = new QDockWidget(tr("Overlay"), this);
	style_dockwidget(overlaydock);
	overlaydock->setObjectName("Overlay");
	overlaydock->setAllowedAreas(Qt::TopDockWidgetArea |
															 Qt::BottomDockWidgetArea);
	overlaydock->setWidget(overlay_widget);
	addDockWidget(Qt::BottomDockWidgetArea, overlaydock);

	QVBoxLayout *vbox2 = new QVBoxLayout;
	vbox2->setSpacing(0);
	vbox2->setMargin(0);
	vbox2->addWidget(hbox1w);
	vbox2->addWidget(hboxslicew);
	vbox2w->setLayout(vbox2);

	QHBoxLayout *hboxlock = new QHBoxLayout;
	hboxlock->setSpacing(0);
	hboxlock->setMargin(0);
	hboxlock->addWidget(cb_tissuelock);
	hboxlock->addWidget(lockTissues);
	hboxlock->addStretch();
	hboxlockw->setLayout(hboxlock);

	QVBoxLayout *vboxtissue = new QVBoxLayout;
	vboxtissue->setSpacing(0);
	vboxtissue->setMargin(0);
	vboxtissue->addWidget(tissueFilter);
	vboxtissue->addWidget(tissueTreeWidget);
	vboxtissue->addWidget(hboxlockw);
	vboxtissue->addWidget(addTissue);
	vboxtissue->addWidget(addFolder);
	vboxtissue->addWidget(modifyTissueFolder);

	QHBoxLayout *hboxtissueremove = new QHBoxLayout;
	hboxtissueremove->addWidget(removeTissueFolder);
	hboxtissueremove->addWidget(removeTissueFolderAll);
	vboxtissue->addLayout(hboxtissueremove);
	vboxtissue->addWidget(tissue3Dopt);
	QHBoxLayout *hboxtissueget = new QHBoxLayout;
	hboxtissueget->addWidget(getTissue);
	hboxtissueget->addWidget(getTissueAll);
	vboxtissue->addLayout(hboxtissueget);
	QHBoxLayout *hboxtissueclear = new QHBoxLayout;
	hboxtissueclear->addWidget(clearTissue);
	hboxtissueclear->addWidget(clearTissues);
	vboxtissue->addLayout(hboxtissueclear);
	vboxtissuew->setLayout(vboxtissue);
	//xxxb	vboxtissuew->setFixedHeight(vboxtissue->sizeHint().height());

	QVBoxLayout *vboxtissueadder1 = new QVBoxLayout;
	vboxtissueadder1->setSpacing(0);
	vboxtissueadder1->setMargin(0);
	vboxtissueadder1->addWidget(cb_addsub3d);
	vboxtissueadder1->addWidget(cb_addsuboverride);
	vboxtissueadder1->addWidget(pb_add);
	vboxtissueadder1->addWidget(pb_sub);
	vboxtissueadder1w->setLayout(vboxtissueadder1);
	vboxtissueadder1w->setFixedHeight(vboxtissueadder1->sizeHint().height());

	QVBoxLayout *vboxtissueadder2 = new QVBoxLayout;
	vboxtissueadder2->setSpacing(0);
	vboxtissueadder2->setMargin(0);
	vboxtissueadder2->addWidget(cb_addsubconn);
	vboxtissueadder2->addWidget(new QLabel(" ", this));
	vboxtissueadder2->addWidget(pb_addhold);
	vboxtissueadder2->addWidget(pb_subhold);
	vboxtissueadder2w->setLayout(vboxtissueadder2);
	vboxtissueadder2w->setFixedHeight(vboxtissueadder1->sizeHint().height());

	QHBoxLayout *hboxtissueadder = new QHBoxLayout;
	hboxtissueadder->setSpacing(0);
	hboxtissueadder->setMargin(0);
	hboxtissueadder->addWidget(vboxtissueadder1w);
	hboxtissueadder->addWidget(vboxtissueadder2w);
	hboxtissueadderw->setLayout(hboxtissueadder);
	hboxtissueadderw->setFixedHeight(hboxtissueadder->sizeHint().height());
	//xxxb	hboxtissueadderw->setFixedWidth(vboxtissue->sizeHint().width());

	QDockWidget *zoomdock = new QDockWidget(tr("Zoom"), this);
	style_dockwidget(zoomdock);
	zoomdock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	zoomdock->setWidget(zoom_widget);
	addDockWidget(Qt::RightDockWidgetArea, zoomdock);

	QDockWidget *tissuewdock = new QDockWidget(tr("Tissues"), this);
	style_dockwidget(tissuewdock);
	tissuewdock->setObjectName("Tissues");
	tissuewdock->setAllowedAreas(Qt::LeftDockWidgetArea |
															 Qt::RightDockWidgetArea);
	tissuewdock->setWidget(vboxtissuew);
	addDockWidget(Qt::RightDockWidgetArea, tissuewdock);

	QDockWidget *tissuehierarchydock =
			new QDockWidget(tr("Tissue Hierarchy"), this);
	style_dockwidget(tissuehierarchydock);
	tissuehierarchydock->setToolTip(
			Format("The tissue hierarchy allows to group and organize complex "
						 "segmentations into a hierarchy."));
	tissuehierarchydock->setObjectName("Tissue Hierarchy");
	tissuehierarchydock->setAllowedAreas(Qt::LeftDockWidgetArea |
																			 Qt::RightDockWidgetArea);
	tissuehierarchydock->setWidget(tissueHierarchyWidget);
	addDockWidget(Qt::RightDockWidgetArea, tissuehierarchydock);

	QDockWidget *tissueadddock = new QDockWidget(tr("Adder"), this);
	style_dockwidget(tissueadddock);
	tissueadddock->setToolTip(Format("The tissue adder provides functionality "
																	 "to add/remove an object selected/picked in "
																	 "the Target to/from the selected tissue."
																	 "Note: Only one tissue can be selected."));
	tissueadddock->setObjectName("Adder");
	tissueadddock->setAllowedAreas(Qt::LeftDockWidgetArea |
																 Qt::RightDockWidgetArea);
	tissueadddock->setWidget(hboxtissueadderw);
	addDockWidget(Qt::RightDockWidgetArea, tissueadddock);

	hboxslice->addStretch();
	hboxslice->addStretch();

	QHBoxLayout *hbox2 = new QHBoxLayout;
	hbox2->setSpacing(0);
	hbox2->setMargin(0);
	hbox2->addWidget(vbox2w);
	hbox2w->setLayout(hbox2);

	//xxxxxxxxxxxxxxxxx

	menubar = menuBar();

	//file = menuBar()->addMenu(tr("&File"));
	file = new MenuWTT();
	file->setTitle(tr("&File"));
	menuBar()->addMenu(file);
	if (!m_editingmode) {
		file->insertItem(
				QIcon(m_picpath.absFilePath(QString("filenew.png")).ascii()), "&New...",
				this, SLOT(execute_new()));
		loadmenu = new Q3PopupMenu(this, "loadmenu");
		loadmenu->insertItem("Open .dcm...", this, SLOT(execute_loaddicom()));
		loadmenu->insertItem("Open .bmp...", this, SLOT(execute_loadbmp()));
		loadmenu->insertItem("Open .png...", this, SLOT(execute_loadpng()));
		//loadmenu->insertItem( "Open .jpg...", this,  SLOT(execute_loadjpg()));
		loadmenu->insertItem("Open .raw...", this, SLOT(execute_loadraw()));
		loadmenu->insertItem("Open .mhd...", this, SLOT(execute_loadmhd()));
		loadmenu->insertItem("Open .avw...", this, SLOT(execute_loadavw()));
		loadmenu->insertItem("Open .vti/.vtk...", this, SLOT(execute_loadvtk()));
		loadmenu->insertItem("Open NIfTI...", this, SLOT(execute_loadnifti()));
#ifndef NORTDOSESUPPORT
		loadmenu->insertItem("Open RTdose...", this, SLOT(execute_loadrtdose()));
#endif
		file->insertItem("&Open", loadmenu);
	}
	reloadmenu = new Q3PopupMenu(
			this, "reloadmenu"); //xxxa add reload function for s4llink data later
	reloadmenu->insertItem("Reopen .dc&m...", this, SLOT(execute_reloaddicom()));
	reloadmenu->insertItem("Reopen .&bmp...", this, SLOT(execute_reloadbmp()));
	reloadmenu->insertItem("Reopen .raw...", this, SLOT(execute_reloadraw()));
	reloadmenu->insertItem("Reopen .mhd...", this, SLOT(execute_reloadmhd()));
	reloadmenu->insertItem("Reopen .avw...", this, SLOT(execute_reloadavw()));
	reloadmenu->insertItem("Reopen .vti/.vtk...", this,
												 SLOT(execute_reloadvtk()));
	reloadmenu->insertItem("Reopen NIfTI...", this, SLOT(execute_reloadnifti()));
#ifndef NORTDOSESUPPORT
	reloadmenu->insertItem("Reopen RTdose...", this,
												 SLOT(execute_reloadrtdose()));
#endif
	file->insertItem("&Reopen", reloadmenu);
#ifndef NORTSTRUCTSUPPORT

	if (!m_editingmode) {
		openS4LLinkPos = file->actions().size();
		QAction *importS4LLink = file->addAction("Import S4L link (h5)...");
		connect(importS4LLink, SIGNAL(triggered()), this,
						SLOT(execute_loads4llivelink()));
		importS4LLink->setToolTip("Loads a Sim4Life live link file (the h5 file, "
															"which is part of an iSEG project).");
		importS4LLink->setEnabled(true);
	}

	importSurfacePos = file->actions().size();
	QAction *importSurface = file->addAction("Import Surface...");
	connect(importSurface, SIGNAL(triggered()), this,
					SLOT(execute_loadsurface()));
	importSurface->setToolTip("Some data must be opened first to load the "
														"surface on top of the existing project.");
	importSurface->setEnabled(true);

	importRTstructPos = file->actions().size();
	QAction *importRTAction = file->addAction("Import RTstruct...");
	connect(importRTAction, SIGNAL(triggered()), this,
					SLOT(execute_loadrtstruct()));
	importRTAction->setToolTip(
			"Some data must be opened first to import its RTStruct file");
	importRTAction->setEnabled(false);
#endif
	file->insertItem("&Export Image(s)...", this, SLOT(execute_saveimg()));
	file->insertItem("Export &Contour...", this, SLOT(execute_saveContours()));
	exportmenu = new Q3PopupMenu(this, "exportmenu");
	exportmenu->insertItem("Export &Labelfield...(am)", this,
												 SLOT(execute_exportlabelfield()));
	exportmenu->insertItem("Export vtk-ascii...(vti/vtk)", this,
												 SLOT(execute_exportvtkascii()));
	exportmenu->insertItem("Export vtk-binary...(vti/vtk)", this,
												 SLOT(execute_exportvtkbinary()));
	exportmenu->insertItem("Export vtk-compressed-ascii...(vti)", this,
												 SLOT(execute_exportvtkcompressedascii()));
	exportmenu->insertItem("Export vtk-compressed-binary...(vti)", this,
												 SLOT(execute_exportvtkcompressedbinary()));
	exportmenu->insertItem("Export Matlab...(mat)", this,
												 SLOT(execute_exportmat()));
	exportmenu->insertItem("Export hdf...(h5)", this, SLOT(execute_exporthdf()));
	exportmenu->insertItem("Export xml-extent index...(xml)", this,
												 SLOT(execute_exportxmlregionextent()));
	exportmenu->insertItem("Export tissue index...(txt)", this,
												 SLOT(execute_exporttissueindex()));
	file->insertItem("Export Tissue Distr.", exportmenu);
	file->insertSeparator();

	if (!m_editingmode)
		file->insertItem("Save &Project as...", this, SLOT(execute_saveprojas()));
	else
		file->insertItem("Save &Project-Copy as...", this,
										 SLOT(execute_savecopyas()));
	file->insertItem(
			QIcon(m_picpath.absFilePath(QString("filesave.png")).ascii()),
			"Save Pro&ject", this, SLOT(execute_saveproj()), QKeySequence("Ctrl+S"));
	file->insertItem("Save Active Slices...", this,
									 SLOT(execute_saveactiveslicesas()));
	if (!m_editingmode)
		file->insertItem(
				QIcon(m_picpath.absFilePath(QString("fileopen.png")).ascii()),
				"Open P&roject...", this, SLOT(execute_loadproj()));
	file->insertSeparator();
	file->insertItem("Save &Tissuelist...", this, SLOT(execute_savetissues()));
	file->insertItem("Open T&issuelist...", this, SLOT(execute_loadtissues()));
	file->insertItem("Set Tissuelist as Default", this,
									 SLOT(execute_settissuesasdef()));
	file->insertItem("Remove Default Tissuelist", this,
									 SLOT(execute_removedeftissues()));
#ifndef NOSURFACEGENERATIONTOOLSUPPORT
	file->insertItem("Export Surface Generation Tool XML...", this,
									 SLOT(execute_exportsurfacegenerationtoolxml()));
#endif
	file->insertSeparator();
	if (!m_editingmode) {
		m_loadprojfilename.lpf1nr =
				file->insertItem("", this, SLOT(execute_loadproj1()));
		m_loadprojfilename.lpf2nr =
				file->insertItem("", this, SLOT(execute_loadproj2()));
		m_loadprojfilename.lpf3nr =
				file->insertItem("", this, SLOT(execute_loadproj3()));
		m_loadprojfilename.lpf4nr =
				file->insertItem("", this, SLOT(execute_loadproj4()));
		m_loadprojfilename.separatornr = file->insertSeparator();
		file->setItemVisible(m_loadprojfilename.lpf1nr, false);
		file->setItemVisible(m_loadprojfilename.lpf2nr, false);
		file->setItemVisible(m_loadprojfilename.lpf3nr, false);
		file->setItemVisible(m_loadprojfilename.lpf4nr, false);
		file->setItemVisible(m_loadprojfilename.separatornr, false);
	}
	//	file->insertItem( "E&xit", qApp,  SLOT(quit()), CTRL+Key_Q );
	file->insertItem("E&xit", this, SLOT(close()), QKeySequence("Ctrl+Q"));
	imagemenu = menuBar()->addMenu(tr("&Image"));
	imagemenu->insertItem("&Pixelsize...", this, SLOT(execute_pixelsize()));
	imagemenu->insertItem("Offset...", this, SLOT(execute_displacement()));
	imagemenu->insertItem("Rotation...", this, SLOT(execute_rotation()));
	//	imagemenu->insertItem( "Resize...", this,  SLOT(execute_resize()));
	if (!m_editingmode) {
		imagemenu->insertItem("Pad...", this, SLOT(execute_pad()));
		imagemenu->insertItem("Crop...", this, SLOT(execute_crop()));
	}
	imagemenu->insertItem(
			QIcon(m_picpath.absFilePath(QString("histo.png")).ascii()),
			"&Histogram...", this, SLOT(execute_histo()));
	imagemenu->insertItem("&Contr./Bright. ...", this, SLOT(execute_scale()));
	imagemenu->insertItem("&Image Math. ...", this, SLOT(execute_imagemath()));
	imagemenu->insertItem("Unwrap", this, SLOT(execute_unwrap()));
	imagemenu->insertItem("Overlay...", this, SLOT(execute_overlay()));
	imagemenu->insertSeparator();
	imagemenu->insertItem("&x Sliced", this, SLOT(execute_xslice()));
	imagemenu->insertItem("&y Sliced", this, SLOT(execute_yslice()));
	imagemenu->insertItem("3D surface view", this,
												SLOT(execute_3Dsurfaceviewer()));
	imagemenu->insertItem("3D isosurface view", this,
												SLOT(execute_3Dsurfaceviewerbmp()));
	imagemenu->insertItem("3D volume view source", this,
												SLOT(execute_3Dvolumeviewerbmp()));
	imagemenu->insertItem("3D volume view tissue", this,
												SLOT(execute_3Dvolumeviewertissue()));
	if (!m_editingmode) {
		//xxxa;
		imagemenu->insertSeparator();
		imagemenu->insertItem("Swap xy", this, SLOT(execute_swapxy()));
		imagemenu->insertItem("Swap xz", this, SLOT(execute_swapxz()));
		imagemenu->insertItem("Swap yz", this, SLOT(execute_swapyz()));
	}

	editmenu = menuBar()->addMenu(tr("E&dit"));
	undonr = editmenu->insertItem(
			QIcon(m_picpath.absFilePath(QString("undo.png")).ascii()), "&Undo", this,
			SLOT(execute_undo()));
	redonr = editmenu->insertItem(
			QIcon(m_picpath.absFilePath(QString("redo.png")).ascii()), "Redo", this,
			SLOT(execute_redo()));
	editmenu->insertSeparator();
	editmenu->insertItem("&Configure Undo...", this, SLOT(execute_undoconf()));
	editmenu->insertItem("&Active Slices...", this,
											 SLOT(execute_activeslicesconf()));
	editmenu->setItemEnabled(undonr, false);
	editmenu->setItemEnabled(redonr, false);
	editmenu->insertItem("&Settings...", this, SLOT(execute_settings()));

	viewmenu = menuBar()->addMenu(tr("&View"));
	hidemenu = new Q3PopupMenu(this, "hidemenu");
	hidesubmenu = new Q3PopupMenu(this, "hidesubmenu");
	hidemenu->addAction(tabswdock->toggleViewAction());
	hidemenu->addAction(methodTabdock->toggleViewAction());
	hidemenu->addAction(notesdock->toggleViewAction());
	hidemenu->addAction(bitstackdock->toggleViewAction());
	hidemenu->addAction(zoomdock->toggleViewAction());
	hidemenu->addAction(tissuewdock->toggleViewAction());
	hidemenu->addAction(tissueadddock->toggleViewAction());
	hidemenu->addAction(tissuehierarchydock->toggleViewAction());
	hidemenu->addAction(overlaydock->toggleViewAction());
	hidemenu->addAction(multiDatasetDock->toggleViewAction());

	hidecontrastbright = new Q3Action("Contr./Bright.", 0, this);
	hidecontrastbright->setToggleAction(true);
	hidecontrastbright->setOn(true);
	connect(hidecontrastbright, SIGNAL(toggled(bool)), this,
					SLOT(execute_hidecontrastbright(bool)));
	hidecontrastbright->addTo(hidemenu);
	hidesource = new Q3Action("Source", 0, this);
	hidesource->setToggleAction(true);
	hidesource->setOn(true);
	connect(hidesource, SIGNAL(toggled(bool)), this,
					SLOT(execute_hidesource(bool)));
	hidesource->addTo(hidemenu);
	hidetarget = new Q3Action("Target", 0, this);
	hidetarget->setToggleAction(true);
	hidetarget->setOn(true);
	connect(hidetarget, SIGNAL(toggled(bool)), this,
					SLOT(execute_hidetarget(bool)));
	hidetarget->addTo(hidemenu);
	hidecopyswap = new Q3Action("Copy/Swap", 0, this);
	hidecopyswap->setToggleAction(true);
	hidecopyswap->setOn(true);
	connect(hidecopyswap, SIGNAL(toggled(bool)), this,
					SLOT(execute_hidecopyswap(bool)));
	hidecopyswap->addTo(hidemenu);
	for (unsigned short i = 0; i < nrtabbuttons; i++) {
		//showtab_action[i]=new Q3Action((std::string("Show ")+tabwidgets[i]->GetName()).c_str(),0,this);
		showtab_action[i] = new Q3Action(tabwidgets[i]->GetName().c_str(), 0, this);
		showtab_action[i]->setToggleAction(true);
		showtab_action[i]->setOn(showpb_tab[i]);
		connect(showtab_action[i], SIGNAL(toggled(bool)), this,
						SLOT(execute_showtabtoggled(bool)));
		showtab_action[i]->addTo(hidesubmenu);
	}
	hideparameters = new Q3Action("Simplified", 0, this);
	hideparameters->setToggleAction(true);
	hideparameters->setOn(QWidget1::get_hideparams());
	connect(hideparameters, SIGNAL(toggled(bool)), this,
					SLOT(execute_hideparameters(bool)));
	hidesubmenu->insertSeparator();
	hideparameters->addTo(hidesubmenu);
	viewmenu->insertItem("&Toolbars", hidemenu);
	viewmenu->insertItem("Methods", hidesubmenu);

	toolmenu = menuBar()->addMenu(tr("T&ools"));
	toolmenu->insertItem("&Group Tissues...", this, SLOT(execute_grouptissues()));
	toolmenu->insertItem("Remove Tissues...", this,
											 SLOT(execute_removetissues()));
	toolmenu->insertItem("Target->Tissue", this, SLOT(do_work2tissue()));
	toolmenu->insertItem("Target->Tissue grouped...", this,
											 SLOT(do_work2tissue_grouped()));
	toolmenu->insertItem("Tissue->Target", this, SLOT(do_tissue2work()));
	toolmenu->insertItem("In&verse Slice Order", this,
											 SLOT(execute_inversesliceorder()));
	toolmenu->insertItem("Clean Up", this, SLOT(execute_cleanup()));
	toolmenu->insertItem("Smooth Steps", this, SLOT(execute_smoothsteps()));
	// toolmenu->insertItem( "Smooth Tissues", this,  SLOT(execute_smoothtissues()));
	if (!m_editingmode)
		toolmenu->insertItem("Merge Projects...", this,
												 SLOT(execute_mergeprojects()));
	if (InternalUse::IsForInternalUse())
		toolmenu->insertItem("Check Bone Connectivity", this,
												 SLOT(execute_boneconnectivity()));

	atlasmenu = menuBar()->addMenu(tr("Atlas"));
	// todo: make atlas method generic, i.e. for loop
	// see e.g. https://stackoverflow.com/questions/9187538/how-to-add-a-list-of-qactions-to-a-qmenu-and-handle-them-with-a-single-slot
	m_atlasfilename.atlasnr[0] =
			atlasmenu->insertItem("", this, SLOT(execute_loadatlas0()));
	m_atlasfilename.atlasnr[1] =
			atlasmenu->insertItem("", this, SLOT(execute_loadatlas1()));
	m_atlasfilename.atlasnr[2] =
			atlasmenu->insertItem("", this, SLOT(execute_loadatlas2()));
	m_atlasfilename.atlasnr[3] =
			atlasmenu->insertItem("", this, SLOT(execute_loadatlas3()));
	m_atlasfilename.atlasnr[4] =
			atlasmenu->insertItem("", this, SLOT(execute_loadatlas4()));
	m_atlasfilename.atlasnr[5] =
			atlasmenu->insertItem("", this, SLOT(execute_loadatlas5()));
	m_atlasfilename.atlasnr[6] =
			atlasmenu->insertItem("", this, SLOT(execute_loadatlas6()));
	m_atlasfilename.atlasnr[7] =
			atlasmenu->insertItem("", this, SLOT(execute_loadatlas7()));
	m_atlasfilename.atlasnr[8] =
			atlasmenu->insertItem("", this, SLOT(execute_loadatlas8()));
	m_atlasfilename.atlasnr[9] =
			atlasmenu->insertItem("", this, SLOT(execute_loadatlas9()));
	m_atlasfilename.atlasnr[10] =
			atlasmenu->insertItem("", this, SLOT(execute_loadatlas10()));
	m_atlasfilename.atlasnr[11] =
			atlasmenu->insertItem("", this, SLOT(execute_loadatlas11()));
	m_atlasfilename.atlasnr[12] =
			atlasmenu->insertItem("", this, SLOT(execute_loadatlas12()));
	m_atlasfilename.atlasnr[13] =
			atlasmenu->insertItem("", this, SLOT(execute_loadatlas13()));
	m_atlasfilename.atlasnr[14] =
			atlasmenu->insertItem("", this, SLOT(execute_loadatlas14()));
	m_atlasfilename.atlasnr[15] =
			atlasmenu->insertItem("", this, SLOT(execute_loadatlas15()));
	m_atlasfilename.atlasnr[16] =
			atlasmenu->insertItem("", this, SLOT(execute_loadatlas16()));
	m_atlasfilename.atlasnr[17] =
			atlasmenu->insertItem("", this, SLOT(execute_loadatlas17()));
	m_atlasfilename.atlasnr[18] =
			atlasmenu->insertItem("", this, SLOT(execute_loadatlas18()));
	m_atlasfilename.atlasnr[19] =
			atlasmenu->insertItem("", this, SLOT(execute_loadatlas19()));
	m_atlasfilename.separatornr = atlasmenu->insertSeparator();
	atlasmenu->insertItem("Create Atlas...", this, SLOT(execute_createatlas()));
	atlasmenu->insertItem("Update Menu", this, SLOT(execute_reloadatlases()));
	atlasmenu->setItemVisible(m_atlasfilename.atlasnr[0], false);
	atlasmenu->setItemVisible(m_atlasfilename.atlasnr[1], false);
	atlasmenu->setItemVisible(m_atlasfilename.atlasnr[2], false);
	atlasmenu->setItemVisible(m_atlasfilename.atlasnr[3], false);
	atlasmenu->setItemVisible(m_atlasfilename.atlasnr[4], false);
	atlasmenu->setItemVisible(m_atlasfilename.atlasnr[5], false);
	atlasmenu->setItemVisible(m_atlasfilename.atlasnr[6], false);
	atlasmenu->setItemVisible(m_atlasfilename.atlasnr[7], false);
	atlasmenu->setItemVisible(m_atlasfilename.atlasnr[8], false);
	atlasmenu->setItemVisible(m_atlasfilename.atlasnr[9], false);
	atlasmenu->setItemVisible(m_atlasfilename.atlasnr[10], false);
	atlasmenu->setItemVisible(m_atlasfilename.atlasnr[11], false);
	atlasmenu->setItemVisible(m_atlasfilename.atlasnr[12], false);
	atlasmenu->setItemVisible(m_atlasfilename.atlasnr[13], false);
	atlasmenu->setItemVisible(m_atlasfilename.atlasnr[14], false);
	atlasmenu->setItemVisible(m_atlasfilename.atlasnr[15], false);
	atlasmenu->setItemVisible(m_atlasfilename.atlasnr[16], false);
	atlasmenu->setItemVisible(m_atlasfilename.atlasnr[17], false);
	atlasmenu->setItemVisible(m_atlasfilename.atlasnr[18], false);
	atlasmenu->setItemVisible(m_atlasfilename.atlasnr[19], false);
	atlasmenu->setItemVisible(m_atlasfilename.separatornr, false);

	helpmenu = menuBar()->addMenu(tr("Help"));
	helpmenu->insertItem(
			QIcon(m_picpath.absFilePath(QString("help.png")).ascii()), "About", this,
			SLOT(execute_about()));

	QObject::connect(toworkBtn, SIGNAL(clicked()), this,
									 SLOT(execute_bmp2work()));
	QObject::connect(tobmpBtn, SIGNAL(clicked()), this, SLOT(execute_work2bmp()));
	QObject::connect(swapBtn, SIGNAL(clicked()), this,
									 SLOT(execute_swap_bmpwork()));
	QObject::connect(swapAllBtn, SIGNAL(clicked()), this,
									 SLOT(execute_swap_bmpworkall()));
	QObject::connect(this, SIGNAL(bmp_changed()), this, SLOT(update_bmp()));
	QObject::connect(this, SIGNAL(work_changed()), this, SLOT(update_work()));
	QObject::connect(this, SIGNAL(work_changed()), bmp_show,
									 SLOT(workborder_changed()));
	QObject::connect(this, SIGNAL(marks_changed()), bmp_show,
									 SLOT(mark_changed()));
	QObject::connect(this, SIGNAL(marks_changed()), work_show,
									 SLOT(mark_changed()));
	QObject::connect(this, SIGNAL(tissues_changed()), this,
									 SLOT(update_tissue()));
	QObject::connect(bmp_show, SIGNAL(addmark_sign(Point)), this,
									 SLOT(add_mark(Point)));
	QObject::connect(bmp_show, SIGNAL(addlabel_sign(Point, std::string)), this,
									 SLOT(add_label(Point, std::string)));
	QObject::connect(bmp_show, SIGNAL(clearmarks_sign()), this,
									 SLOT(clear_marks()));
	QObject::connect(bmp_show, SIGNAL(removemark_sign(Point)), this,
									 SLOT(remove_mark(Point)));
	QObject::connect(bmp_show, SIGNAL(addtissue_sign(Point)), this,
									 SLOT(add_tissue(Point)));
	QObject::connect(bmp_show, SIGNAL(addtissueconnected_sign(Point)), this,
									 SLOT(add_tissue_connected(Point)));
	QObject::connect(bmp_show, SIGNAL(subtissue_sign(Point)), this,
									 SLOT(subtract_tissue(Point)));
	QObject::connect(bmp_show, SIGNAL(addtissue3D_sign(Point)), this,
									 SLOT(add_tissue_3D(Point)));
	QObject::connect(bmp_show, SIGNAL(addtissuelarger_sign(Point)), this,
									 SLOT(add_tissuelarger(Point)));
	QObject::connect(bmp_show, SIGNAL(selecttissue_sign(Point)), this,
									 SLOT(select_tissue(Point)));
	QObject::connect(work_show, SIGNAL(addmark_sign(Point)), this,
									 SLOT(add_mark(Point)));
	QObject::connect(work_show, SIGNAL(addlabel_sign(Point, std::string)), this,
									 SLOT(add_label(Point, std::string)));
	QObject::connect(work_show, SIGNAL(clearmarks_sign()), this,
									 SLOT(clear_marks()));
	QObject::connect(work_show, SIGNAL(removemark_sign(Point)), this,
									 SLOT(remove_mark(Point)));
	QObject::connect(work_show, SIGNAL(addtissue_sign(Point)), this,
									 SLOT(add_tissue(Point)));
	QObject::connect(work_show, SIGNAL(addtissueconnected_sign(Point)), this,
									 SLOT(add_tissue_connected(Point)));
	QObject::connect(work_show, SIGNAL(subtissue_sign(Point)), this,
									 SLOT(subtract_tissue(Point)));
	QObject::connect(work_show, SIGNAL(addtissue3D_sign(Point)), this,
									 SLOT(add_tissue_3D(Point)));
	QObject::connect(work_show, SIGNAL(addtissuelarger_sign(Point)), this,
									 SLOT(add_tissuelarger(Point)));
	QObject::connect(work_show, SIGNAL(selecttissue_sign(Point)), this,
									 SLOT(select_tissue(Point)));
	QObject::connect(tissueFilter, SIGNAL(textChanged(const QString &)), this,
									 SLOT(tissueFilterChanged(const QString &)));
	QObject::connect(lockTissues, SIGNAL(clicked()), this,
									 SLOT(lockAllTissues()));
	QObject::connect(addTissue, SIGNAL(clicked()), this,
									 SLOT(newTissuePressed()));
	QObject::connect(addFolder, SIGNAL(clicked()), this,
									 SLOT(newFolderPressed()));
	QObject::connect(modifyTissueFolder, SIGNAL(clicked()), this,
									 SLOT(modifTissueFolderPressed()));
	QObject::connect(removeTissueFolder, SIGNAL(clicked()), this,
									 SLOT(removeTissueFolderPressed()));
	QObject::connect(removeTissueFolderAll, SIGNAL(clicked()), this,
									 SLOT(removeTissueFolderAllPressed()));
	QObject::connect(getTissue, SIGNAL(clicked()), this,
									 SLOT(selectedtissue2work()));
	QObject::connect(getTissueAll, SIGNAL(clicked()), this,
									 SLOT(tissue2workall()));
	QObject::connect(clearTissue, SIGNAL(clicked()), this, SLOT(clearselected()));
	QObject::connect(clearTissues, SIGNAL(clicked()), this, SLOT(cleartissues()));

	QObject::connect(methodTab, SIGNAL(aboutToShow(QWidget *)), this,
									 SLOT(tab_changed(QWidget *)));

	tissueTreeWidget->setSelectionMode(
			QAbstractItemView::SelectionMode::ExtendedSelection);

	QObject::connect(tissueTreeWidget, SIGNAL(itemSelectionChanged()), this,
									 SLOT(tissue_selection_changed()));
	QObject::connect(tissueTreeWidget,
									 SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this,
									 SLOT(tree_widget_doubleclicked(QTreeWidgetItem *, int)));
	QObject::connect(tissueTreeWidget,
									 SIGNAL(customContextMenuRequested(const QPoint &)), this,
									 SLOT(tree_widget_contextmenu(const QPoint &)));

	tissues_size_t currTissueType = tissueTreeWidget->get_current_type();
	bmp_show->color_changed(currTissueType - 1);
	work_show->color_changed(currTissueType - 1);

	QObject::connect(cb_bmptissuevisible, SIGNAL(clicked()), this,
									 SLOT(bmptissuevisible_changed()));
	QObject::connect(cb_bmpcrosshairvisible, SIGNAL(clicked()), this,
									 SLOT(bmpcrosshairvisible_changed()));
	QObject::connect(cb_bmpoutlinevisible, SIGNAL(clicked()), this,
									 SLOT(bmpoutlinevisible_changed()));
	QObject::connect(cb_worktissuevisible, SIGNAL(clicked()), this,
									 SLOT(worktissuevisible_changed()));
	QObject::connect(cb_workcrosshairvisible, SIGNAL(clicked()), this,
									 SLOT(workcrosshairvisible_changed()));
	QObject::connect(cb_workpicturevisible, SIGNAL(clicked()), this,
									 SLOT(workpicturevisible_changed()));

	QObject::connect(pb_first, SIGNAL(clicked()), this, SLOT(pb_first_pressed()));
	QObject::connect(pb_last, SIGNAL(clicked()), this, SLOT(pb_last_pressed()));
	QObject::connect(sb_slicenr, SIGNAL(valueChanged(int)), this,
									 SLOT(sb_slicenr_changed()));
	QObject::connect(scb_slicenr, SIGNAL(valueChanged(int)), this,
									 SLOT(scb_slicenr_changed()));

	//	QObject::connect(le_slicethick,SIGNAL(returnPressed()),this,SLOT(slicethickness_changed()));
	QObject::connect(le_slicethick, SIGNAL(textChanged(const QString &)), this,
									 SLOT(slicethickness_changed()));

	QObject::connect(pb_add, SIGNAL(clicked()), this, SLOT(add_tissue_pushed()));
	QObject::connect(pb_sub, SIGNAL(clicked()), this,
									 SLOT(subtract_tissue_pushed()));
	QObject::connect(pb_addhold, SIGNAL(clicked()), this,
									 SLOT(addhold_tissue_pushed()));
	//	QObject::connect(pb_stophold,SIGNAL(clicked()),this,SLOT(stophold_tissue_pushed()));
	QObject::connect(pb_subhold, SIGNAL(clicked()), this,
									 SLOT(subtracthold_tissue_pushed()));
	//	QObject::connect(pb_addconn,SIGNAL(clicked()),this,SLOT(add_tissue_connected_pushed()));
	//	QObject::connect(pb_add3D,SIGNAL(clicked()),this,SLOT(add_tissue_3D_pushed()));

	/*if(xsliceshower)
		QObject::connect(xsliceshower,SIGNAL(hasbeenclosed()),this,SLOT(xslice_closed()));
	if(ysliceshower)
		QObject::connect(ysliceshower,SIGNAL(hasbeenclosed()),this,SLOT(yslice_closed()));*/
	QObject::connect(bmp_scroller, SIGNAL(contentsMoving(int, int)), this,
									 SLOT(setWorkContentsPos(int, int)));
	QObject::connect(work_scroller, SIGNAL(contentsMoving(int, int)), this,
									 SLOT(setBmpContentsPos(int, int)));
	QObject::connect(bmp_show, SIGNAL(setcenter_sign(int, int)), bmp_scroller,
									 SLOT(center(int, int)));
	QObject::connect(work_show, SIGNAL(setcenter_sign(int, int)), work_show,
									 SLOT(center(int, int)));
	tomove_scroller = true;

	interpolwidget->tissuenr_changed(currTissueType - 1);

	QObject::connect(zoom_widget, SIGNAL(set_zoom(double)), bmp_show,
									 SLOT(set_zoom(double)));
	QObject::connect(zoom_widget, SIGNAL(set_zoom(double)), work_show,
									 SLOT(set_zoom(double)));

	QObject::connect(
			this, SIGNAL(begin_dataexport(common::DataSelection &, QWidget *)), this,
			SLOT(handle_begin_dataexport(common::DataSelection &, QWidget *)));
	QObject::connect(this, SIGNAL(end_dataexport(QWidget *)), this,
									 SLOT(handle_end_dataexport(QWidget *)));

	QObject::connect(
			this, SIGNAL(begin_datachange(common::DataSelection &, QWidget *, bool)),
			this,
			SLOT(handle_begin_datachange(common::DataSelection &, QWidget *, bool)));
	QObject::connect(
			this, SIGNAL(end_datachange(QWidget *, common::EndUndoAction)), this,
			SLOT(handle_end_datachange(QWidget *, common::EndUndoAction)));
	QObject::connect(
			this, SIGNAL(end_datachange(QRect, QWidget *, common::EndUndoAction)),
			this,
			SLOT(handle_end_datachange(QRect, QWidget *, common::EndUndoAction)));
	QObject::connect(
			threshold_widget,
			SIGNAL(begin_datachange(common::DataSelection &, QWidget *, bool)), this,
			SLOT(handle_begin_datachange(common::DataSelection &, QWidget *, bool)));
	QObject::connect(
			threshold_widget,
			SIGNAL(end_datachange(QWidget *, common::EndUndoAction)), this,
			SLOT(handle_end_datachange(QWidget *, common::EndUndoAction)));
	QObject::connect(
			OutlineCorrect_widget,
			SIGNAL(begin_datachange(common::DataSelection &, QWidget *, bool)), this,
			SLOT(handle_begin_datachange(common::DataSelection &, QWidget *, bool)));
	QObject::connect(
			OutlineCorrect_widget,
			SIGNAL(end_datachange(QWidget *, common::EndUndoAction)), this,
			SLOT(handle_end_datachange(QWidget *, common::EndUndoAction)));
	QObject::connect(
			OutlineCorrect_widget,
			SIGNAL(end_datachange(QRect, QWidget *, common::EndUndoAction)), this,
			SLOT(handle_end_datachange(QRect, QWidget *, common::EndUndoAction)));
	QObject::connect(OutlineCorrect_widget,
									 SIGNAL(signal_request_selected_tissue_TS()), this,
									 SLOT(provide_selected_tissue_TS()));
	QObject::connect(OutlineCorrect_widget,
									 SIGNAL(signal_request_selected_tissue_BG()), this,
									 SLOT(provide_selected_tissue_BG()));
	QObject::connect(
			smoothing_widget,
			SIGNAL(begin_datachange(common::DataSelection &, QWidget *, bool)), this,
			SLOT(handle_begin_datachange(common::DataSelection &, QWidget *, bool)));
	QObject::connect(
			smoothing_widget,
			SIGNAL(end_datachange(QWidget *, common::EndUndoAction)), this,
			SLOT(handle_end_datachange(QWidget *, common::EndUndoAction)));
	QObject::connect(
			edge_widg,
			SIGNAL(begin_datachange(common::DataSelection &, QWidget *, bool)), this,
			SLOT(handle_begin_datachange(common::DataSelection &, QWidget *, bool)));
	QObject::connect(
			edge_widg, SIGNAL(end_datachange(QWidget *, common::EndUndoAction)), this,
			SLOT(handle_end_datachange(QWidget *, common::EndUndoAction)));
	QObject::connect(
			morph_widget,
			SIGNAL(begin_datachange(common::DataSelection &, QWidget *, bool)), this,
			SLOT(handle_begin_datachange(common::DataSelection &, QWidget *, bool)));
	QObject::connect(
			morph_widget, SIGNAL(end_datachange(QWidget *, common::EndUndoAction)),
			this, SLOT(handle_end_datachange(QWidget *, common::EndUndoAction)));
	QObject::connect(
			wshed_widget,
			SIGNAL(begin_datachange(common::DataSelection &, QWidget *, bool)), this,
			SLOT(handle_begin_datachange(common::DataSelection &, QWidget *, bool)));
	QObject::connect(
			wshed_widget, SIGNAL(end_datachange(QWidget *, common::EndUndoAction)),
			this, SLOT(handle_end_datachange(QWidget *, common::EndUndoAction)));
	QObject::connect(
			lw_widget,
			SIGNAL(begin_datachange(common::DataSelection &, QWidget *, bool)), this,
			SLOT(handle_begin_datachange(common::DataSelection &, QWidget *, bool)));
	QObject::connect(
			lw_widget, SIGNAL(end_datachange(QWidget *, common::EndUndoAction)), this,
			SLOT(handle_end_datachange(QWidget *, common::EndUndoAction)));
	QObject::connect(
			iftrg_widget,
			SIGNAL(begin_datachange(common::DataSelection &, QWidget *, bool)), this,
			SLOT(handle_begin_datachange(common::DataSelection &, QWidget *, bool)));
	QObject::connect(
			iftrg_widget, SIGNAL(end_datachange(QWidget *, common::EndUndoAction)),
			this, SLOT(handle_end_datachange(QWidget *, common::EndUndoAction)));
	QObject::connect(
			FMF_widget,
			SIGNAL(begin_datachange(common::DataSelection &, QWidget *, bool)), this,
			SLOT(handle_begin_datachange(common::DataSelection &, QWidget *, bool)));
	QObject::connect(
			FMF_widget, SIGNAL(end_datachange(QWidget *, common::EndUndoAction)),
			this, SLOT(handle_end_datachange(QWidget *, common::EndUndoAction)));
	QObject::connect(
			interpolwidget,
			SIGNAL(begin_datachange(common::DataSelection &, QWidget *, bool)), this,
			SLOT(handle_begin_datachange(common::DataSelection &, QWidget *, bool)));
	QObject::connect(
			interpolwidget, SIGNAL(end_datachange(QWidget *, common::EndUndoAction)),
			this, SLOT(handle_end_datachange(QWidget *, common::EndUndoAction)));
	QObject::connect(
			scalewidget,
			SIGNAL(begin_datachange(common::DataSelection &, QWidget *, bool)), this,
			SLOT(handle_begin_datachange(common::DataSelection &, QWidget *, bool)));
	QObject::connect(
			scalewidget, SIGNAL(end_datachange(QWidget *, common::EndUndoAction)),
			this, SLOT(handle_end_datachange(QWidget *, common::EndUndoAction)));
	QObject::connect(
			imagemathwidget,
			SIGNAL(begin_datachange(common::DataSelection &, QWidget *, bool)), this,
			SLOT(handle_begin_datachange(common::DataSelection &, QWidget *, bool)));
	QObject::connect(
			imagemathwidget, SIGNAL(end_datachange(QWidget *, common::EndUndoAction)),
			this, SLOT(handle_end_datachange(QWidget *, common::EndUndoAction)));
	QObject::connect(
			imageoverlaywidget,
			SIGNAL(begin_datachange(common::DataSelection &, QWidget *, bool)), this,
			SLOT(handle_begin_datachange(common::DataSelection &, QWidget *, bool)));
	QObject::connect(
			imageoverlaywidget,
			SIGNAL(end_datachange(QWidget *, common::EndUndoAction)), this,
			SLOT(handle_end_datachange(QWidget *, common::EndUndoAction)));
	QObject::connect(
			bitstack_widget,
			SIGNAL(begin_datachange(common::DataSelection &, QWidget *, bool)), this,
			SLOT(handle_begin_datachange(common::DataSelection &, QWidget *, bool)));
	QObject::connect(
			bitstack_widget, SIGNAL(end_datachange(QWidget *, common::EndUndoAction)),
			this, SLOT(handle_end_datachange(QWidget *, common::EndUndoAction)));
	QObject::connect(
			bitstack_widget,
			SIGNAL(begin_dataexport(common::DataSelection &, QWidget *)), this,
			SLOT(handle_begin_dataexport(common::DataSelection &, QWidget *)));
	QObject::connect(bitstack_widget, SIGNAL(end_dataexport(QWidget *)), this,
									 SLOT(handle_end_dataexport(QWidget *)));
	QObject::connect(
			hyst_widget,
			SIGNAL(begin_datachange(common::DataSelection &, QWidget *, bool)), this,
			SLOT(handle_begin_datachange(common::DataSelection &, QWidget *, bool)));
	QObject::connect(
			hyst_widget, SIGNAL(end_datachange(QWidget *, common::EndUndoAction)),
			this, SLOT(handle_end_datachange(QWidget *, common::EndUndoAction)));
	QObject::connect(
			pickerwidget,
			SIGNAL(begin_datachange(common::DataSelection &, QWidget *, bool)), this,
			SLOT(handle_begin_datachange(common::DataSelection &, QWidget *, bool)));
	QObject::connect(
			pickerwidget, SIGNAL(end_datachange(QWidget *, common::EndUndoAction)),
			this, SLOT(handle_end_datachange(QWidget *, common::EndUndoAction)));
	QObject::connect(
			transfrmWidget,
			SIGNAL(begin_datachange(common::DataSelection &, QWidget *, bool)), this,
			SLOT(handle_begin_datachange(common::DataSelection &, QWidget *, bool)));
	QObject::connect(
			transfrmWidget, SIGNAL(end_datachange(QWidget *, common::EndUndoAction)),
			this, SLOT(handle_end_datachange(QWidget *, common::EndUndoAction)));
	QObject::connect(overlay_widget, SIGNAL(overlay_changed()), bmp_show,
									 SLOT(overlay_changed()));
	QObject::connect(overlay_widget, SIGNAL(overlay_changed()), work_show,
									 SLOT(overlay_changed()));
	QObject::connect(overlay_widget, SIGNAL(overlayalpha_changed(float)),
									 bmp_show, SLOT(set_overlayalpha(float)));
	QObject::connect(overlay_widget, SIGNAL(overlayalpha_changed(float)),
									 work_show, SLOT(set_overlayalpha(float)));
	QObject::connect(overlay_widget, SIGNAL(bmpoverlayvisible_changed(bool)),
									 bmp_show, SLOT(set_overlayvisible(bool)));
	QObject::connect(overlay_widget, SIGNAL(workoverlayvisible_changed(bool)),
									 work_show, SLOT(set_overlayvisible(bool)));

	QObject::connect(m_MultiDataset_widget, SIGNAL(dataset_changed()), bmp_show,
									 SLOT(overlay_changed()));
	QObject::connect(m_MultiDataset_widget, SIGNAL(dataset_changed()), work_show,
									 SLOT(overlay_changed()));
	QObject::connect(m_MultiDataset_widget, SIGNAL(dataset_changed()), this,
									 SLOT(DatasetChanged()));
	QObject::connect(
			m_MultiDataset_widget,
			SIGNAL(begin_datachange(common::DataSelection &, QWidget *, bool)), this,
			SLOT(handle_begin_datachange(common::DataSelection &, QWidget *, bool)));
	QObject::connect(
			m_MultiDataset_widget,
			SIGNAL(end_datachange(QWidget *, common::EndUndoAction)), this,
			SLOT(handle_end_datachange(QWidget *, common::EndUndoAction)));

	QObject::connect(cb_tissuelock, SIGNAL(clicked()), this,
									 SLOT(tissuelock_toggled()));

	QObject::connect(sl_contrastbmp, SIGNAL(valueChanged(int)), this,
									 SLOT(sl_contrastbmp_moved(int)));
	QObject::connect(sl_contrastwork, SIGNAL(valueChanged(int)), this,
									 SLOT(sl_contrastwork_moved(int)));
	QObject::connect(sl_brightnessbmp, SIGNAL(valueChanged(int)), this,
									 SLOT(sl_brightnessbmp_moved(int)));
	QObject::connect(sl_brightnesswork, SIGNAL(valueChanged(int)), this,
									 SLOT(sl_brightnesswork_moved(int)));

	QObject::connect(le_contrastbmp_val, SIGNAL(editingFinished()), this,
									 SLOT(le_contrastbmp_val_edited()));
	QObject::connect(le_contrastwork_val, SIGNAL(editingFinished()), this,
									 SLOT(le_contrastwork_val_edited()));
	QObject::connect(le_brightnessbmp_val, SIGNAL(editingFinished()), this,
									 SLOT(le_brightnessbmp_val_edited()));
	QObject::connect(le_brightnesswork_val, SIGNAL(editingFinished()), this,
									 SLOT(le_brightnesswork_val_edited()));

	m_widget_signal_mapper = new QSignalMapper(this);
	for (int i = 0; i < nrtabbuttons; ++i) {
		QObject::connect(pb_tab[i], SIGNAL(clicked()), m_widget_signal_mapper,
										 SLOT(map()));
		m_widget_signal_mapper->setMapping(pb_tab[i], i);
		QObject::connect(m_widget_signal_mapper, SIGNAL(mapped(int)), this,
										 SLOT(pb_tab_pressed(int)));
	}

	QObject::connect(bmp_show, SIGNAL(wheelrotatedctrl_sign(int)), this,
									 SLOT(wheelrotated(int)));
	QObject::connect(work_show, SIGNAL(wheelrotatedctrl_sign(int)), this,
									 SLOT(wheelrotated(int)));

	QObject::connect(bmp_show, SIGNAL(mousePosZoom_sign(QPoint)), this,
									 SLOT(mousePosZoom_changed(QPoint)));
	QObject::connect(work_show, SIGNAL(mousePosZoom_sign(QPoint)), this,
									 SLOT(mousePosZoom_changed(QPoint)));

	//	QObject::connect(pb_work2tissue,SIGNAL(clicked()),this,SLOT(do_work2tissue()));

	m_acc_sliceup = new Q3Accel(this);
	m_acc_sliceup->connectItem(
			m_acc_sliceup->insertItem(QKeySequence(Qt::CTRL + Qt::Key_Right)), this,
			SLOT(slicenr_up()));
	m_acc_slicedown = new Q3Accel(this);
	m_acc_slicedown->connectItem(
			m_acc_slicedown->insertItem(QKeySequence(Qt::CTRL + Qt::Key_Left)), this,
			SLOT(slicenr_down()));
	m_acc_sliceup1 = new Q3Accel(this);
	m_acc_sliceup->connectItem(
			m_acc_sliceup->insertItem(QKeySequence(Qt::Key_Next)), this,
			SLOT(slicenr_up()));
	m_acc_slicedown1 = new Q3Accel(this);
	m_acc_slicedown->connectItem(
			m_acc_slicedown->insertItem(QKeySequence(Qt::Key_Prior)), this,
			SLOT(slicenr_down()));
	m_acc_zoomin = new Q3Accel(this);
	m_acc_zoomin->connectItem(
			m_acc_zoomin->insertItem(QKeySequence(Qt::CTRL + Qt::Key_Up)), this,
			SLOT(zoom_in()));
	m_acc_zoomout = new Q3Accel(this);
	m_acc_zoomout->connectItem(
			m_acc_zoomout->insertItem(QKeySequence(Qt::CTRL + Qt::Key_Down)), this,
			SLOT(zoom_out()));
	m_acc_add = new Q3Accel(this);
	m_acc_add->connectItem(
			m_acc_add->insertItem(QKeySequence(Qt::CTRL + Qt::Key_Plus)), this,
			SLOT(add_tissue_shortkey()));
	m_acc_sub = new Q3Accel(this);
	m_acc_sub->connectItem(
			m_acc_sub->insertItem(QKeySequence(Qt::CTRL + Qt::Key_Minus)), this,
			SLOT(subtract_tissue_shortkey()));
	m_acc_undo = new Q3Accel(this);
	m_acc_undo->connectItem(m_acc_undo->insertItem(QKeySequence(Qt::Key_Escape)),
													this, SLOT(execute_undo()));
	m_acc_undo2 = new Q3Accel(this);
	m_acc_undo2->connectItem(m_acc_undo2->insertItem(QKeySequence("Ctrl+Z")),
													 this, SLOT(execute_undo()));
	m_acc_redo = new Q3Accel(this);
	m_acc_redo->connectItem(m_acc_redo->insertItem(QKeySequence("Ctrl+Y")), this,
													SLOT(execute_redo()));
	//	setCentralWidget(hbox);
	//	vbox->insertChild(hbox2);

	update_brightnesscontrast(true);
	update_brightnesscontrast(false);

	tissuenr_changed(currTissueType - 1);

	this->setMinimumHeight(this->minimumHeight() + 50);

	m_Modified = false;
	m_NewDataAfterSwap = false;

	methodTab->raiseWidget(methodTab->visibleWidget());
	return;
}

MainWindow::~MainWindow()
{
	//delete vboxtotal;
	//delete file;
	//delete loadmenu;
	//delete reloadmenu;
	//delete exportmenu;
	//delete imagemenu;
	//delete editmenu;
	//delete toolmenu;
	//delete helpmenu;
	//delete hidemenu;
	//delete hidesubmenu;
	//delete menubar;
	//delete menubarspacer;
	//delete cb_bmptissuevisible;
	//delete cb_bmpcrosshairvisible;
	//delete cb_bmpoutlinevisible;
	//delete cb_worktissuevisible;
	//delete cb_workcrosshairvisible;
	//delete cb_workpicturevisible;
	//delete bmp_show;
	//delete lb_source;
	//delete lb_target;
	//delete bmp_scroller;
	//delete sl_contrastbmp;
	//delete sl_brightnessbmp;
	//delete lb_contrastbmp;
	//delete lb_brightnessbmp;
	//delete toworkBtn;
	//delete tobmpBtn;
	//delete swapBtn;
	//delete swapAllBtn;
	//delete zoom_widget;
	//delete work_show;
	//delete sl_contrastwork;
	//delete sl_brightnesswork;
	//delete lb_contrastwork;
	//delete lb_brightnesswork;
	//delete work_scroller;
	//delete scalewidget;
	//delete imagemathwidget;
	//delete tissueTreeWidget;
	//delete cb_tissuelock;
	//delete lockTissues;
	//delete addTissue;
	//delete modifyTissue;
	//delete removeTissue;
	//delete tissue3Dopt;
	//delete getTissue;
	//delete clearTissue;
	//delete clearTissues;
	//delete cb_addsub3d;
	//delete cb_addsuboverride;
	//delete cb_addsubconn;
	//delete pb_add;
	//delete pb_sub;
	//delete pb_addhold;
	//delete pb_subhold;
	//delete pb_first;
	//delete scb_slicenr;
	//delete pb_last;
	//delete sb_slicenr;
	//delete lb_slicenr;
	//delete lb_inactivewarning;
	//delete lb_slicethick;
	//delete le_slicethick;
	//delete methodTab;
	//delete bitstack_widget;
	//delete m_acc_sliceup;
	//delete m_acc_slicedown;
	//delete m_acc_sliceup1;
	//delete m_acc_slicedown1;
	//delete m_acc_zoomin;
	//delete m_acc_zoomout;
	//delete m_acc_add;
	//delete m_acc_sub;
	//delete m_acc_undo;
	//for(unsigned short i=0;i<nrtabbuttons;i++)
	//	delete pb_tab[i];
	///*delete pb_tab1;
	//delete pb_tab2;
	//delete pb_tab3;
	//delete pb_tab4;
	//delete pb_tab5;
	//delete pb_tab6;
	//delete pb_tab7;
	//delete pb_tab8;
	//delete pb_tab9;
	//delete pb_tab10;
	//delete pb_tab11;
	//delete pb_tab12;
	//delete pb_tab13;
	//delete pb_tab14;*/
	//delete hideparameters;
	//for(unsigned short i=0;i<nrtabbuttons;i++)
	//	delete showtab_action[i];
	//delete hidezoom;
	//delete hidecontrastbright;
	//delete hidecopyswap;
	//delete hidestack;
}

void MainWindow::closeEvent(QCloseEvent *qce)
{
	if (maybeSafe()) {
		if (xsliceshower != NULL) {
			xsliceshower->close();
			QObject::disconnect(bmp_show,
													SIGNAL(scaleoffsetfactor_changed(float, float, bool)),
													xsliceshower, SLOT(set_scale(float, float, bool)));
			QObject::disconnect(work_show,
													SIGNAL(scaleoffsetfactor_changed(float, float, bool)),
													xsliceshower, SLOT(set_scale(float, float, bool)));
			QObject::disconnect(xsliceshower, SIGNAL(slice_changed(int)), this,
													SLOT(xshower_slicechanged()));
			QObject::disconnect(zoom_widget, SIGNAL(set_zoom(double)), xsliceshower,
													SLOT(set_zoom(double)));
			QObject::disconnect(xsliceshower, SIGNAL(hasbeenclosed()), this,
													SLOT(xslice_closed()));
			delete xsliceshower;
		}
		if (ysliceshower != NULL) {
			ysliceshower->close();
			QObject::disconnect(bmp_show,
													SIGNAL(scaleoffsetfactor_changed(float, float, bool)),
													ysliceshower, SLOT(set_scale(float, float, bool)));
			QObject::disconnect(work_show,
													SIGNAL(scaleoffsetfactor_changed(float, float, bool)),
													ysliceshower, SLOT(set_scale(float, float, bool)));
			QObject::disconnect(ysliceshower, SIGNAL(slice_changed(int)), this,
													SLOT(yshower_slicechanged()));
			QObject::disconnect(zoom_widget, SIGNAL(set_zoom(double)), ysliceshower,
													SLOT(set_zoom(double)));
			QObject::disconnect(ysliceshower, SIGNAL(hasbeenclosed()), this,
													SLOT(yslice_closed()));
			delete ysliceshower;
		}
		if (SV3D != NULL) {
			SV3D->close();
			QObject::disconnect(SV3D, SIGNAL(hasbeenclosed()), this,
													SLOT(SV3D_closed()));
			delete SV3D;
		}
		if (SV3Dbmp != NULL) {
			SV3Dbmp->close();
			QObject::disconnect(SV3Dbmp, SIGNAL(hasbeenclosed()), this,
													SLOT(SV3Dbmp_closed()));
			delete SV3Dbmp;
		}
		if (VV3D != NULL) {
			VV3D->close();
			QObject::disconnect(VV3D, SIGNAL(hasbeenclosed()), this,
													SLOT(VV3D_closed()));
			delete VV3D;
		}
		if (VV3Dbmp != NULL) {
			VV3Dbmp->close();
			QObject::disconnect(VV3Dbmp, SIGNAL(hasbeenclosed()), this,
													SLOT(VV3Dbmp_closed()));
			delete VV3Dbmp;
		}

		SaveSettings();
		SaveLoadProj(m_loadprojfilename.m_filename);
		QMainWindow::closeEvent(qce);
	}
	else {
		qce->ignore();
	}
}

bool MainWindow::maybeSafe()
{
	if (modified()) {
		int ret = QMessageBox::warning(
				this, "iSeg", "Do you want to save your changes?",
				QMessageBox::Yes | QMessageBox::Default, QMessageBox::No,
				QMessageBox::Cancel | QMessageBox::Escape);
		if (ret == QMessageBox::Yes) {
			// Handle tissue hierarchy changes
			if (!tissueHierarchyWidget->handle_changed_hierarchy()) {
				return false;
			}
			if ((!m_editingmode && !m_saveprojfilename.isEmpty()) ||
					(m_editingmode && !m_S4Lcommunicationfilename.isEmpty())) {
				execute_saveproj();
			}
			else {
				execute_saveprojas();
			}
			return true;
		}
		else if (ret == QMessageBox::Cancel) {
			return false;
		}
	}
	return true;
}

bool MainWindow::modified() { return m_Modified; }

void MainWindow::provide_selected_tissue_BG()
{
	if (TissueInfos::GetTissueLocked(tissueTreeWidget->get_current_type())) {
		QMessageBox::warning(this, "iSeg",
												 "Error: Unable to get " +
														 tissueTreeWidget->get_current_name() +
														 " because it is locked.\nPlease, unlock it.",
												 QMessageBox::Ok | QMessageBox::Default);
		return;
	}

	OutlineCorrect_widget->Select_selected_tissue_BG(
			tissueTreeWidget->get_current_name(),
			tissueTreeWidget->get_current_type());
}
void MainWindow::provide_selected_tissue_TS()
{
	if (TissueInfos::GetTissueLocked(tissueTreeWidget->get_current_type())) {
		QMessageBox::warning(this, "iSeg",
												 "Error: Unable to get " +
														 tissueTreeWidget->get_current_name() +
														 " because it is locked.\nPlease, unlock it.",
												 QMessageBox::Ok | QMessageBox::Default);
		return;
	}

	OutlineCorrect_widget->Select_selected_tissue_TS(
			tissueTreeWidget->get_current_name(),
			tissueTreeWidget->get_current_type());
}

void MainWindow::execute_bmp2work()
{
	common::DataSelection dataSelection;
	dataSelection.sliceNr = handler3D->get_activeslice();
	dataSelection.work = true;
	emit begin_datachange(dataSelection, this);

	handler3D->bmp2work();

	emit end_datachange(this);
	//	work_show->update();
	return;
}

void MainWindow::execute_work2bmp()
{
	common::DataSelection dataSelection;
	dataSelection.sliceNr = handler3D->get_activeslice();
	dataSelection.bmp = true;
	emit begin_datachange(dataSelection, this);

	handler3D->work2bmp();

	emit end_datachange(this);
	//	bmp_show->update();
	return;
}

void MainWindow::execute_swap_bmpwork()
{
	common::DataSelection dataSelection;
	dataSelection.sliceNr = handler3D->get_activeslice();
	dataSelection.bmp = true;
	dataSelection.work = true;
	emit begin_datachange(dataSelection, this);

	handler3D->swap_bmpwork();

	emit end_datachange(this);
	//	work_show->update();
	//	bmp_show->update();
	return;
}

void MainWindow::execute_swap_bmpworkall()
{
	common::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.bmp = true;
	dataSelection.work = true;
	emit begin_datachange(dataSelection, this);

	handler3D->swap_bmpworkall();

	emit end_datachange(this);
	//	work_show->update();
	//	bmp_show->update();
	return;
}

void MainWindow::execute_saveContours()
{
	common::DataSelection dataSelection;
	dataSelection.tissues = true;
	emit begin_dataexport(dataSelection, this);

	SaveOutlinesWidget SOW(handler3D, this);
	SOW.move(QCursor::pos());
	SOW.exec();

	emit end_dataexport(this);
}

void MainWindow::execute_loadbmp1()
{
	maybeSafe();

	common::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this, false);

	LoaderBmp LB(handler3D, this);
	LB.move(QCursor::pos());
	LB.exec();

	//	work_show->update();//(bmphand->return_width(),bmphand->return_height());
	//	bmp_show->update();//(bmphand->return_width(),bmphand->return_height());
	//	bmp_show->workborder_changed();

	emit end_datachange(this, common::ClearUndo);

	//	hbox1->setFixedSize(bmphand->return_width()*2+vbox1->sizeHint().width(),bmphand->return_height());

	reset_brightnesscontrast();
}

void MainWindow::execute_swapxy()
{
	common::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this, false);

	bool ok = handler3D->SwapXY();

	// Swap also the other datasets
	bool swapExtraDatasets = false;
	if (m_MultiDataset_widget->isVisible() && swapExtraDatasets) {
		unsigned short w, h, nrslices;
		w = handler3D->return_height();
		h = handler3D->return_width();
		nrslices = handler3D->return_nrslices();
		QString str1;
		for (int i = 0; i < m_MultiDataset_widget->GetNumberOfDatasets(); i++) {
			// Swap all but the active one
			if (!m_MultiDataset_widget->IsActive(i)) {
				std::string tempFileName =
						"bmp_float_eds_" + std::to_string(i) + ".raw";
				str1 = QDir::temp().absFilePath(QString(tempFileName.c_str()));
				if (SlicesHandler::SaveRaw_xy_swapped(
								str1.ascii(), m_MultiDataset_widget->GetBmpData(i), w, h,
								nrslices) != 0)
					ok = false;

				tempFileName = "work_float_eds_" + std::to_string(i) + ".raw";
				str1 = QDir::temp().absFilePath(QString(tempFileName.c_str()));
				if (SlicesHandler::SaveRaw_xy_swapped(
								str1.ascii(), m_MultiDataset_widget->GetWorkingData(i), w, h,
								nrslices) != 0)
					ok = false;

				if (ok) {
					tempFileName = "work_float_eds_" + std::to_string(i) + ".raw";
					str1 = QDir::temp().absFilePath(QString(tempFileName.c_str()));
					str1 = QDir::temp().absFilePath(QString("work_float.raw"));
					m_MultiDataset_widget->SetWorkingData(
							i, SlicesHandler::LoadRawFloat(
										 str1.ascii(), handler3D->return_startslice(),
										 handler3D->return_endslice(), 0, w * h));
				}

				if (ok) {
					tempFileName = "bmp_float_eds_" + std::to_string(i) + ".raw";
					str1 = QDir::temp().absFilePath(QString(tempFileName.c_str()));
					str1 = QDir::temp().absFilePath(QString("bmp_float.raw"));
					m_MultiDataset_widget->SetBmpData(
							i, SlicesHandler::LoadRawFloat(
										 str1.ascii(), handler3D->return_startslice(),
										 handler3D->return_endslice(), 0, w * h));
				}
			}
		}

		m_NewDataAfterSwap = true;
	}

	emit end_datachange(this, common::ClearUndo);
	Pair p = handler3D->get_pixelsize();
	if (p.low != p.high) {
		handler3D->set_pixelsize(p.low, p.high);
		pixelsize_changed();
	}
	clear_stack();
}

void MainWindow::execute_swapxz()
{
	common::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this, false);

	bool ok = handler3D->SwapXZ();

	// Swap also the other datasets
	bool swapExtraDatasets = false;
	if (m_MultiDataset_widget->isVisible() && swapExtraDatasets) {
		unsigned short w, h, nrslices;
		w = handler3D->return_nrslices();
		h = handler3D->return_height();
		nrslices = handler3D->return_width();
		QString str1;
		for (int i = 0; i < m_MultiDataset_widget->GetNumberOfDatasets(); i++) {
			// Swap all but the active one
			if (!m_MultiDataset_widget->IsActive(i)) {
				std::string tempFileName = "bmp_float_" + std::to_string(i) + ".raw";
				str1 = QDir::temp().absFilePath(QString(tempFileName.c_str()));
				if (SlicesHandler::SaveRaw_xz_swapped(
								str1.ascii(), m_MultiDataset_widget->GetBmpData(i), w, h,
								nrslices) != 0)
					ok = false;

				tempFileName = "work_float_" + std::to_string(i) + ".raw";
				str1 = QDir::temp().absFilePath(QString(tempFileName.c_str()));
				if (SlicesHandler::SaveRaw_xz_swapped(
								str1.ascii(), m_MultiDataset_widget->GetWorkingData(i), w, h,
								nrslices) != 0)
					ok = false;

				if (ok) {
					tempFileName = "work_float_" + std::to_string(i) + ".raw";
					str1 = QDir::temp().absFilePath(QString(tempFileName.c_str()));
					m_MultiDataset_widget->SetWorkingData(
							i, SlicesHandler::LoadRawFloat(
										 str1.ascii(), handler3D->return_startslice(),
										 handler3D->return_endslice(), 0, w * h));
				}

				if (ok) {
					tempFileName = "bmp_float_" + std::to_string(i) + ".raw";
					str1 = QDir::temp().absFilePath(QString(tempFileName.c_str()));
					m_MultiDataset_widget->SetBmpData(
							i, SlicesHandler::LoadRawFloat(
										 str1.ascii(), handler3D->return_startslice(),
										 handler3D->return_endslice(), 0, w * h));
				}
			}
		}
		m_NewDataAfterSwap = true;
	}

	emit end_datachange(this, common::ClearUndo);

	Pair p = handler3D->get_pixelsize();
	float thick = handler3D->get_slicethickness();
	if (thick != p.high) {
		handler3D->set_pixelsize(thick, p.low);
		handler3D->set_slicethickness(p.high);
		pixelsize_changed();
		slicethickness_changed1();
	}

	clear_stack();
}

void MainWindow::execute_swapyz()
{
	common::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this, false);

	bool ok = handler3D->SwapYZ();

	// Swap also the other datasets
	bool swapExtraDatasets = false;
	if (m_MultiDataset_widget->isVisible() && swapExtraDatasets) {
		unsigned short w, h, nrslices;
		w = handler3D->return_width();
		h = handler3D->return_nrslices();
		nrslices = handler3D->return_height();
		QString str1;
		for (int i = 0; i < m_MultiDataset_widget->GetNumberOfDatasets(); i++) {
			// Swap all but the active one
			if (!m_MultiDataset_widget->IsActive(i)) {
				std::string tempFileName = "bmp_float_" + std::to_string(i) + ".raw";
				str1 = QDir::temp().absFilePath(QString(tempFileName.c_str()));
				if (SlicesHandler::SaveRaw_yz_swapped(
								str1.ascii(), m_MultiDataset_widget->GetBmpData(i), w, h,
								nrslices) != 0)
					ok = false;

				tempFileName = "work_float_" + std::to_string(i) + ".raw";
				str1 = QDir::temp().absFilePath(QString(tempFileName.c_str()));
				if (SlicesHandler::SaveRaw_yz_swapped(
								str1.ascii(), m_MultiDataset_widget->GetWorkingData(i), w, h,
								nrslices) != 0)
					ok = false;

				if (ok) {
					tempFileName = "work_float_" + std::to_string(i) + ".raw";
					str1 = QDir::temp().absFilePath(QString(tempFileName.c_str()));
					m_MultiDataset_widget->SetWorkingData(
							i, SlicesHandler::LoadRawFloat(
										 str1.ascii(), handler3D->return_startslice(),
										 handler3D->return_endslice(), 0, w * h));
				}

				if (ok) {
					tempFileName = "bmp_float_" + std::to_string(i) + ".raw";
					str1 = QDir::temp().absFilePath(QString(tempFileName.c_str()));
					m_MultiDataset_widget->SetBmpData(
							i, SlicesHandler::LoadRawFloat(
										 str1.ascii(), handler3D->return_startslice(),
										 handler3D->return_endslice(), 0, w * h));
				}
			}
		}
		m_NewDataAfterSwap = true;
	}

	emit end_datachange(this, common::ClearUndo);

	Pair p = handler3D->get_pixelsize();
	float thick = handler3D->get_slicethickness();
	if (thick != p.low) {
		handler3D->set_pixelsize(p.high, thick);
		handler3D->set_slicethickness(p.low);
		pixelsize_changed();
		slicethickness_changed1();
	}
	clear_stack();
}

void MainWindow::execute_resize(int resizetype)
{
	common::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this, false);

	ResizeDialog RD(handler3D, static_cast<ResizeDialog::eResizeType>(resizetype),
									this);
	RD.move(QCursor::pos());
	if (!RD.exec()) {
		return;
	}
	auto lut = handler3D->GetColorLookupTable();

	int dxm, dxp, dym, dyp, dzm, dzp;
	RD.return_padding(dxm, dxp, dym, dyp, dzm, dzp);

	unsigned short w, h, nrslices;
	w = handler3D->return_width();
	h = handler3D->return_height();
	nrslices = handler3D->return_nrslices();
	QString str1;
	bool ok = true;
	str1 = QDir::temp().absFilePath(QString("bmp_float.raw"));
	if (handler3D->SaveRaw_resized(str1.ascii(), dxm, dxp, dym, dyp, dzm, dzp,
																 false) != 0)
		ok = false;
	str1 = QDir::temp().absFilePath(QString("work_float.raw"));
	if (handler3D->SaveRaw_resized(str1.ascii(), dxm, dxp, dym, dyp, dzm, dzp,
																 true) != 0)
		ok = false;
	str1 = QDir::temp().absFilePath(QString("tissues.raw"));
	if (handler3D->SaveTissuesRaw_resized(str1.ascii(), dxm, dxp, dym, dyp, dzm,
																				dzp) != 0)
		ok = false;

	if (ok) {
		str1 = QDir::temp().absFilePath(QString("work_float.raw"));
		if (handler3D->ReadRawFloat(str1.ascii(), w + dxm + dxp, h + dym + dyp, 0,
																nrslices + dzm + dzp) != 1)
			ok = false;
	}
	if (ok) {
		str1 = QDir::temp().absFilePath(QString("bmp_float.raw"));
		if (handler3D->ReloadRawFloat(str1.ascii(), 0) != 1)
			ok = false;
	}
	if (ok) {
		str1 = QDir::temp().absFilePath(QString("tissues.raw"));
		if (handler3D->ReloadRawTissues(str1.ascii(), sizeof(tissues_size_t) * 8,
																		0) != 1)
			ok = false;
	}

	float spacing[3];
	Transform tr = handler3D->get_transform();
	handler3D->get_spacing(spacing);

	Transform transform_corrected(tr);
	int plo[3] = {dxm, dym, dzm};
	transform_corrected.paddingUpdateTransform(plo, spacing);
	handler3D->set_transform(transform_corrected);

	// add color lookup table again
	handler3D->UpdateColorLookupTable(lut);

	emit end_datachange(this, common::ClearUndo);

	clear_stack();
}

void MainWindow::execute_pad() { execute_resize(1); }

void MainWindow::execute_crop() { execute_resize(2); }

void MainWindow::sl_contrastbmp_moved(int i)
{
	UNREFERENCED_PARAMETER(i);

	// Update line edit
	float contrast = pow(10, sl_contrastbmp->value() * 4.0f / 100.0f - 2.0f);
	le_contrastbmp_val->setText(QString("%1").arg(contrast, 6, 'f', 2));

	// Update display
	update_brightnesscontrast(true);
}

void MainWindow::sl_contrastwork_moved(int i)
{
	UNREFERENCED_PARAMETER(i);

	// Update line edit
	float contrast = pow(10, sl_contrastwork->value() * 4.0f / 100.0f - 2.0f);
	le_contrastwork_val->setText(QString("%1").arg(contrast, 6, 'f', 2));

	// Update display
	update_brightnesscontrast(false);
}

void MainWindow::sl_brightnessbmp_moved(int i)
{
	UNREFERENCED_PARAMETER(i);

	// Update line edit
	le_brightnessbmp_val->setText(
			QString("%1").arg(sl_brightnessbmp->value() - 50, 3));

	// Update display
	update_brightnesscontrast(true);
}

void MainWindow::sl_brightnesswork_moved(int i)
{
	UNREFERENCED_PARAMETER(i);

	// Update line edit
	le_brightnesswork_val->setText(
			QString("%1").arg(sl_brightnesswork->value() - 50, 3));

	// Update display
	update_brightnesscontrast(false);
}

void MainWindow::le_contrastbmp_val_edited()
{
	// Clamp to range and round to precision
	float contrast =
			max(0.01f, min(le_contrastbmp_val->text().toFloat(), 100.0f));
	le_contrastbmp_val->setText(QString("%1").arg(contrast, 6, 'f', 2));

	// Update slider
	int sliderValue = std::floor(
			100.0f * (std::log10(le_contrastbmp_val->text().toFloat()) + 2.0f) /
					4.0f +
			0.5f);
	sl_contrastbmp->setValue(sliderValue);

	// Update display
	update_brightnesscontrast(true);
}

void MainWindow::le_contrastwork_val_edited()
{
	// Clamp to range and round to precision
	float contrast =
			max(0.01f, min(le_contrastwork_val->text().toFloat(), 100.0f));
	le_contrastwork_val->setText(QString("%1").arg(contrast, 6, 'f', 2));

	// Update slider
	int sliderValue = std::floor(
			100.0f * (std::log10(le_contrastwork_val->text().toFloat()) + 2.0f) /
					4.0f +
			0.5f);
	sl_contrastwork->setValue(sliderValue);

	// Update display
	update_brightnesscontrast(false);
}

void MainWindow::le_brightnessbmp_val_edited()
{
	// Clamp to range and round to precision
	int brightness = (int)max(
			-50.0f,
			min(std::floor(le_brightnessbmp_val->text().toFloat() + 0.5f), 50.0f));
	le_brightnessbmp_val->setText(QString("%1").arg(brightness, 3));

	// Update slider
	sl_brightnessbmp->setValue(brightness + 50);

	// Update display
	update_brightnesscontrast(true);
}

void MainWindow::le_brightnesswork_val_edited()
{
	// Clamp to range and round to precision
	int brightness = (int)max(
			-50.0f,
			min(std::floor(le_brightnesswork_val->text().toFloat() + 0.5f), 50.0f));
	le_brightnesswork_val->setText(QString("%1").arg(brightness, 3));

	// Update slider
	sl_brightnesswork->setValue(brightness + 50);

	// Update display
	update_brightnesscontrast(false);
}

void MainWindow::reset_brightnesscontrast()
{
	sl_contrastbmp->setValue(50);
	le_contrastbmp_val->setText(QString("%1").arg(1.0f, 6, 'f', 2));

	sl_brightnessbmp->setValue(50);
	le_brightnessbmp_val->setText(QString("%1").arg(0, 3));

	sl_contrastwork->setValue(50);
	le_contrastwork_val->setText(QString("%1").arg(1.0f, 6, 'f', 2));

	sl_brightnesswork->setValue(50);
	le_brightnesswork_val->setText(QString("%1").arg(0, 3));

	// Update display
	update_brightnesscontrast(true);
	update_brightnesscontrast(false);

	m_Modified = true;
}

void MainWindow::update_brightnesscontrast(bool bmporwork, bool paint)
{
	if (bmporwork) {
		// Get values from line edits
		float contrast = le_contrastbmp_val->text().toFloat();
		float brightness = (le_brightnessbmp_val->text().toFloat() + 50.0f) * 0.01f;

		// Update bmp shower
		bmp_show->set_brightnesscontrast(brightness, contrast, paint);
	}
	else {
		// Get values from sliders
		float contrast = le_contrastwork_val->text().toFloat();
		float brightness =
				(le_brightnesswork_val->text().toFloat() + 50.0f) * 0.01f;

		// Update work shower
		work_show->set_brightnesscontrast(brightness, contrast, paint);
	}
}

void MainWindow::EnableActionsAfterPrjLoaded(const bool enable)
{
	auto update = [this](int idx, bool enable) {
		if (idx >= 0) {
			if (enable)
				file->actions().at(idx)->setToolTip("");
			file->actions().at(idx)->setEnabled(enable);
		}
	};

	update(openS4LLinkPos, enable);
	update(importSurfacePos, enable);
	update(importRTstructPos, enable);
}

void MainWindow::execute_loadbmp()
{
	maybeSafe();

	QStringList files =
			Q3FileDialog::getOpenFileNames("Images (*.bmp)\n"
																		 "All(*.*)",
																		 QString::null, this, "open files dialog",
																		 "Select one or more files to open");

	if (!files.empty()) {
		sort(files.begin(), files.end());

		vector<int> vi;
		vi.clear();

		short nrelem = files.size();

		for (short i = 0; i < nrelem; i++) {
			vi.push_back(bmpimgnr(&files[i]));
		}
		/*
		int dummy;
		QString dummys;
		for(short k=0;k<nrelem-1;k++)
		{
			for(short j=nrelem-1;j>k;j--)
			{
				if(vi[j]>vi[j-1])
				{
					dummy=vi[j];
					vi[j]=vi[j-1];
					vi[j-1]=dummy;
					dummys=files[j];
					files[j]=files[j-1];
					files[j-1]=dummys;
				}
			}
		}*/

		std::vector<const char *> vfilenames;
		for (short i = 0; i < nrelem; i++) {
			vfilenames.push_back(files[i].ascii());
		}

		common::DataSelection dataSelection;
		dataSelection.allSlices = true;
		dataSelection.bmp = true;
		dataSelection.work = true;
		dataSelection.tissues = true;
		emit begin_datachange(dataSelection, this, false);

		LoaderBmp2 LB(handler3D, vfilenames, this);
		LB.move(QCursor::pos());
		LB.exec();

		emit end_datachange(this, common::ClearUndo);

		//	hbox1->setFixedSize(bmphand->return_width()*2+vbox1->sizeHint().width(),bmphand->return_height());

		reset_brightnesscontrast();

		EnableActionsAfterPrjLoaded(true);
	}

	return;
}

void MainWindow::execute_loadpng()
{
	maybeSafe();

	QStringList files =
			Q3FileDialog::getOpenFileNames("Images (*.png)\n"
																		 "All(*.*)",
																		 QString::null, this, "open files dialog",
																		 "Select one or more files to open");

	if (!files.empty()) {
		sort(files.begin(), files.end());

		vector<int> vi;
		vi.clear();

		short nrelem = files.size();

		for (short i = 0; i < nrelem; i++) {
			vi.push_back(pngimgnr(&files[i]));
		}

		std::vector<const char *> vfilenames;
		for (short i = 0; i < nrelem; i++) {
			vfilenames.push_back(files[i].ascii());
		}

		common::DataSelection dataSelection;
		dataSelection.allSlices = true;
		dataSelection.bmp = true;
		dataSelection.work = true;
		dataSelection.tissues = true;
		emit begin_datachange(dataSelection, this, false);

		LoaderPng LB(handler3D, vfilenames, this);
		LB.move(QCursor::pos());
		LB.exec();

		emit end_datachange(this, common::ClearUndo);

		reset_brightnesscontrast();

		EnableActionsAfterPrjLoaded(true);
	}

	return;
}

void MainWindow::execute_loadjpg()
{
	maybeSafe();

	QStringList files =
			Q3FileDialog::getOpenFileNames("Images (*.jpg)\n"
																		 "All(*.*)",
																		 QString::null, this, "open files dialog",
																		 "Select one or more files to open");

	if (!files.empty()) {
		sort(files.begin(), files.end());

		vector<int> vi;
		vi.clear();

		short nrelem = files.size();

		for (short i = 0; i < nrelem; i++) {
			vi.push_back(jpgimgnr(&files[i]));
		}
		/*
		int dummy;
		QString dummys;
		for(short k=0;k<nrelem-1;k++){
			for(short j=nrelem-1;j>k;j--){
				if(vi[j]>vi[j-1]){
					dummy=vi[j];
					vi[j]=vi[j-1];
					vi[j-1]=dummy;
					dummys=files[j];
					files[j]=files[j-1];
					files[j-1]=dummys;
				}
			}
		}*/

		vector<const char *> vfilenames;
		for (short i = 0; i < nrelem; i++) {
			vfilenames.push_back(files[i].ascii());
		}

		common::DataSelection dataSelection;
		dataSelection.allSlices = true;
		dataSelection.bmp = true;
		dataSelection.work = true;
		dataSelection.tissues = true;
		emit begin_datachange(dataSelection, this, false);

		LoaderJpg LB(handler3D, vfilenames, this);
		LB.move(QCursor::pos());
		LB.exec();

		emit end_datachange(this, common::ClearUndo);

		//	hbox1->setFixedSize(bmphand->return_width()*2+vbox1->sizeHint().width(),bmphand->return_height());

		reset_brightnesscontrast();

		EnableActionsAfterPrjLoaded(true);
	}

	return;
}

void MainWindow::execute_loaddicom()
{
	maybeSafe();

	QStringList files =
			Q3FileDialog::getOpenFileNames("Images (*.dcm *.dicom)\n"
																		 "All(*.*)",
																		 QString::null, this, "open files dialog",
																		 "Select one or more files to open");

	if (!files.empty()) {
		common::DataSelection dataSelection;
		dataSelection.allSlices = true;
		dataSelection.bmp = true;
		dataSelection.work = true;
		dataSelection.tissues = true;
		emit begin_datachange(dataSelection, this, false);

		LoaderDicom LD(handler3D, &files, false, this);
		LD.move(QCursor::pos());
		LD.exec();

		emit end_datachange(this, common::ClearUndo);

		pixelsize_changed();
		slicethickness_changed1();

		//	handler3D->LoadDICOM();

		//	work_show->update();//(bmphand->return_width(),bmphand->return_height());
		//	bmp_show->update();//(bmphand->return_width(),bmphand->return_height());
		//	bmp_show->workborder_changed();

		//	hbox1->setFixedSize(bmphand->return_width()*2+vbox1->sizeHint().width(),bmphand->return_height());

		reset_brightnesscontrast();

		EnableActionsAfterPrjLoaded(true);
	}
	return;
}

void MainWindow::execute_reloaddicom()
{
	QStringList files = Q3FileDialog::getOpenFileNames(
			"Images (*.dcm *.dicom)", QString::null, this, "open files dialog",
			"Select one or more files to open");

	if (!files.empty()) {
		common::DataSelection dataSelection;
		dataSelection.allSlices = true;
		dataSelection.bmp = true;
		dataSelection.work = true;
		dataSelection.tissues = true;
		emit begin_datachange(dataSelection, this, false);

		LoaderDicom LD(handler3D, &files, true, this);
		LD.move(QCursor::pos());
		LD.exec();
		pixelsize_changed();
		slicethickness_changed1();

		//	handler3D->LoadDICOM();

		//	work_show->update();//(bmphand->return_width(),bmphand->return_height());
		//	bmp_show->update();//(bmphand->return_width(),bmphand->return_height());
		//	bmp_show->workborder_changed();

		emit end_datachange(this, common::ClearUndo);

		//	hbox1->setFixedSize(bmphand->return_width()*2+vbox1->sizeHint().width(),bmphand->return_height());

		reset_brightnesscontrast();
	}

	return;
}

void MainWindow::execute_loadraw()
{
	maybeSafe();

	common::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this, false);

	LoaderRaw LR(handler3D, this);
	LR.move(QCursor::pos());
	LR.exec();

	//	work_show->update();//(bmphand->return_width(),bmphand->return_height());
	//	bmp_show->update();//(bmphand->return_width(),bmphand->return_height());
	//	bmp_show->workborder_changed();

	emit end_datachange(this, common::ClearUndo);

	//	hbox1->setFixedSize(bmphand->return_width()*2+vbox1->sizeHint().width(),bmphand->return_height());

	reset_brightnesscontrast();

	EnableActionsAfterPrjLoaded(true);
}

void MainWindow::execute_loadmhd()
{
	maybeSafe();

	common::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this, false);

	QString loadfilename =
			Q3FileDialog::getOpenFileName(QString::null,
																		"Metaheader (*.mhd *.mha)\n"
																		"All(*.*)",
																		this);
	if (!loadfilename.isEmpty()) {
		handler3D->ReadImage(loadfilename.ascii());
	}

	emit end_datachange(this, common::ClearUndo);
	pixelsize_changed();
	slicethickness_changed1();

	reset_brightnesscontrast();

	EnableActionsAfterPrjLoaded(true);
}

void MainWindow::execute_loadvtk()
{
	maybeSafe();

	common::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this, false);

	bool res = true;
	QString loadfilename = Q3FileDialog::getOpenFileName(QString::null,
																											 "VTK (*.vti *.vtk)\n"
																											 "All(*.*)",
																											 this);
	if (!loadfilename.isEmpty()) {
		res = handler3D->ReadImage(loadfilename.ascii());
	}

	emit end_datachange(this, common::ClearUndo);
	pixelsize_changed();
	slicethickness_changed1();

	reset_brightnesscontrast();

	EnableActionsAfterPrjLoaded(true);

	if (!res) {
		QMessageBox::warning(this, "iSeg",
												 "Error: Could not load file\n" + loadfilename + "\n",
												 QMessageBox::Ok | QMessageBox::Default);
	}
}

void MainWindow::execute_loadnifti()
{
	maybeSafe();

	common::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this, false);

	QString loadfilename =
			Q3FileDialog::getOpenFileName(QString::null,
																		"NIFTI (*.nii *.hdr *.img *.nii.gz)\n"
																		"All(*.*)",
																		this);
	if (!loadfilename.isEmpty()) {
		handler3D->ReadImage(loadfilename.ascii());
	}

	emit end_datachange(this, common::ClearUndo);
	pixelsize_changed();
	slicethickness_changed1();

	reset_brightnesscontrast();

	EnableActionsAfterPrjLoaded(true);
}

void MainWindow::execute_loadavw()
{
	maybeSafe();

	common::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this, false);

	QString loadfilename = Q3FileDialog::getOpenFileName(QString::null,
																											 "AnalzyeAVW (*.avw)\n"
																											 "All(*.*)",
																											 this);
	if (!loadfilename.isEmpty()) {
		handler3D->ReadAvw(loadfilename.ascii());
	}

	emit end_datachange(this, common::ClearUndo);
	pixelsize_changed();
	slicethickness_changed1();

	reset_brightnesscontrast();

	EnableActionsAfterPrjLoaded(true);
}

void MainWindow::execute_reloadbmp()
{
	QStringList files =
			Q3FileDialog::getOpenFileNames("Images (*.bmp)\n"
																		 "All(*.*)",
																		 QString::null, this, "open files dialog",
																		 "Select one or more files to open");

	if ((unsigned short)files.size() == handler3D->return_nrslices() ||
			(unsigned short)files.size() ==
					(handler3D->return_endslice() - handler3D->return_startslice())) {
		sort(files.begin(), files.end());

		vector<int> vi;
		vi.clear();

		short nrelem = files.size();

		for (short i = 0; i < nrelem; i++) {
			vi.push_back(bmpimgnr(&files[i]));
		}
		/*
		int dummy;
		QString dummys;
		for(short k=0;k<nrelem-1;k++){
			for(short j=nrelem-1;j>k;j--){
				if(vi[j]>vi[j-1]){
					dummy=vi[j];
					vi[j]=vi[j-1];
					vi[j-1]=dummy;
					dummys=files[j];
					files[j]=files[j-1];
					files[j-1]=dummys;
				}
			}
		}*/

		vector<const char *> vfilenames;
		for (short i = 0; i < nrelem; i++) {
			vfilenames.push_back(files[i].ascii());
		}

		common::DataSelection dataSelection;
		dataSelection.allSlices = true;
		dataSelection.bmp = true;
		dataSelection.work = true;
		dataSelection.tissues = true;
		emit begin_datachange(dataSelection, this, false);

		ReloaderBmp2 RB(handler3D, vfilenames, this);
		RB.move(QCursor::pos());
		RB.exec();

		//	work_show->update();//(bmphand->return_width(),bmphand->return_height());
		//	bmp_show->update();//(bmphand->return_width(),bmphand->return_height());
		//	bmp_show->workborder_changed();

		emit end_datachange(this, common::ClearUndo);

		//	hbox1->setFixedSize(bmphand->return_width()*2+vbox1->sizeHint().width(),bmphand->return_height());
		reset_brightnesscontrast();
	}
	else {
		if (!files.empty()) {
			QMessageBox::information(
					this, "Segm. Tool",
					"You have to select the same number of slices.\n");
		}
	}

	return;
}

void MainWindow::execute_reloadraw()
{
	common::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this, false);

	ReloaderRaw RR(handler3D, this);
	RR.move(QCursor::pos());
	RR.exec();

	//	work_show->update();//(bmphand->return_width(),bmphand->return_height());
	//	bmp_show->update();//(bmphand->return_width(),bmphand->return_height());
	//	bmp_show->workborder_changed();

	emit end_datachange(this, common::ClearUndo);
	//	hbox1->setFixedSize(bmphand->return_width()*2+vbox1->sizeHint().width(),bmphand->return_height());

	reset_brightnesscontrast();
}

void MainWindow::execute_reloadavw()
{
	common::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this, false);

	QString loadfilename = Q3FileDialog::getOpenFileName(QString::null,
																											 "AnalzyeAVW (*.avw)\n"
																											 "All(*.*)",
																											 this);
	if (!loadfilename.isEmpty()) {
		handler3D->ReloadAVW(loadfilename.ascii(), handler3D->return_startslice());
		reset_brightnesscontrast();
	}
	emit end_datachange(this, common::ClearUndo);

	return;
}

void MainWindow::execute_reloadmhd()
{
	common::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this, false);

	QString loadfilename =
			Q3FileDialog::getOpenFileName(QString::null,
																		"Metaheader (*.mhd *.mha)\n"
																		"All(*.*)",
																		this);
	if (!loadfilename.isEmpty()) {
		handler3D->ReloadImage(loadfilename.ascii(),
													 handler3D->return_startslice());
		reset_brightnesscontrast();
	}
	emit end_datachange(this, common::ClearUndo);

	return;
}

void MainWindow::execute_reloadvtk()
{
	common::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this, false);

	QString loadfilename = Q3FileDialog::getOpenFileName(QString::null,
																											 "VTK (*.vti *.vtk)\n"
																											 "All(*.*)",
																											 this);
	if (!loadfilename.isEmpty()) {
		handler3D->ReloadImage(loadfilename.ascii(),
													 handler3D->return_startslice());
		reset_brightnesscontrast();
	}
	emit end_datachange(this, common::ClearUndo);

	return;
}

void MainWindow::execute_reloadnifti()
{
	common::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this, false);

	QString loadfilename =
			Q3FileDialog::getOpenFileName(QString::null,
																		"NIFTI (*.nii *.hdr *.img *.nii.gz)\n"
																		"All(*.*)",
																		this);
	if (!loadfilename.isEmpty()) {
		handler3D->ReloadImage(
				loadfilename.ascii(),
				handler3D->return_startslice()); // TODO: handle failure
		reset_brightnesscontrast();
	}
	emit end_datachange(this, common::ClearUndo);

	return;
}

//void MainWindow::execute_loadrtstruct()
//{
//	QString loadfilename = Q3FileDialog::getOpenFileName(QString::null, "RTstruct (*.dcm)\n" "All(*.*)", this);//, filename);
//
//	if(loadfilename.isEmpty()) return;
//
//	gdcmvtk_rtstruct::tissuevec tissues;
//	tissues.clear();
//
//	gdcmvtk_rtstruct::RequestData_RTStructureSetStorage(loadfilename.ascii() , tissues);
//
//	bool *mask=new bool[handler3D->return_area()];
//
//	if(mask==0) return;
//
//	bool undoactive=false;
//	if(handler3D->return_undo3D()&&handler3D->start_undoall(4)){
//		undoactive=true;
//	} else {
//		do_clearundo();
//	}
//
//	tissues_size_t tissuenr;
//	std::string namedummy=std::string("");
//	for(gdcmvtk_rtstruct::tissuevec::iterator it=tissues.begin();it!=tissues.end();it++) {
//		bool caninsert=true;
//		if((*it)->name!=namedummy) {
//			for(tissuenr=0;tissuenr<tissuenames.size()&&(*it)->name!=tissuenames[tissuenr].toStdString();tissuenr++) {}
//			if(tissuenr==(tissues_size_t)tissuenames.size()) {
//				QString name1=QString((*it)->name.c_str());
//				if(tissuenr<TISSUES_SIZE_MAX&&!name1.isEmpty()) {
//					tissuecount++;
//					tissuenames.append(name1);
//					tissuelocked[tissuecount]=false;
//					tissueopac[tissuecount]=0.5;
//					tissuecolor[tissuecount][0]=(*it)->color[0];
//					tissuecolor[tissuecount][1]=(*it)->color[1];
//					tissuecolor[tissuecount][2]=(*it)->color[2];
//					QPixmap abc(10,10);
//					abc.fill(QColor((int)(255*(*it)->color[0]),(int)(255*(*it)->color[1]),(int)(255*(*it)->color[2])));
//					tissueTreeWidget->insertItem(abc,name1);
//				} else caninsert=false;
//			}
//			tissuenr++;
//		}
//		if(caninsert) {
//			Pair p;
//			p=handler3D->get_pixelsize();
//			float thick=handler3D->get_slicethickness();
//			float disp[3];
//			handler3D->get_displacement(disp);
//			size_t pospoints=0;
//			size_t posoutlines=0;
//			std::vector<float *> points;
//			bool clockwisefill=false;
//			while(posoutlines<(*it)->outlinelength.size()) {
//				points.clear();
//				unsigned int *nrpoints=&((*it)->outlinelength[posoutlines]);
//				float zcoord=(*it)->points[pospoints+2];
//				points.push_back(&((*it)->points[pospoints]));
//				pospoints+=(*it)->outlinelength[posoutlines]*3;
//				posoutlines++;
//				while(posoutlines<(*it)->outlinelength.size()&&zcoord==(*it)->points[pospoints+2]) {
//					points.push_back(&((*it)->points[pospoints]));
//					pospoints+=(*it)->outlinelength[posoutlines]*3;
//					posoutlines++;
//				}
//				int slicenr=handler3D->return_nrslices()-floor((zcoord-disp[2])/thick)-1;
//				if(slicenr>=handler3D->return_startslice()&&slicenr<handler3D->return_endslice()) {
//					fillcontours::fill_contour(mask, handler3D->return_width(), handler3D->return_height(), disp[0], disp[1], p.high, p.low, &(points[0]), nrpoints, points.size(), clockwisefill);
//					handler3D->add2tissue(tissuenr,mask,(unsigned short)slicenr,true);
//				}
//			}
//		}
//	}
//
//	if(undoactive){
//		do_undostepdone();
//	}
//
//	tissuenr_changed(tissueTreeWidget->get_current_type());
//	emit tissues_changed();
//
//	delete[] mask;
//}

void MainWindow::execute_loadsurface()
{
	maybeSafe();

	bool ok = true;
	QString loadfilename = Q3FileDialog::getOpenFileName(QString::null,
																											 "STL (*.stl)\n"
																											 "All(*.*)",
																											 this);
	if (!loadfilename.isEmpty()) {
		QMessageBox msgBox;
		msgBox.setWindowTitle("Import Surface");
		msgBox.setText("What would you like to do what with the imported surface?");
		msgBox.addButton("Overwrite", QMessageBox::YesRole); //==0
		msgBox.addButton("Add", QMessageBox::NoRole);				 //==1
		msgBox.addButton("Cancel", QMessageBox::RejectRole); //==2
		int overwrite = msgBox.exec();

		if (overwrite == 2)
			return;

		ok = handler3D->LoadSurface(loadfilename.ascii(), overwrite == 0);
	}

	reset_brightnesscontrast();

	if (!ok) {
		QMessageBox::warning(this, "iSeg",
												 "Error: Surface does not overlap with image",
												 QMessageBox::Ok | QMessageBox::Default);
	}
}

void MainWindow::execute_loadrtstruct()
{
#ifndef NORTSTRUCTSUPPORT
	QString loadfilename = Q3FileDialog::getOpenFileName(QString::null,
																											 "RTstruct (*.dcm)\n"
																											 "All(*.*)",
																											 this); //, filename);

	if (loadfilename.isEmpty()) {
		return;
	}

	RtstructImport RI(loadfilename, handler3D, this);

	QObject::connect(
			&RI, SIGNAL(begin_datachange(common::DataSelection &, QWidget *, bool)),
			this,
			SLOT(handle_begin_datachange(common::DataSelection &, QWidget *, bool)));
	QObject::connect(
			&RI, SIGNAL(end_datachange(QWidget *, common::EndUndoAction)), this,
			SLOT(handle_end_datachange(QWidget *, common::EndUndoAction)));

	RI.move(QCursor::pos());
	RI.exec();

	tissueTreeWidget->update_tree_widget();
	tissuenr_changed(tissueTreeWidget->get_current_type() - 1);

	QObject::disconnect(
			&RI, SIGNAL(begin_datachange(common::DataSelection &, QWidget *, bool)),
			this,
			SLOT(handle_begin_datachange(common::DataSelection &, QWidget *, bool)));
	QObject::disconnect(
			&RI, SIGNAL(end_datachange(QWidget *, common::EndUndoAction)), this,
			SLOT(handle_end_datachange(QWidget *, common::EndUndoAction)));

	reset_brightnesscontrast();
#endif
}

void MainWindow::execute_loadrtdose()
{
#ifndef NORTDOSESUPPORT

	maybeSafe();

	common::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this, false);

	QString loadfilename = Q3FileDialog::getOpenFileName(QString::null,
																											 "RTdose (*.dcm)\n"
																											 "All(*.*)",
																											 this);
	if (!loadfilename.isEmpty()) {
		handler3D->ReadRTdose(loadfilename.ascii());
	}

	emit end_datachange(this, common::ClearUndo);
	pixelsize_changed();
	slicethickness_changed1();

	reset_brightnesscontrast();

	EnableActionsAfterPrjLoaded(true);
#endif
}

void MainWindow::execute_reloadrtdose()
{
#ifndef NORTDOSESUPPORT
	common::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this, false);

	QString loadfilename = Q3FileDialog::getOpenFileName(QString::null,
																											 "RTdose (*.dcm)\n"
																											 "All(*.*)",
																											 this);
	if (!loadfilename.isEmpty()) {
		handler3D->ReloadRTdose(
				loadfilename.ascii(),
				handler3D->return_startslice()); // TODO: handle failure
		reset_brightnesscontrast();
	}
	emit end_datachange(this, common::ClearUndo);

	return;
#endif
}

void MainWindow::execute_loads4llivelink()
{
	QString loadfilename = Q3FileDialog::getOpenFileName(QString::null,
																											 "S4L Link (*.h5)\n"
																											 "All(*.*)",
																											 this);
	if (!loadfilename.isEmpty()) {
		loadS4Llink(loadfilename);
	}
}

/*
void MainWindow::execute_loadrtss()
{
	common::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this, false);

	QString loadfilename = Q3FileDialog::getOpenFileName(QString::null, "RTSS (*.dcm)\n" "All(*.*)", this);
	if(!loadfilename.isEmpty()){
		//handler3D->ReloadRTdose(loadfilename.ascii(),handler3D->return_startslice()); // TODO: handle failure
		handler3D->ReadRTSS(loadfilename.ascii());
		reset_brightnesscontrast();
	}
	emit end_datachange(this, common::ClearUndo);

	return;
}
*/

void MainWindow::execute_saveimg()
{
	common::DataSelection dataSelection;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	emit begin_dataexport(dataSelection, this);

	SaverImg SI(handler3D, this);
	SI.move(QCursor::pos());
	SI.exec();

	//	work_show->update();//(bmphand->return_width(),bmphand->return_height());
	//	bmp_show->update();//(bmphand->return_width(),bmphand->return_height());
	//	hbox1->setFixedSize(bmphand->return_width()*2+vbox1->sizeHint().width(),bmphand->return_height());

	emit end_dataexport(this);
}

void MainWindow::execute_saveprojas()
{
	common::DataSelection dataSelection;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	dataSelection.tissueHierarchy = true;
	emit begin_dataexport(dataSelection, this);

	QString savefilename = Q3FileDialog::getSaveFileName(
			QString::null, "Projects (*.prj)\n", this); //, filename);

	if (!savefilename.isEmpty()) {
		if (savefilename.length() <= 4 || !savefilename.endsWith(QString(".prj")))
			savefilename.append(".prj");

		m_saveprojfilename = savefilename;

		//Append "Temp" at the end of the file name and rename it at the end of the successful saving process
		QString tempFileName = QString(savefilename);
		int afterDot = tempFileName.lastIndexOf('.');
		if (afterDot != 0)
			tempFileName =
					tempFileName.remove(afterDot, tempFileName.length() - afterDot) +
					"Temp.prj";
		else
			tempFileName = tempFileName + "Temp.prj";

		QString sourceFileNameWithoutExtension;
		afterDot = savefilename.lastIndexOf('.');
		if (afterDot != -1)
			sourceFileNameWithoutExtension = savefilename.mid(0, afterDot);

		QString tempFileNameWithoutExtension;
		afterDot = tempFileName.lastIndexOf('.');
		if (afterDot != -1)
			tempFileNameWithoutExtension = tempFileName.mid(0, afterDot);

		int numTasks = 3;
		QProgressDialog progress("Save in progress...", "Cancel", 0, numTasks,
														 this);
		progress.show();
		QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
		progress.setWindowModality(Qt::WindowModal);
		progress.setModal(true);
		progress.setValue(1);

		setCaption(QString(" iSeg ") + QString::number(iSegVersion) + QString(".") +
							 QString::number(iSegSubversion) + QString(" B") +
							 QString::number(build) + QString(" - ") +
							 TruncateFileName(savefilename));

		//m_saveprojfilename = tempFileName;
		//AddLoadProj(tempFileName);
		AddLoadProj(m_saveprojfilename);
		FILE *fp = handler3D->SaveProject(tempFileName.ascii(), "xmf");
		fp = bitstack_widget->save_proj(fp);
		unsigned short saveProjVersion = 12;
		fp = TissueInfos::SaveTissues(fp, saveProjVersion);
		unsigned short combinedVersion =
				(unsigned short)common::CombineTissuesVersion(saveProjVersion, 1);
		fwrite(&combinedVersion, 1, sizeof(unsigned short), fp);
		/*for(size_t i=0;i<tabwidgets.size();i++){
			fp=((QWidget1 *)(tabwidgets[i]))->SaveParams(fp,saveProjVersion);
		}*/
		threshold_widget->SaveParams(fp, saveProjVersion);
		hyst_widget->SaveParams(fp, saveProjVersion);
		lw_widget->SaveParams(fp, saveProjVersion);
		iftrg_widget->SaveParams(fp, saveProjVersion);
		FMF_widget->SaveParams(fp, saveProjVersion);
		wshed_widget->SaveParams(fp, saveProjVersion);
		OutlineCorrect_widget->SaveParams(fp, saveProjVersion);
		interpolwidget->SaveParams(fp, saveProjVersion);
		smoothing_widget->SaveParams(fp, saveProjVersion);
		morph_widget->SaveParams(fp, saveProjVersion);
		edge_widg->SaveParams(fp, saveProjVersion);
		feature_widget->SaveParams(fp, saveProjVersion);
		measurement_widget->SaveParams(fp, saveProjVersion);
		vesselextr_widget->SaveParams(fp, saveProjVersion);
		pickerwidget->SaveParams(fp, saveProjVersion);
		tissueTreeWidget->SaveParams(fp, saveProjVersion);
		fp = TissueInfos::SaveTissueLocks(fp);
		fp = save_notes(fp, saveProjVersion);

		fclose(fp);

		progress.setValue(2);

		if (QFile::exists(sourceFileNameWithoutExtension + ".xmf"))
			QFile::remove(sourceFileNameWithoutExtension + ".xmf");
		QFile::rename(tempFileNameWithoutExtension + ".xmf",
									sourceFileNameWithoutExtension + ".xmf");

		if (QFile::exists(sourceFileNameWithoutExtension + ".prj"))
			QFile::remove(sourceFileNameWithoutExtension + ".prj");
		QFile::rename(tempFileNameWithoutExtension + ".prj",
									sourceFileNameWithoutExtension + ".prj");

		if (QFile::exists(sourceFileNameWithoutExtension + ".h5"))
			QFile::remove(sourceFileNameWithoutExtension + ".h5");
		QFile::rename(tempFileNameWithoutExtension + ".h5",
									sourceFileNameWithoutExtension + ".h5");

		progress.setValue(numTasks);
	}
	emit end_dataexport(this);
}

void MainWindow::execute_savecopyas()
{
	common::DataSelection dataSelection;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	dataSelection.tissueHierarchy = true;
	emit begin_dataexport(dataSelection, this);

	QString savefilename = Q3FileDialog::getSaveFileName(
			QString::null, "Projects (*.prj)\n", this); //, filename);

	if (!savefilename.isEmpty()) {
		if (savefilename.length() <= 4 || !savefilename.endsWith(QString(".prj")))
			savefilename.append(".prj");

		FILE *fp = handler3D->SaveProject(savefilename.ascii(), "xmf");
		fp = bitstack_widget->save_proj(fp);
		unsigned short saveProjVersion = 12;
		fp = TissueInfos::SaveTissues(fp, saveProjVersion);
		unsigned short combinedVersion =
				(unsigned short)common::CombineTissuesVersion(saveProjVersion, 1);
		fwrite(&combinedVersion, 1, sizeof(unsigned short), fp);
		/*for(size_t i=0;i<tabwidgets.size();i++){
			fp=((QWidget1 *)(tabwidgets[i]))->SaveParams(fp,saveProjVersion);
		}*/
		threshold_widget->SaveParams(fp, saveProjVersion);
		hyst_widget->SaveParams(fp, saveProjVersion);
		lw_widget->SaveParams(fp, saveProjVersion);
		iftrg_widget->SaveParams(fp, saveProjVersion);
		FMF_widget->SaveParams(fp, saveProjVersion);
		wshed_widget->SaveParams(fp, saveProjVersion);
		OutlineCorrect_widget->SaveParams(fp, saveProjVersion);
		interpolwidget->SaveParams(fp, saveProjVersion);
		smoothing_widget->SaveParams(fp, saveProjVersion);
		morph_widget->SaveParams(fp, saveProjVersion);
		edge_widg->SaveParams(fp, saveProjVersion);
		feature_widget->SaveParams(fp, saveProjVersion);
		measurement_widget->SaveParams(fp, saveProjVersion);
		vesselextr_widget->SaveParams(fp, saveProjVersion);
		pickerwidget->SaveParams(fp, saveProjVersion);
		tissueTreeWidget->SaveParams(fp, saveProjVersion);
		fp = TissueInfos::SaveTissueLocks(fp);
		fp = save_notes(fp, saveProjVersion);

		fclose(fp);
	}
	emit end_dataexport(this);
}

void MainWindow::SaveSettings()
{
	FILE *fp = fopen(settingsfile.c_str(), "wb");
	if (fp == NULL)
		return;
	unsigned short saveProjVersion = 12;
	unsigned short combinedVersion =
			(unsigned short)common::CombineTissuesVersion(saveProjVersion, 1);
	fwrite(&combinedVersion, 1, sizeof(unsigned short), fp);
	bool flag;
	flag = QWidget1::get_hideparams();
	fwrite(&flag, 1, sizeof(bool), fp);
	//	flag=!hidestack->isOn();
	flag = false;
	fwrite(&flag, 1, sizeof(bool), fp);
	//	flag=!hidenotes->isOn();
	flag = false;
	fwrite(&flag, 1, sizeof(bool), fp);
	//	flag=!hidezoom->isOn();
	flag = false;
	fwrite(&flag, 1, sizeof(bool), fp);
	flag = !hidecontrastbright->isOn();
	fwrite(&flag, 1, sizeof(bool), fp);
	flag = !hidecopyswap->isOn();
	fwrite(&flag, 1, sizeof(bool), fp);
	for (unsigned short i = 0; i < 16; i++) {
		flag = showtab_action[i]->isOn();
		fwrite(&flag, 1, sizeof(bool), fp);
	}
	fp = TissueInfos::SaveTissues(fp, saveProjVersion);
	/*for(size_t i=0;i<tabwidgets.size();i++){
		fp=((QWidget1 *)(tabwidgets[i]))->SaveParams(fp,saveProjVersion);
	}*/
	threshold_widget->SaveParams(fp, saveProjVersion);
	hyst_widget->SaveParams(fp, saveProjVersion);
	lw_widget->SaveParams(fp, saveProjVersion);
	iftrg_widget->SaveParams(fp, saveProjVersion);
	FMF_widget->SaveParams(fp, saveProjVersion);
	wshed_widget->SaveParams(fp, saveProjVersion);
	OutlineCorrect_widget->SaveParams(fp, saveProjVersion);
	interpolwidget->SaveParams(fp, saveProjVersion);
	smoothing_widget->SaveParams(fp, saveProjVersion);
	morph_widget->SaveParams(fp, saveProjVersion);
	edge_widg->SaveParams(fp, saveProjVersion);
	feature_widget->SaveParams(fp, saveProjVersion);
	measurement_widget->SaveParams(fp, saveProjVersion);
	vesselextr_widget->SaveParams(fp, saveProjVersion);
	pickerwidget->SaveParams(fp, saveProjVersion);
	tissueTreeWidget->SaveParams(fp, saveProjVersion);

	fclose(fp);

	QSettings settings(QSettings::IniFormat, QSettings::UserScope, "ZMT", "iSeg");
	settings.beginGroup("MainWindow");
	settings.setValue("geometry", saveGeometry());
	settings.setValue("state", saveState());
	settings.setValue("NumberOfUndoSteps",
										this->handler3D->GetNumberOfUndoSteps());
	settings.setValue("NumberOfUndoArrays",
										this->handler3D->GetNumberOfUndoArrays());
	settings.setValue("Compression", this->handler3D->GetCompression());
	settings.setValue("ContiguousMemory", this->handler3D->GetContiguousMemory());
	settings.endGroup();
	settings.sync();
}

void MainWindow::LoadSettings(const char *loadfilename)
{
	settingsfile = loadfilename;
	FILE *fp;
	if ((fp = fopen(loadfilename, "rb")) == NULL) {
		return;
	}

	unsigned short combinedVersion = 0;
	if (fread(&combinedVersion, sizeof(unsigned short), 1, fp) == 0) {
		fclose(fp);
		return;
	}
	int loadProjVersion, tissuesVersion;
	common::ExtractTissuesVersion((int)combinedVersion, loadProjVersion,
																tissuesVersion);

	common::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this, false);

	bool flag;
	fread(&flag, sizeof(bool), 1, fp);
	execute_hideparameters(flag);
	hideparameters->setOn(flag);
	fread(&flag, sizeof(bool), 1, fp);
	//	hidestack->setOn(!flag);
	//	execute_hidestack(!flag);

	if (loadProjVersion >= 7) {
		fread(&flag, sizeof(bool), 1, fp);
	}
	else {
		flag = false;
	}
	//	hidenotes->setOn(!flag);
	//	execute_hidenotes(!flag);
	fread(&flag, sizeof(bool), 1, fp);
	//	hidezoom->setOn(!flag);
	//	execute_hidezoom(!flag);
	fread(&flag, sizeof(bool), 1, fp);
	hidecontrastbright->setOn(!flag);
	execute_hidecontrastbright(!flag);
	fread(&flag, sizeof(bool), 1, fp);
	hidecopyswap->setOn(!flag);
	execute_hidecopyswap(!flag);
	for (unsigned short i = 0; i < 14; i++) {
		fread(&flag, sizeof(bool), 1, fp);
		showtab_action[i]->setOn(flag);
	}
	if (loadProjVersion >= 6) {
		fread(&flag, sizeof(bool), 1, fp);
		showtab_action[14]->setOn(flag);
	}
	if (loadProjVersion >= 9) {
		fread(&flag, sizeof(bool), 1, fp);
		showtab_action[15]->setOn(flag);
	}
	//New added. Show all loaded widgets
	for (int i = 16; i < nrtabbuttons; i++) {
		showtab_action[i]->setOn(flag);
	}

	execute_showtabtoggled(flag);

	fp = TissueInfos::LoadTissues(fp, tissuesVersion);
	const char *defaultTissuesFilename =
			m_tmppath.absFilePath(QString("def_tissues.txt"));
	FILE *fpTmp = fopen(defaultTissuesFilename, "r");
	if (fpTmp != NULL || TissueInfos::GetTissueCount() <= 0) {
		if (fpTmp != NULL)
			fclose(fpTmp);
		TissueInfos::LoadDefaultTissueList(defaultTissuesFilename);
	}
	tissueTreeWidget->update_tree_widget();
	emit end_datachange(this, common::ClearUndo);
	tissuenr_changed(tissueTreeWidget->get_current_type() - 1);

	/*for(size_t i=0;i<tabwidgets.size();i++){
		fp=((QWidget1 *)(tabwidgets[i]))->LoadParams(fp,version);
	}*/
	threshold_widget->LoadParams(fp, loadProjVersion);
	hyst_widget->LoadParams(fp, loadProjVersion);
	lw_widget->LoadParams(fp, loadProjVersion);
	iftrg_widget->LoadParams(fp, loadProjVersion);
	FMF_widget->LoadParams(fp, loadProjVersion);
	wshed_widget->LoadParams(fp, loadProjVersion);
	OutlineCorrect_widget->LoadParams(fp, loadProjVersion);
	interpolwidget->LoadParams(fp, loadProjVersion);
	smoothing_widget->LoadParams(fp, loadProjVersion);
	morph_widget->LoadParams(fp, loadProjVersion);
	edge_widg->LoadParams(fp, loadProjVersion);
	feature_widget->LoadParams(fp, loadProjVersion);
	measurement_widget->LoadParams(fp, loadProjVersion);
	vesselextr_widget->LoadParams(fp, loadProjVersion);
	pickerwidget->LoadParams(fp, loadProjVersion);
	tissueTreeWidget->LoadParams(fp, loadProjVersion);
	fclose(fp);

	if (loadProjVersion > 7) {
		cerr << "LoadSettings() : restoring values..." << endl;
		QSettings settings(QSettings::IniFormat, QSettings::UserScope, "ZMT",
											 "iSeg");
		settings.beginGroup("MainWindow");
		restoreGeometry(settings.value("geometry").toByteArray());
		restoreState(settings.value("state").toByteArray());
		this->handler3D->SetNumberOfUndoSteps(
				settings.value("NumberOfUndoSteps", 50).toUInt());
		cerr << "NumberOfUndoSteps = " << this->handler3D->GetNumberOfUndoSteps()
				 << endl;
		this->handler3D->SetNumberOfUndoArrays(
				settings.value("NumberOfUndoArrays", 20).toUInt());
		cerr << "NumberOfUndoArrays = " << this->handler3D->GetNumberOfUndoArrays()
				 << endl;
		this->handler3D->SetCompression(settings.value("Compression", 0).toInt());
		cerr << "Compression = " << this->handler3D->GetCompression() << endl;
		this->handler3D->SetContiguousMemory(
				settings.value("ContiguousMemory", true).toBool());
		cerr << "ContiguousMemory = " << this->handler3D->GetContiguousMemory()
				 << endl;
		settings.endGroup();

		if (this->handler3D->return_nrundo() == 0)
			this->editmenu->setItemEnabled(undonr, false);
		else
			editmenu->setItemEnabled(undonr, true);
	}
}

void MainWindow::execute_saveactiveslicesas()
{
	common::DataSelection dataSelection;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	dataSelection.tissueHierarchy = true;
	emit begin_dataexport(dataSelection, this);

	QString savefilename = Q3FileDialog::getSaveFileName(
			QString::null, "Projects (*.prj)\n", this); //, filename);

	if (savefilename.length() <= 4 || !savefilename.endsWith(QString(".prj")))
		savefilename.append(".prj");

	if (!savefilename.isEmpty()) {
		FILE *fp = handler3D->SaveActiveSlices(savefilename.ascii(), "xmf");
		fp = bitstack_widget->save_proj(fp);
		unsigned short saveProjVersion = 12;
		fp = TissueInfos::SaveTissues(fp, saveProjVersion);
		unsigned short combinedVersion =
				(unsigned short)common::CombineTissuesVersion(saveProjVersion, 1);
		fwrite(&combinedVersion, 1, sizeof(unsigned short), fp);
		/*for(size_t i=0;i<tabwidgets.size();i++){
			fp=((QWidget1 *)(tabwidgets[i]))->SaveParams(fp,saveProjVersion);
		}*/
		threshold_widget->SaveParams(fp, saveProjVersion);
		hyst_widget->SaveParams(fp, saveProjVersion);
		lw_widget->SaveParams(fp, saveProjVersion);
		iftrg_widget->SaveParams(fp, saveProjVersion);
		FMF_widget->SaveParams(fp, saveProjVersion);
		wshed_widget->SaveParams(fp, saveProjVersion);
		OutlineCorrect_widget->SaveParams(fp, saveProjVersion);
		interpolwidget->SaveParams(fp, saveProjVersion);
		smoothing_widget->SaveParams(fp, saveProjVersion);
		morph_widget->SaveParams(fp, saveProjVersion);
		edge_widg->SaveParams(fp, saveProjVersion);
		feature_widget->SaveParams(fp, saveProjVersion);
		measurement_widget->SaveParams(fp, saveProjVersion);
		vesselextr_widget->SaveParams(fp, saveProjVersion);
		pickerwidget->SaveParams(fp, saveProjVersion);
		tissueTreeWidget->SaveParams(fp, saveProjVersion);
		fp = TissueInfos::SaveTissueLocks(fp);
		fp = save_notes(fp, saveProjVersion);

		fclose(fp);
	}
	emit end_dataexport(this);
}

void MainWindow::execute_saveproj()
{
	common::DataSelection dataSelection;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	dataSelection.tissueHierarchy = true;
	emit begin_dataexport(dataSelection, this);

	if (m_editingmode) {
		if (!m_S4Lcommunicationfilename.isEmpty()) {
			handler3D->SaveCommunicationFile(m_S4Lcommunicationfilename.ascii());
		}
	}
	else {
		if (!m_saveprojfilename.isEmpty()) {
			//Append "Temp" at the end of the file name and rename it at the end of the successful saving process
			QString tempFileName = QString(m_saveprojfilename);
			int afterDot = tempFileName.lastIndexOf('.');
			if (afterDot != 0)
				tempFileName =
						tempFileName.remove(afterDot, tempFileName.length() - afterDot) +
						"Temp.prj";
			else
				tempFileName = tempFileName + "Temp.prj";

			string pjFN = m_saveprojfilename.toStdString();

			QString sourceFileNameWithoutExtension;
			afterDot = m_saveprojfilename.lastIndexOf('.');
			if (afterDot != -1)
				sourceFileNameWithoutExtension = m_saveprojfilename.mid(0, afterDot);

			QString tempFileNameWithoutExtension;
			afterDot = tempFileName.lastIndexOf('.');
			if (afterDot != -1)
				tempFileNameWithoutExtension = tempFileName.mid(0, afterDot);

			int numTasks = 3;
			QProgressDialog progress("Save in progress...", "Cancel", 0, numTasks,
															 this);
			progress.show();
			QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
			progress.setWindowModality(Qt::WindowModal);
			progress.setModal(true);
			progress.setValue(1);

			setCaption(QString(" iSeg ") + QString::number(iSegVersion) +
								 QString(".") + QString::number(iSegSubversion) +
								 QString(" B") + QString::number(build) + QString(" - ") +
								 TruncateFileName(m_saveprojfilename));

			m_saveprojfilename = tempFileName;

			//FILE *fp=handler3D->SaveProject(m_saveprojfilename.ascii(),"xmf");
			FILE *fp = handler3D->SaveProject(tempFileName.ascii(), "xmf");
			fp = bitstack_widget->save_proj(fp);
			unsigned short saveProjVersion = 12;
			fp = TissueInfos::SaveTissues(fp, saveProjVersion);
			unsigned short combinedVersion =
					(unsigned short)common::CombineTissuesVersion(saveProjVersion, 1);
			fwrite(&combinedVersion, 1, sizeof(unsigned short), fp);
			/*for(size_t i=0;i<tabwidgets.size();i++){
				fp=((QWidget1 *)(tabwidgets[i]))->SaveParams(fp,saveProjVersion);
			}*/
			threshold_widget->SaveParams(fp, saveProjVersion);
			hyst_widget->SaveParams(fp, saveProjVersion);
			lw_widget->SaveParams(fp, saveProjVersion);
			iftrg_widget->SaveParams(fp, saveProjVersion);
			FMF_widget->SaveParams(fp, saveProjVersion);
			wshed_widget->SaveParams(fp, saveProjVersion);
			OutlineCorrect_widget->SaveParams(fp, saveProjVersion);
			interpolwidget->SaveParams(fp, saveProjVersion);
			smoothing_widget->SaveParams(fp, saveProjVersion);
			morph_widget->SaveParams(fp, saveProjVersion);
			edge_widg->SaveParams(fp, saveProjVersion);
			feature_widget->SaveParams(fp, saveProjVersion);
			measurement_widget->SaveParams(fp, saveProjVersion);
			vesselextr_widget->SaveParams(fp, saveProjVersion);
			pickerwidget->SaveParams(fp, saveProjVersion);
			tissueTreeWidget->SaveParams(fp, saveProjVersion);
			fp = TissueInfos::SaveTissueLocks(fp);
			fp = save_notes(fp, saveProjVersion);

			fclose(fp);

			progress.setValue(2);

			QMessageBox mBox;
			mBox.setWindowTitle("Saving project");
			mBox.setText("The project you are trying to save is open somewhere else. "
									 "Please, close it before continuing and press OK or press "
									 "Cancel to stop saving process.");
			mBox.addButton(QMessageBox::Ok);
			mBox.addButton(QMessageBox::Cancel);

			if (QFile::exists(sourceFileNameWithoutExtension + ".xmf")) {
				bool removeSuccess =
						QFile::remove(sourceFileNameWithoutExtension + ".xmf");
				while (!removeSuccess) {
					int ret = mBox.exec();
					if (ret == QMessageBox::Cancel)
						return;

					removeSuccess =
							QFile::remove(sourceFileNameWithoutExtension + ".xmf");
				}
			}
			QFile::rename(tempFileNameWithoutExtension + ".xmf",
										sourceFileNameWithoutExtension + ".xmf");

			if (QFile::exists(sourceFileNameWithoutExtension + ".prj")) {
				bool removeSuccess =
						QFile::remove(sourceFileNameWithoutExtension + ".prj");
				while (!removeSuccess) {
					int ret = mBox.exec();
					if (ret == QMessageBox::Cancel)
						return;

					removeSuccess =
							QFile::remove(sourceFileNameWithoutExtension + ".prj");
				}
			}
			QFile::rename(tempFileNameWithoutExtension + ".prj",
										sourceFileNameWithoutExtension + ".prj");

			if (QFile::exists(sourceFileNameWithoutExtension + ".h5")) {
				bool removeSuccess =
						QFile::remove(sourceFileNameWithoutExtension + ".h5");
				while (!removeSuccess) {
					int ret = mBox.exec();
					if (ret == QMessageBox::Cancel)
						return;

					removeSuccess = QFile::remove(sourceFileNameWithoutExtension + ".h5");
				}
			}
			QFile::rename(tempFileNameWithoutExtension + ".h5",
										sourceFileNameWithoutExtension + ".h5");

			m_saveprojfilename = sourceFileNameWithoutExtension + ".prj";

			progress.setValue(numTasks);
		}
		else {
			if (modified()) {
				execute_saveprojas();
			}
		}
	}
	emit end_dataexport(this);
}

void MainWindow::loadproj(const QString &loadfilename)
{
	FILE *fp;
	if ((fp = fopen(loadfilename.ascii(), "r")) == NULL) {
		return;
	}
	else {
		fclose(fp);
	}
	bool stillopen = false;

	common::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this, false);

	if (!loadfilename.isEmpty()) {
		m_saveprojfilename = loadfilename;
		setCaption(QString(" iSeg ") + QString::number(iSegVersion) + QString(".") +
							 QString::number(iSegSubversion) + QString(" B") +
							 QString::number(build) + QString(" - ") +
							 TruncateFileName(loadfilename));
		AddLoadProj(m_saveprojfilename);
		int tissuesVersion = 0;
		fp = handler3D->LoadProject(loadfilename.ascii(), tissuesVersion);
		fp = bitstack_widget->load_proj(fp);
		fp = TissueInfos::LoadTissues(fp, tissuesVersion);
		stillopen = true;
	}

	emit end_datachange(this, common::ClearUndo);
	tissuenr_changed(tissueTreeWidget->get_current_type() - 1);

	//if(xsliceshower!=NULL) xsliceshower->thickness_changed(handler3D->get_slicethickness());
	//if(ysliceshower!=NULL) ysliceshower->thickness_changed(handler3D->get_slicethickness());
	pixelsize_changed();
	/*if(xsliceshower!=NULL) xsliceshower->pixelsize_changed(handler3D->get_pixelsize());
	if(ysliceshower!=NULL) ysliceshower->pixelsize_changed(handler3D->get_pixelsize());
	bmp_show->pixelsize_changed(handler3D->get_pixelsize());
	work_show->pixelsize_changed(handler3D->get_pixelsize());*/
	slicethickness_changed1();

	if (stillopen) {
		unsigned short combinedVersion = 0;
		if (fread(&combinedVersion, sizeof(unsigned short), 1, fp) > 0) {
			int loadProjVersion, tissuesVersion;
			common::ExtractTissuesVersion((int)combinedVersion, loadProjVersion,
																		tissuesVersion);
			/*for(size_t i=0;i<tabwidgets.size();i++){
				fp=((QWidget1 *)(tabwidgets[i]))->LoadParams(fp,version);
			}*/
			threshold_widget->LoadParams(fp, loadProjVersion);
			hyst_widget->LoadParams(fp, loadProjVersion);
			lw_widget->LoadParams(fp, loadProjVersion);
			iftrg_widget->LoadParams(fp, loadProjVersion);
			FMF_widget->LoadParams(fp, loadProjVersion);
			wshed_widget->LoadParams(fp, loadProjVersion);
			OutlineCorrect_widget->LoadParams(fp, loadProjVersion);
			interpolwidget->LoadParams(fp, loadProjVersion);
			smoothing_widget->LoadParams(fp, loadProjVersion);
			morph_widget->LoadParams(fp, loadProjVersion);
			edge_widg->LoadParams(fp, loadProjVersion);
			feature_widget->LoadParams(fp, loadProjVersion);
			measurement_widget->LoadParams(fp, loadProjVersion);
			vesselextr_widget->LoadParams(fp, loadProjVersion);
			pickerwidget->LoadParams(fp, loadProjVersion);
			tissueTreeWidget->LoadParams(fp, loadProjVersion);

			if (loadProjVersion >= 3) {
				fp = TissueInfos::LoadTissueLocks(fp);
				tissues_size_t currTissueType = tissueTreeWidget->get_current_type();
				cb_tissuelock->setChecked(
						TissueInfos::GetTissueLocked(currTissueType + 1));
				tissueTreeWidget->update_tissue_icons();
				tissueTreeWidget->update_folder_icons();
			}
			fp = load_notes(fp, loadProjVersion);
		}

		//		le_slicethick->setText(QString::number(handler3D->get_slicethickness()));

		fclose(fp);
	}

	reset_brightnesscontrast();

	EnableActionsAfterPrjLoaded(true);
	//m_S4Lcommunicationfilename=QString("F:\\applications\\bifurcation\\bifurcation2.h5");xxxa
}

void MainWindow::loadS4Llink(const QString &loadfilename)
{
	bool stillopen = false;

	common::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this, false);

	if (!loadfilename.isEmpty()) {
		m_S4Lcommunicationfilename = loadfilename;
		int tissuesVersion = 0;
		handler3D->LoadS4Llink(loadfilename.ascii(), tissuesVersion);
		TissueInfos::LoadTissuesHDF(loadfilename.ascii(), tissuesVersion);
		stillopen = true;
	}

	emit end_datachange(this, common::ClearUndo);
	tissues_size_t m;
	handler3D->get_rangetissue(&m);
	handler3D->buildmissingtissues(m);
	tissueTreeWidget->update_tree_widget();
	tissuenr_changed(tissueTreeWidget->get_current_type() - 1);

	pixelsize_changed();
	slicethickness_changed1();

	if (stillopen) {
		tissueTreeWidget->update_tissue_icons();
		tissueTreeWidget->update_folder_icons();
	}

	reset_brightnesscontrast();
}

void MainWindow::execute_mergeprojects()
{
	common::DataSelection dataSelection;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	dataSelection.tissueHierarchy = true;
	emit begin_dataexport(dataSelection, this);

	// Get list of project file names to be merged
	MergeProjectsDialog mergeDialog(this, "Merge Projects");
	if (mergeDialog.exec() != QDialog::Accepted) {
		emit end_dataexport(this);
		return;
	}
	std::vector<QString> mergefilenames;
	mergeDialog.get_filenames(mergefilenames);

	// Get save file name
	QString savefilename =
			Q3FileDialog::getSaveFileName(QString::null, "Projects (*.prj)", this,
																		"iSeg", "Save merged project as");
	if (savefilename.length() <= 4 || !savefilename.endsWith(QString(".prj")))
		savefilename.append(".prj");

	if (!savefilename.isEmpty() && mergefilenames.size() > 0) {
		FILE *fp = handler3D->MergeProjects(savefilename.ascii(), mergefilenames);
		if (!fp) {
			QMessageBox::warning(this, "iSeg",
													 "Merge projects failed.\n\nPlease make sure that "
													 "all projects have the same xy extents\nand their "
													 "image data is contained in a .h5 and .xmf file.",
													 QMessageBox::Ok | QMessageBox::Default);
			return;
		}
		fp = bitstack_widget->save_proj(fp);
		unsigned short saveProjVersion = 12;
		fp = TissueInfos::SaveTissues(fp, saveProjVersion);
		unsigned short combinedVersion =
				(unsigned short)common::CombineTissuesVersion(saveProjVersion, 1);
		fwrite(&combinedVersion, 1, sizeof(unsigned short), fp);
		/*for(size_t i=0;i<tabwidgets.size();i++){
			fp=((QWidget1 *)(tabwidgets[i]))->SaveParams(fp,saveProjVersion);
		}*/
		threshold_widget->SaveParams(fp, saveProjVersion);
		hyst_widget->SaveParams(fp, saveProjVersion);
		lw_widget->SaveParams(fp, saveProjVersion);
		iftrg_widget->SaveParams(fp, saveProjVersion);
		FMF_widget->SaveParams(fp, saveProjVersion);
		wshed_widget->SaveParams(fp, saveProjVersion);
		OutlineCorrect_widget->SaveParams(fp, saveProjVersion);
		interpolwidget->SaveParams(fp, saveProjVersion);
		smoothing_widget->SaveParams(fp, saveProjVersion);
		morph_widget->SaveParams(fp, saveProjVersion);
		edge_widg->SaveParams(fp, saveProjVersion);
		feature_widget->SaveParams(fp, saveProjVersion);
		measurement_widget->SaveParams(fp, saveProjVersion);
		vesselextr_widget->SaveParams(fp, saveProjVersion);
		pickerwidget->SaveParams(fp, saveProjVersion);
		tissueTreeWidget->SaveParams(fp, saveProjVersion);
		fp = TissueInfos::SaveTissueLocks(fp);
		fp = save_notes(fp, saveProjVersion);

		fclose(fp);

		// Load merged project
		if (QMessageBox::question(this, "iSeg",
															"Would you like to load the merged project?",
															QMessageBox::Yes | QMessageBox::Default,
															QMessageBox::No) == QMessageBox::Yes) {
			loadproj(savefilename);
		}
	}
	emit end_dataexport(this);
}

void MainWindow::execute_boneconnectivity()
{
	boneConnectivityDialog = new CheckBoneConnectivityDialog(
			handler3D, "Bone Connectivity", this, Qt::Window);
	QObject::connect(boneConnectivityDialog, SIGNAL(slice_changed()), this,
									 SLOT(update_slice()));

	boneConnectivityDialog->show();
	boneConnectivityDialog->raise();

	emit end_dataexport(this);
}

void MainWindow::update_slice() { slice_changed(); }

void MainWindow::execute_loadproj()
{
	maybeSafe();

	QString loadfilename = Q3FileDialog::getOpenFileName(QString::null,
																											 "Projects (*.prj)\n"
																											 "All(*.*)",
																											 this); //, filename);

	if (!loadfilename.isEmpty()) {
		loadproj(loadfilename);
	}
}

void MainWindow::execute_loadproj1()
{
	maybeSafe();

	QString loadfilename = m_loadprojfilename.m_loadprojfilename1;

	if (!loadfilename.isEmpty()) {
		loadproj(loadfilename);
	}
}

void MainWindow::execute_loadproj2()
{
	maybeSafe();

	QString loadfilename = m_loadprojfilename.m_loadprojfilename2;

	if (!loadfilename.isEmpty()) {
		loadproj(loadfilename);
	}
}

void MainWindow::execute_loadproj3()
{
	maybeSafe();

	QString loadfilename = m_loadprojfilename.m_loadprojfilename3;

	if (!loadfilename.isEmpty()) {
		loadproj(loadfilename);
	}
}

void MainWindow::execute_loadproj4()
{
	maybeSafe();

	QString loadfilename = m_loadprojfilename.m_loadprojfilename4;

	if (!loadfilename.isEmpty()) {
		loadproj(loadfilename);
	}
}

void MainWindow::execute_loadatlas(int i)
{
	AtlasWidget *AW = new AtlasWidget(
			m_atlasfilename.m_atlasdir.absFilePath(m_atlasfilename.m_atlasfilename[i])
					.ascii(),
			m_picpath);
	if (AW->isOK) {
		AW->show();
		AW->raise();
		AW->setAttribute(Qt::WA_DeleteOnClose);
	}
	else {
		delete AW;
	}
}

void MainWindow::execute_loadatlas0()
{
	int i = 0;
	execute_loadatlas(i);
}

void MainWindow::execute_loadatlas1()
{
	int i = 1;
	execute_loadatlas(i);
}

void MainWindow::execute_loadatlas2()
{
	int i = 2;
	execute_loadatlas(i);
}

void MainWindow::execute_loadatlas3()
{
	int i = 3;
	execute_loadatlas(i);
}

void MainWindow::execute_loadatlas4()
{
	int i = 4;
	execute_loadatlas(i);
}

void MainWindow::execute_loadatlas5()
{
	int i = 5;
	execute_loadatlas(i);
}

void MainWindow::execute_loadatlas6()
{
	int i = 6;
	execute_loadatlas(i);
}
void MainWindow::execute_loadatlas7()
{
	int i = 7;
	execute_loadatlas(i);
}
void MainWindow::execute_loadatlas8()
{
	int i = 8;
	execute_loadatlas(i);
}
void MainWindow::execute_loadatlas9()
{
	int i = 9;
	execute_loadatlas(i);
}
void MainWindow::execute_loadatlas10()
{
	int i = 10;
	execute_loadatlas(i);
}
void MainWindow::execute_loadatlas11()
{
	int i = 11;
	execute_loadatlas(i);
}
void MainWindow::execute_loadatlas12()
{
	int i = 12;
	execute_loadatlas(i);
}
void MainWindow::execute_loadatlas13()
{
	int i = 13;
	execute_loadatlas(i);
}
void MainWindow::execute_loadatlas14()
{
	int i = 14;
	execute_loadatlas(i);
}
void MainWindow::execute_loadatlas15()
{
	int i = 15;
	execute_loadatlas(i);
}
void MainWindow::execute_loadatlas16()
{
	int i = 16;
	execute_loadatlas(i);
}
void MainWindow::execute_loadatlas17()
{
	int i = 17;
	execute_loadatlas(i);
}
void MainWindow::execute_loadatlas18()
{
	int i = 18;
	execute_loadatlas(i);
}
void MainWindow::execute_loadatlas19()
{
	int i = 19;
	execute_loadatlas(i);
}

void MainWindow::execute_createatlas()
{
	common::DataSelection dataSelection;
	dataSelection.tissues = true;
	emit begin_dataexport(dataSelection, this);

	QString savefilename = Q3FileDialog::getSaveFileName(
			QString::null, "Atlas file (*.atl)", this); //, filename);

	if (savefilename.length() > 4 && !savefilename.endsWith(QString(".atl")))
		savefilename.append(".atl");

	if (!savefilename.isEmpty()) {
		handler3D->print_atlas(savefilename.ascii());
		LoadAtlas(m_atlasfilename.m_atlasdir);
	}

	emit end_dataexport(this);
}

void MainWindow::execute_reloadatlases()
{
	LoadAtlas(m_atlasfilename.m_atlasdir);
}

void MainWindow::execute_savetissues()
{
	QString savefilename = Q3FileDialog::getSaveFileName(
			QString::null, QString::null, this); //, filename);

	if (!savefilename.isEmpty()) {
		unsigned short saveVersion = 7;
		TissueInfos::SaveTissuesReadable(savefilename.ascii(), saveVersion);
	}
}

void MainWindow::execute_exportsurfacegenerationtoolxml()
{
	QString savefilename = Q3FileDialog::getSaveFileName(
			QString::null, QString::null, this); //, filename);

	if (!savefilename.isEmpty()) {
		TissueInfos::ExportSurfaceGenerationToolXML(savefilename.ascii());
	}
}

void MainWindow::execute_exportlabelfield()
{
	common::DataSelection dataSelection;
	dataSelection.tissues = true;
	emit begin_dataexport(dataSelection, this);

	QString savefilename = Q3FileDialog::getSaveFileName(
			QString::null, "AmiraMesh Ascii (*.am)", this); //, filename);

	if (savefilename.length() > 4 && !savefilename.endsWith(QString(".am")))
		savefilename.append(".am");

	if (!savefilename.isEmpty()) {
		handler3D->print_amascii(savefilename.ascii());
	}

	emit end_dataexport(this);
}

void MainWindow::execute_exportmat()
{
	common::DataSelection dataSelection;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	emit begin_dataexport(dataSelection, this);

	QString savefilename = Q3FileDialog::getSaveFileName(
			QString::null, "Matlab (*.mat)", this); //, filename);

	if (savefilename.length() > 4 && !savefilename.endsWith(QString(".mat")))
		savefilename.append(".mat");

	if (!savefilename.isEmpty()) {
		handler3D->print_tissuemat(savefilename.ascii());
	}

	emit end_dataexport(this);
}

void MainWindow::execute_exporthdf()
{
	common::DataSelection dataSelection;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	emit begin_dataexport(dataSelection, this);

	QString savefilename = Q3FileDialog::getSaveFileName(
			QString::null, "HDF (*.h5)", this); //, filename);

	if (savefilename.length() > 3 && !savefilename.endsWith(QString(".h5")))
		savefilename.append(".h5");

	if (!savefilename.isEmpty()) {
		handler3D->SaveCommunicationFile(savefilename.ascii());
	}

	emit end_dataexport(this);
}

void MainWindow::execute_exportvtkascii()
{
	common::DataSelection dataSelection;
	dataSelection.tissues = true;
	emit begin_dataexport(dataSelection, this);

	QString savefilename = Q3FileDialog::getSaveFileName(
			QString::null, "VTK Ascii (*.vti *.vtk)", this); //, filename);

	if (savefilename.length() > 4 && !(savefilename.endsWith(QString(".vti")) ||
																		 savefilename.endsWith(QString(".vtk"))))
		savefilename.append(".vti");

	if (!savefilename.isEmpty()) {
		handler3D->export_tissue(savefilename.ascii(), false);
	}

	emit end_dataexport(this);
}

void MainWindow::execute_exportvtkbinary()
{
	common::DataSelection dataSelection;
	dataSelection.tissues = true;
	emit begin_dataexport(dataSelection, this);

	QString savefilename = Q3FileDialog::getSaveFileName(
			QString::null, "VTK bin (*.vti *.vtk)", this); //, filename);

	if (savefilename.length() > 4 && !(savefilename.endsWith(QString(".vti")) ||
																		 savefilename.endsWith(QString(".vtk"))))
		savefilename.append(".vti");

	if (!savefilename.isEmpty()) {
		handler3D->export_tissue(savefilename.ascii(), true);
	}

	emit end_dataexport(this);
}

void MainWindow::execute_exportvtkcompressedascii()
{
	common::DataSelection dataSelection;
	dataSelection.tissues = true;
	emit begin_dataexport(dataSelection, this);

	QString savefilename = Q3FileDialog::getSaveFileName(
			QString::null, "VTK comp (*.vti)", this); //, filename);

	if (savefilename.length() > 4 && !savefilename.endsWith(QString(".vti")))
		savefilename.append(".vti");

	if (!savefilename.isEmpty()) {
		handler3D->export_tissue(savefilename.ascii(), false);
	}

	emit end_dataexport(this);
}

void MainWindow::execute_exportvtkcompressedbinary()
{
	common::DataSelection dataSelection;
	dataSelection.tissues = true;
	emit begin_dataexport(dataSelection, this);

	QString savefilename = Q3FileDialog::getSaveFileName(
			QString::null, "VTK comp (*.vti)", this); //, filename);

	if (savefilename.length() > 4 && !savefilename.endsWith(QString(".vti")))
		savefilename.append(".vti");

	if (!savefilename.isEmpty()) {
		handler3D->export_tissue(savefilename.ascii(), true);
	}

	emit end_dataexport(this);
}

void MainWindow::execute_exportxmlregionextent()
{
	common::DataSelection dataSelection;
	dataSelection.tissues = true;
	emit begin_dataexport(dataSelection, this);

	QString savefilename =
			Q3FileDialog::getSaveFileName(QString::null, "XML extent (*.xml)", this);

	if (savefilename.length() > 4 && !savefilename.endsWith(QString(".xml")))
		savefilename.append(".xml");

	QString relfilename = QString("");
	if (!m_saveprojfilename.isEmpty())
		relfilename = m_saveprojfilename.right(m_saveprojfilename.length() -
																					 m_saveprojfilename.findRev("/") - 1);

	if (!savefilename.isEmpty()) {
		if (relfilename.isEmpty())
			handler3D->print_xmlregionextent(savefilename.ascii(), true);
		else
			handler3D->print_xmlregionextent(savefilename.ascii(), true,
																			 relfilename.ascii());
	}

	emit end_dataexport(this);
}

void MainWindow::execute_exporttissueindex()
{
	common::DataSelection dataSelection;
	emit begin_dataexport(dataSelection, this);

	QString savefilename = Q3FileDialog::getSaveFileName(
			QString::null, "tissue index (*.txt)", this);

	if (savefilename.length() > 4 && !savefilename.endsWith(QString(".txt")))
		savefilename.append(".txt");

	QString relfilename = QString("");
	if (!m_saveprojfilename.isEmpty())
		relfilename = m_saveprojfilename.right(m_saveprojfilename.length() -
																					 m_saveprojfilename.findRev("/") - 1);

	if (!savefilename.isEmpty()) {
		if (relfilename.isEmpty())
			handler3D->print_tissueindex(savefilename.ascii(), true);
		else
			handler3D->print_tissueindex(savefilename.ascii(), true,
																	 relfilename.ascii());
	}

	emit end_dataexport(this);
}

void MainWindow::execute_loadtissues()
{
	QString loadfilename = Q3FileDialog::getOpenFileName(
			QString::null, QString::null, this); //, filename);

	if (!loadfilename.isEmpty()) {
		QMessageBox msgBox;
		msgBox.setText(
				"Do you want to append the new tissues or to replace the old tissues?");
		QPushButton *appendButton =
				msgBox.addButton(tr("Append"), QMessageBox::AcceptRole);
		QPushButton *replaceButton =
				msgBox.addButton(tr("Replace"), QMessageBox::AcceptRole);
		QPushButton *abortButton = msgBox.addButton(QMessageBox::Abort);
		msgBox.setIcon(QMessageBox::Question);

		msgBox.exec();

		tissues_size_t removeTissuesRange;
		if (msgBox.clickedButton() == appendButton) {
			TissueInfos::LoadTissuesReadable(loadfilename.ascii(), handler3D,
																			 removeTissuesRange);
			int nr = tissueTreeWidget->get_current_type() - 1;
			tissueTreeWidget->update_tree_widget();

			tissues_size_t currTissueType = tissueTreeWidget->get_current_type();
			if (nr != currTissueType - 1) {
				tissuenr_changed(currTissueType - 1);
			}
		}
		else if (msgBox.clickedButton() == replaceButton) {
			if (TissueInfos::LoadTissuesReadable(loadfilename.ascii(), handler3D,
																					 removeTissuesRange)) {
				if (removeTissuesRange > 0) {
#if 1 // Version: Ask user whether to delete tissues
					int ret = QMessageBox::question(
							this, "iSeg",
							"Some of the previously existing tissues are\nnot contained in "
							"the loaded tissue list.\n\nDo you want to delete them?",
							QMessageBox::Yes | QMessageBox::Default, QMessageBox::No);
					if (ret == QMessageBox::Yes) {
						std::set<tissues_size_t> removeTissues;
						for (tissues_size_t type = 1; type <= removeTissuesRange; ++type) {
							removeTissues.insert(type);
						}
						common::DataSelection dataSelection;
						dataSelection.allSlices = true;
						dataSelection.tissues = true;
						emit begin_datachange(dataSelection, this, false);
						handler3D->remove_tissues(removeTissues);
						emit end_datachange(this, common::ClearUndo);
					}
#else
					std::set<tissues_size_t> removeTissues;
					for (tissues_size_t type = 1; type <= removeTissuesRange; ++type) {
						removeTissues.insert(type);
					}
					common::DataSelection dataSelection;
					dataSelection.allSlices = true;
					dataSelection.tissues = true;
					emit begin_datachange(dataSelection, this, false);
					handler3D->remove_tissues(removeTissues);
					emit end_datachange(this, common::ClearUndo);
#endif
				}
			}
			tissues_size_t tissueCount = TissueInfos::GetTissueCount();
			handler3D->cap_tissue(tissueCount);

			tissueTreeWidget->update_tree_widget();
			tissuenr_changed(tissueTreeWidget->get_current_type() - 1);
		}
	}
}

void MainWindow::execute_settissuesasdef()
{
	TissueInfos::SaveDefaultTissueList(
			m_tmppath.absFilePath(QString("def_tissues.txt")));
}

void MainWindow::execute_removedeftissues()
{
	remove(m_tmppath.absFilePath(QString("def_tissues.txt")));
}

void MainWindow::execute_new()
{
	common::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this, false);

	NewImg NI(handler3D, this);
	NI.move(QCursor::pos());
	NI.exec();

	if (!NI.new_pressed())
		return;

	handler3D->set_pixelsize(1.f, 1.f);
	handler3D->set_slicethickness(1.f);

	emit end_datachange(this, common::ClearUndo);

	m_saveprojfilename = QString("");
	setCaption(QString(" iSeg ") + QString::number(iSegVersion) + QString(".") +
						 QString::number(iSegSubversion) + QString(" B") +
						 QString::number(build) + QString(" - No Filename"));
	m_notes->clear();

	reset_brightnesscontrast();
}

void MainWindow::execute_3Dsurfaceviewer()
{
	common::DataSelection dataSelection;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	emit begin_dataexport(dataSelection, this);

	if (SV3D == NULL) {
		SV3D = new surfaceviewer3D(handler3D, false, 0);
		QObject::connect(SV3D, SIGNAL(hasbeenclosed()), this, SLOT(SV3D_closed()));
	}

	SV3D->show();
	SV3D->raise();

	emit end_dataexport(this);
}

void MainWindow::execute_3Dsurfaceviewerbmp()
{
	common::DataSelection dataSelection;
	dataSelection.bmp = true;
	emit begin_dataexport(dataSelection, this);

	if (SV3Dbmp == NULL) {
		SV3Dbmp = new surfaceviewer3D(handler3D, true, 0);
		QObject::connect(SV3Dbmp, SIGNAL(hasbeenclosed()), this,
										 SLOT(SV3Dbmp_closed()));
	}

	SV3Dbmp->show();
	SV3Dbmp->raise();

	emit end_dataexport(this);
}

void MainWindow::execute_3Dvolumeviewertissue()
{
	common::DataSelection dataSelection;
	dataSelection.tissues = true;
	emit begin_dataexport(dataSelection, this);

	if (VV3D == NULL) {
		VV3D = new volumeviewer3D(handler3D, false, true, true, 0);
		QObject::connect(VV3D, SIGNAL(hasbeenclosed()), this, SLOT(VV3D_closed()));
	}

	VV3D->show();
	VV3D->raise();

	emit end_dataexport(this);
}

void MainWindow::execute_3Dvolumeviewerbmp()
{
	common::DataSelection dataSelection;
	dataSelection.bmp = true;
	emit begin_dataexport(dataSelection, this);

	if (VV3Dbmp == NULL) {
		VV3Dbmp = new volumeviewer3D(handler3D, true, true, true, 0);
		QObject::connect(VV3Dbmp, SIGNAL(hasbeenclosed()), this,
										 SLOT(VV3Dbmp_closed()));
	}

	VV3Dbmp->show();
	VV3Dbmp->raise();

	emit end_dataexport(this);
}

void MainWindow::execute_settings()
{
	Settings settings(this);
	settings.exec();
}

void MainWindow::update_bmp()
{
	bmp_show->update();
	if (xsliceshower != NULL) {
		xsliceshower->bmp_changed();
	}
	if (ysliceshower != NULL) {
		ysliceshower->bmp_changed();
	}
}

void MainWindow::update_bmp(QRect rect)
{
	bmp_show->update(rect);
	if (xsliceshower != NULL) {
		xsliceshower->bmp_changed();
	}
	if (ysliceshower != NULL) {
		ysliceshower->bmp_changed();
	}
}

void MainWindow::update_work()
{
	work_show->update();

	if (xsliceshower != NULL) {
		xsliceshower->work_changed();
	}
	if (ysliceshower != NULL) {
		ysliceshower->work_changed();
	}
}

void MainWindow::update_work(QRect rect)
{
	work_show->update(rect);

	if (xsliceshower != NULL) {
		xsliceshower->work_changed();
	}
	if (ysliceshower != NULL) {
		ysliceshower->work_changed();
	}
}

void MainWindow::update_tissue()
{
	bmp_show->tissue_changed();
	work_show->tissue_changed();
	if (xsliceshower != NULL)
		xsliceshower->tissue_changed();
	if (ysliceshower != NULL)
		ysliceshower->tissue_changed();
	if (VV3D != NULL)
		VV3D->tissue_changed();
	if (SV3D != NULL)
		SV3D->tissue_changed();
}

void MainWindow::update_tissue(QRect rect)
{
	bmp_show->tissue_changed(rect);
	work_show->tissue_changed(rect);
	if (xsliceshower != NULL)
		xsliceshower->tissue_changed();
	if (ysliceshower != NULL)
		ysliceshower->tissue_changed();
	if (VV3D != NULL)
		VV3D->tissue_changed();
	if (SV3D != NULL)
		SV3D->tissue_changed();
}

void MainWindow::xslice_closed()
{
	if (xsliceshower != NULL) {
		QObject::disconnect(bmp_show,
												SIGNAL(scaleoffsetfactor_changed(float, float, bool)),
												xsliceshower, SLOT(set_scale(float, float, bool)));
		QObject::disconnect(work_show,
												SIGNAL(scaleoffsetfactor_changed(float, float, bool)),
												xsliceshower, SLOT(set_scale(float, float, bool)));
		QObject::disconnect(xsliceshower, SIGNAL(slice_changed(int)), this,
												SLOT(xshower_slicechanged()));
		QObject::disconnect(xsliceshower, SIGNAL(hasbeenclosed()), this,
												SLOT(xslice_closed()));
		QObject::disconnect(zoom_widget, SIGNAL(set_zoom(double)), xsliceshower,
												SLOT(set_zoom(double)));

		if (ysliceshower != NULL) {
			ysliceshower->xyexists_changed(false);
		}
		bmp_show->set_crosshairyvisible(false);
		work_show->set_crosshairyvisible(false);

		delete xsliceshower;
		xsliceshower = NULL;
	}
}

void MainWindow::yslice_closed()
{
	if (ysliceshower != NULL) {
		QObject::disconnect(bmp_show,
												SIGNAL(scaleoffsetfactor_changed(float, float, bool)),
												ysliceshower, SLOT(set_scale(float, float, bool)));
		QObject::disconnect(work_show,
												SIGNAL(scaleoffsetfactor_changed(float, float, bool)),
												ysliceshower, SLOT(set_scale(float, float, bool)));
		QObject::disconnect(ysliceshower, SIGNAL(slice_changed(int)), this,
												SLOT(yshower_slicechanged()));
		QObject::disconnect(ysliceshower, SIGNAL(hasbeenclosed()), this,
												SLOT(yslice_closed()));
		QObject::disconnect(zoom_widget, SIGNAL(set_zoom(double)), ysliceshower,
												SLOT(set_zoom(double)));

		if (xsliceshower != NULL) {
			xsliceshower->xyexists_changed(false);
		}
		bmp_show->set_crosshairxvisible(false);
		work_show->set_crosshairxvisible(false);

		delete ysliceshower;
		ysliceshower = NULL;
	}
}

void MainWindow::SV3D_closed()
{
	if (SV3D != NULL) {
		QObject::disconnect(SV3D, SIGNAL(hasbeenclosed()), this,
												SLOT(SV3D_closed()));
		delete SV3D;
		SV3D = NULL;
	}
}

void MainWindow::SV3Dbmp_closed()
{
	if (SV3Dbmp != NULL) {
		QObject::disconnect(SV3D, SIGNAL(hasbeenclosed()), this,
												SLOT(SV3Dbmp_closed()));
		delete SV3Dbmp;
		SV3Dbmp = NULL;
	}
}

void MainWindow::VV3D_closed()
{
	if (VV3D != NULL) {
		QObject::disconnect(VV3D, SIGNAL(hasbeenclosed()), this,
												SLOT(VV3D_closed()));
		delete VV3D;
		VV3D = NULL;
	}
}

void MainWindow::VV3Dbmp_closed()
{
	if (VV3Dbmp != NULL) {
		QObject::disconnect(VV3Dbmp, SIGNAL(hasbeenclosed()), this,
												SLOT(VV3Dbmp_closed()));
		delete VV3Dbmp;
		VV3Dbmp = NULL;
	}
}

void MainWindow::xshower_slicechanged()
{
	if (ysliceshower != NULL) {
		ysliceshower->xypos_changed(xsliceshower->get_slicenr());
	}
	bmp_show->crosshairy_changed(xsliceshower->get_slicenr());
	work_show->crosshairy_changed(xsliceshower->get_slicenr());
}

void MainWindow::yshower_slicechanged()
{
	if (xsliceshower != NULL) {
		xsliceshower->xypos_changed(ysliceshower->get_slicenr());
	}
	bmp_show->crosshairx_changed(ysliceshower->get_slicenr());
	work_show->crosshairx_changed(ysliceshower->get_slicenr());
}

void MainWindow::setWorkContentsPos(int x, int y)
{
	if (tomove_scroller) {
		tomove_scroller = false;
		work_scroller->setContentsPos(x, y);
	}
	else {
		tomove_scroller = true;
	}
}

void MainWindow::setBmpContentsPos(int x, int y)
{
	if (tomove_scroller) {
		tomove_scroller = false;
		bmp_scroller->setContentsPos(x, y);
	}
	else {
		tomove_scroller = true;
	}
}

void MainWindow::execute_histo()
{
	common::DataSelection dataSelection;
	dataSelection.work = true;
	emit begin_dataexport(dataSelection, this);

	ShowHisto SH(handler3D, this);
	SH.move(QCursor::pos());
	SH.exec();

	emit end_dataexport(this);
}

void MainWindow::execute_scale()
{
	ScaleWork SW(handler3D, m_picpath, this);
	QObject::connect(
			&SW, SIGNAL(begin_datachange(common::DataSelection &, QWidget *, bool)),
			this,
			SLOT(handle_begin_datachange(common::DataSelection &, QWidget *, bool)));
	QObject::connect(
			&SW, SIGNAL(end_datachange(QWidget *, common::EndUndoAction)), this,
			SLOT(handle_end_datachange(QWidget *, common::EndUndoAction)));
	SW.move(QCursor::pos());
	SW.exec();
	QObject::disconnect(
			&SW, SIGNAL(begin_datachange(common::DataSelection &, QWidget *, bool)),
			this,
			SLOT(handle_begin_datachange(common::DataSelection &, QWidget *, bool)));
	QObject::disconnect(
			&SW, SIGNAL(end_datachange(QWidget *, common::EndUndoAction)), this,
			SLOT(handle_end_datachange(QWidget *, common::EndUndoAction)));

	return;
}

void MainWindow::execute_imagemath()
{
	ImageMath IM(handler3D, this);
	QObject::connect(
			&IM, SIGNAL(begin_datachange(common::DataSelection &, QWidget *, bool)),
			this,
			SLOT(handle_begin_datachange(common::DataSelection &, QWidget *, bool)));
	QObject::connect(
			&IM, SIGNAL(end_datachange(QWidget *, common::EndUndoAction)), this,
			SLOT(handle_end_datachange(QWidget *, common::EndUndoAction)));
	IM.move(QCursor::pos());
	IM.exec();
	QObject::disconnect(
			&IM, SIGNAL(begin_datachange(common::DataSelection &, QWidget *, bool)),
			this,
			SLOT(handle_begin_datachange(common::DataSelection &, QWidget *, bool)));
	QObject::disconnect(
			&IM, SIGNAL(end_datachange(QWidget *, common::EndUndoAction)), this,
			SLOT(handle_end_datachange(QWidget *, common::EndUndoAction)));

	return;
}

void MainWindow::execute_unwrap()
{
	common::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.work = true;
	emit begin_datachange(dataSelection, this);

	handler3D->unwrap(0.90f);

	emit end_datachange(this);
	return;
}

void MainWindow::execute_overlay()
{
	ImageOverlay IO(handler3D, this);
	QObject::connect(
			&IO, SIGNAL(begin_datachange(common::DataSelection &, QWidget *, bool)),
			this,
			SLOT(handle_begin_datachange(common::DataSelection &, QWidget *, bool)));
	QObject::connect(
			&IO, SIGNAL(end_datachange(QWidget *, common::EndUndoAction)), this,
			SLOT(handle_end_datachange(QWidget *, common::EndUndoAction)));
	IO.move(QCursor::pos());
	IO.exec();
	QObject::disconnect(
			&IO, SIGNAL(begin_datachange(common::DataSelection &, QWidget *, bool)),
			this,
			SLOT(handle_begin_datachange(common::DataSelection &, QWidget *, bool)));
	QObject::disconnect(
			&IO, SIGNAL(end_datachange(QWidget *, common::EndUndoAction)), this,
			SLOT(handle_end_datachange(QWidget *, common::EndUndoAction)));

	return;
}

void MainWindow::execute_pixelsize()
{
	PixelResize PR(handler3D, this);
	PR.move(QCursor::pos());
	if (PR.exec()) {
		Pair p = PR.return_pixelsize();
		handler3D->set_pixelsize(p.high, p.low);
		pixelsize_changed();
		/*if(xsliceshower!=NULL) xsliceshower->pixelsize_changed(p);
		if(ysliceshower!=NULL) ysliceshower->pixelsize_changed(p);
		bmp_show->pixelsize_changed(p);
		work_show->pixelsize_changed(p);*/
	}

	return;
}

void MainWindow::pixelsize_changed()
{
	Pair p = handler3D->get_pixelsize();
	if (xsliceshower != NULL)
		xsliceshower->pixelsize_changed(p);
	if (ysliceshower != NULL)
		ysliceshower->pixelsize_changed(p);
	bmp_show->pixelsize_changed(p);
	work_show->pixelsize_changed(p);
	if (VV3D != NULL) {
		VV3D->pixelsize_changed(p);
	}
	if (VV3Dbmp != NULL) {
		VV3Dbmp->pixelsize_changed(p);
	}
	if (SV3D != NULL) {
		SV3D->pixelsize_changed(p);
	}
	if (SV3Dbmp != NULL) {
		SV3Dbmp->pixelsize_changed(p);
	}
}

void MainWindow::execute_displacement()
{
	DisplacementDialog DD(handler3D, this);
	DD.move(QCursor::pos());
	if (DD.exec()) {
		float disp[3];
		DD.return_displacement(disp);
		handler3D->set_displacement(disp);
	}

	return;
}

void MainWindow::execute_rotation()
{
	RotationDialog RD(handler3D, this);
	RD.move(QCursor::pos());
	if (RD.exec()) {
		float dc[6];
		RD.return_direction_cosines(dc);
		handler3D->set_direction_cosines(dc);
	}

	return;
}

void MainWindow::execute_undoconf()
{
	UndoConfig UC(handler3D, this);
	UC.move(QCursor::pos());
	UC.exec();

	if (handler3D->return_nrundo() == 0)
		editmenu->setItemEnabled(undonr, false);
	else
		editmenu->setItemEnabled(undonr, true);

	if (handler3D->return_nrredo() == 0)
		editmenu->setItemEnabled(redonr, false);
	else
		editmenu->setItemEnabled(redonr, true);

	this->SaveSettings();

	return;
}

void MainWindow::execute_activeslicesconf()
{
	ActiveSlicesConfig AC(handler3D, this);
	AC.move(QCursor::pos());
	AC.exec();

	unsigned short slicenr = handler3D->get_activeslice() + 1;
	if (handler3D->return_startslice() >= slicenr ||
			handler3D->return_endslice() + 1 <= slicenr) {
		lb_inactivewarning->setText(QString("   3D Inactive Slice!   "));
		lb_inactivewarning->setPaletteBackgroundColor(QColor(0, 255, 0));
	}
	else {
		lb_inactivewarning->setText(QString(" "));
		lb_inactivewarning->setPaletteBackgroundColor(
				this->paletteBackgroundColor());
	}

	return;
}

void MainWindow::execute_hideparameters(bool checked)
{
	QWidget1::set_hideparams(checked);
	if (tab_old != NULL)
		tab_old->hideparams_changed();
}

void MainWindow::execute_hidezoom(bool checked)
{
	if (!checked) {
		zoom_widget->hide();
	}
	else {
		zoom_widget->show();
	}
}

void MainWindow::execute_hidecontrastbright(bool checked)
{
	if (!checked) {
		lb_contrastbmp->hide();
		le_contrastbmp_val->hide();
		lb_contrastbmp_val->hide();
		sl_contrastbmp->hide();
		lb_brightnessbmp->hide();
		le_brightnessbmp_val->hide();
		lb_brightnessbmp_val->hide();
		sl_brightnessbmp->hide();
		lb_contrastwork->hide();
		le_contrastwork_val->hide();
		lb_contrastwork_val->hide();
		sl_contrastwork->hide();
		lb_brightnesswork->hide();
		le_brightnesswork_val->hide();
		lb_brightnesswork_val->hide();
		sl_brightnesswork->hide();
	}
	else {
		lb_contrastbmp->show();
		le_contrastbmp_val->show();
		lb_contrastbmp_val->show();
		sl_contrastbmp->show();
		lb_brightnessbmp->show();
		le_brightnessbmp_val->show();
		lb_brightnessbmp_val->show();
		sl_brightnessbmp->show();
		lb_contrastwork->show();
		le_contrastwork_val->show();
		lb_contrastwork_val->show();
		sl_contrastwork->show();
		lb_brightnesswork->show();
		le_brightnesswork_val->show();
		lb_brightnesswork_val->show();
		sl_brightnesswork->show();
	}
}

void MainWindow::execute_hidesource(bool checked)
{
	if (!checked) {
		vboxbmpw->hide();
	}
	else {
		vboxbmpw->show();
	}
}

void MainWindow::execute_hidetarget(bool checked)
{
	if (!checked) {
		vboxworkw->hide();
	}
	else {
		vboxworkw->show();
	}
}

void MainWindow::execute_hidecopyswap(bool checked)
{
	if (!checked) {
		toworkBtn->hide();
		tobmpBtn->hide();
		swapBtn->hide();
		swapAllBtn->hide();
	}
	else {
		toworkBtn->show();
		tobmpBtn->show();
		swapBtn->show();
		swapAllBtn->show();
	}
}

void MainWindow::execute_hidestack(bool checked)
{
	if (!checked) {
		bitstack_widget->hide();
	}
	else {
		bitstack_widget->show();
	}
}

void MainWindow::execute_hideoverlay(bool checked)
{
	if (!checked) {
		overlay_widget->hide();
	}
	else {
		overlay_widget->show();
	}
}

void MainWindow::execute_multidataset(bool checked)
{
	if (!checked) {
		m_MultiDataset_widget->hide();
	}
	else {
		m_MultiDataset_widget->show();
	}
}

void MainWindow::execute_hidenotes(bool checked)
{
	if (!checked) {
		m_notes->hide();
		m_lb_notes->hide();
	}
	else {
		m_notes->show();
		m_lb_notes->show();
	}
}

void MainWindow::execute_showtabtoggled(bool)
{
	for (unsigned short i = 0; i < nrtabbuttons; i++) {
		showpb_tab[i] = showtab_action[i]->isOn();
	}

	QWidget1 *currentwidget = (QWidget1 *)methodTab->visibleWidget();
	unsigned short i = 0;
	while ((i < nrtabbuttons) && (currentwidget != tabwidgets[i]))
		i++;
	if (i == nrtabbuttons) {
		updateTabvisibility();
		return;
	}

	if (!showpb_tab[i]) {
		i = 0;
		while ((i < nrtabbuttons) && !showpb_tab[i])
			i++;
		if (i != nrtabbuttons) {
			int pos = methodTab->id(tabwidgets[i]);
			methodTab->raiseWidget(pos);
		}
	}
	updateTabvisibility();
}

void MainWindow::execute_xslice()
{
	common::DataSelection dataSelection;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	emit begin_dataexport(dataSelection, this);

	if (xsliceshower == NULL) {
		xsliceshower = new sliceshower_widget(
				handler3D, true, handler3D->get_slicethickness(),
				zoom_widget->get_zoom(), 0, 0, Qt::WStyle_StaysOnTop);
		xsliceshower->zpos_changed();
		if (ysliceshower != NULL) {
			xsliceshower->xypos_changed(ysliceshower->get_slicenr());
			xsliceshower->xyexists_changed(true);
			ysliceshower->xyexists_changed(true);
		}
		bmp_show->set_crosshairyvisible(cb_bmpcrosshairvisible->isOn());
		work_show->set_crosshairyvisible(cb_workcrosshairvisible->isOn());
		xshower_slicechanged();
		float offset1, factor1;
		bmp_show->get_scaleoffsetfactor(offset1, factor1);
		xsliceshower->set_scale(offset1, factor1, true);
		work_show->get_scaleoffsetfactor(offset1, factor1);
		xsliceshower->set_scale(offset1, factor1, false);
		QObject::connect(bmp_show,
										 SIGNAL(scaleoffsetfactor_changed(float, float, bool)),
										 xsliceshower, SLOT(set_scale(float, float, bool)));
		QObject::connect(work_show,
										 SIGNAL(scaleoffsetfactor_changed(float, float, bool)),
										 xsliceshower, SLOT(set_scale(float, float, bool)));
		QObject::connect(xsliceshower, SIGNAL(slice_changed(int)), this,
										 SLOT(xshower_slicechanged()));
		QObject::connect(xsliceshower, SIGNAL(hasbeenclosed()), this,
										 SLOT(xslice_closed()));
		QObject::connect(zoom_widget, SIGNAL(set_zoom(double)), xsliceshower,
										 SLOT(set_zoom(double)));
	}

	xsliceshower->show();
	xsliceshower->raise(); //xxxa

	emit end_dataexport(this);
}

void MainWindow::execute_yslice()
{
	common::DataSelection dataSelection;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	emit begin_dataexport(dataSelection, this);

	if (ysliceshower == NULL) {
		ysliceshower = new sliceshower_widget(
				handler3D, false, handler3D->get_slicethickness(),
				zoom_widget->get_zoom(), 0, 0, Qt::WStyle_StaysOnTop);
		ysliceshower->zpos_changed();
		if (xsliceshower != NULL) {
			ysliceshower->xypos_changed(xsliceshower->get_slicenr());
			ysliceshower->xyexists_changed(true);
			xsliceshower->xyexists_changed(true);
		}
		bmp_show->set_crosshairxvisible(cb_bmpcrosshairvisible->isOn());
		work_show->set_crosshairxvisible(cb_workcrosshairvisible->isOn());
		yshower_slicechanged();
		float offset1, factor1;
		bmp_show->get_scaleoffsetfactor(offset1, factor1);
		ysliceshower->set_scale(offset1, factor1, true);
		work_show->get_scaleoffsetfactor(offset1, factor1);
		ysliceshower->set_scale(offset1, factor1, false);
		QObject::connect(bmp_show,
										 SIGNAL(scaleoffsetfactor_changed(float, float, bool)),
										 ysliceshower, SLOT(set_scale(float, float, bool)));
		QObject::connect(work_show,
										 SIGNAL(scaleoffsetfactor_changed(float, float, bool)),
										 ysliceshower, SLOT(set_scale(float, float, bool)));
		QObject::connect(ysliceshower, SIGNAL(slice_changed(int)), this,
										 SLOT(yshower_slicechanged()));
		QObject::connect(ysliceshower, SIGNAL(hasbeenclosed()), this,
										 SLOT(yslice_closed()));
		QObject::connect(zoom_widget, SIGNAL(set_zoom(double)), ysliceshower,
										 SLOT(set_zoom(double)));
	}

	ysliceshower->show();
	ysliceshower->raise();

	emit end_dataexport(this);
}

void MainWindow::execute_removetissues()
{
	QString filename = Q3FileDialog::getOpenFileName(QString::null,
																									 "Text (*.txt)\n"
																									 "All(*.*)",
																									 this);
	if (!filename.isEmpty()) {
		std::vector<tissues_size_t> types;
		if (read_tissues(filename.ascii(), types)) {
			// this actually goes through slices and removes it from segmentation
			handler3D->remove_tissues(
					std::set<tissues_size_t>(types.begin(), types.end()));

			tissueTreeWidget->update_tree_widget();
			tissuenr_changed(tissueTreeWidget->get_current_type() - 1);
		}
		else {
			QMessageBox::warning(this, "iSeg",
													 "Error: not all tissues are in tissue list",
													 QMessageBox::Ok | QMessageBox::Default);
		}
	}
}

void MainWindow::execute_grouptissues()
{
	vector<tissues_size_t> olds, news;

	QString filename = Q3FileDialog::getOpenFileName(QString::null,
																									 "Text (*.txt)\n"
																									 "All(*.*)",
																									 this);
	if (!filename.isEmpty()) {
		bool fail_on_unknown_tissue = true;
		if (read_grouptissuescapped(filename.ascii(), olds, news,
																fail_on_unknown_tissue)) {
			common::DataSelection dataSelection;
			dataSelection.allSlices = true;
			dataSelection.tissues = true;
			emit begin_datachange(dataSelection, this);

			handler3D->group_tissues(olds, news);

			emit end_datachange(this);
		}
		else {
			QMessageBox::warning(this, "iSeg",
													 "Error: not all tissues are in tissue list",
													 QMessageBox::Ok | QMessageBox::Default);
		}
	}
}

void MainWindow::execute_about()
{
	std::ostringstream stream1;
	stream1 << "\n\niSeg            \nVersion: " << iSegVersion << "."
					<< iSegSubversion << "\nBuild: " << build;
	QMessageBox::about(this, "About", QString(stream1.str().c_str()));
}

void MainWindow::add_mark(Point p)
{
	common::DataSelection dataSelection;
	dataSelection.sliceNr = handler3D->get_activeslice();
	dataSelection.marks = true;
	emit begin_datachange(dataSelection, this);

	handler3D->add_mark(p, tissueTreeWidget->get_current_type());

	emit end_datachange();
}

void MainWindow::add_label(Point p, std::string str)
{
	common::DataSelection dataSelection;
	dataSelection.sliceNr = handler3D->get_activeslice();
	dataSelection.marks = true;
	emit begin_datachange(dataSelection, this);

	handler3D->add_mark(p, tissueTreeWidget->get_current_type(), str);

	emit end_datachange(this);
}

void MainWindow::clear_marks()
{
	common::DataSelection dataSelection;
	dataSelection.sliceNr = handler3D->get_activeslice();
	dataSelection.marks = true;
	emit begin_datachange(dataSelection, this);

	handler3D->clear_marks();

	emit end_datachange(this);
}

void MainWindow::remove_mark(Point p)
{
	common::DataSelection dataSelection;
	dataSelection.sliceNr = handler3D->get_activeslice();
	dataSelection.marks = true;
	emit begin_datachange(dataSelection, this);

	if (handler3D->remove_mark(p, 3)) {
		emit end_datachange(this);
	}
	else {
		emit end_datachange(this, common::AbortUndo);
	}
}

void MainWindow::select_tissue(Point p)
{
	QList<QTreeWidgetItem *> list = tissueTreeWidget->selectedItems();
	for (auto i = list.begin(); i != list.end(); ++i) {
		(*i)->setSelected(false);
	}
	tissueTreeWidget->set_current_tissue(
			handler3D->get_tissue_pt(p, handler3D->get_activeslice()));
}

void MainWindow::next_featuring_slice()
{
	tissues_size_t type = tissueTreeWidget->get_current_type();
	if (type <= 0) {
		return;
	}

	bool found;
	unsigned short nextslice = handler3D->get_next_featuring_slice(type, found);
	if (found) {
		handler3D->set_activeslice(nextslice);
		slice_changed();
	}
	else {
		QMessageBox::information(this, "iSeg", "The selected tissue its empty.\n");
	}
}

void MainWindow::add_tissue(Point p)
{
	common::DataSelection dataSelection;
	dataSelection.sliceNr = handler3D->get_activeslice();
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this);

	tissues_size_t currTissueType = tissueTreeWidget->get_current_type();
	handler3D->add2tissue(currTissueType, p, cb_addsuboverride->isChecked());

	emit end_datachange(this);
}

void MainWindow::add_tissue_connected(Point p)
{
	common::DataSelection dataSelection;
	dataSelection.sliceNr = handler3D->get_activeslice();
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this);

	tissues_size_t currTissueType = tissueTreeWidget->get_current_type();
	handler3D->add2tissue_connected(currTissueType, p,
																	cb_addsuboverride->isChecked());

	emit end_datachange(this);
}

void MainWindow::add_tissuelarger(Point p)
{
	common::DataSelection dataSelection;
	dataSelection.sliceNr = handler3D->get_activeslice();
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this);

	tissues_size_t currTissueType = tissueTreeWidget->get_current_type();
	handler3D->add2tissue_thresh(currTissueType, p);

	emit end_datachange(this);
}

void MainWindow::add_tissue_3D(Point p)
{
	common::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this);

	tissues_size_t currTissueType = tissueTreeWidget->get_current_type();
	handler3D->add2tissueall(currTissueType, p, cb_addsuboverride->isChecked());

	emit end_datachange(this);
}

void MainWindow::subtract_tissue(Point p)
{
	common::DataSelection dataSelection;
	dataSelection.sliceNr = handler3D->get_activeslice();
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this);

	tissues_size_t currTissueType = tissueTreeWidget->get_current_type();
	handler3D->subtract_tissue(currTissueType, p);

	emit end_datachange();
}

void MainWindow::add_tissue_clicked(Point p)
{
	QObject::disconnect(work_show, SIGNAL(mousepressed_sign(Point)), this,
											SLOT(add_tissue_clicked(Point)));
	pb_add->setOn(false);
	QObject::connect(work_show, SIGNAL(mousereleased_sign(Point)), this,
									 SLOT(reconnectmouse_afterrelease(Point)));
	addhold_tissue_clicked(p);
}

void MainWindow::addhold_tissue_clicked(Point p)
{
	common::DataSelection dataSelection;
	dataSelection.allSlices = cb_addsub3d->isChecked();
	dataSelection.sliceNr = handler3D->get_activeslice();
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this);

	tissues_size_t currTissueType = tissueTreeWidget->get_current_type();
	if (cb_addsub3d->isChecked()) {
		QApplication::setOverrideCursor(QCursor(Qt::waitCursor));
		if (cb_addsubconn->isChecked())
			handler3D->add2tissueall_connected(currTissueType, p,
																				 cb_addsuboverride->isChecked());
		else
			handler3D->add2tissueall(currTissueType, p,
															 cb_addsuboverride->isChecked());
		QApplication::restoreOverrideCursor();
	}
	else {
		if (cb_addsubconn->isChecked())
			handler3D->add2tissue_connected(currTissueType, p,
																			cb_addsuboverride->isChecked());
		else
			handler3D->add2tissue(currTissueType, p, cb_addsuboverride->isChecked());
	}

	emit end_datachange(this);
}

/*void MainWindow::add_tissue_connected_clicked(Point p)
{
	QObject::disconnect(work_show,SIGNAL(mousepressed_sign(Point)),this,SLOT(add_tissue_connected_clicked(Point)));
	pb_addconn->setDown(false);
	handler3D->add2tissue_connected((tissues_size_t)tissueTreeWidget->currentItem()+1,p);
	emit tissues_changed();
}*/

void MainWindow::subtract_tissue_clicked(Point p)
{
	QObject::disconnect(work_show, SIGNAL(mousepressed_sign(Point)), this,
											SLOT(subtract_tissue_clicked(Point)));
	pb_sub->setOn(false);
	QObject::connect(work_show, SIGNAL(mousereleased_sign(Point)), this,
									 SLOT(reconnectmouse_afterrelease(Point)));
	subtracthold_tissue_clicked(p);
}

void MainWindow::subtracthold_tissue_clicked(Point p)
{
	common::DataSelection dataSelection;
	dataSelection.allSlices = cb_addsub3d->isChecked();
	dataSelection.sliceNr = handler3D->get_activeslice();
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this);

	tissues_size_t currTissueType = tissueTreeWidget->get_current_type();
	if (cb_addsub3d->isChecked()) {
		if (cb_addsubconn->isChecked())
			handler3D->subtract_tissueall_connected(currTissueType, p);
		else
			handler3D->subtract_tissueall(currTissueType, p);
	}
	else {
		if (cb_addsubconn->isChecked())
			handler3D->subtract_tissue_connected(currTissueType, p);
		else
			handler3D->subtract_tissue(currTissueType, p);
	}

	emit end_datachange(this);
}

/*void MainWindow::add_tissue_3D_clicked(Point p)
{
	QObject::disconnect(work_show,SIGNAL(mousepressed_sign(Point)),this,SLOT(add_tissue_3D_clicked(Point)));
	pb_add3D->setDown(false);
	handler3D->add2tissueall((tissues_size_t)tissueTreeWidget->currentItem()+1,p);
	emit tissues_changed();
}*/

void MainWindow::add_tissue_pushed()
{
	/*	if(pb_addconn->isDown()){
		QObject::disconnect(work_show,SIGNAL(mousepressed_sign(Point)),this,SLOT(add_tissue_connected_clicked(Point)));
		pb_addconn->setDown(false);
	}*/
	if (pb_sub->isOn()) {
		QObject::disconnect(work_show, SIGNAL(mousepressed_sign(Point)), this,
												SLOT(subtract_tissue_clicked(Point)));
		pb_sub->setOn(false);
	}
	if (pb_subhold->isOn()) {
		QObject::disconnect(work_show, SIGNAL(mousepressed_sign(Point)), this,
												SLOT(subtracthold_tissue_clicked(Point)));
		pb_subhold->setOn(false);
	}
	if (pb_addhold->isOn()) {
		QObject::disconnect(work_show, SIGNAL(mousepressed_sign(Point)), this,
												SLOT(addhold_tissue_clicked(Point)));
		pb_addhold->setOn(false);
	}

	if (pb_add->isOn()) {
		//		pb_add->setDown(false);
		QObject::connect(work_show, SIGNAL(mousepressed_sign(Point)), this,
										 SLOT(add_tissue_clicked(Point)));
		disconnect_mouseclick();
	}
	else {
		//		pb_add->setDown(true);
		QObject::disconnect(work_show, SIGNAL(mousepressed_sign(Point)), this,
												SLOT(add_tissue_clicked(Point)));
		connect_mouseclick();
	}
	/*	if(pb_add3D->isDown()){
			QObject::disconnect(work_show,SIGNAL(mousepressed_sign(Point)),this,SLOT(add_tissue_3D_clicked(Point)));
			pb_add3D->setDown(false);
		}*/
}

void MainWindow::add_tissue_shortkey()
{
	pb_add->toggle();
	add_tissue_pushed();
}

void MainWindow::subtract_tissue_shortkey()
{
	pb_sub->toggle();
	subtract_tissue_pushed();
}

void MainWindow::addhold_tissue_pushed()
{
	/*	if(pb_addconn->isDown()){
	QObject::disconnect(work_show,SIGNAL(mousepressed_sign(Point)),this,SLOT(add_tissue_connected_clicked(Point)));
	pb_addconn->setDown(false);
	}*/
	if (pb_sub->isOn()) {
		QObject::disconnect(work_show, SIGNAL(mousepressed_sign(Point)), this,
												SLOT(subtract_tissue_clicked(Point)));
		pb_sub->setOn(false);
	}
	if (pb_subhold->isOn()) {
		QObject::disconnect(work_show, SIGNAL(mousepressed_sign(Point)), this,
												SLOT(subtracthold_tissue_clicked(Point)));
		pb_subhold->setOn(false);
	}
	if (pb_add->isOn()) {
		QObject::disconnect(work_show, SIGNAL(mousepressed_sign(Point)), this,
												SLOT(add_tissue_clicked(Point)));
		pb_add->setOn(false);
	}

	if (pb_addhold->isOn()) {
		QObject::connect(work_show, SIGNAL(mousepressed_sign(Point)), this,
										 SLOT(addhold_tissue_clicked(Point)));
		disconnect_mouseclick();
		//		pb_addhold->setDown(false);
	}
	else {
		//		pb_addhold->setDown(true);
		QObject::disconnect(work_show, SIGNAL(mousepressed_sign(Point)), this,
												SLOT(addhold_tissue_clicked(Point)));
		connect_mouseclick();
	}
	/*	if(pb_add3D->isDown()){
	QObject::disconnect(work_show,SIGNAL(mousepressed_sign(Point)),this,SLOT(add_tissue_3D_clicked(Point)));
	pb_add3D->setDown(false);
	}*/
}

/*void MainWindow::add_tissue_connected_pushed()
{
	if(pb_add->isDown()){
		QObject::disconnect(work_show,SIGNAL(mousepressed_sign(Point)),this,SLOT(add_tissue_clicked(Point)));
		pb_add->setDown(false);
	}
	if(pb_sub->isDown()){
		QObject::disconnect(work_show,SIGNAL(mousepressed_sign(Point)),this,SLOT(subtract_tissue_clicked(Point)));
		pb_sub->setDown(false);
	}
	if(pb_add3D->isDown()){
		QObject::disconnect(work_show,SIGNAL(mousepressed_sign(Point)),this,SLOT(add_tissue_3D_clicked(Point)));
		pb_add3D->setDown(false);
	}
	pb_addconn->setDown(true);

	QObject::connect(work_show,SIGNAL(mousepressed_sign(Point)),this,SLOT(add_tissue_connected_clicked(Point)));
}*/

void MainWindow::subtract_tissue_pushed()
{
	if (pb_add->isOn()) {
		QObject::disconnect(work_show, SIGNAL(mousepressed_sign(Point)), this,
												SLOT(add_tissue_clicked(Point)));
		pb_add->setOn(false);
	}
	if (pb_sub->isOn()) {
		QObject::connect(work_show, SIGNAL(mousepressed_sign(Point)), this,
										 SLOT(subtract_tissue_clicked(Point)));
		disconnect_mouseclick();
	}
	else {
		QObject::disconnect(work_show, SIGNAL(mousepressed_sign(Point)), this,
												SLOT(subtract_tissue_clicked(Point)));
		connect_mouseclick();
	}
	if (pb_subhold->isOn()) {
		QObject::disconnect(work_show, SIGNAL(mousepressed_sign(Point)), this,
												SLOT(subtracthold_tissue_clicked(Point)));
		//		pb_subhold->setDown(false);
		pb_subhold->setOn(false);
	}
	if (pb_addhold->isOn()) {
		QObject::disconnect(work_show, SIGNAL(mousepressed_sign(Point)), this,
												SLOT(addhold_tissue_clicked(Point)));
		//		pb_addhold->setOn(false);
		pb_addhold->setOn(false);
	}
	//	pb_sub->setDown(!pb_sub->isDown());
	/*	if(pb_addconn->isDown()){
			QObject::disconnect(work_show,SIGNAL(mousepressed_sign(Point)),this,SLOT(add_tissue_connected_clicked(Point)));
			pb_addconn->setDown(false);
		}
		if(pb_add3D->isDown()){
			QObject::disconnect(work_show,SIGNAL(mousepressed_sign(Point)),this,SLOT(add_tissue_3D_clicked(Point)));
			pb_add3D->setDown(false);
		}*/
}

void MainWindow::subtracthold_tissue_pushed()
{
	if (pb_add->isOn()) {
		QObject::disconnect(work_show, SIGNAL(mousepressed_sign(Point)), this,
												SLOT(add_tissue_clicked(Point)));
		pb_add->setOn(false);
	}
	if (pb_subhold->isOn()) {
		QObject::connect(work_show, SIGNAL(mousepressed_sign(Point)), this,
										 SLOT(subtracthold_tissue_clicked(Point)));
		disconnect_mouseclick();
	}
	else {
		QObject::disconnect(work_show, SIGNAL(mousepressed_sign(Point)), this,
												SLOT(subtracthold_tissue_clicked(Point)));
		connect_mouseclick();
	}
	if (pb_sub->isOn()) {
		QObject::disconnect(work_show, SIGNAL(mousepressed_sign(Point)), this,
												SLOT(subtract_tissue_clicked(Point)));
		pb_sub->setOn(false);
	}
	if (pb_addhold->isOn()) {
		QObject::disconnect(work_show, SIGNAL(mousepressed_sign(Point)), this,
												SLOT(addhold_tissue_clicked(Point)));
		pb_addhold->setOn(false);
	}
	//	pb_subhold->setDown(!pb_subhold->isDown());
	/*	if(pb_addconn->isDown()){
	QObject::disconnect(work_show,SIGNAL(mousepressed_sign(Point)),this,SLOT(add_tissue_connected_clicked(Point)));
	pb_addconn->setDown(false);
	}
	if(pb_add3D->isDown()){
	QObject::disconnect(work_show,SIGNAL(mousepressed_sign(Point)),this,SLOT(add_tissue_3D_clicked(Point)));
	pb_add3D->setDown(false);
	}*/
}

void MainWindow::stophold_tissue_pushed()
{
	if (pb_add->isDown()) {
		QObject::disconnect(work_show, SIGNAL(mousepressed_sign(Point)), this,
												SLOT(add_tissue_clicked(Point)));
		pb_add->setDown(false);
	}
	if (pb_subhold->isOn()) {
		QObject::disconnect(work_show, SIGNAL(mousepressed_sign(Point)), this,
												SLOT(subtracthold_tissue_clicked(Point)));
		pb_subhold->setOn(false);
	}
	if (pb_sub->isDown()) {
		QObject::disconnect(work_show, SIGNAL(mousepressed_sign(Point)), this,
												SLOT(subtract_tissue_clicked(Point)));
		pb_sub->setDown(false);
	}
	if (pb_addhold->isOn()) {
		QObject::disconnect(work_show, SIGNAL(mousepressed_sign(Point)), this,
												SLOT(addhold_tissue_clicked(Point)));
		pb_addhold->setOn(false);
	}
}

/*void MainWindow::add_tissue_3D_pushed()
{
	if(pb_add->isDown()){
		QObject::disconnect(work_show,SIGNAL(mousepressed_sign(Point)),this,SLOT(add_tissue_clicked(Point)));
		pb_add->setDown(false);
	}
	if(pb_addconn->isDown()){
		QObject::disconnect(work_show,SIGNAL(mousepressed_sign(Point)),this,SLOT(add_tissue_connected_clicked(Point)));
		pb_addconn->setDown(false);
	}
	if(pb_sub->isDown()){
		QObject::disconnect(work_show,SIGNAL(mousepressed_sign(Point)),this,SLOT(subtract_tissue_clicked(Point)));
		pb_sub->setDown(false);
	}
	pb_add3D->setDown(true);
	QObject::connect(work_show,SIGNAL(mousepressed_sign(Point)),this,SLOT(add_tissue_3D_clicked(Point)));
}*/

void MainWindow::do_work2tissue()
{
	common::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this);

	handler3D->work2tissueall();

	tissues_size_t m;
	handler3D->get_rangetissue(&m);
	handler3D->buildmissingtissues(m);
	//	handler3D->build255tissues();
	tissueTreeWidget->update_tree_widget();

	tissues_size_t currTissueType = tissueTreeWidget->get_current_type();
	tissuenr_changed(currTissueType - 1);

	emit end_datachange(this);
}

void MainWindow::do_tissue2work()
{
	common::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.work = true;
	emit begin_datachange(dataSelection, this);

	handler3D->tissue2workall3D();

	emit end_datachange(this);
}

void MainWindow::do_work2tissue_grouped()
{
	vector<tissues_size_t> olds, news;

	QString filename = Q3FileDialog::getOpenFileName(QString::null,
																									 "Text (*.txt)\n"
																									 "All(*.*)",
																									 this);

	if (!filename.isEmpty()) {
		if (read_grouptissues(filename.ascii(), olds, news)) {
			common::DataSelection dataSelection;
			dataSelection.allSlices = true;
			dataSelection.tissues = true;
			emit begin_datachange(dataSelection, this);

			handler3D->work2tissueall();
			handler3D->group_tissues(olds, news);

			tissues_size_t m;
			handler3D->get_rangetissue(&m);
			handler3D->buildmissingtissues(m);
			//	handler3D->build255tissues();
			tissueTreeWidget->update_tree_widget();

			tissues_size_t currTissueType = tissueTreeWidget->get_current_type();
			tissuenr_changed(currTissueType);

			emit end_datachange(this);
		}
	}
}

void MainWindow::tissueFilterChanged(const QString &text)
{
	tissueTreeWidget->set_tissue_filter(text);
}

void MainWindow::newTissuePressed()
{
	TissueAdder TA(false, tissueTreeWidget, this);
	//TA.move(QCursor::pos());
	TA.exec();

	tissues_size_t currTissueType = tissueTreeWidget->get_current_type();
	tissuenr_changed(currTissueType - 1);
}

void MainWindow::merge()
{
	bool anyLocked = false;
	QList<QTreeWidgetItem *> list_ = tissueTreeWidget->selectedItems();
	for (auto a = list_.begin(); a != list_.end() && !anyLocked; ++a) {
		anyLocked |= TissueInfos::GetTissueLocked(tissueTreeWidget->get_type(*a));
	}
	if (anyLocked) {
		QMessageBox::warning(this, "iSeg", "Locked tissues can not be merged.",
												 QMessageBox::Ok | QMessageBox::Default);
		return;
	}

	int ret = QMessageBox::warning(
			this, "iSeg", "Do you really want to merge the selected tissues?",
			QMessageBox::Yes | QMessageBox::Default, QMessageBox::No,
			QMessageBox::Cancel | QMessageBox::Escape);
	if (ret == QMessageBox::Yes) {
		bool tmp;
		tmp = tissue3Dopt->isChecked();
		tissue3Dopt->setChecked(true);
		selectedtissue2work();
		QList<QTreeWidgetItem *> list;
		list = tissueTreeWidget->selectedItems();
		TissueAdder TA(false, tissueTreeWidget, this);
		//TA.move(QCursor::pos());
		TA.exec();

		tissues_size_t currTissueType = tissueTreeWidget->get_current_type();
		tissuenr_changed(currTissueType - 1);
		handler3D->mergetissues(currTissueType);
		removeselectedmerge(list);
		if (tmp == false)
			tissue3Dopt->setChecked(false);
	}
}

void MainWindow::newFolderPressed()
{
	TissueFolderAdder dialog(tissueTreeWidget, this);
	dialog.exec();
}

void MainWindow::lockAllTissues()
{
	bool lockstate = lockTissues->isOn();
	cb_tissuelock->setChecked(lockstate);
	TissueInfos::SetTissuesLocked(lockstate);
	tissueTreeWidget->update_folder_icons();
	tissueTreeWidget->update_tissue_icons();
}

void MainWindow::modifTissueFolderPressed()
{
	if (tissueTreeWidget->get_current_is_folder()) {
		modifFolder();
	}
	else {
		modifTissue();
	}
}

void MainWindow::modifTissue()
{
	common::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this, false);

	tissues_size_t nr = tissueTreeWidget->get_current_type() - 1;

	TissueAdder TA(true, tissueTreeWidget, this);
	//TA.move(QCursor::pos());
	TA.exec();

	emit end_datachange(this, common::ClearUndo);
}

void MainWindow::modifFolder()
{
	bool ok = false;
	QString newFolderName = QInputDialog::getText(
			"Folder name", "Enter a name for the folder:", QLineEdit::Normal,
			tissueTreeWidget->get_current_name(), &ok, this);
	if (ok) {
		tissueTreeWidget->set_current_folder_name(newFolderName);
	}
}

void MainWindow::removeTissueFolderPressed()
{
	if (tissueTreeWidget->get_current_is_folder()) {
		removeFolder();
	}
	else {
		removeselected();
	}
}

void MainWindow::removeselected()
{
	bool anyLocked = false;
	QList<QTreeWidgetItem *> list_ = tissueTreeWidget->selectedItems();
	for (auto a = list_.begin(); a != list_.end() && !anyLocked; ++a) {
		anyLocked |= TissueInfos::GetTissueLocked(tissueTreeWidget->get_type(*a));
	}
	if (anyLocked) {
		QMessageBox::warning(this, "iSeg", "Locked tissues can not be removed.",
												 QMessageBox::Ok | QMessageBox::Default);
		return;
	}

	bool removeCompletely = false;
	int ret = QMessageBox::warning(
			this, "iSeg", "Do you really want to remove the selected tissues?",
			QMessageBox::Yes | QMessageBox::Default, QMessageBox::No,
			QMessageBox::Cancel | QMessageBox::Escape);
	if (ret == QMessageBox::Yes) {
		removeCompletely = true;
	}
	else {
		return; // Cancel
	}
	QList<QTreeWidgetItem *> list = tissueTreeWidget->selectedItems();
	for (auto a = list.begin(); a != list.end(); ++a) {
		QTreeWidgetItem *item = *a;
		tissues_size_t currTissueType = tissueTreeWidget->get_type(item);
		if (tissueTreeWidget->get_tissue_instance_count(currTissueType) > 1) {
			// There are more than one instance of the same tissue in the tree
			int ret = QMessageBox::question(
					this, "iSeg",
					"There are multiple occurrences\nof a selected tissue in the "
					"hierarchy.\nDo you want to remove them all?",
					QMessageBox::Yes | QMessageBox::Default, QMessageBox::No,
					QMessageBox::Cancel | QMessageBox::Escape);
			if (ret == QMessageBox::Yes) {
				removeCompletely = true;
			}
			else if (ret == QMessageBox::No) {
				removeCompletely = false;
			}
			else {
				return; // Cancel
			}
		}
		else {
			// There is only a single instance of the tissue in the tree
		}

		if (removeCompletely) {
			// Remove tissue completely
			tissues_size_t tissueCount = TissueInfos::GetTissueCount();
			if (tissueCount < 2) {
				QMessageBox::information(this, "iSeg",
																 "It is not possible to erase this tissue.\nAt "
																 "least one tissue must be defined.");
				return;
			}

			common::DataSelection dataSelection;
			dataSelection.allSlices = true;
			dataSelection.tissues = true;
			emit begin_datachange(dataSelection, this, false);

			QString tissueName = TissueInfos::GetTissueName(currTissueType);
			handler3D->remove_tissue(currTissueType, tissueCount);
			tissueTreeWidget->remove_tissue(tissueName);

			emit end_datachange(this, common::ClearUndo);

			currTissueType = tissueTreeWidget->get_current_type();
			tissuenr_changed(currTissueType - 1);
		}
		else {
			// Remove only tissue tree item
			tissueTreeWidget->remove_current_item();
			currTissueType = tissueTreeWidget->get_current_type();
			tissuenr_changed(currTissueType - 1);
		}
	}
}

void MainWindow::removeselectedmerge(QList<QTreeWidgetItem *> list)
{
	for (auto a = list.begin(); a != list.end(); ++a) {
		QTreeWidgetItem *item = *a;
		tissues_size_t currTissueType = tissueTreeWidget->get_type(item);
		tissues_size_t tissueCount = TissueInfos::GetTissueCount();
		common::DataSelection dataSelection;
		dataSelection.allSlices = true;
		dataSelection.tissues = true;
		emit begin_datachange(dataSelection, this, false);

		QString tissueName = TissueInfos::GetTissueName(currTissueType);
		handler3D->remove_tissue(currTissueType, tissueCount);
		tissueTreeWidget->remove_tissue(tissueName);

		emit end_datachange(this, common::ClearUndo);

		currTissueType = tissueTreeWidget->get_current_type();
		tissuenr_changed(currTissueType - 1);
	}
}

void MainWindow::removeFolder()
{
	if (tissueTreeWidget->get_current_has_children()) {
		QMessageBox msgBox;
		msgBox.setText("Remove Folder");
		msgBox.setInformativeText("Do you want to keep all tissues\nand folders "
															"contained in this folder?");
		msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No |
															QMessageBox::Cancel);
		msgBox.setDefaultButton(QMessageBox::Yes);
		int ret = msgBox.exec();
		if (ret == QMessageBox::Yes) {
			tissueTreeWidget->remove_current_item(false);
		}
		else if (ret == QMessageBox::No) {
			// Get child tissues and their instance count in the subtree
			std::map<tissues_size_t, unsigned short> childTissues;
			tissueTreeWidget->get_current_child_tissues(childTissues);

			// Only remove tissues completely with no corresponding item left in the tissue tree
			bool anyLocked = false;
			std::set<tissues_size_t> removeTissues;
			for (std::map<tissues_size_t, unsigned short>::iterator iter =
							 childTissues.begin();
					 iter != childTissues.end(); ++iter) {
				if (iter->second ==
						tissueTreeWidget->get_tissue_instance_count(iter->first)) {
					anyLocked |= TissueInfos::GetTissueLocked(iter->first);
					removeTissues.insert(iter->first);
				}
			}
			if (anyLocked) {
				QMessageBox::warning(this, "iSeg", "Locked tissues can not be removed.",
														 QMessageBox::Ok | QMessageBox::Default);
				return;
			}
			if (removeTissues.size() >= TissueInfos::GetTissueCount()) {
				QMessageBox::information(this, "iSeg",
																 "It is not possible to erase this folder.\nAt "
																 "least one tissue must be defined.");
				return;
			}

			common::DataSelection dataSelection;
			dataSelection.allSlices = true;
			dataSelection.tissues = true;
			emit begin_datachange(dataSelection, this, false);

			// Remove child tissues in SlicesHandler
			handler3D->remove_tissues(removeTissues);

			// Remove item in tree widget
			tissueTreeWidget->remove_current_item(true);

			emit end_datachange(this, common::ClearUndo);

			tissues_size_t currTissueType = tissueTreeWidget->get_current_type();
			tissuenr_changed(currTissueType - 1);
		}
	}
	else {
		tissueTreeWidget->remove_current_item(false);
	}
}

void MainWindow::removeTissueFolderAllPressed()
{
	bool anyLocked = false;
	QList<QTreeWidgetItem *> list_ = tissueTreeWidget->get_all_items();
	for (auto a = list_.begin(); a != list_.end() && !anyLocked; ++a) {
		anyLocked |= TissueInfos::GetTissueLocked(tissueTreeWidget->get_type(*a));
	}
	if (anyLocked) {
		QMessageBox::warning(this, "iSeg", "Locked tissues can not be removed.",
												 QMessageBox::Ok | QMessageBox::Default);
		return;
	}

	int ret = QMessageBox::warning(
			this, "iSeg", "Do you really want to remove all tissues and folders?",
			QMessageBox::Yes | QMessageBox::Default, QMessageBox::No,
			QMessageBox::Cancel | QMessageBox::Escape);
	if (ret == QMessageBox::Yes) {
		common::DataSelection dataSelection;
		dataSelection.allSlices = true;
		dataSelection.tissues = true;
		emit begin_datachange(dataSelection, this, false);

		handler3D->remove_tissueall();
		tissueTreeWidget->remove_all_folders(false);
		tissueTreeWidget->update_tree_widget();
		tissues_size_t currTissueType = tissueTreeWidget->get_current_type();
		tissuenr_changed(currTissueType - 1);

		emit end_datachange(this, common::ClearUndo);
	}
}

// BL TODO replace by selectedtissue2work (SK)
void MainWindow::tissue2work()
{
	common::DataSelection dataSelection;
	dataSelection.allSlices = tissue3Dopt->isChecked();
	dataSelection.sliceNr = handler3D->get_activeslice();
	dataSelection.work = true;
	emit begin_datachange(dataSelection, this);

	tissues_size_t nr = tissueTreeWidget->get_current_type();

	if (tissue3Dopt->isChecked()) {
		handler3D->tissue2work3D(nr);
	}
	else {
		handler3D->tissue2work(nr);
	}

	emit end_datachange(this);
}

void MainWindow::selectedtissue2work()
{
	common::DataSelection dataSelection;
	dataSelection.allSlices = tissue3Dopt->isChecked();
	dataSelection.sliceNr = handler3D->get_activeslice();
	dataSelection.work = true;
	emit begin_datachange(dataSelection, this);

	handler3D->clear_work();
	QList<QTreeWidgetItem *> list;
	list = tissueTreeWidget->selectedItems();
	for (auto a = list.begin(); a != list.end(); ++a) {
		QTreeWidgetItem *item = *a;
		tissues_size_t currTissueType = tissueTreeWidget->get_type(item);

		if (tissue3Dopt->isChecked()) {
			handler3D->selectedtissue2work3D(currTissueType);
		}
		else {
			handler3D->selectedtissue2work(currTissueType);
		}
	}
	emit end_datachange(this);
}

void MainWindow::tissue2workall()
{
	common::DataSelection dataSelection;
	dataSelection.allSlices = tissue3Dopt->isChecked();
	dataSelection.sliceNr = handler3D->get_activeslice();
	dataSelection.work = true;
	emit begin_datachange(dataSelection, this);

	if (tissue3Dopt->isChecked()) {
		handler3D->tissue2workall3D();
	}
	else {
		handler3D->tissue2workall();
	}

	emit end_datachange(this);
}

void MainWindow::cleartissues()
{
	bool anyLocked = false;
	QList<QTreeWidgetItem *> list_ = tissueTreeWidget->get_all_items();
	for (auto a = list_.begin(); a != list_.end() && !anyLocked; ++a) {
		anyLocked |= TissueInfos::GetTissueLocked(tissueTreeWidget->get_type(*a));
	}
	if (anyLocked) {
		QMessageBox::warning(this, "iSeg", "Locked tissues can not be cleared.",
												 QMessageBox::Ok | QMessageBox::Default);
		return;
	}

	int ret = QMessageBox::warning(
			this, "iSeg", "Do you really want to clear all tissues?",
			QMessageBox::Yes | QMessageBox::Default, QMessageBox::No,
			QMessageBox::Cancel | QMessageBox::Escape);
	if (ret == QMessageBox::Yes) {
		common::DataSelection dataSelection;
		dataSelection.allSlices = tissue3Dopt->isChecked();
		dataSelection.sliceNr = handler3D->get_activeslice();
		dataSelection.tissues = true;
		emit begin_datachange(dataSelection, this);

		if (tissue3Dopt->isChecked()) {
			handler3D->cleartissues3D();
		}
		else {
			handler3D->cleartissues();
		}

		emit end_datachange(this);
	}
}

// BL TODO replace by clearselected (SK)
void MainWindow::cleartissue()
{
	bool isLocked =
			TissueInfos::GetTissueLocked(tissueTreeWidget->get_current_type());
	if (isLocked) {
		QMessageBox::warning(this, "iSeg", "Locked tissue can not be removed.",
												 QMessageBox::Ok | QMessageBox::Default);
		return;
	}

	int ret = QMessageBox::warning(
			this, "iSeg", "Do you really want to clear the tissue?",
			QMessageBox::Yes | QMessageBox::Default, QMessageBox::No,
			QMessageBox::Cancel | QMessageBox::Escape);
	if (ret == QMessageBox::Yes) {
		common::DataSelection dataSelection;
		dataSelection.allSlices = tissue3Dopt->isChecked();
		dataSelection.sliceNr = handler3D->get_activeslice();
		dataSelection.tissues = true;
		emit begin_datachange(dataSelection, this);

		tissues_size_t nr = tissueTreeWidget->get_current_type();
		if (tissue3Dopt->isChecked()) {
			handler3D->cleartissue3D(nr);
		}
		else {
			handler3D->cleartissue(nr);
		}

		emit end_datachange(this);
	}
}

void MainWindow::clearselected()
{
	bool anyLocked = false;
	QList<QTreeWidgetItem *> list_ = tissueTreeWidget->selectedItems();
	for (auto a = list_.begin(); a != list_.end() && !anyLocked; ++a) {
		anyLocked |= TissueInfos::GetTissueLocked(tissueTreeWidget->get_type(*a));
	}
	if (anyLocked) {
		QMessageBox::warning(this, "iSeg", "Locked tissues can not be cleared.",
												 QMessageBox::Ok | QMessageBox::Default);
		return;
	}

	int ret = QMessageBox::warning(
			this, "iSeg", "Do you really want to clear tissues?",
			QMessageBox::Yes | QMessageBox::Default, QMessageBox::No,
			QMessageBox::Cancel | QMessageBox::Escape);
	if (ret == QMessageBox::Yes) {
		common::DataSelection dataSelection;
		dataSelection.allSlices = tissue3Dopt->isChecked();
		dataSelection.sliceNr = handler3D->get_activeslice();
		dataSelection.tissues = true;
		emit begin_datachange(dataSelection, this);

		QList<QTreeWidgetItem *> list;
		list = tissueTreeWidget->selectedItems();
		for (auto a = list.begin(); a != list.end(); ++a) {
			QTreeWidgetItem *item = *a;
			tissues_size_t nr = tissueTreeWidget->get_type(item);
			if (tissue3Dopt->isChecked()) {
				handler3D->cleartissue3D(nr);
			}
			else {
				handler3D->cleartissue(nr);
			}
		}
		emit end_datachange(this);
	}
}

void MainWindow::bmptissuevisible_changed()
{
	if (cb_bmptissuevisible->isChecked()) {
		bmp_show->set_tissuevisible(true);
	}
	else {
		bmp_show->set_tissuevisible(false);
	}
}

void MainWindow::bmpoutlinevisible_changed()
{
	if (cb_bmpoutlinevisible->isChecked()) {
		bmp_show->set_workbordervisible(true);
	}
	else {
		bmp_show->set_workbordervisible(false);
	}
}

void MainWindow::worktissuevisible_changed()
{
	if (cb_worktissuevisible->isChecked()) {
		work_show->set_tissuevisible(true);
	}
	else {
		work_show->set_tissuevisible(false);
	}
}

void MainWindow::workpicturevisible_changed()
{
	if (cb_workpicturevisible->isChecked()) {
		work_show->set_picturevisible(true);
	}
	else {
		work_show->set_picturevisible(false);
	}
}

void MainWindow::slice_changed()
{
	QWidget1 *qw = (QWidget1 *)methodTab->visibleWidget();
	qw->slicenr_changed();

	unsigned short slicenr = handler3D->get_activeslice() + 1;
	QObject::disconnect(scb_slicenr, SIGNAL(valueChanged(int)), this,
											SLOT(scb_slicenr_changed()));
	scb_slicenr->setValue(int(slicenr));
	QObject::connect(scb_slicenr, SIGNAL(valueChanged(int)), this,
									 SLOT(scb_slicenr_changed()));
	QObject::disconnect(sb_slicenr, SIGNAL(valueChanged(int)), this,
											SLOT(sb_slicenr_changed()));
	sb_slicenr->setValue(int(slicenr));
	QObject::connect(sb_slicenr, SIGNAL(valueChanged(int)), this,
									 SLOT(sb_slicenr_changed()));
	bmp_show->slicenr_changed();
	work_show->slicenr_changed();
	scalewidget->slicenr_changed();
	imagemathwidget->slicenr_changed();
	imageoverlaywidget->slicenr_changed();
	overlay_widget->slicenr_changed();
	if (handler3D->return_startslice() >= slicenr ||
			handler3D->return_endslice() + 1 <= slicenr) {
		lb_inactivewarning->setText(QString("   3D Inactive Slice!   "));
		lb_inactivewarning->setPaletteBackgroundColor(QColor(0, 255, 0));
	}
	else {
		lb_inactivewarning->setText(QString(" "));
		lb_inactivewarning->setPaletteBackgroundColor(
				this->paletteBackgroundColor());
	}

	if (xsliceshower != NULL) {
		xsliceshower->zpos_changed();
	}
	if (ysliceshower != NULL) {
		ysliceshower->zpos_changed();
	}
}

void MainWindow::slices3d_changed(bool new_bitstack)
{
	if (new_bitstack) {
		bitstack_widget->newloaded();
	}
	overlay_widget->newloaded();
	imageoverlaywidget->newloaded();

	for (size_t i = 0; i < tabwidgets.size(); i++) {
		((QWidget1 *)(tabwidgets[i]))->newloaded();
	}

	QWidget1 *qw = (QWidget1 *)methodTab->visibleWidget();

	if (handler3D->return_nrslices() != nrslices) {
		QObject::disconnect(scb_slicenr, SIGNAL(valueChanged(int)), this,
												SLOT(scb_slicenr_changed()));
		scb_slicenr->setMaxValue((int)handler3D->return_nrslices());
		QObject::connect(scb_slicenr, SIGNAL(valueChanged(int)), this,
										 SLOT(scb_slicenr_changed()));
		QObject::disconnect(sb_slicenr, SIGNAL(valueChanged(int)), this,
												SLOT(sb_slicenr_changed()));
		sb_slicenr->setMaxValue((int)handler3D->return_nrslices());
		QObject::connect(sb_slicenr, SIGNAL(valueChanged(int)), this,
										 SLOT(sb_slicenr_changed()));
		lb_slicenr->setText(QString(" of ") +
												QString::number((int)handler3D->return_nrslices()));
		interpolwidget->handler3D_changed();
		nrslices = handler3D->return_nrslices();
	}

	unsigned short slicenr = handler3D->get_activeslice() + 1;
	QObject::disconnect(scb_slicenr, SIGNAL(valueChanged(int)), this,
											SLOT(scb_slicenr_changed()));
	scb_slicenr->setValue(int(slicenr));
	QObject::connect(scb_slicenr, SIGNAL(valueChanged(int)), this,
									 SLOT(scb_slicenr_changed()));
	QObject::disconnect(sb_slicenr, SIGNAL(valueChanged(int)), this,
											SLOT(sb_slicenr_changed()));
	sb_slicenr->setValue(int(slicenr));
	QObject::connect(sb_slicenr, SIGNAL(valueChanged(int)), this,
									 SLOT(sb_slicenr_changed()));
	if (handler3D->return_startslice() >= slicenr ||
			handler3D->return_endslice() + 1 <= slicenr) {
		lb_inactivewarning->setText(QString("   3D Inactive Slice!   "));
		lb_inactivewarning->setPaletteBackgroundColor(QColor(0, 255, 0));
	}
	else {
		lb_inactivewarning->setText(QString(" "));
		lb_inactivewarning->setPaletteBackgroundColor(
				this->paletteBackgroundColor());
	}

	if (xsliceshower != NULL)
		xsliceshower->zpos_changed();
	if (ysliceshower != NULL)
		ysliceshower->zpos_changed();

	if (VV3D != NULL)
		VV3D->reload();

	if (VV3Dbmp != NULL)
		VV3Dbmp->reload();

	if (SV3D != NULL)
		SV3D->reload();

	if (SV3Dbmp != NULL)
		SV3Dbmp->reload();

	qw->init();
}

void MainWindow::slicenr_up()
{
	handler3D->set_activeslice(handler3D->get_activeslice() + 1);
	slice_changed();
}

void MainWindow::zoom_in()
{
	//	bmp_show->set_zoom(bmp_show->return_zoom()*2);
	//	work_show->set_zoom(work_show->return_zoom()*2);
	zoom_widget->zoom_changed(work_show->return_zoom() * 2);
}

void MainWindow::zoom_out()
{
	//	bmp_show->set_zoom(bmp_show->return_zoom()/2);
	//	work_show->set_zoom(work_show->return_zoom()/2);
	//	zoom_widget->set_zoom(work_show->return_zoom());
	zoom_widget->zoom_changed(work_show->return_zoom() / 2);
}

void MainWindow::slicenr_down()
{
	handler3D->set_activeslice(handler3D->get_activeslice() - 1);
	slice_changed();
}

void MainWindow::sb_slicenr_changed()
{
	handler3D->set_activeslice(sb_slicenr->value() - 1);
	slice_changed();
}

void MainWindow::scb_slicenr_changed()
{
	handler3D->set_activeslice(scb_slicenr->value() - 1);
	slice_changed();
}

void MainWindow::pb_first_pressed()
{
	handler3D->set_activeslice(0);
	slice_changed();
}

void MainWindow::pb_last_pressed()
{
	handler3D->set_activeslice(handler3D->return_nrslices() - 1);
	slice_changed();
}

void MainWindow::slicethickness_changed()
{
	bool b1;
	float thickness = le_slicethick->text().toFloat(&b1);
	if (b1) {
		handler3D->set_slicethickness(thickness);
		if (xsliceshower != NULL)
			xsliceshower->thickness_changed(thickness);
		if (ysliceshower != NULL)
			ysliceshower->thickness_changed(thickness);
		if (VV3D != NULL)
			VV3D->thickness_changed(thickness);
		if (VV3Dbmp != NULL)
			VV3Dbmp->thickness_changed(thickness);
		if (SV3D != NULL)
			SV3D->thickness_changed(thickness);
		if (SV3Dbmp != NULL)
			SV3Dbmp->thickness_changed(thickness);
	}
	else {
		if (le_slicethick->text() != QString("."))
			QApplication::beep();
	}
}

void MainWindow::slicethickness_changed1()
{
	le_slicethick->setText(QString::number(handler3D->get_slicethickness()));

	if (xsliceshower != NULL)
		xsliceshower->thickness_changed(handler3D->get_slicethickness());
	if (ysliceshower != NULL)
		ysliceshower->thickness_changed(handler3D->get_slicethickness());
	if (VV3D != NULL)
		VV3D->thickness_changed(handler3D->get_slicethickness());
	if (VV3Dbmp != NULL)
		VV3Dbmp->thickness_changed(handler3D->get_slicethickness());
	if (SV3D != NULL)
		SV3D->thickness_changed(handler3D->get_slicethickness());
	if (SV3Dbmp != NULL)
		SV3Dbmp->thickness_changed(handler3D->get_slicethickness());
}

void MainWindow::tissuenr_changed(int i)
{
	QWidget *qw = methodTab->visibleWidget();

	if (qw == iftrg_widget)
		iftrg_widget->tissuenr_changed(i);
	else if (qw == interpolwidget)
		interpolwidget->tissuenr_changed(i);

	bmp_show->color_changed(i);
	work_show->color_changed(i);
	OutlineCorrect_widget->tissuenr_changed(i);
	bitstack_widget->tissuenr_changed(i);

	cb_tissuelock->setChecked(TissueInfos::GetTissueLocked(i + 1));
}

void MainWindow::tissue_selection_changed()
{
	QList<QTreeWidgetItem *> list = tissueTreeWidget->selectedItems();
	if (list.size() > 1) {
		showpb_tab[6] = false;
		showpb_tab[14] = false;
		OutlineCorrect_widget->setDisabled(true);
		pickerwidget->setDisabled(true);
		cb_bmpoutlinevisible->setChecked(false);
		bmpoutlinevisible_changed();
		updateTabvisibility();
	}
	else {
		showpb_tab[6] = true;
		showpb_tab[14] = true;
		OutlineCorrect_widget->setDisabled(false);
		pickerwidget->setDisabled(false);
		updateTabvisibility();
	}

	if (list.size() > 0) {
		tissuenr_changed((int)tissueTreeWidget->get_current_type() - 1);
	}
}

void MainWindow::unselectall()
{
	QList<QTreeWidgetItem *> list = tissueTreeWidget->selectedItems();

	for (auto a = list.begin(); a != list.end(); ++a) {
		QTreeWidgetItem *item = *a;
		item->setSelected(false);
	}
}

void MainWindow::tree_widget_doubleclicked(QTreeWidgetItem *item, int column)
{
	modifTissueFolderPressed();
}

void MainWindow::tree_widget_contextmenu(const QPoint &pos)
{
	QList<QTreeWidgetItem *> list = tissueTreeWidget->selectedItems();

	if (list.size() <= 1) // single selection
	{
		Q3PopupMenu contextMenu(tissueTreeWidget, "tissuetreemenu");
		if (tissueTreeWidget->get_current_is_folder()) {
			contextMenu.insertItem("Toggle Lock", cb_tissuelock, SLOT(click()));
			contextMenu.insertSeparator();
			contextMenu.insertItem("New Tissue...", this, SLOT(newTissuePressed()));
			contextMenu.insertItem("New Folder...", this, SLOT(newFolderPressed()));
			contextMenu.insertItem("Mod. Folder...", this,
														 SLOT(modifTissueFolderPressed()));
			contextMenu.insertItem("Del. Folder...", this,
														 SLOT(removeTissueFolderPressed()));
		}
		else {
			contextMenu.insertItem("Toggle Lock", cb_tissuelock, SLOT(click()));
			contextMenu.insertSeparator();
			contextMenu.insertItem("New Tissue...", this, SLOT(newTissuePressed()));
			contextMenu.insertItem("New Folder...", this, SLOT(newFolderPressed()));
			contextMenu.insertItem("Mod. Tissue...", this,
														 SLOT(modifTissueFolderPressed()));
			contextMenu.insertItem("Del. Tissue...", this,
														 SLOT(removeTissueFolderPressed()));
			contextMenu.insertSeparator();
			contextMenu.insertItem("Get Tissue", this, SLOT(tissue2work()));
			contextMenu.insertItem("Clear Tissue", this, SLOT(cleartissue()));
			contextMenu.insertSeparator();
			contextMenu.insertItem("Next Feat. Slice", this,
														 SLOT(next_featuring_slice()));
#ifdef ISEG_GPU_MC
			contextMenu.insertItem("3D View", this, SLOT(startwidget()));
#endif
		}
		contextMenu.insertSeparator();
		int itemId = contextMenu.insertItem("Show Tissue Indices", tissueTreeWidget,
																				SLOT(toggle_show_tissue_indices()));
		contextMenu.setItemChecked(itemId,
															 !tissueTreeWidget->get_tissue_indices_hidden());
		contextMenu.insertItem("Sort By Name", tissueTreeWidget,
													 SLOT(sort_by_tissue_name()));
		contextMenu.insertItem("Sort By Index", tissueTreeWidget,
													 SLOT(sort_by_tissue_index()));
		contextMenu.exec(tissueTreeWidget->viewport()->mapToGlobal(pos));
	}
	else // multi-selection
	{
		Q3PopupMenu contextMenu(tissueTreeWidget, "tissuetreemenu");
		contextMenu.insertItem("Toggle Lock", cb_tissuelock, SLOT(click()));
		contextMenu.insertSeparator();
		contextMenu.insertItem("Delete Selected", this, SLOT(removeselected()));
		contextMenu.insertItem("Clear Selected", this, SLOT(clearselected()));
		contextMenu.insertItem("Get Selected", this, SLOT(selectedtissue2work()));
		contextMenu.insertItem("Merge", this, SLOT(merge()));
		contextMenu.insertItem("Unselect all", this, SLOT(unselectall()));
#ifdef ISEG_GPU_MC
		contextMenu.insertItem("3D View", this, SLOT(startwidget()));
#endif
		contextMenu.exec(tissueTreeWidget->viewport()->mapToGlobal(pos));
	}
}

void MainWindow::tissuelock_toggled()
{
	if (tissueTreeWidget->get_current_is_folder()) {
		// Set lock state of all child tissues
		std::map<tissues_size_t, unsigned short> childTissues;
		tissueTreeWidget->get_current_child_tissues(childTissues);
		for (std::map<tissues_size_t, unsigned short>::iterator iter =
						 childTissues.begin();
				 iter != childTissues.end(); ++iter) {
			TissueInfos::SetTissueLocked(iter->first, cb_tissuelock->isChecked());
		}
	}
	else {
		QList<QTreeWidgetItem *> list;
		list = tissueTreeWidget->selectedItems();
		for (auto a = list.begin(); a != list.end(); ++a) {
			QTreeWidgetItem *item = *a;
			tissues_size_t currTissueType = tissueTreeWidget->get_type(item);
			//tissues_size_t currTissueType = tissueTreeWidget->get_current_type();
			TissueInfos::SetTissueLocked(currTissueType, cb_tissuelock->isChecked());
		}
	}
	tissueTreeWidget->update_tissue_icons();
	tissueTreeWidget->update_folder_icons();

	if (!cb_tissuelock->isChecked())
		lockTissues->setOn(false);
}

void MainWindow::execute_undo()
{
	if (handler3D->return_nrundo() > 0) {
		common::DataSelection selectedData;
		if (methodTab->visibleWidget() == transfrmWidget) {
			cancel_transform_helper();
			selectedData = handler3D->undo();
			transfrmWidget->init();
		}
		else {
			selectedData = handler3D->undo();
		}

		// Update ranges
		update_ranges_helper();

		//	if(undotype & )
		slice_changed();

		if (selectedData.DataSelected()) {
			editmenu->setItemEnabled(redonr, true);
			if (handler3D->return_nrundo() == 0)
				editmenu->setItemEnabled(undonr, false);
		}
	}
}

void MainWindow::execute_redo()
{
	common::DataSelection selectedData;
	if (methodTab->visibleWidget() == transfrmWidget) {
		cancel_transform_helper();
		selectedData = handler3D->redo();
		transfrmWidget->init();
	}
	else {
		selectedData = handler3D->redo();
	}

	// Update ranges
	update_ranges_helper();

	//	if(undotype & )
	slice_changed();

	if (selectedData.DataSelected()) {
		editmenu->setItemEnabled(undonr, true);
		if (handler3D->return_nrredo() == 0)
			editmenu->setItemEnabled(redonr, false);
	}
}

void MainWindow::clear_stack() { bitstack_widget->clear_stack(); }

/*void MainWindow::do_startundo(unsigned short undotype, unsigned short slicenr1)
{
	handler3D->start_undo(undotype,slicenr1);
}

void MainWindow::do_endundo()
{
	handler3D->end_undo();
	editmenu->setItemEnabled(redonr,false);
	editmenu->setItemEnabled(undonr,true);
}*/

void MainWindow::do_undostepdone()
{
	editmenu->setItemEnabled(redonr, false);
	editmenu->setItemEnabled(undonr, handler3D->return_nrundo() > 0);
}

void MainWindow::do_clearundo()
{
	handler3D->clear_undo();
	editmenu->setItemEnabled(redonr, false);
	editmenu->setItemEnabled(undonr, false);
}

void MainWindow::tab_changed(QWidget *qw)
{
	//	int index=methodTab->currentPageIndex();
	if (qw != tab_old) {
		/*		if(qw==threshold_widget||qw==wshed_widget||qw==hyst_widget||qw==iftrg_widget||
				 qw==FMF_widget||qw==OutlineCorrect_widget){
					bmp_show->set_workbordervisible(true);
					cb_bmpoutlinevisible->setChecked(true);
				} else {
					bmp_show->set_workbordervisible(false);
					cb_bmpoutlinevisible->setChecked(false);
				}*/

		if (tab_old == threshold_widget) {
			QObject::disconnect(this, SIGNAL(bmp_changed()), threshold_widget,
													SLOT(bmp_changed()));
		}
		else if (tab_old == measurement_widget) {
			measurement_widget->cleanup();
			//		QObject::connect(this,SIGNAL(marks_changed()),work_show,SLOT(mark_changed()));
			QObject::disconnect(this, SIGNAL(marks_changed()), measurement_widget,
													SLOT(marks_changed()));
			QObject::disconnect(bmp_show, SIGNAL(mousepressed_sign(Point)),
													measurement_widget, SLOT(pt_clicked(Point)));
			QObject::disconnect(work_show, SIGNAL(mousepressed_sign(Point)),
													measurement_widget, SLOT(pt_clicked(Point)));
			QObject::disconnect(measurement_widget,
													SIGNAL(vp1_changed(std::vector<Point> *)), bmp_show,
													SLOT(set_vp1(std::vector<Point> *)));
			QObject::disconnect(measurement_widget,
													SIGNAL(vp1_changed(std::vector<Point> *)), work_show,
													SLOT(set_vp1(std::vector<Point> *)));
			QObject::disconnect(measurement_widget,
													SIGNAL(vpdyn_changed(std::vector<Point> *)), bmp_show,
													SLOT(set_vpdyn(std::vector<Point> *)));
			QObject::disconnect(measurement_widget,
													SIGNAL(vpdyn_changed(std::vector<Point> *)),
													work_show, SLOT(set_vpdyn(std::vector<Point> *)));
			QObject::disconnect(
					measurement_widget,
					SIGNAL(
							vp1dyn_changed(std::vector<Point> *, std::vector<Point> *, bool)),
					bmp_show,
					SLOT(set_vp1_dyn(std::vector<Point> *, std::vector<Point> *, bool)));
			QObject::disconnect(
					measurement_widget,
					SIGNAL(
							vp1dyn_changed(std::vector<Point> *, std::vector<Point> *, bool)),
					work_show,
					SLOT(set_vp1_dyn(std::vector<Point> *, std::vector<Point> *, bool)));
			QObject::disconnect(bmp_show, SIGNAL(mousemoved_sign(Point)),
													measurement_widget, SLOT(pt_moved(Point)));
			QObject::disconnect(work_show, SIGNAL(mousemoved_sign(Point)),
													measurement_widget, SLOT(pt_moved(Point)));

			work_show->setMouseTracking(false);
			bmp_show->setMouseTracking(false);
		}
		else if (tab_old == vesselextr_widget) {
			vesselextr_widget->clean_up();
			QObject::disconnect(this, SIGNAL(marks_changed()), measurement_widget,
													SLOT(marks_changed()));
			QObject::disconnect(vesselextr_widget,
													SIGNAL(vp1_changed(std::vector<Point> *)), bmp_show,
													SLOT(set_vp1(std::vector<Point> *)));
		}
		else if (tab_old == wshed_widget) {
			//		QObject::connect(this,SIGNAL(marks_changed()),work_show,SLOT(mark_changed()));
			QObject::disconnect(this, SIGNAL(marks_changed()), wshed_widget,
													SLOT(marks_changed()));
		}
		else if (tab_old == hyst_widget) {
			hyst_widget->clean_up();

			QObject::disconnect(this, SIGNAL(bmp_changed()), hyst_widget,
													SLOT(bmp_changed()));
			QObject::disconnect(bmp_show, SIGNAL(mousepressed_sign(Point)),
													hyst_widget, SLOT(pt_clicked(Point)));
			QObject::disconnect(bmp_show, SIGNAL(mousemoved_sign(Point)), hyst_widget,
													SLOT(pt_moved(Point)));
			QObject::disconnect(bmp_show, SIGNAL(mousereleased_sign(Point)),
													hyst_widget, SLOT(pt_released(Point)));

			QObject::disconnect(hyst_widget,
													SIGNAL(vp1_changed(std::vector<Point> *)), bmp_show,
													SLOT(set_vp1(std::vector<Point> *)));
			QObject::disconnect(hyst_widget,
													SIGNAL(vpdyn_changed(std::vector<Point> *)), bmp_show,
													SLOT(set_vpdyn(std::vector<Point> *)));
			QObject::disconnect(
					hyst_widget,
					SIGNAL(vp1dyn_changed(std::vector<Point> *, std::vector<Point> *)),
					bmp_show,
					SLOT(set_vp1_dyn(std::vector<Point> *, std::vector<Point> *)));
		}
		else if (tab_old == lw_widget) {
			lw_widget->cleanup();

			QObject::disconnect(this, SIGNAL(bmp_changed()), lw_widget,
													SLOT(bmp_changed()));
			QObject::disconnect(bmp_show, SIGNAL(mousepressed_sign(Point)), lw_widget,
													SLOT(pt_clicked(Point)));
			QObject::disconnect(bmp_show, SIGNAL(mousedoubleclick_sign(Point)),
													lw_widget, SLOT(pt_doubleclicked(Point)));
			QObject::disconnect(bmp_show, SIGNAL(mousepressedmid_sign(Point)),
													lw_widget, SLOT(pt_midclicked(Point)));
			QObject::disconnect(bmp_show, SIGNAL(mousedoubleclickmid_sign(Point)),
													lw_widget, SLOT(pt_doubleclickedmid(Point)));
			QObject::disconnect(bmp_show, SIGNAL(mousemoved_sign(Point)), lw_widget,
													SLOT(pt_moved(Point)));
			QObject::disconnect(bmp_show, SIGNAL(mousereleased_sign(Point)),
													lw_widget, SLOT(pt_released(Point)));
			QObject::disconnect(lw_widget, SIGNAL(vp1_changed(std::vector<Point> *)),
													bmp_show, SLOT(set_vp1(std::vector<Point> *)));
			QObject::disconnect(lw_widget,
													SIGNAL(vpdyn_changed(std::vector<Point> *)), bmp_show,
													SLOT(set_vpdyn(std::vector<Point> *)));
			QObject::disconnect(
					lw_widget,
					SIGNAL(vp1dyn_changed(std::vector<Point> *, std::vector<Point> *)),
					bmp_show,
					SLOT(set_vp1_dyn(std::vector<Point> *, std::vector<Point> *)));

			bmp_show->setMouseTracking(false);
		}
		else if (tab_old == iftrg_widget) {
			iftrg_widget->cleanup();

			QObject::disconnect(this, SIGNAL(bmp_changed()), iftrg_widget,
													SLOT(bmp_changed()));

			QObject::disconnect(bmp_show, SIGNAL(mousepressed_sign(Point)),
													iftrg_widget, SLOT(mouse_clicked(Point)));
			QObject::disconnect(bmp_show, SIGNAL(mousereleased_sign(Point)),
													iftrg_widget, SLOT(mouse_released(Point)));
			QObject::disconnect(bmp_show, SIGNAL(mousemoved_sign(Point)),
													iftrg_widget, SLOT(mouse_moved(Point)));

			QObject::disconnect(iftrg_widget, SIGNAL(vm_changed(std::vector<mark> *)),
													bmp_show, SLOT(set_vm(std::vector<mark> *)));
			QObject::disconnect(iftrg_widget,
													SIGNAL(vmdyn_changed(std::vector<Point> *)), bmp_show,
													SLOT(set_vpdyn(std::vector<Point> *)));

			//			QObject::disconnect(tissueTreeWidget,SIGNAL(activated(int)),iftrg_widget,SLOT(tissuenr_changed(int)));
			//			QObject::disconnect(tissueTreeWidget,SIGNAL(activated(int)),this,SLOT(tissuenr_changed(int)));
		}
		else if (tab_old == FMF_widget) {
			FMF_widget->cleanup();

			QObject::disconnect(this, SIGNAL(bmp_changed()), FMF_widget,
													SLOT(bmp_changed()));

			QObject::disconnect(bmp_show, SIGNAL(mousepressed_sign(Point)),
													FMF_widget, SLOT(mouse_clicked(Point)));
			QObject::disconnect(bmp_show, SIGNAL(mousereleased_sign(Point)),
													FMF_widget, SLOT(mouse_released(Point)));
			QObject::disconnect(bmp_show, SIGNAL(mousemoved_sign(Point)), FMF_widget,
													SLOT(mouse_moved(Point)));

			QObject::disconnect(FMF_widget,
													SIGNAL(vpdyn_changed(std::vector<Point> *)), bmp_show,
													SLOT(set_vpdyn(std::vector<Point> *)));
		}
		else if (tab_old == OutlineCorrect_widget) {
			OutlineCorrect_widget->cleanup();
			QObject::disconnect(bmp_show, SIGNAL(mousepressed_sign(Point)),
													OutlineCorrect_widget, SLOT(mouse_clicked(Point)));
			QObject::disconnect(bmp_show, SIGNAL(mousereleased_sign(Point)),
													OutlineCorrect_widget, SLOT(mouse_released(Point)));
			QObject::disconnect(bmp_show, SIGNAL(mousemoved_sign(Point)),
													OutlineCorrect_widget, SLOT(mouse_moved(Point)));

			QObject::disconnect(work_show, SIGNAL(mousepressed_sign(Point)),
													OutlineCorrect_widget, SLOT(mouse_clicked(Point)));
			QObject::disconnect(work_show, SIGNAL(mousereleased_sign(Point)),
													OutlineCorrect_widget, SLOT(mouse_released(Point)));
			QObject::disconnect(work_show, SIGNAL(mousemoved_sign(Point)),
													OutlineCorrect_widget, SLOT(mouse_moved(Point)));

			QObject::disconnect(this, SIGNAL(work_changed()), OutlineCorrect_widget,
													SLOT(workbits_changed()));

			QObject::disconnect(OutlineCorrect_widget,
													SIGNAL(vpdyn_changed(std::vector<Point> *)), bmp_show,
													SLOT(set_vpdyn(std::vector<Point> *)));
			QObject::disconnect(OutlineCorrect_widget,
													SIGNAL(vpdyn_changed(std::vector<Point> *)),
													work_show, SLOT(set_vpdyn(std::vector<Point> *)));
		}
		else if (tab_old == feature_widget) {
			QObject::disconnect(bmp_show, SIGNAL(mousepressed_sign(Point)),
													feature_widget, SLOT(pt_clicked(Point)));
			QObject::disconnect(bmp_show, SIGNAL(mousemoved_sign(Point)),
													feature_widget, SLOT(pt_moved(Point)));
			QObject::disconnect(bmp_show, SIGNAL(mousereleased_sign(Point)),
													feature_widget, SLOT(pt_released(Point)));
			QObject::disconnect(work_show, SIGNAL(mousepressed_sign(Point)),
													feature_widget, SLOT(pt_clicked(Point)));
			QObject::disconnect(work_show, SIGNAL(mousemoved_sign(Point)),
													feature_widget, SLOT(kpt_moved(Point)));
			QObject::disconnect(work_show, SIGNAL(mousereleased_sign(Point)),
													feature_widget, SLOT(pt_released(Point)));
			QObject::disconnect(feature_widget,
													SIGNAL(vpdyn_changed(std::vector<Point> *)), bmp_show,
													SLOT(set_vpdyn(std::vector<Point> *)));
			QObject::disconnect(feature_widget,
													SIGNAL(vpdyn_changed(std::vector<Point> *)),
													work_show, SLOT(set_vpdyn(std::vector<Point> *)));

			work_show->setMouseTracking(false);
			bmp_show->setMouseTracking(false);
		}
		else if (tab_old == interpolwidget) {
			//			QObject::disconnect(tissueTreeWidget,SIGNAL(activated(int)),interpolwidget,SLOT(tissuenr_changed(int)));
			//			QObject::disconnect(tissueTreeWidget,SIGNAL(activated(int)),this,SLOT(tissuenr_changed(int)));
		}
		else if (tab_old == pickerwidget) {
			pickerwidget->cleanup();
			QObject::disconnect(bmp_show, SIGNAL(mousepressed_sign(Point)),
													pickerwidget, SLOT(pt_clicked(Point)));
			QObject::disconnect(work_show, SIGNAL(mousepressed_sign(Point)),
													pickerwidget, SLOT(pt_clicked(Point)));

			QObject::disconnect(pickerwidget,
													SIGNAL(vp1_changed(std::vector<Point> *)), bmp_show,
													SLOT(set_vp1(std::vector<Point> *)));
			QObject::disconnect(pickerwidget,
													SIGNAL(vp1_changed(std::vector<Point> *)), work_show,
													SLOT(set_vp1(std::vector<Point> *)));
		}
		else if (tab_old == transfrmWidget) {
			transfrmWidget->cleanup();
			QObject::disconnect(bmp_show, SIGNAL(mousepressed_sign(Point)),
													transfrmWidget, SLOT(pt_clicked(Point)));
			QObject::disconnect(work_show, SIGNAL(mousepressed_sign(Point)),
													transfrmWidget, SLOT(pt_clicked(Point)));
			QObject::disconnect(this, SIGNAL(bmp_changed()), transfrmWidget,
													SLOT(bmp_changed()));
			QObject::disconnect(this, SIGNAL(work_changed()), transfrmWidget,
													SLOT(work_changed()));
			QObject::disconnect(this, SIGNAL(tissues_changed()), transfrmWidget,
													SLOT(tissues_changed()));
		}

		if (qw == threshold_widget) {
			QObject::connect(this, SIGNAL(bmp_changed()), threshold_widget,
											 SLOT(bmp_changed()));
			//			threshold_widget->init();
		}
		else if (qw == measurement_widget) {
			QObject::connect(this, SIGNAL(marks_changed()), measurement_widget,
											 SLOT(marks_changed()));
			QObject::connect(bmp_show, SIGNAL(mousepressed_sign(Point)),
											 measurement_widget, SLOT(pt_clicked(Point)));
			QObject::connect(work_show, SIGNAL(mousepressed_sign(Point)),
											 measurement_widget, SLOT(pt_clicked(Point)));
			QObject::connect(measurement_widget,
											 SIGNAL(vp1_changed(std::vector<Point> *)), bmp_show,
											 SLOT(set_vp1(std::vector<Point> *)));
			QObject::connect(measurement_widget,
											 SIGNAL(vp1_changed(std::vector<Point> *)), work_show,
											 SLOT(set_vp1(std::vector<Point> *)));
			QObject::connect(measurement_widget,
											 SIGNAL(vpdyn_changed(std::vector<Point> *)), bmp_show,
											 SLOT(set_vpdyn(std::vector<Point> *)));
			QObject::connect(measurement_widget,
											 SIGNAL(vpdyn_changed(std::vector<Point> *)), work_show,
											 SLOT(set_vpdyn(std::vector<Point> *)));
			QObject::connect(
					measurement_widget,
					SIGNAL(
							vp1dyn_changed(std::vector<Point> *, std::vector<Point> *, bool)),
					bmp_show,
					SLOT(set_vp1_dyn(std::vector<Point> *, std::vector<Point> *, bool)));
			QObject::connect(
					measurement_widget,
					SIGNAL(
							vp1dyn_changed(std::vector<Point> *, std::vector<Point> *, bool)),
					work_show,
					SLOT(set_vp1_dyn(std::vector<Point> *, std::vector<Point> *, bool)));
			QObject::connect(bmp_show, SIGNAL(mousemoved_sign(Point)),
											 measurement_widget, SLOT(pt_moved(Point)));
			QObject::connect(work_show, SIGNAL(mousemoved_sign(Point)),
											 measurement_widget, SLOT(pt_moved(Point)));
			work_show->setMouseTracking(true);
			bmp_show->setMouseTracking(true);
		}
		else if (qw == vesselextr_widget) {
			QObject::connect(this, SIGNAL(marks_changed()), vesselextr_widget,
											 SLOT(marks_changed()));
			QObject::connect(vesselextr_widget,
											 SIGNAL(vp1_changed(std::vector<Point> *)), bmp_show,
											 SLOT(set_vp1(std::vector<Point> *)));
		}
		else if (qw == wshed_widget) {
			//		QObject::disconnect(this,SIGNAL(marks_changed()),work_show,SLOT(mark_changed()));
			QObject::connect(this, SIGNAL(marks_changed()), wshed_widget,
											 SLOT(marks_changed()));
			//			wshed_widget->init();
		}
		else if (qw == hyst_widget) {
			QObject::connect(this, SIGNAL(bmp_changed()), hyst_widget,
											 SLOT(bmp_changed()));
			QObject::connect(bmp_show, SIGNAL(mousepressed_sign(Point)), hyst_widget,
											 SLOT(pt_clicked(Point)));
			QObject::connect(bmp_show, SIGNAL(mousemoved_sign(Point)), hyst_widget,
											 SLOT(pt_moved(Point)));
			QObject::connect(bmp_show, SIGNAL(mousereleased_sign(Point)), hyst_widget,
											 SLOT(pt_released(Point)));

			QObject::connect(hyst_widget, SIGNAL(vp1_changed(std::vector<Point> *)),
											 bmp_show, SLOT(set_vp1(std::vector<Point> *)));
			QObject::connect(hyst_widget, SIGNAL(vpdyn_changed(std::vector<Point> *)),
											 bmp_show, SLOT(set_vpdyn(std::vector<Point> *)));
			QObject::connect(
					hyst_widget,
					SIGNAL(vp1dyn_changed(std::vector<Point> *, std::vector<Point> *)),
					bmp_show,
					SLOT(set_vp1_dyn(std::vector<Point> *, std::vector<Point> *)));

			//			hyst_widget->init();
		}
		else if (qw == lw_widget) {
			QObject::connect(this, SIGNAL(bmp_changed()), lw_widget,
											 SLOT(bmp_changed()));
			QObject::connect(bmp_show, SIGNAL(mousepressed_sign(Point)), lw_widget,
											 SLOT(pt_clicked(Point)));
			QObject::connect(bmp_show, SIGNAL(mousedoubleclick_sign(Point)),
											 lw_widget, SLOT(pt_doubleclicked(Point)));
			QObject::connect(bmp_show, SIGNAL(mousepressedmid_sign(Point)), lw_widget,
											 SLOT(pt_midclicked(Point)));
			QObject::connect(bmp_show, SIGNAL(mousedoubleclickmid_sign(Point)),
											 lw_widget, SLOT(pt_doubleclickedmid(Point)));
			QObject::connect(bmp_show, SIGNAL(mousemoved_sign(Point)), lw_widget,
											 SLOT(pt_moved(Point)));
			QObject::connect(bmp_show, SIGNAL(mousereleased_sign(Point)), lw_widget,
											 SLOT(pt_released(Point)));
			QObject::connect(lw_widget, SIGNAL(vp1_changed(std::vector<Point> *)),
											 bmp_show, SLOT(set_vp1(std::vector<Point> *)));
			QObject::connect(lw_widget, SIGNAL(vpdyn_changed(std::vector<Point> *)),
											 bmp_show, SLOT(set_vpdyn(std::vector<Point> *)));
			QObject::connect(
					lw_widget,
					SIGNAL(vp1dyn_changed(std::vector<Point> *, std::vector<Point> *)),
					bmp_show,
					SLOT(set_vp1_dyn(std::vector<Point> *, std::vector<Point> *)));

			//			lw_widget->init();
			bmp_show->setMouseTracking(true);
		}
		else if (qw == iftrg_widget) {
			tissues_size_t currTissueType = tissueTreeWidget->get_current_type();
			iftrg_widget->tissuenr_changed(currTissueType);

			QObject::connect(this, SIGNAL(bmp_changed()), iftrg_widget,
											 SLOT(bmp_changed()));

			QObject::connect(bmp_show, SIGNAL(mousepressed_sign(Point)), iftrg_widget,
											 SLOT(mouse_clicked(Point)));
			QObject::connect(bmp_show, SIGNAL(mousereleased_sign(Point)),
											 iftrg_widget, SLOT(mouse_released(Point)));
			QObject::connect(bmp_show, SIGNAL(mousemoved_sign(Point)), iftrg_widget,
											 SLOT(mouse_moved(Point)));

			QObject::connect(iftrg_widget, SIGNAL(vm_changed(std::vector<mark> *)),
											 bmp_show, SLOT(set_vm(std::vector<mark> *)));
			QObject::connect(iftrg_widget,
											 SIGNAL(vmdyn_changed(std::vector<Point> *)), bmp_show,
											 SLOT(set_vpdyn(std::vector<Point> *)));
		}
		else if (qw == FMF_widget) {
			QObject::connect(this, SIGNAL(bmp_changed()), FMF_widget,
											 SLOT(bmp_changed()));

			QObject::connect(bmp_show, SIGNAL(mousepressed_sign(Point)), FMF_widget,
											 SLOT(mouse_clicked(Point)));
			QObject::connect(bmp_show, SIGNAL(mousereleased_sign(Point)), FMF_widget,
											 SLOT(mouse_released(Point)));
			QObject::connect(bmp_show, SIGNAL(mousemoved_sign(Point)), FMF_widget,
											 SLOT(mouse_moved(Point)));

			QObject::connect(FMF_widget, SIGNAL(vpdyn_changed(std::vector<Point> *)),
											 bmp_show, SLOT(set_vpdyn(std::vector<Point> *)));
		}
		else if (qw == OutlineCorrect_widget) {
			QObject::connect(bmp_show, SIGNAL(mousepressed_sign(Point)),
											 OutlineCorrect_widget, SLOT(mouse_clicked(Point)));
			QObject::connect(bmp_show, SIGNAL(mousereleased_sign(Point)),
											 OutlineCorrect_widget, SLOT(mouse_released(Point)));
			QObject::connect(bmp_show, SIGNAL(mousemoved_sign(Point)),
											 OutlineCorrect_widget, SLOT(mouse_moved(Point)));

			QObject::connect(work_show, SIGNAL(mousepressed_sign(Point)),
											 OutlineCorrect_widget, SLOT(mouse_clicked(Point)));
			QObject::connect(work_show, SIGNAL(mousereleased_sign(Point)),
											 OutlineCorrect_widget, SLOT(mouse_released(Point)));
			QObject::connect(work_show, SIGNAL(mousemoved_sign(Point)),
											 OutlineCorrect_widget, SLOT(mouse_moved(Point)));

			QObject::connect(this, SIGNAL(work_changed()), OutlineCorrect_widget,
											 SLOT(workbits_changed()));

			QObject::connect(OutlineCorrect_widget,
											 SIGNAL(vpdyn_changed(std::vector<Point> *)), bmp_show,
											 SLOT(set_vpdyn(std::vector<Point> *)));
			QObject::connect(OutlineCorrect_widget,
											 SIGNAL(vpdyn_changed(std::vector<Point> *)), work_show,
											 SLOT(set_vpdyn(std::vector<Point> *)));

			OutlineCorrect_widget->workbits_changed();
			//			OutlineCorrect_widget->init();
		}
		else if (qw == feature_widget) {
			QObject::connect(bmp_show, SIGNAL(mousepressed_sign(Point)),
											 feature_widget, SLOT(pt_clicked(Point)));
			QObject::connect(bmp_show, SIGNAL(mousemoved_sign(Point)), feature_widget,
											 SLOT(pt_moved(Point)));
			QObject::connect(bmp_show, SIGNAL(mousereleased_sign(Point)),
											 feature_widget, SLOT(pt_released(Point)));
			QObject::connect(work_show, SIGNAL(mousepressed_sign(Point)),
											 feature_widget, SLOT(pt_clicked(Point)));
			QObject::connect(work_show, SIGNAL(mousemoved_sign(Point)),
											 feature_widget, SLOT(pt_moved(Point)));
			QObject::connect(work_show, SIGNAL(mousereleased_sign(Point)),
											 feature_widget, SLOT(pt_released(Point)));
			QObject::connect(feature_widget,
											 SIGNAL(vpdyn_changed(std::vector<Point> *)), bmp_show,
											 SLOT(set_vpdyn(std::vector<Point> *)));
			QObject::connect(feature_widget,
											 SIGNAL(vpdyn_changed(std::vector<Point> *)), work_show,
											 SLOT(set_vpdyn(std::vector<Point> *)));

			work_show->setMouseTracking(true);
			bmp_show->setMouseTracking(true);
			//			feature_widget->init();
		}
		else if (qw == interpolwidget) {
			//			QObject::connect(tissueTreeWidget,SIGNAL(activated(int)),interpolwidget,SLOT(tissuenr_changed(int)));
			//			QObject::connect(tissueTreeWidget,SIGNAL(activated(int)),this,SLOT(tissuenr_changed(int)));
		}
		else if (qw == pickerwidget) {
			QObject::connect(bmp_show, SIGNAL(mousepressed_sign(Point)), pickerwidget,
											 SLOT(pt_clicked(Point)));
			QObject::connect(work_show, SIGNAL(mousepressed_sign(Point)),
											 pickerwidget, SLOT(pt_clicked(Point)));

			QObject::connect(pickerwidget, SIGNAL(vp1_changed(vector<Point> *)),
											 bmp_show, SLOT(set_vp1(vector<Point> *)));
			QObject::connect(pickerwidget, SIGNAL(vp1_changed(vector<Point> *)),
											 work_show, SLOT(set_vp1(vector<Point> *)));
		}
		else if (qw == transfrmWidget) {
			QObject::connect(bmp_show, SIGNAL(mousepressed_sign(Point)),
											 transfrmWidget, SLOT(pt_clicked(Point)));
			QObject::connect(work_show, SIGNAL(mousepressed_sign(Point)),
											 transfrmWidget, SLOT(pt_clicked(Point)));
			QObject::connect(this, SIGNAL(bmp_changed()), transfrmWidget,
											 SLOT(bmp_changed()));
			QObject::connect(this, SIGNAL(work_changed()), transfrmWidget,
											 SLOT(work_changed()));
			QObject::connect(this, SIGNAL(tissues_changed()), transfrmWidget,
											 SLOT(tissues_changed()));
		}

		tab_old = (QWidget1 *)qw;
		tab_old->init();
		bmp_show->setCursor(*(tab_old->m_cursor));
		work_show->setCursor(*(tab_old->m_cursor));

		updateMethodButtonsPressed(tab_old);
	}
	else
		tab_old = (QWidget1 *)qw;

	tab_old->setFocus();

	return;
}

void MainWindow::updateTabvisibility()
{
	unsigned short counter = 0;
	for (unsigned short i = 0; i < nrtabbuttons; i++) {
		if (showpb_tab[i])
			counter++;
	}
	unsigned short counter1 = 0;
	for (unsigned short i = 0; i < nrtabbuttons; i++) {
		if (showpb_tab[i]) {
			pb_tab[counter1]->setIconSet(tabwidgets[i]->GetIcon(m_picpath));
			pb_tab[counter1]->setText(tabwidgets[i]->GetName().c_str());
			pb_tab[counter1]->setToolTip(tabwidgets[i]->toolTip());
			pb_tab[counter1]->show();
			counter1++;
			if (counter1 == (counter + 1) / 2) {
				while (counter1 < (nrtabbuttons + 1) / 2) {
					pb_tab[counter1]->hide();
					counter1++;
				}
			}
		}
	}

	while (counter1 < nrtabbuttons) {
		pb_tab[counter1]->hide();
		counter1++;
	}

	QWidget1 *qw = (QWidget1 *)methodTab->visibleWidget();
	updateMethodButtonsPressed(qw);
}

void MainWindow::updateMethodButtonsPressed(QWidget1 *qw)
{
	//	QWidget *qw=methodTab->visibleWidget();
	unsigned short counter = 0;
	unsigned short pos = nrtabbuttons;
	for (unsigned short i = 0; i < nrtabbuttons; i++) {
		if (showpb_tab[i]) {
			if (tabwidgets[i] == qw)
				pos = counter;
			counter++;
		}
	}

	unsigned short counter1 = 0;
	for (unsigned short i = 0; i < (counter + 1) / 2; i++, counter1++) {
		if (counter1 == pos)
			pb_tab[i]->setOn(true);
		else
			pb_tab[i]->setOn(false);
	}

	for (unsigned short i = (nrtabbuttons + 1) / 2; counter1 < counter;
			 i++, counter1++) {
		if (counter1 == pos)
			pb_tab[i]->setOn(true);
		else
			pb_tab[i]->setOn(false);
	}
}

void MainWindow::LoadAtlas(QDir path1)
{
	m_atlasfilename.m_atlasdir = path1;
	QStringList filters;
	filters << "*.atl";
	QStringList names1 = path1.entryList(filters);
	m_atlasfilename.nratlases = (int)names1.size();
	if (m_atlasfilename.nratlases > m_atlasfilename.maxnr)
		m_atlasfilename.nratlases = m_atlasfilename.maxnr;
	for (size_t i = 0; i < m_atlasfilename.nratlases; i++) {
		m_atlasfilename.m_atlasfilename[i] = names1[i];
		QFileInfo names1fi(names1[i]);
		atlasmenu->changeItem(m_atlasfilename.atlasnr[i],
													names1fi.completeBaseName());
		atlasmenu->setItemVisible(m_atlasfilename.atlasnr[i], true);
	}
	for (size_t i = m_atlasfilename.nratlases; i < m_atlasfilename.maxnr; i++) {
		atlasmenu->setItemVisible(m_atlasfilename.atlasnr[i], false);
	}

	if (!names1.empty())
		atlasmenu->setItemVisible(m_atlasfilename.separatornr, true);
}

void MainWindow::LoadLoadProj(QString path1)
{
	unsigned short projcounter = 0;
	FILE *fplatestproj = fopen(path1.ascii(), "r");
	char c;
	m_loadprojfilename.m_filename = path1;
	while (fplatestproj != NULL && projcounter < 4) {
		projcounter++;
		QString qs_filename1 = "";
		while (fscanf(fplatestproj, "%c", &c) == 1 && c != '\n') {
			qs_filename1 += c;
		}
		if (!qs_filename1.isEmpty())
			AddLoadProj(qs_filename1);
	}
	if (fplatestproj != 0)
		fclose(fplatestproj);
}

void MainWindow::AddLoadProj(QString path1)
{
	if (m_loadprojfilename.m_loadprojfilename1 != path1 &&
			m_loadprojfilename.m_loadprojfilename2 != path1 &&
			m_loadprojfilename.m_loadprojfilename3 != path1 &&
			m_loadprojfilename.m_loadprojfilename4 != path1) {
		m_loadprojfilename.m_loadprojfilename4 =
				m_loadprojfilename.m_loadprojfilename3;
		m_loadprojfilename.m_loadprojfilename3 =
				m_loadprojfilename.m_loadprojfilename2;
		m_loadprojfilename.m_loadprojfilename2 =
				m_loadprojfilename.m_loadprojfilename1;
		m_loadprojfilename.m_loadprojfilename1 = path1;
	}
	else {
		if (m_loadprojfilename.m_loadprojfilename2 == path1) {
			QString dummy = m_loadprojfilename.m_loadprojfilename2;
			m_loadprojfilename.m_loadprojfilename2 =
					m_loadprojfilename.m_loadprojfilename1;
			m_loadprojfilename.m_loadprojfilename1 = dummy;
		}
		if (m_loadprojfilename.m_loadprojfilename3 == path1) {
			QString dummy = m_loadprojfilename.m_loadprojfilename3;
			m_loadprojfilename.m_loadprojfilename3 =
					m_loadprojfilename.m_loadprojfilename1;
			m_loadprojfilename.m_loadprojfilename1 = dummy;
		}
		if (m_loadprojfilename.m_loadprojfilename4 == path1) {
			QString dummy = m_loadprojfilename.m_loadprojfilename4;
			m_loadprojfilename.m_loadprojfilename4 =
					m_loadprojfilename.m_loadprojfilename1;
			m_loadprojfilename.m_loadprojfilename1 = dummy;
		}
	}

	if (m_loadprojfilename.m_loadprojfilename1 != QString("")) {
		int pos = m_loadprojfilename.m_loadprojfilename1.findRev('/', -2);
		if (pos != -1 &&
				(int)m_loadprojfilename.m_loadprojfilename1.length() > pos + 1) {
			QString name1 = m_loadprojfilename.m_loadprojfilename1.right(
					m_loadprojfilename.m_loadprojfilename1.length() - pos - 1);
			file->changeItem(m_loadprojfilename.lpf1nr, name1);
			file->setItemVisible(m_loadprojfilename.lpf1nr, true);
		}
	}
	if (m_loadprojfilename.m_loadprojfilename2 != QString("")) {
		int pos = m_loadprojfilename.m_loadprojfilename2.findRev('/', -2);
		if (pos != -1 &&
				(int)m_loadprojfilename.m_loadprojfilename2.length() > pos + 1) {
			QString name1 = m_loadprojfilename.m_loadprojfilename2.right(
					m_loadprojfilename.m_loadprojfilename2.length() - pos - 1);
			file->changeItem(m_loadprojfilename.lpf2nr, name1);
			file->setItemVisible(m_loadprojfilename.lpf2nr, true);
		}
	}
	if (m_loadprojfilename.m_loadprojfilename3 != QString("")) {
		int pos = m_loadprojfilename.m_loadprojfilename3.findRev('/', -2);
		if (pos != -1 &&
				(int)m_loadprojfilename.m_loadprojfilename3.length() > pos + 1) {
			QString name1 = m_loadprojfilename.m_loadprojfilename3.right(
					m_loadprojfilename.m_loadprojfilename3.length() - pos - 1);
			file->changeItem(m_loadprojfilename.lpf3nr, name1);
			file->setItemVisible(m_loadprojfilename.lpf3nr, true);
		}
	}
	if (m_loadprojfilename.m_loadprojfilename4 != QString("")) {
		int pos = m_loadprojfilename.m_loadprojfilename4.findRev('/', -2);
		if (pos != -1 &&
				(int)m_loadprojfilename.m_loadprojfilename4.length() > pos + 1) {
			QString name1 = m_loadprojfilename.m_loadprojfilename4.right(
					m_loadprojfilename.m_loadprojfilename4.length() - pos - 1);
			file->changeItem(m_loadprojfilename.lpf4nr, name1);
			file->setItemVisible(m_loadprojfilename.lpf4nr, true);
		}
	}
	file->setItemVisible(m_loadprojfilename.separatornr, true);
}

void MainWindow::SaveLoadProj(QString latestprojpath)
{
	if (latestprojpath == QString(""))
		return;
	FILE *fplatestproj = fopen(latestprojpath.ascii(), "w");
	if (fplatestproj != NULL) {
		if (m_loadprojfilename.m_loadprojfilename4 != QString("")) {
			fprintf(fplatestproj, "%s\n",
							m_loadprojfilename.m_loadprojfilename4.ascii());
		}
		if (m_loadprojfilename.m_loadprojfilename3 != QString("")) {
			fprintf(fplatestproj, "%s\n",
							m_loadprojfilename.m_loadprojfilename3.ascii());
		}
		if (m_loadprojfilename.m_loadprojfilename2 != QString("")) {
			fprintf(fplatestproj, "%s\n",
							m_loadprojfilename.m_loadprojfilename2.ascii());
		}
		if (m_loadprojfilename.m_loadprojfilename1 != QString("")) {
			fprintf(fplatestproj, "%s\n",
							m_loadprojfilename.m_loadprojfilename1.ascii());
		}

		fclose(fplatestproj);
	}
}

void MainWindow::pb_tab_pressed(int nr)
{
	unsigned short tabnr = nr + 1;
	for (unsigned short tabnr1 = 0; tabnr1 < pb_tab.size(); tabnr1++) {
		pb_tab[tabnr1]->setOn(tabnr == tabnr1 + 1);
	}
	unsigned short pos1 = 0;
	for (unsigned short i = 0; i < nrtabbuttons; i++)
		if (showpb_tab[i])
			pos1++;
	if ((tabnr > (nrtabbuttons + 1) / 2))
		tabnr = tabnr + (pos1 + 1) / 2 - (nrtabbuttons + 1) / 2;
	if (tabnr > pos1)
		return;
	pos1 = 0;
	unsigned short pos2 = 0;
	while (pos1 < tabnr) {
		if (showpb_tab[pos2])
			pos1++;
		pos2++;
	}
	pos2--;
	methodTab->raiseWidget(tabwidgets[pos2]);
}

void MainWindow::bmpcrosshairvisible_changed()
{
	if (cb_bmpcrosshairvisible->isChecked()) {
		if (xsliceshower != NULL) {
			bmp_show->crosshairy_changed(xsliceshower->get_slicenr());
			bmp_show->set_crosshairyvisible(true);
		}
		if (ysliceshower != NULL) {
			bmp_show->crosshairx_changed(ysliceshower->get_slicenr());
			bmp_show->set_crosshairxvisible(true);
		}
	}
	else {
		bmp_show->set_crosshairxvisible(false);
		bmp_show->set_crosshairyvisible(false);
	}

	return;
}

void MainWindow::workcrosshairvisible_changed()
{
	if (cb_workcrosshairvisible->isChecked()) {
		if (xsliceshower != NULL) {
			work_show->crosshairy_changed(xsliceshower->get_slicenr());
			work_show->set_crosshairyvisible(true);
		}
		if (ysliceshower != NULL) {
			work_show->crosshairx_changed(ysliceshower->get_slicenr());
			work_show->set_crosshairxvisible(true);
		}
	}
	else {
		work_show->set_crosshairxvisible(false);
		work_show->set_crosshairyvisible(false);
	}

	return;
}

void MainWindow::execute_inversesliceorder()
{
	common::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this, false);

	handler3D->inversesliceorder();

	//	pixelsize_changed();
	emit end_datachange(this, common::NoUndo);

	return;
}

void MainWindow::execute_smoothsteps()
{
	common::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this);

	handler3D->stepsmooth_z(5);

	emit end_datachange(this);
}

void MainWindow::execute_smoothtissues()
{
	common::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this);

	handler3D->smooth_tissues(3);

	emit end_datachange(this);
}

void MainWindow::execute_cleanup()
{
	tissues_size_t **slices = new tissues_size_t
			*[handler3D->return_endslice() - handler3D->return_startslice()];
	tissuelayers_size_t activelayer = handler3D->get_active_tissuelayer();
	for (unsigned short i = handler3D->return_startslice();
			 i < handler3D->return_endslice(); i++) {
		slices[i - handler3D->return_startslice()] =
				handler3D->return_tissues(activelayer, i);
	}
	TissueCleaner TC(
			slices, handler3D->return_endslice() - handler3D->return_startslice(),
			handler3D->return_width(), handler3D->return_height());
	if (!TC.Allocate()) {
		QMessageBox::information(
				0, "iSeg", "Not enough memory.\nThis operation cannot be performed.\n");
	}
	else {
		int rate, minsize;
		CleanerParams CP(&rate, &minsize);
		CP.exec();
		if (rate == 0 && minsize == 0)
			return;

		common::DataSelection dataSelection;
		dataSelection.allSlices = true;
		dataSelection.tissues = true;
		emit begin_datachange(dataSelection, this);

		TC.ConnectedComponents();
		TC.MakeStat();
		TC.Clean(1.0f / rate, minsize);

		emit end_datachange(this);
	}
}

void MainWindow::wheelrotated(int delta)
{
	zoom_widget->zoom_changed(work_show->return_zoom() * pow(1.2, delta / 120.0));
}

void MainWindow::mousePosZoom_changed(QPoint point)
{
	bmp_show->setMousePosZoom(point);
	work_show->setMousePosZoom(point);
	//mousePosZoom = point;
}

FILE *MainWindow::save_notes(FILE *fp, unsigned short version)
{
	if (version >= 7) {
		QString text = m_notes->toPlainText();
		int dummy = (int)text.length();
		fwrite(&(dummy), 1, sizeof(int), fp);
		fwrite(text.ascii(), 1, dummy, fp);
	}
	return fp;
}

FILE *MainWindow::load_notes(FILE *fp, unsigned short version)
{
	m_notes->clear();
	if (version >= 7) {
		int dummy = 0;
		fread(&dummy, sizeof(int), 1, fp);
		char *text = new char[dummy + 1];
		text[dummy] = '\0';
		fread(text, dummy, 1, fp);
		m_notes->setPlainText(QString(text));
		free(text);
	}
	return fp;
}

void MainWindow::disconnect_mouseclick()
{
	if (tab_old == measurement_widget) {
		QObject::disconnect(bmp_show, SIGNAL(mousepressed_sign(Point)),
												measurement_widget, SLOT(pt_clicked(Point)));
		QObject::disconnect(work_show, SIGNAL(mousepressed_sign(Point)),
												measurement_widget, SLOT(pt_clicked(Point)));
		QObject::disconnect(bmp_show, SIGNAL(mousemoved_sign(Point)),
												measurement_widget, SLOT(pt_moved(Point)));
		QObject::disconnect(work_show, SIGNAL(mousemoved_sign(Point)),
												measurement_widget, SLOT(pt_moved(Point)));
	}
	else if (tab_old == hyst_widget) {
		QObject::disconnect(bmp_show, SIGNAL(mousepressed_sign(Point)), hyst_widget,
												SLOT(pt_clicked(Point)));
		QObject::disconnect(bmp_show, SIGNAL(mousemoved_sign(Point)), hyst_widget,
												SLOT(pt_moved(Point)));
		QObject::disconnect(bmp_show, SIGNAL(mousereleased_sign(Point)),
												hyst_widget, SLOT(pt_released(Point)));
	}
	else if (tab_old == lw_widget) {
		QObject::disconnect(bmp_show, SIGNAL(mousepressed_sign(Point)), lw_widget,
												SLOT(pt_clicked(Point)));
		QObject::disconnect(bmp_show, SIGNAL(mousemoved_sign(Point)), lw_widget,
												SLOT(pt_moved(Point)));
		QObject::disconnect(bmp_show, SIGNAL(mousereleased_sign(Point)), lw_widget,
												SLOT(pt_released(Point)));
	}
	else if (tab_old == iftrg_widget) {
		QObject::disconnect(bmp_show, SIGNAL(mousepressed_sign(Point)),
												iftrg_widget, SLOT(mouse_clicked(Point)));
		QObject::disconnect(bmp_show, SIGNAL(mousereleased_sign(Point)),
												iftrg_widget, SLOT(mouse_released(Point)));
		QObject::disconnect(bmp_show, SIGNAL(mousemoved_sign(Point)), iftrg_widget,
												SLOT(mouse_moved(Point)));
	}
	else if (tab_old == FMF_widget) {
		QObject::disconnect(bmp_show, SIGNAL(mousepressed_sign(Point)), FMF_widget,
												SLOT(mouse_clicked(Point)));
		QObject::disconnect(bmp_show, SIGNAL(mousereleased_sign(Point)), FMF_widget,
												SLOT(mouse_released(Point)));
		QObject::disconnect(bmp_show, SIGNAL(mousemoved_sign(Point)), FMF_widget,
												SLOT(mouse_moved(Point)));
	}
	else if (tab_old == OutlineCorrect_widget) {
		QObject::disconnect(bmp_show, SIGNAL(mousepressed_sign(Point)),
												OutlineCorrect_widget, SLOT(mouse_clicked(Point)));
		QObject::disconnect(bmp_show, SIGNAL(mousereleased_sign(Point)),
												OutlineCorrect_widget, SLOT(mouse_released(Point)));
		QObject::disconnect(bmp_show, SIGNAL(mousemoved_sign(Point)),
												OutlineCorrect_widget, SLOT(mouse_moved(Point)));

		QObject::disconnect(work_show, SIGNAL(mousepressed_sign(Point)),
												OutlineCorrect_widget, SLOT(mouse_clicked(Point)));
		QObject::disconnect(work_show, SIGNAL(mousereleased_sign(Point)),
												OutlineCorrect_widget, SLOT(mouse_released(Point)));
		QObject::disconnect(work_show, SIGNAL(mousemoved_sign(Point)),
												OutlineCorrect_widget, SLOT(mouse_moved(Point)));
	}
	else if (tab_old == feature_widget) {
		QObject::disconnect(bmp_show, SIGNAL(mousepressed_sign(Point)),
												feature_widget, SLOT(pt_clicked(Point)));
		QObject::disconnect(bmp_show, SIGNAL(mousemoved_sign(Point)),
												feature_widget, SLOT(pt_moved(Point)));
		QObject::disconnect(bmp_show, SIGNAL(mousereleased_sign(Point)),
												feature_widget, SLOT(pt_released(Point)));
		QObject::disconnect(work_show, SIGNAL(mousepressed_sign(Point)),
												feature_widget, SLOT(pt_clicked(Point)));
		QObject::disconnect(work_show, SIGNAL(mousemoved_sign(Point)),
												feature_widget, SLOT(kpt_moved(Point)));
		QObject::disconnect(work_show, SIGNAL(mousereleased_sign(Point)),
												feature_widget, SLOT(pt_released(Point)));
	}
	else if (tab_old == pickerwidget) {
		QObject::disconnect(bmp_show, SIGNAL(mousepressed_sign(Point)),
												pickerwidget, SLOT(pt_clicked(Point)));
		QObject::disconnect(work_show, SIGNAL(mousepressed_sign(Point)),
												pickerwidget, SLOT(pt_clicked(Point)));
	}
	else if (tab_old == transfrmWidget) {
		QObject::disconnect(bmp_show, SIGNAL(mousepressed_sign(Point)),
												transfrmWidget, SLOT(pt_clicked(Point)));
		QObject::disconnect(work_show, SIGNAL(mousepressed_sign(Point)),
												transfrmWidget, SLOT(pt_clicked(Point)));
	}
}

void MainWindow::connect_mouseclick()
{
	if (tab_old == measurement_widget) {
		QObject::connect(bmp_show, SIGNAL(mousepressed_sign(Point)),
										 measurement_widget, SLOT(pt_clicked(Point)));
		QObject::connect(work_show, SIGNAL(mousepressed_sign(Point)),
										 measurement_widget, SLOT(pt_clicked(Point)));
		QObject::connect(bmp_show, SIGNAL(mousemoved_sign(Point)),
										 measurement_widget, SLOT(pt_moved(Point)));
		QObject::connect(work_show, SIGNAL(mousemoved_sign(Point)),
										 measurement_widget, SLOT(pt_moved(Point)));
	}
	else if (tab_old == hyst_widget) {
		QObject::connect(bmp_show, SIGNAL(mousepressed_sign(Point)), hyst_widget,
										 SLOT(pt_clicked(Point)));
		QObject::connect(bmp_show, SIGNAL(mousemoved_sign(Point)), hyst_widget,
										 SLOT(pt_moved(Point)));
		QObject::connect(bmp_show, SIGNAL(mousereleased_sign(Point)), hyst_widget,
										 SLOT(pt_released(Point)));
	}
	else if (tab_old == lw_widget) {
		QObject::connect(bmp_show, SIGNAL(mousepressed_sign(Point)), lw_widget,
										 SLOT(pt_clicked(Point)));
		QObject::connect(bmp_show, SIGNAL(mousemoved_sign(Point)), lw_widget,
										 SLOT(pt_moved(Point)));
		QObject::connect(bmp_show, SIGNAL(mousereleased_sign(Point)), lw_widget,
										 SLOT(pt_released(Point)));
	}
	else if (tab_old == iftrg_widget) {
		QObject::connect(bmp_show, SIGNAL(mousepressed_sign(Point)), iftrg_widget,
										 SLOT(mouse_clicked(Point)));
		QObject::connect(bmp_show, SIGNAL(mousereleased_sign(Point)), iftrg_widget,
										 SLOT(mouse_released(Point)));
		QObject::connect(bmp_show, SIGNAL(mousemoved_sign(Point)), iftrg_widget,
										 SLOT(mouse_moved(Point)));
	}
	else if (tab_old == FMF_widget) {
		QObject::connect(bmp_show, SIGNAL(mousepressed_sign(Point)), FMF_widget,
										 SLOT(mouse_clicked(Point)));
		QObject::connect(bmp_show, SIGNAL(mousereleased_sign(Point)), FMF_widget,
										 SLOT(mouse_released(Point)));
		QObject::connect(bmp_show, SIGNAL(mousemoved_sign(Point)), FMF_widget,
										 SLOT(mouse_moved(Point)));
	}
	else if (tab_old == OutlineCorrect_widget) {
		QObject::connect(bmp_show, SIGNAL(mousepressed_sign(Point)),
										 OutlineCorrect_widget, SLOT(mouse_clicked(Point)));
		QObject::connect(bmp_show, SIGNAL(mousereleased_sign(Point)),
										 OutlineCorrect_widget, SLOT(mouse_released(Point)));
		QObject::connect(bmp_show, SIGNAL(mousemoved_sign(Point)),
										 OutlineCorrect_widget, SLOT(mouse_moved(Point)));
		QObject::connect(work_show, SIGNAL(mousepressed_sign(Point)),
										 OutlineCorrect_widget, SLOT(mouse_clicked(Point)));
		QObject::connect(work_show, SIGNAL(mousereleased_sign(Point)),
										 OutlineCorrect_widget, SLOT(mouse_released(Point)));
		QObject::connect(work_show, SIGNAL(mousemoved_sign(Point)),
										 OutlineCorrect_widget, SLOT(mouse_moved(Point)));
	}
	else if (tab_old == feature_widget) {
		QObject::connect(bmp_show, SIGNAL(mousepressed_sign(Point)), feature_widget,
										 SLOT(pt_clicked(Point)));
		QObject::connect(bmp_show, SIGNAL(mousemoved_sign(Point)), feature_widget,
										 SLOT(pt_moved(Point)));
		QObject::connect(bmp_show, SIGNAL(mousereleased_sign(Point)),
										 feature_widget, SLOT(pt_released(Point)));
		QObject::connect(work_show, SIGNAL(mousepressed_sign(Point)),
										 feature_widget, SLOT(pt_clicked(Point)));
		QObject::connect(work_show, SIGNAL(mousemoved_sign(Point)), feature_widget,
										 SLOT(pt_moved(Point)));
		QObject::connect(work_show, SIGNAL(mousereleased_sign(Point)),
										 feature_widget, SLOT(pt_released(Point)));
	}
	else if (tab_old == pickerwidget) {
		QObject::connect(bmp_show, SIGNAL(mousepressed_sign(Point)), pickerwidget,
										 SLOT(pt_clicked(Point)));
		QObject::connect(work_show, SIGNAL(mousepressed_sign(Point)), pickerwidget,
										 SLOT(pt_clicked(Point)));
	}
	else if (tab_old == transfrmWidget) {
		QObject::connect(bmp_show, SIGNAL(mousepressed_sign(Point)), transfrmWidget,
										 SLOT(pt_clicked(Point)));
		QObject::connect(work_show, SIGNAL(mousepressed_sign(Point)),
										 transfrmWidget, SLOT(pt_clicked(Point)));
	}

	return;
}

void MainWindow::reconnectmouse_afterrelease(Point)
{
	QObject::disconnect(work_show, SIGNAL(mousereleased_sign(Point)), this,
											SLOT(reconnectmouse_afterrelease(Point)));
	connect_mouseclick();
}

void MainWindow::handle_begin_datachange(common::DataSelection &dataSelection,
																				 QWidget *sender, bool beginUndo)
{
	undoStarted = beginUndo || undoStarted;
	changeData = dataSelection;

	// Handle pending transforms
	if (methodTab->visibleWidget() == transfrmWidget &&
			sender != transfrmWidget) {
		cancel_transform_helper();
	}

	// Begin undo
	if (beginUndo) {
		if (changeData.allSlices) {
			// Start undo for all slices
			canUndo3D =
					handler3D->return_undo3D() && handler3D->start_undoall(changeData);
		}
		else {
			// Start undo for single slice
			handler3D->start_undo(changeData);
		}
	}
}

//void MainWindow::handle_abort_datachange(QWidget *sender, bool abortUndo)
//{
//	if (abortUndo) {
//		handler3D->abort_undo();
//		undoStarted = false;
//	}
//}

void MainWindow::end_undo_helper(common::EndUndoAction undoAction)
{
	if (undoStarted) {
		if (undoAction == common::EndUndo) {
			if (changeData.allSlices) {
				// End undo for all slices
				if (canUndo3D) {
					handler3D->end_undo();
					do_undostepdone();
				}
				else {
					do_clearundo();
				}
			}
			else {
				// End undo for single slice
				handler3D->end_undo();
				do_undostepdone();
			}
			undoStarted = false;
		}
		else if (undoAction == common::MergeUndo) {
			handler3D->merge_undo();
		}
		else if (undoAction == common::AbortUndo) {
			handler3D->abort_undo();
			undoStarted = false;
		}
	}
	else if (undoAction == common::ClearUndo) {
		do_clearundo();
	}
}

void MainWindow::handle_end_datachange(QWidget *sender,
																			 common::EndUndoAction undoAction)
{
	// End undo
	end_undo_helper(undoAction);

	// Handle 3d data change
	if (changeData.allSlices) {
		slices3d_changed(sender != bitstack_widget);
	}

	// Update ranges
	update_ranges_helper();

	// Block changed data signals for visible widget
	if (sender == methodTab->visibleWidget()) {
		QObject::disconnect(this, SIGNAL(bmp_changed()), sender,
												SLOT(bmp_changed()));
		QObject::disconnect(this, SIGNAL(work_changed()), sender,
												SLOT(work_changed()));
		QObject::disconnect(this, SIGNAL(tissues_changed()), sender,
												SLOT(tissues_changed()));
		QObject::disconnect(this, SIGNAL(marks_changed()), sender,
												SLOT(marks_changed()));
	}

	// Signal changed data
	if (changeData.bmp) {
		emit bmp_changed();
	}
	if (changeData.work) {
		emit work_changed();
	}
	if (changeData.tissues) {
		emit tissues_changed();
	}
	if (changeData.marks) {
		emit marks_changed();
	}

	// Signal changed data
	if (changeData.bmp && changeData.work && changeData.allSlices) {
		// Do not reinitialize m_MultiDataset_widget after swapping
		if (m_NewDataAfterSwap) {
			m_NewDataAfterSwap = false;
		}
		else {
			m_MultiDataset_widget->NewLoaded();
		}
	}

	if (sender == methodTab->visibleWidget()) {
		QObject::connect(this, SIGNAL(bmp_changed()), sender, SLOT(bmp_changed()));
		QObject::connect(this, SIGNAL(work_changed()), sender,
										 SLOT(work_changed()));
		QObject::connect(this, SIGNAL(tissues_changed()), sender,
										 SLOT(tissues_changed()));
		QObject::connect(this, SIGNAL(marks_changed()), sender,
										 SLOT(marks_changed()));
	}

	//// Data loaded
	//if (changeData.allSlices && changeData.bmp && changeData.work)
	//{
	//	emit main_dataset_loaded();
	//}
}

void MainWindow::handle_end_datachange(QRect rect, QWidget *sender,
																			 common::EndUndoAction undoAction)
{
	// End undo
	end_undo_helper(undoAction);

	// Handle 3d data change
	if (changeData.allSlices) {
		slices3d_changed(sender != bitstack_widget);
	}

	// Update ranges
	update_ranges_helper();

#if 0 // TODO: Introduce signals bmp_changed(QRect rect) etc
	if (changeData.bmp)
	{
		update_bmp(rect);
	}
	if (changeData.work)
	{
		update_work(rect);
	}
	//if (changeData.tissues) {
	//	update_tissues(rect);
	//}
	//if (changeData.marks) {
	//	upate_marks(rect);
	//}
#else
	// Block changed data signals for visible widget
	if (sender == methodTab->visibleWidget()) {
		QObject::disconnect(this, SIGNAL(bmp_changed()), sender,
												SLOT(bmp_changed()));
		QObject::disconnect(this, SIGNAL(work_changed()), sender,
												SLOT(work_changed()));
		QObject::disconnect(this, SIGNAL(tissues_changed()), sender,
												SLOT(tissues_changed()));
		QObject::disconnect(this, SIGNAL(marks_changed()), sender,
												SLOT(marks_changed()));
	}

	// Signal changed data
	if (changeData.bmp) {
		emit bmp_changed();
	}
	if (changeData.work) {
		emit work_changed();
	}
	if (changeData.tissues) {
		emit tissues_changed();
	}
	if (changeData.marks) {
		emit marks_changed();
	}

	if (sender == methodTab->visibleWidget()) {
		QObject::connect(this, SIGNAL(bmp_changed()), sender, SLOT(bmp_changed()));
		QObject::connect(this, SIGNAL(work_changed()), sender,
										 SLOT(work_changed()));
		QObject::connect(this, SIGNAL(tissues_changed()), sender,
										 SLOT(tissues_changed()));
		QObject::connect(this, SIGNAL(marks_changed()), sender,
										 SLOT(marks_changed()));
	}
#endif
}

void MainWindow::DatasetChanged()
{
	emit bmp_changed();
	emit work_changed();

	reset_brightnesscontrast();
}

void MainWindow::update_ranges_helper()
{
	if (changeData.bmp) {
		if (changeData.allSlices) {
			bmp_show->update_range();
		}
		else {
			bmp_show->update_range(changeData.sliceNr);
		}
		update_brightnesscontrast(true, false);
	}
	if (changeData.work) {
		if (changeData.allSlices) {
			work_show->update_range();
		}
		else {
			work_show->update_range(changeData.sliceNr);
		}
		update_brightnesscontrast(false, false);
	}
}

void MainWindow::cancel_transform_helper()
{
	QObject::disconnect(
			transfrmWidget,
			SIGNAL(begin_datachange(common::DataSelection &, QWidget *, bool)), this,
			SLOT(handle_begin_datachange(common::DataSelection &, QWidget *, bool)));
	QObject::disconnect(
			transfrmWidget, SIGNAL(end_datachange(QWidget *, common::EndUndoAction)),
			this, SLOT(handle_end_datachange(QWidget *, common::EndUndoAction)));
	transfrmWidget->CancelPushButtonClicked();
	QObject::connect(
			transfrmWidget,
			SIGNAL(begin_datachange(common::DataSelection &, QWidget *, bool)), this,
			SLOT(handle_begin_datachange(common::DataSelection &, QWidget *, bool)));
	QObject::connect(
			transfrmWidget, SIGNAL(end_datachange(QWidget *, common::EndUndoAction)),
			this, SLOT(handle_end_datachange(QWidget *, common::EndUndoAction)));

	// Signal changed data
	bool bmp, work, tissues;
	transfrmWidget->GetDataSelection(bmp, work, tissues);
	if (bmp) {
		emit bmp_changed();
	}
	if (work) {
		emit work_changed();
	}
	if (tissues) {
		emit tissues_changed();
	}
}

void MainWindow::handle_begin_dataexport(common::DataSelection &dataSelection,
																				 QWidget *sender)
{
	// Handle pending transforms
	if (methodTab->visibleWidget() == transfrmWidget &&
			(dataSelection.bmp || dataSelection.work || dataSelection.tissues)) {
		cancel_transform_helper();
	}

	// Handle pending tissue hierarchy modifications
	if (dataSelection.tissueHierarchy) {
		tissueHierarchyWidget->handle_changed_hierarchy();
	}
}

void MainWindow::handle_end_dataexport(QWidget *sender) {}

void MainWindow::startwidget()
{
#ifdef ISEG_GPU_MC
	delete surfaceview;
	surfaceview = new Window();
	for (int i = surfaceview->tissuebox->count(); i >= 0; --i)
		surfaceview->tissuebox->removeItem(i);

	QList<QTreeWidgetItem *> list;
	list = tissueTreeWidget->selectedItems();
	for (auto a = list.begin(); a != list.end(); ++a) {
		QTreeWidgetItem *item = *a;
		tissues_size_t nr = tissueTreeWidget->get_type(item);
		QString name = tissueTreeWidget->get_name(item);
		surfaceview->tissuebox->addItem(name, Qt::DisplayRole);
	}
	int dimension[3];
	dimension[0] = (int)handler3D->return_width();
	dimension[1] = (int)handler3D->return_height();
	dimension[2] =
			(int)(handler3D->return_endslice() - handler3D->return_startslice());
	surfaceview->setDim(dimension);
	Pair ps = handler3D->get_pixelsize();
	float spacing[3] = {ps.high, ps.low, handler3D->get_slicethickness()};
	surfaceview->setSpace(spacing);
	std::vector<unsigned char *> voxel;
	for (int i = 0; i < list.size(); ++i) {
		voxel.push_back(
				new unsigned char[dimension[0] * dimension[1] * dimension[2]]);
		for (int s = 0; s < dimension[0] * dimension[1] * dimension[2]; s++) {
			voxel[i][s] = 0;
		}
	}
	std::vector<int> isovalues;
	std::vector<unsigned char> colours;
	int tmp = 0;
	for (auto a = list.begin(); a != list.end(); ++a) {
		QTreeWidgetItem *item = *a;
		tissues_size_t currTissueType = tissueTreeWidget->get_type(item);
		isovalues.push_back(currTissueType - 1);
		unsigned char r, g, b;
		TissueInfos::GetTissueColorRGB(currTissueType, r, g, b);
		colours.push_back(r);
		colours.push_back(g);
		colours.push_back(b);
		handler3D->selectedtissue2mc(currTissueType, &voxel[tmp]);
		tmp = +1;
	}

	surfaceview->setisovalues(isovalues);
	surfaceview->setColours(colours);
	surfaceview->setVoxel(voxel);
	surfaceview->resize(surfaceview->sizeHint());
	int desktopArea =
			QApplication::desktop()->width() * QApplication::desktop()->height();
	int widgetArea = surfaceview->width() * surfaceview->height();
	if (((float)widgetArea / (float)desktopArea) < 0.75f)
		surfaceview->show();
	else
		surfaceview->showMaximized();
#else
	assert(false && "Calling surface view, but it has been disabled.");
#endif
}