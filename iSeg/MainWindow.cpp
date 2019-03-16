/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 *
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 *
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "Precompiled.h"

#include "../config.h"

#include "ActiveslicesConfigWidget.h"
#include "AtlasWidget.h"
#include "EdgeWidget.h"
#include "FastmarchingFuzzyWidget.h"
#include "FeatureWidget.h"
#include "HystereticGrowingWidget.h"
#include "ImageForestingTransformRegionGrowingWidget.h"
#include "ImageInformationDialogs.h"
#include "ImageViewerWidget.h"
#include "InterpolationWidget.h"
#include "LivewireWidget.h"
#include "LoaderWidgets.h"
#include "MainWindow.h"
#include "MeasurementWidget.h"
#include "MorphologyWidget.h"
#include "MultiDatasetWidget.h"
#include "OutlineCorrectionWidget.h"
#include "PickerWidget.h"
#include "SaveOutlinesWidget.h"
#include "Settings.h"
#include "SliceViewerWidget.h"
#include "SmoothingWidget.h"
#include "StdStringToQString.h"
#include "SurfaceViewerWidget.h"
#include "ThresholdWidget.h"
#include "TissueCleaner.h"
#include "TissueInfos.h"
#include "TissueTreeWidget.h"
#include "TransformWidget.h"
#include "UndoConfigurationDialog.h"
#include "VesselWidget.h"
#include "VolumeViewerWidget.h"
#include "WatershedWidget.h"
#include "XdmfImageWriter.h"

#include "RadiotherapyStructureSetImporter.h"

#include "Interface/Plugin.h"
#include "Interface/ProgressDialog.h"

#include "Data/Transform.h"

#include "Core/HDF5Blosc.h"
#include "Core/LoadPlugin.h"
#include "Core/ProjectVersion.h"
#include "Core/VotingReplaceLabel.h"

#include <boost/filesystem.hpp>

#include <QDesktopWidget>
#include <QFileDialog>
#include <QSignalMapper.h>
#include <QStackedWidget>
#include <q3accel.h>
#include <q3popupmenu.h>
#include <qapplication.h>
#include <qdockwidget.h>
#include <qmenubar.h>
#include <qprogressdialog.h>
#include <qsettings.h>
#include <qtextedit.h>
#include <qtooltip.h>

#define str_macro(s) #s
#define xstr(s) str_macro(s)

#define UNREFERENCED_PARAMETER(P) (P)

using namespace iseg;

namespace {
int openS4LLinkPos = -1;
int importSurfacePos = -1;
int importRTstructPos = -1;
} // namespace

int bmpimgnr(QString* s)
{
	int result = 0;
	uint pos;
	if (s->length() > 4 && s->endsWith(QString(".bmp")))
	{
		pos = s->length() - 5;
	}
	else
	{
		pos = s->length() - 1;
	}
	int base = 1;
	QChar cdigit;
	while (pos > 0 && (cdigit = s->at(pos)).isDigit())
	{
		result += cdigit.digitValue() * base;
		base *= 10;
		pos--;
	}
	if (pos == 0 && (cdigit = s->at(0)).isDigit())
		result += cdigit.digitValue() * base;

	return result;
}

int pngimgnr(QString* s)
{
	int result = 0;
	uint pos;
	if (s->length() > 4 && s->endsWith(QString(".png")))
	{
		pos = s->length() - 5;
	}
	else
	{
		pos = s->length() - 1;
	}
	int base = 1;
	QChar cdigit;
	while (pos > 0 && (cdigit = s->at(pos)).isDigit())
	{
		result += cdigit.digitValue() * base;
		base *= 10;
		pos--;
	}
	if (pos == 0 && (cdigit = s->at(0)).isDigit())
		result += cdigit.digitValue() * base;

	return result;
}

int jpgimgnr(QString* s)
{
	int result = 0;
	uint pos;
	if (s->length() > 4 && s->endsWith(QString(".jpg")))
	{
		pos = s->length() - 5;
	}
	else
	{
		pos = s->length() - 1;
	}
	int base = 1;
	QChar cdigit;
	while (pos > 0 && (cdigit = s->at(pos)).isDigit())
	{
		result += cdigit.digitValue() * base;
		base *= 10;
		pos--;
	}
	if (pos == 0 && (cdigit = s->at(0)).isDigit())
		result += cdigit.digitValue() * base;

	return result;
}

QString TruncateFileName(QString str)
{
	if (str != QString(""))
	{
		int pos = str.findRev('/', -2);
		if (pos != -1 && (int)str.length() > pos + 1)
		{
			QString name1 = str.right(str.length() - pos - 1);
			return name1;
		}
	}
	return str;
}

bool read_grouptissues(const char* filename, std::vector<tissues_size_t>& olds,
		std::vector<tissues_size_t>& news)
{
	FILE* fp;
	if ((fp = fopen(filename, "r")) == nullptr)
	{
		return false;
	}
	else
	{
		char name1[1000];
		char name2[1000];
		while (fscanf(fp, "%s %s\n", name1, name2) == 2)
		{
			olds.push_back(TissueInfos::GetTissueType(std::string(name1)));
			news.push_back(TissueInfos::GetTissueType(std::string(name2)));
		}
		fclose(fp);
		return true;
	}
}

bool read_grouptissuescapped(const char* filename, std::vector<tissues_size_t>& olds,
		std::vector<tissues_size_t>& news,
		bool fail_on_unknown_tissue)
{
	FILE* fp;
	if ((fp = fopen(filename, "r")) == nullptr)
	{
		return false;
	}
	else
	{
		char name1[1000];
		char name2[1000];
		tissues_size_t type1, type2;
		tissues_size_t tissueCount = TissueInfos::GetTissueCount();

		// Try scan first entry to decide between tissue name and index input
		bool readIndices = false;
		if (fscanf(fp, "%s %s\n", name1, name2) == 2)
		{
			type1 = (tissues_size_t)QString(name1).toUInt(&readIndices);
		}

		bool ok = true;
		if (readIndices)
		{
			// Read input as tissue indices
			type2 = (tissues_size_t)QString(name2).toUInt(&readIndices);
			if (type1 > 0 && type1 <= tissueCount && type2 > 0 &&
					type2 <= tissueCount)
			{
				olds.push_back(type1);
				news.push_back(type2);
			}
			else
				ok = false;

			int tmp1, tmp2;
			while (fscanf(fp, "%i %i\n", &tmp1, &tmp2) == 2)
			{
				type1 = (tissues_size_t)tmp1;
				type2 = (tissues_size_t)tmp2;
				if (type1 > 0 && type1 <= tissueCount && type2 > 0 && type2 <= tissueCount)
				{
					olds.push_back(type1);
					news.push_back(type2);
				}
				else
					ok = false;
			}
		}
		else
		{
			// Read input as tissue names
			type1 = TissueInfos::GetTissueType(std::string(name1));
			type2 = TissueInfos::GetTissueType(std::string(name2));
			if (type1 > 0 && type1 <= tissueCount && type2 > 0 && type2 <= tissueCount)
			{
				olds.push_back(type1);
				news.push_back(type2);
			}
			else
			{
				ok = false;
				if (type1 == 0 || type1 > tissueCount)
					ISEG_WARNING("old: " << name1 << " not in tissue list");
				if (type2 == 0 || type2 > tissueCount)
					ISEG_WARNING("new: " << name2 << " not in tissue list");
			}
			memset(name1, 0, 1000);
			memset(name2, 0, 1000);

			while (fscanf(fp, "%s %s\n", name1, name2) == 2)
			{
				type1 = TissueInfos::GetTissueType(std::string(name1));
				type2 = TissueInfos::GetTissueType(std::string(name2));
				if (type1 > 0 && type1 <= tissueCount && type2 > 0 && type2 <= tissueCount)
				{
					olds.push_back(type1);
					news.push_back(type2);
				}
				else
				{
					ok = false;
					if (type1 == 0 || type1 > tissueCount)
						ISEG_WARNING("old: " << name1 << " not in tissue list");
					if (type2 == 0 || type2 > tissueCount)
						ISEG_WARNING("new: " << name2 << " not in tissue list");
				}

				memset(name1, 0, 1000);
				memset(name2, 0, 1000);
			}
		}

		fclose(fp);

		return ok || !fail_on_unknown_tissue;
	}
}

bool read_tissues(const char* filename, std::vector<tissues_size_t>& types)
{
	FILE* fp;
	if ((fp = fopen(filename, "r")) == nullptr)
	{
		return false;
	}

	tissues_size_t const tissueCount = TissueInfos::GetTissueCount();
	char name[1000];
	bool ok = true;

	while (fscanf(fp, "%s\n", name) == 1)
	{
		tissues_size_t type = TissueInfos::GetTissueType(std::string(name));
		if (type > 0 && type <= tissueCount)
		{
			types.push_back(type);
		}
		else
		{
			ok = false;
			ISEG_WARNING(name << " not in tissue list");
		}
	}

	fclose(fp);

	return ok;
}

void style_dockwidget(QDockWidget* dockwidg)
{
	//dockwidg->setStyleSheet("QDockWidget { color: black; } QDockWidget::title { background: gray; padding-left: 5px; padding-top: 3px; color: darkgray} QDockWidget::close-button, QDockWidget::float-button { background: gray; }");
}

bool MenuWTT::event(QEvent* e)
{
	const QHelpEvent* helpEvent = static_cast<QHelpEvent*>(e);
	if (helpEvent->type() == QEvent::ToolTip)
	{
		// call QToolTip::showText on that QAction's tooltip.
		QPoint gpos = helpEvent->globalPos();
		QPoint pos = helpEvent->pos();
		if (pos.x() > 0 && pos.y() > 0 && pos.x() < 400 && pos.y() < 600)
		{
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

MainWindow::MainWindow(SlicesHandler* hand3D, const QString& locationstring,
		const QDir& picpath, const QDir& tmppath, bool editingmode,
		QWidget* parent, const char* name,
		Qt::WindowFlags wFlags, const std::vector<std::string>& plugin_search_dirs)
		: QMainWindow(parent, name, wFlags)
{
	setObjectName("MainWindow");

	undoStarted = false;
	setContentsMargins(9, 4, 9, 4);
	m_picpath = picpath;
	m_tmppath = tmppath;
	m_editingmode = editingmode;
	tab_old = nullptr;

	setCaption(QString(" iSeg ") + QString(xstr(ISEG_VERSION)) +
						 QString(" - No Filename"));
	QIcon isegicon(m_picpath.absFilePath(QString("isegicon.png")).ascii());
	setWindowIcon(isegicon);
	m_locationstring = locationstring;
	m_saveprojfilename = QString("");
	m_S4Lcommunicationfilename = QString("");

	handler3D = hand3D;

	handler3D->on_tissue_selection_changed.connect([this](const std::vector<tissues_size_t>& sel) {
		std::set<tissues_size_t> selection(sel.begin(), sel.end());
		QTreeWidgetItem* first = nullptr;
		bool clear_filter = false;
		for (auto item : tissueTreeWidget->get_all_items())
		{
			bool select = selection.count(tissueTreeWidget->get_type(item)) != 0;
			item->setSelected(select);
			if (select && !first)
			{
				first = item;
			}

			if (select && item->isHidden())
			{
				clear_filter = true;
			}
		}

		if (clear_filter)
		{
			tissueFilter->setText(QString(""));
		}
		tissueTreeWidget->scrollToItem(first);
	});

	handler3D->on_active_slice_changed.connect([this](unsigned short slice) {
		slice_changed();
	});

	if (!handler3D->isloaded())
	{
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

	bmp_show = new ImageViewerWidget(this, "new window", Qt::WDestructiveClose | Qt::WResizeNoErase);
	lb_source = new QLabel("Source", this);
	lb_target = new QLabel("Target", this);
	bmp_scroller = new Q3ScrollView(this);
	sl_contrastbmp = new QSlider(Qt::Horizontal, this);
	sl_contrastbmp->setRange(0, 100);
	sl_brightnessbmp = new QSlider(Qt::Horizontal, this);
	sl_brightnessbmp->setRange(0, 100);
	lb_contrastbmp = new QLabel("C:", this);
	lb_contrastbmp->setPixmap(QIcon(m_picpath.absFilePath(QString("icon-contrast.png")).ascii()).pixmap());
	le_contrastbmp_val = new QLineEdit(this);
	le_contrastbmp_val->setAlignment(Qt::AlignRight);
	le_contrastbmp_val->setText(QString("%1").arg(9999.99, 6, 'f', 2));
	QString text = le_contrastbmp_val->text();
	QFontMetrics fm = le_contrastbmp_val->fontMetrics();
	QRect rect = fm.boundingRect(text);
	le_contrastbmp_val->setFixedSize(rect.width() + 4, rect.height() + 4);
	lb_contrastbmp_val = new QLabel("x", this);
	lb_brightnessbmp = new QLabel("B:", this);
	lb_brightnessbmp->setPixmap(QIcon(m_picpath.absFilePath(QString("icon-brightness.png")).ascii()).pixmap());
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
	bmp_show->update();

	toworkBtn = new QPushButton(QIcon(m_picpath.absFilePath(QString("next.png")).ascii()), "", this);
	toworkBtn->setFixedWidth(50);
	tobmpBtn = new QPushButton(QIcon(m_picpath.absFilePath(QString("previous.png")).ascii()), "", this);
	tobmpBtn->setFixedWidth(50);
	swapBtn = new QPushButton(QIcon(m_picpath.absFilePath(QString("swap.png")).ascii()), "", this);
	swapBtn->setFixedWidth(50);
	swapAllBtn = new QPushButton(QIcon(m_picpath.absFilePath(QString("swap.png")).ascii()), "3D", this);
	swapAllBtn->setFixedWidth(50);

	work_show = new ImageViewerWidget(this, "new window", Qt::WDestructiveClose | Qt::WResizeNoErase);
	sl_contrastwork = new QSlider(Qt::Horizontal, this);
	sl_contrastwork->setRange(0, 100);
	sl_brightnesswork = new QSlider(Qt::Horizontal, this);
	sl_brightnesswork->setRange(0, 100);
	lb_contrastwork = new QLabel(this);
	lb_contrastwork->setPixmap(QIcon(m_picpath.absFilePath(QString("icon-contrast.png")).ascii()).pixmap());
	le_contrastwork_val = new QLineEdit(this);
	le_contrastwork_val->setAlignment(Qt::AlignRight);
	le_contrastwork_val->setText(QString("%1").arg(9999.99, 6, 'f', 2));
	text = le_contrastwork_val->text();
	fm = le_contrastwork_val->fontMetrics();
	rect = fm.boundingRect(text);
	le_contrastwork_val->setFixedSize(rect.width() + 4, rect.height() + 4);
	lb_contrastwork_val = new QLabel("x", this);
	lb_brightnesswork = new QLabel(this);
	lb_brightnesswork->setPixmap(QIcon(m_picpath.absFilePath(QString("icon-brightness.png")).ascii()).pixmap());
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

	work_show->init(handler3D, FALSE);
	work_show->set_tissuevisible(false); //toggle_tissuevisible();
	work_show->setIsBmp(false);

	reset_brightnesscontrast();

	zoom_widget = new ZoomWidget(1.0, m_picpath, this);

	tissueTreeWidget = new TissueTreeWidget(handler3D->get_tissue_hierachy(), m_picpath, this);
	tissueTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
	tissueFilter = new QLineEdit(this);
	tissueFilter->setMargin(1);
	tissueHierarchyWidget = new TissueHierarchyWidget(tissueTreeWidget, this);
	tissueTreeWidget->update_tree_widget(); // Reload hierarchy
	cb_tissuelock = new QCheckBox(this);
	cb_tissuelock->setPixmap(QIcon(m_picpath.absFilePath(QString("lock.png")).ascii()).pixmap());
	cb_tissuelock->setChecked(false);
	lockTissues = new QPushButton("All", this);
	lockTissues->setToggleButton(true);
	lockTissues->setFixedWidth(50);
	addTissue = new QPushButton("New Tissue...", this);
	addFolder = new QPushButton("New Folder...", this);

	modifyTissueFolder = new QPushButton("Mod. Tissue/Folder", this);
	modifyTissueFolder->setToolTip(
			Format("Edit the selected tissue or folder properties."));

	removeTissueFolder = new QPushButton("Del. Tissue/Folder", this);
	removeTissueFolder->setToolTip(
			Format("Remove the selected tissues or folders."));
	removeTissueFolderAll = new QPushButton("All", this);
	removeTissueFolderAll->setFixedWidth(30);

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
	clearTissue = new QPushButton("Clear Tissue", this);
	clearTissue->setToolTip(
			Format("Clears the currently selected tissue (use '3D' option to clear "
						 "whole tissue or just the current slice)."));

	clearTissues = new QPushButton("All", this);
	clearTissues->setFixedWidth(30);
	clearTissues->setToolTip(
			Format("Clears all tissues (use '3D' option to clear the entire "
						 "segmentation or just the current slice)."));

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

	pb_add = new QPushButton("+", this);
	pb_add->setToggleButton(true);
	pb_add->setToolTip(Format(
			"Adds next selected/picked Target image region to current tissue."));
	pb_sub = new QPushButton("-", this);
	pb_sub->setToggleButton(true);
	pb_sub->setToolTip(Format("Removes next selected/picked Target image "
														"region from current tissue."));
	pb_addhold = new QPushButton("++", this);
	pb_addhold->setToggleButton(true);
	pb_addhold->setToolTip(
			Format("Adds selected/picked Target image regions to current tissue."));

	pb_subhold = new QPushButton("--", this);
	pb_subhold->setToggleButton(true);
	pb_subhold->setToolTip(Format(
			"Removes selected/picked Target image regions from current tissue."));

	pb_add->setFixedWidth(50);
	pb_sub->setFixedWidth(50);
	pb_addhold->setFixedWidth(50);
	pb_subhold->setFixedWidth(50);

	unsigned short slicenr = handler3D->active_slice() + 1;
	pb_first = new QPushButton("|<<", this);
	scb_slicenr = new QScrollBar(1, (int)handler3D->num_slices(), 1, 5, 1, Qt::Horizontal, this);
	scb_slicenr->setMinimumWidth(350);
	scb_slicenr->setValue(int(slicenr));
	pb_last = new QPushButton(">>|", this);
	sb_slicenr = new QSpinBox(1, (int)handler3D->num_slices(), 1, this);
	sb_slicenr->setValue(slicenr);
	lb_slicenr = new QLabel(QString(" of ") + QString::number((int)handler3D->num_slices()), this);

	lb_stride = new QLabel(QString("Stride: "));
	sb_stride = new QSpinBox(1, 1000, 1, this);
	sb_stride->setValue(1);

	lb_inactivewarning = new QLabel("  3D Inactive Slice!  ", this);
	lb_inactivewarning->setStyleSheet("QLabel  { color: red; }");

	threshold_widget = new ThresholdWidget(handler3D, nullptr, "new window", Qt::WDestructiveClose | Qt::WResizeNoErase);
	tabwidgets.push_back(threshold_widget);
	hyst_widget = new HystereticGrowingWidget(handler3D, nullptr, "new window", Qt::WDestructiveClose | Qt::WResizeNoErase);
	tabwidgets.push_back(hyst_widget);
	livewire_widget = new LivewireWidget(handler3D, nullptr, "new window", Qt::WDestructiveClose | Qt::WResizeNoErase);
	tabwidgets.push_back(livewire_widget);
	iftrg_widget = new ImageForestingTransformRegionGrowingWidget(handler3D, nullptr, "new window", Qt::WDestructiveClose | Qt::WResizeNoErase);
	tabwidgets.push_back(iftrg_widget);
	fastmarching_widget = new FastmarchingFuzzyWidget(handler3D, nullptr, "new window", Qt::WDestructiveClose | Qt::WResizeNoErase);
	tabwidgets.push_back(fastmarching_widget);
	watershed_widget = new WatershedWidget(handler3D, nullptr, "new window", Qt::WDestructiveClose | Qt::WResizeNoErase);
	tabwidgets.push_back(watershed_widget);
	olc_widget = new OutlineCorrectionWidget(handler3D, nullptr, "new window", Qt::WDestructiveClose | Qt::WResizeNoErase);
	tabwidgets.push_back(olc_widget);
	interpolation_widget = new InterpolationWidget(handler3D, nullptr, "new window", Qt::WDestructiveClose | Qt::WResizeNoErase);
	tabwidgets.push_back(interpolation_widget);
	smoothing_widget = new SmoothingWidget(handler3D, nullptr, "new window", Qt::WDestructiveClose | Qt::WResizeNoErase);
	tabwidgets.push_back(smoothing_widget);
	morphology_widget = new MorphologyWidget(handler3D, nullptr, "new window", Qt::WDestructiveClose | Qt::WResizeNoErase);
	tabwidgets.push_back(morphology_widget);
	edge_widget = new EdgeWidget(handler3D, nullptr, "new window", Qt::WDestructiveClose | Qt::WResizeNoErase);
	tabwidgets.push_back(edge_widget);
	feature_widget = new FeatureWidget(handler3D, nullptr, "new window", Qt::WDestructiveClose | Qt::WResizeNoErase);
	tabwidgets.push_back(feature_widget);
	measurement_widget = new MeasurementWidget(handler3D, nullptr, "new window", Qt::WDestructiveClose | Qt::WResizeNoErase);
	tabwidgets.push_back(measurement_widget);
	vesselextr_widget = new VesselWidget(handler3D, nullptr, "new window", Qt::WDestructiveClose | Qt::WResizeNoErase);
#ifdef PLUGIN_VESSEL_WIDGET
	tabwidgets.push_back(vesselextr_widget);
#endif
	picker_widget = new PickerWidget(handler3D, nullptr, "new window", Qt::WDestructiveClose | Qt::WResizeNoErase);
	tabwidgets.push_back(picker_widget);
	transform_widget = new TransformWidget(handler3D, nullptr, "new window", Qt::WDestructiveClose | Qt::WResizeNoErase);
	tabwidgets.push_back(transform_widget);

	for (auto dir : plugin_search_dirs)
	{
		bool ok = iseg::plugin::LoadPlugins(dir);
		if (!ok)
		{
			ISEG_WARNING("Could not load plugins from " << dir);
		}
	}

	auto addons = iseg::plugin::PluginRegistry::registered_plugins();
	for (auto a : addons)
	{
		a->install_slice_handler(handler3D);
		tabwidgets.push_back(a->create_widget(nullptr, "new window", Qt::WDestructiveClose | Qt::WResizeNoErase));
	}

	auto nrtabbuttons = (unsigned short)tabwidgets.size();
	pb_tab.resize(nrtabbuttons);
	showpb_tab.resize(nrtabbuttons);
	showtab_action.resize(nrtabbuttons);

	for (unsigned short i = 0; i < nrtabbuttons; i++)
	{
		showpb_tab[i] = true;

		pb_tab[i] = new QPushButton(this);
		pb_tab[i]->setToggleButton(true);
		pb_tab[i]->setStyleSheet("text-align: left");
	}

	methodTab = new QStackedWidget(this);
	methodTab->setFrameStyle(Q3Frame::Box | Q3Frame::Sunken);
	methodTab->setLineWidth(2);

	for (size_t i = 0; i < tabwidgets.size(); i++)
	{
		methodTab->addWidget(tabwidgets[i]);
	}

	{
		unsigned short posi = 0;
		while (posi < nrtabbuttons && !showpb_tab[posi])
			posi++;
		if (posi < nrtabbuttons)
			methodTab->setCurrentWidget(tabwidgets[posi]);
	}

	tab_changed(methodTab->currentIndex());

	updateTabvisibility();

	int height_max = 0;
	QSize qs;
	for (size_t i = 0; i < tabwidgets.size(); i++)
	{
		qs = tabwidgets[i]->sizeHint();
		height_max = std::max(height_max, qs.height());
	}
	height_max += 65;

	scale_dialog = new ScaleWork(handler3D, m_picpath, this, "new window", Qt::WDestructiveClose | Qt::WResizeNoErase);
	imagemath_dialog = new ImageMath(handler3D, this, "new window", Qt::WDestructiveClose | Qt::WResizeNoErase);
	imageoverlay_dialog = new ImageOverlay(handler3D, this, "new window", Qt::WDestructiveClose | Qt::WResizeNoErase);

	bitstack_widget = new bits_stack(handler3D, this, "new window", Qt::WDestructiveClose | Qt::WResizeNoErase);
	overlay_widget = new extoverlay_widget(handler3D, this, "new window", Qt::WDestructiveClose | Qt::WResizeNoErase);
	multidataset_widget = new MultiDatasetWidget(handler3D, this, "multi dataset window", Qt::WDestructiveClose | Qt::WResizeNoErase);

	int height_max1 = height_max;

	m_notes = new QTextEdit(this);
	m_notes->clear();

	xsliceshower = ysliceshower = nullptr;
	surface_viewer = nullptr;
	VV3D = nullptr;
	VV3Dbmp = nullptr;

	if (handler3D->start_slice() >= slicenr || handler3D->end_slice() + 1 <= slicenr)
	{
		lb_inactivewarning->setText(QString("   3D Inactive Slice!   "));
	}
	else
	{
		lb_inactivewarning->setText(QString(" "));
	}

	QWidget* hbox2w = new QWidget;
	setCentralWidget(hbox2w);

	QWidget* vbox2w = new QWidget;
	QWidget* rightvboxw = new QWidget;
	QWidget* hbox1w = new QWidget;
	QWidget* hboxslicew = new QWidget;
	QWidget* hboxslicenrw = new QWidget;
	QWidget* hboxstridew = new QWidget;
	vboxbmpw = new QWidget;
	vboxworkw = new QWidget;
	QWidget* vbox1w = new QWidget;
	QWidget* hboxbmpw = new QWidget;
	QWidget* hboxworkw = new QWidget;
	QWidget* hboxbmp1w = new QWidget;
	QWidget* hboxwork1w = new QWidget;
	QWidget* vboxtissuew = new QWidget;
	QWidget* hboxlockw = new QWidget;
	QWidget* hboxtissueadderw = new QWidget;
	QWidget* vboxtissueadder1w = new QWidget;
	QWidget* vboxtissueadder2w = new QWidget;
	QWidget* hboxtabsw = new QWidget;
	QWidget* vboxtabs1w = new QWidget;
	QWidget* vboxtabs2w = new QWidget;

	QHBoxLayout* hboxbmp1 = new QHBoxLayout;
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

	QHBoxLayout* hboxbmp = new QHBoxLayout;
	hboxbmp->setSpacing(0);
	hboxbmp->setMargin(0);
	hboxbmp->addWidget(cb_bmptissuevisible);
	hboxbmp->addWidget(cb_bmpcrosshairvisible);
	hboxbmp->addWidget(cb_bmpoutlinevisible);
	hboxbmpw->setLayout(hboxbmp);

	QVBoxLayout* vboxbmp = new QVBoxLayout;
	vboxbmp->setSpacing(0);
	vboxbmp->setMargin(0);
	vboxbmp->addWidget(lb_source);
	vboxbmp->addWidget(hboxbmp1w);
	vboxbmp->addWidget(bmp_scroller);
	vboxbmp->addWidget(hboxbmpw);
	vboxbmpw->setLayout(vboxbmp);

	QVBoxLayout* vbox1 = new QVBoxLayout;
	vbox1->setSpacing(0);
	vbox1->setMargin(0);
	vbox1->addWidget(toworkBtn);
	vbox1->addWidget(tobmpBtn);
	vbox1->addWidget(swapBtn);
	vbox1->addWidget(swapAllBtn);
	vbox1w->setLayout(vbox1);

	QHBoxLayout* hboxwork1 = new QHBoxLayout;
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

	QHBoxLayout* hboxwork = new QHBoxLayout;
	hboxwork->setSpacing(0);
	hboxwork->setMargin(0);
	hboxwork->addWidget(cb_worktissuevisible);
	hboxwork->addWidget(cb_workcrosshairvisible);
	hboxwork->addWidget(cb_workpicturevisible);
	hboxworkw->setLayout(hboxwork);

	QVBoxLayout* vboxwork = new QVBoxLayout;
	vboxwork->setSpacing(0);
	vboxwork->setMargin(0);
	vboxwork->addWidget(lb_target);
	vboxwork->addWidget(hboxwork1w);
	vboxwork->addWidget(work_scroller);
	vboxwork->addWidget(hboxworkw);
	vboxworkw->setLayout(vboxwork);

	QHBoxLayout* hbox1 = new QHBoxLayout;
	hbox1->setSpacing(0);
	hbox1->setMargin(0);
	hbox1->addWidget(vboxbmpw);
	hbox1->addWidget(vbox1w);
	hbox1->addWidget(vboxworkw);
	hbox1w->setLayout(hbox1);

	QHBoxLayout* hboxslicenr = new QHBoxLayout;
	hboxslicenr->setSpacing(0);
	hboxslicenr->setMargin(0);
	hboxslicenr->addWidget(pb_first);
	hboxslicenr->addWidget(scb_slicenr);
	hboxslicenr->addWidget(pb_last);
	hboxslicenr->addWidget(sb_slicenr);
	hboxslicenr->addWidget(lb_slicenr);
	hboxslicenr->addWidget(lb_inactivewarning);
	hboxslicenrw->setLayout(hboxslicenr);

	QHBoxLayout* hboxstride = new QHBoxLayout;
	hboxstride->setSpacing(0);
	hboxstride->setMargin(0);
	hboxstride->addWidget(lb_stride);
	hboxstride->addWidget(sb_stride);
	hboxstridew->setLayout(hboxstride);

	QHBoxLayout* hboxslice = new QHBoxLayout;
	hboxslice->setSpacing(0);
	hboxslice->setMargin(0);
	hboxslice->addWidget(hboxslicenrw);
	hboxslice->addStretch();
	hboxslice->addWidget(hboxstridew);
	hboxslicew->setLayout(hboxslice);

	auto add_widget_filter = [this](QVBoxLayout* vbox, QPushButton* pb,
															 unsigned short i) {
#ifdef PLUGIN_VESSEL_WIDGET
		vbox->addWidget(pb);
#else
		if (tabwidgets[i] != vesselextr_widget)
		{
			vbox->addWidget(pb);
		}
#endif
	};

	QVBoxLayout* vboxtabs1 = new QVBoxLayout;
	vboxtabs1->setSpacing(0);
	vboxtabs1->setMargin(0);
	for (unsigned short i = 0; i < (nrtabbuttons + 1) / 2; i++)
	{
		add_widget_filter(vboxtabs1, pb_tab[i], i);
	}
	vboxtabs1w->setLayout(vboxtabs1);
	QVBoxLayout* vboxtabs2 = new QVBoxLayout;
	vboxtabs2->setSpacing(0);
	vboxtabs2->setMargin(0);
	for (unsigned short i = (nrtabbuttons + 1) / 2; i < nrtabbuttons; i++)
	{
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

	QHBoxLayout* hboxtabs = new QHBoxLayout;
	hboxtabs->setSpacing(0);
	hboxtabs->setMargin(0);
	hboxtabs->addWidget(vboxtabs1w);
	hboxtabs->addWidget(vboxtabs2w);
	hboxtabsw->setLayout(hboxtabs);
	hboxtabsw->setFixedWidth(hboxtabs->sizeHint().width());

	QDockWidget* tabswdock = new QDockWidget(tr("Methods"), this);
	style_dockwidget(tabswdock);
	tabswdock->setObjectName("Methods");
	tabswdock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
	tabswdock->setWidget(hboxtabsw);
	addDockWidget(Qt::BottomDockWidgetArea, tabswdock);

	QDockWidget* methodTabdock = new QDockWidget(tr("Parameters"), this);
	style_dockwidget(methodTabdock);
	methodTabdock->setObjectName("Parameters");
	methodTabdock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
	//	Q3ScrollView *tab_scroller=new Q3ScrollView(this);xxxa
	//	tab_scroller->addChild(methodTab);
	//	methodTabdock->setWidget(tab_scroller);
	methodTabdock->setWidget(methodTab);
	addDockWidget(Qt::BottomDockWidgetArea, methodTabdock);

	QVBoxLayout* vboxnotes = new QVBoxLayout;
	vboxnotes->setSpacing(0);
	vboxnotes->setMargin(0);

	QDockWidget* notesdock = new QDockWidget(tr("Notes"), this);
	style_dockwidget(notesdock);
	notesdock->setObjectName("Notes");
	notesdock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
	notesdock->setWidget(m_notes);
	addDockWidget(Qt::BottomDockWidgetArea, notesdock);

	QDockWidget* bitstackdock = new QDockWidget(tr("Img Clipboard"), this);
	style_dockwidget(bitstackdock);
	bitstackdock->setObjectName("Clipboard");
	bitstackdock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
	bitstackdock->setWidget(bitstack_widget);
	addDockWidget(Qt::BottomDockWidgetArea, bitstackdock);

	QDockWidget* multiDatasetDock = new QDockWidget(tr("Multi Dataset"), this);
	style_dockwidget(multiDatasetDock);
	multiDatasetDock->setObjectName("Multi Dataset");
	multiDatasetDock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
	multiDatasetDock->setWidget(multidataset_widget);
	addDockWidget(Qt::BottomDockWidgetArea, multiDatasetDock);

	QDockWidget* overlaydock = new QDockWidget(tr("Overlay"), this);
	style_dockwidget(overlaydock);
	overlaydock->setObjectName("Overlay");
	overlaydock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
	overlaydock->setWidget(overlay_widget);
	addDockWidget(Qt::BottomDockWidgetArea, overlaydock);

	QVBoxLayout* vbox2 = new QVBoxLayout;
	vbox2->setSpacing(0);
	vbox2->setMargin(0);
	vbox2->addWidget(hbox1w);
	vbox2->addWidget(hboxslicew);
	vbox2w->setLayout(vbox2);

	QHBoxLayout* hboxlock = new QHBoxLayout;
	hboxlock->setSpacing(0);
	hboxlock->setMargin(0);
	hboxlock->addWidget(cb_tissuelock);
	hboxlock->addWidget(lockTissues);
	hboxlock->addStretch();
	hboxlockw->setLayout(hboxlock);

	QVBoxLayout* vboxtissue = new QVBoxLayout;
	vboxtissue->setSpacing(0);
	vboxtissue->setMargin(0);
	vboxtissue->addWidget(tissueFilter);
	vboxtissue->addWidget(tissueTreeWidget);
	vboxtissue->addWidget(hboxlockw);
	vboxtissue->addWidget(addTissue);
	vboxtissue->addWidget(addFolder);
	vboxtissue->addWidget(modifyTissueFolder);

	QHBoxLayout* hboxtissueremove = new QHBoxLayout;
	hboxtissueremove->addWidget(removeTissueFolder);
	hboxtissueremove->addWidget(removeTissueFolderAll);
	vboxtissue->addLayout(hboxtissueremove);
	vboxtissue->addWidget(tissue3Dopt);
	QHBoxLayout* hboxtissueget = new QHBoxLayout;
	hboxtissueget->addWidget(getTissue);
	hboxtissueget->addWidget(getTissueAll);
	vboxtissue->addLayout(hboxtissueget);
	QHBoxLayout* hboxtissueclear = new QHBoxLayout;
	hboxtissueclear->addWidget(clearTissue);
	hboxtissueclear->addWidget(clearTissues);
	vboxtissue->addLayout(hboxtissueclear);
	vboxtissuew->setLayout(vboxtissue);
	//xxxb	vboxtissuew->setFixedHeight(vboxtissue->sizeHint().height());

	QVBoxLayout* vboxtissueadder1 = new QVBoxLayout;
	vboxtissueadder1->setSpacing(0);
	vboxtissueadder1->setMargin(0);
	vboxtissueadder1->addWidget(cb_addsub3d);
	vboxtissueadder1->addWidget(cb_addsuboverride);
	vboxtissueadder1->addWidget(pb_add);
	vboxtissueadder1->addWidget(pb_sub);
	vboxtissueadder1w->setLayout(vboxtissueadder1);
	vboxtissueadder1w->setFixedHeight(vboxtissueadder1->sizeHint().height());

	QVBoxLayout* vboxtissueadder2 = new QVBoxLayout;
	vboxtissueadder2->setSpacing(0);
	vboxtissueadder2->setMargin(0);
	vboxtissueadder2->addWidget(cb_addsubconn);
	vboxtissueadder2->addWidget(new QLabel(" ", this));
	vboxtissueadder2->addWidget(pb_addhold);
	vboxtissueadder2->addWidget(pb_subhold);
	vboxtissueadder2w->setLayout(vboxtissueadder2);
	vboxtissueadder2w->setFixedHeight(vboxtissueadder1->sizeHint().height());

	QHBoxLayout* hboxtissueadder = new QHBoxLayout;
	hboxtissueadder->setSpacing(0);
	hboxtissueadder->setMargin(0);
	hboxtissueadder->addWidget(vboxtissueadder1w);
	hboxtissueadder->addWidget(vboxtissueadder2w);
	hboxtissueadderw->setLayout(hboxtissueadder);
	hboxtissueadderw->setFixedHeight(hboxtissueadder->sizeHint().height());
	//xxxb	hboxtissueadderw->setFixedWidth(vboxtissue->sizeHint().width());

	QDockWidget* zoomdock = new QDockWidget(tr("Zoom"), this);
	style_dockwidget(zoomdock);
	zoomdock->setObjectName("Zoom");
	zoomdock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	zoomdock->setWidget(zoom_widget);
	addDockWidget(Qt::RightDockWidgetArea, zoomdock);

	QDockWidget* tissuewdock = new QDockWidget(tr("Tissues"), this);
	style_dockwidget(tissuewdock);
	tissuewdock->setObjectName("Tissues");
	tissuewdock->setAllowedAreas(Qt::LeftDockWidgetArea |
															 Qt::RightDockWidgetArea);
	tissuewdock->setWidget(vboxtissuew);
	addDockWidget(Qt::RightDockWidgetArea, tissuewdock);
	tissuesDock = tissuewdock;

	QDockWidget* tissuehierarchydock =
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

	QDockWidget* tissueadddock = new QDockWidget(tr("Adder"), this);
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

	QHBoxLayout* hbox2 = new QHBoxLayout;
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
	if (!m_editingmode)
	{
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
		loadmenu->insertItem("Open RTdose...", this, SLOT(execute_loadrtdose()));
		file->insertItem("&Open", loadmenu);
	}
	reloadmenu = new Q3PopupMenu(this, "reloadmenu");
	reloadmenu->insertItem("Reopen .dc&m...", this, SLOT(execute_reloaddicom()));
	reloadmenu->insertItem("Reopen .&bmp...", this, SLOT(execute_reloadbmp()));
	reloadmenu->insertItem("Reopen .raw...", this, SLOT(execute_reloadraw()));
	reloadmenu->insertItem("Reopen .mhd...", this, SLOT(execute_reloadmhd()));
	reloadmenu->insertItem("Reopen .avw...", this, SLOT(execute_reloadavw()));
	reloadmenu->insertItem("Reopen .vti/.vtk...", this, SLOT(execute_reloadvtk()));
	reloadmenu->insertItem("Reopen NIfTI...", this, SLOT(execute_reloadnifti()));
	reloadmenu->insertItem("Reopen RTdose...", this, SLOT(execute_reloadrtdose()));
	file->insertItem("&Reopen", reloadmenu);

	if (!m_editingmode)
	{
		openS4LLinkPos = file->actions().size();
		QAction* importS4LLink = file->addAction("Import S4L-link (h5)...");
		connect(importS4LLink, SIGNAL(triggered()), this,
				SLOT(execute_loads4llivelink()));
		importS4LLink->setToolTip("Loads a Sim4Life live link file (iSEG project .h5 file).");
		importS4LLink->setEnabled(true);
	}

	importSurfacePos = file->actions().size();
	QAction* importSurface = file->addAction("Import Surface/Lines...");
	connect(importSurface, SIGNAL(triggered()), this, SLOT(execute_loadsurface()));
	importSurface->setToolTip("Voxelize a surface or polyline mesh in the Target image.");
	importSurface->setEnabled(true);

	importRTstructPos = file->actions().size();
	QAction* importRTAction = file->addAction("Import RTstruct...");
	connect(importRTAction, SIGNAL(triggered()), this, SLOT(execute_loadrtstruct()));
	importRTAction->setToolTip("Some data must be opened first to import its RTStruct file");

	file->insertItem("&Export Image(s)...", this, SLOT(execute_saveimg()));
	file->insertItem("Export &Contour...", this, SLOT(execute_saveContours()));

	exportmenu = new Q3PopupMenu(this, "exportmenu");
	exportmenu->insertItem("Export &Labelfield...(am)", this, SLOT(execute_exportlabelfield()));
	exportmenu->insertItem("Export vtk-ascii...(vti/vtk)", this, SLOT(execute_exportvtkascii()));
	exportmenu->insertItem("Export vtk-binary...(vti/vtk)", this, SLOT(execute_exportvtkbinary()));
	exportmenu->insertItem("Export vtk-compressed-ascii...(vti)", this, SLOT(execute_exportvtkcompressedascii()));
	exportmenu->insertItem("Export vtk-compressed-binary...(vti)", this, SLOT(execute_exportvtkcompressedbinary()));
	exportmenu->insertItem("Export Matlab...(mat)", this, SLOT(execute_exportmat()));
	exportmenu->insertItem("Export hdf...(h5)", this, SLOT(execute_exporthdf()));
	exportmenu->insertItem("Export xml-extent index...(xml)", this, SLOT(execute_exportxmlregionextent()));
	exportmenu->insertItem("Export tissue index...(txt)", this, SLOT(execute_exporttissueindex()));
	file->insertItem("Export Tissue Distr.", exportmenu);
	file->insertItem("Export Color Lookup...", this, SLOT(execute_savecolorlookup()));
	file->insertSeparator();

	if (!m_editingmode)
		file->insertItem("Save &Project as...", this, SLOT(execute_saveprojas()));
	else
		file->insertItem("Save &Project-Copy as...", this, SLOT(execute_savecopyas()));
	file->insertItem(QIcon(m_picpath.absFilePath(QString("filesave.png"))), "Save Pro&ject", this, SLOT(execute_saveproj()), QKeySequence("Ctrl+S"));
	file->insertItem("Save Active Slices...", this, SLOT(execute_saveactiveslicesas()));
	if (!m_editingmode)
	{
		file->insertItem(QIcon(m_picpath.absFilePath(QString("fileopen.png"))), "Open P&roject...", this, SLOT(execute_loadproj()));
	}
	file->insertSeparator();
	file->insertItem("Save &Tissuelist...", this, SLOT(execute_savetissues()));
	file->insertItem("Open T&issuelist...", this, SLOT(execute_loadtissues()));
	file->insertItem("Set Tissuelist as Default", this, SLOT(execute_settissuesasdef()));
	file->insertItem("Remove Default Tissuelist", this, SLOT(execute_removedeftissues()));
	file->insertSeparator();
	if (!m_editingmode)
	{
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
	if (!m_editingmode)
	{
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
	imagemenu->insertItem("Tissue surface view", this,
			SLOT(execute_tissue_surfaceviewer()));
	imagemenu->insertItem("Target surface view", this,
			SLOT(execute_target_surfaceviewer()));
	imagemenu->insertItem("Source iso-surface view", this,
			SLOT(execute_source_surfaceviewer()));
	imagemenu->insertItem("Tissue volume view", this,
			SLOT(execute_3Dvolumeviewertissue()));
	imagemenu->insertItem("Source volume view", this,
			SLOT(execute_3Dvolumeviewerbmp()));
	if (!m_editingmode)
	{
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
	for (unsigned short i = 0; i < nrtabbuttons; i++)
	{
		showtab_action[i] = new Q3Action(tabwidgets[i]->GetName().c_str(), 0, this);
		showtab_action[i]->setToggleAction(true);
		showtab_action[i]->setOn(showpb_tab[i]);
		connect(showtab_action[i], SIGNAL(toggled(bool)), this, SLOT(execute_showtabtoggled(bool)));
		showtab_action[i]->addTo(hidesubmenu);
	}
	hideparameters = new Q3Action("Simplified", 0, this);
	hideparameters->setToggleAction(true);
	hideparameters->setOn(WidgetInterface::get_hideparams());
	connect(hideparameters, SIGNAL(toggled(bool)), this, SLOT(execute_hideparameters(bool)));
	hidesubmenu->insertSeparator();
	hideparameters->addTo(hidesubmenu);
	viewmenu->insertItem("&Toolbars", hidemenu);
	viewmenu->insertItem("Methods", hidesubmenu);

	toolmenu = menuBar()->addMenu(tr("T&ools"));
	toolmenu->insertItem("Target->Tissue", this, SLOT(do_work2tissue()));
	toolmenu->insertItem("Target->Tissue grouped...", this, SLOT(do_work2tissue_grouped()));
	toolmenu->insertItem("Tissue->Target", this, SLOT(do_tissue2work()));
	toolmenu->insertItem("In&verse Slice Order", this, SLOT(execute_inversesliceorder()));
	toolmenu->addSeparator();
	toolmenu->insertItem("&Group Tissues...", this, SLOT(execute_grouptissues()));
	toolmenu->insertItem("Remove Tissues...", this, SLOT(execute_removetissues()));
	if (!m_editingmode)
	{
		toolmenu->insertItem("Merge Projects...", this, SLOT(execute_mergeprojects()));
	}
	toolmenu->insertItem("Remove Unused Tissues", this, SLOT(execute_remove_unused_tissues()));
	toolmenu->insertItem("Supplant Selected Tissue", this, SLOT(execute_voting_replace_labels()));
	toolmenu->insertItem("Split Disconnected Tissue Regions", this, SLOT(execute_split_tissue()));
	toolmenu->insertItem("Compute Target Connectivity", this, SLOT(execute_target_connected_components()));
	toolmenu->addSeparator();
	toolmenu->insertItem("Clean Up", this, SLOT(execute_cleanup()));
	toolmenu->insertItem("Smooth Steps", this, SLOT(execute_smoothsteps()));
	toolmenu->insertItem("Check Bone Connectivity", this, SLOT(execute_boneconnectivity()));

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
	QObject::connect(bmp_show, SIGNAL(selecttissue_sign(Point, bool)), this,
			SLOT(select_tissue(Point, bool)));
	QObject::connect(bmp_show, SIGNAL(viewtissue_sign(Point)), this,
			SLOT(view_tissue(Point)));
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
	QObject::connect(work_show, SIGNAL(selecttissue_sign(Point, bool)), this,
			SLOT(select_tissue(Point, bool)));
	QObject::connect(work_show, SIGNAL(viewtissue_sign(Point)), this,
			SLOT(view_tissue(Point)));
	QObject::connect(tissueFilter, SIGNAL(textChanged(const QString&)), this,
			SLOT(tissueFilterChanged(const QString&)));
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

	QObject::connect(methodTab, SIGNAL(currentChanged(int)), this,
			SLOT(tab_changed(int)));

	tissueTreeWidget->setSelectionMode(
			QAbstractItemView::SelectionMode::ExtendedSelection);

	QObject::connect(tissueTreeWidget, SIGNAL(itemSelectionChanged()), this,
			SLOT(tissue_selection_changed()));
	QObject::connect(tissueTreeWidget,
			SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this,
			SLOT(tree_widget_doubleclicked(QTreeWidgetItem*, int)));
	QObject::connect(tissueTreeWidget,
			SIGNAL(customContextMenuRequested(const QPoint&)), this,
			SLOT(tree_widget_contextmenu(const QPoint&)));

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
	QObject::connect(sb_slicenr, SIGNAL(valueChanged(int)), this, SLOT(sb_slicenr_changed()));
	QObject::connect(scb_slicenr, SIGNAL(valueChanged(int)), this, SLOT(scb_slicenr_changed()));
	QObject::connect(sb_stride, SIGNAL(valueChanged(int)), this, SLOT(sb_stride_changed()));

	QObject::connect(pb_add, SIGNAL(clicked()), this, SLOT(add_tissue_pushed()));
	QObject::connect(pb_sub, SIGNAL(clicked()), this,
			SLOT(subtract_tissue_pushed()));
	QObject::connect(pb_addhold, SIGNAL(clicked()), this,
			SLOT(addhold_tissue_pushed()));
	QObject::connect(pb_subhold, SIGNAL(clicked()), this,
			SLOT(subtracthold_tissue_pushed()));

	QObject::connect(bmp_scroller, SIGNAL(contentsMoving(int, int)), this,
			SLOT(setWorkContentsPos(int, int)));
	QObject::connect(work_scroller, SIGNAL(contentsMoving(int, int)), this,
			SLOT(setBmpContentsPos(int, int)));
	QObject::connect(bmp_show, SIGNAL(setcenter_sign(int, int)), bmp_scroller,
			SLOT(center(int, int)));
	QObject::connect(work_show, SIGNAL(setcenter_sign(int, int)), work_scroller,
			SLOT(center(int, int)));
	tomove_scroller = true;

	QObject::connect(zoom_widget, SIGNAL(set_zoom(double)), bmp_show,
			SLOT(set_zoom(double)));
	QObject::connect(zoom_widget, SIGNAL(set_zoom(double)), work_show,
			SLOT(set_zoom(double)));

	QObject::connect(this,
			SIGNAL(begin_dataexport(iseg::DataSelection&, QWidget*)), this,
			SLOT(handle_begin_dataexport(iseg::DataSelection&, QWidget*)));
	QObject::connect(this, SIGNAL(end_dataexport(QWidget*)), this,
			SLOT(handle_end_dataexport(QWidget*)));

	QObject::connect(this,
			SIGNAL(begin_datachange(iseg::DataSelection&, QWidget*, bool)), this,
			SLOT(handle_begin_datachange(iseg::DataSelection&, QWidget*, bool)));
	QObject::connect(this,
			SIGNAL(end_datachange(QWidget*, iseg::EndUndoAction)), this,
			SLOT(handle_end_datachange(QWidget*, iseg::EndUndoAction)));

	QObject::connect(scale_dialog,
			SIGNAL(begin_datachange(iseg::DataSelection&, QWidget*, bool)), this,
			SLOT(handle_begin_datachange(iseg::DataSelection&, QWidget*, bool)));
	QObject::connect(scale_dialog,
			SIGNAL(end_datachange(QWidget*, iseg::EndUndoAction)), this,
			SLOT(handle_end_datachange(QWidget*, iseg::EndUndoAction)));
	QObject::connect(imagemath_dialog,
			SIGNAL(begin_datachange(iseg::DataSelection&, QWidget*, bool)), this,
			SLOT(handle_begin_datachange(iseg::DataSelection&, QWidget*, bool)));
	QObject::connect(imagemath_dialog,
			SIGNAL(end_datachange(QWidget*, iseg::EndUndoAction)), this,
			SLOT(handle_end_datachange(QWidget*, iseg::EndUndoAction)));
	QObject::connect(imageoverlay_dialog,
			SIGNAL(begin_datachange(iseg::DataSelection&, QWidget*, bool)), this,
			SLOT(handle_begin_datachange(iseg::DataSelection&, QWidget*, bool)));
	QObject::connect(imageoverlay_dialog,
			SIGNAL(end_datachange(QWidget*, iseg::EndUndoAction)), this,
			SLOT(handle_end_datachange(QWidget*, iseg::EndUndoAction)));
	QObject::connect(bitstack_widget,
			SIGNAL(begin_datachange(iseg::DataSelection&, QWidget*, bool)), this,
			SLOT(handle_begin_datachange(iseg::DataSelection&, QWidget*, bool)));
	QObject::connect(bitstack_widget,
			SIGNAL(end_datachange(QWidget*, iseg::EndUndoAction)), this,
			SLOT(handle_end_datachange(QWidget*, iseg::EndUndoAction)));
	QObject::connect(bitstack_widget,
			SIGNAL(begin_dataexport(iseg::DataSelection&, QWidget*)), this,
			SLOT(handle_begin_dataexport(iseg::DataSelection&, QWidget*)));
	QObject::connect(bitstack_widget, SIGNAL(end_dataexport(QWidget*)), this,
			SLOT(handle_end_dataexport(QWidget*)));
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

	QObject::connect(multidataset_widget, SIGNAL(dataset_changed()), bmp_show,
			SLOT(overlay_changed()));
	QObject::connect(multidataset_widget, SIGNAL(dataset_changed()), work_show,
			SLOT(overlay_changed()));
	QObject::connect(multidataset_widget, SIGNAL(dataset_changed()), this,
			SLOT(DatasetChanged()));
	QObject::connect(multidataset_widget,
			SIGNAL(begin_datachange(iseg::DataSelection&, QWidget*, bool)), this,
			SLOT(handle_begin_datachange(iseg::DataSelection&, QWidget*, bool)));
	QObject::connect(multidataset_widget,
			SIGNAL(end_datachange(QWidget*, iseg::EndUndoAction)), this,
			SLOT(handle_end_datachange(QWidget*, iseg::EndUndoAction)));

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

	// \todo BL add generic connections here, e.g. begin/end_datachange

	m_widget_signal_mapper = new QSignalMapper(this);
	for (int i = 0; i < nrtabbuttons; ++i)
	{
		QObject::connect(pb_tab[i], SIGNAL(clicked()), m_widget_signal_mapper,
				SLOT(map()));
		m_widget_signal_mapper->setMapping(pb_tab[i], i);
		QObject::connect(m_widget_signal_mapper, SIGNAL(mapped(int)), this,
				SLOT(pb_tab_pressed(int)));
	}
	for (auto widget : tabwidgets)
	{
		assert(widget);
		QObject::connect(widget,
				SIGNAL(begin_datachange(iseg::DataSelection&, QWidget*, bool)), this,
				SLOT(handle_begin_datachange(iseg::DataSelection&, QWidget*, bool)));
		QObject::connect(widget,
				SIGNAL(end_datachange(QWidget*, iseg::EndUndoAction)), this,
				SLOT(handle_end_datachange(QWidget*, iseg::EndUndoAction)));
	}

	QObject::connect(bmp_show, SIGNAL(mousePosZoom_sign(QPoint)), this,
			SLOT(mousePosZoom_changed(const QPoint&)));
	QObject::connect(work_show, SIGNAL(mousePosZoom_sign(QPoint)), this,
			SLOT(mousePosZoom_changed(const QPoint&)));

	QObject::connect(bmp_show, SIGNAL(wheelrotatedctrl_sign(int)), this,
			SLOT(wheelrotated(int)));
	QObject::connect(work_show, SIGNAL(wheelrotatedctrl_sign(int)), this,
			SLOT(wheelrotated(int)));

	//	QObject::connect(pb_work2tissue,SIGNAL(clicked()),this,SLOT(do_work2tissue()));

	m_acc_sliceup = new Q3Accel(this);
	m_acc_sliceup->connectItem(m_acc_sliceup->insertItem(QKeySequence(Qt::CTRL + Qt::Key_Right)), this,
			SLOT(slicenr_up()));
	m_acc_slicedown = new Q3Accel(this);
	m_acc_slicedown->connectItem(m_acc_slicedown->insertItem(QKeySequence(Qt::CTRL + Qt::Key_Left)), this,
			SLOT(slicenr_down()));
	m_acc_sliceup1 = new Q3Accel(this);
	m_acc_sliceup->connectItem(m_acc_sliceup->insertItem(QKeySequence(Qt::Key_Next)), this,
			SLOT(slicenr_up()));
	m_acc_slicedown1 = new Q3Accel(this);
	m_acc_slicedown->connectItem(m_acc_slicedown->insertItem(QKeySequence(Qt::Key_Prior)), this,
			SLOT(slicenr_down()));
	m_acc_zoomin = new Q3Accel(this);
	m_acc_zoomin->connectItem(m_acc_zoomin->insertItem(QKeySequence(Qt::CTRL + Qt::Key_Up)), this,
			SLOT(zoom_in()));
	m_acc_zoomout = new Q3Accel(this);
	m_acc_zoomout->connectItem(m_acc_zoomout->insertItem(QKeySequence(Qt::CTRL + Qt::Key_Down)), this,
			SLOT(zoom_out()));
	m_acc_add = new Q3Accel(this);
	m_acc_add->connectItem(m_acc_add->insertItem(QKeySequence(Qt::CTRL + Qt::Key_Plus)), this,
			SLOT(add_tissue_shortkey()));
	m_acc_sub = new Q3Accel(this);
	m_acc_sub->connectItem(m_acc_sub->insertItem(QKeySequence(Qt::CTRL + Qt::Key_Minus)), this,
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

	update_brightnesscontrast(true);
	update_brightnesscontrast(false);

	tissuenr_changed(currTissueType - 1);

	this->setMinimumHeight(this->minimumHeight() + 50);

	m_Modified = false;
	m_NewDataAfterSwap = false;
}

void MainWindow::closeEvent(QCloseEvent* qce)
{
	if (maybeSafe())
	{
		if (xsliceshower != nullptr)
		{
			xsliceshower->close();
			delete xsliceshower;
		}
		if (ysliceshower != nullptr)
		{
			ysliceshower->close();
			delete ysliceshower;
		}
		if (surface_viewer != nullptr)
		{
			surface_viewer->close();
			delete surface_viewer;
		}
		if (VV3D != nullptr)
		{
			VV3D->close();
			delete VV3D;
		}
		if (VV3Dbmp != nullptr)
		{
			VV3Dbmp->close();
			delete VV3Dbmp;
		}

		SaveSettings();
		SaveLoadProj(m_loadprojfilename.m_filename);
		QMainWindow::closeEvent(qce);
	}
	else
	{
		qce->ignore();
	}
}

bool MainWindow::maybeSafe()
{
	if (modified())
	{
		int ret = QMessageBox::warning(
				this, "iSeg", "Do you want to save your changes?",
				QMessageBox::Yes | QMessageBox::Default, QMessageBox::No,
				QMessageBox::Cancel | QMessageBox::Escape);
		if (ret == QMessageBox::Yes)
		{
			// Handle tissue hierarchy changes
			if (!tissueHierarchyWidget->handle_changed_hierarchy())
			{
				return false;
			}
			if ((!m_editingmode && !m_saveprojfilename.isEmpty()) ||
					(m_editingmode && !m_S4Lcommunicationfilename.isEmpty()))
			{
				execute_saveproj();
			}
			else
			{
				execute_saveprojas();
			}
			return true;
		}
		else if (ret == QMessageBox::Cancel)
		{
			return false;
		}
	}
	return true;
}

bool MainWindow::modified() { return m_Modified; }

void MainWindow::execute_bmp2work()
{
	iseg::DataSelection dataSelection;
	dataSelection.sliceNr = handler3D->active_slice();
	dataSelection.work = true;
	emit begin_datachange(dataSelection, this);

	handler3D->bmp2work();

	emit end_datachange(this);
}

void MainWindow::execute_work2bmp()
{
	iseg::DataSelection dataSelection;
	dataSelection.sliceNr = handler3D->active_slice();
	dataSelection.bmp = true;
	emit begin_datachange(dataSelection, this);

	handler3D->work2bmp();

	emit end_datachange(this);
}

void MainWindow::execute_swap_bmpwork()
{
	iseg::DataSelection dataSelection;
	dataSelection.sliceNr = handler3D->active_slice();
	dataSelection.bmp = true;
	dataSelection.work = true;
	emit begin_datachange(dataSelection, this);

	handler3D->swap_bmpwork();

	emit end_datachange(this);
}

void MainWindow::execute_swap_bmpworkall()
{
	iseg::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.bmp = true;
	dataSelection.work = true;
	emit begin_datachange(dataSelection, this);

	handler3D->swap_bmpworkall();

	emit end_datachange(this);
}

void MainWindow::execute_saveContours()
{
	iseg::DataSelection dataSelection;
	dataSelection.tissues = true;
	emit begin_dataexport(dataSelection, this);

	SaveOutlinesWidget SOW(handler3D, this);
	SOW.move(QCursor::pos());
	SOW.exec();

	emit end_dataexport(this);
}

void MainWindow::execute_swapxy()
{
	iseg::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this, false);

	bool ok = handler3D->SwapXY();

	// Swap also the other datasets
	bool swapExtraDatasets = false;
	if (multidataset_widget->isVisible() && swapExtraDatasets)
	{
		unsigned short w, h, nrslices;
		w = handler3D->height();
		h = handler3D->width();
		nrslices = handler3D->num_slices();
		QString str1;
		for (int i = 0; i < multidataset_widget->GetNumberOfDatasets(); i++)
		{
			// Swap all but the active one
			if (!multidataset_widget->IsActive(i))
			{
				std::string tempFileName = "bmp_float_eds_" + std::to_string(i) + ".raw";
				str1 = QDir::temp().absFilePath(QString(tempFileName.c_str()));
				if (SlicesHandler::SaveRaw_xy_swapped(str1.ascii(), multidataset_widget->GetBmpData(i), w, h, nrslices) != 0)
					ok = false;

				if (ok)
				{
					tempFileName = "bmp_float_eds_" + std::to_string(i) + ".raw";
					str1 = QDir::temp().absFilePath(QString(tempFileName.c_str()));
					str1 = QDir::temp().absFilePath(QString("bmp_float.raw"));
					multidataset_widget->SetBmpData(i, SlicesHandler::LoadRawFloat(str1.ascii(), handler3D->start_slice(), handler3D->end_slice(), 0, w * h));
				}
			}
		}

		m_NewDataAfterSwap = true;
	}

	emit end_datachange(this, iseg::ClearUndo);
	Pair p = handler3D->get_pixelsize();
	if (p.low != p.high)
	{
		handler3D->set_pixelsize(p.low, p.high);
		pixelsize_changed();
	}
	clear_stack();
}

void MainWindow::execute_swapxz()
{
	iseg::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this, false);

	bool ok = handler3D->SwapXZ();

	// Swap also the other datasets
	bool swapExtraDatasets = false;
	if (multidataset_widget->isVisible() && swapExtraDatasets)
	{
		unsigned short w, h, nrslices;
		w = handler3D->num_slices();
		h = handler3D->height();
		nrslices = handler3D->width();
		QString str1;
		for (int i = 0; i < multidataset_widget->GetNumberOfDatasets(); i++)
		{
			// Swap all but the active one
			if (!multidataset_widget->IsActive(i))
			{
				std::string tempFileName = "bmp_float_" + std::to_string(i) + ".raw";
				str1 = QDir::temp().absFilePath(QString(tempFileName.c_str()));
				if (SlicesHandler::SaveRaw_xz_swapped(str1.ascii(), multidataset_widget->GetBmpData(i), w, h, nrslices) != 0)
					ok = false;

				if (ok)
				{
					tempFileName = "bmp_float_" + std::to_string(i) + ".raw";
					str1 = QDir::temp().absFilePath(QString(tempFileName.c_str()));
					multidataset_widget->SetBmpData(i, SlicesHandler::LoadRawFloat(str1.ascii(), handler3D->start_slice(), handler3D->end_slice(), 0, w * h));
				}
			}
		}
		m_NewDataAfterSwap = true;
	}

	emit end_datachange(this, iseg::ClearUndo);

	Pair p = handler3D->get_pixelsize();
	float thick = handler3D->get_slicethickness();
	if (thick != p.high)
	{
		handler3D->set_pixelsize(thick, p.low);
		handler3D->set_slicethickness(p.high);
		pixelsize_changed();
		slicethickness_changed();
	}

	clear_stack();
}

void MainWindow::execute_swapyz()
{
	iseg::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this, false);

	bool ok = handler3D->SwapYZ();

	// Swap also the other datasets
	bool swapExtraDatasets = false;
	if (multidataset_widget->isVisible() && swapExtraDatasets)
	{
		unsigned short w, h, nrslices;
		w = handler3D->width();
		h = handler3D->num_slices();
		nrslices = handler3D->height();
		QString str1;
		for (int i = 0; i < multidataset_widget->GetNumberOfDatasets(); i++)
		{
			// Swap all but the active one
			if (!multidataset_widget->IsActive(i))
			{
				std::string tempFileName = "bmp_float_" + std::to_string(i) + ".raw";
				str1 = QDir::temp().absFilePath(QString(tempFileName.c_str()));
				if (SlicesHandler::SaveRaw_yz_swapped(str1.ascii(), multidataset_widget->GetBmpData(i), w, h, nrslices) != 0)
					ok = false;

				if (ok)
				{
					tempFileName = "bmp_float_" + std::to_string(i) + ".raw";
					str1 = QDir::temp().absFilePath(QString(tempFileName.c_str()));
					multidataset_widget->SetBmpData(i, SlicesHandler::LoadRawFloat(str1.ascii(), handler3D->start_slice(), handler3D->end_slice(), 0, w * h));
				}
			}
		}
		m_NewDataAfterSwap = true;
	}

	emit end_datachange(this, iseg::ClearUndo);

	Pair p = handler3D->get_pixelsize();
	float thick = handler3D->get_slicethickness();
	if (thick != p.low)
	{
		handler3D->set_pixelsize(p.high, thick);
		handler3D->set_slicethickness(p.low);
		pixelsize_changed();
		slicethickness_changed();
	}
	clear_stack();
}

void MainWindow::execute_resize(int resizetype)
{
	iseg::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this, false);

	ResizeDialog RD(handler3D, static_cast<ResizeDialog::eResizeType>(resizetype),
			this);
	RD.move(QCursor::pos());
	if (!RD.exec())
	{
		return;
	}
	auto lut = handler3D->GetColorLookupTable();

	int dxm, dxp, dym, dyp, dzm, dzp;
	RD.return_padding(dxm, dxp, dym, dyp, dzm, dzp);

	unsigned short w, h, nrslices;
	w = handler3D->width();
	h = handler3D->height();
	nrslices = handler3D->num_slices();
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

	if (ok)
	{
		str1 = QDir::temp().absFilePath(QString("work_float.raw"));
		if (handler3D->ReadRawFloat(str1.ascii(), w + dxm + dxp, h + dym + dyp, 0,
						nrslices + dzm + dzp) != 1)
			ok = false;
	}
	if (ok)
	{
		str1 = QDir::temp().absFilePath(QString("bmp_float.raw"));
		if (handler3D->ReloadRawFloat(str1.ascii(), 0) != 1)
			ok = false;
	}
	if (ok)
	{
		str1 = QDir::temp().absFilePath(QString("tissues.raw"));
		if (handler3D->ReloadRawTissues(str1.ascii(), sizeof(tissues_size_t) * 8,
						0) != 1)
			ok = false;
	}

	Transform tr = handler3D->transform();
	Vec3 spacing = handler3D->spacing();

	Transform transform_corrected(tr);
	int plo[3] = {dxm, dym, dzm};
	transform_corrected.paddingUpdateTransform(plo, spacing.v);
	handler3D->set_transform(transform_corrected);

	// add color lookup table again
	handler3D->UpdateColorLookupTable(lut);

	emit end_datachange(this, iseg::ClearUndo);

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
			std::max(0.01f, std::min(le_contrastbmp_val->text().toFloat(), 100.0f));
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
			std::max(0.01f, std::min(le_contrastwork_val->text().toFloat(), 100.0f));
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
	int brightness = (int)std::max(-50.0f, std::min(std::floor(le_brightnessbmp_val->text().toFloat() + 0.5f), 50.0f));
	le_brightnessbmp_val->setText(QString("%1").arg(brightness, 3));

	// Update slider
	sl_brightnessbmp->setValue(brightness + 50);

	// Update display
	update_brightnesscontrast(true);
}

void MainWindow::le_brightnesswork_val_edited()
{
	// Clamp to range and round to precision
	int brightness = (int)std::max(-50.0f, std::min(std::floor(le_brightnesswork_val->text().toFloat() + 0.5f), 50.0f));
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
	if (bmporwork)
	{
		// Get values from line edits
		float contrast = le_contrastbmp_val->text().toFloat();
		float brightness = (le_brightnessbmp_val->text().toFloat() + 50.0f) * 0.01f;

		// Update bmp shower
		bmp_show->set_brightnesscontrast(brightness, contrast, paint);
	}
	else
	{
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
		if (idx >= 0)
		{
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

	QStringList files = QFileDialog::getOpenFileNames("Images (*.bmp)\nAll (*.*)",
			QString::null, this, "open files dialog", "Select one or more files to open");

	if (!files.empty())
	{
		files.sort();

		std::vector<int> vi;
		vi.clear();

		short nrelem = files.size();

		for (short i = 0; i < nrelem; i++)
		{
			vi.push_back(bmpimgnr(&files[i]));
		}

		std::vector<const char*> vfilenames;
		for (short i = 0; i < nrelem; i++)
		{
			vfilenames.push_back(files[i].ascii());
		}

		iseg::DataSelection dataSelection;
		dataSelection.allSlices = true;
		dataSelection.bmp = true;
		dataSelection.work = true;
		dataSelection.tissues = true;
		emit begin_datachange(dataSelection, this, false);

		LoaderColorImages LB(handler3D, LoaderColorImages::kBMP, vfilenames, this);
		LB.move(QCursor::pos());
		LB.exec();

		emit end_datachange(this, iseg::ClearUndo);

		reset_brightnesscontrast();

		EnableActionsAfterPrjLoaded(true);
	}
}

void MainWindow::execute_loadpng()
{
	maybeSafe();

	QStringList files = QFileDialog::getOpenFileNames("Images (*.png)\nAll (*.*)",
			QString::null, this, "open files dialog",
			"Select one or more files to open");

	if (!files.empty())
	{
		files.sort();

		int nrelem = files.size();
		std::vector<const char*> vfilenames;
		for (int i = 0; i < nrelem; i++)
		{
			vfilenames.push_back(files[i].ascii());
		}

		iseg::DataSelection dataSelection;
		dataSelection.allSlices = true;
		dataSelection.bmp = true;
		dataSelection.work = true;
		dataSelection.tissues = true;
		emit begin_datachange(dataSelection, this, false);

		LoaderColorImages LB(handler3D, LoaderColorImages::kPNG, vfilenames, this);
		LB.move(QCursor::pos());
		LB.exec();

		emit end_datachange(this, iseg::ClearUndo);

		reset_brightnesscontrast();

		EnableActionsAfterPrjLoaded(true);
	}
}

void MainWindow::execute_loadjpg()
{
	maybeSafe();

	QStringList files =
			QFileDialog::getOpenFileNames("Images (*.jpg)\nAll (*.*)",
					QString::null, this, "open files dialog",
					"Select one or more files to open");

	if (!files.empty())
	{
		files.sort();

		std::vector<int> vi;
		vi.clear();

		short nrelem = files.size();

		for (short i = 0; i < nrelem; i++)
		{
			vi.push_back(jpgimgnr(&files[i]));
		}

		std::vector<const char*> vfilenames;
		for (short i = 0; i < nrelem; i++)
		{
			vfilenames.push_back(files[i].ascii());
		}

		iseg::DataSelection dataSelection;
		dataSelection.allSlices = true;
		dataSelection.bmp = true;
		dataSelection.work = true;
		dataSelection.tissues = true;
		emit begin_datachange(dataSelection, this, false);

		LoaderColorImages LB(handler3D, LoaderColorImages::kJPG, vfilenames, this);
		LB.move(QCursor::pos());
		LB.exec();

		emit end_datachange(this, iseg::ClearUndo);

		//	hbox1->setFixedSize(bmphand->return_width()*2+vbox1->sizeHint().width(),bmphand->return_height());

		reset_brightnesscontrast();

		EnableActionsAfterPrjLoaded(true);
	}
}

void MainWindow::execute_loaddicom()
{
	maybeSafe();

	QStringList files = QFileDialog::getOpenFileNames("Images (*.dcm *.dicom)\nAll (*)",
			QString::null, this, "open files dialog",
			"Select one or more files to open");

	if (!files.empty())
	{
		iseg::DataSelection dataSelection;
		dataSelection.allSlices = true;
		dataSelection.bmp = true;
		dataSelection.work = true;
		dataSelection.tissues = true;
		emit begin_datachange(dataSelection, this, false);

		LoaderDicom LD(handler3D, &files, false, this);
		LD.move(QCursor::pos());
		LD.exec();

		emit end_datachange(this, iseg::ClearUndo);

		pixelsize_changed();
		slicethickness_changed();

		//	handler3D->LoadDICOM();

		//	work_show->update();//(bmphand->return_width(),bmphand->return_height());
		//	bmp_show->update();//(bmphand->return_width(),bmphand->return_height());
		//	bmp_show->workborder_changed();

		//	hbox1->setFixedSize(bmphand->return_width()*2+vbox1->sizeHint().width(),bmphand->return_height());

		reset_brightnesscontrast();

		EnableActionsAfterPrjLoaded(true);
	}
}

void MainWindow::execute_reloaddicom()
{
	QStringList files = QFileDialog::getOpenFileNames(
			"Images (*.dcm *.dicom)", QString::null, this, "open files dialog",
			"Select one or more files to open");

	if (!files.empty())
	{
		iseg::DataSelection dataSelection;
		dataSelection.allSlices = true;
		dataSelection.bmp = true;
		dataSelection.work = true;
		dataSelection.tissues = true;
		emit begin_datachange(dataSelection, this, false);

		LoaderDicom LD(handler3D, &files, true, this);
		LD.move(QCursor::pos());
		LD.exec();
		pixelsize_changed();
		slicethickness_changed();

		//	handler3D->LoadDICOM();

		//	work_show->update();//(bmphand->return_width(),bmphand->return_height());
		//	bmp_show->update();//(bmphand->return_width(),bmphand->return_height());
		//	bmp_show->workborder_changed();

		emit end_datachange(this, iseg::ClearUndo);

		//	hbox1->setFixedSize(bmphand->return_width()*2+vbox1->sizeHint().width(),bmphand->return_height());

		reset_brightnesscontrast();
	}
}

void MainWindow::execute_loadraw()
{
	maybeSafe();

	iseg::DataSelection dataSelection;
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

	emit end_datachange(this, iseg::ClearUndo);

	//	hbox1->setFixedSize(bmphand->return_width()*2+vbox1->sizeHint().width(),bmphand->return_height());

	reset_brightnesscontrast();

	EnableActionsAfterPrjLoaded(true);
}

void MainWindow::execute_loadmhd()
{
	maybeSafe();

	iseg::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this, false);

	QString loadfilename = QFileDialog::getOpenFileName(QString::null,
			"Metaheader (*.mhd *.mha)\nAll (*.*)", this);
	if (!loadfilename.isEmpty())
	{
		handler3D->ReadImage(loadfilename.ascii());
	}

	emit end_datachange(this, iseg::ClearUndo);
	pixelsize_changed();
	slicethickness_changed();

	reset_brightnesscontrast();

	EnableActionsAfterPrjLoaded(true);
}

void MainWindow::execute_loadvtk()
{
	maybeSafe();

	iseg::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this, false);

	bool res = true;
	QString loadfilename = QFileDialog::getOpenFileName(QString::null,
			"VTK (*.vti *.vtk)\n"
			"All (*.*)",
			this);
	if (!loadfilename.isEmpty())
	{
		res = handler3D->ReadImage(loadfilename.ascii());
	}

	emit end_datachange(this, iseg::ClearUndo);
	pixelsize_changed();
	slicethickness_changed();

	reset_brightnesscontrast();

	EnableActionsAfterPrjLoaded(true);

	if (!res)
	{
		QMessageBox::warning(this, "iSeg",
				"Error: Could not load file\n" + loadfilename + "\n",
				QMessageBox::Ok | QMessageBox::Default);
	}
}

void MainWindow::execute_loadnifti()
{
	maybeSafe();

	iseg::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this, false);

	QString loadfilename =
			QFileDialog::getOpenFileName(QString::null,
					"NIFTI (*.nii *.hdr *.img *.nii.gz)\n"
					"All (*.*)",
					this);
	if (!loadfilename.isEmpty())
	{
		handler3D->ReadImage(loadfilename.ascii());
	}

	emit end_datachange(this, iseg::ClearUndo);
	pixelsize_changed();
	slicethickness_changed();

	reset_brightnesscontrast();

	EnableActionsAfterPrjLoaded(true);
}

void MainWindow::execute_loadavw()
{
	maybeSafe();

	iseg::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this, false);

	QString loadfilename = QFileDialog::getOpenFileName(QString::null,
			"AnalzyeAVW (*.avw)\n"
			"All (*.*)",
			this);
	if (!loadfilename.isEmpty())
	{
		handler3D->ReadAvw(loadfilename.ascii());
	}

	emit end_datachange(this, iseg::ClearUndo);
	pixelsize_changed();
	slicethickness_changed();

	reset_brightnesscontrast();

	EnableActionsAfterPrjLoaded(true);
}

void MainWindow::execute_reloadbmp()
{
	QStringList files = QFileDialog::getOpenFileNames("Images (*.bmp)\nAll (*.*)",
			QString::null, this, "open files dialog",
			"Select one or more files to open");

	if ((unsigned short)files.size() == handler3D->num_slices() ||
			(unsigned short)files.size() ==
					(handler3D->end_slice() - handler3D->start_slice()))
	{
		files.sort();

		std::vector<int> vi;
		vi.clear();

		short nrelem = files.size();

		for (short i = 0; i < nrelem; i++)
		{
			vi.push_back(bmpimgnr(&files[i]));
		}

		std::vector<const char*> vfilenames;
		for (short i = 0; i < nrelem; i++)
		{
			vfilenames.push_back(files[i].ascii());
		}

		iseg::DataSelection dataSelection;
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

		emit end_datachange(this, iseg::ClearUndo);

		//	hbox1->setFixedSize(bmphand->return_width()*2+vbox1->sizeHint().width(),bmphand->return_height());
		reset_brightnesscontrast();
	}
	else
	{
		if (!files.empty())
		{
			QMessageBox::information(
					this, "Segm. Tool",
					"You have to select the same number of slices.\n");
		}
	}
}

void MainWindow::execute_reloadraw()
{
	iseg::DataSelection dataSelection;
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

	emit end_datachange(this, iseg::ClearUndo);
	//	hbox1->setFixedSize(bmphand->return_width()*2+vbox1->sizeHint().width(),bmphand->return_height());

	reset_brightnesscontrast();
}

void MainWindow::execute_reloadavw()
{
	iseg::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this, false);

	QString loadfilename = QFileDialog::getOpenFileName(QString::null,
			"AnalzyeAVW (*.avw)\n"
			"All (*.*)",
			this);
	if (!loadfilename.isEmpty())
	{
		handler3D->ReloadAVW(loadfilename.ascii(), handler3D->start_slice());
		reset_brightnesscontrast();
	}
	emit end_datachange(this, iseg::ClearUndo);
}

void MainWindow::execute_reloadmhd()
{
	iseg::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this, false);

	QString loadfilename =
			QFileDialog::getOpenFileName(QString::null,
					"Metaheader (*.mhd *.mha)\n"
					"All (*.*)",
					this);
	if (!loadfilename.isEmpty())
	{
		handler3D->ReloadImage(loadfilename.ascii(),
				handler3D->start_slice());
		reset_brightnesscontrast();
	}
	emit end_datachange(this, iseg::ClearUndo);
}

void MainWindow::execute_reloadvtk()
{
	iseg::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this, false);

	QString loadfilename = QFileDialog::getOpenFileName(QString::null,
			"VTK (*.vti *.vtk)\n"
			"All (*.*)",
			this);
	if (!loadfilename.isEmpty())
	{
		handler3D->ReloadImage(loadfilename.ascii(),
				handler3D->start_slice());
		reset_brightnesscontrast();
	}
	emit end_datachange(this, iseg::ClearUndo);
}

void MainWindow::execute_reloadnifti()
{
	iseg::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this, false);

	QString loadfilename =
			QFileDialog::getOpenFileName(QString::null,
					"NIFTI (*.nii *.hdr *.img *.nii.gz)\n"
					"All (*.*)",
					this);
	if (!loadfilename.isEmpty())
	{
		handler3D->ReloadImage(
				loadfilename.ascii(),
				handler3D->start_slice()); // TODO: handle failure
		reset_brightnesscontrast();
	}
	emit end_datachange(this, iseg::ClearUndo);
}

void MainWindow::execute_loadsurface()
{
	maybeSafe();

	bool ok = true;
	QString loadfilename = QFileDialog::getOpenFileName(QString::null,
			"Surfaces & Polylines (*.stl *.vtk)\n"
			"All (*.*)",
			this);
	if (!loadfilename.isEmpty())
	{
		QCheckBox* cb = new QCheckBox("Intersect Only");
		cb->setToolTip("If on, the intersection of the surface with the voxels will be computed, not the interior of a closed surface.");
		cb->setChecked(false);
		cb->blockSignals(true);

		QMessageBox msgBox;
		msgBox.setWindowTitle("Import Surface");
		msgBox.setText("What would you like to do what with the imported surface?");
		msgBox.addButton("Overwrite", QMessageBox::YesRole); //==0
		msgBox.addButton("Add", QMessageBox::NoRole);				 //==1
		msgBox.addButton(cb, QMessageBox::ResetRole);				 //
		msgBox.addButton("Cancel", QMessageBox::RejectRole); //==2
		int overwrite = msgBox.exec();
		bool intersect = cb->isChecked();

		if (overwrite == 2)
			return;

		ok = handler3D->LoadSurface(loadfilename.toStdString(), overwrite == 0, intersect);
	}

	if (ok)
	{
		reset_brightnesscontrast();
	}
	else
	{
		QMessageBox::warning(this, "iSeg",
				"Error: Surface does not overlap with image",
				QMessageBox::Ok | QMessageBox::Default);
	}
}

void MainWindow::execute_loadrtstruct()
{
#ifndef NORTSTRUCTSUPPORT
	QString loadfilename = QFileDialog::getOpenFileName(QString::null,
			"RTstruct (*.dcm)\n"
			"All (*.*)",
			this); //, filename);

	if (loadfilename.isEmpty())
	{
		return;
	}

	RadiotherapyStructureSetImporter RI(loadfilename, handler3D, this);

	QObject::connect(
			&RI, SIGNAL(begin_datachange(iseg::DataSelection&, QWidget*, bool)),
			this,
			SLOT(handle_begin_datachange(iseg::DataSelection&, QWidget*, bool)));
	QObject::connect(&RI, SIGNAL(end_datachange(QWidget*, iseg::EndUndoAction)),
			this,
			SLOT(handle_end_datachange(QWidget*, iseg::EndUndoAction)));

	RI.move(QCursor::pos());
	RI.exec();

	tissueTreeWidget->update_tree_widget();
	tissuenr_changed(tissueTreeWidget->get_current_type() - 1);

	QObject::disconnect(
			&RI, SIGNAL(begin_datachange(iseg::DataSelection&, QWidget*, bool)),
			this,
			SLOT(handle_begin_datachange(iseg::DataSelection&, QWidget*, bool)));
	QObject::disconnect(
			&RI, SIGNAL(end_datachange(QWidget*, iseg::EndUndoAction)), this,
			SLOT(handle_end_datachange(QWidget*, iseg::EndUndoAction)));

	reset_brightnesscontrast();
#endif
}

void MainWindow::execute_loadrtdose()
{
	maybeSafe();

	iseg::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this, false);

	QString loadfilename = QFileDialog::getOpenFileName(QString::null, "RTdose (*.dcm)\nAll (*.*)", this);
	if (!loadfilename.isEmpty())
	{
		handler3D->ReadRTdose(loadfilename.ascii());
	}

	emit end_datachange(this, iseg::ClearUndo);
	pixelsize_changed();
	slicethickness_changed();

	reset_brightnesscontrast();

	EnableActionsAfterPrjLoaded(true);
}

void MainWindow::execute_reloadrtdose()
{
	iseg::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this, false);

	QString loadfilename = QFileDialog::getOpenFileName(QString::null, "RTdose (*.dcm)\nAll (*.*)", this);
	if (!loadfilename.isEmpty())
	{
		handler3D->ReloadRTdose(loadfilename.ascii(), handler3D->start_slice()); // TODO: handle failure
		reset_brightnesscontrast();
	}
	emit end_datachange(this, iseg::ClearUndo);
}

void MainWindow::execute_loads4llivelink()
{
	QString loadfilename = QFileDialog::getOpenFileName(QString::null,
			"S4L Link (*.h5)\n"
			"All (*.*)",
			this);
	if (!loadfilename.isEmpty())
	{
		loadS4Llink(loadfilename);
	}
}

void MainWindow::execute_saveimg()
{
	iseg::DataSelection dataSelection;
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
	iseg::DataSelection dataSelection;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	dataSelection.tissueHierarchy = true;
	emit begin_dataexport(dataSelection, this);

	QString savefilename = QFileDialog::getSaveFileName(
			QString::null, "Projects (*.prj)\n", this); //, filename);

	if (!savefilename.isEmpty())
	{
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

		setCaption(QString(" iSeg ") + QString(xstr(ISEG_VERSION)) +
							 QString(" - ") + TruncateFileName(savefilename));

		//m_saveprojfilename = tempFileName;
		//AddLoadProj(tempFileName);
		AddLoadProj(m_saveprojfilename);
		FILE* fp = handler3D->SaveProject(tempFileName.ascii(), "xmf");
		fp = bitstack_widget->save_proj(fp);
		unsigned short saveProjVersion = 12;
		fp = TissueInfos::SaveTissues(fp, saveProjVersion);
		unsigned short combinedVersion =
				(unsigned short)iseg::CombineTissuesVersion(saveProjVersion, 1);
		fwrite(&combinedVersion, 1, sizeof(unsigned short), fp);
		/*for(size_t i=0;i<tabwidgets.size();i++){
			fp=((QWidget1 *)(tabwidgets[i]))->SaveParams(fp,saveProjVersion);
		}*/
		threshold_widget->SaveParams(fp, saveProjVersion);
		hyst_widget->SaveParams(fp, saveProjVersion);
		livewire_widget->SaveParams(fp, saveProjVersion);
		iftrg_widget->SaveParams(fp, saveProjVersion);
		fastmarching_widget->SaveParams(fp, saveProjVersion);
		watershed_widget->SaveParams(fp, saveProjVersion);
		olc_widget->SaveParams(fp, saveProjVersion);
		interpolation_widget->SaveParams(fp, saveProjVersion);
		smoothing_widget->SaveParams(fp, saveProjVersion);
		morphology_widget->SaveParams(fp, saveProjVersion);
		edge_widget->SaveParams(fp, saveProjVersion);
		feature_widget->SaveParams(fp, saveProjVersion);
		measurement_widget->SaveParams(fp, saveProjVersion);
		vesselextr_widget->SaveParams(fp, saveProjVersion);
		picker_widget->SaveParams(fp, saveProjVersion);
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
	iseg::DataSelection dataSelection;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	dataSelection.tissueHierarchy = true;
	emit begin_dataexport(dataSelection, this);

	QString savefilename = QFileDialog::getSaveFileName(
			QString::null, "Projects (*.prj)\n", this); //, filename);

	if (!savefilename.isEmpty())
	{
		if (savefilename.length() <= 4 || !savefilename.endsWith(QString(".prj")))
			savefilename.append(".prj");

		FILE* fp = handler3D->SaveProject(savefilename.ascii(), "xmf");
		fp = bitstack_widget->save_proj(fp);
		unsigned short saveProjVersion = 12;
		fp = TissueInfos::SaveTissues(fp, saveProjVersion);
		unsigned short combinedVersion =
				(unsigned short)iseg::CombineTissuesVersion(saveProjVersion, 1);
		fwrite(&combinedVersion, 1, sizeof(unsigned short), fp);
		/*for(size_t i=0;i<tabwidgets.size();i++){
			fp=((QWidget1 *)(tabwidgets[i]))->SaveParams(fp,saveProjVersion);
		}*/
		threshold_widget->SaveParams(fp, saveProjVersion);
		hyst_widget->SaveParams(fp, saveProjVersion);
		livewire_widget->SaveParams(fp, saveProjVersion);
		iftrg_widget->SaveParams(fp, saveProjVersion);
		fastmarching_widget->SaveParams(fp, saveProjVersion);
		watershed_widget->SaveParams(fp, saveProjVersion);
		olc_widget->SaveParams(fp, saveProjVersion);
		interpolation_widget->SaveParams(fp, saveProjVersion);
		smoothing_widget->SaveParams(fp, saveProjVersion);
		morphology_widget->SaveParams(fp, saveProjVersion);
		edge_widget->SaveParams(fp, saveProjVersion);
		feature_widget->SaveParams(fp, saveProjVersion);
		measurement_widget->SaveParams(fp, saveProjVersion);
		vesselextr_widget->SaveParams(fp, saveProjVersion);
		picker_widget->SaveParams(fp, saveProjVersion);
		tissueTreeWidget->SaveParams(fp, saveProjVersion);
		fp = TissueInfos::SaveTissueLocks(fp);
		fp = save_notes(fp, saveProjVersion);

		fclose(fp);
	}
	emit end_dataexport(this);
}

void MainWindow::SaveSettings()
{
	FILE* fp = fopen(settingsfile.c_str(), "wb");
	if (fp == nullptr)
		return;
	unsigned short saveProjVersion = 12;
	unsigned short combinedVersion =
			(unsigned short)iseg::CombineTissuesVersion(saveProjVersion, 1);
	fwrite(&combinedVersion, 1, sizeof(unsigned short), fp);
	bool flag;
	flag = WidgetInterface::get_hideparams();
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
	for (unsigned short i = 0; i < 16; i++)
	{
		flag = (showtab_action[i]) ? showtab_action[i]->isOn() : true;
		fwrite(&flag, 1, sizeof(bool), fp);
	}
	fp = TissueInfos::SaveTissues(fp, saveProjVersion);
	/*for(size_t i=0;i<tabwidgets.size();i++){
		fp=((QWidget1 *)(tabwidgets[i]))->SaveParams(fp,saveProjVersion);
	}*/
	threshold_widget->SaveParams(fp, saveProjVersion);
	hyst_widget->SaveParams(fp, saveProjVersion);
	livewire_widget->SaveParams(fp, saveProjVersion);
	iftrg_widget->SaveParams(fp, saveProjVersion);
	fastmarching_widget->SaveParams(fp, saveProjVersion);
	watershed_widget->SaveParams(fp, saveProjVersion);
	olc_widget->SaveParams(fp, saveProjVersion);
	interpolation_widget->SaveParams(fp, saveProjVersion);
	smoothing_widget->SaveParams(fp, saveProjVersion);
	morphology_widget->SaveParams(fp, saveProjVersion);
	edge_widget->SaveParams(fp, saveProjVersion);
	feature_widget->SaveParams(fp, saveProjVersion);
	measurement_widget->SaveParams(fp, saveProjVersion);
	vesselextr_widget->SaveParams(fp, saveProjVersion);
	picker_widget->SaveParams(fp, saveProjVersion);
	tissueTreeWidget->SaveParams(fp, saveProjVersion);

	fclose(fp);

	QSettings settings(QSettings::IniFormat, QSettings::UserScope, "ZMT", "iSeg");
	settings.beginGroup("MainWindow");
	settings.setValue("geometry", saveGeometry());
	settings.setValue("state", saveState());
	settings.setValue("NumberOfUndoSteps", this->handler3D->GetNumberOfUndoSteps());
	settings.setValue("NumberOfUndoArrays", this->handler3D->GetNumberOfUndoArrays());
	settings.setValue("Compression", this->handler3D->GetCompression());
	settings.setValue("ContiguousMemory", this->handler3D->GetContiguousMemory());
	settings.setValue("BloscEnabled", BloscEnabled());
	settings.endGroup();
	settings.sync();
}

void MainWindow::LoadSettings(const char* loadfilename)
{
	settingsfile = loadfilename;
	FILE* fp;
	if ((fp = fopen(loadfilename, "rb")) == nullptr)
	{
		return;
	}

	unsigned short combinedVersion = 0;
	if (fread(&combinedVersion, sizeof(unsigned short), 1, fp) == 0)
	{
		fclose(fp);
		return;
	}
	int loadProjVersion, tissuesVersion;
	iseg::ExtractTissuesVersion((int)combinedVersion, loadProjVersion, tissuesVersion);

	iseg::DataSelection dataSelection;
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

	if (loadProjVersion >= 7)
	{
		fread(&flag, sizeof(bool), 1, fp);
	}
	else
	{
		flag = false;
	}

	fread(&flag, sizeof(bool), 1, fp);
	fread(&flag, sizeof(bool), 1, fp);
	hidecontrastbright->setOn(!flag);
	execute_hidecontrastbright(!flag);

	fread(&flag, sizeof(bool), 1, fp);
	hidecopyswap->setOn(!flag);
	execute_hidecopyswap(!flag);

	// turn visibility on for all
	auto nrtabbuttons = (unsigned short)tabwidgets.size();
	for (int i = 0; i < nrtabbuttons; i++)
	{
		showtab_action[i]->setOn(true);
	}

	// load visibility settings from file
	for (unsigned short i = 0; i < 14; i++)
	{
		fread(&flag, sizeof(bool), 1, fp);
		if (i < nrtabbuttons)
			showtab_action.at(i)->setOn(flag);
	}
	if (loadProjVersion >= 6)
	{
		fread(&flag, sizeof(bool), 1, fp);
		if (14 < nrtabbuttons)
			showtab_action.at(14)->setOn(flag);
	}
	if (loadProjVersion >= 9)
	{
		fread(&flag, sizeof(bool), 1, fp);
		if (15 < nrtabbuttons)
			showtab_action.at(15)->setOn(flag);
	}
	execute_showtabtoggled(flag);

	fp = TissueInfos::LoadTissues(fp, tissuesVersion);
	const char* defaultTissuesFilename = m_tmppath.absFilePath(QString("def_tissues.txt"));
	FILE* fpTmp = fopen(defaultTissuesFilename, "r");
	if (fpTmp != nullptr || TissueInfos::GetTissueCount() <= 0)
	{
		if (fpTmp != nullptr)
			fclose(fpTmp);
		TissueInfos::LoadDefaultTissueList(defaultTissuesFilename);
	}
	tissueTreeWidget->update_tree_widget();
	emit end_datachange(this, iseg::ClearUndo);
	tissuenr_changed(tissueTreeWidget->get_current_type() - 1);

	/*for(size_t i=0;i<tabwidgets.size();i++){
		fp=((QWidget1 *)(tabwidgets[i]))->LoadParams(fp,version);
	}*/
	threshold_widget->LoadParams(fp, loadProjVersion);
	hyst_widget->LoadParams(fp, loadProjVersion);
	livewire_widget->LoadParams(fp, loadProjVersion);
	iftrg_widget->LoadParams(fp, loadProjVersion);
	fastmarching_widget->LoadParams(fp, loadProjVersion);
	watershed_widget->LoadParams(fp, loadProjVersion);
	olc_widget->LoadParams(fp, loadProjVersion);
	interpolation_widget->LoadParams(fp, loadProjVersion);
	smoothing_widget->LoadParams(fp, loadProjVersion);
	morphology_widget->LoadParams(fp, loadProjVersion);
	edge_widget->LoadParams(fp, loadProjVersion);
	feature_widget->LoadParams(fp, loadProjVersion);
	measurement_widget->LoadParams(fp, loadProjVersion);
	vesselextr_widget->LoadParams(fp, loadProjVersion);
	picker_widget->LoadParams(fp, loadProjVersion);
	tissueTreeWidget->LoadParams(fp, loadProjVersion);
	fclose(fp);

	if (loadProjVersion > 7)
	{
		ISEG_INFO_MSG("LoadSettings() : restoring values...");
		QSettings settings(QSettings::IniFormat, QSettings::UserScope, "ZMT", "iSeg");
		settings.beginGroup("MainWindow");
		restoreGeometry(settings.value("geometry").toByteArray());
		restoreState(settings.value("state").toByteArray());
		this->handler3D->SetNumberOfUndoSteps(settings.value("NumberOfUndoSteps", 50).toUInt());
		ISEG_INFO("NumberOfUndoSteps = " << this->handler3D->GetNumberOfUndoSteps());
		this->handler3D->SetNumberOfUndoArrays(settings.value("NumberOfUndoArrays", 20).toUInt());
		ISEG_INFO("NumberOfUndoArrays = " << this->handler3D->GetNumberOfUndoArrays());
		this->handler3D->SetCompression(settings.value("Compression", 0).toInt());
		ISEG_INFO("Compression = " << this->handler3D->GetCompression());
		this->handler3D->SetContiguousMemory(settings.value("ContiguousMemory", true).toBool());
		ISEG_INFO("ContiguousMemory = " << this->handler3D->GetContiguousMemory());
		SetBloscEnabled(settings.value("BloscEnabled", false).toBool());
		ISEG_INFO("BloscEnabled = " << BloscEnabled());
		settings.endGroup();

		if (this->handler3D->return_nrundo() == 0)
			this->editmenu->setItemEnabled(undonr, false);
		else
			editmenu->setItemEnabled(undonr, true);
	}
}

void MainWindow::execute_saveactiveslicesas()
{
	iseg::DataSelection dataSelection;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	dataSelection.tissueHierarchy = true;
	emit begin_dataexport(dataSelection, this);

	QString savefilename = QFileDialog::getSaveFileName(
			QString::null, "Projects (*.prj)\n", this); //, filename);

	if (savefilename.length() <= 4 || !savefilename.endsWith(QString(".prj")))
		savefilename.append(".prj");

	if (!savefilename.isEmpty())
	{
		FILE* fp = handler3D->SaveActiveSlices(savefilename.ascii(), "xmf");
		fp = bitstack_widget->save_proj(fp);
		unsigned short saveProjVersion = 12;
		fp = TissueInfos::SaveTissues(fp, saveProjVersion);
		unsigned short combinedVersion =
				(unsigned short)iseg::CombineTissuesVersion(saveProjVersion, 1);
		fwrite(&combinedVersion, 1, sizeof(unsigned short), fp);
		/*for(size_t i=0;i<tabwidgets.size();i++){
			fp=((QWidget1 *)(tabwidgets[i]))->SaveParams(fp,saveProjVersion);
		}*/
		threshold_widget->SaveParams(fp, saveProjVersion);
		hyst_widget->SaveParams(fp, saveProjVersion);
		livewire_widget->SaveParams(fp, saveProjVersion);
		iftrg_widget->SaveParams(fp, saveProjVersion);
		fastmarching_widget->SaveParams(fp, saveProjVersion);
		watershed_widget->SaveParams(fp, saveProjVersion);
		olc_widget->SaveParams(fp, saveProjVersion);
		interpolation_widget->SaveParams(fp, saveProjVersion);
		smoothing_widget->SaveParams(fp, saveProjVersion);
		morphology_widget->SaveParams(fp, saveProjVersion);
		edge_widget->SaveParams(fp, saveProjVersion);
		feature_widget->SaveParams(fp, saveProjVersion);
		measurement_widget->SaveParams(fp, saveProjVersion);
		vesselextr_widget->SaveParams(fp, saveProjVersion);
		picker_widget->SaveParams(fp, saveProjVersion);
		tissueTreeWidget->SaveParams(fp, saveProjVersion);
		fp = TissueInfos::SaveTissueLocks(fp);
		fp = save_notes(fp, saveProjVersion);

		fclose(fp);
	}
	emit end_dataexport(this);
}

void MainWindow::execute_saveproj()
{
	iseg::DataSelection dataSelection;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	dataSelection.tissueHierarchy = true;
	emit begin_dataexport(dataSelection, this);

	if (m_editingmode)
	{
		if (!m_S4Lcommunicationfilename.isEmpty())
		{
			handler3D->SaveCommunicationFile(m_S4Lcommunicationfilename.ascii());
		}
	}
	else
	{
		if (!m_saveprojfilename.isEmpty())
		{
			//Append "Temp" at the end of the file name and rename it at the end of the successful saving process
			QString tempFileName = QString(m_saveprojfilename);
			int afterDot = tempFileName.lastIndexOf('.');
			if (afterDot != 0)
				tempFileName =
						tempFileName.remove(afterDot, tempFileName.length() - afterDot) +
						"Temp.prj";
			else
				tempFileName = tempFileName + "Temp.prj";

			std::string pjFN = m_saveprojfilename.toStdString();

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

			setCaption(QString(" iSeg ") + QString(xstr(ISEG_VERSION)) +
								 QString(" - ") + TruncateFileName(m_saveprojfilename));

			m_saveprojfilename = tempFileName;

			//FILE *fp=handler3D->SaveProject(m_saveprojfilename.ascii(),"xmf");
			FILE* fp = handler3D->SaveProject(tempFileName.ascii(), "xmf");
			fp = bitstack_widget->save_proj(fp);
			unsigned short saveProjVersion = 12;
			fp = TissueInfos::SaveTissues(fp, saveProjVersion);
			unsigned short combinedVersion =
					(unsigned short)iseg::CombineTissuesVersion(saveProjVersion, 1);
			fwrite(&combinedVersion, 1, sizeof(unsigned short), fp);
			/*for(size_t i=0;i<tabwidgets.size();i++){
				fp=((QWidget1 *)(tabwidgets[i]))->SaveParams(fp,saveProjVersion);
			}*/
			threshold_widget->SaveParams(fp, saveProjVersion);
			hyst_widget->SaveParams(fp, saveProjVersion);
			livewire_widget->SaveParams(fp, saveProjVersion);
			iftrg_widget->SaveParams(fp, saveProjVersion);
			fastmarching_widget->SaveParams(fp, saveProjVersion);
			watershed_widget->SaveParams(fp, saveProjVersion);
			olc_widget->SaveParams(fp, saveProjVersion);
			interpolation_widget->SaveParams(fp, saveProjVersion);
			smoothing_widget->SaveParams(fp, saveProjVersion);
			morphology_widget->SaveParams(fp, saveProjVersion);
			edge_widget->SaveParams(fp, saveProjVersion);
			feature_widget->SaveParams(fp, saveProjVersion);
			measurement_widget->SaveParams(fp, saveProjVersion);
			vesselextr_widget->SaveParams(fp, saveProjVersion);
			picker_widget->SaveParams(fp, saveProjVersion);
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

			if (QFile::exists(sourceFileNameWithoutExtension + ".xmf"))
			{
				bool removeSuccess =
						QFile::remove(sourceFileNameWithoutExtension + ".xmf");
				while (!removeSuccess)
				{
					int ret = mBox.exec();
					if (ret == QMessageBox::Cancel)
						return;

					removeSuccess =
							QFile::remove(sourceFileNameWithoutExtension + ".xmf");
				}
			}
			QFile::rename(tempFileNameWithoutExtension + ".xmf",
					sourceFileNameWithoutExtension + ".xmf");

			if (QFile::exists(sourceFileNameWithoutExtension + ".prj"))
			{
				bool removeSuccess =
						QFile::remove(sourceFileNameWithoutExtension + ".prj");
				while (!removeSuccess)
				{
					int ret = mBox.exec();
					if (ret == QMessageBox::Cancel)
						return;

					removeSuccess =
							QFile::remove(sourceFileNameWithoutExtension + ".prj");
				}
			}
			QFile::rename(tempFileNameWithoutExtension + ".prj",
					sourceFileNameWithoutExtension + ".prj");

			if (QFile::exists(sourceFileNameWithoutExtension + ".h5"))
			{
				bool removeSuccess =
						QFile::remove(sourceFileNameWithoutExtension + ".h5");
				while (!removeSuccess)
				{
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
		else
		{
			if (modified())
			{
				execute_saveprojas();
			}
		}
	}
	emit end_dataexport(this);
}

void MainWindow::loadproj(const QString& loadfilename)
{
	FILE* fp;
	if ((fp = fopen(loadfilename.ascii(), "r")) == nullptr)
	{
		return;
	}
	else
	{
		fclose(fp);
	}
	bool stillopen = false;

	iseg::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this, false);

	if (!loadfilename.isEmpty())
	{
		m_saveprojfilename = loadfilename;
		setCaption(QString(" iSeg ") + QString(xstr(ISEG_VERSION)) + QString(" - ") + TruncateFileName(loadfilename));
		AddLoadProj(m_saveprojfilename);
		int tissuesVersion = 0;
		fp = handler3D->LoadProject(loadfilename.ascii(), tissuesVersion);
		fp = bitstack_widget->load_proj(fp);
		fp = TissueInfos::LoadTissues(fp, tissuesVersion);
		stillopen = true;
	}

	emit end_datachange(this, iseg::ClearUndo);
	tissuenr_changed(tissueTreeWidget->get_current_type() - 1);

	pixelsize_changed();
	slicethickness_changed();

	if (stillopen)
	{
		unsigned short combinedVersion = 0;
		if (fread(&combinedVersion, sizeof(unsigned short), 1, fp) > 0)
		{
			int loadProjVersion, tissuesVersion;
			iseg::ExtractTissuesVersion((int)combinedVersion, loadProjVersion, tissuesVersion);
			/*for(size_t i=0;i<tabwidgets.size();i++){
				fp=((QWidget1 *)(tabwidgets[i]))->LoadParams(fp,version);
			}*/
			threshold_widget->LoadParams(fp, loadProjVersion);
			hyst_widget->LoadParams(fp, loadProjVersion);
			livewire_widget->LoadParams(fp, loadProjVersion);
			iftrg_widget->LoadParams(fp, loadProjVersion);
			fastmarching_widget->LoadParams(fp, loadProjVersion);
			watershed_widget->LoadParams(fp, loadProjVersion);
			olc_widget->LoadParams(fp, loadProjVersion);
			interpolation_widget->LoadParams(fp, loadProjVersion);
			smoothing_widget->LoadParams(fp, loadProjVersion);
			morphology_widget->LoadParams(fp, loadProjVersion);
			edge_widget->LoadParams(fp, loadProjVersion);
			feature_widget->LoadParams(fp, loadProjVersion);
			measurement_widget->LoadParams(fp, loadProjVersion);
			vesselextr_widget->LoadParams(fp, loadProjVersion);
			picker_widget->LoadParams(fp, loadProjVersion);
			tissueTreeWidget->LoadParams(fp, loadProjVersion);

			if (loadProjVersion >= 3)
			{
				fp = TissueInfos::LoadTissueLocks(fp);
				tissues_size_t currTissueType = tissueTreeWidget->get_current_type();
				cb_tissuelock->setChecked(
						TissueInfos::GetTissueLocked(currTissueType + 1));
				tissueTreeWidget->update_tissue_icons();
				tissueTreeWidget->update_folder_icons();
			}
			fp = load_notes(fp, loadProjVersion);
		}

		fclose(fp);
	}

	reset_brightnesscontrast();

	EnableActionsAfterPrjLoaded(true);
}

void MainWindow::loadS4Llink(const QString& loadfilename)
{
	if (loadfilename.isEmpty() || !boost::filesystem::exists(loadfilename.toStdString()))
	{
		return;
	}

	iseg::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this, false);

	m_S4Lcommunicationfilename = loadfilename;
	int tissuesVersion = 0;
	handler3D->LoadS4Llink(loadfilename.ascii(), tissuesVersion);
	TissueInfos::LoadTissuesHDF(loadfilename.ascii(), tissuesVersion);

	emit end_datachange(this, iseg::ClearUndo);
	tissues_size_t m;
	handler3D->get_rangetissue(&m);
	handler3D->buildmissingtissues(m);
	tissueTreeWidget->update_tree_widget();
	tissuenr_changed(tissueTreeWidget->get_current_type() - 1);

	pixelsize_changed();
	slicethickness_changed();

	tissueTreeWidget->update_tissue_icons();
	tissueTreeWidget->update_folder_icons();

	reset_brightnesscontrast();
}

void MainWindow::execute_mergeprojects()
{
	iseg::DataSelection dataSelection;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	dataSelection.tissueHierarchy = true;
	emit begin_dataexport(dataSelection, this);

	// Get list of project file names to be merged
	MergeProjectsDialog mergeDialog(this, "Merge Projects");
	if (mergeDialog.exec() != QDialog::Accepted)
	{
		emit end_dataexport(this);
		return;
	}
	std::vector<QString> mergefilenames;
	mergeDialog.get_filenames(mergefilenames);

	// Get save file name
	QString savefilename = QFileDialog::getSaveFileName(QString::null, "Projects (*.prj)", this,
			"iSeg", "Save merged project as");
	if (savefilename.length() <= 4 || !savefilename.endsWith(QString(".prj")))
		savefilename.append(".prj");

	if (!savefilename.isEmpty() && mergefilenames.size() > 0)
	{
		FILE* fp = handler3D->MergeProjects(savefilename.ascii(), mergefilenames);
		if (!fp)
		{
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
		unsigned short combinedVersion = (unsigned short)iseg::CombineTissuesVersion(saveProjVersion, 1);
		fwrite(&combinedVersion, 1, sizeof(unsigned short), fp);

		threshold_widget->SaveParams(fp, saveProjVersion);
		hyst_widget->SaveParams(fp, saveProjVersion);
		livewire_widget->SaveParams(fp, saveProjVersion);
		iftrg_widget->SaveParams(fp, saveProjVersion);
		fastmarching_widget->SaveParams(fp, saveProjVersion);
		watershed_widget->SaveParams(fp, saveProjVersion);
		olc_widget->SaveParams(fp, saveProjVersion);
		interpolation_widget->SaveParams(fp, saveProjVersion);
		smoothing_widget->SaveParams(fp, saveProjVersion);
		morphology_widget->SaveParams(fp, saveProjVersion);
		edge_widget->SaveParams(fp, saveProjVersion);
		feature_widget->SaveParams(fp, saveProjVersion);
		measurement_widget->SaveParams(fp, saveProjVersion);
		vesselextr_widget->SaveParams(fp, saveProjVersion);
		picker_widget->SaveParams(fp, saveProjVersion);
		tissueTreeWidget->SaveParams(fp, saveProjVersion);
		fp = TissueInfos::SaveTissueLocks(fp);
		fp = save_notes(fp, saveProjVersion);

		fclose(fp);

		// Load merged project
		if (QMessageBox::question(this, "iSeg",
						"Would you like to load the merged project?",
						QMessageBox::Yes | QMessageBox::Default,
						QMessageBox::No) == QMessageBox::Yes)
		{
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

	QString loadfilename = QFileDialog::getOpenFileName(QString::null,
			"Projects (*.prj)\n"
			"All (*.*)",
			this); //, filename);

	if (!loadfilename.isEmpty())
	{
		loadproj(loadfilename);
	}
}

void MainWindow::execute_loadproj1()
{
	maybeSafe();

	QString loadfilename = m_loadprojfilename.m_loadprojfilename1;

	if (!loadfilename.isEmpty())
	{
		loadproj(loadfilename);
	}
}

void MainWindow::execute_loadproj2()
{
	maybeSafe();

	QString loadfilename = m_loadprojfilename.m_loadprojfilename2;

	if (!loadfilename.isEmpty())
	{
		loadproj(loadfilename);
	}
}

void MainWindow::execute_loadproj3()
{
	maybeSafe();

	QString loadfilename = m_loadprojfilename.m_loadprojfilename3;

	if (!loadfilename.isEmpty())
	{
		loadproj(loadfilename);
	}
}

void MainWindow::execute_loadproj4()
{
	maybeSafe();

	QString loadfilename = m_loadprojfilename.m_loadprojfilename4;

	if (!loadfilename.isEmpty())
	{
		loadproj(loadfilename);
	}
}

void MainWindow::execute_loadatlas(int i)
{
	AtlasWidget* AW = new AtlasWidget(
			m_atlasfilename.m_atlasdir.absFilePath(m_atlasfilename.m_atlasfilename[i])
					.ascii(),
			m_picpath);
	if (AW->isOK)
	{
		AW->show();
		AW->raise();
		AW->setAttribute(Qt::WA_DeleteOnClose);
	}
	else
	{
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
	iseg::DataSelection dataSelection;
	dataSelection.tissues = true;
	emit begin_dataexport(dataSelection, this);

	QString savefilename = QFileDialog::getSaveFileName(QString::null, "Atlas file (*.atl)", this);

	if (savefilename.length() > 4 && !savefilename.endsWith(QString(".atl")))
		savefilename.append(".atl");

	if (!savefilename.isEmpty())
	{
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
	QString savefilename = QFileDialog::getSaveFileName(QString::null, QString::null, this);

	if (!savefilename.isEmpty())
	{
		unsigned short saveVersion = 7;
		TissueInfos::SaveTissuesReadable(savefilename.ascii(), saveVersion);
	}
}

void MainWindow::execute_exportlabelfield()
{
	iseg::DataSelection dataSelection;
	dataSelection.tissues = true;
	emit begin_dataexport(dataSelection, this);

	QString savefilename = QFileDialog::getSaveFileName(QString::null, "AmiraMesh Ascii (*.am)", this);

	if (savefilename.length() > 4 && !savefilename.endsWith(QString(".am")))
		savefilename.append(".am");

	if (!savefilename.isEmpty())
	{
		handler3D->print_amascii(savefilename.ascii());
	}

	emit end_dataexport(this);
}

void MainWindow::execute_exportmat()
{
	iseg::DataSelection dataSelection;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	emit begin_dataexport(dataSelection, this);

	QString savefilename = QFileDialog::getSaveFileName(QString::null, "Matlab (*.mat)", this);

	if (savefilename.length() > 4 && !savefilename.endsWith(QString(".mat")))
		savefilename.append(".mat");

	if (!savefilename.isEmpty())
	{
		handler3D->print_tissuemat(savefilename.ascii());
	}

	emit end_dataexport(this);
}

void MainWindow::execute_exporthdf()
{
	QString savefilename = QFileDialog::getSaveFileName(QString::null, "HDF (*.h5)", this);

	if (savefilename.length() > 3 && !savefilename.endsWith(QString(".h5")))
		savefilename.append(".h5");

	if (!savefilename.isEmpty())
	{
		iseg::DataSelection dataSelection;
		dataSelection.bmp = true;
		dataSelection.work = true;
		dataSelection.tissues = true;
		emit begin_dataexport(dataSelection, this);

		handler3D->SaveCommunicationFile(savefilename.ascii());

		emit end_dataexport(this);
	}
}

void MainWindow::execute_exportvtkascii()
{
	iseg::DataSelection dataSelection;
	dataSelection.tissues = true;
	emit begin_dataexport(dataSelection, this);

	QString savefilename = QFileDialog::getSaveFileName(QString::null, "VTK Ascii (*.vti *.vtk)", this);

	if (savefilename.length() > 4 && !(savefilename.endsWith(QString(".vti")) || savefilename.endsWith(QString(".vtk"))))
		savefilename.append(".vti");

	if (!savefilename.isEmpty())
	{
		handler3D->export_tissue(savefilename.ascii(), false);
	}

	emit end_dataexport(this);
}

void MainWindow::execute_exportvtkbinary()
{
	iseg::DataSelection dataSelection;
	dataSelection.tissues = true;
	emit begin_dataexport(dataSelection, this);

	QString savefilename = QFileDialog::getSaveFileName(QString::null, "VTK bin (*.vti *.vtk)", this);

	if (savefilename.length() > 4 && !(savefilename.endsWith(QString(".vti")) || savefilename.endsWith(QString(".vtk"))))
		savefilename.append(".vti");

	if (!savefilename.isEmpty())
	{
		handler3D->export_tissue(savefilename.ascii(), true);
	}

	emit end_dataexport(this);
}

void MainWindow::execute_exportvtkcompressedascii()
{
	iseg::DataSelection dataSelection;
	dataSelection.tissues = true;
	emit begin_dataexport(dataSelection, this);

	QString savefilename = QFileDialog::getSaveFileName(QString::null, "VTK comp (*.vti)", this);

	if (savefilename.length() > 4 && !savefilename.endsWith(QString(".vti")))
		savefilename.append(".vti");

	if (!savefilename.isEmpty())
	{
		handler3D->export_tissue(savefilename.ascii(), false);
	}

	emit end_dataexport(this);
}

void MainWindow::execute_exportvtkcompressedbinary()
{
	iseg::DataSelection dataSelection;
	dataSelection.tissues = true;
	emit begin_dataexport(dataSelection, this);

	QString savefilename = QFileDialog::getSaveFileName(QString::null, "VTK comp (*.vti)", this);

	if (savefilename.length() > 4 && !savefilename.endsWith(QString(".vti")))
		savefilename.append(".vti");

	if (!savefilename.isEmpty())
	{
		handler3D->export_tissue(savefilename.ascii(), true);
	}

	emit end_dataexport(this);
}

void MainWindow::execute_exportxmlregionextent()
{
	iseg::DataSelection dataSelection;
	dataSelection.tissues = true;
	emit begin_dataexport(dataSelection, this);

	QString savefilename = QFileDialog::getSaveFileName(QString::null, "XML extent (*.xml)", this);

	if (savefilename.length() > 4 && !savefilename.endsWith(QString(".xml")))
		savefilename.append(".xml");

	QString relfilename = QString("");
	if (!m_saveprojfilename.isEmpty())
		relfilename = m_saveprojfilename.right(m_saveprojfilename.length() -
																					 m_saveprojfilename.findRev("/") - 1);

	if (!savefilename.isEmpty())
	{
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
	iseg::DataSelection dataSelection;
	emit begin_dataexport(dataSelection, this);

	QString savefilename = QFileDialog::getSaveFileName(QString::null, "tissue index (*.txt)", this);

	if (savefilename.length() > 4 && !savefilename.endsWith(QString(".txt")))
		savefilename.append(".txt");

	QString relfilename = QString("");
	if (!m_saveprojfilename.isEmpty())
		relfilename = m_saveprojfilename.right(m_saveprojfilename.length() - m_saveprojfilename.findRev("/") - 1);

	if (!savefilename.isEmpty())
	{
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
	QString loadfilename = QFileDialog::getOpenFileName(QString::null, QString::null, this);
	if (!loadfilename.isEmpty())
	{
		QMessageBox msgBox;
		msgBox.setText("Do you want to append the new tissues or to replace the old tissues?");
		QPushButton* appendButton = msgBox.addButton(tr("Append"), QMessageBox::AcceptRole);
		QPushButton* replaceButton = msgBox.addButton(tr("Replace"), QMessageBox::AcceptRole);
		QPushButton* abortButton = msgBox.addButton(QMessageBox::Abort);
		msgBox.setIcon(QMessageBox::Question);
		msgBox.exec();

		tissues_size_t removeTissuesRange;
		if (msgBox.clickedButton() == appendButton)
		{
			TissueInfos::LoadTissuesReadable(loadfilename.ascii(), handler3D,
					removeTissuesRange);
			int nr = tissueTreeWidget->get_current_type() - 1;
			tissueTreeWidget->update_tree_widget();

			tissues_size_t currTissueType = tissueTreeWidget->get_current_type();
			if (nr != currTissueType - 1)
			{
				tissuenr_changed(currTissueType - 1);
			}
		}
		else if (msgBox.clickedButton() == replaceButton)
		{
			if (TissueInfos::LoadTissuesReadable(loadfilename.ascii(), handler3D,
							removeTissuesRange))
			{
				if (removeTissuesRange > 0)
				{
#if 1 // Version: Ask user whether to delete tissues
					int ret = QMessageBox::question(
							this, "iSeg",
							"Some of the previously existing tissues are\nnot "
							"contained in "
							"the loaded tissue list.\n\nDo you want to delete "
							"them?",
							QMessageBox::Yes | QMessageBox::Default, QMessageBox::No);
					if (ret == QMessageBox::Yes)
					{
						std::set<tissues_size_t> removeTissues;
						for (tissues_size_t type = 1; type <= removeTissuesRange; ++type)
						{
							removeTissues.insert(type);
						}
						iseg::DataSelection dataSelection;
						dataSelection.allSlices = true;
						dataSelection.tissues = true;
						emit begin_datachange(dataSelection, this, false);
						handler3D->remove_tissues(removeTissues);
						emit end_datachange(this, iseg::ClearUndo);
					}
#else
					std::set<tissues_size_t> removeTissues;
					for (tissues_size_t type = 1; type <= removeTissuesRange; ++type)
					{
						removeTissues.insert(type);
					}
					iseg::DataSelection dataSelection;
					dataSelection.allSlices = true;
					dataSelection.tissues = true;
					emit begin_datachange(dataSelection, this, false);
					handler3D->remove_tissues(removeTissues);
					emit end_datachange(this, iseg::ClearUndo);
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
	iseg::DataSelection dataSelection;
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

	emit end_datachange(this, iseg::ClearUndo);

	m_saveprojfilename = QString("");
	setCaption(QString(" iSeg ") + QString(xstr(ISEG_VERSION)) +
						 QString(" - No Filename"));
	m_notes->clear();

	reset_brightnesscontrast();
}

void MainWindow::start_surfaceviewer(int mode)
{
	// ensure we don't have a viewer running
	if (surface_viewer != nullptr)
	{
		surface_viewer->close();
		delete surface_viewer;
	}

	iseg::DataSelection dataSelection;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	emit begin_dataexport(dataSelection, this);

	surface_viewer = new SurfaceViewerWidget(handler3D, static_cast<SurfaceViewerWidget::eInputType>(mode), 0);
	QObject::connect(surface_viewer, SIGNAL(hasbeenclosed()), this, SLOT(surface_viewer_closed()));

	QObject::connect(surface_viewer,
			SIGNAL(begin_datachange(iseg::DataSelection&, QWidget*, bool)), this,
			SLOT(handle_begin_datachange(iseg::DataSelection&, QWidget*, bool)));
	QObject::connect(surface_viewer,
			SIGNAL(end_datachange(QWidget*, iseg::EndUndoAction)), this,
			SLOT(handle_end_datachange(QWidget*, iseg::EndUndoAction)));

	surface_viewer->show();
	surface_viewer->raise();

	emit end_dataexport(this);
}

void MainWindow::view_tissue(Point p)
{
	select_tissue(p, true);
	start_surfaceviewer(SurfaceViewerWidget::kSelectedTissues);
}

void MainWindow::execute_tissue_surfaceviewer()
{
	start_surfaceviewer(SurfaceViewerWidget::kTissues);
}

void MainWindow::execute_selectedtissue_surfaceviewer()
{
	start_surfaceviewer(SurfaceViewerWidget::kSelectedTissues);
}

void MainWindow::execute_source_surfaceviewer()
{
	start_surfaceviewer(SurfaceViewerWidget::kSource);
}

void MainWindow::execute_target_surfaceviewer()
{
	start_surfaceviewer(SurfaceViewerWidget::kTarget);
}

void MainWindow::execute_3Dvolumeviewertissue()
{
	iseg::DataSelection dataSelection;
	dataSelection.tissues = true;
	emit begin_dataexport(dataSelection, this);

	if (VV3D == nullptr)
	{
		VV3D = new VolumeViewerWidget(handler3D, false, true, true, 0);
		QObject::connect(VV3D, SIGNAL(hasbeenclosed()), this, SLOT(VV3D_closed()));
	}

	VV3D->show();
	VV3D->raise();

	emit end_dataexport(this);
}

void MainWindow::execute_3Dvolumeviewerbmp()
{
	iseg::DataSelection dataSelection;
	dataSelection.bmp = true;
	emit begin_dataexport(dataSelection, this);

	if (VV3Dbmp == nullptr)
	{
		VV3Dbmp = new VolumeViewerWidget(handler3D, true, true, true, 0);
		QObject::connect(VV3Dbmp, SIGNAL(hasbeenclosed()), this, SLOT(VV3Dbmp_closed()));
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
	if (xsliceshower != nullptr)
	{
		xsliceshower->bmp_changed();
	}
	if (ysliceshower != nullptr)
	{
		ysliceshower->bmp_changed();
	}
}

void MainWindow::update_work()
{
	work_show->update();

	if (xsliceshower != nullptr)
	{
		xsliceshower->work_changed();
	}
	if (ysliceshower != nullptr)
	{
		ysliceshower->work_changed();
	}
}

void MainWindow::update_tissue()
{
	bmp_show->tissue_changed();
	work_show->tissue_changed();
	if (xsliceshower != nullptr)
		xsliceshower->tissue_changed();
	if (ysliceshower != nullptr)
		ysliceshower->tissue_changed();
	if (VV3D != nullptr)
		VV3D->tissue_changed();
	if (surface_viewer != nullptr)
		surface_viewer->tissue_changed();
}

void MainWindow::xslice_closed()
{
	if (xsliceshower != nullptr)
	{
		if (ysliceshower != nullptr)
		{
			ysliceshower->xyexists_changed(false);
		}
		bmp_show->set_crosshairyvisible(false);
		work_show->set_crosshairyvisible(false);

		delete xsliceshower;
		xsliceshower = nullptr;
	}
}

void MainWindow::yslice_closed()
{
	if (ysliceshower != nullptr)
	{
		if (xsliceshower != nullptr)
		{
			xsliceshower->xyexists_changed(false);
		}
		bmp_show->set_crosshairxvisible(false);
		work_show->set_crosshairxvisible(false);

		delete ysliceshower;
		ysliceshower = nullptr;
	}
}

void MainWindow::surface_viewer_closed()
{
	if (surface_viewer != nullptr)
	{
		delete surface_viewer;
		surface_viewer = nullptr;
	}
}

void MainWindow::VV3D_closed()
{
	if (VV3D != nullptr)
	{
		delete VV3D;
		VV3D = nullptr;
	}
}

void MainWindow::VV3Dbmp_closed()
{
	if (VV3Dbmp != nullptr)
	{
		delete VV3Dbmp;
		VV3Dbmp = nullptr;
	}
}

void MainWindow::xshower_slicechanged()
{
	if (ysliceshower != nullptr)
	{
		ysliceshower->xypos_changed(xsliceshower->get_slicenr());
	}
	bmp_show->crosshairy_changed(xsliceshower->get_slicenr());
	work_show->crosshairy_changed(xsliceshower->get_slicenr());
}

void MainWindow::yshower_slicechanged()
{
	if (xsliceshower != nullptr)
	{
		xsliceshower->xypos_changed(ysliceshower->get_slicenr());
	}
	bmp_show->crosshairx_changed(ysliceshower->get_slicenr());
	work_show->crosshairx_changed(ysliceshower->get_slicenr());
}

void MainWindow::setWorkContentsPos(int x, int y)
{
	if (tomove_scroller)
	{
		tomove_scroller = false;
		work_scroller->setContentsPos(x, y);
	}
	else
	{
		tomove_scroller = true;
	}
}

void MainWindow::setBmpContentsPos(int x, int y)
{
	if (tomove_scroller)
	{
		tomove_scroller = false;
		bmp_scroller->setContentsPos(x, y);
	}
	else
	{
		tomove_scroller = true;
	}
}

void MainWindow::execute_histo()
{
	iseg::DataSelection dataSelection;
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
			&SW, SIGNAL(begin_datachange(iseg::DataSelection&, QWidget*, bool)),
			this,
			SLOT(handle_begin_datachange(iseg::DataSelection&, QWidget*, bool)));
	QObject::connect(&SW, SIGNAL(end_datachange(QWidget*, iseg::EndUndoAction)),
			this,
			SLOT(handle_end_datachange(QWidget*, iseg::EndUndoAction)));
	SW.move(QCursor::pos());
	SW.exec();
	QObject::disconnect(
			&SW, SIGNAL(begin_datachange(iseg::DataSelection&, QWidget*, bool)),
			this,
			SLOT(handle_begin_datachange(iseg::DataSelection&, QWidget*, bool)));
	QObject::disconnect(
			&SW, SIGNAL(end_datachange(QWidget*, iseg::EndUndoAction)), this,
			SLOT(handle_end_datachange(QWidget*, iseg::EndUndoAction)));

	return;
}

void MainWindow::execute_imagemath()
{
	ImageMath IM(handler3D, this);
	QObject::connect(
			&IM, SIGNAL(begin_datachange(iseg::DataSelection&, QWidget*, bool)),
			this,
			SLOT(handle_begin_datachange(iseg::DataSelection&, QWidget*, bool)));
	QObject::connect(&IM, SIGNAL(end_datachange(QWidget*, iseg::EndUndoAction)),
			this,
			SLOT(handle_end_datachange(QWidget*, iseg::EndUndoAction)));
	IM.move(QCursor::pos());
	IM.exec();
	QObject::disconnect(
			&IM, SIGNAL(begin_datachange(iseg::DataSelection&, QWidget*, bool)),
			this,
			SLOT(handle_begin_datachange(iseg::DataSelection&, QWidget*, bool)));
	QObject::disconnect(
			&IM, SIGNAL(end_datachange(QWidget*, iseg::EndUndoAction)), this,
			SLOT(handle_end_datachange(QWidget*, iseg::EndUndoAction)));

	return;
}

void MainWindow::execute_unwrap()
{
	iseg::DataSelection dataSelection;
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
			&IO, SIGNAL(begin_datachange(iseg::DataSelection&, QWidget*, bool)),
			this,
			SLOT(handle_begin_datachange(iseg::DataSelection&, QWidget*, bool)));
	QObject::connect(&IO, SIGNAL(end_datachange(QWidget*, iseg::EndUndoAction)),
			this,
			SLOT(handle_end_datachange(QWidget*, iseg::EndUndoAction)));
	IO.move(QCursor::pos());
	IO.exec();
	QObject::disconnect(
			&IO, SIGNAL(begin_datachange(iseg::DataSelection&, QWidget*, bool)),
			this,
			SLOT(handle_begin_datachange(iseg::DataSelection&, QWidget*, bool)));
	QObject::disconnect(
			&IO, SIGNAL(end_datachange(QWidget*, iseg::EndUndoAction)), this,
			SLOT(handle_end_datachange(QWidget*, iseg::EndUndoAction)));
}

void MainWindow::execute_pixelsize()
{
	PixelResize PR(handler3D, this);
	PR.move(QCursor::pos());
	if (PR.exec())
	{
		auto p = PR.get_pixelsize();
		if (!(p == handler3D->spacing()))
		{
			handler3D->set_pixelsize(p.x, p.y);
			handler3D->set_slicethickness(p.z);
			pixelsize_changed();
			slicethickness_changed();
		}
	}
}

void MainWindow::pixelsize_changed()
{
	Pair p = handler3D->get_pixelsize();
	if (xsliceshower != nullptr)
		xsliceshower->pixelsize_changed(p);
	if (ysliceshower != nullptr)
		ysliceshower->pixelsize_changed(p);
	bmp_show->pixelsize_changed(p);
	work_show->pixelsize_changed(p);
	if (VV3D != nullptr)
	{
		VV3D->pixelsize_changed(p);
	}
	if (VV3Dbmp != nullptr)
	{
		VV3Dbmp->pixelsize_changed(p);
	}
	if (surface_viewer != nullptr)
	{
		surface_viewer->pixelsize_changed(p);
	}
}

void MainWindow::execute_displacement()
{
	DisplacementDialog DD(handler3D, this);
	DD.move(QCursor::pos());
	if (DD.exec())
	{
		float disp[3];
		DD.return_displacement(disp);
		handler3D->set_displacement(disp);
	}
}

void MainWindow::execute_rotation()
{
	RotationDialog RD(handler3D, this);
	RD.move(QCursor::pos());
	if (RD.exec())
	{
		float rot[3][3];
		RD.get_rotation(rot);

		auto tr = handler3D->transform();
		tr.setRotation(rot);
		handler3D->set_transform(tr);
	}
}

void MainWindow::execute_undoconf()
{
	UndoConfigurationDialog UC(handler3D, this);
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
	ActiveSlicesConfigWidget AC(handler3D, this);
	AC.move(QCursor::pos());
	AC.exec();

	unsigned short slicenr = handler3D->active_slice() + 1;
	if (handler3D->start_slice() >= slicenr ||
			handler3D->end_slice() + 1 <= slicenr)
	{
		lb_inactivewarning->setText(QString("   3D Inactive Slice!   "));
	}
	else
	{
		lb_inactivewarning->setText(QString(" "));
	}
}

void MainWindow::execute_hideparameters(bool checked)
{
	WidgetInterface::set_hideparams(checked);
	if (tab_old != nullptr)
		tab_old->hideparams_changed();
}

void MainWindow::execute_hidezoom(bool checked)
{
	if (!checked)
	{
		zoom_widget->hide();
	}
	else
	{
		zoom_widget->show();
	}
}

void MainWindow::execute_hidecontrastbright(bool checked)
{
	if (!checked)
	{
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
	else
	{
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
	if (!checked)
	{
		vboxbmpw->hide();
	}
	else
	{
		vboxbmpw->show();
	}
}

void MainWindow::execute_hidetarget(bool checked)
{
	if (!checked)
	{
		vboxworkw->hide();
	}
	else
	{
		vboxworkw->show();
	}
}

void MainWindow::execute_hidecopyswap(bool checked)
{
	if (!checked)
	{
		toworkBtn->hide();
		tobmpBtn->hide();
		swapBtn->hide();
		swapAllBtn->hide();
	}
	else
	{
		toworkBtn->show();
		tobmpBtn->show();
		swapBtn->show();
		swapAllBtn->show();
	}
}

void MainWindow::execute_hidestack(bool checked)
{
	if (!checked)
	{
		bitstack_widget->hide();
	}
	else
	{
		bitstack_widget->show();
	}
}

void MainWindow::execute_hideoverlay(bool checked)
{
	if (!checked)
	{
		overlay_widget->hide();
	}
	else
	{
		overlay_widget->show();
	}
}

void MainWindow::execute_multidataset(bool checked)
{
	if (!checked)
	{
		multidataset_widget->hide();
	}
	else
	{
		multidataset_widget->show();
	}
}

void MainWindow::execute_hidenotes(bool checked)
{
	if (!checked)
	{
		m_notes->hide();
	}
	else
	{
		m_notes->show();
	}
}

void MainWindow::execute_showtabtoggled(bool)
{
	auto nrtabbuttons = (unsigned short)tabwidgets.size();
	for (unsigned short i = 0; i < nrtabbuttons; i++)
	{
		showpb_tab[i] = showtab_action[i]->isOn();
	}

	WidgetInterface* currentwidget = static_cast<WidgetInterface*>(methodTab->currentWidget());
	unsigned short i = 0;
	while ((i < nrtabbuttons) && (currentwidget != tabwidgets[i]))
		i++;
	if (i == nrtabbuttons)
	{
		updateTabvisibility();
		return;
	}

	if (!showpb_tab[i])
	{
		i = 0;
		while ((i < nrtabbuttons) && !showpb_tab[i])
			i++;
		if (i != nrtabbuttons)
		{
			methodTab->setCurrentWidget(tabwidgets[i]);
		}
	}
	updateTabvisibility();
}

void MainWindow::execute_xslice()
{
	iseg::DataSelection dataSelection;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	emit begin_dataexport(dataSelection, this);

	if (xsliceshower == nullptr)
	{
		xsliceshower = new SliceViewerWidget(
				handler3D, true, handler3D->get_slicethickness(),
				zoom_widget->get_zoom(), 0, 0, Qt::WStyle_StaysOnTop);
		xsliceshower->zpos_changed();
		if (ysliceshower != nullptr)
		{
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
	iseg::DataSelection dataSelection;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	emit begin_dataexport(dataSelection, this);

	if (ysliceshower == nullptr)
	{
		ysliceshower = new SliceViewerWidget(
				handler3D, false, handler3D->get_slicethickness(),
				zoom_widget->get_zoom(), 0, 0, Qt::WStyle_StaysOnTop);
		ysliceshower->zpos_changed();
		if (xsliceshower != nullptr)
		{
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
	QString filename = QFileDialog::getOpenFileName(QString::null,
			"Text (*.txt)\n"
			"All (*.*)",
			this);
	if (!filename.isEmpty())
	{
		std::vector<tissues_size_t> types;
		if (read_tissues(filename.ascii(), types))
		{
			// this actually goes through slices and removes it from segmentation
			handler3D->remove_tissues(
					std::set<tissues_size_t>(types.begin(), types.end()));

			tissueTreeWidget->update_tree_widget();
			tissuenr_changed(tissueTreeWidget->get_current_type() - 1);
		}
		else
		{
			QMessageBox::warning(this, "iSeg",
					"Error: not all tissues are in tissue list",
					QMessageBox::Ok | QMessageBox::Default);
		}
	}
}

void MainWindow::execute_grouptissues()
{
	std::vector<tissues_size_t> olds, news;

	QString filename = QFileDialog::getOpenFileName(QString::null,
			"Text (*.txt)\n"
			"All (*.*)",
			this);
	if (!filename.isEmpty())
	{
		bool fail_on_unknown_tissue = true;
		if (read_grouptissuescapped(filename.ascii(), olds, news,
						fail_on_unknown_tissue))
		{
			iseg::DataSelection dataSelection;
			dataSelection.allSlices = true;
			dataSelection.tissues = true;
			emit begin_datachange(dataSelection, this);

			handler3D->group_tissues(olds, news);

			emit end_datachange(this);
		}
		else
		{
			QMessageBox::warning(this, "iSeg",
					"Error: not all tissues are in tissue list",
					QMessageBox::Ok | QMessageBox::Default);
		}
	}
}

void MainWindow::execute_about()
{
	std::ostringstream ss;
	ss << "\n\niSeg\n"
		 << std::string(xstr(ISEG_DESCRIPTION));
	QMessageBox::about(this, "About", QString(ss.str().c_str()));
}

void MainWindow::add_mark(Point p)
{
	iseg::DataSelection dataSelection;
	dataSelection.sliceNr = handler3D->active_slice();
	dataSelection.marks = true;
	emit begin_datachange(dataSelection, this);

	handler3D->add_mark(p, tissueTreeWidget->get_current_type());

	emit end_datachange();
}

void MainWindow::add_label(Point p, std::string str)
{
	iseg::DataSelection dataSelection;
	dataSelection.sliceNr = handler3D->active_slice();
	dataSelection.marks = true;
	emit begin_datachange(dataSelection, this);

	handler3D->add_mark(p, tissueTreeWidget->get_current_type(), str);

	emit end_datachange(this);
}

void MainWindow::clear_marks()
{
	iseg::DataSelection dataSelection;
	dataSelection.sliceNr = handler3D->active_slice();
	dataSelection.marks = true;
	emit begin_datachange(dataSelection, this);

	handler3D->clear_marks();

	emit end_datachange(this);
}

void MainWindow::remove_mark(Point p)
{
	iseg::DataSelection dataSelection;
	dataSelection.sliceNr = handler3D->active_slice();
	dataSelection.marks = true;
	emit begin_datachange(dataSelection, this);

	if (handler3D->remove_mark(p, 3))
	{
		emit end_datachange(this);
	}
	else
	{
		emit end_datachange(this, iseg::AbortUndo);
	}
}

void MainWindow::select_tissue(Point p, bool clear_selection)
{
	auto const type = handler3D->get_tissue_pt(p, handler3D->active_slice());

	if (clear_selection)
	{
		QList<QTreeWidgetItem*> list = tissueTreeWidget->selectedItems();
		for (auto item : list)
		{
			// avoid unselecting if should be selected
			item->setSelected(tissueTreeWidget->get_type(item) == type);
		}
	}

	// remove filter if it is preventing tissue from being shown
	if (!tissueTreeWidget->is_visible(type))
	{
		tissueFilter->setText(QString(""));
	}
	// now select the tissue
	if (clear_selection && type > 0)
	{
		tissueTreeWidget->set_current_tissue(type);
	}
	else
	{
		for (auto item : tissueTreeWidget->get_all_items())
		{
			if (tissueTreeWidget->get_type(item) == type)
			{
				item->setSelected(true);
				if (auto p = item->parent())
				{
					p->setExpanded(true);
				}
			}
		}
	}
}

void MainWindow::next_featuring_slice()
{
	tissues_size_t type = tissueTreeWidget->get_current_type();
	if (type <= 0)
	{
		return;
	}

	bool found;
	unsigned short nextslice = handler3D->get_next_featuring_slice(type, found);
	if (found)
	{
		handler3D->set_active_slice(nextslice);
		slice_changed();
	}
	else
	{
		QMessageBox::information(this, "iSeg", "The selected tissue its empty.\n");
	}
}

void MainWindow::add_tissue(Point p)
{
	iseg::DataSelection dataSelection;
	dataSelection.sliceNr = handler3D->active_slice();
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this);

	tissues_size_t currTissueType = tissueTreeWidget->get_current_type();
	handler3D->add2tissue(currTissueType, p, cb_addsuboverride->isChecked());

	emit end_datachange(this);
}

void MainWindow::add_tissue_connected(Point p)
{
	iseg::DataSelection dataSelection;
	dataSelection.sliceNr = handler3D->active_slice();
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this);

	tissues_size_t currTissueType = tissueTreeWidget->get_current_type();
	handler3D->add2tissue_connected(currTissueType, p,
			cb_addsuboverride->isChecked());

	emit end_datachange(this);
}

void MainWindow::add_tissuelarger(Point p)
{
	iseg::DataSelection dataSelection;
	dataSelection.sliceNr = handler3D->active_slice();
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this);

	tissues_size_t currTissueType = tissueTreeWidget->get_current_type();
	handler3D->add2tissue_thresh(currTissueType, p);

	emit end_datachange(this);
}

void MainWindow::add_tissue_3D(Point p)
{
	iseg::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this);

	tissues_size_t currTissueType = tissueTreeWidget->get_current_type();
	handler3D->add2tissueall(currTissueType, p, cb_addsuboverride->isChecked());

	emit end_datachange(this);
}

void MainWindow::subtract_tissue(Point p)
{
	iseg::DataSelection dataSelection;
	dataSelection.sliceNr = handler3D->active_slice();
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
	iseg::DataSelection dataSelection;
	dataSelection.allSlices = cb_addsub3d->isChecked();
	dataSelection.sliceNr = handler3D->active_slice();
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this);

	tissues_size_t currTissueType = tissueTreeWidget->get_current_type();
	if (cb_addsub3d->isChecked())
	{
		QApplication::setOverrideCursor(QCursor(Qt::waitCursor));
		if (cb_addsubconn->isChecked())
			handler3D->add2tissueall_connected(currTissueType, p,
					cb_addsuboverride->isChecked());
		else
			handler3D->add2tissueall(currTissueType, p,
					cb_addsuboverride->isChecked());
		QApplication::restoreOverrideCursor();
	}
	else
	{
		if (cb_addsubconn->isChecked())
			handler3D->add2tissue_connected(currTissueType, p,
					cb_addsuboverride->isChecked());
		else
			handler3D->add2tissue(currTissueType, p, cb_addsuboverride->isChecked());
	}

	emit end_datachange(this);
}

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
	iseg::DataSelection dataSelection;
	dataSelection.allSlices = cb_addsub3d->isChecked();
	dataSelection.sliceNr = handler3D->active_slice();
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this);

	tissues_size_t currTissueType = tissueTreeWidget->get_current_type();
	if (cb_addsub3d->isChecked())
	{
		if (cb_addsubconn->isChecked())
			handler3D->subtract_tissueall_connected(currTissueType, p);
		else
			handler3D->subtract_tissueall(currTissueType, p);
	}
	else
	{
		if (cb_addsubconn->isChecked())
			handler3D->subtract_tissue_connected(currTissueType, p);
		else
			handler3D->subtract_tissue(currTissueType, p);
	}

	emit end_datachange(this);
}

void MainWindow::add_tissue_pushed()
{
	if (pb_sub->isOn())
	{
		QObject::disconnect(work_show, SIGNAL(mousepressed_sign(Point)), this,
				SLOT(subtract_tissue_clicked(Point)));
		pb_sub->setOn(false);
	}
	if (pb_subhold->isOn())
	{
		QObject::disconnect(work_show, SIGNAL(mousepressed_sign(Point)), this,
				SLOT(subtracthold_tissue_clicked(Point)));
		pb_subhold->setOn(false);
	}
	if (pb_addhold->isOn())
	{
		QObject::disconnect(work_show, SIGNAL(mousepressed_sign(Point)), this,
				SLOT(addhold_tissue_clicked(Point)));
		pb_addhold->setOn(false);
	}

	if (pb_add->isOn())
	{
		QObject::connect(work_show, SIGNAL(mousepressed_sign(Point)), this,
				SLOT(add_tissue_clicked(Point)));
		disconnect_mouseclick();
	}
	else
	{
		QObject::disconnect(work_show, SIGNAL(mousepressed_sign(Point)), this,
				SLOT(add_tissue_clicked(Point)));
		connect_mouseclick();
	}
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
	if (pb_sub->isOn())
	{
		QObject::disconnect(work_show, SIGNAL(mousepressed_sign(Point)), this,
				SLOT(subtract_tissue_clicked(Point)));
		pb_sub->setOn(false);
	}
	if (pb_subhold->isOn())
	{
		QObject::disconnect(work_show, SIGNAL(mousepressed_sign(Point)), this,
				SLOT(subtracthold_tissue_clicked(Point)));
		pb_subhold->setOn(false);
	}
	if (pb_add->isOn())
	{
		QObject::disconnect(work_show, SIGNAL(mousepressed_sign(Point)), this,
				SLOT(add_tissue_clicked(Point)));
		pb_add->setOn(false);
	}

	if (pb_addhold->isOn())
	{
		QObject::connect(work_show, SIGNAL(mousepressed_sign(Point)), this,
				SLOT(addhold_tissue_clicked(Point)));
		disconnect_mouseclick();
		//		pb_addhold->setDown(false);
	}
	else
	{
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
	if (pb_add->isOn())
	{
		QObject::disconnect(work_show, SIGNAL(mousepressed_sign(Point)), this,
				SLOT(add_tissue_clicked(Point)));
		pb_add->setOn(false);
	}
	if (pb_sub->isOn())
	{
		QObject::connect(work_show, SIGNAL(mousepressed_sign(Point)), this,
				SLOT(subtract_tissue_clicked(Point)));
		disconnect_mouseclick();
	}
	else
	{
		QObject::disconnect(work_show, SIGNAL(mousepressed_sign(Point)), this,
				SLOT(subtract_tissue_clicked(Point)));
		connect_mouseclick();
	}
	if (pb_subhold->isOn())
	{
		QObject::disconnect(work_show, SIGNAL(mousepressed_sign(Point)), this,
				SLOT(subtracthold_tissue_clicked(Point)));
		//		pb_subhold->setDown(false);
		pb_subhold->setOn(false);
	}
	if (pb_addhold->isOn())
	{
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
	if (pb_add->isOn())
	{
		QObject::disconnect(work_show, SIGNAL(mousepressed_sign(Point)), this,
				SLOT(add_tissue_clicked(Point)));
		pb_add->setOn(false);
	}
	if (pb_subhold->isOn())
	{
		QObject::connect(work_show, SIGNAL(mousepressed_sign(Point)), this,
				SLOT(subtracthold_tissue_clicked(Point)));
		disconnect_mouseclick();
	}
	else
	{
		QObject::disconnect(work_show, SIGNAL(mousepressed_sign(Point)), this,
				SLOT(subtracthold_tissue_clicked(Point)));
		connect_mouseclick();
	}
	if (pb_sub->isOn())
	{
		QObject::disconnect(work_show, SIGNAL(mousepressed_sign(Point)), this,
				SLOT(subtract_tissue_clicked(Point)));
		pb_sub->setOn(false);
	}
	if (pb_addhold->isOn())
	{
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
	if (pb_add->isDown())
	{
		QObject::disconnect(work_show, SIGNAL(mousepressed_sign(Point)), this,
				SLOT(add_tissue_clicked(Point)));
		pb_add->setDown(false);
	}
	if (pb_subhold->isOn())
	{
		QObject::disconnect(work_show, SIGNAL(mousepressed_sign(Point)), this,
				SLOT(subtracthold_tissue_clicked(Point)));
		pb_subhold->setOn(false);
	}
	if (pb_sub->isDown())
	{
		QObject::disconnect(work_show, SIGNAL(mousepressed_sign(Point)), this,
				SLOT(subtract_tissue_clicked(Point)));
		pb_sub->setDown(false);
	}
	if (pb_addhold->isOn())
	{
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
	iseg::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this);

	handler3D->work2tissueall();

	tissues_size_t m;
	handler3D->get_rangetissue(&m);
	handler3D->buildmissingtissues(m);
	tissueTreeWidget->update_tree_widget();

	tissues_size_t currTissueType = tissueTreeWidget->get_current_type();
	tissuenr_changed(currTissueType - 1);

	emit end_datachange(this);
}

void MainWindow::do_tissue2work()
{
	iseg::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.work = true;
	emit begin_datachange(dataSelection, this);

	handler3D->tissue2workall3D();

	emit end_datachange(this);
}

void MainWindow::do_work2tissue_grouped()
{
	std::vector<tissues_size_t> olds, news;

	QString filename = QFileDialog::getOpenFileName(QString::null,
			"Text (*.txt)\n"
			"All (*.*)",
			this);

	if (!filename.isEmpty())
	{
		if (read_grouptissues(filename.ascii(), olds, news))
		{
			iseg::DataSelection dataSelection;
			dataSelection.allSlices = true;
			dataSelection.tissues = true;
			emit begin_datachange(dataSelection, this);

			handler3D->work2tissueall();
			handler3D->group_tissues(olds, news);

			tissues_size_t m;
			handler3D->get_rangetissue(&m);
			handler3D->buildmissingtissues(m);
			tissueTreeWidget->update_tree_widget();

			tissues_size_t currTissueType = tissueTreeWidget->get_current_type();
			tissuenr_changed(currTissueType - 1); // TODO BL is this a bug? (I added the -1)

			emit end_datachange(this);
		}
	}
}

void MainWindow::randomize_colors()
{
	const float golden_ratio_conjugate = 0.618033988749895f;

	if (!tissueTreeWidget->selectedItems().empty())
	{
		static Color random = Color(0.1f, 0.9f, 0.1f);
		float h, s, l;
		for (auto item : tissueTreeWidget->selectedItems())
		{
			tissues_size_t label = tissueTreeWidget->get_type(item);

			std::tie(h, s, l) = random.toHSL();

			// See http://martin.ankerl.com/2009/12/09/how-to-create-random-colors-programmatically/
			h += golden_ratio_conjugate;
			h = std::fmod(h, 1.0f);

			random = Color::fromHSL(h, s, l);
			TissueInfos::SetTissueColor(label, random.r, random.g, random.b);
		}

		tissueTreeWidget->update_tissue_icons();
	}

	update_tissue();
}

void MainWindow::tissueFilterChanged(const QString& text)
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
	for (auto item : tissueTreeWidget->selectedItems())
	{
		if (TissueInfos::GetTissueLocked(tissueTreeWidget->get_type(item)))
		{
			QMessageBox::warning(this, "iSeg", "Locked tissues can not be merged.",
					QMessageBox::Ok | QMessageBox::Default);
			return;
		}
	}

	int ret = QMessageBox::warning(
			this, "iSeg", "Do you really want to merge the selected tissues?",
			QMessageBox::Yes | QMessageBox::Default, QMessageBox::No,
			QMessageBox::Cancel | QMessageBox::Escape);
	if (ret == QMessageBox::Yes)
	{
		bool option_3d_checked = tissue3Dopt->isChecked();
		tissue3Dopt->setChecked(true);
		selectedtissue2work();

		auto list = tissueTreeWidget->selectedItems();
		TissueAdder TA(false, tissueTreeWidget, this);
		TA.exec();

		tissues_size_t currTissueType = tissueTreeWidget->get_current_type();
		tissuenr_changed(currTissueType - 1);
		handler3D->mergetissues(currTissueType);

		removeselected(std::vector<QTreeWidgetItem*>(list.begin(), list.end()), false);
		tissue3Dopt->setChecked(option_3d_checked);
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
	if (tissueTreeWidget->get_current_is_folder())
	{
		modifFolder();
	}
	else
	{
		modifTissue();
	}
}

void MainWindow::modifTissue()
{
	iseg::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this, false);

	tissues_size_t nr = tissueTreeWidget->get_current_type() - 1;

	TissueAdder TA(true, tissueTreeWidget, this);
	TA.exec();

	emit end_datachange(this, iseg::ClearUndo);
}

void MainWindow::modifFolder()
{
	bool ok = false;
	QString newFolderName = QInputDialog::getText(
			"Folder name", "Enter a name for the folder:", QLineEdit::Normal,
			tissueTreeWidget->get_current_name(), &ok, this);
	if (ok)
	{
		tissueTreeWidget->set_current_folder_name(newFolderName);
	}
}

void MainWindow::removeTissueFolderPressed()
{
	if (tissueTreeWidget->get_current_is_folder())
	{
		if (tissueTreeWidget->get_current_has_children())
		{
			QMessageBox msgBox;
			msgBox.setText("Remove Folder");
			msgBox.setInformativeText("Do you want to keep all tissues\nand folders contained in this folder?");
			msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
			msgBox.setDefaultButton(QMessageBox::Yes);
			int ret = msgBox.exec();
			if (ret == QMessageBox::Yes)
			{
				tissueTreeWidget->remove_item(tissueTreeWidget->currentItem(), false);
			}
			else if (ret == QMessageBox::No)
			{
				removeselected();
			}
		}
		else
		{
			tissueTreeWidget->remove_item(tissueTreeWidget->currentItem(), false);
		}
	}
	else
	{
		removeselected();
	}
}

void MainWindow::removeselected()
{
	auto list = tissueTreeWidget->selectedItems();
	removeselected(std::vector<QTreeWidgetItem*>(list.begin(), list.end()), true);
}

void MainWindow::removeselected(const std::vector<QTreeWidgetItem*>& input, bool perform_checks)
{
	auto list = tissueTreeWidget->collect(input);
	std::vector<QTreeWidgetItem*> itemsToRemove;
	std::set<tissues_size_t> tissuesToRemove;
	if (perform_checks)
	{
		// check if any tissue is locked
		for (auto item : list)
		{
			if (TissueInfos::GetTissueLocked(tissueTreeWidget->get_type(item)))
			{
				QMessageBox::warning(this, "iSeg", "Locked tissues can not be removed.", QMessageBox::Ok | QMessageBox::Default);
				return;
			}
		}

		// confirm that user really intends to remove tissues/delete segmentation
		int ret = QMessageBox::warning(this, "iSeg", "Do you really want to remove the selected tissues?",
				QMessageBox::Yes | QMessageBox::Default, QMessageBox::Cancel | QMessageBox::Escape);
		if (ret != QMessageBox::Yes)
		{
			return; // Cancel
		}

		// check if any tissues are in the hierarchy more than once
		auto allItems = tissueTreeWidget->get_all_items();
		for (auto item : list)
		{
			itemsToRemove.push_back(item);

			tissues_size_t currTissueType = tissueTreeWidget->get_type(item);

			// if currTissueType==0 -> item is a folder
			bool removeCompletely = currTissueType != 0;
			if (removeCompletely && tissueTreeWidget->get_tissue_instance_count(currTissueType) > 1)
			{
				// There are more than one instance of the same tissue in the tree
				int ret = QMessageBox::question(this, "iSeg",
						"There are multiple occurrences\nof a selected tissue in the "
						"hierarchy.\nDo you want to remove them all?",
						QMessageBox::Yes | QMessageBox::Default, QMessageBox::No,
						QMessageBox::Cancel | QMessageBox::Escape);
				if (ret == QMessageBox::Yes)
				{
					removeCompletely = true;
				}
				else if (ret == QMessageBox::No)
				{
					removeCompletely = false;
				}
				else
				{
					return; // Cancel
				}
			}

			if (removeCompletely)
			{
				tissuesToRemove.insert(currTissueType);

				// find all tree items with that name and delete
				for (auto other : allItems)
				{
					if (other != item && tissueTreeWidget->get_type(other) == currTissueType)
					{
						itemsToRemove.push_back(other);
					}
				}
			}
		}
	}
	else
	{
		itemsToRemove = list;
		for (auto item : list)
		{
			auto currTissueType = tissueTreeWidget->get_type(item);
			if (currTissueType != 0)
			{
				tissuesToRemove.insert(currTissueType);
			}
		}
	}

	iseg::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this, false);

	handler3D->remove_tissues(tissuesToRemove);
	tissueTreeWidget->remove_items(itemsToRemove);

	emit end_datachange(this, iseg::ClearUndo);

	if (TissueInfos::GetTissueCount() == 0)
	{
		TissueInfo tissueInfo;
		tissueInfo.name = "Tissue1";
		tissueInfo.color = Color(1.0f, 0.0f, 0.1f);
		TissueInfos::AddTissue(tissueInfo);

		// Insert new tissue in hierarchy
		tissueTreeWidget->insert_item(false, ToQ(tissueInfo.name));
	}
	else
	{
		auto currTissueType = tissueTreeWidget->get_current_type();
		tissuenr_changed(currTissueType - 1);
	}
}

void MainWindow::removeTissueFolderAllPressed()
{
	// check if any tissue is locked
	for (auto item : tissueTreeWidget->get_all_items())
	{
		if (TissueInfos::GetTissueLocked(tissueTreeWidget->get_type(item)))
		{
			QMessageBox::warning(this, "iSeg", "Locked tissues can not be removed.", QMessageBox::Ok | QMessageBox::Default);
			return;
		}
	}

	// confirm that user really intends to remove tissues/delete segmentation
	int ret = QMessageBox::warning(
			this, "iSeg", "Do you really want to remove all tissues and folders?",
			QMessageBox::Yes | QMessageBox::Default, QMessageBox::No,
			QMessageBox::Cancel | QMessageBox::Escape);
	if (ret == QMessageBox::Yes)
	{
		iseg::DataSelection dataSelection;
		dataSelection.allSlices = true;
		dataSelection.tissues = true;
		emit begin_datachange(dataSelection, this, false);

		handler3D->remove_tissueall();
		tissueTreeWidget->remove_all_folders(false);
		tissueTreeWidget->update_tree_widget();
		tissues_size_t currTissueType = tissueTreeWidget->get_current_type();
		tissuenr_changed(currTissueType - 1);

		emit end_datachange(this, iseg::ClearUndo);
	}
}

// BL TODO replace by selectedtissue2work (SK)
void MainWindow::tissue2work()
{
	iseg::DataSelection dataSelection;
	dataSelection.allSlices = tissue3Dopt->isChecked();
	dataSelection.sliceNr = handler3D->active_slice();
	dataSelection.work = true;
	emit begin_datachange(dataSelection, this);

	std::vector<tissues_size_t> tissue_list;
	tissue_list.push_back(tissueTreeWidget->get_current_type());

	if (tissue3Dopt->isChecked())
	{
		handler3D->selectedtissue2work3D(tissue_list);
	}
	else
	{
		handler3D->selectedtissue2work(tissue_list);
	}

	emit end_datachange(this);
}

void MainWindow::selectedtissue2work()
{
	iseg::DataSelection dataSelection;
	dataSelection.allSlices = tissue3Dopt->isChecked();
	dataSelection.sliceNr = handler3D->active_slice();
	dataSelection.work = true;
	emit begin_datachange(dataSelection, this);

	handler3D->clear_work(); // resets work to 0.0f, then adds each tissue one-by-one

	std::vector<tissues_size_t> selected_tissues;
	for (auto item : tissueTreeWidget->selectedItems())
	{
		selected_tissues.push_back(tissueTreeWidget->get_type(item));
	}

	try
	{
		if (tissue3Dopt->isChecked())
		{
			handler3D->selectedtissue2work3D(selected_tissues);
		}
		else
		{
			handler3D->selectedtissue2work(selected_tissues);
		}
	}
	catch (std::exception&)
	{
		ISEG_ERROR_MSG("could not get tissue. Something might be wrong with tissue list.");
	}
	emit end_datachange(this);
}

void MainWindow::tissue2workall()
{
	iseg::DataSelection dataSelection;
	dataSelection.allSlices = tissue3Dopt->isChecked();
	dataSelection.sliceNr = handler3D->active_slice();
	dataSelection.work = true;
	emit begin_datachange(dataSelection, this);

	if (tissue3Dopt->isChecked())
	{
		handler3D->tissue2workall3D();
	}
	else
	{
		handler3D->tissue2workall();
	}

	emit end_datachange(this);
}

void MainWindow::cleartissues()
{
	// check if any tissue is locked
	for (auto item : tissueTreeWidget->get_all_items())
	{
		if (TissueInfos::GetTissueLocked(tissueTreeWidget->get_type(item)))
		{
			QMessageBox::warning(this, "iSeg", "Locked tissues can not be cleared.", QMessageBox::Ok | QMessageBox::Default);
			return;
		}
	}

	int ret = QMessageBox::warning(
			this, "iSeg", "Do you really want to clear all tissues?",
			QMessageBox::Yes | QMessageBox::Default, QMessageBox::No,
			QMessageBox::Cancel | QMessageBox::Escape);
	if (ret == QMessageBox::Yes)
	{
		iseg::DataSelection dataSelection;
		dataSelection.allSlices = tissue3Dopt->isChecked();
		dataSelection.sliceNr = handler3D->active_slice();
		dataSelection.tissues = true;
		emit begin_datachange(dataSelection, this);

		if (tissue3Dopt->isChecked())
		{
			handler3D->cleartissues3D();
		}
		else
		{
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
	if (isLocked)
	{
		QMessageBox::warning(this, "iSeg", "Locked tissue can not be removed.",
				QMessageBox::Ok | QMessageBox::Default);
		return;
	}

	int ret = QMessageBox::warning(
			this, "iSeg", "Do you really want to clear the tissue?",
			QMessageBox::Yes | QMessageBox::Default, QMessageBox::No,
			QMessageBox::Cancel | QMessageBox::Escape);
	if (ret == QMessageBox::Yes)
	{
		iseg::DataSelection dataSelection;
		dataSelection.allSlices = tissue3Dopt->isChecked();
		dataSelection.sliceNr = handler3D->active_slice();
		dataSelection.tissues = true;
		emit begin_datachange(dataSelection, this);

		tissues_size_t nr = tissueTreeWidget->get_current_type();
		if (tissue3Dopt->isChecked())
		{
			handler3D->cleartissue3D(nr);
		}
		else
		{
			handler3D->cleartissue(nr);
		}

		emit end_datachange(this);
	}
}

void MainWindow::clearselected()
{
	bool anyLocked = false;
	QList<QTreeWidgetItem*> list_ = tissueTreeWidget->selectedItems();
	for (auto a = list_.begin(); a != list_.end() && !anyLocked; ++a)
	{
		anyLocked |= TissueInfos::GetTissueLocked(tissueTreeWidget->get_type(*a));
	}
	if (anyLocked)
	{
		QMessageBox::warning(this, "iSeg", "Locked tissues can not be cleared.",
				QMessageBox::Ok | QMessageBox::Default);
		return;
	}

	int ret = QMessageBox::warning(
			this, "iSeg", "Do you really want to clear tissues?",
			QMessageBox::Yes | QMessageBox::Default, QMessageBox::No,
			QMessageBox::Cancel | QMessageBox::Escape);
	if (ret == QMessageBox::Yes)
	{
		iseg::DataSelection dataSelection;
		dataSelection.allSlices = tissue3Dopt->isChecked();
		dataSelection.sliceNr = handler3D->active_slice();
		dataSelection.tissues = true;
		emit begin_datachange(dataSelection, this);

		auto list = tissueTreeWidget->selectedItems();
		for (auto a = list.begin(); a != list.end(); ++a)
		{
			QTreeWidgetItem* item = *a;
			tissues_size_t nr = tissueTreeWidget->get_type(item);
			if (tissue3Dopt->isChecked())
			{
				handler3D->cleartissue3D(nr);
			}
			else
			{
				handler3D->cleartissue(nr);
			}
		}
		emit end_datachange(this);
	}
}

void MainWindow::bmptissuevisible_changed()
{
	if (cb_bmptissuevisible->isChecked())
	{
		bmp_show->set_tissuevisible(true);
	}
	else
	{
		bmp_show->set_tissuevisible(false);
	}
}

void MainWindow::bmpoutlinevisible_changed()
{
	if (cb_bmpoutlinevisible->isChecked())
	{
		bmp_show->set_workbordervisible(true);
	}
	else
	{
		bmp_show->set_workbordervisible(false);
	}
}

void MainWindow::worktissuevisible_changed()
{
	if (cb_worktissuevisible->isChecked())
	{
		work_show->set_tissuevisible(true);
	}
	else
	{
		work_show->set_tissuevisible(false);
	}
}

void MainWindow::workpicturevisible_changed()
{
	if (cb_workpicturevisible->isChecked())
	{
		work_show->set_picturevisible(true);
	}
	else
	{
		work_show->set_picturevisible(false);
	}
}

void MainWindow::slice_changed()
{
	WidgetInterface* qw = (WidgetInterface*)methodTab->currentWidget();
	qw->on_slicenr_changed();

	unsigned short slicenr = handler3D->active_slice() + 1;
	QObject::disconnect(scb_slicenr, SIGNAL(valueChanged(int)), this, SLOT(scb_slicenr_changed()));
	scb_slicenr->setValue(int(slicenr));
	QObject::connect(scb_slicenr, SIGNAL(valueChanged(int)), this, SLOT(scb_slicenr_changed()));
	QObject::disconnect(sb_slicenr, SIGNAL(valueChanged(int)), this, SLOT(sb_slicenr_changed()));
	sb_slicenr->setValue(int(slicenr));
	QObject::connect(sb_slicenr, SIGNAL(valueChanged(int)), this, SLOT(sb_slicenr_changed()));
	bmp_show->slicenr_changed();
	work_show->slicenr_changed();
	scale_dialog->slicenr_changed();
	imagemath_dialog->slicenr_changed();
	imageoverlay_dialog->slicenr_changed();
	overlay_widget->slicenr_changed();
	if (handler3D->start_slice() >= slicenr || handler3D->end_slice() + 1 <= slicenr)
	{
		lb_inactivewarning->setText(QString("   3D Inactive Slice!   "));
	}
	else
	{
		lb_inactivewarning->setText(QString(" "));
	}

	if (xsliceshower != nullptr)
	{
		xsliceshower->zpos_changed();
	}
	if (ysliceshower != nullptr)
	{
		ysliceshower->zpos_changed();
	}
}

void MainWindow::slices3d_changed(bool new_bitstack)
{
	if (new_bitstack)
	{
		bitstack_widget->newloaded();
	}
	overlay_widget->newloaded();
	imageoverlay_dialog->newloaded();

	for (size_t i = 0; i < tabwidgets.size(); i++)
	{
		((WidgetInterface*)(tabwidgets[i]))->newloaded();
	}

	WidgetInterface* qw = (WidgetInterface*)methodTab->currentWidget();

	if (handler3D->num_slices() != nrslices)
	{
		QObject::disconnect(scb_slicenr, SIGNAL(valueChanged(int)), this,
				SLOT(scb_slicenr_changed()));
		scb_slicenr->setMaxValue((int)handler3D->num_slices());
		QObject::connect(scb_slicenr, SIGNAL(valueChanged(int)), this,
				SLOT(scb_slicenr_changed()));
		QObject::disconnect(sb_slicenr, SIGNAL(valueChanged(int)), this,
				SLOT(sb_slicenr_changed()));
		sb_slicenr->setMaxValue((int)handler3D->num_slices());
		QObject::connect(sb_slicenr, SIGNAL(valueChanged(int)), this,
				SLOT(sb_slicenr_changed()));
		lb_slicenr->setText(QString(" of ") +
												QString::number((int)handler3D->num_slices()));
		interpolation_widget->handler3D_changed();
		nrslices = handler3D->num_slices();
	}

	unsigned short slicenr = handler3D->active_slice() + 1;
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
	if (handler3D->start_slice() >= slicenr ||
			handler3D->end_slice() + 1 <= slicenr)
	{
		lb_inactivewarning->setText(QString("   3D Inactive Slice!   "));
	}
	else
	{
		lb_inactivewarning->setText(QString(" "));
	}

	if (xsliceshower != nullptr)
		xsliceshower->zpos_changed();
	if (ysliceshower != nullptr)
		ysliceshower->zpos_changed();

	qw->init();
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

void MainWindow::slicenr_up()
{
	auto n = std::min(handler3D->active_slice() + sb_stride->value(), handler3D->num_slices() - 1);
	handler3D->set_active_slice(n);
	slice_changed();
}

void MainWindow::slicenr_down()
{
	auto n = std::max(static_cast<int>(handler3D->active_slice()) - sb_stride->value(), 0);
	handler3D->set_active_slice(n);
	slice_changed();
}

void MainWindow::sb_slicenr_changed()
{
	handler3D->set_active_slice(sb_slicenr->value() - 1);
	slice_changed();
}

void MainWindow::scb_slicenr_changed()
{
	handler3D->set_active_slice(scb_slicenr->value() - 1);
	slice_changed();
}

void MainWindow::sb_stride_changed()
{
	sb_slicenr->setSingleStep(sb_stride->value());
	scb_slicenr->setSingleStep(sb_stride->value());
}

void MainWindow::pb_first_pressed()
{
	handler3D->set_active_slice(0);
	slice_changed();
}

void MainWindow::pb_last_pressed()
{
	handler3D->set_active_slice(handler3D->num_slices() - 1);
	slice_changed();
}

void MainWindow::slicethickness_changed()
{
	float thickness = handler3D->get_slicethickness();
	if (xsliceshower != nullptr)
		xsliceshower->thickness_changed(thickness);
	if (ysliceshower != nullptr)
		ysliceshower->thickness_changed(thickness);
	if (VV3D != nullptr)
		VV3D->thickness_changed(thickness);
	if (VV3Dbmp != nullptr)
		VV3Dbmp->thickness_changed(thickness);
	if (surface_viewer != nullptr)
		surface_viewer->thickness_changed(thickness);
}

void MainWindow::tissuenr_changed(int i)
{
	QWidget* qw = methodTab->currentWidget();
	if (auto tool = dynamic_cast<WidgetInterface*>(qw))
	{
		tool->on_tissuenr_changed(i);
	}

	bmp_show->color_changed(i);
	work_show->color_changed(i);
	bitstack_widget->tissuenr_changed(i);

	cb_tissuelock->setChecked(TissueInfos::GetTissueLocked(i + 1));
}

void MainWindow::tissue_selection_changed()
{
	QList<QTreeWidgetItem*> list = tissueTreeWidget->selectedItems();
	if (list.size() > 1)
	{
		//showpb_tab[6] = false;
		cb_bmpoutlinevisible->setChecked(false);
		bmpoutlinevisible_changed();
		updateTabvisibility();
	}
	else
	{
		//showpb_tab[6] = true;
		//showpb_tab[14] = true;
		updateTabvisibility();
	}

	tissuesDock->setWindowTitle(QString("Tissues (%1)").arg(list.size()));

	if (list.size() > 0)
	{
		tissuenr_changed((int)tissueTreeWidget->get_current_type() - 1);
	}

	std::set<tissues_size_t> sel;
	for (auto a : list)
	{
		sel.insert(tissueTreeWidget->get_type(a));
	}
	TissueInfos::SetSelectedTissues(sel);
}

void MainWindow::unselectall()
{
	QList<QTreeWidgetItem*> list = tissueTreeWidget->selectedItems();

	for (auto a = list.begin(); a != list.end(); ++a)
	{
		QTreeWidgetItem* item = *a;
		item->setSelected(false);
	}
}

void MainWindow::tree_widget_doubleclicked(QTreeWidgetItem* item, int column)
{
	modifTissueFolderPressed();
}

void MainWindow::tree_widget_contextmenu(const QPoint& pos)
{
	QList<QTreeWidgetItem*> list = tissueTreeWidget->selectedItems();

	Q3PopupMenu contextMenu(tissueTreeWidget, "tissuetreemenu");
	if (list.size() <= 1) // single selection
	{
		if (tissueTreeWidget->get_current_is_folder())
		{
			contextMenu.insertItem("Toggle Lock", cb_tissuelock, SLOT(click()));
			contextMenu.insertSeparator();
			contextMenu.insertItem("New Tissue...", this, SLOT(newTissuePressed()));
			contextMenu.insertItem("New Folder...", this, SLOT(newFolderPressed()));
			contextMenu.insertItem("Mod. Folder...", this, SLOT(modifTissueFolderPressed()));
			contextMenu.insertItem("Del. Folder...", this, SLOT(removeTissueFolderPressed()));
		}
		else
		{
			contextMenu.insertItem("Toggle Lock", cb_tissuelock, SLOT(click()));
			contextMenu.insertSeparator();
			contextMenu.insertItem("New Tissue...", this, SLOT(newTissuePressed()));
			contextMenu.insertItem("New Folder...", this, SLOT(newFolderPressed()));
			contextMenu.insertItem("Mod. Tissue...", this, SLOT(modifTissueFolderPressed()));
			contextMenu.insertItem("Del. Tissue...", this, SLOT(removeTissueFolderPressed()));
			contextMenu.insertSeparator();
			contextMenu.insertItem("Get Tissue", this, SLOT(tissue2work()));
			contextMenu.insertItem("Clear Tissue", this, SLOT(cleartissue()));
			contextMenu.insertSeparator();
			contextMenu.insertItem("Next Feat. Slice", this, SLOT(next_featuring_slice()));
		}
	}
	else // multi-selection
	{
		contextMenu.insertItem("Toggle Lock", cb_tissuelock, SLOT(click()));
		contextMenu.insertSeparator();
		contextMenu.insertItem("Delete Selected", this, SLOT(removeselected()));
		contextMenu.insertItem("Clear Selected", this, SLOT(clearselected()));
		contextMenu.insertItem("Get Selected", this, SLOT(selectedtissue2work()));
		contextMenu.insertItem("Merge", this, SLOT(merge()));
	}

	if (list.size() > 0)
	{
		contextMenu.insertItem("Unselect All", this, SLOT(unselectall()));
		contextMenu.insertItem("Assign Random Colors", this, SLOT(randomize_colors()));
		contextMenu.insertItem("View Tissue Surface", this, SLOT(execute_selectedtissue_surfaceviewer()));
	}
	contextMenu.insertSeparator();
	int itemId = contextMenu.insertItem("Show Tissue Indices", tissueTreeWidget, SLOT(toggle_show_tissue_indices()));
	contextMenu.setItemChecked(itemId, !tissueTreeWidget->get_tissue_indices_hidden());
	contextMenu.insertItem("Sort By Name", tissueTreeWidget, SLOT(sort_by_tissue_name()));
	contextMenu.insertItem("Sort By Index", tissueTreeWidget, SLOT(sort_by_tissue_index()));
	contextMenu.exec(tissueTreeWidget->viewport()->mapToGlobal(pos));
}

void MainWindow::tissuelock_toggled()
{
	if (tissueTreeWidget->get_current_is_folder())
	{
		// Set lock state of all child tissues
		std::map<tissues_size_t, unsigned short> childTissues;
		tissueTreeWidget->get_current_child_tissues(childTissues);
		for (std::map<tissues_size_t, unsigned short>::iterator iter =
						 childTissues.begin();
				 iter != childTissues.end(); ++iter)
		{
			TissueInfos::SetTissueLocked(iter->first, cb_tissuelock->isChecked());
		}
	}
	else
	{
		QList<QTreeWidgetItem*> list;
		list = tissueTreeWidget->selectedItems();
		for (auto a = list.begin(); a != list.end(); ++a)
		{
			QTreeWidgetItem* item = *a;
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
	if (handler3D->return_nrundo() > 0)
	{
		iseg::DataSelection selectedData;
		if (methodTab->currentWidget() == transform_widget)
		{
			cancel_transform_helper();
			selectedData = handler3D->undo();
			transform_widget->init();
		}
		else
		{
			selectedData = handler3D->undo();
		}

		// Update ranges
		update_ranges_helper();

		//	if(undotype & )
		slice_changed();

		if (selectedData.DataSelected())
		{
			editmenu->setItemEnabled(redonr, true);
			if (handler3D->return_nrundo() == 0)
				editmenu->setItemEnabled(undonr, false);
		}
	}
}

void MainWindow::execute_redo()
{
	iseg::DataSelection selectedData;
	if (methodTab->currentWidget() == transform_widget)
	{
		cancel_transform_helper();
		selectedData = handler3D->redo();
		transform_widget->init();
	}
	else
	{
		selectedData = handler3D->redo();
	}

	// Update ranges
	update_ranges_helper();

	//	if(undotype & )
	slice_changed();

	if (selectedData.DataSelected())
	{
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

void MainWindow::tab_changed(int idx)
{
	QWidget* qw = methodTab->widget(idx);
	if (qw != tab_old)
	{
		ISEG_INFO("Starting widget: " << qw->metaObject()->className());

		// disconnect signal-slots of previous widget

		if (tab_old) // generic slots from WidgetInterface
		{
			tab_old->cleanup();

			// always connect to bmp_show, but not always to work_show
			QObject::disconnect(tab_old, SIGNAL(vm_changed(std::vector<Mark>*)), bmp_show,
					SLOT(set_vm(std::vector<Mark>*)));
			QObject::disconnect(tab_old, SIGNAL(vpdyn_changed(std::vector<Point>*)), bmp_show,
					SLOT(set_vpdyn(std::vector<Point>*)));

			QObject::disconnect(bmp_show, SIGNAL(mousepressed_sign(Point)), tab_old,
					SLOT(mouse_clicked(Point)));
			QObject::disconnect(bmp_show, SIGNAL(mousemoved_sign(Point)), tab_old,
					SLOT(mouse_moved(Point)));
			QObject::disconnect(bmp_show, SIGNAL(mousereleased_sign(Point)), tab_old,
					SLOT(mouse_released(Point)));
			QObject::disconnect(work_show, SIGNAL(mousepressed_sign(Point)), tab_old,
					SLOT(mouse_clicked(Point)));
			QObject::disconnect(work_show, SIGNAL(mousemoved_sign(Point)), tab_old,
					SLOT(mouse_moved(Point)));
			QObject::disconnect(work_show, SIGNAL(mousereleased_sign(Point)), tab_old,
					SLOT(mouse_released(Point)));
		}

		if (tab_old == threshold_widget)
		{
			QObject::disconnect(this, SIGNAL(bmp_changed()), threshold_widget,
					SLOT(bmp_changed()));
		}
		else if (tab_old == measurement_widget)
		{
			QObject::disconnect(this, SIGNAL(marks_changed()), measurement_widget,
					SLOT(marks_changed()));
			QObject::disconnect(measurement_widget,
					SIGNAL(vp1_changed(std::vector<Point>*)), bmp_show,
					SLOT(set_vp1(std::vector<Point>*)));
			QObject::disconnect(measurement_widget,
					SIGNAL(vp1_changed(std::vector<Point>*)), work_show,
					SLOT(set_vp1(std::vector<Point>*)));
			QObject::disconnect(measurement_widget,
					SIGNAL(vpdyn_changed(std::vector<Point>*)), work_show,
					SLOT(set_vpdyn(std::vector<Point>*)));
			QObject::disconnect(measurement_widget,
					SIGNAL(vp1dyn_changed(std::vector<Point>*, std::vector<Point>*, bool)),
					bmp_show, SLOT(set_vp1_dyn(std::vector<Point>*, std::vector<Point>*, bool)));
			QObject::disconnect(measurement_widget,
					SIGNAL(vp1dyn_changed(std::vector<Point>*, std::vector<Point>*, bool)),
					work_show, SLOT(set_vp1_dyn(std::vector<Point>*, std::vector<Point>*, bool)));

			work_show->setMouseTracking(false);
			bmp_show->setMouseTracking(false);
		}
		else if (tab_old == vesselextr_widget)
		{
			QObject::disconnect(this, SIGNAL(marks_changed()), measurement_widget,
					SLOT(marks_changed()));
			QObject::disconnect(vesselextr_widget,
					SIGNAL(vp1_changed(std::vector<Point>*)), bmp_show,
					SLOT(set_vp1(std::vector<Point>*)));
		}
		else if (tab_old == watershed_widget)
		{
			QObject::disconnect(this, SIGNAL(marks_changed()), watershed_widget,
					SLOT(marks_changed()));
		}
		else if (tab_old == hyst_widget)
		{
			QObject::disconnect(this, SIGNAL(bmp_changed()), hyst_widget,
					SLOT(bmp_changed()));

			QObject::disconnect(hyst_widget,
					SIGNAL(vp1_changed(std::vector<Point>*)), bmp_show,
					SLOT(set_vp1(std::vector<Point>*)));
			QObject::disconnect(hyst_widget,
					SIGNAL(vp1dyn_changed(std::vector<Point>*, std::vector<Point>*)), bmp_show,
					SLOT(set_vp1_dyn(std::vector<Point>*, std::vector<Point>*)));
		}
		else if (tab_old == livewire_widget)
		{
			QObject::disconnect(this, SIGNAL(bmp_changed()), livewire_widget,
					SLOT(bmp_changed()));
			QObject::disconnect(bmp_show, SIGNAL(mousedoubleclick_sign(Point)),
					livewire_widget, SLOT(pt_doubleclicked(Point)));
			QObject::disconnect(bmp_show, SIGNAL(mousepressedmid_sign(Point)),
					livewire_widget, SLOT(pt_midclicked(Point)));
			QObject::disconnect(bmp_show, SIGNAL(mousedoubleclickmid_sign(Point)),
					livewire_widget, SLOT(pt_doubleclickedmid(Point)));
			QObject::disconnect(livewire_widget, SIGNAL(vp1_changed(std::vector<Point>*)),
					bmp_show, SLOT(set_vp1(std::vector<Point>*)));
			QObject::disconnect(livewire_widget,
					SIGNAL(vp1dyn_changed(std::vector<Point>*, std::vector<Point>*)), bmp_show,
					SLOT(set_vp1_dyn(std::vector<Point>*, std::vector<Point>*)));

			bmp_show->setMouseTracking(false);
		}
		else if (tab_old == iftrg_widget)
		{
			QObject::disconnect(this, SIGNAL(bmp_changed()), iftrg_widget,
					SLOT(bmp_changed()));
		}
		else if (tab_old == fastmarching_widget)
		{
			QObject::disconnect(this, SIGNAL(bmp_changed()), fastmarching_widget,
					SLOT(bmp_changed()));
		}
		else if (tab_old == olc_widget)
		{
			QObject::disconnect(this, SIGNAL(work_changed()), olc_widget,
					SLOT(workbits_changed()));

			QObject::disconnect(olc_widget,
					SIGNAL(vpdyn_changed(std::vector<Point>*)), work_show,
					SLOT(set_vpdyn(std::vector<Point>*)));
		}
		else if (tab_old == feature_widget)
		{
			QObject::disconnect(feature_widget,
					SIGNAL(vpdyn_changed(std::vector<Point>*)), work_show,
					SLOT(set_vpdyn(std::vector<Point>*)));

			work_show->setMouseTracking(false);
			bmp_show->setMouseTracking(false);
		}
		else if (tab_old == interpolation_widget)
		{
			// nothing
		}
		else if (tab_old == picker_widget)
		{
			QObject::disconnect(picker_widget,
					SIGNAL(vp1_changed(std::vector<Point>*)), bmp_show,
					SLOT(set_vp1(std::vector<Point>*)));
			QObject::disconnect(picker_widget,
					SIGNAL(vp1_changed(std::vector<Point>*)), work_show,
					SLOT(set_vp1(std::vector<Point>*)));
		}
		else if (tab_old == transform_widget)
		{
			QObject::disconnect(this, SIGNAL(bmp_changed()), transform_widget,
					SLOT(bmp_changed()));
			QObject::disconnect(this, SIGNAL(work_changed()), transform_widget,
					SLOT(work_changed()));
			QObject::disconnect(this, SIGNAL(tissues_changed()), transform_widget,
					SLOT(tissues_changed()));
		}

		// connect signal-slots new widget

		if (auto widget = dynamic_cast<WidgetInterface*>(qw)) // generic slots from WidgetInterface
		{
			QObject::connect(widget, SIGNAL(vm_changed(std::vector<Mark>*)), bmp_show,
					SLOT(set_vm(std::vector<Mark>*)));
			QObject::connect(widget, SIGNAL(vpdyn_changed(std::vector<Point>*)), bmp_show,
					SLOT(set_vpdyn(std::vector<Point>*)));

			QObject::connect(bmp_show, SIGNAL(mousepressed_sign(Point)), widget,
					SLOT(mouse_clicked(Point)));
			QObject::connect(bmp_show, SIGNAL(mousemoved_sign(Point)), widget,
					SLOT(mouse_moved(Point)));
			QObject::connect(bmp_show, SIGNAL(mousereleased_sign(Point)), widget,
					SLOT(mouse_released(Point)));
			QObject::connect(work_show, SIGNAL(mousepressed_sign(Point)), widget,
					SLOT(mouse_clicked(Point)));
			QObject::connect(work_show, SIGNAL(mousemoved_sign(Point)), widget,
					SLOT(mouse_moved(Point)));
			QObject::connect(work_show, SIGNAL(mousereleased_sign(Point)), widget,
					SLOT(mouse_released(Point)));
		}

		if (qw == threshold_widget)
		{
			QObject::connect(this, SIGNAL(bmp_changed()), threshold_widget,
					SLOT(bmp_changed()));
		}
		else if (qw == measurement_widget)
		{
			QObject::connect(this, SIGNAL(marks_changed()), measurement_widget,
					SLOT(marks_changed()));
			QObject::connect(measurement_widget,
					SIGNAL(vp1_changed(std::vector<Point>*)), bmp_show,
					SLOT(set_vp1(std::vector<Point>*)));
			QObject::connect(measurement_widget,
					SIGNAL(vp1_changed(std::vector<Point>*)), work_show,
					SLOT(set_vp1(std::vector<Point>*)));
			QObject::connect(measurement_widget,
					SIGNAL(vpdyn_changed(std::vector<Point>*)), work_show,
					SLOT(set_vpdyn(std::vector<Point>*)));
			QObject::connect(measurement_widget,
					SIGNAL(vp1dyn_changed(std::vector<Point>*, std::vector<Point>*, bool)),
					bmp_show, SLOT(set_vp1_dyn(std::vector<Point>*, std::vector<Point>*, bool)));
			QObject::connect(measurement_widget,
					SIGNAL(vp1dyn_changed(std::vector<Point>*, std::vector<Point>*, bool)),
					work_show, SLOT(set_vp1_dyn(std::vector<Point>*, std::vector<Point>*, bool)));

			work_show->setMouseTracking(true);
			bmp_show->setMouseTracking(true);
		}
		else if (qw == vesselextr_widget)
		{
			QObject::connect(this, SIGNAL(marks_changed()), vesselextr_widget,
					SLOT(marks_changed()));
			QObject::connect(vesselextr_widget,
					SIGNAL(vp1_changed(std::vector<Point>*)), bmp_show,
					SLOT(set_vp1(std::vector<Point>*)));
		}
		else if (qw == watershed_widget)
		{
			QObject::connect(this, SIGNAL(marks_changed()), watershed_widget,
					SLOT(marks_changed()));
		}
		else if (qw == hyst_widget)
		{
			QObject::connect(this, SIGNAL(bmp_changed()), hyst_widget,
					SLOT(bmp_changed()));

			QObject::connect(hyst_widget, SIGNAL(vp1_changed(std::vector<Point>*)),
					bmp_show, SLOT(set_vp1(std::vector<Point>*)));
			QObject::connect(hyst_widget,
					SIGNAL(vp1dyn_changed(std::vector<Point>*, std::vector<Point>*)), bmp_show,
					SLOT(set_vp1_dyn(std::vector<Point>*, std::vector<Point>*)));
		}
		else if (qw == livewire_widget)
		{
			QObject::connect(this, SIGNAL(bmp_changed()), livewire_widget,
					SLOT(bmp_changed()));
			QObject::connect(bmp_show, SIGNAL(mousedoubleclick_sign(Point)),
					livewire_widget, SLOT(pt_doubleclicked(Point)));
			QObject::connect(bmp_show, SIGNAL(mousepressedmid_sign(Point)), livewire_widget,
					SLOT(pt_midclicked(Point)));
			QObject::connect(bmp_show, SIGNAL(mousedoubleclickmid_sign(Point)),
					livewire_widget, SLOT(pt_doubleclickedmid(Point)));
			QObject::connect(livewire_widget,
					SIGNAL(vp1_changed(std::vector<Point>*)), bmp_show,
					SLOT(set_vp1(std::vector<Point>*)));
			QObject::connect(livewire_widget,
					SIGNAL(vp1dyn_changed(std::vector<Point>*, std::vector<Point>*)), bmp_show,
					SLOT(set_vp1_dyn(std::vector<Point>*, std::vector<Point>*)));

			bmp_show->setMouseTracking(true);
		}
		else if (qw == iftrg_widget)
		{
			QObject::connect(this, SIGNAL(bmp_changed()), iftrg_widget,
					SLOT(bmp_changed()));
		}
		else if (qw == fastmarching_widget)
		{
			QObject::connect(this, SIGNAL(bmp_changed()), fastmarching_widget,
					SLOT(bmp_changed()));
		}
		else if (qw == olc_widget)
		{
			QObject::connect(this, SIGNAL(work_changed()), olc_widget,
					SLOT(workbits_changed()));

			QObject::connect(olc_widget,
					SIGNAL(vpdyn_changed(std::vector<Point>*)), work_show,
					SLOT(set_vpdyn(std::vector<Point>*)));

			olc_widget->workbits_changed();
		}
		else if (qw == feature_widget)
		{
			QObject::connect(feature_widget,
					SIGNAL(vpdyn_changed(std::vector<Point>*)), work_show,
					SLOT(set_vpdyn(std::vector<Point>*)));

			work_show->setMouseTracking(true);
			bmp_show->setMouseTracking(true);
		}
		else if (qw == interpolation_widget)
		{
			// anything?
		}
		else if (qw == picker_widget)
		{
			QObject::connect(picker_widget, SIGNAL(vp1_changed(vector<Point>*)),
					bmp_show, SLOT(set_vp1(vector<Point>*)));
			QObject::connect(picker_widget, SIGNAL(vp1_changed(vector<Point>*)),
					work_show, SLOT(set_vp1(vector<Point>*)));
		}
		else if (qw == transform_widget)
		{
			QObject::connect(this, SIGNAL(bmp_changed()), transform_widget,
					SLOT(bmp_changed()));
			QObject::connect(this, SIGNAL(work_changed()), transform_widget,
					SLOT(work_changed()));
			QObject::connect(this, SIGNAL(tissues_changed()), transform_widget,
					SLOT(tissues_changed()));
		}

		tab_old = (WidgetInterface*)qw;
		tab_old->init();
		tab_old->on_tissuenr_changed(tissueTreeWidget->get_current_type() - 1); // \todo BL: is this correct for all widgets?

		bmp_show->setCursor(*(tab_old->get_cursor()));
		work_show->setCursor(*(tab_old->get_cursor()));

		updateMethodButtonsPressed(tab_old);
	}
	else
	{
		tab_old = (WidgetInterface*)qw;
	}

	tab_old->setFocus();
}

void MainWindow::updateTabvisibility()
{
	auto nrtabbuttons = (unsigned short)tabwidgets.size();
	unsigned short counter = 0;
	for (unsigned short i = 0; i < nrtabbuttons; i++)
	{
		if (showpb_tab[i])
			counter++;
	}
	unsigned short counter1 = 0;
	for (unsigned short i = 0; i < nrtabbuttons; i++)
	{
		if (showpb_tab[i])
		{
			pb_tab[counter1]->setIconSet(tabwidgets[i]->GetIcon(m_picpath));
			pb_tab[counter1]->setText(tabwidgets[i]->GetName().c_str());
			pb_tab[counter1]->setToolTip(tabwidgets[i]->toolTip());
			pb_tab[counter1]->show();
			counter1++;
			if (counter1 == (counter + 1) / 2)
			{
				while (counter1 < (nrtabbuttons + 1) / 2)
				{
					pb_tab[counter1]->hide();
					counter1++;
				}
			}
		}
	}

	while (counter1 < nrtabbuttons)
	{
		pb_tab[counter1]->hide();
		counter1++;
	}

	WidgetInterface* qw = static_cast<WidgetInterface*>(methodTab->currentWidget());
	updateMethodButtonsPressed(qw);
}

void MainWindow::updateMethodButtonsPressed(WidgetInterface* qw)
{
	auto nrtabbuttons = (unsigned short)tabwidgets.size();
	unsigned short counter = 0;
	unsigned short pos = nrtabbuttons;
	for (unsigned short i = 0; i < nrtabbuttons; i++)
	{
		if (showpb_tab[i])
		{
			if (tabwidgets[i] == qw)
				pos = counter;
			counter++;
		}
	}

	unsigned short counter1 = 0;
	for (unsigned short i = 0; i < (counter + 1) / 2; i++, counter1++)
	{
		if (counter1 == pos)
			pb_tab[i]->setOn(true);
		else
			pb_tab[i]->setOn(false);
	}

	for (unsigned short i = (nrtabbuttons + 1) / 2; counter1 < counter;
			 i++, counter1++)
	{
		if (counter1 == pos)
			pb_tab[i]->setOn(true);
		else
			pb_tab[i]->setOn(false);
	}
}

void MainWindow::LoadAtlas(const QDir& path1)
{
	m_atlasfilename.m_atlasdir = path1;
	QStringList filters;
	filters << "*.atl";
	QStringList names1 = path1.entryList(filters);
	m_atlasfilename.nratlases = (int)names1.size();
	if (m_atlasfilename.nratlases > m_atlasfilename.maxnr)
		m_atlasfilename.nratlases = m_atlasfilename.maxnr;
	for (int i = 0; i < m_atlasfilename.nratlases; i++)
	{
		m_atlasfilename.m_atlasfilename[i] = names1[i];
		QFileInfo names1fi(names1[i]);
		atlasmenu->changeItem(m_atlasfilename.atlasnr[i],
				names1fi.completeBaseName());
		atlasmenu->setItemVisible(m_atlasfilename.atlasnr[i], true);
	}
	for (int i = m_atlasfilename.nratlases; i < m_atlasfilename.maxnr; i++)
	{
		atlasmenu->setItemVisible(m_atlasfilename.atlasnr[i], false);
	}

	if (!names1.empty())
		atlasmenu->setItemVisible(m_atlasfilename.separatornr, true);
}

void MainWindow::LoadLoadProj(const QString& path1)
{
	unsigned short projcounter = 0;
	FILE* fplatestproj = fopen(path1.ascii(), "r");
	char c;
	m_loadprojfilename.m_filename = path1;
	while (fplatestproj != nullptr && projcounter < 4)
	{
		projcounter++;
		QString qs_filename1 = "";
		while (fscanf(fplatestproj, "%c", &c) == 1 && c != '\n')
		{
			qs_filename1 += c;
		}
		if (!qs_filename1.isEmpty())
			AddLoadProj(qs_filename1);
	}
	if (fplatestproj != 0)
		fclose(fplatestproj);
}

void MainWindow::AddLoadProj(const QString& path1)
{
	if (m_loadprojfilename.m_loadprojfilename1 != path1 &&
			m_loadprojfilename.m_loadprojfilename2 != path1 &&
			m_loadprojfilename.m_loadprojfilename3 != path1 &&
			m_loadprojfilename.m_loadprojfilename4 != path1)
	{
		m_loadprojfilename.m_loadprojfilename4 =
				m_loadprojfilename.m_loadprojfilename3;
		m_loadprojfilename.m_loadprojfilename3 =
				m_loadprojfilename.m_loadprojfilename2;
		m_loadprojfilename.m_loadprojfilename2 =
				m_loadprojfilename.m_loadprojfilename1;
		m_loadprojfilename.m_loadprojfilename1 = path1;
	}
	else
	{
		if (m_loadprojfilename.m_loadprojfilename2 == path1)
		{
			QString dummy = m_loadprojfilename.m_loadprojfilename2;
			m_loadprojfilename.m_loadprojfilename2 =
					m_loadprojfilename.m_loadprojfilename1;
			m_loadprojfilename.m_loadprojfilename1 = dummy;
		}
		if (m_loadprojfilename.m_loadprojfilename3 == path1)
		{
			QString dummy = m_loadprojfilename.m_loadprojfilename3;
			m_loadprojfilename.m_loadprojfilename3 =
					m_loadprojfilename.m_loadprojfilename1;
			m_loadprojfilename.m_loadprojfilename1 = dummy;
		}
		if (m_loadprojfilename.m_loadprojfilename4 == path1)
		{
			QString dummy = m_loadprojfilename.m_loadprojfilename4;
			m_loadprojfilename.m_loadprojfilename4 =
					m_loadprojfilename.m_loadprojfilename1;
			m_loadprojfilename.m_loadprojfilename1 = dummy;
		}
	}

	if (m_loadprojfilename.m_loadprojfilename1 != QString(""))
	{
		int pos = m_loadprojfilename.m_loadprojfilename1.findRev('/', -2);
		if (pos != -1 &&
				(int)m_loadprojfilename.m_loadprojfilename1.length() > pos + 1)
		{
			QString name1 = m_loadprojfilename.m_loadprojfilename1.right(
					m_loadprojfilename.m_loadprojfilename1.length() - pos - 1);
			file->changeItem(m_loadprojfilename.lpf1nr, name1);
			file->setItemVisible(m_loadprojfilename.lpf1nr, true);
		}
	}
	if (m_loadprojfilename.m_loadprojfilename2 != QString(""))
	{
		int pos = m_loadprojfilename.m_loadprojfilename2.findRev('/', -2);
		if (pos != -1 &&
				(int)m_loadprojfilename.m_loadprojfilename2.length() > pos + 1)
		{
			QString name1 = m_loadprojfilename.m_loadprojfilename2.right(
					m_loadprojfilename.m_loadprojfilename2.length() - pos - 1);
			file->changeItem(m_loadprojfilename.lpf2nr, name1);
			file->setItemVisible(m_loadprojfilename.lpf2nr, true);
		}
	}
	if (m_loadprojfilename.m_loadprojfilename3 != QString(""))
	{
		int pos = m_loadprojfilename.m_loadprojfilename3.findRev('/', -2);
		if (pos != -1 &&
				(int)m_loadprojfilename.m_loadprojfilename3.length() > pos + 1)
		{
			QString name1 = m_loadprojfilename.m_loadprojfilename3.right(
					m_loadprojfilename.m_loadprojfilename3.length() - pos - 1);
			file->changeItem(m_loadprojfilename.lpf3nr, name1);
			file->setItemVisible(m_loadprojfilename.lpf3nr, true);
		}
	}
	if (m_loadprojfilename.m_loadprojfilename4 != QString(""))
	{
		int pos = m_loadprojfilename.m_loadprojfilename4.findRev('/', -2);
		if (pos != -1 &&
				(int)m_loadprojfilename.m_loadprojfilename4.length() > pos + 1)
		{
			QString name1 = m_loadprojfilename.m_loadprojfilename4.right(
					m_loadprojfilename.m_loadprojfilename4.length() - pos - 1);
			file->changeItem(m_loadprojfilename.lpf4nr, name1);
			file->setItemVisible(m_loadprojfilename.lpf4nr, true);
		}
	}
	file->setItemVisible(m_loadprojfilename.separatornr, true);
}

void MainWindow::SaveLoadProj(const QString& latestprojpath)
{
	if (latestprojpath == QString(""))
		return;
	FILE* fplatestproj = fopen(latestprojpath.ascii(), "w");
	if (fplatestproj != nullptr)
	{
		if (m_loadprojfilename.m_loadprojfilename4 != QString(""))
		{
			fprintf(fplatestproj, "%s\n",
					m_loadprojfilename.m_loadprojfilename4.ascii());
		}
		if (m_loadprojfilename.m_loadprojfilename3 != QString(""))
		{
			fprintf(fplatestproj, "%s\n",
					m_loadprojfilename.m_loadprojfilename3.ascii());
		}
		if (m_loadprojfilename.m_loadprojfilename2 != QString(""))
		{
			fprintf(fplatestproj, "%s\n",
					m_loadprojfilename.m_loadprojfilename2.ascii());
		}
		if (m_loadprojfilename.m_loadprojfilename1 != QString(""))
		{
			fprintf(fplatestproj, "%s\n",
					m_loadprojfilename.m_loadprojfilename1.ascii());
		}

		fclose(fplatestproj);
	}
}

void MainWindow::pb_tab_pressed(int nr)
{
	auto nrtabbuttons = (unsigned short)tabwidgets.size();
	unsigned short tabnr = nr + 1;
	for (unsigned short tabnr1 = 0; tabnr1 < pb_tab.size(); tabnr1++)
	{
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
	while (pos1 < tabnr)
	{
		if (showpb_tab[pos2])
			pos1++;
		pos2++;
	}
	pos2--;
	methodTab->setCurrentWidget(tabwidgets[pos2]);
}

void MainWindow::bmpcrosshairvisible_changed()
{
	if (cb_bmpcrosshairvisible->isChecked())
	{
		if (xsliceshower != nullptr)
		{
			bmp_show->crosshairy_changed(xsliceshower->get_slicenr());
			bmp_show->set_crosshairyvisible(true);
		}
		if (ysliceshower != nullptr)
		{
			bmp_show->crosshairx_changed(ysliceshower->get_slicenr());
			bmp_show->set_crosshairxvisible(true);
		}
	}
	else
	{
		bmp_show->set_crosshairxvisible(false);
		bmp_show->set_crosshairyvisible(false);
	}
}

void MainWindow::workcrosshairvisible_changed()
{
	if (cb_workcrosshairvisible->isChecked())
	{
		if (xsliceshower != nullptr)
		{
			work_show->crosshairy_changed(xsliceshower->get_slicenr());
			work_show->set_crosshairyvisible(true);
		}
		if (ysliceshower != nullptr)
		{
			work_show->crosshairx_changed(ysliceshower->get_slicenr());
			work_show->set_crosshairxvisible(true);
		}
	}
	else
	{
		work_show->set_crosshairxvisible(false);
		work_show->set_crosshairyvisible(false);
	}
}

void MainWindow::execute_inversesliceorder()
{
	iseg::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.bmp = true;
	dataSelection.work = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this, false);

	handler3D->inversesliceorder();

	//	pixelsize_changed();
	emit end_datachange(this, iseg::NoUndo);
}

void MainWindow::execute_smoothsteps()
{
	iseg::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this);

	handler3D->stepsmooth_z(5);

	emit end_datachange(this);
}

void MainWindow::execute_smoothtissues()
{
	iseg::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this);

	handler3D->smooth_tissues(3);

	emit end_datachange(this);
}
void MainWindow::execute_remove_unused_tissues()
{
	// get all unused tissue ids
	auto unused = handler3D->find_unused_tissues();

	if (unused.empty())
	{
		QMessageBox::information(this, "iSeg", "No unused tissues found");
		return;
	}
	else
	{
		int ret = QMessageBox::warning(this, "iSeg", "Found " + QString::number(unused.size()) + " unused tissues. Do you want to remove them?",
				QMessageBox::Yes | QMessageBox::Default, QMessageBox::Cancel | QMessageBox::Escape);
		if (ret != QMessageBox::Yes)
		{
			return;
		}
	}

	auto all = tissueTreeWidget->get_all_items(true);

	// collect tree items matching the list of tissue ids
	std::vector<QTreeWidgetItem*> list;
	for (auto item : all)
	{
		auto type = tissueTreeWidget->get_type(item);
		if (std::find(unused.begin(), unused.end(), type) != unused.end())
		{
			list.push_back(item);
		}
	}
	removeselected(list, false);
}

void MainWindow::execute_cleanup()
{
	tissues_size_t** slices = new tissues_size_t*[handler3D->end_slice() - handler3D->start_slice()];
	tissuelayers_size_t activelayer = handler3D->active_tissuelayer();
	for (unsigned short i = handler3D->start_slice(); i < handler3D->end_slice(); i++)
	{
		slices[i - handler3D->start_slice()] = handler3D->return_tissues(activelayer, i);
	}
	TissueCleaner TC(
			slices, handler3D->end_slice() - handler3D->start_slice(),
			handler3D->width(), handler3D->height());
	if (!TC.Allocate())
	{
		QMessageBox::information(this, "iSeg", "Not enough memory.\nThis operation cannot be performed.\n");
	}
	else
	{
		int rate, minsize;
		CleanerParams CP(&rate, &minsize);
		CP.exec();
		if (rate == 0 && minsize == 0)
			return;

		iseg::DataSelection dataSelection;
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

void MainWindow::mousePosZoom_changed(const QPoint& point)
{
	bmp_show->setMousePosZoom(point);
	work_show->setMousePosZoom(point);
	//mousePosZoom = point;
}

FILE* MainWindow::save_notes(FILE* fp, unsigned short version)
{
	if (version >= 7)
	{
		QString text = m_notes->toPlainText();
		int dummy = (int)text.length();
		fwrite(&(dummy), 1, sizeof(int), fp);
		fwrite(text.ascii(), 1, dummy, fp);
	}
	return fp;
}

FILE* MainWindow::load_notes(FILE* fp, unsigned short version)
{
	m_notes->clear();
	if (version >= 7)
	{
		int dummy = 0;
		fread(&dummy, sizeof(int), 1, fp);
		char* text = new char[dummy + 1];
		text[dummy] = '\0';
		fread(text, dummy, 1, fp);
		m_notes->setPlainText(QString(text));
		free(text);
	}
	return fp;
}

void MainWindow::disconnect_mouseclick()
{
	if (tab_old)
	{
		QObject::disconnect(bmp_show, SIGNAL(mousepressed_sign(Point)), tab_old,
				SLOT(mouse_clicked(Point)));
		QObject::disconnect(bmp_show, SIGNAL(mousereleased_sign(Point)), tab_old,
				SLOT(mouse_released(Point)));
		QObject::disconnect(bmp_show, SIGNAL(mousemoved_sign(Point)), tab_old,
				SLOT(mouse_moved(Point)));

		QObject::disconnect(work_show, SIGNAL(mousepressed_sign(Point)), tab_old,
				SLOT(mouse_clicked(Point)));
		QObject::disconnect(work_show, SIGNAL(mousereleased_sign(Point)), tab_old,
				SLOT(mouse_released(Point)));
		QObject::disconnect(work_show, SIGNAL(mousemoved_sign(Point)), tab_old,
				SLOT(mouse_moved(Point)));
	}
}

void MainWindow::connect_mouseclick()
{
	if (tab_old)
	{
		QObject::connect(bmp_show, SIGNAL(mousepressed_sign(Point)), tab_old,
				SLOT(mouse_clicked(Point)));
		QObject::connect(bmp_show, SIGNAL(mousereleased_sign(Point)), tab_old,
				SLOT(mouse_released(Point)));
		QObject::connect(bmp_show, SIGNAL(mousemoved_sign(Point)), tab_old,
				SLOT(mouse_moved(Point)));
		QObject::connect(work_show, SIGNAL(mousepressed_sign(Point)), tab_old,
				SLOT(mouse_clicked(Point)));
		QObject::connect(work_show, SIGNAL(mousereleased_sign(Point)), tab_old,
				SLOT(mouse_released(Point)));
		QObject::connect(work_show, SIGNAL(mousemoved_sign(Point)), tab_old,
				SLOT(mouse_moved(Point)));
	}
}

void MainWindow::reconnectmouse_afterrelease(Point)
{
	QObject::disconnect(work_show, SIGNAL(mousereleased_sign(Point)), this,
			SLOT(reconnectmouse_afterrelease(Point)));
	connect_mouseclick();
}

void MainWindow::handle_begin_datachange(iseg::DataSelection& dataSelection,
		QWidget* sender, bool beginUndo)
{
	undoStarted = beginUndo || undoStarted;
	changeData = dataSelection;

	// Handle pending transforms
	if (methodTab->currentWidget() == transform_widget && sender != transform_widget)
	{
		cancel_transform_helper();
	}

	// Begin undo
	if (beginUndo)
	{
		if (changeData.allSlices)
		{
			// Start undo for all slices
			canUndo3D = handler3D->return_undo3D() && handler3D->start_undoall(changeData);
		}
		else
		{
			// Start undo for single slice
			handler3D->start_undo(changeData);
		}
	}
}

void MainWindow::end_undo_helper(iseg::EndUndoAction undoAction)
{
	if (undoStarted)
	{
		if (undoAction == iseg::EndUndo)
		{
			if (changeData.allSlices)
			{
				// End undo for all slices
				if (canUndo3D)
				{
					handler3D->end_undo();
					do_undostepdone();
				}
				else
				{
					do_clearundo();
				}
			}
			else
			{
				// End undo for single slice
				handler3D->end_undo();
				do_undostepdone();
			}
			undoStarted = false;
		}
		else if (undoAction == iseg::MergeUndo)
		{
			handler3D->merge_undo();
		}
		else if (undoAction == iseg::AbortUndo)
		{
			handler3D->abort_undo();
			undoStarted = false;
		}
	}
	else if (undoAction == iseg::ClearUndo)
	{
		do_clearundo();
	}
}

void MainWindow::handle_end_datachange(QWidget* sender, iseg::EndUndoAction undoAction)
{
	// End undo
	end_undo_helper(undoAction);

	// Handle 3d data change
	if (changeData.allSlices)
	{
		slices3d_changed(sender != bitstack_widget);
	}

	// Update ranges
	update_ranges_helper();

	// Block changed data signals for visible widget
	if (sender == methodTab->currentWidget())
	{
		QObject::disconnect(this, SIGNAL(bmp_changed()), sender, SLOT(bmp_changed()));
		QObject::disconnect(this, SIGNAL(work_changed()), sender, SLOT(work_changed()));
		QObject::disconnect(this, SIGNAL(tissues_changed()), sender, SLOT(tissues_changed()));
		QObject::disconnect(this, SIGNAL(marks_changed()), sender, SLOT(marks_changed()));
	}

	// Signal changed data
	if (changeData.bmp)
	{
		emit bmp_changed();
	}
	if (changeData.work)
	{
		emit work_changed();
	}
	if (changeData.tissues)
	{
		emit tissues_changed();
	}
	if (changeData.marks)
	{
		emit marks_changed();
	}

	// Signal changed data
	// \hack BL this looks like a hack, fixme
	if (changeData.bmp && changeData.work && changeData.allSlices)
	{
		// Do not reinitialize m_MultiDataset_widget after swapping
		if (m_NewDataAfterSwap)
		{
			m_NewDataAfterSwap = false;
		}
		else
		{
			multidataset_widget->NewLoaded();
		}
	}

	if (sender == methodTab->currentWidget())
	{
		QObject::connect(this, SIGNAL(bmp_changed()), sender, SLOT(bmp_changed()));
		QObject::connect(this, SIGNAL(work_changed()), sender, SLOT(work_changed()));
		QObject::connect(this, SIGNAL(tissues_changed()), sender, SLOT(tissues_changed()));
		QObject::connect(this, SIGNAL(marks_changed()), sender, SLOT(marks_changed()));
	}
}

void MainWindow::DatasetChanged()
{
	emit bmp_changed();
	emit work_changed();

	reset_brightnesscontrast();
}

void MainWindow::update_ranges_helper()
{
	if (changeData.bmp)
	{
		if (changeData.allSlices)
		{
			bmp_show->update_range();
		}
		else
		{
			bmp_show->update_range(changeData.sliceNr);
		}
		update_brightnesscontrast(true, false);
	}
	if (changeData.work)
	{
		if (changeData.allSlices)
		{
			work_show->update_range();
		}
		else
		{
			work_show->update_range(changeData.sliceNr);
		}
		update_brightnesscontrast(false, false);
	}
}

void MainWindow::cancel_transform_helper()
{
	QObject::disconnect(
			transform_widget,
			SIGNAL(begin_datachange(iseg::DataSelection&, QWidget*, bool)), this,
			SLOT(handle_begin_datachange(iseg::DataSelection&, QWidget*, bool)));
	QObject::disconnect(
			transform_widget, SIGNAL(end_datachange(QWidget*, iseg::EndUndoAction)),
			this, SLOT(handle_end_datachange(QWidget*, iseg::EndUndoAction)));
	transform_widget->CancelPushButtonClicked();
	QObject::connect(
			transform_widget,
			SIGNAL(begin_datachange(iseg::DataSelection&, QWidget*, bool)), this,
			SLOT(handle_begin_datachange(iseg::DataSelection&, QWidget*, bool)));
	QObject::connect(transform_widget,
			SIGNAL(end_datachange(QWidget*, iseg::EndUndoAction)), this,
			SLOT(handle_end_datachange(QWidget*, iseg::EndUndoAction)));

	// Signal changed data
	bool bmp, work, tissues;
	transform_widget->GetDataSelection(bmp, work, tissues);
	if (bmp)
	{
		emit bmp_changed();
	}
	if (work)
	{
		emit work_changed();
	}
	if (tissues)
	{
		emit tissues_changed();
	}
}

void MainWindow::handle_begin_dataexport(iseg::DataSelection& dataSelection,
		QWidget* sender)
{
	// Handle pending transforms
	if (methodTab->currentWidget() == transform_widget &&
			(dataSelection.bmp || dataSelection.work || dataSelection.tissues))
	{
		cancel_transform_helper();
	}

	// Handle pending tissue hierarchy modifications
	if (dataSelection.tissueHierarchy)
	{
		tissueHierarchyWidget->handle_changed_hierarchy();
	}
}

void MainWindow::handle_end_dataexport(QWidget* sender) {}

void MainWindow::execute_savecolorlookup()
{
	if (!handler3D->GetColorLookupTable())
	{
		QMessageBox::warning(this, "iSeg", "No color lookup table to export\n", QMessageBox::Ok | QMessageBox::Default);
		return;
	}

	QString savefilename = QFileDialog::getSaveFileName(QString::null, "iSEG Color Lookup Table (*.lut)", this);

	if (!savefilename.endsWith(QString(".lut")))
		savefilename.append(".lut");

	XdmfImageWriter writer(savefilename.toStdString().c_str());
	writer.SetCompression(handler3D->GetCompression());
	if (!writer.WriteColorLookup(handler3D->GetColorLookupTable().get(), true))
	{
		QMessageBox::warning(this, "iSeg",
				"ERROR: occurred while exporting color lookup table\n", QMessageBox::Ok | QMessageBox::Default);
	}
}

void MainWindow::execute_voting_replace_labels()
{
	auto sel = handler3D->tissue_selection();
	if (sel.size() == 1 && !handler3D->tissue_locks().at(sel.at(0)))
	{
		tissues_size_t FG = sel.front();
		std::array<unsigned int, 3> radius = {1, 1, 1};

		iseg::DataSelection dataSelection;
		dataSelection.allSlices = true;
		dataSelection.tissues = true;
		emit begin_datachange(dataSelection, this);

		auto remaining_voxels = VotingReplaceLabel(handler3D, FG, 0, radius, 1, 10);

		emit end_datachange(this, iseg::EndUndo);
	}
	else
	{
		QMessageBox::warning(this, "iSeg", "Please select only one non-locked tissue\n", QMessageBox::Ok | QMessageBox::Default);
	}
}

void MainWindow::execute_target_connected_components()
{
	ProgressDialog progress("Connected component analysis", this);

	iseg::DataSelection dataSelection;
	dataSelection.allSlices = true;
	dataSelection.work = true;
	emit begin_datachange(dataSelection, this);

	bool ok = handler3D->compute_target_connectivity(&progress);

	emit end_datachange(this, ok ? iseg::EndUndo : iseg::AbortUndo);
}

void MainWindow::execute_split_tissue()
{
	auto sel = handler3D->tissue_selection();
	if (sel.size() == 1 && !handler3D->tissue_locks().at(sel.at(0)))
	{
		ProgressDialog progress("Connected component analysis", this);

		iseg::DataSelection dataSelection;
		dataSelection.allSlices = true;
		dataSelection.tissues = true;
		emit begin_datachange(dataSelection, this);

		bool ok = handler3D->compute_split_tissues(sel.front(), &progress);

		emit end_datachange(this, ok ? iseg::EndUndo : iseg::AbortUndo);

		// update tree view after adding new tissues
		tissueTreeWidget->update_tree_widget();
		tissueTreeWidget->update_tissue_icons();
		tissueTreeWidget->update_folder_icons();
	}
	else
	{
		QMessageBox::warning(this, "iSeg", "Please select only one non-locked tissue\n", QMessageBox::Ok | QMessageBox::Default);
	}
}
