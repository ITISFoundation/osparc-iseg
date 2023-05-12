/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 *
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 *
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "Precompiled.h"

#include "../config.h"

#include "ActiveSlicesConfigDialog.h"
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
#include "LogTable.h"
#include "MainWindow.h"
#include "MeasurementWidget.h"
#include "MorphologyWidget.h"
#include "MultiDatasetWidget.h"
#include "OutlineCorrectionWidget.h"
#include "PickerWidget.h"
#include "SaveOutlinesDialog.h"
#include "SelectColorButton.h"
#include "Settings.h"
#include "SliceViewerWidget.h"
#include "SmoothingWidget.h"
#include "StdStringToQString.h"
#include "SurfaceViewerWidget.h"
#include "ThresholdWidgetQt4.h"
#include "TissueCleaner.h"
#include "TissueInfos.h"
#include "TissueTreeWidget.h"
#include "TransformWidget.h"
#include "UndoConfigurationDialog.h"
#include "VesselWidget.h"
#include "VolumeViewerWidget.h"
#include "WatershedWidget.h"
#include "WidgetCollection.h"
#include "XdmfImageWriter.h"

#include "RadiotherapyStructureSetImporter.h"

#include "Interface/Plugin.h"
#include "Interface/ProgressDialog.h"
#include "Interface/RecentPlaces.h"

#include "Data/Transform.h"

#include "Core/HDF5Blosc.h"
#include "Core/LoadPlugin.h"
#include "Core/ProjectVersion.h"
#include "Core/VotingReplaceLabel.h"

#include <boost/filesystem.hpp>

#include <QDesktopWidget>
#include <QFileDialog>
#include <QInputDialog>
#include <QShortcut>
#include <QSignalMapper>
#include <QStackedWidget>
#include <QStatusBar>
#include <QToolButton>
#include <QApplication>
#include <QDockWidget>
#include <QMenuBar>
#include <QProgressDialog>
#include <QSettings>
#include <QTextEdit>
#include <QToolTip>

#include <Q3ScrollView>

#include <algorithm>

#define str_macro(s) #s
#define xstr(s) str_macro(s)

namespace iseg {

namespace {
int open_s4_l_link_pos = -1;
int import_surface_pos = -1;
int import_r_tstruct_pos = -1;
} // namespace

int bmpimgnr(QString* s, const QString& ext = ".bmp")
{
	int result = 0;
	uint pos;
	if (s->length() > 4 && s->endsWith(ext))
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
	if (str != "")
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

bool read_grouptissues(const char* filename, std::vector<tissues_size_t>& olds, std::vector<tissues_size_t>& news)
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

bool read_grouptissuescapped(const char* filename, std::vector<tissues_size_t>& olds, std::vector<tissues_size_t>& news, bool fail_on_unknown_tissue)
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
		tissues_size_t tissue_count = TissueInfos::GetTissueCount();

		// Try scan first entry to decide between tissue name and index input
		bool read_indices = false;
		if (fscanf(fp, "%s %s\n", name1, name2) == 2)
		{
			type1 = (tissues_size_t)QString(name1).toUInt(&read_indices);
		}

		bool ok = true;
		if (read_indices)
		{
			// Read input as tissue indices
			type2 = (tissues_size_t)QString(name2).toUInt(&read_indices);
			if (type1 > 0 && type1 <= tissue_count && type2 > 0 &&
					type2 <= tissue_count)
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
				if (type1 > 0 && type1 <= tissue_count && type2 > 0 && type2 <= tissue_count)
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
			if (type1 > 0 && type1 <= tissue_count && type2 > 0 && type2 <= tissue_count)
			{
				olds.push_back(type1);
				news.push_back(type2);
			}
			else
			{
				ok = false;
				if (type1 == 0 || type1 > tissue_count)
					ISEG_WARNING("old: " << name1 << " not in tissue list");
				if (type2 == 0 || type2 > tissue_count)
					ISEG_WARNING("new: " << name2 << " not in tissue list");
			}
			memset(name1, 0, 1000);
			memset(name2, 0, 1000);

			while (fscanf(fp, "%s %s\n", name1, name2) == 2)
			{
				type1 = TissueInfos::GetTissueType(std::string(name1));
				type2 = TissueInfos::GetTissueType(std::string(name2));
				if (type1 > 0 && type1 <= tissue_count && type2 > 0 && type2 <= tissue_count)
				{
					olds.push_back(type1);
					news.push_back(type2);
				}
				else
				{
					ok = false;
					if (type1 == 0 || type1 > tissue_count)
						ISEG_WARNING("old: " << name1 << " not in tissue list");
					if (type2 == 0 || type2 > tissue_count)
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

	tissues_size_t const tissue_count = TissueInfos::GetTissueCount();
	char name[1000];
	bool ok = true;

	while (fscanf(fp, "%s\n", name) == 1)
	{
		tissues_size_t type = TissueInfos::GetTissueType(std::string(name));
		if (type > 0 && type <= tissue_count)
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

bool MenuWTT::event(QEvent* e)
{
	// not needed from Qt 5.1 -> see QMenu::setToolTipVisible
	const QHelpEvent* help_event = static_cast<QHelpEvent*>(e);
	if (help_event->type() == QEvent::ToolTip && activeAction() != nullptr)
	{
		QToolTip::showText(help_event->globalPos(), activeAction()->toolTip());
	}
	else
		QToolTip::hideText();

	return QMenu::event(e);
}

MainWindow::MainWindow(SlicesHandler* hand3D, const QString& locationstring, const QDir& picpath, const QDir& tmppath, bool editingmode, const std::vector<std::string>& plugin_search_dirs)
		: m_Handler3D(hand3D)
{
	setObjectName("MainWindow");
	statusBar()->showMessage("Ready");

	auto log_button = new QToolButton;
	//log_button->setStyleSheet("QToolButton { border: none; background-color: rgb(70,70,70); }");
	log_button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	log_button->setArrowType(Qt::ArrowType::RightArrow);
	log_button->setText("Log");
	log_button->setCheckable(true);
	log_button->setChecked(false);
	log_button->setMinimumWidth(100);
	statusBar()->addPermanentWidget(log_button);

	m_UndoStarted = false;
	setContentsMargins(9, 4, 9, 4);
	m_MPicpath = picpath;
	m_MTmppath = tmppath;
	m_MEditingmode = editingmode;
	m_TabOld = nullptr;

	setWindowTitle(QString(" iSeg ") + QString(xstr(ISEG_VERSION)) +
						 QString(" - No Filename"));
	QIcon isegicon(m_MPicpath.absoluteFilePath("isegicon.png"));
	setWindowIcon(isegicon);
	m_MLocationstring = locationstring;
	m_MSaveprojfilename = "";
	m_S4Lcommunicationfilename = "";

	m_Handler3D->m_OnTissueSelectionChanged.connect([this](const std::vector<tissues_size_t>& sel) {
		std::set<tissues_size_t> selection(sel.begin(), sel.end());
		QTreeWidgetItem* first = nullptr;
		bool clear_filter = false;
		for (auto item : m_TissueTreeWidget->GetAllItems())
		{
			bool select = selection.count(m_TissueTreeWidget->GetType(item)) != 0;
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
			m_TissueFilter->setText(QString(""));
		}
		m_TissueTreeWidget->scrollToItem(first);
	});

	m_Handler3D->m_OnActiveSliceChanged.connect([this](unsigned short slice) {
		SliceChanged();
	});

	if (!m_Handler3D->Isloaded())
	{
		m_Handler3D->Newbmp(512, 512, 10);
	}

	m_CbBmptissuevisible = new QCheckBox("Show Tissues", this);
	m_CbBmptissuevisible->setChecked(true);
	m_CbBmpcrosshairvisible = new QCheckBox("Show Crosshair", this);
	m_CbBmpcrosshairvisible->setChecked(false);
	m_CbBmpoutlinevisible = new QCheckBox("Show Outlines", this);
	m_CbBmpoutlinevisible->setChecked(true);
	auto bt_select_outline_color = new SelectColorButton(this);
	bt_select_outline_color->setMaximumWidth(17);
	bt_select_outline_color->setMaximumHeight(17);
	m_CbWorktissuevisible = new QCheckBox("Show Tissues", this);
	m_CbWorktissuevisible->setChecked(false);
	m_CbWorkcrosshairvisible = new QCheckBox("Show Crosshair", this);
	m_CbWorkcrosshairvisible->setChecked(false);
	m_CbWorkpicturevisible = new QCheckBox("Show Image", this);
	m_CbWorkpicturevisible->setChecked(true);

	m_BmpShow = new ImageViewerWidget(this);
	m_LbSource = new QLabel("Source", this);
	m_LbTarget = new QLabel("Target", this);
	m_BmpScroller = new Q3ScrollView(this);
	m_SlContrastbmp = new QSlider(Qt::Horizontal, this);
	m_SlContrastbmp->setRange(0, 100);
	m_SlBrightnessbmp = new QSlider(Qt::Horizontal, this);
	m_SlBrightnessbmp->setRange(0, 100);
	m_LbContrastbmp = new QLabel("C:", this);
	m_LbContrastbmp->setPixmap(QIcon(m_MPicpath.absoluteFilePath("icon-contrast.png")).pixmap());
	m_LeContrastbmpVal = new QLineEdit(this);
	m_LeContrastbmpVal->setAlignment(Qt::AlignRight);
	m_LeContrastbmpVal->setText(QString("%1").arg(9999.99, 6, 'f', 2));
	QString text = m_LeContrastbmpVal->text();
	QFontMetrics fm = m_LeContrastbmpVal->fontMetrics();
	QRect rect = fm.boundingRect(text);
	m_LeContrastbmpVal->setFixedSize(rect.width() + 4, rect.height() + 4);
	m_LbContrastbmpVal = new QLabel("x", this);
	m_LbBrightnessbmp = new QLabel("B:", this);
	m_LbBrightnessbmp->setPixmap(QIcon(m_MPicpath.absoluteFilePath("icon-brightness.png")).pixmap());
	m_LeBrightnessbmpVal = new QLineEdit(this);
	m_LeBrightnessbmpVal->setAlignment(Qt::AlignRight);
	m_LeBrightnessbmpVal->setText(QString("%1").arg(9999, 3));
	text = m_LeBrightnessbmpVal->text();
	fm = m_LeBrightnessbmpVal->fontMetrics();
	rect = fm.boundingRect(text);
	m_LeBrightnessbmpVal->setFixedSize(rect.width() + 4, rect.height() + 4);
	m_LbBrightnessbmpVal = new QLabel("%", this);
	m_BmpScroller->addChild(m_BmpShow);
	m_BmpShow->Init(m_Handler3D, TRUE);
	m_BmpShow->SetWorkbordervisible(TRUE);
	m_BmpShow->SetIsBmp(true);
	m_BmpShow->update();

	m_ToworkBtn = new QPushButton(QIcon(m_MPicpath.absoluteFilePath("next.png")), "", this);
	m_ToworkBtn->setFixedWidth(50);
	m_TobmpBtn = new QPushButton(QIcon(m_MPicpath.absoluteFilePath("previous.png")), "", this);
	m_TobmpBtn->setFixedWidth(50);
	m_SwapBtn = new QPushButton(QIcon(m_MPicpath.absoluteFilePath("swap.png")), "", this);
	m_SwapBtn->setFixedWidth(50);
	m_SwapAllBtn = new QPushButton(QIcon(m_MPicpath.absoluteFilePath("swap.png")), "3D", this);
	m_SwapAllBtn->setFixedWidth(50);

	m_WorkShow = new ImageViewerWidget(this);
	m_SlContrastwork = new QSlider(Qt::Horizontal, this);
	m_SlContrastwork->setRange(0, 100);
	m_SlBrightnesswork = new QSlider(Qt::Horizontal, this);
	m_SlBrightnesswork->setRange(0, 100);
	m_LbContrastwork = new QLabel(this);
	m_LbContrastwork->setPixmap(QIcon(m_MPicpath.absoluteFilePath("icon-contrast.png")).pixmap());
	m_LeContrastworkVal = new QLineEdit(this);
	m_LeContrastworkVal->setAlignment(Qt::AlignRight);
	m_LeContrastworkVal->setText(QString("%1").arg(9999.99, 6, 'f', 2));
	text = m_LeContrastworkVal->text();
	fm = m_LeContrastworkVal->fontMetrics();
	rect = fm.boundingRect(text);
	m_LeContrastworkVal->setFixedSize(rect.width() + 4, rect.height() + 4);
	m_LbContrastworkVal = new QLabel("x", this);
	m_LbBrightnesswork = new QLabel(this);
	m_LbBrightnesswork->setPixmap(QIcon(m_MPicpath.absoluteFilePath("icon-brightness.png")).pixmap());
	m_LeBrightnessworkVal = new QLineEdit(this);
	m_LeBrightnessworkVal->setAlignment(Qt::AlignRight);
	m_LeBrightnessworkVal->setText(QString("%1").arg(9999, 3));
	text = m_LeBrightnessworkVal->text();
	fm = m_LeBrightnessworkVal->fontMetrics();
	rect = fm.boundingRect(text);
	m_LeBrightnessworkVal->setFixedSize(rect.width() + 4, rect.height() + 4);
	m_LbBrightnessworkVal = new QLabel("%", this);
	m_WorkScroller = new Q3ScrollView(this);
	m_WorkScroller->addChild(m_WorkShow);

	m_WorkShow->Init(m_Handler3D, FALSE);
	m_WorkShow->SetTissuevisible(false); //toggle_tissuevisible();
	m_WorkShow->SetIsBmp(false);

	ResetBrightnesscontrast();

	m_LogWindow = new QLogTable(2, this);

	m_ZoomWidget = new ZoomWidget(1.0, m_MPicpath, this);

	m_TissueTreeWidget = new TissueTreeWidget(m_Handler3D->GetTissueHierachy(), m_MPicpath, this);
	m_TissueTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
	m_TissueFilter = new QLineEdit(this);
	m_TissueFilter->setMargin(1);
	m_TissueHierarchyWidget = new TissueHierarchyWidget(m_TissueTreeWidget, this);
	m_TissueTreeWidget->UpdateTreeWidget(); // Reload hierarchy
	m_CbTissuelock = new QCheckBox(this);
	m_CbTissuelock->setPixmap(QIcon(m_MPicpath.absoluteFilePath("lock.png")).pixmap());
	m_CbTissuelock->setChecked(false);
	m_LockTissues = new QPushButton("All", this);
	m_LockTissues->setToggleButton(true);
	m_LockTissues->setFixedWidth(50);
	m_AddTissue = new QPushButton("New Tissue...", this);
	m_AddFolder = new QPushButton("New Folder...", this);

	m_ModifyTissueFolder = new QPushButton("Mod. Tissue/Folder", this);
	m_ModifyTissueFolder->setToolTip(Format("Edit the selected tissue or folder properties."));

	m_RemoveTissueFolder = new QPushButton("Del. Tissue/Folder", this);
	m_RemoveTissueFolder->setToolTip(Format("Remove the selected tissues or folders."));
	m_RemoveTissueFolderAll = new QPushButton("All", this);
	m_RemoveTissueFolderAll->setFixedWidth(30);

	m_Tissue3Dopt = new QCheckBox("3D", this);
	m_Tissue3Dopt->setChecked(false);
	m_Tissue3Dopt->setToolTip(Format("'Get Tissue' is applied in 3D or current slice only."));
	m_GetTissue = new QPushButton("Get Tissue", this);
	m_GetTissue->setToolTip(Format("Get tissue creates a mask in the Target from "
																 "the currently selected Tissue."));
	m_GetTissueAll = new QPushButton("All", this);
	m_GetTissueAll->setFixedWidth(30);
	m_GetTissueAll->setToolTip(Format("Get all tissue creates a mask in the Target "
																		"from all Tissue excluding the background."));
	m_ClearTissue = new QPushButton("Clear Tissue", this);
	m_ClearTissue->setToolTip(Format("Clears the currently selected tissue (use '3D' option to clear "
																	 "whole tissue or just the current slice)."));

	m_ClearTissues = new QPushButton("All", this);
	m_ClearTissues->setFixedWidth(30);
	m_ClearTissues->setToolTip(Format("Clears all tissues (use '3D' option to clear the entire "
																		"segmentation or just the current slice)."));

	m_CbAddsub3d = new QCheckBox("3D", this);
	m_CbAddsub3d->setToolTip(Format("Apply add/remove actions in 3D or current slice only."));
	m_CbAddsuboverride = new QCheckBox("Override", this);
	m_CbAddsuboverride->setToolTip(Format("If override is off, Tissue voxels which are already assigned "
																				"will not be modified. Override allows to change these voxels, "
																				"unless they are locked."));
	m_CbAddsubconn = new QCheckBox("Connected", this);
	m_CbAddsubconn->setToolTip(Format("Only the connected image region is added/removed."));

	m_PbAdd = new QPushButton("+", this);
	m_PbAdd->setToggleButton(true);
	m_PbAdd->setToolTip(Format("Adds next selected/picked Target image region to current tissue."));
	m_PbSub = new QPushButton("-", this);
	m_PbSub->setToggleButton(true);
	m_PbSub->setToolTip(Format("Removes next selected/picked Target image "
														 "region from current tissue."));
	m_PbAddhold = new QPushButton("++", this);
	m_PbAddhold->setToggleButton(true);
	m_PbAddhold->setToolTip(Format("Adds selected/picked Target image regions to current tissue."));

	m_PbSubhold = new QPushButton("--", this);
	m_PbSubhold->setToggleButton(true);
	m_PbSubhold->setToolTip(Format("Removes selected/picked Target image regions from current tissue."));

	m_PbAdd->setFixedWidth(50);
	m_PbSub->setFixedWidth(50);
	m_PbAddhold->setFixedWidth(50);
	m_PbSubhold->setFixedWidth(50);

	m_CbMask3d = new QCheckBox("3D", this);
	m_CbMask3d->setToolTip(Format("Mask source image in 3D or current slice only."));

	auto lb_mask_value = new QLabel("Mask Value");
	m_EditMaskValue = new QLineEdit(QString::number(0.f));
	m_EditMaskValue->setValidator(new QDoubleValidator);
	m_EditMaskValue->setToolTip(Format("Value which will be set in source outside of mask."));

	m_PbMask = new QPushButton("Mask Source", this);
	m_PbMask->setToolTip(Format("Mask source image based on target. The source image is set to zero at zero target pixels."));

	unsigned short slicenr = m_Handler3D->ActiveSlice() + 1;
	m_PbFirst = new QPushButton("|<<", this);
	m_ScbSlicenr = new QScrollBar(Qt::Horizontal, this);
	m_ScbSlicenr->setRange(1, m_Handler3D->NumSlices());
	m_ScbSlicenr->setSingleStep(1);
	m_ScbSlicenr->setPageStep(5);
	m_ScbSlicenr->setMinimumWidth(350);
	m_ScbSlicenr->setValue(static_cast<int>(slicenr));
	m_PbLast = new QPushButton(">>|", this);
	m_SbSlicenr = new QSpinBox(1, (int)m_Handler3D->NumSlices(), 1, this);
	m_SbSlicenr->setValue(slicenr);
	m_LbSlicenr = new QLabel(QString(" of ") + QString::number((int)m_Handler3D->NumSlices()), this);

	m_LbStride = new QLabel(QString("Stride: "));
	m_SbStride = new QSpinBox(1, 1000, 1, this);
	m_SbStride->setValue(1);

	m_LbInactivewarning = new QLabel("  3D Inactive Slice!  ", this);
	m_LbInactivewarning->setStyleSheet("QLabel  { color: red; }");

	m_ThresholdWidget = new ThresholdWidgetQt4(m_Handler3D);
	m_Tabwidgets.push_back(m_ThresholdWidget);
	m_HystWidget = new HystereticGrowingWidget(m_Handler3D);
	m_Tabwidgets.push_back(m_HystWidget);
	m_LivewireWidget = new LivewireWidget(m_Handler3D);
	m_Tabwidgets.push_back(m_LivewireWidget);
	m_IftrgWidget = new ImageForestingTransformRegionGrowingWidget(m_Handler3D);
	m_Tabwidgets.push_back(m_IftrgWidget);
	m_FastmarchingWidget = new FastmarchingFuzzyWidget(m_Handler3D);
	m_Tabwidgets.push_back(m_FastmarchingWidget);
	m_WatershedWidget = new WatershedWidget(m_Handler3D);
	m_Tabwidgets.push_back(m_WatershedWidget);
	m_OlcWidget = new OutlineCorrectionWidget(m_Handler3D);
	m_Tabwidgets.push_back(m_OlcWidget);
	m_InterpolationWidget = new InterpolationWidget(m_Handler3D);
	m_Tabwidgets.push_back(m_InterpolationWidget);
	m_SmoothingWidget = new SmoothingWidget(m_Handler3D);
	m_Tabwidgets.push_back(m_SmoothingWidget);
	m_MorphologyWidget = new MorphologyWidget(m_Handler3D);
	m_Tabwidgets.push_back(m_MorphologyWidget);
	m_EdgeWidget = new EdgeWidget(m_Handler3D);
	m_Tabwidgets.push_back(m_EdgeWidget);
	m_FeatureWidget = new FeatureWidget(m_Handler3D);
	m_Tabwidgets.push_back(m_FeatureWidget);
	m_MeasurementWidget = new MeasurementWidget(m_Handler3D);
	m_Tabwidgets.push_back(m_MeasurementWidget);
	m_VesselextrWidget = new VesselWidget(m_Handler3D);
#ifdef PLUGIN_VESSEL_WIDGET
	tabwidgets.push_back(vesselextr_widget);
#endif
	m_PickerWidget = new PickerWidget(m_Handler3D);
	m_Tabwidgets.push_back(m_PickerWidget);
	m_TransformWidget = new TransformWidget(m_Handler3D);
	m_Tabwidgets.push_back(m_TransformWidget);

	for (const auto& dir : plugin_search_dirs)
	{
		bool ok = iseg::plugin::LoadPlugins(dir);
		if (!ok)
		{
			ISEG_WARNING("Could not load plugins from " << dir);
		}
	}

	auto addons = iseg::plugin::PluginRegistry::RegisteredPlugins();
	for (auto a : addons)
	{
		a->InstallSliceHandler(m_Handler3D);
		m_Tabwidgets.push_back(a->CreateWidget());
	}

	auto nrtabbuttons = (unsigned short)m_Tabwidgets.size();
	m_PbTab.resize(nrtabbuttons);
	m_ShowpbTab.resize(nrtabbuttons);
	m_ShowtabAction.resize(nrtabbuttons);

	for (unsigned short i = 0; i < nrtabbuttons; i++)
	{
		m_ShowpbTab[i] = true;

		m_PbTab[i] = new QPushButton(this);
		m_PbTab[i]->setToggleButton(true);
		m_PbTab[i]->setStyleSheet("text-align: left");
		m_PbTab[i]->setMaximumHeight(20);
	}

	m_MethodTab = new QStackedWidget(this);
	m_MethodTab->setFrameStyle(QFrame::Plain);
	m_MethodTab->setLineWidth(2);

	for (size_t i = 0; i < m_Tabwidgets.size(); i++)
	{
		m_Tabwidgets[i]->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
		m_MethodTab->addWidget(m_Tabwidgets[i]);
	}

	{
		unsigned short posi = 0;
		while (posi < nrtabbuttons && !m_ShowpbTab[posi])
			posi++;
		if (posi < nrtabbuttons)
			m_MethodTab->setCurrentWidget(m_Tabwidgets[posi]);
	}

	TabChanged(m_MethodTab->currentIndex());

	UpdateTabvisibility();

	int height_max = 0;
	QSize qs;
	for (size_t i = 0; i < m_Tabwidgets.size(); i++)
	{
		qs = m_Tabwidgets[i]->sizeHint();
		height_max = std::max(height_max, qs.height());
	}
	height_max += 45;

	m_ScaleDialog = new ScaleWork(m_Handler3D, m_MPicpath, this);
	m_ImagemathDialog = new ImageMath(m_Handler3D, this);
	m_ImageoverlayDialog = new ImageOverlay(m_Handler3D, this);

	m_BitstackWidget = new BitsStack(m_Handler3D, this);
	m_OverlayWidget = new ExtoverlayWidget(m_Handler3D, this);
	m_MultidatasetWidget = new MultiDatasetWidget(m_Handler3D, this);

	int height_max1 = height_max;

	m_MNotes = new QTextEdit(this);
	m_MNotes->clear();

	m_Xsliceshower = m_Ysliceshower = nullptr;
	m_SurfaceViewer = nullptr;
	m_VV3D = nullptr;
	m_VV3Dbmp = nullptr;

	if (m_Handler3D->StartSlice() >= slicenr || m_Handler3D->EndSlice() + 1 <= slicenr)
	{
		m_LbInactivewarning->setText(QString("   3D Inactive Slice!   "));
	}
	else
	{
		m_LbInactivewarning->setText(QString(" "));
	}

	QWidget* hbox2w = new QWidget;
	setCentralWidget(hbox2w);

	QWidget* vbox2w = new QWidget;
	QWidget* hbox1w = new QWidget;
	QWidget* hboxslicew = new QWidget;
	QWidget* hboxslicenrw = new QWidget;
	QWidget* hboxstridew = new QWidget;
	m_Vboxbmpw = new QWidget;
	m_Vboxworkw = new QWidget;
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
	hboxbmp1->addWidget(m_LbContrastbmp);
	hboxbmp1->addWidget(m_LeContrastbmpVal);
	hboxbmp1->addWidget(m_LbContrastbmpVal);
	hboxbmp1->addWidget(m_SlContrastbmp);
	hboxbmp1->addWidget(m_LbBrightnessbmp);
	hboxbmp1->addWidget(m_LeBrightnessbmpVal);
	hboxbmp1->addWidget(m_LbBrightnessbmpVal);
	hboxbmp1->addWidget(m_SlBrightnessbmp);
	hboxbmp1w->setLayout(hboxbmp1);

	QHBoxLayout* hboxbmp = new QHBoxLayout;
	hboxbmp->setSpacing(0);
	hboxbmp->setMargin(0);
	hboxbmp->addWidget(m_CbBmptissuevisible, 1);
	hboxbmp->addWidget(m_CbBmpcrosshairvisible, 1);
	hboxbmp->addWidget(m_CbBmpoutlinevisible);
	hboxbmp->addWidget(bt_select_outline_color);
	hboxbmpw->setLayout(hboxbmp);

	QVBoxLayout* vboxbmp = new QVBoxLayout;
	vboxbmp->setSpacing(0);
	vboxbmp->setMargin(0);
	vboxbmp->addWidget(m_LbSource);
	vboxbmp->addWidget(hboxbmp1w);
	vboxbmp->addWidget(m_BmpScroller);
	vboxbmp->addWidget(hboxbmpw);
	m_Vboxbmpw->setLayout(vboxbmp);

	QVBoxLayout* vbox1 = new QVBoxLayout;
	vbox1->setSpacing(0);
	vbox1->setMargin(0);
	vbox1->addWidget(m_ToworkBtn);
	vbox1->addWidget(m_TobmpBtn);
	vbox1->addWidget(m_SwapBtn);
	vbox1->addWidget(m_SwapAllBtn);
	vbox1w->setLayout(vbox1);

	QHBoxLayout* hboxwork1 = new QHBoxLayout;
	hboxwork1->setSpacing(0);
	hboxwork1->setMargin(0);
	hboxwork1->addWidget(m_LbContrastwork);
	hboxwork1->addWidget(m_LeContrastworkVal);
	hboxwork1->addWidget(m_LbContrastworkVal);
	hboxwork1->addWidget(m_SlContrastwork);
	hboxwork1->addWidget(m_LbBrightnesswork);
	hboxwork1->addWidget(m_LeBrightnessworkVal);
	hboxwork1->addWidget(m_LbBrightnessworkVal);
	hboxwork1->addWidget(m_SlBrightnesswork);
	hboxwork1w->setLayout(hboxwork1);

	QHBoxLayout* hboxwork = new QHBoxLayout;
	hboxwork->setSpacing(0);
	hboxwork->setMargin(0);
	hboxwork->addWidget(m_CbWorktissuevisible, 1);
	hboxwork->addWidget(m_CbWorkcrosshairvisible, 1);
	hboxwork->addWidget(m_CbWorkpicturevisible);
	hboxworkw->setLayout(hboxwork);

	QVBoxLayout* vboxwork = new QVBoxLayout;
	vboxwork->setSpacing(0);
	vboxwork->setMargin(0);
	vboxwork->addWidget(m_LbTarget);
	vboxwork->addWidget(hboxwork1w);
	vboxwork->addWidget(m_WorkScroller);
	vboxwork->addWidget(hboxworkw);
	m_Vboxworkw->setLayout(vboxwork);

	QHBoxLayout* hbox1 = new QHBoxLayout;
	hbox1->setSpacing(0);
	hbox1->setMargin(0);
	hbox1->addWidget(m_Vboxbmpw);
	hbox1->addWidget(vbox1w);
	hbox1->addWidget(m_Vboxworkw);
	hbox1w->setLayout(hbox1);

	QHBoxLayout* hboxslicenr = new QHBoxLayout;
	hboxslicenr->setSpacing(0);
	hboxslicenr->setMargin(0);
	hboxslicenr->addWidget(m_PbFirst);
	hboxslicenr->addWidget(m_ScbSlicenr);
	hboxslicenr->addWidget(m_PbLast);
	hboxslicenr->addWidget(m_SbSlicenr);
	hboxslicenr->addWidget(m_LbSlicenr);
	hboxslicenr->addWidget(m_LbInactivewarning);
	hboxslicenrw->setLayout(hboxslicenr);

	QHBoxLayout* hboxstride = new QHBoxLayout;
	hboxstride->setSpacing(0);
	hboxstride->setMargin(0);
	hboxstride->addWidget(m_LbStride);
	hboxstride->addWidget(m_SbStride);
	hboxstridew->setLayout(hboxstride);

	QHBoxLayout* hboxslice = new QHBoxLayout;
	hboxslice->setSpacing(0);
	hboxslice->setMargin(0);
	hboxslice->addWidget(hboxslicenrw);
	hboxslice->addStretch();
	hboxslice->addWidget(hboxstridew);
	hboxslicew->setLayout(hboxslice);

	auto add_widget_filter = [this](QVBoxLayout* vbox, QPushButton* pb, unsigned short i) {
#ifdef PLUGIN_VESSEL_WIDGET
		vbox->addWidget(pb);
#else
		if (m_Tabwidgets[i] != m_VesselextrWidget)
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
		add_widget_filter(vboxtabs1, m_PbTab[i], i);
	}
	vboxtabs1w->setLayout(vboxtabs1);
	QVBoxLayout* vboxtabs2 = new QVBoxLayout;
	vboxtabs2->setSpacing(0);
	vboxtabs2->setMargin(0);
	for (unsigned short i = (nrtabbuttons + 1) / 2; i < nrtabbuttons; i++)
	{
		add_widget_filter(vboxtabs2, m_PbTab[i], i);
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
	tabswdock->setObjectName("Methods");
	tabswdock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
	tabswdock->setWidget(hboxtabsw);
	addDockWidget(Qt::BottomDockWidgetArea, tabswdock);

	QDockWidget* method_tabdock = new QDockWidget(tr("Parameters"), this);
	method_tabdock->setObjectName("Parameters");
	method_tabdock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
	method_tabdock->setWidget(m_MethodTab);
	addDockWidget(Qt::BottomDockWidgetArea, method_tabdock);

	QVBoxLayout* vboxnotes = new QVBoxLayout;
	vboxnotes->setSpacing(0);
	vboxnotes->setMargin(0);

	QDockWidget* notesdock = new QDockWidget(tr("Notes"), this);
	notesdock->setObjectName("Notes");
	notesdock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
	notesdock->setWidget(m_MNotes);
	addDockWidget(Qt::BottomDockWidgetArea, notesdock);

	QDockWidget* bitstackdock = new QDockWidget(tr("Img Clipboard"), this);
	bitstackdock->setObjectName("Clipboard");
	bitstackdock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
	bitstackdock->setWidget(m_BitstackWidget);
	addDockWidget(Qt::BottomDockWidgetArea, bitstackdock);

	QDockWidget* multi_dataset_dock = new QDockWidget(tr("Multi Dataset"), this);
	multi_dataset_dock->setObjectName("Multi Dataset");
	multi_dataset_dock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
	multi_dataset_dock->setWidget(m_MultidatasetWidget);
	addDockWidget(Qt::BottomDockWidgetArea, multi_dataset_dock);

	QDockWidget* overlaydock = new QDockWidget(tr("Overlay"), this);
	overlaydock->setObjectName("Overlay");
	overlaydock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
	overlaydock->setWidget(m_OverlayWidget);
	addDockWidget(Qt::BottomDockWidgetArea, overlaydock);

	auto log_window_dock = new QDockWidget("Log", this);
	log_window_dock->setObjectName("Log Messages");
	log_window_dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
	log_window_dock->setWidget(m_LogWindow);
	addDockWidget(Qt::BottomDockWidgetArea, log_window_dock);

	QVBoxLayout* vbox2 = new QVBoxLayout;
	vbox2->setSpacing(0);
	vbox2->setMargin(0);
	vbox2->addWidget(hbox1w);
	vbox2->addWidget(hboxslicew);
	vbox2w->setLayout(vbox2);

	QHBoxLayout* hboxlock = new QHBoxLayout;
	hboxlock->setSpacing(0);
	hboxlock->setMargin(0);
	hboxlock->addWidget(m_CbTissuelock);
	hboxlock->addWidget(m_LockTissues);
	hboxlock->addStretch();
	hboxlockw->setLayout(hboxlock);

	QVBoxLayout* vboxtissue = new QVBoxLayout;
	vboxtissue->setSpacing(0);
	vboxtissue->setMargin(0);
	vboxtissue->addWidget(m_TissueFilter);
	vboxtissue->addWidget(m_TissueTreeWidget);
	vboxtissue->addWidget(hboxlockw);
	vboxtissue->addWidget(m_AddTissue);
	vboxtissue->addWidget(m_AddFolder);
	vboxtissue->addWidget(m_ModifyTissueFolder);

	QHBoxLayout* hboxtissueremove = new QHBoxLayout;
	hboxtissueremove->addWidget(m_RemoveTissueFolder);
	hboxtissueremove->addWidget(m_RemoveTissueFolderAll);
	vboxtissue->addLayout(hboxtissueremove);
	vboxtissue->addWidget(m_Tissue3Dopt);

	QHBoxLayout* hboxtissueget = new QHBoxLayout;
	hboxtissueget->addWidget(m_GetTissue);
	hboxtissueget->addWidget(m_GetTissueAll);
	vboxtissue->addLayout(hboxtissueget);

	QHBoxLayout* hboxtissueclear = new QHBoxLayout;
	hboxtissueclear->addWidget(m_ClearTissue);
	hboxtissueclear->addWidget(m_ClearTissues);
	vboxtissue->addLayout(hboxtissueclear);
	vboxtissuew->setLayout(vboxtissue);

	QVBoxLayout* vboxtissueadder1 = new QVBoxLayout;
	vboxtissueadder1->setSpacing(0);
	vboxtissueadder1->setMargin(0);
	vboxtissueadder1->addWidget(m_CbAddsub3d);
	vboxtissueadder1->addWidget(m_CbAddsuboverride);
	vboxtissueadder1->addWidget(m_PbAdd);
	vboxtissueadder1->addWidget(m_PbSub);
	vboxtissueadder1w->setLayout(vboxtissueadder1);
	vboxtissueadder1w->setFixedHeight(vboxtissueadder1->sizeHint().height());

	QVBoxLayout* vboxtissueadder2 = new QVBoxLayout;
	vboxtissueadder2->setSpacing(0);
	vboxtissueadder2->setMargin(0);
	vboxtissueadder2->addWidget(m_CbAddsubconn);
	vboxtissueadder2->addWidget(new QLabel(" ", this));
	vboxtissueadder2->addWidget(m_PbAddhold);
	vboxtissueadder2->addWidget(m_PbSubhold);
	vboxtissueadder2w->setLayout(vboxtissueadder2);
	vboxtissueadder2w->setFixedHeight(vboxtissueadder1->sizeHint().height());

	QHBoxLayout* hboxtissueadder = new QHBoxLayout;
	hboxtissueadder->setSpacing(0);
	hboxtissueadder->setMargin(0);
	hboxtissueadder->addWidget(vboxtissueadder1w);
	hboxtissueadder->addWidget(vboxtissueadder2w);
	hboxtissueadderw->setLayout(hboxtissueadder);
	hboxtissueadderw->setFixedHeight(hboxtissueadder->sizeHint().height());

	QDockWidget* zoomdock = new QDockWidget(tr("Zoom"), this);
	zoomdock->setObjectName("Zoom");
	zoomdock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	zoomdock->setWidget(m_ZoomWidget);
	addDockWidget(Qt::RightDockWidgetArea, zoomdock);

	QDockWidget* tissuewdock = new QDockWidget(tr("Tissues"), this);
	tissuewdock->setObjectName("Tissues");
	tissuewdock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	tissuewdock->setWidget(vboxtissuew);
	addDockWidget(Qt::RightDockWidgetArea, tissuewdock);
	m_TissuesDock = tissuewdock;

	QDockWidget* tissuehierarchydock = new QDockWidget(tr("Tissue Hierarchy"), this);
	tissuehierarchydock->setToolTip(Format("The tissue hierarchy allows to group and organize complex segmentations into a hierarchy."));
	tissuehierarchydock->setObjectName("Tissue Hierarchy");
	tissuehierarchydock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	tissuehierarchydock->setWidget(m_TissueHierarchyWidget);
	addDockWidget(Qt::RightDockWidgetArea, tissuehierarchydock);

	QDockWidget* tissueadddock = new QDockWidget(tr("Adder"), this);
	tissueadddock->setToolTip(Format(
			"The tissue adder provides functionality "
			"to add/remove an object selected/picked in "
			"the Target to/from the selected tissue."
			"Note: Only one tissue can be selected."));
	tissueadddock->setObjectName("Adder");
	tissueadddock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	tissueadddock->setWidget(hboxtissueadderw);
	addDockWidget(Qt::RightDockWidgetArea, tissueadddock);

	QHBoxLayout* hboxmaskvalue = new QHBoxLayout;
	hboxmaskvalue->setSpacing(0);
	hboxmaskvalue->setMargin(0);
	hboxmaskvalue->addWidget(lb_mask_value);
	hboxmaskvalue->addWidget(m_EditMaskValue);

	QVBoxLayout* vboxmasking = new QVBoxLayout;
	vboxmasking->setSpacing(0);
	vboxmasking->setMargin(0);
	vboxmasking->addWidget(m_CbMask3d);
	vboxmasking->addLayout(hboxmaskvalue);
	vboxmasking->addWidget(m_PbMask);

	QWidget* vboxmaskingw = new QWidget;
	vboxmaskingw->setLayout(vboxmasking);

	QDockWidget* maskdock = new QDockWidget(tr("Mask"), this);
	maskdock->setObjectName("Mask");
	maskdock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	maskdock->setWidget(vboxmaskingw);
	maskdock->setVisible(false); // default hide
	addDockWidget(Qt::RightDockWidgetArea, maskdock);

	hboxslice->addStretch();
	hboxslice->addStretch();

	QHBoxLayout* hbox2 = new QHBoxLayout;
	hbox2->setSpacing(0);
	hbox2->setMargin(0);
	hbox2->addWidget(vbox2w);
	hbox2w->setLayout(hbox2);

	//xxxxxxxxxxxxxxxxx

	m_Menubar = menuBar();

	//file = menuBar()->addMenu(tr("&File"));
	m_File = new MenuWTT();
	m_File->setTitle(tr("&File"));
	menuBar()->addMenu(m_File);
	if (!m_MEditingmode)
	{
		m_File->addAction(QIcon(m_MPicpath.absoluteFilePath("filenew.png")), "&New...", this, SLOT(ExecuteNew()));
		m_Loadmenu = m_File->addMenu("&Open");
		m_Loadmenu->addAction("Open Dicom (.dcm...)", this, SLOT(ExecuteLoaddicom()));
		m_Loadmenu->addAction("Open Image Series (.bmp, .png, ...)", this, SLOT(ExecuteLoadImageSeries()));
		m_Loadmenu->addAction("Open Medical Image (.mhd, .nii, ...)", this, SLOT(ExecuteLoadMedicalImage()));
		m_Loadmenu->addAction("Open Raw (.raw...)", this, SLOT(ExecuteLoadraw()));
		m_Loadmenu->addAction("Open Analyze Direct (.avw...)", this, SLOT(ExecuteLoadavw()));
		m_Loadmenu->addAction("Open VTK...", this, SLOT(ExecuteLoadvtk()));
		m_Loadmenu->addAction("Open RTdose...", this, SLOT(ExecuteLoadrtdose()));
	}
	m_Reloadmenu = m_File->addMenu("&Reopen");
	m_Reloadmenu->addAction("Reopen Dicom (.dcm...)", this, SLOT(ExecuteReloaddicom()));
	m_Reloadmenu->addAction("Reopen Image Series (.bmp)", this, SLOT(ExecuteReloadbmp()));
	m_Reloadmenu->addAction("Reopen RAW (.raw...)", this, SLOT(ExecuteReloadraw()));
	m_Reloadmenu->addAction("Reopen Medical Image (.mhd, .nii, ...)", this, SLOT(ExecuteReloadMedicalImage()));
	m_Reloadmenu->addAction("Reopen Analyze Direct (.avw...)", this, SLOT(ExecuteReloadavw()));
	m_Reloadmenu->addAction("Reopen VTK...", this, SLOT(ExecuteReloadvtk()));
	m_Reloadmenu->addAction("Reopen RTdose...", this, SLOT(ExecuteReloadrtdose()));

	if (!m_MEditingmode)
	{
		open_s4_l_link_pos = m_File->actions().size();
		QAction* import_s4_l_link = m_File->addAction("Import S4L-link (h5)...");
		QObject_connect(import_s4_l_link, SIGNAL(triggered()), this, SLOT(ExecuteLoads4llivelink()));
		import_s4_l_link->setToolTip("Loads a Sim4Life live link file (iSEG project .h5 file).");
		import_s4_l_link->setEnabled(true);
	}

	import_surface_pos = m_File->actions().size();
	QAction* import_surface = m_File->addAction("Import Surface/Lines...");
	QObject_connect(import_surface, SIGNAL(triggered()), this, SLOT(ExecuteLoadsurface()));
	import_surface->setToolTip("Voxelize a surface or polyline mesh in the Target image.");
	import_surface->setEnabled(true);

	import_r_tstruct_pos = m_File->actions().size();
	QAction* import_rt_action = m_File->addAction("Import RTstruct...");
	QObject_connect(import_rt_action, SIGNAL(triggered()), this, SLOT(ExecuteLoadrtstruct()));
	import_rt_action->setToolTip("Some data must be opened first to import its RTStruct file");

	m_File->addAction("&Export Image(s)...", this, SLOT(ExecuteSaveimg()));
	m_File->addAction("Export &Contour...", this, SLOT(ExecuteSaveContours()));

	m_Exportmenu = m_File->addMenu("Export Tissue Distr.");
	m_Exportmenu->addAction("Export &Labelfield...(am)", this, SLOT(ExecuteExportlabelfield()));
	m_Exportmenu->addAction("Export vtk-ascii...(vti/vtk)", this, SLOT(ExecuteExportvtkascii()));
	m_Exportmenu->addAction("Export vtk-binary...(vti/vtk)", this, SLOT(ExecuteExportvtkbinary()));
	m_Exportmenu->addAction("Export vtk-compressed-ascii...(vti)", this, SLOT(ExecuteExportvtkcompressedascii()));
	m_Exportmenu->addAction("Export vtk-compressed-binary...(vti)", this, SLOT(ExecuteExportvtkcompressedbinary()));
	m_Exportmenu->addAction("Export Matlab...(mat)", this, SLOT(ExecuteExportmat()));
	m_Exportmenu->addAction("Export hdf...(h5)", this, SLOT(ExecuteExporthdf()));
	m_Exportmenu->addAction("Export xml-extent index...(xml)", this, SLOT(ExecuteExportxmlregionextent()));
	m_Exportmenu->addAction("Export tissue index...(txt)", this, SLOT(ExecuteExporttissueindex()));
	m_File->addAction("Export Color Lookup...", this, SLOT(ExecuteSavecolorlookup()));
	m_File->addSeparator();

	if (!m_MEditingmode)
		m_File->addAction("Save &Project as...", this, SLOT(ExecuteSaveprojas()));
	else
		m_File->addAction("Save &Project-Copy as...", this, SLOT(ExecuteSavecopyas()));
	m_File->addAction(QIcon(m_MPicpath.absoluteFilePath("filesave.png")), "Save Pro&ject", this, SLOT(ExecuteSaveproj()), QKeySequence("Ctrl+S"));
	m_File->addAction("Save Active Slices...", this, SLOT(ExecuteSaveactiveslicesas()));
	if (!m_MEditingmode)
	{
		m_File->addAction(QIcon(m_MPicpath.absoluteFilePath("fileopen.png")), "Open P&roject...", this, SLOT(ExecuteLoadproj()));
	}
	m_File->addSeparator();
	m_File->addAction("Save &Tissuelist...", this, SLOT(ExecuteSaveTissues()));
	m_File->addAction("Open T&issuelist...", this, SLOT(ExecuteLoadTissues()));
	m_File->addAction("Set Tissuelist as Default", this, SLOT(ExecuteSettissuesasdef()));
	m_File->addAction("Remove Default Tissuelist", this, SLOT(ExecuteRemovedeftissues()));
	m_File->addSeparator();
	if (!m_MEditingmode)
	{
		m_MLoadprojfilename.m_LoadRecentProjects[0] = m_File->addAction("", this, SLOT(ExecuteLoadproj1()));
		m_MLoadprojfilename.m_LoadRecentProjects[1] = m_File->addAction("", this, SLOT(ExecuteLoadproj2()));
		m_MLoadprojfilename.m_LoadRecentProjects[2] = m_File->addAction("", this, SLOT(ExecuteLoadproj3()));
		m_MLoadprojfilename.m_LoadRecentProjects[3] = m_File->addAction("", this, SLOT(ExecuteLoadproj4()));
		m_MLoadprojfilename.m_Separator = m_File->addSeparator();

		m_MLoadprojfilename.m_LoadRecentProjects[0]->setVisible(false);
		m_MLoadprojfilename.m_LoadRecentProjects[1]->setVisible(false);
		m_MLoadprojfilename.m_LoadRecentProjects[2]->setVisible(false);
		m_MLoadprojfilename.m_LoadRecentProjects[3]->setVisible(false);
		m_MLoadprojfilename.m_Separator->setVisible(false);
	}
	//	file->addAction( "E&xit", qApp,  SLOT(quit()), CTRL+Key_Q );
	m_File->addAction("E&xit", this, SLOT(close()), QKeySequence("Ctrl+Q"));
	m_Imagemenu = menuBar()->addMenu(tr("&Image"));
	m_Imagemenu->addAction("&Pixelsize...", this, SLOT(ExecutePixelsize()));
	m_Imagemenu->addAction("Offset...", this, SLOT(ExecuteDisplacement()));
	m_Imagemenu->addAction("Rotation...", this, SLOT(ExecuteRotation()));
	//	imagemenu->addAction( "Resize...", this,  SLOT(ExecuteResize()));
	if (!m_MEditingmode)
	{
		m_Imagemenu->addAction("Pad...", this, SLOT(ExecutePad()));
		m_Imagemenu->addAction("Crop...", this, SLOT(ExecuteCrop()));
	}
	m_Imagemenu->addAction(QIcon(m_MPicpath.absoluteFilePath("histo.png")), "&Histogram...", this, SLOT(ExecuteHisto()));
	m_Imagemenu->addAction("&Contr./Bright. ...", this, SLOT(ExecuteScale()));
	m_Imagemenu->addAction("&Image Math. ...", this, SLOT(ExecuteImagemath()));
	m_Imagemenu->addAction("Unwrap", this, SLOT(ExecuteUnwrap()));
	m_Imagemenu->addAction("Overlay...", this, SLOT(ExecuteOverlay()));
	m_Imagemenu->addSeparator();
	m_Imagemenu->addAction("&x Sliced", this, SLOT(ExecuteXslice()));
	m_Imagemenu->addAction("&y Sliced", this, SLOT(ExecuteYslice()));
	m_Imagemenu->addAction("Tissue surface view", this, SLOT(ExecuteTissueSurfaceviewer()));
	m_Imagemenu->addAction("Target surface view", this, SLOT(ExecuteTargetSurfaceviewer()));
	m_Imagemenu->addAction("Source iso-surface view", this, SLOT(ExecuteSourceSurfaceviewer()));
	m_Imagemenu->addAction("Tissue volume view", this, SLOT(Execute3Dvolumeviewertissue()));
	m_Imagemenu->addAction("Source volume view", this, SLOT(Execute3Dvolumeviewerbmp()));
	if (!m_MEditingmode)
	{
		//xxxa;
		m_Imagemenu->addSeparator();
		m_Imagemenu->addAction("Swap xy", this, SLOT(ExecuteSwapxy()));
		m_Imagemenu->addAction("Swap xz", this, SLOT(ExecuteSwapxz()));
		m_Imagemenu->addAction("Swap yz", this, SLOT(ExecuteSwapyz()));
	}

	m_Editmenu = menuBar()->addMenu(tr("E&dit"));
	m_Undonr = m_Editmenu->addAction(QIcon(m_MPicpath.absoluteFilePath("undo.png")), "&Undo", this, SLOT(ExecuteUndo()));
	m_Redonr = m_Editmenu->addAction(QIcon(m_MPicpath.absoluteFilePath("redo.png")), "Redo", this, SLOT(ExecuteRedo()));
	m_Editmenu->addSeparator();
	m_Editmenu->addAction("&Configure Undo...", this, SLOT(ExecuteUndoconf()));
	m_Editmenu->addAction("&Active Slices...", this, SLOT(ExecuteActiveslicesconf()));
	m_Undonr->setEnabled(false);
	m_Redonr->setEnabled(false);
	m_Editmenu->addAction("&Settings...", this, SLOT(ExecuteSettings()));

	m_Viewmenu = menuBar()->addMenu(tr("&View"));
	m_Hidemenu = m_Viewmenu->addMenu("&Toolbars");
	m_Hidesubmenu = m_Viewmenu->addMenu("Methods");
	m_Hidemenu->addAction(tabswdock->toggleViewAction());
	m_Hidemenu->addAction(method_tabdock->toggleViewAction());
	m_Hidemenu->addAction(notesdock->toggleViewAction());
	m_Hidemenu->addAction(log_window_dock->toggleViewAction());
	m_Hidemenu->addAction(bitstackdock->toggleViewAction());
	m_Hidemenu->addAction(zoomdock->toggleViewAction());
	m_Hidemenu->addAction(tissuewdock->toggleViewAction());
	m_Hidemenu->addAction(tissueadddock->toggleViewAction());
	m_Hidemenu->addAction(tissuehierarchydock->toggleViewAction());
	m_Hidemenu->addAction(overlaydock->toggleViewAction());
	m_Hidemenu->addAction(multi_dataset_dock->toggleViewAction());
	m_Hidemenu->addAction(maskdock->toggleViewAction());

	m_Hidecontrastbright = new QAction("Contr./Bright.", this);
	m_Hidecontrastbright->setCheckable(true);
	m_Hidecontrastbright->setChecked(true);
	QObject_connect(m_Hidecontrastbright, SIGNAL(toggled(bool)), this, SLOT(ExecuteHidecontrastbright(bool)));
	m_Hidemenu->addAction(m_Hidecontrastbright);
	m_Hidesource = new QAction("Source", this);
	m_Hidesource->setCheckable(true);
	m_Hidesource->setChecked(true);
	QObject_connect(m_Hidesource, SIGNAL(toggled(bool)), this, SLOT(ExecuteHidesource(bool)));
	m_Hidemenu->addAction(m_Hidesource);
	m_Hidetarget = new QAction("Target", this);
	m_Hidetarget->setCheckable(true);
	m_Hidetarget->setChecked(true);
	QObject_connect(m_Hidetarget, SIGNAL(toggled(bool)), this, SLOT(ExecuteHidetarget(bool)));
	m_Hidemenu->addAction(m_Hidetarget);
	m_Hidecopyswap = new QAction("Copy/Swap", this);
	m_Hidecopyswap->setCheckable(true);
	m_Hidecopyswap->setChecked(true);
	QObject_connect(m_Hidecopyswap, SIGNAL(toggled(bool)), this, SLOT(ExecuteHidecopyswap(bool)));
	m_Hidemenu->addAction(m_Hidecopyswap);
	for (unsigned short i = 0; i < nrtabbuttons; i++)
	{
		m_ShowtabAction[i] = new QAction(m_Tabwidgets[i]->GetName().c_str(), this);
		m_ShowtabAction[i]->setCheckable(true);
		m_ShowtabAction[i]->setChecked(m_ShowpbTab[i]);
		QObject_connect(m_ShowtabAction[i], SIGNAL(toggled(bool)), this, SLOT(ExecuteShowtabtoggled(bool)));
		m_Hidesubmenu->addAction(m_ShowtabAction[i]);
	}
	m_Hideparameters = new QAction("Simplified", this);
	m_Hideparameters->setCheckable(true);
	m_Hideparameters->setChecked(WidgetInterface::GetHideParams());
	QObject_connect(m_Hideparameters, SIGNAL(toggled(bool)), this, SLOT(ExecuteHideparameters(bool)));
	m_Hidesubmenu->addSeparator();
	m_Hidesubmenu->addAction(m_Hideparameters);

	m_Toolmenu = menuBar()->addMenu(tr("T&ools"));
	m_Toolmenu->addAction("Target->Tissue", this, SLOT(DoWork2tissue()));
	m_Toolmenu->addAction("Target->Tissue grouped...", this, SLOT(DoWork2tissueGrouped()));
	m_Toolmenu->addAction("Tissue->Target", this, SLOT(DoTissue2work()));
	m_Toolmenu->addAction("In&verse Slice Order", this, SLOT(ExecuteInversesliceorder()));
	m_Toolmenu->addSeparator();
	m_Toolmenu->addAction("&Group Tissues...", this, SLOT(ExecuteGrouptissues()));
	m_Toolmenu->addAction("Remove Tissues...", this, SLOT(ExecuteRemovetissues()));
	if (!m_MEditingmode)
	{
		m_Toolmenu->addAction("Merge Projects...", this, SLOT(ExecuteMergeprojects()));
	}
	m_Toolmenu->addAction("Remove Unused Tissues", this, SLOT(ExecuteRemoveUnusedTissues()));
	auto action = m_Toolmenu->addAction("Supplant Selected Tissue", this, SLOT(ExecuteVotingReplaceLabels()));
	action->setToolTip("Remove selected tissue by iteratively assigning it to adjacent tissues.");
	m_Toolmenu->addAction("Split Disconnected Tissue Regions", this, SLOT(ExecuteSplitTissue()));
	m_Toolmenu->addAction("Compute Target Connectivity", this, SLOT(ExecuteTargetConnectedComponents()));
	m_Toolmenu->addSeparator();
	m_Toolmenu->addAction("Clean Up", this, SLOT(ExecuteCleanup()));
	m_Toolmenu->addAction("Smooth Steps", this, SLOT(ExecuteSmoothsteps()));
	m_Toolmenu->addAction("Check Bone Connectivity", this, SLOT(ExecuteBoneconnectivity()));

	m_Atlasmenu = menuBar()->addMenu(tr("Atlas"));
	// todo: make atlas method generic, i.e. for loop
	// see e.g. https://stackoverflow.com/questions/9187538/how-to-add-a-list-of-qactions-to-a-qmenu-and-handle-them-with-a-single-slot
	m_MAtlasfilename.m_Atlasnr[0] = m_Atlasmenu->addAction("", this, SLOT(ExecuteLoadatlas0()));
	m_MAtlasfilename.m_Atlasnr[1] = m_Atlasmenu->addAction("", this, SLOT(ExecuteLoadatlas1()));
	m_MAtlasfilename.m_Atlasnr[2] = m_Atlasmenu->addAction("", this, SLOT(ExecuteLoadatlas2()));
	m_MAtlasfilename.m_Atlasnr[3] = m_Atlasmenu->addAction("", this, SLOT(ExecuteLoadatlas3()));
	m_MAtlasfilename.m_Atlasnr[4] = m_Atlasmenu->addAction("", this, SLOT(ExecuteLoadatlas4()));
	m_MAtlasfilename.m_Atlasnr[5] = m_Atlasmenu->addAction("", this, SLOT(ExecuteLoadatlas5()));
	m_MAtlasfilename.m_Atlasnr[6] = m_Atlasmenu->addAction("", this, SLOT(ExecuteLoadatlas6()));
	m_MAtlasfilename.m_Atlasnr[7] = m_Atlasmenu->addAction("", this, SLOT(ExecuteLoadatlas7()));
	m_MAtlasfilename.m_Atlasnr[8] = m_Atlasmenu->addAction("", this, SLOT(ExecuteLoadatlas8()));
	m_MAtlasfilename.m_Atlasnr[9] = m_Atlasmenu->addAction("", this, SLOT(ExecuteLoadatlas9()));
	m_MAtlasfilename.m_Atlasnr[10] = m_Atlasmenu->addAction("", this, SLOT(ExecuteLoadatlas10()));
	m_MAtlasfilename.m_Atlasnr[11] = m_Atlasmenu->addAction("", this, SLOT(ExecuteLoadatlas11()));
	m_MAtlasfilename.m_Atlasnr[12] = m_Atlasmenu->addAction("", this, SLOT(ExecuteLoadatlas12()));
	m_MAtlasfilename.m_Atlasnr[13] = m_Atlasmenu->addAction("", this, SLOT(ExecuteLoadatlas13()));
	m_MAtlasfilename.m_Atlasnr[14] = m_Atlasmenu->addAction("", this, SLOT(ExecuteLoadatlas14()));
	m_MAtlasfilename.m_Atlasnr[15] = m_Atlasmenu->addAction("", this, SLOT(ExecuteLoadatlas15()));
	m_MAtlasfilename.m_Atlasnr[16] = m_Atlasmenu->addAction("", this, SLOT(ExecuteLoadatlas16()));
	m_MAtlasfilename.m_Atlasnr[17] = m_Atlasmenu->addAction("", this, SLOT(ExecuteLoadatlas17()));
	m_MAtlasfilename.m_Atlasnr[18] = m_Atlasmenu->addAction("", this, SLOT(ExecuteLoadatlas18()));
	m_MAtlasfilename.m_Atlasnr[19] = m_Atlasmenu->addAction("", this, SLOT(ExecuteLoadatlas19()));
	m_MAtlasfilename.m_Separator = m_Atlasmenu->addSeparator();
	m_Atlasmenu->addAction("Create Atlas...", this, SLOT(ExecuteCreateatlas()));
	m_Atlasmenu->addAction("Update Menu", this, SLOT(ExecuteReloadatlases()));
	m_MAtlasfilename.m_Atlasnr[0]->setVisible(false);
	m_MAtlasfilename.m_Atlasnr[1]->setVisible(false);
	m_MAtlasfilename.m_Atlasnr[2]->setVisible(false);
	m_MAtlasfilename.m_Atlasnr[3]->setVisible(false);
	m_MAtlasfilename.m_Atlasnr[4]->setVisible(false);
	m_MAtlasfilename.m_Atlasnr[5]->setVisible(false);
	m_MAtlasfilename.m_Atlasnr[6]->setVisible(false);
	m_MAtlasfilename.m_Atlasnr[7]->setVisible(false);
	m_MAtlasfilename.m_Atlasnr[8]->setVisible(false);
	m_MAtlasfilename.m_Atlasnr[9]->setVisible(false);
	m_MAtlasfilename.m_Atlasnr[10]->setVisible(false);
	m_MAtlasfilename.m_Atlasnr[11]->setVisible(false);
	m_MAtlasfilename.m_Atlasnr[12]->setVisible(false);
	m_MAtlasfilename.m_Atlasnr[13]->setVisible(false);
	m_MAtlasfilename.m_Atlasnr[14]->setVisible(false);
	m_MAtlasfilename.m_Atlasnr[15]->setVisible(false);
	m_MAtlasfilename.m_Atlasnr[16]->setVisible(false);
	m_MAtlasfilename.m_Atlasnr[17]->setVisible(false);
	m_MAtlasfilename.m_Atlasnr[18]->setVisible(false);
	m_MAtlasfilename.m_Atlasnr[19]->setVisible(false);
	m_MAtlasfilename.m_Separator->setVisible(false);

	m_Helpmenu = menuBar()->addMenu(tr("Help"));
	m_Helpmenu->addAction(QIcon(m_MPicpath.absoluteFilePath("help.png")), "About", this, SLOT(ExecuteAbout()));

	QObject_connect(m_ToworkBtn, SIGNAL(clicked()), this, SLOT(ExecuteBmp2work()));
	QObject_connect(m_TobmpBtn, SIGNAL(clicked()), this, SLOT(ExecuteWork2bmp()));
	QObject_connect(m_SwapBtn, SIGNAL(clicked()), this, SLOT(ExecuteSwapBmpwork()));
	QObject_connect(m_SwapAllBtn, SIGNAL(clicked()), this, SLOT(ExecuteSwapBmpworkall()));
	QObject_connect(this, SIGNAL(BmpChanged()), this, SLOT(UpdateBmp()));
	QObject_connect(this, SIGNAL(WorkChanged()), this, SLOT(UpdateWork()));
	QObject_connect(this, SIGNAL(WorkChanged()), m_BmpShow, SLOT(WorkborderChanged()));
	QObject_connect(this, SIGNAL(MarksChanged()), m_BmpShow, SLOT(MarkChanged()));
	QObject_connect(this, SIGNAL(MarksChanged()), m_WorkShow, SLOT(MarkChanged()));
	QObject_connect(this, SIGNAL(TissuesChanged()), this, SLOT(UpdateTissue()));
	QObject_connect(m_BmpShow, SIGNAL(AddmarkSign(Point)), this, SLOT(AddMark(Point)));
	QObject_connect(m_BmpShow, SIGNAL(AddlabelSign(Point,std::string)), this, SLOT(AddLabel(Point,std::string)));
	QObject_connect(m_BmpShow, SIGNAL(ClearmarksSign()), this, SLOT(ClearMarks()));
	QObject_connect(m_BmpShow, SIGNAL(RemovemarkSign(Point)), this, SLOT(RemoveMark(Point)));
	QObject_connect(m_BmpShow, SIGNAL(AddtissueSign(Point)), this, SLOT(AddTissue(Point)));
	QObject_connect(m_BmpShow, SIGNAL(AddtissueconnectedSign(Point)), this, SLOT(AddTissueConnected(Point)));
	QObject_connect(m_BmpShow, SIGNAL(SubtissueSign(Point)), this, SLOT(SubtractTissue(Point)));
	QObject_connect(m_BmpShow, SIGNAL(Addtissue3DSign(Point)), this, SLOT(AddTissue3D(Point)));
	QObject_connect(m_BmpShow, SIGNAL(AddtissuelargerSign(Point)), this, SLOT(AddTissuelarger(Point)));
	QObject_connect(m_BmpShow, SIGNAL(SelecttissueSign(Point,bool)), this, SLOT(SelectTissue(Point,bool)));
	QObject_connect(m_BmpShow, SIGNAL(ViewtissueSign(Point)), this, SLOT(ViewTissue(Point)));
	QObject_connect(m_WorkShow, SIGNAL(AddmarkSign(Point)), this, SLOT(AddMark(Point)));
	QObject_connect(m_WorkShow, SIGNAL(AddlabelSign(Point,std::string)), this, SLOT(AddLabel(Point,std::string)));
	QObject_connect(m_WorkShow, SIGNAL(ClearmarksSign()), this, SLOT(ClearMarks()));
	QObject_connect(m_WorkShow, SIGNAL(RemovemarkSign(Point)), this, SLOT(RemoveMark(Point)));
	QObject_connect(m_WorkShow, SIGNAL(AddtissueSign(Point)), this, SLOT(AddTissue(Point)));
	QObject_connect(m_WorkShow, SIGNAL(AddtissueconnectedSign(Point)), this, SLOT(AddTissueConnected(Point)));
	QObject_connect(m_WorkShow, SIGNAL(SubtissueSign(Point)), this, SLOT(SubtractTissue(Point)));
	QObject_connect(m_WorkShow, SIGNAL(Addtissue3DSign(Point)), this, SLOT(AddTissue3D(Point)));
	QObject_connect(m_WorkShow, SIGNAL(AddtissuelargerSign(Point)), this, SLOT(AddTissuelarger(Point)));
	QObject_connect(m_WorkShow, SIGNAL(SelecttissueSign(Point,bool)), this, SLOT(SelectTissue(Point,bool)));
	QObject_connect(m_WorkShow, SIGNAL(ViewtargetSign(Point)), this, SLOT(ExecuteTargetSurfaceviewer()));
	QObject_connect(m_TissueFilter, SIGNAL(textChanged(QString)), this, SLOT(TissueFilterChanged(QString)));
	QObject_connect(m_LockTissues, SIGNAL(clicked()), this, SLOT(LockAllTissues()));
	QObject_connect(m_AddTissue, SIGNAL(clicked()), this, SLOT(NewTissuePressed()));
	QObject_connect(m_AddFolder, SIGNAL(clicked()), this, SLOT(NewFolderPressed()));
	QObject_connect(m_ModifyTissueFolder, SIGNAL(clicked()), this, SLOT(ModifTissueFolderPressed()));
	QObject_connect(m_RemoveTissueFolder, SIGNAL(clicked()), this, SLOT(RemoveTissueFolderPressed()));
	QObject_connect(m_RemoveTissueFolderAll, SIGNAL(clicked()), this, SLOT(RemoveTissueFolderAllPressed()));
	QObject_connect(m_GetTissue, SIGNAL(clicked()), this, SLOT(Selectedtissue2work()));
	QObject_connect(m_GetTissueAll, SIGNAL(clicked()), this, SLOT(Tissue2workall()));
	QObject_connect(m_ClearTissue, SIGNAL(clicked()), this, SLOT(Clearselected()));
	QObject_connect(m_ClearTissues, SIGNAL(clicked()), this, SLOT(Cleartissues()));

	QObject_connect(m_MethodTab, SIGNAL(currentChanged(int)), this, SLOT(TabChanged(int)));

	m_TissueTreeWidget->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);

	QObject_connect(m_TissueTreeWidget, SIGNAL(itemSelectionChanged()), this, SLOT(TissueSelectionChanged()));
	QObject_connect(m_TissueTreeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(TreeWidgetDoubleclicked(QTreeWidgetItem*,int)));
	QObject_connect(m_TissueTreeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(TreeWidgetContextmenu(QPoint)));

	tissues_size_t curr_tissue_type = m_TissueTreeWidget->GetCurrentType();
	m_BmpShow->ColorChanged(curr_tissue_type - 1);
	m_WorkShow->ColorChanged(curr_tissue_type - 1);

	QObject_connect(m_CbBmptissuevisible, SIGNAL(clicked()), this, SLOT(BmptissuevisibleChanged()));
	QObject_connect(m_CbBmpcrosshairvisible, SIGNAL(clicked()), this, SLOT(BmpcrosshairvisibleChanged()));
	QObject_connect(m_CbBmpoutlinevisible, SIGNAL(clicked()), this, SLOT(BmpoutlinevisibleChanged()));
	QObject_connect(bt_select_outline_color, SIGNAL(OnColorChanged(QColor)), this, SLOT(SetOutlineColor(QColor)));
	QObject_connect(m_CbWorktissuevisible, SIGNAL(clicked()), this, SLOT(WorktissuevisibleChanged()));
	QObject_connect(m_CbWorkcrosshairvisible, SIGNAL(clicked()), this, SLOT(WorkcrosshairvisibleChanged()));
	QObject_connect(m_CbWorkpicturevisible, SIGNAL(clicked()), this, SLOT(WorkpicturevisibleChanged()));

	QObject_connect(m_PbFirst, SIGNAL(clicked()), this, SLOT(PbFirstPressed()));
	QObject_connect(m_PbLast, SIGNAL(clicked()), this, SLOT(PbLastPressed()));
	QObject_connect(m_SbSlicenr, SIGNAL(valueChanged(int)), this, SLOT(SbSlicenrChanged()));
	QObject_connect(m_ScbSlicenr, SIGNAL(valueChanged(int)), this, SLOT(ScbSlicenrChanged()));
	QObject_connect(m_SbStride, SIGNAL(valueChanged(int)), this, SLOT(SbStrideChanged()));

	QObject_connect(m_PbAdd, SIGNAL(clicked()), this, SLOT(AddTissuePushed()));
	QObject_connect(m_PbSub, SIGNAL(clicked()), this, SLOT(SubtractTissuePushed()));
	QObject_connect(m_PbAddhold, SIGNAL(clicked()), this, SLOT(AddholdTissuePushed()));
	QObject_connect(m_PbSubhold, SIGNAL(clicked()), this, SLOT(SubtractholdTissuePushed()));

	QObject_connect(m_PbMask, SIGNAL(clicked()), this, SLOT(MaskSource()));

	QObject_connect(m_BmpScroller, SIGNAL(contentsMoving(int,int)), this, SLOT(SetWorkContentsPos(int,int)));
	QObject_connect(m_WorkScroller, SIGNAL(contentsMoving(int,int)), this, SLOT(SetBmpContentsPos(int,int)));
	QObject_connect(m_BmpShow, SIGNAL(SetcenterSign(int,int)), m_BmpScroller, SLOT(center(int,int)));
	QObject_connect(m_WorkShow, SIGNAL(SetcenterSign(int,int)), m_WorkScroller, SLOT(center(int,int)));
	m_TomoveScroller = true;

	QObject_connect(m_ZoomWidget, SIGNAL(SetZoom(double)), m_BmpShow, SLOT(SetZoom(double)));
	QObject_connect(m_ZoomWidget, SIGNAL(SetZoom(double)), m_WorkShow, SLOT(SetZoom(double)));

	QObject_connect(this, SIGNAL(BeginDataexport(DataSelection&,QWidget*)), this, SLOT(HandleBeginDataexport(DataSelection&,QWidget*)));
	QObject_connect(this, SIGNAL(EndDataexport(QWidget*)), this, SLOT(HandleEndDataexport(QWidget*)));

	QObject_connect(this, SIGNAL(BeginDatachange(DataSelection&,QWidget*,bool)), this, SLOT(HandleBeginDatachange(DataSelection&,QWidget*,bool)));
	QObject_connect(this, SIGNAL(EndDatachange(QWidget*,eEndUndoAction)), this, SLOT(HandleEndDatachange(QWidget*,eEndUndoAction)));

	QObject_connect(m_ScaleDialog, SIGNAL(BeginDatachange(DataSelection&,QWidget*,bool)), this, SLOT(HandleBeginDatachange(DataSelection&,QWidget*,bool)));
	QObject_connect(m_ScaleDialog, SIGNAL(EndDatachange(QWidget*,eEndUndoAction)), this, SLOT(HandleEndDatachange(QWidget*,eEndUndoAction)));
	QObject_connect(m_ImagemathDialog, SIGNAL(BeginDatachange(DataSelection&,QWidget*,bool)), this, SLOT(HandleBeginDatachange(DataSelection&,QWidget*,bool)));
	QObject_connect(m_ImagemathDialog, SIGNAL(EndDatachange(QWidget*,eEndUndoAction)), this, SLOT(HandleEndDatachange(QWidget*,eEndUndoAction)));
	QObject_connect(m_ImageoverlayDialog, SIGNAL(BeginDatachange(DataSelection&,QWidget*,bool)), this, SLOT(HandleBeginDatachange(DataSelection&,QWidget*,bool)));
	QObject_connect(m_ImageoverlayDialog, SIGNAL(EndDatachange(QWidget*,eEndUndoAction)), this, SLOT(HandleEndDatachange(QWidget*,eEndUndoAction)));
	QObject_connect(m_BitstackWidget, SIGNAL(BeginDatachange(DataSelection&,QWidget*,bool)), this, SLOT(HandleBeginDatachange(DataSelection&,QWidget*,bool)));
	QObject_connect(m_BitstackWidget, SIGNAL(EndDatachange(QWidget*,eEndUndoAction)), this, SLOT(HandleEndDatachange(QWidget*,eEndUndoAction)));
	QObject_connect(m_BitstackWidget, SIGNAL(BeginDataexport(DataSelection&,QWidget*)), this, SLOT(HandleBeginDataexport(DataSelection&,QWidget*)));
	QObject_connect(m_BitstackWidget, SIGNAL(EndDataexport(QWidget*)), this, SLOT(HandleEndDataexport(QWidget*)));
	QObject_connect(m_OverlayWidget, SIGNAL(OverlayChanged()), m_BmpShow, SLOT(OverlayChanged()));
	QObject_connect(m_OverlayWidget, SIGNAL(OverlayChanged()), m_WorkShow, SLOT(OverlayChanged()));
	QObject_connect(m_OverlayWidget, SIGNAL(OverlayalphaChanged(float)), m_BmpShow, SLOT(SetOverlayalpha(float)));
	QObject_connect(m_OverlayWidget, SIGNAL(OverlayalphaChanged(float)), m_WorkShow, SLOT(SetOverlayalpha(float)));
	QObject_connect(m_OverlayWidget, SIGNAL(BmpoverlayvisibleChanged(bool)), m_BmpShow, SLOT(SetOverlayvisible(bool)));
	QObject_connect(m_OverlayWidget, SIGNAL(WorkoverlayvisibleChanged(bool)), m_WorkShow, SLOT(SetOverlayvisible(bool)));

	QObject_connect(m_MultidatasetWidget, SIGNAL(DatasetChanged()), m_BmpShow, SLOT(OverlayChanged()));
	QObject_connect(m_MultidatasetWidget, SIGNAL(DatasetChanged()), this, SLOT(DatasetChanged()));
	QObject_connect(m_MultidatasetWidget, SIGNAL(BeginDatachange(DataSelection&,QWidget*,bool)), this, SLOT(HandleBeginDatachange(DataSelection&,QWidget*,bool)));
	QObject_connect(m_MultidatasetWidget, SIGNAL(EndDatachange(QWidget*,eEndUndoAction)), this, SLOT(HandleEndDatachange(QWidget*,eEndUndoAction)));

	QObject_connect(m_CbTissuelock, SIGNAL(clicked()), this, SLOT(TissuelockToggled()));

	QObject_connect(m_SlContrastbmp, SIGNAL(valueChanged(int)), this, SLOT(SlContrastbmpMoved(int)));
	QObject_connect(m_SlContrastwork, SIGNAL(valueChanged(int)), this, SLOT(SlContrastworkMoved(int)));
	QObject_connect(m_SlBrightnessbmp, SIGNAL(valueChanged(int)), this, SLOT(SlBrightnessbmpMoved(int)));
	QObject_connect(m_SlBrightnesswork, SIGNAL(valueChanged(int)), this, SLOT(SlBrightnessworkMoved(int)));

	QObject_connect(m_LeContrastbmpVal, SIGNAL(editingFinished()), this, SLOT(LeContrastbmpValEdited()));
	QObject_connect(m_LeContrastworkVal, SIGNAL(editingFinished()), this, SLOT(LeContrastworkValEdited()));
	QObject_connect(m_LeBrightnessbmpVal, SIGNAL(editingFinished()), this, SLOT(LeBrightnessbmpValEdited()));
	QObject_connect(m_LeBrightnessworkVal, SIGNAL(editingFinished()), this, SLOT(LeBrightnessworkValEdited()));

	// \todo BL add generic connections here, e.g. begin/EndDatachange

	m_MWidgetSignalMapper = new QSignalMapper(this);
	for (int i = 0; i < nrtabbuttons; ++i)
	{
		QObject_connect(m_PbTab[i], SIGNAL(clicked()), m_MWidgetSignalMapper, SLOT(map()));
		m_MWidgetSignalMapper->setMapping(m_PbTab[i], i);
	}
	QObject_connect(m_MWidgetSignalMapper, SIGNAL(mapped(int)), this, SLOT(PbTabPressed(int)));
	for (auto widget : m_Tabwidgets)
	{
		assert(widget);
		QObject_connect(widget, SIGNAL(BeginDatachange(DataSelection&,QWidget*,bool)), this, SLOT(HandleBeginDatachange(DataSelection&,QWidget*,bool)));
		QObject_connect(widget, SIGNAL(EndDatachange(QWidget*,eEndUndoAction)), this, SLOT(HandleEndDatachange(QWidget*,eEndUndoAction)));
	}

	QObject_connect(m_BmpShow, SIGNAL(MousePosZoomSign(QPoint)), this, SLOT(MousePosZoomChanged(QPoint)));
	QObject_connect(m_WorkShow, SIGNAL(MousePosZoomSign(QPoint)), this, SLOT(MousePosZoomChanged(QPoint)));

	QObject_connect(m_BmpShow, SIGNAL(WheelrotatedctrlSign(int)), this, SLOT(Wheelrotated(int)));
	QObject_connect(m_WorkShow, SIGNAL(WheelrotatedctrlSign(int)), this, SLOT(Wheelrotated(int)));

	//	QObject_connect(pb_work2tissue,SIGNAL(clicked()),this,SLOT(DoWork2tissue()));

	// shortcuts -> TODO: replace those which have a button/menu action so user can learn about shortcut
	auto shortcut_sliceup = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Right), this);
	QObject_connect(shortcut_sliceup, SIGNAL(activated()), this, SLOT(SlicenrUp()));
	auto shortcut_sliceup1 = new QShortcut(QKeySequence(Qt::Key_PageDown), this);
	QObject_connect(shortcut_sliceup1, SIGNAL(activated()), this, SLOT(SlicenrUp()));

	auto shortcut_slicedown = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Left), this);
	QObject_connect(shortcut_slicedown, SIGNAL(activated()), this, SLOT(SlicenrDown()));
	auto shortcut_slicedown1 = new QShortcut(QKeySequence(Qt::Key_PageUp), this);
	QObject_connect(shortcut_slicedown1, SIGNAL(activated()), this, SLOT(SlicenrDown()));

	auto shortcut_zoomin = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Up), this);
	QObject_connect(shortcut_zoomin, SIGNAL(activated()), this, SLOT(ZoomIn()));
	auto shortcut_zoomout = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Down), this);
	QObject_connect(shortcut_zoomout, SIGNAL(activated()), this, SLOT(ZoomOut()));

	auto shortcut_add = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Plus), this);
	QObject_connect(shortcut_add, SIGNAL(activated()), this, SLOT(AddTissueShortkey()));
	auto shortcut_sub = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Minus), this);
	QObject_connect(shortcut_sub, SIGNAL(activated()), this, SLOT(SubtractTissueShortkey()));

	auto shortcut_undo = new QShortcut(QKeySequence(Qt::Key_Escape), this);
	QObject_connect(shortcut_undo, SIGNAL(activated()), this, SLOT(ExecuteUndo()));
	auto shortcut_undo2 = new QShortcut(QKeySequence("Ctrl+Z"), this);
	QObject_connect(shortcut_undo2, SIGNAL(activated()), this, SLOT(ExecuteUndo()));
	auto shortcut_redo = new QShortcut(QKeySequence("Ctrl+Y"), this);
	QObject_connect(shortcut_redo, SIGNAL(activated()), this, SLOT(ExecuteRedo()));

	UpdateBrightnesscontrast(true);
	UpdateBrightnesscontrast(false);

	TissuenrChanged(curr_tissue_type - 1);

	this->setMinimumHeight(this->minimumHeight() + 50);

	m_Modified = false;
	m_NewDataAfterSwap = false;
}

void MainWindow::closeEvent(QCloseEvent* qce)
{
	if (MaybeSafe())
	{
		if (m_Xsliceshower != nullptr)
		{
			m_Xsliceshower->close();
			delete m_Xsliceshower;
		}
		if (m_Ysliceshower != nullptr)
		{
			m_Ysliceshower->close();
			delete m_Ysliceshower;
		}
		if (m_SurfaceViewer != nullptr)
		{
			m_SurfaceViewer->close();
			delete m_SurfaceViewer;
		}
		if (m_VV3D != nullptr)
		{
			m_VV3D->close();
			delete m_VV3D;
		}
		if (m_VV3Dbmp != nullptr)
		{
			m_VV3Dbmp->close();
			delete m_VV3Dbmp;
		}

		SaveSettings();
		SaveLoadProj(m_MLoadprojfilename.m_CurrentFilename);
		QMainWindow::closeEvent(qce);
	}
	else
	{
		qce->ignore();
	}
}

bool MainWindow::MaybeSafe()
{
	if (Modified())
	{
		int ret = QMessageBox::warning(this, "iSeg", "Do you want to save your changes?", QMessageBox::Yes | QMessageBox::Default, QMessageBox::No, QMessageBox::Cancel | QMessageBox::Escape);
		if (ret == QMessageBox::Yes)
		{
			// Handle tissue hierarchy changes
			if (!m_TissueHierarchyWidget->HandleChangedHierarchy())
			{
				return false;
			}
			if ((!m_MEditingmode && !m_MSaveprojfilename.isEmpty()) ||
					(m_MEditingmode && !m_S4Lcommunicationfilename.isEmpty()))
			{
				ExecuteSaveproj();
			}
			else
			{
				ExecuteSaveprojas();
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

bool MainWindow::Modified() const { return m_Modified; }

void MainWindow::ExecuteBmp2work()
{
	DataSelection data_selection;
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.work = true;
	emit BeginDatachange(data_selection, this);

	m_Handler3D->Bmp2work();

	emit EndDatachange(this);
}

void MainWindow::ExecuteWork2bmp()
{
	DataSelection data_selection;
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.bmp = true;
	emit BeginDatachange(data_selection, this);

	m_Handler3D->Work2bmp();

	emit EndDatachange(this);
}

void MainWindow::ExecuteSwapBmpwork()
{
	DataSelection data_selection;
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.bmp = true;
	data_selection.work = true;
	emit BeginDatachange(data_selection, this);

	m_Handler3D->SwapBmpwork();

	emit EndDatachange(this);
}

void MainWindow::ExecuteSwapBmpworkall()
{
	DataSelection data_selection;
	data_selection.allSlices = true;
	data_selection.bmp = true;
	data_selection.work = true;
	emit BeginDatachange(data_selection, this);

	m_Handler3D->SwapBmpworkall();

	emit EndDatachange(this);
}

void MainWindow::ExecuteSaveContours()
{
	DataSelection data_selection;
	data_selection.tissues = true;
	emit BeginDataexport(data_selection, this);

	SaveOutlinesDialog sow(m_Handler3D, this);
	sow.move(QCursor::pos());
	sow.exec();

	emit EndDataexport(this);
}

void MainWindow::ExecuteSwapxy()
{
	// make sure we don't have surface viewer open
	SurfaceViewerClosed();

	DataSelection data_selection;
	data_selection.allSlices = true;
	data_selection.bmp = true;
	data_selection.work = true;
	data_selection.tissues = true;
	emit BeginDatachange(data_selection, this, false);

	bool ok = m_Handler3D->SwapXY();

	// Swap also the other datasets
	bool swap_extra_datasets = false;
	if (m_MultidatasetWidget->isVisible() && swap_extra_datasets)
	{
		unsigned short w, h, nrslices;
		w = m_Handler3D->Height();
		h = m_Handler3D->Width();
		nrslices = m_Handler3D->NumSlices();
		for (int i = 0; i < m_MultidatasetWidget->GetNumberOfDatasets(); i++)
		{
			// Swap all but the active one
			if (!m_MultidatasetWidget->IsActive(i))
			{
				QString str1 = QDir::temp().absoluteFilePath(QString("bmp_float_eds_%1.raw").arg(i));
				if (SlicesHandler::SaveRawXySwapped(str1.toAscii(), m_MultidatasetWidget->GetBmpData(i), w, h, nrslices) != 0)
					ok = false;

				if (ok)
				{
					m_MultidatasetWidget->SetBmpData(i, SlicesHandler::LoadRawFloat(str1.toAscii(), m_Handler3D->StartSlice(), m_Handler3D->EndSlice(), 0, w * h));
				}
			}
		}

		m_NewDataAfterSwap = true;
	}

	emit EndDatachange(this, iseg::ClearUndo);

	PixelsizeChanged();
	ClearStack();
}

void MainWindow::ExecuteSwapxz()
{
	// make sure we don't have surface viewer open
	SurfaceViewerClosed();

	DataSelection data_selection;
	data_selection.allSlices = true;
	data_selection.bmp = true;
	data_selection.work = true;
	data_selection.tissues = true;
	emit BeginDatachange(data_selection, this, false);

	bool ok = m_Handler3D->SwapXZ();

	// Swap also the other datasets
	bool swap_extra_datasets = false;
	if (m_MultidatasetWidget->isVisible() && swap_extra_datasets)
	{
		unsigned short w, h, nrslices;
		w = m_Handler3D->NumSlices();
		h = m_Handler3D->Height();
		nrslices = m_Handler3D->Width();
		for (int i = 0; i < m_MultidatasetWidget->GetNumberOfDatasets(); i++)
		{
			// Swap all but the active one
			if (!m_MultidatasetWidget->IsActive(i))
			{
				QString str1 = QDir::temp().absoluteFilePath(QString("bmp_float_%1.raw").arg(i));
				if (SlicesHandler::SaveRawXzSwapped(str1.toAscii(), m_MultidatasetWidget->GetBmpData(i), w, h, nrslices) != 0)
					ok = false;

				if (ok)
				{
					m_MultidatasetWidget->SetBmpData(i, SlicesHandler::LoadRawFloat(str1.toAscii(), m_Handler3D->StartSlice(), m_Handler3D->EndSlice(), 0, w * h));
				}
			}
		}
		m_NewDataAfterSwap = true;
	}

	emit EndDatachange(this, iseg::ClearUndo);

	PixelsizeChanged();
	SlicethicknessChanged();
	ClearStack();
}

void MainWindow::ExecuteSwapyz()
{
	// make sure we don't have surface viewer open
	SurfaceViewerClosed();

	DataSelection data_selection;
	data_selection.allSlices = true;
	data_selection.bmp = true;
	data_selection.work = true;
	data_selection.tissues = true;
	emit BeginDatachange(data_selection, this, false);

	bool ok = m_Handler3D->SwapYZ();

	// Swap also the other datasets
	bool swap_extra_datasets = false;
	if (m_MultidatasetWidget->isVisible() && swap_extra_datasets)
	{
		unsigned short w, h, nrslices;
		w = m_Handler3D->Width();
		h = m_Handler3D->NumSlices();
		nrslices = m_Handler3D->Height();
		for (int i = 0; i < m_MultidatasetWidget->GetNumberOfDatasets(); i++)
		{
			// Swap all but the active one
			if (!m_MultidatasetWidget->IsActive(i))
			{
				QString str1 = QDir::temp().absoluteFilePath(QString("bmp_float_%1.raw").arg(i));
				if (SlicesHandler::SaveRawYzSwapped(str1.toAscii(), m_MultidatasetWidget->GetBmpData(i), w, h, nrslices) != 0)
					ok = false;

				if (ok)
				{
					m_MultidatasetWidget->SetBmpData(i, SlicesHandler::LoadRawFloat(str1.toAscii(), m_Handler3D->StartSlice(), m_Handler3D->EndSlice(), 0, w * h));
				}
			}
		}
		m_NewDataAfterSwap = true;
	}

	emit EndDatachange(this, iseg::ClearUndo);

	PixelsizeChanged();
	SlicethicknessChanged();
	ClearStack();
}

void MainWindow::ExecuteResize(int resizetype)
{
	// make sure we don't have surface viewer open
	SurfaceViewerClosed();

	DataSelection data_selection;
	data_selection.allSlices = true;
	data_selection.bmp = true;
	data_selection.work = true;
	data_selection.tissues = true;
	emit BeginDatachange(data_selection, this, false);

	ResizeDialog rd(m_Handler3D, static_cast<ResizeDialog::eResizeType>(resizetype), this);
	rd.move(QCursor::pos());
	if (!rd.exec())
	{
		return;
	}
	auto lut = m_Handler3D->GetColorLookupTable();

	int dxm, dxp, dym, dyp, dzm, dzp;
	rd.ReturnPadding(dxm, dxp, dym, dyp, dzm, dzp);

	unsigned short w, h, nrslices;
	w = m_Handler3D->Width();
	h = m_Handler3D->Height();
	nrslices = m_Handler3D->NumSlices();
	QString str1;
	bool ok = true;
	str1 = QDir::temp().absoluteFilePath("bmp_float.raw");
	if (m_Handler3D->SaveRawResized(str1.toAscii(), dxm, dxp, dym, dyp, dzm, dzp, false) != 0)
		ok = false;
	str1 = QDir::temp().absoluteFilePath("work_float.raw");
	if (m_Handler3D->SaveRawResized(str1.toAscii(), dxm, dxp, dym, dyp, dzm, dzp, true) != 0)
		ok = false;
	str1 = QDir::temp().absoluteFilePath("tissues.raw");
	if (m_Handler3D->SaveTissuesRawResized(str1.toAscii(), dxm, dxp, dym, dyp, dzm, dzp) != 0)
		ok = false;

	if (ok)
	{
		str1 = QDir::temp().absoluteFilePath("work_float.raw");
		if (m_Handler3D->ReadRawFloat(str1.toAscii(), w + dxm + dxp, h + dym + dyp, 0, nrslices + dzm + dzp) != 1)
			ok = false;
	}
	if (ok)
	{
		str1 = QDir::temp().absoluteFilePath("bmp_float.raw");
		if (m_Handler3D->ReloadRawFloat(str1.toAscii(), 0) != 1)
			ok = false;
	}
	if (ok)
	{
		str1 = QDir::temp().absoluteFilePath("tissues.raw");
		if (m_Handler3D->ReloadRawTissues(str1.toAscii(), sizeof(tissues_size_t) * 8, 0) != 1)
		{
			ok = false;
		}
	}

	Transform tr = m_Handler3D->ImageTransform();
	int plo[3] = {dxm, dym, dzm};
	tr.PaddingUpdateTransform(plo, m_Handler3D->Spacing());
	m_Handler3D->SetTransform(tr);

	// add color lookup table again
	m_Handler3D->UpdateColorLookupTable(lut);

	emit EndDatachange(this, iseg::ClearUndo);

	ClearStack();
}

void MainWindow::ExecutePad() { ExecuteResize(1); }

void MainWindow::ExecuteCrop() { ExecuteResize(2); }

void MainWindow::SlContrastbmpMoved(int /* i */)
{
	// Update line edit
	float contrast = pow(10, m_SlContrastbmp->value() * 4.0f / 100.0f - 2.0f);
	m_LeContrastbmpVal->setText(QString("%1").arg(contrast, 6, 'f', 2));

	// Update display
	UpdateBrightnesscontrast(true);
}

void MainWindow::SlContrastworkMoved(int /* i */)
{
	// Update line edit
	float contrast = pow(10, m_SlContrastwork->value() * 4.0f / 100.0f - 2.0f);
	m_LeContrastworkVal->setText(QString("%1").arg(contrast, 6, 'f', 2));

	// Update display
	UpdateBrightnesscontrast(false);
}

void MainWindow::SlBrightnessbmpMoved(int /* i */)
{
	// Update line edit
	m_LeBrightnessbmpVal->setText(QString("%1").arg(m_SlBrightnessbmp->value() - 50, 3));

	// Update display
	UpdateBrightnesscontrast(true);
}

void MainWindow::SlBrightnessworkMoved(int /* i */)
{
	// Update line edit
	m_LeBrightnessworkVal->setText(QString("%1").arg(m_SlBrightnesswork->value() - 50, 3));

	// Update display
	UpdateBrightnesscontrast(false);
}

void MainWindow::LeContrastbmpValEdited()
{
	// Clamp to range and round to precision
	float contrast = std::max(0.01f, std::min(m_LeContrastbmpVal->text().toFloat(), 100.0f));
	m_LeContrastbmpVal->setText(QString("%1").arg(contrast, 6, 'f', 2));

	// Update slider
	int slider_value = std::floor(100.0f * (std::log10(m_LeContrastbmpVal->text().toFloat()) + 2.0f) / 4.0f + 0.5f);
	m_SlContrastbmp->setValue(slider_value);

	// Update display
	UpdateBrightnesscontrast(true);
}

void MainWindow::LeContrastworkValEdited()
{
	// Clamp to range and round to precision
	float contrast =
			std::max(0.01f, std::min(m_LeContrastworkVal->text().toFloat(), 100.0f));
	m_LeContrastworkVal->setText(QString("%1").arg(contrast, 6, 'f', 2));

	// Update slider
	int slider_value = std::floor(100.0f * (std::log10(m_LeContrastworkVal->text().toFloat()) + 2.0f) / 4.0f + 0.5f);
	m_SlContrastwork->setValue(slider_value);

	// Update display
	UpdateBrightnesscontrast(false);
}

void MainWindow::LeBrightnessbmpValEdited()
{
	// Clamp to range and round to precision
	int brightness = (int)std::max(-50.0f, std::min(std::floor(m_LeBrightnessbmpVal->text().toFloat() + 0.5f), 50.0f));
	m_LeBrightnessbmpVal->setText(QString("%1").arg(brightness, 3));

	// Update slider
	m_SlBrightnessbmp->setValue(brightness + 50);

	// Update display
	UpdateBrightnesscontrast(true);
}

void MainWindow::LeBrightnessworkValEdited()
{
	// Clamp to range and round to precision
	int brightness = (int)std::max(-50.0f, std::min(std::floor(m_LeBrightnessworkVal->text().toFloat() + 0.5f), 50.0f));
	m_LeBrightnessworkVal->setText(QString("%1").arg(brightness, 3));

	// Update slider
	m_SlBrightnesswork->setValue(brightness + 50);

	// Update display
	UpdateBrightnesscontrast(false);
}

void MainWindow::ResetBrightnesscontrast()
{
	m_SlContrastbmp->setValue(50);
	m_LeContrastbmpVal->setText(QString("%1").arg(1.0f, 6, 'f', 2));

	m_SlBrightnessbmp->setValue(50);
	m_LeBrightnessbmpVal->setText(QString("%1").arg(0, 3));

	m_SlContrastwork->setValue(50);
	m_LeContrastworkVal->setText(QString("%1").arg(1.0f, 6, 'f', 2));

	m_SlBrightnesswork->setValue(50);
	m_LeBrightnessworkVal->setText(QString("%1").arg(0, 3));

	// Update display
	UpdateBrightnesscontrast(true);
	UpdateBrightnesscontrast(false);

	m_Modified = true;
}

void MainWindow::UpdateBrightnesscontrast(bool bmporwork, bool paint)
{
	if (bmporwork)
	{
		// Get values from line edits
		float contrast = m_LeContrastbmpVal->text().toFloat();
		float brightness = (m_LeBrightnessbmpVal->text().toFloat() + 50.0f) * 0.01f;

		// Update bmp shower
		m_BmpShow->SetBrightnesscontrast(brightness, contrast, paint);
	}
	else
	{
		// Get values from sliders
		float contrast = m_LeContrastworkVal->text().toFloat();
		float brightness =
				(m_LeBrightnessworkVal->text().toFloat() + 50.0f) * 0.01f;

		// Update work shower
		m_WorkShow->SetBrightnesscontrast(brightness, contrast, paint);
	}
}

void MainWindow::EnableActionsAfterPrjLoaded(const bool enable)
{
	auto update = [this](int idx, bool enable) {
		if (idx >= 0)
		{
			if (enable)
				m_File->actions().at(idx)->setToolTip("");
			m_File->actions().at(idx)->setEnabled(enable);
		}
	};

	update(open_s4_l_link_pos, enable);
	update(import_surface_pos, enable);
	update(import_r_tstruct_pos, enable);
}

void MainWindow::ExecuteLoadImageSeries()
{
	if (!MaybeSafe())
	{
		return;
	}

	QStringList files = QFileDialog::getOpenFileNames(
			this, "Select one or more files to open", QString::null, "Images (*.bmp);;Images (*.png);;Images (*.jpg *.jpeg);;Images (*.tif *.tiff);;All (*.*)");

	if (!files.empty())
	{
		files.sort();

		std::vector<int> vi;
		vi.clear();

		short nrelem = static_cast<short>(files.size());
		for (short i = 0; i < nrelem; i++)
		{
			vi.push_back(bmpimgnr(&files[i]));
		}

		std::vector<std::string> vfilenames;
		for (short i = 0; i < nrelem; i++)
		{
			vfilenames.push_back(files[i].toStdString());
		}

		auto format = LoaderColorImages::kBMP;
		if (files.front().endsWith(".png", Qt::CaseInsensitive))
		{
			format = LoaderColorImages::kPNG;
		}
		else if (files.front().endsWith(".jpg", Qt::CaseInsensitive) || files.front().endsWith(".jpeg", Qt::CaseInsensitive))
		{
			format = LoaderColorImages::kJPG;
		}
		else if (files.front().endsWith(".tif", Qt::CaseInsensitive) || files.front().endsWith(".tiff", Qt::CaseInsensitive))
		{
			format = LoaderColorImages::kTIF;
		}

		DataSelection data_selection;
		data_selection.allSlices = true;
		data_selection.bmp = true;
		data_selection.work = true;
		data_selection.tissues = true;
		emit BeginDatachange(data_selection, this, false);

		LoaderColorImages lb(m_Handler3D, format, vfilenames, this);
		lb.move(QCursor::pos());
		lb.exec();

		emit EndDatachange(this, iseg::ClearUndo);

		ResetBrightnesscontrast();

		EnableActionsAfterPrjLoaded(true);
	}
}

void MainWindow::ExecuteLoaddicom()
{
	if (!MaybeSafe())
	{
		return;
	}

	QStringList files = QFileDialog::getOpenFileNames(
			this, "Select one or more files to open", QString::null, "Images (*.dcm *.dicom)\nAll (*)");

	if (!files.empty())
	{
		DataSelection data_selection;
		data_selection.allSlices = true;
		data_selection.bmp = true;
		data_selection.work = true;
		data_selection.tissues = true;
		emit BeginDatachange(data_selection, this, false);

		LoaderDicom ld(m_Handler3D, &files, false, this);
		ld.move(QCursor::pos());
		ld.exec();

		emit EndDatachange(this, iseg::ClearUndo);

		PixelsizeChanged();
		SlicethicknessChanged();

		ResetBrightnesscontrast();

		EnableActionsAfterPrjLoaded(true);
	}
}

void MainWindow::ExecuteReloaddicom()
{
	QStringList files = QFileDialog::getOpenFileNames(
			this, "Select one or more files to open", QString::null, "Images (*.dcm *.dicom)\nAll (*)");

	if (!files.empty())
	{
		DataSelection data_selection;
		data_selection.allSlices = true;
		data_selection.bmp = true;
		data_selection.work = true;
		data_selection.tissues = true;
		emit BeginDatachange(data_selection, this, false);

		LoaderDicom ld(m_Handler3D, &files, true, this);
		ld.move(QCursor::pos());
		ld.exec();
		PixelsizeChanged();
		SlicethicknessChanged();

		//	handler3D->LoadDICOM();

		//	work_show->update();//(bmphand->return_width(),bmphand->return_height());
		//	bmp_show->update();//(bmphand->return_width(),bmphand->return_height());
		//	bmp_show->WorkborderChanged();

		emit EndDatachange(this, iseg::ClearUndo);

		//	hbox1->setFixedSize(bmphand->return_width()*2+vbox1->sizeHint().width(),bmphand->return_height());

		ResetBrightnesscontrast();
	}
}

void MainWindow::ExecuteLoadraw(const QString& file_name)
{
	QString file_path = !file_name.isEmpty() ? file_name : RecentPlaces::GetOpenFileName(this, "Open file", QString::null, QString::null);
	if (file_path.isEmpty())
	{
		return;
	}

	if (!MaybeSafe())
	{
		return;
	}

	DataSelection data_selection;
	data_selection.allSlices = true;
	data_selection.bmp = true;
	data_selection.work = true;
	data_selection.tissues = true;
	emit BeginDatachange(data_selection, this, false);

	LoaderRaw lr(m_Handler3D, file_path, this);
	lr.move(QCursor::pos());
	lr.exec();

	emit EndDatachange(this, iseg::ClearUndo);

	ResetBrightnesscontrast();

	EnableActionsAfterPrjLoaded(true);
}

void MainWindow::ExecuteLoadMedicalImage(const QString& file_name)
{
	if (!MaybeSafe())
	{
		return;
	}

	DataSelection data_selection;
	data_selection.allSlices = true;
	data_selection.bmp = true;
	data_selection.work = true;
	data_selection.tissues = true;
	emit BeginDatachange(data_selection, this, false);

	QString loadfilename = !file_name.isEmpty() ? file_name : RecentPlaces::GetOpenFileName(this, "Open file", QString::null, "Images (*.nii *.hdr *.img *.nii.gz *.mhd *.mha *.nrrd);;All (*.*)");
	if (!loadfilename.isEmpty())
	{
		m_Handler3D->ReadImage(loadfilename.toAscii());
	}

	emit EndDatachange(this, iseg::ClearUndo);
	PixelsizeChanged();
	SlicethicknessChanged();

	ResetBrightnesscontrast();

	EnableActionsAfterPrjLoaded(true);
}

void MainWindow::ExecuteLoadvtk(const QString& file_name)
{
	if (!MaybeSafe())
	{
		return;
	}

	DataSelection data_selection;
	data_selection.allSlices = true;
	data_selection.bmp = true;
	data_selection.work = true;
	data_selection.tissues = true;
	emit BeginDatachange(data_selection, this, false);

	bool res = true;
	QString loadfilename = !file_name.isEmpty() ? file_name : RecentPlaces::GetOpenFileName(this, "Open file", QString::null, "VTK (*.vti *.vtk)\nAll (*.*)");
	if (!loadfilename.isEmpty())
	{
		res = m_Handler3D->ReadImage(loadfilename.toAscii());
	}

	emit EndDatachange(this, iseg::ClearUndo);
	PixelsizeChanged();
	SlicethicknessChanged();

	ResetBrightnesscontrast();

	EnableActionsAfterPrjLoaded(true);

	if (!res)
	{
		QMessageBox::warning(this, "iSeg", "Error: Could not load file\n" + loadfilename + "\n", QMessageBox::Ok | QMessageBox::Default);
	}
}

void MainWindow::ExecuteLoadavw()
{
	if (!MaybeSafe())
	{
		return;
	}

	DataSelection data_selection;
	data_selection.allSlices = true;
	data_selection.bmp = true;
	data_selection.work = true;
	data_selection.tissues = true;
	emit BeginDatachange(data_selection, this, false);

	QString loadfilename = RecentPlaces::GetOpenFileName(this, "Open file", QString(), "AnalzyeAVW (*.avw)\nAll (*.*)");
	if (!loadfilename.isEmpty())
	{
		m_Handler3D->ReadAvw(loadfilename.toAscii());
	}

	emit EndDatachange(this, iseg::ClearUndo);
	PixelsizeChanged();
	SlicethicknessChanged();

	ResetBrightnesscontrast();

	EnableActionsAfterPrjLoaded(true);
}

void MainWindow::ExecuteReloadbmp()
{
	QStringList files = QFileDialog::getOpenFileNames(
			this, "Select one or more files to open", QString::null, "Images (*.bmp)\nAll (*.*)");

	if ((unsigned short)files.size() == m_Handler3D->NumSlices() ||
			(unsigned short)files.size() == (m_Handler3D->EndSlice() - m_Handler3D->StartSlice()))
	{
		files.sort();

		std::vector<int> vi;

		short nrelem = files.size();

		for (short i = 0; i < nrelem; i++)
		{
			vi.push_back(bmpimgnr(&files[i]));
		}

		std::vector<std::string> vfilenames;
		for (short i = 0; i < nrelem; i++)
		{
			vfilenames.push_back(files[i].toStdString());
		}

		DataSelection data_selection;
		data_selection.allSlices = true;
		data_selection.bmp = true;
		data_selection.work = true;
		data_selection.tissues = true;
		emit BeginDatachange(data_selection, this, false);

		ReloaderBmp2 rb(m_Handler3D, vfilenames, this);
		rb.move(QCursor::pos());
		rb.exec();

		emit EndDatachange(this, iseg::ClearUndo);

		ResetBrightnesscontrast();
	}
	else
	{
		if (!files.empty())
		{
			QMessageBox::information(this, "iSEG Warning", "You have to select the same number of slices.\n");
		}
	}
}

void MainWindow::ExecuteReloadraw()
{
	QString file_path = RecentPlaces::GetOpenFileName(this, QString::null, QString::null, QString::null);
	if (file_path.isEmpty())
	{
		return;
	}

	DataSelection data_selection;
	data_selection.allSlices = true;
	data_selection.bmp = true;
	data_selection.work = true;
	data_selection.tissues = true;
	emit BeginDatachange(data_selection, this, false);

	ReloaderRaw rr(m_Handler3D, file_path, this);
	rr.move(QCursor::pos());
	rr.exec();

	emit EndDatachange(this, iseg::ClearUndo);

	ResetBrightnesscontrast();
}

void MainWindow::ExecuteReloadavw()
{
	DataSelection data_selection;
	data_selection.allSlices = true;
	data_selection.bmp = true;
	data_selection.work = true;
	data_selection.tissues = true;
	emit BeginDatachange(data_selection, this, false);

	QString loadfilename = RecentPlaces::GetOpenFileName(this, "Open file", QString::null, "AnalzyeAVW (*.avw)\nAll (*.*)");
	if (!loadfilename.isEmpty())
	{
		m_Handler3D->ReloadAVW(loadfilename.toAscii(), m_Handler3D->StartSlice());
		ResetBrightnesscontrast();
	}
	emit EndDatachange(this, iseg::ClearUndo);
}

void MainWindow::ExecuteReloadMedicalImage()
{
	auto filename = RecentPlaces::GetOpenFileName(this, "Open file", QString::null, "Images (*.nii *.hdr *.img *.nii.gz *.mhd *.mha *.nrrd);;All (*.*)");

	ReloadMedicalImage(filename);
}

void MainWindow::ExecuteReloadvtk()
{
	DataSelection data_selection;
	data_selection.allSlices = true;
	data_selection.bmp = true;
	data_selection.work = true;
	data_selection.tissues = true;
	emit BeginDatachange(data_selection, this, false);

	QString loadfilename = RecentPlaces::GetOpenFileName(this, "Open file", QString::null, "VTK (*.vti *.vtk)\nAll (*.*)");
	if (!loadfilename.isEmpty())
	{
		m_Handler3D->ReloadImage(loadfilename.toAscii(), m_Handler3D->StartSlice());
		ResetBrightnesscontrast();
	}
	emit EndDatachange(this, iseg::ClearUndo);
}

void MainWindow::ExecuteLoadsurface()
{
	if (!MaybeSafe())
	{
		return;
	}

	bool ok = true;
	QString loadfilename = RecentPlaces::GetOpenFileName(this, "Import Surface/Lines", QString::null, "Surfaces & Polylines (*.stl *.vtk)");
	if (!loadfilename.isEmpty())
	{
		QCheckBox* cb = new QCheckBox("Intersect Only");
		cb->setToolTip("If on, the intersection of the surface with the voxels will be computed, not the interior of a closed surface.");
		cb->setChecked(false);
		cb->blockSignals(true);

		QMessageBox msg_box;
		msg_box.setWindowTitle("Import Surface");
		msg_box.setText("What would you like to do what with the imported surface?");
		msg_box.addButton("Overwrite", QMessageBox::YesRole); //==0
		msg_box.addButton("Add", QMessageBox::NoRole);				//==1
		msg_box.addButton(cb, QMessageBox::ResetRole);				//
		msg_box.addButton("Cancel", QMessageBox::RejectRole); //==2
		int overwrite = msg_box.exec();
		bool intersect = cb->isChecked();

		if (overwrite == 2)
			return;

		ok = m_Handler3D->LoadSurface(loadfilename.toStdString(), overwrite == 0, intersect);
	}

	if (ok)
	{
		// TODO: this is probably incorrect
		ResetBrightnesscontrast();
	}
	else
	{
		QMessageBox::warning(this, "iSeg", "Error: Surface does not overlap with image", QMessageBox::Ok | QMessageBox::Default);
	}
}

void MainWindow::ExecuteLoadrtstruct()
{
#ifndef NORTSTRUCTSUPPORT
	QString loadfilename = RecentPlaces::GetOpenFileName(this, "Open file", QString(), "RTstruct (*.dcm)\nAll (*.*)");
	if (loadfilename.isEmpty())
	{
		return;
	}

	RadiotherapyStructureSetImporter ri(loadfilename, m_Handler3D, this);

	QObject_connect(&ri, SIGNAL(BeginDatachange(DataSelection&,QWidget*,bool)), this, SLOT(HandleBeginDatachange(DataSelection&,QWidget*,bool)));
	QObject_connect(&ri, SIGNAL(EndDatachange(QWidget*,eEndUndoAction)), this, SLOT(HandleEndDatachange(QWidget*,eEndUndoAction)));

	ri.move(QCursor::pos());
	ri.exec();

	m_TissueTreeWidget->UpdateTreeWidget();
	TissuenrChanged(m_TissueTreeWidget->GetCurrentType() - 1);

	// \todo is this necessary?
	QObject_disconnect(&ri, SIGNAL(BeginDatachange(DataSelection&,QWidget*,bool)), this, SLOT(HandleBeginDatachange(DataSelection&,QWidget*,bool)));
	QObject_disconnect(&ri, SIGNAL(EndDatachange(QWidget*,eEndUndoAction)), this, SLOT(HandleEndDatachange(QWidget*,eEndUndoAction)));

	ResetBrightnesscontrast();
#endif
}

void MainWindow::ExecuteLoadrtdose()
{
	if (!MaybeSafe())
	{
		return;
	}

	DataSelection data_selection;
	data_selection.allSlices = true;
	data_selection.bmp = true;
	data_selection.work = true;
	data_selection.tissues = true;
	emit BeginDatachange(data_selection, this, false);

	QString loadfilename = RecentPlaces::GetOpenFileName(this, "Open file", QString(), "RTdose (*.dcm)\nAll (*.*)");
	if (!loadfilename.isEmpty())
	{
		m_Handler3D->ReadRTdose(loadfilename.toAscii());
	}

	emit EndDatachange(this, iseg::ClearUndo);
	PixelsizeChanged();
	SlicethicknessChanged();

	ResetBrightnesscontrast();

	EnableActionsAfterPrjLoaded(true);
}

void MainWindow::ExecuteReloadrtdose()
{
	DataSelection data_selection;
	data_selection.allSlices = true;
	data_selection.bmp = true;
	data_selection.work = true;
	data_selection.tissues = true;
	emit BeginDatachange(data_selection, this, false);

	QString loadfilename = RecentPlaces::GetOpenFileName(this, "Open file", QString(), "RTdose (*.dcm)\nAll (*.*)");
	if (!loadfilename.isEmpty())
	{
		m_Handler3D->ReloadRTdose(loadfilename.toAscii(), m_Handler3D->StartSlice()); // TODO: handle failure
		ResetBrightnesscontrast();
	}
	emit EndDatachange(this, iseg::ClearUndo);
}

void MainWindow::ExecuteLoads4llivelink()
{
	QString loadfilename = RecentPlaces::GetOpenFileName(this, "Open file", QString(), "S4L Link (*.h5)\nAll (*.*)");
	if (!loadfilename.isEmpty())
	{
		LoadS4Llink(loadfilename);
	}
}

void MainWindow::ExecuteSaveimg()
{
	DataSelection data_selection;
	data_selection.bmp = true;
	data_selection.work = true;
	data_selection.tissues = true;
	emit BeginDataexport(data_selection, this);

	ExportImg export_dialog(m_Handler3D, this);
	export_dialog.move(QCursor::pos());
	export_dialog.exec();

	emit EndDataexport(this);
}

void MainWindow::ExecuteSaveprojas()
{
	QString savefilename = RecentPlaces::GetSaveFileName(this, "Save as", QString::null, "Projects (*.prj)");
	if (savefilename.length() > 4)
	{
		DataSelection data_selection;
		data_selection.bmp = true;
		data_selection.work = true;
		data_selection.tissues = true;
		data_selection.tissueHierarchy = true;
		emit BeginDataexport(data_selection, this);

		if (!savefilename.endsWith(QString(".prj")))
			savefilename.append(".prj");

		m_MSaveprojfilename = savefilename;

		//Append "Temp" at the end of the file name and rename it at the end of the successful saving process
		QString temp_file_name = QString(savefilename);
		int after_dot = temp_file_name.lastIndexOf('.');
		if (after_dot != 0)
			temp_file_name =
					temp_file_name.remove(after_dot, temp_file_name.length() - after_dot) +
					"Temp.prj";
		else
			temp_file_name = temp_file_name + "Temp.prj";

		QString source_file_name_without_extension;
		after_dot = savefilename.lastIndexOf('.');
		if (after_dot != -1)
			source_file_name_without_extension = savefilename.mid(0, after_dot);

		QString temp_file_name_without_extension;
		after_dot = temp_file_name.lastIndexOf('.');
		if (after_dot != -1)
			temp_file_name_without_extension = temp_file_name.mid(0, after_dot);

		int num_tasks = 3;
		QProgressDialog progress("Save in progress...", "Cancel", 0, num_tasks, this);
		progress.show();
		QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
		progress.setWindowModality(Qt::WindowModal);
		progress.setModal(true);
		progress.setValue(1);

		setWindowTitle(QString(" iSeg ") + QString(xstr(ISEG_VERSION)) +
							 QString(" - ") + TruncateFileName(savefilename));

		//m_saveprojfilename = tempFileName;
		//AddLoadProj(tempFileName);
		AddLoadProj(m_MSaveprojfilename);
		FILE* fp = m_Handler3D->SaveProject(temp_file_name.toAscii(), "xmf");
		fp = m_BitstackWidget->SaveProj(fp);
		unsigned short save_proj_version = 12;
		fp = TissueInfos::SaveTissues(fp, save_proj_version);
		unsigned short combined_version =
				(unsigned short)iseg::CombineTissuesVersion(save_proj_version, 1);
		fwrite(&combined_version, 1, sizeof(unsigned short), fp);
		/*for(size_t i=0;i<tabwidgets.size();i++){
			fp=((QWidget1 *)(tabwidgets[i]))->SaveParams(fp,saveProjVersion);
		}*/
		m_ThresholdWidget->SaveParams(fp, save_proj_version);
		m_HystWidget->SaveParams(fp, save_proj_version);
		m_LivewireWidget->SaveParams(fp, save_proj_version);
		m_IftrgWidget->SaveParams(fp, save_proj_version);
		m_FastmarchingWidget->SaveParams(fp, save_proj_version);
		m_WatershedWidget->SaveParams(fp, save_proj_version);
		m_OlcWidget->SaveParams(fp, save_proj_version);
		m_InterpolationWidget->SaveParams(fp, save_proj_version);
		m_SmoothingWidget->SaveParams(fp, save_proj_version);
		m_MorphologyWidget->SaveParams(fp, save_proj_version);
		m_EdgeWidget->SaveParams(fp, save_proj_version);
		m_FeatureWidget->SaveParams(fp, save_proj_version);
		m_MeasurementWidget->SaveParams(fp, save_proj_version);
		m_VesselextrWidget->SaveParams(fp, save_proj_version);
		m_PickerWidget->SaveParams(fp, save_proj_version);
		m_TissueTreeWidget->SaveParams(fp, save_proj_version);
		fp = TissueInfos::SaveTissueLocks(fp);
		fp = SaveNotes(fp, save_proj_version);

		fclose(fp);

		progress.setValue(2);

		if (QFile::exists(source_file_name_without_extension + ".xmf"))
			QFile::remove(source_file_name_without_extension + ".xmf");
		QFile::rename(temp_file_name_without_extension + ".xmf", source_file_name_without_extension + ".xmf");

		if (QFile::exists(source_file_name_without_extension + ".prj"))
			QFile::remove(source_file_name_without_extension + ".prj");
		QFile::rename(temp_file_name_without_extension + ".prj", source_file_name_without_extension + ".prj");

		if (QFile::exists(source_file_name_without_extension + ".h5"))
			QFile::remove(source_file_name_without_extension + ".h5");
		QFile::rename(temp_file_name_without_extension + ".h5", source_file_name_without_extension + ".h5");

		progress.setValue(num_tasks);

		emit EndDataexport(this);
	}
}

void MainWindow::ExecuteSavecopyas()
{
	QString savefilename = RecentPlaces::GetSaveFileName(this, "Save copy as", QString::null, "Projects (*.prj)");
	if (savefilename.length() > 4)
	{
		DataSelection data_selection;
		data_selection.bmp = true;
		data_selection.work = true;
		data_selection.tissues = true;
		data_selection.tissueHierarchy = true;
		emit BeginDataexport(data_selection, this);

		if (!savefilename.endsWith(QString(".prj")))
			savefilename.append(".prj");

		FILE* fp = m_Handler3D->SaveProject(savefilename.toAscii(), "xmf");
		fp = m_BitstackWidget->SaveProj(fp);
		unsigned short save_proj_version = 12;
		fp = TissueInfos::SaveTissues(fp, save_proj_version);
		unsigned short combined_version =
				(unsigned short)iseg::CombineTissuesVersion(save_proj_version, 1);
		fwrite(&combined_version, 1, sizeof(unsigned short), fp);
		/*for(size_t i=0;i<tabwidgets.size();i++){
			fp=((QWidget1 *)(tabwidgets[i]))->SaveParams(fp,saveProjVersion);
		}*/
		m_ThresholdWidget->SaveParams(fp, save_proj_version);
		m_HystWidget->SaveParams(fp, save_proj_version);
		m_LivewireWidget->SaveParams(fp, save_proj_version);
		m_IftrgWidget->SaveParams(fp, save_proj_version);
		m_FastmarchingWidget->SaveParams(fp, save_proj_version);
		m_WatershedWidget->SaveParams(fp, save_proj_version);
		m_OlcWidget->SaveParams(fp, save_proj_version);
		m_InterpolationWidget->SaveParams(fp, save_proj_version);
		m_SmoothingWidget->SaveParams(fp, save_proj_version);
		m_MorphologyWidget->SaveParams(fp, save_proj_version);
		m_EdgeWidget->SaveParams(fp, save_proj_version);
		m_FeatureWidget->SaveParams(fp, save_proj_version);
		m_MeasurementWidget->SaveParams(fp, save_proj_version);
		m_VesselextrWidget->SaveParams(fp, save_proj_version);
		m_PickerWidget->SaveParams(fp, save_proj_version);
		m_TissueTreeWidget->SaveParams(fp, save_proj_version);
		fp = TissueInfos::SaveTissueLocks(fp);
		fp = SaveNotes(fp, save_proj_version);

		fclose(fp);

		emit EndDataexport(this);
	}
}

void MainWindow::SaveSettings()
{
	FILE* fp = fopen(m_Settingsfile.c_str(), "wb");
	if (fp == nullptr)
		return;
	unsigned short save_proj_version = 12;
	unsigned short combined_version = (unsigned short)iseg::CombineTissuesVersion(save_proj_version, 1);
	fwrite(&combined_version, 1, sizeof(unsigned short), fp);
	bool flag;
	flag = WidgetInterface::GetHideParams();
	fwrite(&flag, 1, sizeof(bool), fp);
	//	flag=!hidestack->isChecked();
	flag = false;
	fwrite(&flag, 1, sizeof(bool), fp);
	//	flag=!hidenotes->isChecked();
	flag = false;
	fwrite(&flag, 1, sizeof(bool), fp);
	//	flag=!hidezoom->isChecked();
	flag = false;
	fwrite(&flag, 1, sizeof(bool), fp);
	flag = !m_Hidecontrastbright->isChecked();
	fwrite(&flag, 1, sizeof(bool), fp);
	flag = !m_Hidecopyswap->isChecked();
	fwrite(&flag, 1, sizeof(bool), fp);
	for (unsigned short i = 0; i < 16; i++)
	{
		flag = true;
		if (i < m_ShowtabAction.size() && m_ShowtabAction[i])
			flag = m_ShowtabAction[i]->isChecked();
		fwrite(&flag, 1, sizeof(bool), fp);
	}
	fp = TissueInfos::SaveTissues(fp, save_proj_version);
	m_ThresholdWidget->SaveParams(fp, save_proj_version);
	m_HystWidget->SaveParams(fp, save_proj_version);
	m_LivewireWidget->SaveParams(fp, save_proj_version);
	m_IftrgWidget->SaveParams(fp, save_proj_version);
	m_FastmarchingWidget->SaveParams(fp, save_proj_version);
	m_WatershedWidget->SaveParams(fp, save_proj_version);
	m_OlcWidget->SaveParams(fp, save_proj_version);
	m_InterpolationWidget->SaveParams(fp, save_proj_version);
	m_SmoothingWidget->SaveParams(fp, save_proj_version);
	m_MorphologyWidget->SaveParams(fp, save_proj_version);
	m_EdgeWidget->SaveParams(fp, save_proj_version);
	m_FeatureWidget->SaveParams(fp, save_proj_version);
	m_MeasurementWidget->SaveParams(fp, save_proj_version);
	m_VesselextrWidget->SaveParams(fp, save_proj_version);
	m_PickerWidget->SaveParams(fp, save_proj_version);
	m_TissueTreeWidget->SaveParams(fp, save_proj_version);

	fclose(fp);

	QSettings settings(QSettings::IniFormat, QSettings::UserScope, "ZMT", "iSeg");
	settings.beginGroup("MainWindow");
	settings.setValue("geometry", saveGeometry());
	settings.setValue("state", saveState());
	settings.setValue("NumberOfUndoSteps", this->m_Handler3D->GetNumberOfUndoSteps());
	settings.setValue("NumberOfUndoArrays", this->m_Handler3D->GetNumberOfUndoArrays());
	settings.setValue("Compression", this->m_Handler3D->GetCompression());
	settings.setValue("ContiguousMemory", this->m_Handler3D->GetContiguousMemory());
	settings.setValue("BloscEnabled", BloscEnabled());
	settings.setValue("SaveTarget", this->m_Handler3D->SaveTarget());
	settings.endGroup();
	settings.beginGroup("RecentPlaces");
	auto places = RecentPlaces::RecentDirectories();
	settings.setValue("NumberOfRecentPlaces", static_cast<unsigned int>(places.size()));
	for (unsigned int i = 0; i < places.size(); ++i)
	{
		settings.setValue(("dir" + std::to_string(i)).c_str(), places[i]);
	}
	settings.endGroup();
	settings.sync();
}

void MainWindow::LoadSettings(const std::string& loadfilename)
{
	m_Settingsfile = loadfilename;

	// also make this the default path for user scope QSettings
	QString settings_path = QFileInfo(QString::fromStdString(m_Settingsfile)).absolutePath();
	QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings_path);

	FILE* fp;
	if ((fp = fopen(loadfilename.c_str(), "rb")) == nullptr)
	{
		return;
	}

	unsigned short combined_version = 0;
	if (fread(&combined_version, sizeof(unsigned short), 1, fp) == 0)
	{
		fclose(fp);
		return;
	}
	int load_proj_version, tissues_version;
	iseg::ExtractTissuesVersion((int)combined_version, load_proj_version, tissues_version);

	DataSelection data_selection;
	data_selection.allSlices = true;
	data_selection.bmp = true;
	data_selection.work = true;
	data_selection.tissues = true;
	emit BeginDatachange(data_selection, this, false);

	bool flag;
	fread(&flag, sizeof(bool), 1, fp);
	ExecuteHideparameters(flag);
	m_Hideparameters->setChecked(flag);
	fread(&flag, sizeof(bool), 1, fp);

	if (load_proj_version >= 7)
	{
		fread(&flag, sizeof(bool), 1, fp);
	}
	else
	{
		flag = false;
	}

	fread(&flag, sizeof(bool), 1, fp);
	fread(&flag, sizeof(bool), 1, fp);
	m_Hidecontrastbright->setChecked(!flag);
	ExecuteHidecontrastbright(!flag);

	fread(&flag, sizeof(bool), 1, fp);
	m_Hidecopyswap->setChecked(!flag);
	ExecuteHidecopyswap(!flag);

	// turn visibility on for all
	auto nrtabbuttons = (unsigned short)m_Tabwidgets.size();
	for (int i = 0; i < nrtabbuttons; i++)
	{
		m_ShowtabAction[i]->setChecked(true);
	}

	// load visibility settings from file
	for (unsigned short i = 0; i < 14; i++)
	{
		fread(&flag, sizeof(bool), 1, fp);
		if (i < nrtabbuttons)
			m_ShowtabAction.at(i)->setChecked(flag);
	}
	if (load_proj_version >= 6)
	{
		fread(&flag, sizeof(bool), 1, fp);
		if (14 < nrtabbuttons)
			m_ShowtabAction.at(14)->setChecked(flag);
	}
	if (load_proj_version >= 9)
	{
		fread(&flag, sizeof(bool), 1, fp);
		if (15 < nrtabbuttons)
			m_ShowtabAction.at(15)->setChecked(flag);
	}
	ExecuteShowtabtoggled(flag);

	fp = TissueInfos::LoadTissues(fp, tissues_version);
	const char* default_tissues_filename = m_MTmppath.absoluteFilePath("def_tissues.txt");
	FILE* fp_tmp = fopen(default_tissues_filename, "r");
	if (fp_tmp != nullptr || TissueInfos::GetTissueCount() <= 0)
	{
		if (fp_tmp != nullptr)
			fclose(fp_tmp);
		TissueInfos::LoadDefaultTissueList(default_tissues_filename);
	}
	m_TissueTreeWidget->UpdateTreeWidget();
	emit EndDatachange(this, iseg::ClearUndo);
	TissuenrChanged(m_TissueTreeWidget->GetCurrentType() - 1);

	/*for(size_t i=0;i<tabwidgets.size();i++){
		fp=((QWidget1 *)(tabwidgets[i]))->LoadParams(fp,version);
	}*/
	m_ThresholdWidget->LoadParams(fp, load_proj_version);
	m_HystWidget->LoadParams(fp, load_proj_version);
	m_LivewireWidget->LoadParams(fp, load_proj_version);
	m_IftrgWidget->LoadParams(fp, load_proj_version);
	m_FastmarchingWidget->LoadParams(fp, load_proj_version);
	m_WatershedWidget->LoadParams(fp, load_proj_version);
	m_OlcWidget->LoadParams(fp, load_proj_version);
	m_InterpolationWidget->LoadParams(fp, load_proj_version);
	m_SmoothingWidget->LoadParams(fp, load_proj_version);
	m_MorphologyWidget->LoadParams(fp, load_proj_version);
	m_EdgeWidget->LoadParams(fp, load_proj_version);
	m_FeatureWidget->LoadParams(fp, load_proj_version);
	m_MeasurementWidget->LoadParams(fp, load_proj_version);
	m_VesselextrWidget->LoadParams(fp, load_proj_version);
	m_PickerWidget->LoadParams(fp, load_proj_version);
	m_TissueTreeWidget->LoadParams(fp, load_proj_version);
	fclose(fp);

	if (load_proj_version > 7)
	{
		ISEG_INFO_MSG("LoadSettings() : restoring values...");
		QSettings settings(QSettings::IniFormat, QSettings::UserScope, "ZMT", "iSeg");
		settings.beginGroup("MainWindow");
		restoreGeometry(settings.value("geometry").toByteArray());
		restoreState(settings.value("state").toByteArray());
		this->m_Handler3D->SetNumberOfUndoSteps(settings.value("NumberOfUndoSteps", 50).toUInt());
		this->m_Handler3D->SetNumberOfUndoArrays(settings.value("NumberOfUndoArrays", 20).toUInt());
		this->m_Handler3D->SetCompression(settings.value("Compression", 0).toInt());
		this->m_Handler3D->SetContiguousMemory(settings.value("ContiguousMemory", true).toBool());
		SetBloscEnabled(settings.value("BloscEnabled", false).toBool());
		this->m_Handler3D->SetSaveTarget(settings.value("SaveTarget", false).toBool());
		settings.endGroup();

		settings.beginGroup("RecentPlaces");
		auto n = settings.value("NumberOfRecentPlaces", 0).toUInt();
		for (unsigned i = n; i > 0; --i)
		{
			auto dir = settings.value(("dir" + std::to_string(i - 1)).c_str(), QString()).toString();
			RecentPlaces::AddRecent(dir);
		}
		settings.endGroup();

		m_Undonr->setEnabled(this->m_Handler3D->ReturnNrundo() > 0);
	}
}

void MainWindow::ExecuteSaveactiveslicesas()
{
	DataSelection data_selection;
	data_selection.bmp = true;
	data_selection.work = true;
	data_selection.tissues = true;
	data_selection.tissueHierarchy = true;
	emit BeginDataexport(data_selection, this);

	QString savefilename = RecentPlaces::GetSaveFileName(this, "Save as", QString::null, "Projects (*.prj)");

	if (savefilename.length() <= 4 || !savefilename.endsWith(QString(".prj")))
		savefilename.append(".prj");

	if (!savefilename.isEmpty())
	{
		FILE* fp = m_Handler3D->SaveActiveSlices(savefilename.toAscii(), "xmf");
		fp = m_BitstackWidget->SaveProj(fp);
		unsigned short save_proj_version = 12;
		fp = TissueInfos::SaveTissues(fp, save_proj_version);
		unsigned short combined_version =
				(unsigned short)iseg::CombineTissuesVersion(save_proj_version, 1);
		fwrite(&combined_version, 1, sizeof(unsigned short), fp);
		/*for(size_t i=0;i<tabwidgets.size();i++){
			fp=((QWidget1 *)(tabwidgets[i]))->SaveParams(fp,saveProjVersion);
		}*/
		m_ThresholdWidget->SaveParams(fp, save_proj_version);
		m_HystWidget->SaveParams(fp, save_proj_version);
		m_LivewireWidget->SaveParams(fp, save_proj_version);
		m_IftrgWidget->SaveParams(fp, save_proj_version);
		m_FastmarchingWidget->SaveParams(fp, save_proj_version);
		m_WatershedWidget->SaveParams(fp, save_proj_version);
		m_OlcWidget->SaveParams(fp, save_proj_version);
		m_InterpolationWidget->SaveParams(fp, save_proj_version);
		m_SmoothingWidget->SaveParams(fp, save_proj_version);
		m_MorphologyWidget->SaveParams(fp, save_proj_version);
		m_EdgeWidget->SaveParams(fp, save_proj_version);
		m_FeatureWidget->SaveParams(fp, save_proj_version);
		m_MeasurementWidget->SaveParams(fp, save_proj_version);
		m_VesselextrWidget->SaveParams(fp, save_proj_version);
		m_PickerWidget->SaveParams(fp, save_proj_version);
		m_TissueTreeWidget->SaveParams(fp, save_proj_version);
		fp = TissueInfos::SaveTissueLocks(fp);
		fp = SaveNotes(fp, save_proj_version);

		fclose(fp);
	}
	emit EndDataexport(this);
}

void MainWindow::ExecuteSaveproj()
{
	DataSelection data_selection;
	data_selection.bmp = true;
	data_selection.work = true;
	data_selection.tissues = true;
	data_selection.tissueHierarchy = true;
	emit BeginDataexport(data_selection, this);

	if (m_MEditingmode)
	{
		if (!m_S4Lcommunicationfilename.isEmpty())
		{
			m_Handler3D->SaveCommunicationFile(m_S4Lcommunicationfilename.toAscii());
		}
	}
	else
	{
		if (!m_MSaveprojfilename.isEmpty())
		{
			//Append "Temp" at the end of the file name and rename it at the end of the successful saving process
			QString temp_file_name = QString(m_MSaveprojfilename);
			int after_dot = temp_file_name.lastIndexOf('.');
			if (after_dot != 0)
				temp_file_name =
						temp_file_name.remove(after_dot, temp_file_name.length() - after_dot) +
						"Temp.prj";
			else
				temp_file_name = temp_file_name + "Temp.prj";

			std::string pj_fn = m_MSaveprojfilename.toStdString();

			QString source_file_name_without_extension;
			after_dot = m_MSaveprojfilename.lastIndexOf('.');
			if (after_dot != -1)
				source_file_name_without_extension = m_MSaveprojfilename.mid(0, after_dot);

			QString temp_file_name_without_extension;
			after_dot = temp_file_name.lastIndexOf('.');
			if (after_dot != -1)
				temp_file_name_without_extension = temp_file_name.mid(0, after_dot);

			int num_tasks = 3;
			QProgressDialog progress("Save in progress...", "Cancel", 0, num_tasks, this);
			progress.show();
			QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
			progress.setWindowModality(Qt::WindowModal);
			progress.setModal(true);
			progress.setValue(1);

			setWindowTitle(QString(" iSeg ") + QString(xstr(ISEG_VERSION)) +
								 QString(" - ") + TruncateFileName(m_MSaveprojfilename));

			m_MSaveprojfilename = temp_file_name;

			//FILE *fp=handler3D->SaveProject(m_saveprojfilename.toAscii(),"xmf");
			FILE* fp = m_Handler3D->SaveProject(temp_file_name.toAscii(), "xmf");
			fp = m_BitstackWidget->SaveProj(fp);
			unsigned short save_proj_version = 12;
			fp = TissueInfos::SaveTissues(fp, save_proj_version);
			unsigned short combined_version =
					(unsigned short)iseg::CombineTissuesVersion(save_proj_version, 1);
			fwrite(&combined_version, 1, sizeof(unsigned short), fp);
			/*for(size_t i=0;i<tabwidgets.size();i++){
				fp=((QWidget1 *)(tabwidgets[i]))->SaveParams(fp,saveProjVersion);
			}*/
			m_ThresholdWidget->SaveParams(fp, save_proj_version);
			m_HystWidget->SaveParams(fp, save_proj_version);
			m_LivewireWidget->SaveParams(fp, save_proj_version);
			m_IftrgWidget->SaveParams(fp, save_proj_version);
			m_FastmarchingWidget->SaveParams(fp, save_proj_version);
			m_WatershedWidget->SaveParams(fp, save_proj_version);
			m_OlcWidget->SaveParams(fp, save_proj_version);
			m_InterpolationWidget->SaveParams(fp, save_proj_version);
			m_SmoothingWidget->SaveParams(fp, save_proj_version);
			m_MorphologyWidget->SaveParams(fp, save_proj_version);
			m_EdgeWidget->SaveParams(fp, save_proj_version);
			m_FeatureWidget->SaveParams(fp, save_proj_version);
			m_MeasurementWidget->SaveParams(fp, save_proj_version);
			m_VesselextrWidget->SaveParams(fp, save_proj_version);
			m_PickerWidget->SaveParams(fp, save_proj_version);
			m_TissueTreeWidget->SaveParams(fp, save_proj_version);
			fp = TissueInfos::SaveTissueLocks(fp);
			fp = SaveNotes(fp, save_proj_version);

			fclose(fp);

			progress.setValue(2);

			QMessageBox m_box;
			m_box.setWindowTitle("Saving project");
			m_box.setText("The project you are trying to save is open somewhere else. "
										"Please, close it before continuing and press OK or press "
										"Cancel to stop saving process.");
			m_box.addButton(QMessageBox::Ok);
			m_box.addButton(QMessageBox::Cancel);

			if (QFile::exists(source_file_name_without_extension + ".xmf"))
			{
				bool remove_success =
						QFile::remove(source_file_name_without_extension + ".xmf");
				while (!remove_success)
				{
					int ret = m_box.exec();
					if (ret == QMessageBox::Cancel)
						return;

					remove_success =
							QFile::remove(source_file_name_without_extension + ".xmf");
				}
			}
			QFile::rename(temp_file_name_without_extension + ".xmf", source_file_name_without_extension + ".xmf");

			if (QFile::exists(source_file_name_without_extension + ".prj"))
			{
				bool remove_success =
						QFile::remove(source_file_name_without_extension + ".prj");
				while (!remove_success)
				{
					int ret = m_box.exec();
					if (ret == QMessageBox::Cancel)
						return;

					remove_success =
							QFile::remove(source_file_name_without_extension + ".prj");
				}
			}
			QFile::rename(temp_file_name_without_extension + ".prj", source_file_name_without_extension + ".prj");

			if (QFile::exists(source_file_name_without_extension + ".h5"))
			{
				bool remove_success =
						QFile::remove(source_file_name_without_extension + ".h5");
				while (!remove_success)
				{
					int ret = m_box.exec();
					if (ret == QMessageBox::Cancel)
						return;

					remove_success = QFile::remove(source_file_name_without_extension + ".h5");
				}
			}
			QFile::rename(temp_file_name_without_extension + ".h5", source_file_name_without_extension + ".h5");

			m_MSaveprojfilename = source_file_name_without_extension + ".prj";

			progress.setValue(num_tasks);
		}
		else
		{
			if (Modified())
			{
				ExecuteSaveprojas();
			}
		}
	}
	emit EndDataexport(this);
}

void MainWindow::LoadAny(const QString& loadfilename)
{
	const auto file_path = boost::filesystem::path(loadfilename.toStdString());
	if (!boost::filesystem::exists(file_path))
	{
		return;
	}
	
	// Deduce the importer from the file-ending
	const auto extension = boost::filesystem::extension(file_path);

	static const std::string proj_extenstion = ".prj";
	static const std::string s4l_extenstion = ".h5";
	static const std::vector<std::string> medical_image_extensions = {".mhd", ".mha", ".nii", ".hdr", ".img", ".gz"};
	static const std::vector<std::string> vtk_extensions = {".vtk", ".vki"};

	if (extension == proj_extenstion)
	{
		Loadproj(loadfilename);
	}
	else if (extension == s4l_extenstion)
	{
		LoadS4Llink(loadfilename);
	}
	else if (std::count(medical_image_extensions.begin(), medical_image_extensions.end(), extension))
	{
		ExecuteLoadMedicalImage(loadfilename);
	}
	else if (std::count(vtk_extensions.begin(), vtk_extensions.end(), extension))
	{
		ExecuteLoadvtk(loadfilename);
	}
	else
	{
		ISEG_WARNING("Could not load file: " << file_path.string()
								 << " : unsupported extension: " << extension);
	}
}

void MainWindow::Loadproj(const QString& loadfilename)
{
	FILE* fp;
	if ((fp = fopen(loadfilename.toAscii(), "r")) == nullptr)
	{
		return;
	}
	else
	{
		fclose(fp);
	}
	bool stillopen = false;

	DataSelection data_selection;
	data_selection.allSlices = true;
	data_selection.bmp = true;
	data_selection.work = true;
	data_selection.tissues = true;
	emit BeginDatachange(data_selection, this, false);

	if (!loadfilename.isEmpty())
	{
		m_MSaveprojfilename = loadfilename;
		setWindowTitle(QString(" iSeg ") + QString(xstr(ISEG_VERSION)) + QString(" - ") + TruncateFileName(loadfilename));
		AddLoadProj(m_MSaveprojfilename);
		int tissues_version = 0;
		fp = m_Handler3D->LoadProject(loadfilename.toAscii(), tissues_version);
		fp = m_BitstackWidget->LoadProj(fp);
		fp = TissueInfos::LoadTissues(fp, tissues_version);
		stillopen = true;
	}

	emit EndDatachange(this, iseg::ClearUndo);
	TissuenrChanged(m_TissueTreeWidget->GetCurrentType() - 1);

	PixelsizeChanged();
	SlicethicknessChanged();

	if (stillopen)
	{
		unsigned short combined_version = 0;
		if (fread(&combined_version, sizeof(unsigned short), 1, fp) > 0)
		{
			int load_proj_version, tissues_version;
			iseg::ExtractTissuesVersion((int)combined_version, load_proj_version, tissues_version);
			/*for(size_t i=0;i<tabwidgets.size();i++){
				fp=((QWidget1 *)(tabwidgets[i]))->LoadParams(fp,version);
			}*/
			m_ThresholdWidget->LoadParams(fp, load_proj_version);
			m_HystWidget->LoadParams(fp, load_proj_version);
			m_LivewireWidget->LoadParams(fp, load_proj_version);
			m_IftrgWidget->LoadParams(fp, load_proj_version);
			m_FastmarchingWidget->LoadParams(fp, load_proj_version);
			m_WatershedWidget->LoadParams(fp, load_proj_version);
			m_OlcWidget->LoadParams(fp, load_proj_version);
			m_InterpolationWidget->LoadParams(fp, load_proj_version);
			m_SmoothingWidget->LoadParams(fp, load_proj_version);
			m_MorphologyWidget->LoadParams(fp, load_proj_version);
			m_EdgeWidget->LoadParams(fp, load_proj_version);
			m_FeatureWidget->LoadParams(fp, load_proj_version);
			m_MeasurementWidget->LoadParams(fp, load_proj_version);
			m_VesselextrWidget->LoadParams(fp, load_proj_version);
			m_PickerWidget->LoadParams(fp, load_proj_version);
			m_TissueTreeWidget->LoadParams(fp, load_proj_version);

			if (load_proj_version >= 3)
			{
				fp = TissueInfos::LoadTissueLocks(fp);
				tissues_size_t curr_tissue_type = m_TissueTreeWidget->GetCurrentType();
				m_CbTissuelock->setChecked(TissueInfos::GetTissueLocked(curr_tissue_type + 1));
				m_TissueTreeWidget->UpdateTissueIcons();
				m_TissueTreeWidget->UpdateFolderIcons();
			}
			fp = LoadNotes(fp, load_proj_version);
		}

		fclose(fp);
	}

	ResetBrightnesscontrast();

	EnableActionsAfterPrjLoaded(true);
}

void MainWindow::LoadS4Llink(const QString& loadfilename)
{
	if (loadfilename.isEmpty() || !boost::filesystem::exists(loadfilename.toStdString()))
	{
		return;
	}

	DataSelection data_selection;
	data_selection.allSlices = true;
	data_selection.bmp = true;
	data_selection.work = true;
	data_selection.tissues = true;
	emit BeginDatachange(data_selection, this, false);

	m_S4Lcommunicationfilename = loadfilename;
	int tissues_version = 0;
	m_Handler3D->LoadS4Llink(loadfilename.toAscii(), tissues_version);
	TissueInfos::LoadTissuesHDF(loadfilename.toAscii(), tissues_version);

	emit EndDatachange(this, iseg::ClearUndo);
	tissues_size_t m;
	m_Handler3D->GetRangetissue(&m);
	m_Handler3D->Buildmissingtissues(m);
	m_TissueTreeWidget->UpdateTreeWidget();
	TissuenrChanged(m_TissueTreeWidget->GetCurrentType() - 1);

	PixelsizeChanged();
	SlicethicknessChanged();

	m_TissueTreeWidget->UpdateTissueIcons();
	m_TissueTreeWidget->UpdateFolderIcons();

	ResetBrightnesscontrast();
}

void MainWindow::ExecuteMergeprojects()
{
	DataSelection data_selection;
	data_selection.bmp = true;
	data_selection.work = true;
	data_selection.tissues = true;
	data_selection.tissueHierarchy = true;
	emit BeginDataexport(data_selection, this);

	// Get list of project file names to be merged
	MergeProjectsDialog merge_dialog(this);
	if (merge_dialog.exec() != QDialog::Accepted)
	{
		emit EndDataexport(this);
		return;
	}
	std::vector<QString> mergefilenames;
	merge_dialog.GetFilenames(mergefilenames);

	QString savefilename = RecentPlaces::GetSaveFileName(this, "Save as", QString::null, "Projects (*.prj)");
	if (savefilename.length() <= 4 || !savefilename.endsWith(QString(".prj")))
		savefilename.append(".prj");

	if (!savefilename.isEmpty() && !mergefilenames.empty())
	{
		FILE* fp = m_Handler3D->MergeProjects(savefilename.toAscii(), mergefilenames);
		if (!fp)
		{
			QMessageBox::warning(this, "iSeg", "Merge projects failed.\n\nPlease make sure that "
																				 "all projects have the same xy extents\nand their "
																				 "image data is contained in a .h5 and .xmf file.",
					QMessageBox::Ok | QMessageBox::Default);
			return;
		}
		fp = m_BitstackWidget->SaveProj(fp);
		unsigned short save_proj_version = 12;
		fp = TissueInfos::SaveTissues(fp, save_proj_version);
		unsigned short combined_version = (unsigned short)iseg::CombineTissuesVersion(save_proj_version, 1);
		fwrite(&combined_version, 1, sizeof(unsigned short), fp);

		m_ThresholdWidget->SaveParams(fp, save_proj_version);
		m_HystWidget->SaveParams(fp, save_proj_version);
		m_LivewireWidget->SaveParams(fp, save_proj_version);
		m_IftrgWidget->SaveParams(fp, save_proj_version);
		m_FastmarchingWidget->SaveParams(fp, save_proj_version);
		m_WatershedWidget->SaveParams(fp, save_proj_version);
		m_OlcWidget->SaveParams(fp, save_proj_version);
		m_InterpolationWidget->SaveParams(fp, save_proj_version);
		m_SmoothingWidget->SaveParams(fp, save_proj_version);
		m_MorphologyWidget->SaveParams(fp, save_proj_version);
		m_EdgeWidget->SaveParams(fp, save_proj_version);
		m_FeatureWidget->SaveParams(fp, save_proj_version);
		m_MeasurementWidget->SaveParams(fp, save_proj_version);
		m_VesselextrWidget->SaveParams(fp, save_proj_version);
		m_PickerWidget->SaveParams(fp, save_proj_version);
		m_TissueTreeWidget->SaveParams(fp, save_proj_version);
		fp = TissueInfos::SaveTissueLocks(fp);
		fp = SaveNotes(fp, save_proj_version);

		fclose(fp);

		// Load merged project
		if (QMessageBox::question(this, "iSeg", "Would you like to load the merged project?", QMessageBox::Yes | QMessageBox::Default, QMessageBox::No) == QMessageBox::Yes)
		{
			Loadproj(savefilename);
		}
	}
	emit EndDataexport(this);
}

void MainWindow::ExecuteBoneconnectivity()
{
	m_BoneConnectivityDialog = new CheckBoneConnectivityDialog(m_Handler3D, "Bone Connectivity", this, Qt::Window);
	QObject_connect(m_BoneConnectivityDialog, SIGNAL(SliceChanged()), this, SLOT(UpdateSlice()));

	m_BoneConnectivityDialog->show();
	m_BoneConnectivityDialog->raise();

	emit EndDataexport(this);
}

void MainWindow::UpdateSlice() { SliceChanged(); }

void MainWindow::ExecuteLoadproj()
{
	if (!MaybeSafe())
	{
		return;
	}

	QString loadfilename = RecentPlaces::GetOpenFileName(this, "Open file", QString(), "Projects (*.prj)\nAll (*.*)");
	if (!loadfilename.isEmpty())
	{
		Loadproj(loadfilename);
	}
}

void MainWindow::ExecuteLoadproj1()
{
	if (!MaybeSafe())
	{
		return;
	}

	QString loadfilename = m_MLoadprojfilename.m_RecentProjectFileNames[0];

	if (!loadfilename.isEmpty())
	{
		Loadproj(loadfilename);
	}
}

void MainWindow::ExecuteLoadproj2()
{
	if (!MaybeSafe())
	{
		return;
	}

	QString loadfilename = m_MLoadprojfilename.m_RecentProjectFileNames[1];

	if (!loadfilename.isEmpty())
	{
		Loadproj(loadfilename);
	}
}

void MainWindow::ExecuteLoadproj3()
{
	if (!MaybeSafe())
	{
		return;
	}

	QString loadfilename = m_MLoadprojfilename.m_RecentProjectFileNames[2];

	if (!loadfilename.isEmpty())
	{
		Loadproj(loadfilename);
	}
}

void MainWindow::ExecuteLoadproj4()
{
	if (!MaybeSafe())
	{
		return;
	}

	QString loadfilename = m_MLoadprojfilename.m_RecentProjectFileNames[3];

	if (!loadfilename.isEmpty())
	{
		Loadproj(loadfilename);
	}
}

void MainWindow::ExecuteLoadatlas(int i)
{
	AtlasWidget* aw = new AtlasWidget(
			m_MAtlasfilename.m_MAtlasdir.absoluteFilePath(m_MAtlasfilename.m_MAtlasfilename[i]),
			m_MPicpath);
	if (aw->m_IsOk)
	{
		aw->show();
		aw->raise();
		aw->setAttribute(Qt::WA_DeleteOnClose);
	}
	else
	{
		delete aw;
	}
}

void MainWindow::ExecuteLoadatlas0()
{
	int i = 0;
	ExecuteLoadatlas(i);
}

void MainWindow::ExecuteLoadatlas1()
{
	int i = 1;
	ExecuteLoadatlas(i);
}

void MainWindow::ExecuteLoadatlas2()
{
	int i = 2;
	ExecuteLoadatlas(i);
}

void MainWindow::ExecuteLoadatlas3()
{
	int i = 3;
	ExecuteLoadatlas(i);
}

void MainWindow::ExecuteLoadatlas4()
{
	int i = 4;
	ExecuteLoadatlas(i);
}

void MainWindow::ExecuteLoadatlas5()
{
	int i = 5;
	ExecuteLoadatlas(i);
}

void MainWindow::ExecuteLoadatlas6()
{
	int i = 6;
	ExecuteLoadatlas(i);
}
void MainWindow::ExecuteLoadatlas7()
{
	int i = 7;
	ExecuteLoadatlas(i);
}
void MainWindow::ExecuteLoadatlas8()
{
	int i = 8;
	ExecuteLoadatlas(i);
}
void MainWindow::ExecuteLoadatlas9()
{
	int i = 9;
	ExecuteLoadatlas(i);
}
void MainWindow::ExecuteLoadatlas10()
{
	int i = 10;
	ExecuteLoadatlas(i);
}
void MainWindow::ExecuteLoadatlas11()
{
	int i = 11;
	ExecuteLoadatlas(i);
}
void MainWindow::ExecuteLoadatlas12()
{
	int i = 12;
	ExecuteLoadatlas(i);
}
void MainWindow::ExecuteLoadatlas13()
{
	int i = 13;
	ExecuteLoadatlas(i);
}
void MainWindow::ExecuteLoadatlas14()
{
	int i = 14;
	ExecuteLoadatlas(i);
}
void MainWindow::ExecuteLoadatlas15()
{
	int i = 15;
	ExecuteLoadatlas(i);
}
void MainWindow::ExecuteLoadatlas16()
{
	int i = 16;
	ExecuteLoadatlas(i);
}
void MainWindow::ExecuteLoadatlas17()
{
	int i = 17;
	ExecuteLoadatlas(i);
}
void MainWindow::ExecuteLoadatlas18()
{
	int i = 18;
	ExecuteLoadatlas(i);
}
void MainWindow::ExecuteLoadatlas19()
{
	int i = 19;
	ExecuteLoadatlas(i);
}

void MainWindow::ExecuteCreateatlas()
{
	DataSelection data_selection;
	data_selection.tissues = true;
	emit BeginDataexport(data_selection, this);

	QString savefilename = RecentPlaces::GetSaveFileName(this, "Save as", QString::null, "Atlas file (*.atl)");

	if (savefilename.length() > 4 && !savefilename.endsWith(QString(".atl")))
		savefilename.append(".atl");

	if (!savefilename.isEmpty())
	{
		m_Handler3D->PrintAtlas(savefilename.toAscii());
		LoadAtlas(m_MAtlasfilename.m_MAtlasdir);
	}

	emit EndDataexport(this);
}

void MainWindow::ExecuteReloadatlases()
{
	LoadAtlas(m_MAtlasfilename.m_MAtlasdir);
}

void MainWindow::ExecuteSaveTissues()
{
	QString savefilename = RecentPlaces::GetSaveFileName(this, "Save as", QString::null, QString::null);

	if (!savefilename.isEmpty())
	{
		unsigned short save_version = 7;
		TissueInfos::SaveTissuesReadable(savefilename.toAscii(), save_version);
	}
}

void MainWindow::ExecuteExportlabelfield()
{
	DataSelection data_selection;
	data_selection.tissues = true;
	emit BeginDataexport(data_selection, this);

	QString savefilename = RecentPlaces::GetSaveFileName(this, "Save as", QString::null, "AmiraMesh Ascii (*.am)");

	if (savefilename.length() > 4 && !savefilename.endsWith(QString(".am")))
		savefilename.append(".am");

	if (!savefilename.isEmpty())
	{
		m_Handler3D->PrintAmascii(savefilename.toAscii());
	}

	emit EndDataexport(this);
}

void MainWindow::ExecuteExportmat()
{
	DataSelection data_selection;
	data_selection.bmp = true;
	data_selection.work = true;
	data_selection.tissues = true;
	emit BeginDataexport(data_selection, this);

	QString savefilename = RecentPlaces::GetSaveFileName(this, "Save as", QString::null, "Matlab (*.mat)");

	if (savefilename.length() > 4 && !savefilename.endsWith(QString(".mat")))
		savefilename.append(".mat");

	if (!savefilename.isEmpty())
	{
		m_Handler3D->PrintTissuemat(savefilename.toAscii());
	}

	emit EndDataexport(this);
}

void MainWindow::ExecuteExporthdf()
{
	QString savefilename = RecentPlaces::GetSaveFileName(this, "Save as", QString::null, "HDF (*.h5)");

	if (savefilename.length() > 3 && !savefilename.endsWith(QString(".h5")))
		savefilename.append(".h5");

	if (!savefilename.isEmpty())
	{
		DataSelection data_selection;
		data_selection.bmp = true;
		data_selection.work = true;
		data_selection.tissues = true;
		emit BeginDataexport(data_selection, this);

		m_Handler3D->SaveCommunicationFile(savefilename.toAscii());

		emit EndDataexport(this);
	}
}

void MainWindow::ExecuteExportvtkascii()
{
	DataSelection data_selection;
	data_selection.tissues = true;
	emit BeginDataexport(data_selection, this);

	QString savefilename = RecentPlaces::GetSaveFileName(this, "Save as", QString::null, "VTK Ascii (*.vti *.vtk)");

	if (savefilename.length() > 4 && !(savefilename.endsWith(QString(".vti")) || savefilename.endsWith(QString(".vtk"))))
		savefilename.append(".vti");

	if (!savefilename.isEmpty())
	{
		m_Handler3D->ExportTissue(savefilename.toAscii(), false);
	}

	emit EndDataexport(this);
}

void MainWindow::ExecuteExportvtkbinary()
{
	DataSelection data_selection;
	data_selection.tissues = true;
	emit BeginDataexport(data_selection, this);

	QString savefilename = RecentPlaces::GetSaveFileName(this, "Save as", QString::null, "VTK bin (*.vti *.vtk)");

	if (savefilename.length() > 4 && !(savefilename.endsWith(QString(".vti")) || savefilename.endsWith(QString(".vtk"))))
		savefilename.append(".vti");

	if (!savefilename.isEmpty())
	{
		m_Handler3D->ExportTissue(savefilename.toAscii(), true);
	}

	emit EndDataexport(this);
}

void MainWindow::ExecuteExportvtkcompressedascii()
{
	DataSelection data_selection;
	data_selection.tissues = true;
	emit BeginDataexport(data_selection, this);

	QString savefilename = RecentPlaces::GetSaveFileName(this, "Save as", QString::null, "VTK comp (*.vti)");

	if (savefilename.length() > 4 && !savefilename.endsWith(QString(".vti")))
		savefilename.append(".vti");

	if (!savefilename.isEmpty())
	{
		m_Handler3D->ExportTissue(savefilename.toAscii(), false);
	}

	emit EndDataexport(this);
}

void MainWindow::ExecuteExportvtkcompressedbinary()
{
	DataSelection data_selection;
	data_selection.tissues = true;
	emit BeginDataexport(data_selection, this);

	QString savefilename = RecentPlaces::GetSaveFileName(this, "Save as", QString::null, "VTK comp (*.vti)");
	if (savefilename.length() > 4 && !savefilename.endsWith(QString(".vti")))
		savefilename.append(".vti");

	if (!savefilename.isEmpty())
	{
		m_Handler3D->ExportTissue(savefilename.toAscii(), true);
	}

	emit EndDataexport(this);
}

void MainWindow::ExecuteExportxmlregionextent()
{
	DataSelection data_selection;
	data_selection.tissues = true;
	emit BeginDataexport(data_selection, this);

	QString savefilename = RecentPlaces::GetSaveFileName(this, "Save as", QString::null, "XML extent (*.xml)");
	if (savefilename.length() > 4 && !savefilename.endsWith(QString(".xml")))
		savefilename.append(".xml");

	QString relfilename = "";
	if (!m_MSaveprojfilename.isEmpty())
		relfilename = m_MSaveprojfilename.right(m_MSaveprojfilename.length() -
																						m_MSaveprojfilename.findRev("/") - 1);

	if (!savefilename.isEmpty())
	{
		if (relfilename.isEmpty())
			m_Handler3D->PrintXmlregionextent(savefilename.toAscii(), true);
		else
			m_Handler3D->PrintXmlregionextent(savefilename.toAscii(), true, relfilename.toAscii());
	}

	emit EndDataexport(this);
}

void MainWindow::ExecuteExporttissueindex()
{
	DataSelection data_selection;
	emit BeginDataexport(data_selection, this);

	QString savefilename = RecentPlaces::GetSaveFileName(this, "Save as", QString::null, "Tissue index (*.txt)");
	if (savefilename.length() > 4 && !savefilename.endsWith(QString(".txt")))
		savefilename.append(".txt");

	QString relfilename = "";
	if (!m_MSaveprojfilename.isEmpty())
		relfilename = m_MSaveprojfilename.right(m_MSaveprojfilename.length() - m_MSaveprojfilename.findRev("/") - 1);

	if (!savefilename.isEmpty())
	{
		if (relfilename.isEmpty())
			m_Handler3D->PrintTissueindex(savefilename.toAscii(), true);
		else
			m_Handler3D->PrintTissueindex(savefilename.toAscii(), true, relfilename.toAscii());
	}

	emit EndDataexport(this);
}

void MainWindow::LoadTissuelist(const QString& loadfilename, bool append, bool no_popup)
{
	tissues_size_t remove_tissues_range;
	TissueInfos::LoadTissuesReadable(loadfilename.toAscii(), m_Handler3D, remove_tissues_range);

	if (append)
	{
		int nr = m_TissueTreeWidget->GetCurrentType() - 1;
		m_TissueTreeWidget->UpdateTreeWidget();

		tissues_size_t curr_tissue_type = m_TissueTreeWidget->GetCurrentType();
		if (nr != curr_tissue_type - 1)
		{
			TissuenrChanged(curr_tissue_type - 1);
		}
	}
	else // replace
	{
		if (remove_tissues_range > 0)
		{
			const auto can_delete = [this]() {
				return QMessageBox::question(this, "iSeg",
						"Some of the previously existing tissues are\nnot "
						"contained in the loaded tissue list.\n\nDo you want "
						"to delete them?",
						QMessageBox::Yes | QMessageBox::Default, QMessageBox::No);
			};

			if (no_popup || can_delete() == QMessageBox::Yes)
			{
				std::set<tissues_size_t> remove_tissues;
				for (tissues_size_t type = 1; type <= remove_tissues_range; ++type)
				{
					remove_tissues.insert(type);
				}
				DataSelection data_selection;
				data_selection.allSlices = true;
				data_selection.tissues = true;
				emit BeginDatachange(data_selection, this, false);
				m_Handler3D->RemoveTissues(remove_tissues);
				emit EndDatachange(this, iseg::ClearUndo);
			}
		}

		tissues_size_t tissue_count = TissueInfos::GetTissueCount();
		m_Handler3D->CapTissue(tissue_count);

		m_TissueTreeWidget->UpdateTreeWidget();
		TissuenrChanged(m_TissueTreeWidget->GetCurrentType() - 1);
	}
}

void MainWindow::ReloadMedicalImage(const QString& loadfilename)
{
	if (!loadfilename.isEmpty())
	{
		DataSelection data_selection;
		data_selection.allSlices = true;
		data_selection.bmp = true;
		data_selection.work = true;
		data_selection.tissues = true;
		emit BeginDatachange(data_selection, this, false);

		m_Handler3D->ReloadImage(loadfilename.toAscii(), m_Handler3D->StartSlice());
		ResetBrightnesscontrast();

		emit EndDatachange(this, iseg::ClearUndo);
	}
}

void MainWindow::ExecuteLoadTissues()
{
	// no filter ?
	QString loadfilename = RecentPlaces::GetOpenFileName(this, "Open file", QString::null, QString::null);
	if (!loadfilename.isEmpty())
	{
		QMessageBox msg_box;
		msg_box.setText("Do you want to append the new tissues or to replace the old tissues?");
		QPushButton* append_button = msg_box.addButton(tr("Append"), QMessageBox::AcceptRole);
		QPushButton* replace_button = msg_box.addButton(tr("Replace"), QMessageBox::AcceptRole);
		msg_box.addButton(QMessageBox::Abort);
		msg_box.setIcon(QMessageBox::Question);
		msg_box.exec();

		if (msg_box.clickedButton() == append_button || msg_box.clickedButton() == replace_button)
		{
			LoadTissuelist(loadfilename, msg_box.clickedButton() == append_button, false);
		}
	}
}

void MainWindow::ExecuteSettissuesasdef()
{
	TissueInfos::SaveDefaultTissueList(m_MTmppath.absoluteFilePath("def_tissues.txt"));
}

void MainWindow::ExecuteRemovedeftissues()
{
	remove(m_MTmppath.absoluteFilePath("def_tissues.txt"));
}

void MainWindow::ExecuteNew()
{
	DataSelection data_selection;
	data_selection.allSlices = true;
	data_selection.bmp = true;
	data_selection.work = true;
	data_selection.tissues = true;
	emit BeginDatachange(data_selection, this, false);

	NewImg ni(m_Handler3D, this);
	ni.move(QCursor::pos());
	ni.exec();

	if (!ni.NewPressed())
		return;

	m_Handler3D->SetPixelsize(1.f, 1.f);
	m_Handler3D->SetSlicethickness(1.f);

	emit EndDatachange(this, iseg::ClearUndo);

	m_MSaveprojfilename = "";
	setWindowTitle(QString(" iSeg ") + QString(xstr(ISEG_VERSION)) +
						 QString(" - No Filename"));
	m_MNotes->clear();

	ResetBrightnesscontrast();
}

void MainWindow::StartSurfaceviewer(int mode)
{
	// ensure we don't have a viewer running
	SurfaceViewerClosed();

	if (!SurfaceViewerWidget::IsOpenGlSupported())
	{
		return;
	}

	DataSelection data_selection;
	data_selection.bmp = true;
	data_selection.work = true;
	data_selection.tissues = true;
	emit BeginDataexport(data_selection, this);

	m_SurfaceViewer = new SurfaceViewerWidget(m_Handler3D, static_cast<SurfaceViewerWidget::eInputType>(mode), nullptr);
	QObject_connect(m_SurfaceViewer, SIGNAL(Hasbeenclosed()), this, SLOT(SurfaceViewerClosed()));

	QObject_connect(m_SurfaceViewer, SIGNAL(BeginDatachange(DataSelection&,QWidget*,bool)), this, SLOT(HandleBeginDatachange(DataSelection&,QWidget*,bool)));
	QObject_connect(m_SurfaceViewer, SIGNAL(EndDatachange(QWidget*,eEndUndoAction)), this, SLOT(HandleEndDatachange(QWidget*,eEndUndoAction)));

	m_SurfaceViewer->show();
	m_SurfaceViewer->raise();
	m_SurfaceViewer->activateWindow();

	emit EndDataexport(this);
}

void MainWindow::ViewTissue(Point p)
{
	SelectTissue(p, true);
	StartSurfaceviewer(SurfaceViewerWidget::kSelectedTissues);
}

void MainWindow::ExecuteTissueSurfaceviewer()
{
	StartSurfaceviewer(SurfaceViewerWidget::kSelectedTissues);
}

void MainWindow::ExecuteSelectedtissueSurfaceviewer()
{
	StartSurfaceviewer(SurfaceViewerWidget::kSelectedTissues);
}

void MainWindow::ExecuteSourceSurfaceviewer()
{
	StartSurfaceviewer(SurfaceViewerWidget::kSource);
}

void MainWindow::ExecuteTargetSurfaceviewer()
{
	StartSurfaceviewer(SurfaceViewerWidget::kTarget);
}

void MainWindow::Execute3Dvolumeviewertissue()
{
	DataSelection data_selection;
	data_selection.tissues = true;
	emit BeginDataexport(data_selection, this);

	if (m_VV3D == nullptr)
	{
		m_VV3D = new VolumeViewerWidget(m_Handler3D, false, true, true, nullptr);
		QObject_connect(m_VV3D, SIGNAL(Hasbeenclosed()), this, SLOT(VV3DClosed()));
	}

	m_VV3D->show();
	m_VV3D->raise();

	emit EndDataexport(this);
}

void MainWindow::Execute3Dvolumeviewerbmp()
{
	DataSelection data_selection;
	data_selection.bmp = true;
	emit BeginDataexport(data_selection, this);

	if (m_VV3Dbmp == nullptr)
	{
		m_VV3Dbmp = new VolumeViewerWidget(m_Handler3D, true, true, true, nullptr);
		QObject_connect(m_VV3Dbmp, SIGNAL(Hasbeenclosed()), this, SLOT(VV3DbmpClosed()));
	}

	m_VV3Dbmp->show();
	m_VV3Dbmp->raise();

	emit EndDataexport(this);
}

void MainWindow::ExecuteSettings()
{
	Settings settings(this);
	settings.exec();
}

void MainWindow::UpdateBmp()
{
	m_BmpShow->update();
	if (m_Xsliceshower != nullptr)
	{
		m_Xsliceshower->BmpChanged();
	}
	if (m_Ysliceshower != nullptr)
	{
		m_Ysliceshower->BmpChanged();
	}
}

void MainWindow::UpdateWork()
{
	m_WorkShow->update();

	if (m_Xsliceshower != nullptr)
	{
		m_Xsliceshower->WorkChanged();
	}
	if (m_Ysliceshower != nullptr)
	{
		m_Ysliceshower->WorkChanged();
	}
}

void MainWindow::UpdateTissue()
{
	m_BmpShow->TissueChanged();
	m_WorkShow->TissueChanged();
	if (m_Xsliceshower != nullptr)
		m_Xsliceshower->TissueChanged();
	if (m_Ysliceshower != nullptr)
		m_Ysliceshower->TissueChanged();
	if (m_VV3D != nullptr)
		m_VV3D->TissueChanged();
	if (m_SurfaceViewer != nullptr)
		m_SurfaceViewer->TissueChanged();
}

void MainWindow::XsliceClosed()
{
	if (m_Xsliceshower != nullptr)
	{
		if (m_Ysliceshower != nullptr)
		{
			m_Ysliceshower->XyexistsChanged(false);
		}
		m_BmpShow->SetCrosshairyvisible(false);
		m_WorkShow->SetCrosshairyvisible(false);

		delete m_Xsliceshower;
		m_Xsliceshower = nullptr;
	}
}

void MainWindow::YsliceClosed()
{
	if (m_Ysliceshower != nullptr)
	{
		if (m_Xsliceshower != nullptr)
		{
			m_Xsliceshower->XyexistsChanged(false);
		}
		m_BmpShow->SetCrosshairxvisible(false);
		m_WorkShow->SetCrosshairxvisible(false);

		delete m_Ysliceshower;
		m_Ysliceshower = nullptr;
	}
}

void MainWindow::SurfaceViewerClosed()
{
	if (m_SurfaceViewer != nullptr)
	{
		delete m_SurfaceViewer;
		m_SurfaceViewer = nullptr;
	}
}

void MainWindow::VV3DClosed()
{
	if (m_VV3D != nullptr)
	{
		delete m_VV3D;
		m_VV3D = nullptr;
	}
}

void MainWindow::VV3DbmpClosed()
{
	if (m_VV3Dbmp != nullptr)
	{
		delete m_VV3Dbmp;
		m_VV3Dbmp = nullptr;
	}
}

void MainWindow::XshowerSlicechanged()
{
	if (m_Ysliceshower != nullptr)
	{
		m_Ysliceshower->XyposChanged(m_Xsliceshower->GetSlicenr());
	}
	m_BmpShow->CrosshairyChanged(m_Xsliceshower->GetSlicenr());
	m_WorkShow->CrosshairyChanged(m_Xsliceshower->GetSlicenr());
}

void MainWindow::YshowerSlicechanged()
{
	if (m_Xsliceshower != nullptr)
	{
		m_Xsliceshower->XyposChanged(m_Ysliceshower->GetSlicenr());
	}
	m_BmpShow->CrosshairxChanged(m_Ysliceshower->GetSlicenr());
	m_WorkShow->CrosshairxChanged(m_Ysliceshower->GetSlicenr());
}

void MainWindow::SetWorkContentsPos(int x, int y)
{
	if (m_TomoveScroller)
	{
		m_TomoveScroller = false;
		m_WorkScroller->setContentsPos(x, y);
	}
	else
	{
		m_TomoveScroller = true;
	}
}

void MainWindow::SetBmpContentsPos(int x, int y)
{
	if (m_TomoveScroller)
	{
		m_TomoveScroller = false;
		m_BmpScroller->setContentsPos(x, y);
	}
	else
	{
		m_TomoveScroller = true;
	}
}

void MainWindow::ExecuteHisto()
{
	DataSelection data_selection;
	data_selection.work = true;
	emit BeginDataexport(data_selection, this);

	ShowHisto sh(m_Handler3D, this);
	sh.move(QCursor::pos());
	sh.exec();

	emit EndDataexport(this);
}

void MainWindow::ExecuteScale()
{
	ScaleWork sw(m_Handler3D, m_MPicpath, this);
	QObject_connect(&sw, SIGNAL(BeginDatachange(DataSelection&,QWidget*,bool)), this, SLOT(HandleBeginDatachange(DataSelection&,QWidget*,bool)));
	QObject_connect(&sw, SIGNAL(EndDatachange(QWidget*,eEndUndoAction)), this, SLOT(HandleEndDatachange(QWidget*,eEndUndoAction)));
	sw.move(QCursor::pos());
	sw.exec();
	QObject_disconnect(&sw, SIGNAL(BeginDatachange(DataSelection&,QWidget*,bool)), this, SLOT(HandleBeginDatachange(DataSelection&,QWidget*,bool)));
	QObject_disconnect(&sw, SIGNAL(EndDatachange(QWidget*,eEndUndoAction)), this, SLOT(HandleEndDatachange(QWidget*,eEndUndoAction)));
}

void MainWindow::ExecuteImagemath()
{
	ImageMath im(m_Handler3D, this);
	QObject_connect(&im, SIGNAL(BeginDatachange(DataSelection&,QWidget*,bool)), this, SLOT(HandleBeginDatachange(DataSelection&,QWidget*,bool)));
	QObject_connect(&im, SIGNAL(EndDatachange(QWidget*,eEndUndoAction)), this, SLOT(HandleEndDatachange(QWidget*,eEndUndoAction)));
	im.move(QCursor::pos());
	im.exec();
	QObject_disconnect(&im, SIGNAL(BeginDatachange(DataSelection&,QWidget*,bool)), this, SLOT(HandleBeginDatachange(DataSelection&,QWidget*,bool)));
	QObject_disconnect(&im, SIGNAL(EndDatachange(QWidget*,eEndUndoAction)), this, SLOT(HandleEndDatachange(QWidget*,eEndUndoAction)));
}

void MainWindow::ExecuteUnwrap()
{
	DataSelection data_selection;
	data_selection.allSlices = true;
	data_selection.work = true;
	emit BeginDatachange(data_selection, this);

	m_Handler3D->Unwrap(0.90f);

	emit EndDatachange(this);
}

void MainWindow::ExecuteOverlay()
{
	ImageOverlay io(m_Handler3D, this);
	QObject_connect(&io, SIGNAL(BeginDatachange(DataSelection&,QWidget*,bool)), this, SLOT(HandleBeginDatachange(DataSelection&,QWidget*,bool)));
	QObject_connect(&io, SIGNAL(EndDatachange(QWidget*,eEndUndoAction)), this, SLOT(HandleEndDatachange(QWidget*,eEndUndoAction)));
	io.move(QCursor::pos());
	io.exec();
	QObject_disconnect(&io, SIGNAL(BeginDatachange(DataSelection&,QWidget*,bool)), this, SLOT(HandleBeginDatachange(DataSelection&,QWidget*,bool)));
	QObject_disconnect(&io, SIGNAL(EndDatachange(QWidget*,eEndUndoAction)), this, SLOT(HandleEndDatachange(QWidget*,eEndUndoAction)));
}

void MainWindow::ExecutePixelsize()
{
	PixelResize pr(m_Handler3D, this);
	pr.move(QCursor::pos());
	if (pr.exec())
	{
		auto p = pr.GetPixelsize();
		if (!(p == m_Handler3D->Spacing()))
		{
			m_Handler3D->SetPixelsize(p.x, p.y);
			m_Handler3D->SetSlicethickness(p.z);
			PixelsizeChanged();
			SlicethicknessChanged();
		}
	}
}

void MainWindow::PixelsizeChanged()
{
	Pair p = m_Handler3D->GetPixelsize();
	if (m_Xsliceshower != nullptr)
		m_Xsliceshower->PixelsizeChanged(p);
	if (m_Ysliceshower != nullptr)
		m_Ysliceshower->PixelsizeChanged(p);
	m_BmpShow->PixelsizeChanged(p);
	m_WorkShow->PixelsizeChanged(p);
	if (m_VV3D != nullptr)
	{
		m_VV3D->PixelsizeChanged(p);
	}
	if (m_VV3Dbmp != nullptr)
	{
		m_VV3Dbmp->PixelsizeChanged(p);
	}
	if (m_SurfaceViewer != nullptr)
	{
		m_SurfaceViewer->PixelsizeChanged(p);
	}
}

void MainWindow::ExecuteDisplacement()
{
	DisplacementDialog dd(m_Handler3D, this);
	dd.move(QCursor::pos());
	if (dd.exec())
	{
		float disp[3];
		dd.ReturnDisplacement(disp);
		m_Handler3D->SetDisplacement(disp);
	}
}

void MainWindow::ExecuteRotation()
{
	RotationDialog rd(m_Handler3D, this);
	rd.move(QCursor::pos());
	if (rd.exec())
	{
		float rot[3][3];
		rd.GetRotation(rot);

		auto tr = m_Handler3D->ImageTransform();
		tr.SetRotation(rot);
		m_Handler3D->SetTransform(tr);
	}
}

void MainWindow::ExecuteUndoconf()
{
	UndoConfigurationDialog uc(m_Handler3D, this);
	uc.move(QCursor::pos());
	uc.exec();

	m_Undonr->setEnabled(m_Handler3D->ReturnNrundo() > 0);
	m_Redonr->setEnabled(m_Handler3D->ReturnNrredo() > 0);

	this->SaveSettings();
}

void MainWindow::ExecuteActiveslicesconf()
{
	ActiveSlicesConfigDialog ac(m_Handler3D, this);
	ac.move(QCursor::pos());
	ac.exec();

	unsigned short slicenr = m_Handler3D->ActiveSlice() + 1;
	if (m_Handler3D->StartSlice() >= slicenr ||
			m_Handler3D->EndSlice() + 1 <= slicenr)
	{
		m_LbInactivewarning->setText(QString("   3D Inactive Slice!   "));
	}
	else
	{
		m_LbInactivewarning->setText(QString(" "));
	}
}

void MainWindow::ExecuteHideparameters(bool checked)
{
	WidgetInterface::SetHideParams(checked);
	if (m_TabOld != nullptr)
		m_TabOld->HideParamsChanged();
}

void MainWindow::ExecuteHidezoom(bool checked)
{
	if (!checked)
	{
		m_ZoomWidget->hide();
	}
	else
	{
		m_ZoomWidget->show();
	}
}

void MainWindow::ExecuteHidecontrastbright(bool checked)
{
	if (!checked)
	{
		m_LbContrastbmp->hide();
		m_LeContrastbmpVal->hide();
		m_LbContrastbmpVal->hide();
		m_SlContrastbmp->hide();
		m_LbBrightnessbmp->hide();
		m_LeBrightnessbmpVal->hide();
		m_LbBrightnessbmpVal->hide();
		m_SlBrightnessbmp->hide();
		m_LbContrastwork->hide();
		m_LeContrastworkVal->hide();
		m_LbContrastworkVal->hide();
		m_SlContrastwork->hide();
		m_LbBrightnesswork->hide();
		m_LeBrightnessworkVal->hide();
		m_LbBrightnessworkVal->hide();
		m_SlBrightnesswork->hide();
	}
	else
	{
		m_LbContrastbmp->show();
		m_LeContrastbmpVal->show();
		m_LbContrastbmpVal->show();
		m_SlContrastbmp->show();
		m_LbBrightnessbmp->show();
		m_LeBrightnessbmpVal->show();
		m_LbBrightnessbmpVal->show();
		m_SlBrightnessbmp->show();
		m_LbContrastwork->show();
		m_LeContrastworkVal->show();
		m_LbContrastworkVal->show();
		m_SlContrastwork->show();
		m_LbBrightnesswork->show();
		m_LeBrightnessworkVal->show();
		m_LbBrightnessworkVal->show();
		m_SlBrightnesswork->show();
	}
}

void MainWindow::ExecuteHidesource(bool checked)
{
	if (!checked)
	{
		m_Vboxbmpw->hide();
	}
	else
	{
		m_Vboxbmpw->show();
	}
}

void MainWindow::ExecuteHidetarget(bool checked)
{
	if (!checked)
	{
		m_Vboxworkw->hide();
	}
	else
	{
		m_Vboxworkw->show();
	}
}

void MainWindow::ExecuteHidecopyswap(bool checked)
{
	if (!checked)
	{
		m_ToworkBtn->hide();
		m_TobmpBtn->hide();
		m_SwapBtn->hide();
		m_SwapAllBtn->hide();
	}
	else
	{
		m_ToworkBtn->show();
		m_TobmpBtn->show();
		m_SwapBtn->show();
		m_SwapAllBtn->show();
	}
}

void MainWindow::ExecuteHidestack(bool checked)
{
	if (!checked)
	{
		m_BitstackWidget->hide();
	}
	else
	{
		m_BitstackWidget->show();
	}
}

void MainWindow::ExecuteHideoverlay(bool checked)
{
	if (!checked)
	{
		m_OverlayWidget->hide();
	}
	else
	{
		m_OverlayWidget->show();
	}
}

void MainWindow::ExecuteMultidataset(bool checked)
{
	if (!checked)
	{
		m_MultidatasetWidget->hide();
	}
	else
	{
		m_MultidatasetWidget->show();
	}
}

void MainWindow::ExecuteHidenotes(bool checked)
{
	if (!checked)
	{
		m_MNotes->hide();
	}
	else
	{
		m_MNotes->show();
	}
}

void MainWindow::ExecuteShowtabtoggled(bool)
{
	auto nrtabbuttons = (unsigned short)m_Tabwidgets.size();
	for (unsigned short i = 0; i < nrtabbuttons; i++)
	{
		m_ShowpbTab[i] = m_ShowtabAction[i]->isChecked();
	}

	WidgetInterface* currentwidget = static_cast<WidgetInterface*>(m_MethodTab->currentWidget());
	unsigned short i = 0;
	while ((i < nrtabbuttons) && (currentwidget != m_Tabwidgets[i]))
		i++;
	if (i == nrtabbuttons)
	{
		UpdateTabvisibility();
		return;
	}

	if (!m_ShowpbTab[i])
	{
		i = 0;
		while ((i < nrtabbuttons) && !m_ShowpbTab[i])
			i++;
		if (i != nrtabbuttons)
		{
			m_MethodTab->setCurrentWidget(m_Tabwidgets[i]);
		}
	}
	UpdateTabvisibility();
}

void MainWindow::ExecuteXslice()
{
	DataSelection data_selection;
	data_selection.bmp = true;
	data_selection.work = true;
	data_selection.tissues = true;
	emit BeginDataexport(data_selection, this);

	if (m_Xsliceshower == nullptr)
	{
		m_Xsliceshower = new SliceViewerWidget(m_Handler3D, true, m_Handler3D->GetSlicethickness(), m_ZoomWidget->GetZoom(), nullptr);
		m_Xsliceshower->ZposChanged();
		if (m_Ysliceshower != nullptr)
		{
			m_Xsliceshower->XyposChanged(m_Ysliceshower->GetSlicenr());
			m_Xsliceshower->XyexistsChanged(true);
			m_Ysliceshower->XyexistsChanged(true);
		}
		m_BmpShow->SetCrosshairyvisible(m_CbBmpcrosshairvisible->isChecked());
		m_WorkShow->SetCrosshairyvisible(m_CbWorkcrosshairvisible->isChecked());
		XshowerSlicechanged();
		float offset1, factor1;
		m_BmpShow->GetScaleoffsetfactor(offset1, factor1);
		m_Xsliceshower->SetScale(offset1, factor1, true);
		m_WorkShow->GetScaleoffsetfactor(offset1, factor1);
		m_Xsliceshower->SetScale(offset1, factor1, false);
		QObject_connect(m_BmpShow, SIGNAL(ScaleoffsetfactorChanged(float,float,bool)), m_Xsliceshower, SLOT(SetScale(float,float,bool)));
		QObject_connect(m_WorkShow, SIGNAL(ScaleoffsetfactorChanged(float,float,bool)), m_Xsliceshower, SLOT(SetScale(float,float,bool)));
		QObject_connect(m_Xsliceshower, SIGNAL(SliceChanged(int)), this, SLOT(XshowerSlicechanged()));
		QObject_connect(m_Xsliceshower, SIGNAL(Hasbeenclosed()), this, SLOT(XsliceClosed()));
		QObject_connect(m_ZoomWidget, SIGNAL(SetZoom(double)), m_Xsliceshower, SLOT(SetZoom(double)));
	}

	m_Xsliceshower->show();
	m_Xsliceshower->raise(); //xxxa
	m_Xsliceshower->activateWindow();

	emit EndDataexport(this);
}

void MainWindow::ExecuteYslice()
{
	DataSelection data_selection;
	data_selection.bmp = true;
	data_selection.work = true;
	data_selection.tissues = true;
	emit BeginDataexport(data_selection, this);

	if (m_Ysliceshower == nullptr)
	{
		m_Ysliceshower = new SliceViewerWidget(m_Handler3D, false, m_Handler3D->GetSlicethickness(), m_ZoomWidget->GetZoom(), nullptr);
		m_Ysliceshower->ZposChanged();
		if (m_Xsliceshower != nullptr)
		{
			m_Ysliceshower->XyposChanged(m_Xsliceshower->GetSlicenr());
			m_Ysliceshower->XyexistsChanged(true);
			m_Xsliceshower->XyexistsChanged(true);
		}
		m_BmpShow->SetCrosshairxvisible(m_CbBmpcrosshairvisible->isChecked());
		m_WorkShow->SetCrosshairxvisible(m_CbWorkcrosshairvisible->isChecked());
		YshowerSlicechanged();
		float offset1, factor1;
		m_BmpShow->GetScaleoffsetfactor(offset1, factor1);
		m_Ysliceshower->SetScale(offset1, factor1, true);
		m_WorkShow->GetScaleoffsetfactor(offset1, factor1);
		m_Ysliceshower->SetScale(offset1, factor1, false);
		QObject_connect(m_BmpShow, SIGNAL(ScaleoffsetfactorChanged(float,float,bool)), m_Ysliceshower, SLOT(SetScale(float,float,bool)));
		QObject_connect(m_WorkShow, SIGNAL(ScaleoffsetfactorChanged(float,float,bool)), m_Ysliceshower, SLOT(SetScale(float,float,bool)));
		QObject_connect(m_Ysliceshower, SIGNAL(SliceChanged(int)), this, SLOT(YshowerSlicechanged()));
		QObject_connect(m_Ysliceshower, SIGNAL(Hasbeenclosed()), this, SLOT(YsliceClosed()));
		QObject_connect(m_ZoomWidget, SIGNAL(SetZoom(double)), m_Ysliceshower, SLOT(SetZoom(double)));
	}

	m_Ysliceshower->resize(600, 600);
	m_Ysliceshower->show();
	m_Ysliceshower->raise();
	m_Ysliceshower->activateWindow();

	emit EndDataexport(this);
}

void MainWindow::ExecuteRemovetissues()
{
	QString filename = RecentPlaces::GetOpenFileName(this, "Open file", QString(), "Text (*.txt)\nAll (*.*)");
	if (!filename.isEmpty())
	{
		std::vector<tissues_size_t> types;
		if (read_tissues(filename.toAscii(), types))
		{
			// this actually goes through slices and removes it from segmentation
			m_Handler3D->RemoveTissues(std::set<tissues_size_t>(types.begin(), types.end()));

			m_TissueTreeWidget->UpdateTreeWidget();
			TissuenrChanged(m_TissueTreeWidget->GetCurrentType() - 1);
		}
		else
		{
			QMessageBox::warning(this, "iSeg", "Error: not all tissues are in tissue list", QMessageBox::Ok | QMessageBox::Default);
		}
	}
}

void MainWindow::ExecuteGrouptissues()
{
	std::vector<tissues_size_t> olds, news;

	QString filename = RecentPlaces::GetOpenFileName(this, "Open file", QString(), "Text (*.txt)\nAll (*.*)");
	if (!filename.isEmpty())
	{
		bool fail_on_unknown_tissue = true;
		if (read_grouptissuescapped(filename.toAscii(), olds, news, fail_on_unknown_tissue))
		{
			DataSelection data_selection;
			data_selection.allSlices = true;
			data_selection.tissues = true;
			emit BeginDatachange(data_selection, this);

			m_Handler3D->GroupTissues(olds, news);

			emit EndDatachange(this);
		}
		else
		{
			QMessageBox::warning(this, "iSeg", "Error: not all tissues are in tissue list", QMessageBox::Ok | QMessageBox::Default);
		}
	}
}

void MainWindow::ExecuteAbout()
{
	std::ostringstream ss;
	ss << "\n\niSeg\n"
		 << std::string(xstr(ISEG_DESCRIPTION));
	QMessageBox::about(this, "About", QString(ss.str().c_str()));
}

void MainWindow::AddMark(Point p)
{
	DataSelection data_selection;
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.marks = true;
	emit BeginDatachange(data_selection, this);

	m_Handler3D->AddMark(p, m_TissueTreeWidget->GetCurrentType());

	emit EndDatachange();
}

void MainWindow::AddLabel(Point p, std::string str)
{
	DataSelection data_selection;
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.marks = true;
	emit BeginDatachange(data_selection, this);

	m_Handler3D->AddMark(p, m_TissueTreeWidget->GetCurrentType(), str);

	emit EndDatachange(this);
}

void MainWindow::ClearMarks()
{
	DataSelection data_selection;
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.marks = true;
	emit BeginDatachange(data_selection, this);

	m_Handler3D->ClearMarks();

	emit EndDatachange(this);
}

void MainWindow::RemoveMark(Point p)
{
	DataSelection data_selection;
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.marks = true;
	emit BeginDatachange(data_selection, this);

	if (m_Handler3D->RemoveMark(p, 3))
	{
		emit EndDatachange(this);
	}
	else
	{
		emit EndDatachange(this, iseg::AbortUndo);
	}
}

void MainWindow::SelectTissue(Point p, bool clear_selection)
{
	auto const type = m_Handler3D->GetTissuePt(p, m_Handler3D->ActiveSlice());

	if (clear_selection)
	{
		const auto list = m_TissueTreeWidget->selectedItems();
		for (auto& item : list)
		{
			// avoid unselecting if should be selected
			item->setSelected(m_TissueTreeWidget->GetType(item) == type);
		}
	}

	// remove filter if it is preventing tissue from being shown
	if (!m_TissueTreeWidget->IsVisible(type))
	{
		m_TissueFilter->setText(QString(""));
	}
	// now select the tissue
	if (clear_selection && type > 0)
	{
		m_TissueTreeWidget->SetCurrentTissue(type);
	}
	else
	{
		for (auto item : m_TissueTreeWidget->GetAllItems())
		{
			if (m_TissueTreeWidget->GetType(item) == type)
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

void MainWindow::NextFeaturingSlice()
{
	tissues_size_t type = m_TissueTreeWidget->GetCurrentType();
	if (type <= 0)
	{
		return;
	}

	bool found;
	unsigned short nextslice = m_Handler3D->GetNextFeaturingSlice(type, found);
	if (found)
	{
		m_Handler3D->SetActiveSlice(nextslice);
		SliceChanged();
	}
	else
	{
		QMessageBox::information(this, "iSeg", "The selected tissue its empty.\n");
	}
}

void MainWindow::AddTissue(Point p)
{
	DataSelection data_selection;
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.tissues = true;
	emit BeginDatachange(data_selection, this);

	tissues_size_t curr_tissue_type = m_TissueTreeWidget->GetCurrentType();
	m_Handler3D->Add2tissue(curr_tissue_type, p, m_CbAddsuboverride->isChecked());

	emit EndDatachange(this);
}

void MainWindow::AddTissueConnected(Point p)
{
	DataSelection data_selection;
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.tissues = true;
	emit BeginDatachange(data_selection, this);

	tissues_size_t curr_tissue_type = m_TissueTreeWidget->GetCurrentType();
	m_Handler3D->Add2tissueConnected(curr_tissue_type, p, m_CbAddsuboverride->isChecked());

	emit EndDatachange(this);
}

void MainWindow::AddTissuelarger(Point p)
{
	DataSelection data_selection;
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.tissues = true;
	emit BeginDatachange(data_selection, this);

	tissues_size_t curr_tissue_type = m_TissueTreeWidget->GetCurrentType();
	m_Handler3D->Add2tissueThresh(curr_tissue_type, p);

	emit EndDatachange(this);
}

void MainWindow::AddTissue3D(Point p)
{
	DataSelection data_selection;
	data_selection.allSlices = true;
	data_selection.tissues = true;
	emit BeginDatachange(data_selection, this);

	tissues_size_t curr_tissue_type = m_TissueTreeWidget->GetCurrentType();
	m_Handler3D->Add2tissueall(curr_tissue_type, p, m_CbAddsuboverride->isChecked());

	emit EndDatachange(this);
}

void MainWindow::SubtractTissue(Point p)
{
	DataSelection data_selection;
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.tissues = true;
	emit BeginDatachange(data_selection, this);

	tissues_size_t curr_tissue_type = m_TissueTreeWidget->GetCurrentType();
	m_Handler3D->SubtractTissue(curr_tissue_type, p);

	emit EndDatachange();
}

void MainWindow::AddTissueClicked(Point p)
{
	QObject_disconnect(m_WorkShow, SIGNAL(MousepressedSign(Point)), this, SLOT(AddTissueClicked(Point)));
	m_PbAdd->setChecked(false);
	QObject_connect(m_WorkShow, SIGNAL(MousereleasedSign(Point)), this, SLOT(ReconnectmouseAfterrelease(Point)));
	AddholdTissueClicked(p);
}

void MainWindow::AddholdTissueClicked(Point p)
{
	DataSelection data_selection;
	data_selection.allSlices = m_CbAddsub3d->isChecked();
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.tissues = true;
	emit BeginDatachange(data_selection, this);

	tissues_size_t curr_tissue_type = m_TissueTreeWidget->GetCurrentType();
	if (m_CbAddsub3d->isChecked())
	{
		QApplication::setOverrideCursor(QCursor(Qt::waitCursor));
		if (m_CbAddsubconn->isChecked())
			m_Handler3D->Add2tissueallConnected(curr_tissue_type, p, m_CbAddsuboverride->isChecked());
		else
			m_Handler3D->Add2tissueall(curr_tissue_type, p, m_CbAddsuboverride->isChecked());
		QApplication::restoreOverrideCursor();
	}
	else
	{
		if (m_CbAddsubconn->isChecked())
			m_Handler3D->Add2tissueConnected(curr_tissue_type, p, m_CbAddsuboverride->isChecked());
		else
			m_Handler3D->Add2tissue(curr_tissue_type, p, m_CbAddsuboverride->isChecked());
	}

	emit EndDatachange(this);
}

void MainWindow::SubtractTissueClicked(Point p)
{
	QObject_disconnect(m_WorkShow, SIGNAL(MousepressedSign(Point)), this, SLOT(SubtractTissueClicked(Point)));
	m_PbSub->setChecked(false);
	QObject_connect(m_WorkShow, SIGNAL(MousereleasedSign(Point)), this, SLOT(ReconnectmouseAfterrelease(Point)));
	SubtractholdTissueClicked(p);
}

void MainWindow::SubtractholdTissueClicked(Point p)
{
	DataSelection data_selection;
	data_selection.allSlices = m_CbAddsub3d->isChecked();
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.tissues = true;
	emit BeginDatachange(data_selection, this);

	tissues_size_t curr_tissue_type = m_TissueTreeWidget->GetCurrentType();
	if (m_CbAddsub3d->isChecked())
	{
		if (m_CbAddsubconn->isChecked())
			m_Handler3D->SubtractTissueallConnected(curr_tissue_type, p);
		else
			m_Handler3D->SubtractTissueall(curr_tissue_type, p);
	}
	else
	{
		if (m_CbAddsubconn->isChecked())
			m_Handler3D->SubtractTissueConnected(curr_tissue_type, p);
		else
			m_Handler3D->SubtractTissue(curr_tissue_type, p);
	}

	emit EndDatachange(this);
}

void MainWindow::AddTissuePushed()
{
	if (m_PbSub->isChecked())
	{
		QObject_disconnect(m_WorkShow, SIGNAL(MousepressedSign(Point)), this, SLOT(SubtractTissueClicked(Point)));
		m_PbSub->setChecked(false);
	}
	if (m_PbSubhold->isChecked())
	{
		QObject_disconnect(m_WorkShow, SIGNAL(MousepressedSign(Point)), this, SLOT(SubtractholdTissueClicked(Point)));
		m_PbSubhold->setChecked(false);
	}
	if (m_PbAddhold->isChecked())
	{
		QObject_disconnect(m_WorkShow, SIGNAL(MousepressedSign(Point)), this, SLOT(AddholdTissueClicked(Point)));
		m_PbAddhold->setChecked(false);
	}

	if (m_PbAdd->isChecked())
	{
		QObject_connect(m_WorkShow, SIGNAL(MousepressedSign(Point)), this, SLOT(AddTissueClicked(Point)));
		DisconnectMouseclick();
	}
	else
	{
		QObject_disconnect(m_WorkShow, SIGNAL(MousepressedSign(Point)), this, SLOT(AddTissueClicked(Point)));
		ConnectMouseclick();
	}
}

void MainWindow::AddTissueShortkey()
{
	m_PbAdd->toggle();
	AddTissuePushed();
}

void MainWindow::SubtractTissueShortkey()
{
	m_PbSub->toggle();
	SubtractTissuePushed();
}

void MainWindow::AddholdTissuePushed()
{
	if (m_PbSub->isChecked())
	{
		QObject_disconnect(m_WorkShow, SIGNAL(MousepressedSign(Point)), this, SLOT(SubtractTissueClicked(Point)));
		m_PbSub->setChecked(false);
	}
	if (m_PbSubhold->isChecked())
	{
		QObject_disconnect(m_WorkShow, SIGNAL(MousepressedSign(Point)), this, SLOT(SubtractholdTissueClicked(Point)));
		m_PbSubhold->setChecked(false);
	}
	if (m_PbAdd->isChecked())
	{
		QObject_disconnect(m_WorkShow, SIGNAL(MousepressedSign(Point)), this, SLOT(AddTissueClicked(Point)));
		m_PbAdd->setChecked(false);
	}

	if (m_PbAddhold->isChecked())
	{
		QObject_connect(m_WorkShow, SIGNAL(MousepressedSign(Point)), this, SLOT(AddholdTissueClicked(Point)));
		DisconnectMouseclick();
	}
	else
	{
		QObject_disconnect(m_WorkShow, SIGNAL(MousepressedSign(Point)), this, SLOT(AddholdTissueClicked(Point)));
		ConnectMouseclick();
	}
}

void MainWindow::SubtractTissuePushed()
{
	if (m_PbAdd->isChecked())
	{
		QObject_disconnect(m_WorkShow, SIGNAL(MousepressedSign(Point)), this, SLOT(AddTissueClicked(Point)));
		m_PbAdd->setChecked(false);
	}
	if (m_PbSub->isChecked())
	{
		QObject_connect(m_WorkShow, SIGNAL(MousepressedSign(Point)), this, SLOT(SubtractTissueClicked(Point)));
		DisconnectMouseclick();
	}
	else
	{
		QObject_disconnect(m_WorkShow, SIGNAL(MousepressedSign(Point)), this, SLOT(SubtractTissueClicked(Point)));
		ConnectMouseclick();
	}
	if (m_PbSubhold->isChecked())
	{
		QObject_disconnect(m_WorkShow, SIGNAL(MousepressedSign(Point)), this, SLOT(SubtractholdTissueClicked(Point)));
		m_PbSubhold->setChecked(false);
	}
	if (m_PbAddhold->isChecked())
	{
		QObject_disconnect(m_WorkShow, SIGNAL(MousepressedSign(Point)), this, SLOT(AddholdTissueClicked(Point)));
		m_PbAddhold->setChecked(false);
	}
}

void MainWindow::SubtractholdTissuePushed()
{
	if (m_PbAdd->isChecked())
	{
		QObject_disconnect(m_WorkShow, SIGNAL(MousepressedSign(Point)), this, SLOT(AddTissueClicked(Point)));
		m_PbAdd->setChecked(false);
	}
	if (m_PbSubhold->isChecked())
	{
		QObject_connect(m_WorkShow, SIGNAL(MousepressedSign(Point)), this, SLOT(SubtractholdTissueClicked(Point)));
		DisconnectMouseclick();
	}
	else
	{
		QObject_disconnect(m_WorkShow, SIGNAL(MousepressedSign(Point)), this, SLOT(SubtractholdTissueClicked(Point)));
		ConnectMouseclick();
	}
	if (m_PbSub->isChecked())
	{
		QObject_disconnect(m_WorkShow, SIGNAL(MousepressedSign(Point)), this, SLOT(SubtractTissueClicked(Point)));
		m_PbSub->setChecked(false);
	}
	if (m_PbAddhold->isChecked())
	{
		QObject_disconnect(m_WorkShow, SIGNAL(MousepressedSign(Point)), this, SLOT(AddholdTissueClicked(Point)));
		m_PbAddhold->setChecked(false);
	}
}

void MainWindow::MaskSource()
{
	const bool mask3d = m_CbMask3d->isChecked();
	const float maskvalue = m_EditMaskValue->text().toFloat();

	DataSelection data_selection;
	data_selection.allSlices = mask3d;
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.bmp = true;
	emit BeginDatachange(data_selection, this);

	m_Handler3D->MaskSource(mask3d, maskvalue);

	emit EndDatachange(this);
}

void MainWindow::StopholdTissuePushed()
{
	if (m_PbAdd->isDown())
	{
		QObject_disconnect(m_WorkShow, SIGNAL(MousepressedSign(Point)), this, SLOT(AddTissueClicked(Point)));
		m_PbAdd->setDown(false);
	}
	if (m_PbSubhold->isChecked())
	{
		QObject_disconnect(m_WorkShow, SIGNAL(MousepressedSign(Point)), this, SLOT(SubtractholdTissueClicked(Point)));
		m_PbSubhold->setChecked(false);
	}
	if (m_PbSub->isDown())
	{
		QObject_disconnect(m_WorkShow, SIGNAL(MousepressedSign(Point)), this, SLOT(SubtractTissueClicked(Point)));
		m_PbSub->setDown(false);
	}
	if (m_PbAddhold->isChecked())
	{
		QObject_disconnect(m_WorkShow, SIGNAL(MousepressedSign(Point)), this, SLOT(AddholdTissueClicked(Point)));
		m_PbAddhold->setChecked(false);
	}
}

void MainWindow::DoWork2tissue()
{
	DataSelection data_selection;
	data_selection.allSlices = true;
	data_selection.tissues = true;
	emit BeginDatachange(data_selection, this);

	m_Handler3D->Work2tissueall();

	tissues_size_t m;
	m_Handler3D->GetRangetissue(&m);
	m_Handler3D->Buildmissingtissues(m);
	m_TissueTreeWidget->UpdateTreeWidget();

	tissues_size_t curr_tissue_type = m_TissueTreeWidget->GetCurrentType();
	TissuenrChanged(curr_tissue_type - 1);

	emit EndDatachange(this);
}

void MainWindow::DoTissue2work()
{
	DataSelection data_selection;
	data_selection.allSlices = true;
	data_selection.work = true;
	emit BeginDatachange(data_selection, this);

	m_Handler3D->Tissue2workall3D();

	emit EndDatachange(this);
}

void MainWindow::DoWork2tissueGrouped()
{
	std::vector<tissues_size_t> olds, news;

	QString filename = RecentPlaces::GetOpenFileName(this, "Open file", QString(), "Text (*.txt)\nAll (*.*)");
	if (!filename.isEmpty())
	{
		if (read_grouptissues(filename.toAscii(), olds, news))
		{
			DataSelection data_selection;
			data_selection.allSlices = true;
			data_selection.tissues = true;
			emit BeginDatachange(data_selection, this);

			m_Handler3D->Work2tissueall();
			m_Handler3D->GroupTissues(olds, news);

			tissues_size_t m;
			m_Handler3D->GetRangetissue(&m);
			m_Handler3D->Buildmissingtissues(m);
			m_TissueTreeWidget->UpdateTreeWidget();

			tissues_size_t curr_tissue_type = m_TissueTreeWidget->GetCurrentType();
			TissuenrChanged(curr_tissue_type - 1); // TODO BL is this a bug? (I added the -1)

			emit EndDatachange(this);
		}
	}
}

void MainWindow::RandomizeColors()
{
	const auto sel = m_TissueTreeWidget->selectedItems();
	if (!sel.empty())
	{
		static Color random = Color(0.1f, 0.9f, 0.1f);
		for (const auto& item : sel)
		{
			tissues_size_t label = m_TissueTreeWidget->GetType(item);
			random = Color::NextRandom(random);
			TissueInfos::SetTissueColor(label, random.r, random.g, random.b);
		}

		m_TissueTreeWidget->UpdateTissueIcons();
	}

	UpdateTissue();
}

void MainWindow::TissueFilterChanged(const QString& text)
{
	m_TissueTreeWidget->SetTissueFilter(text);
}

void MainWindow::NewTissuePressed()
{
	TissueAdder ta(false, m_TissueTreeWidget, this);
	//TA.move(QCursor::pos());
	ta.exec();

	tissues_size_t curr_tissue_type = m_TissueTreeWidget->GetCurrentType();
	TissuenrChanged(curr_tissue_type - 1);
}

void MainWindow::Merge()
{
	const auto sel = m_TissueTreeWidget->selectedItems();
	for (const auto& item : sel)
	{
		if (TissueInfos::GetTissueLocked(m_TissueTreeWidget->GetType(item)))
		{
			QMessageBox::warning(this, "iSeg", "Locked tissues can not be merged.", QMessageBox::Ok | QMessageBox::Default);
			return;
		}
	}

	int ret = QMessageBox::warning(this, "iSeg", "Do you really want to merge the selected tissues?", QMessageBox::Yes | QMessageBox::Default, QMessageBox::No, QMessageBox::Cancel | QMessageBox::Escape);
	if (ret == QMessageBox::Yes)
	{
		bool option_3d_checked = m_Tissue3Dopt->isChecked();
		m_Tissue3Dopt->setChecked(true);
		Selectedtissue2work();

		const auto list = m_TissueTreeWidget->selectedItems();
		TissueAdder ta(false, m_TissueTreeWidget, this);
		ta.exec();

		tissues_size_t curr_tissue_type = m_TissueTreeWidget->GetCurrentType();
		TissuenrChanged(curr_tissue_type - 1);
		m_Handler3D->Mergetissues(curr_tissue_type);

		Removeselected(std::vector<QTreeWidgetItem*>(list.begin(), list.end()), false);
		m_Tissue3Dopt->setChecked(option_3d_checked);
	}
}

void MainWindow::NewFolderPressed()
{
	TissueFolderAdder dialog(m_TissueTreeWidget, this);
	dialog.exec();
}

void MainWindow::LockAllTissues()
{
	bool lockstate = m_LockTissues->isChecked();
	m_CbTissuelock->setChecked(lockstate);
	TissueInfos::SetTissuesLocked(lockstate);
	m_TissueTreeWidget->UpdateFolderIcons();
	m_TissueTreeWidget->UpdateTissueIcons();
}

void MainWindow::ModifTissueFolderPressed()
{
	if (m_TissueTreeWidget->GetCurrentIsFolder())
	{
		ModifFolder();
	}
	else
	{
		ModifTissue();
	}
}

void MainWindow::ModifTissue()
{
	DataSelection data_selection;
	data_selection.allSlices = true;
	data_selection.tissues = true;
	emit BeginDatachange(data_selection, this, false);

	//tissues_size_t nr = tissueTreeWidget->get_current_type() - 1;

	TissueAdder ta(true, m_TissueTreeWidget, this);
	ta.exec();

	emit EndDatachange(this, iseg::ClearUndo);
}

void MainWindow::ModifFolder()
{
	bool ok = false;
	QString new_folder_name = QInputDialog::getText("Folder name", "Enter a name for the folder:", QLineEdit::Normal, m_TissueTreeWidget->GetCurrentName(), &ok, this);
	if (ok)
	{
		m_TissueTreeWidget->SetCurrentFolderName(new_folder_name);
	}
}

void MainWindow::RemoveTissueFolderPressed()
{
	if (m_TissueTreeWidget->GetCurrentIsFolder())
	{
		if (m_TissueTreeWidget->GetCurrentHasChildren())
		{
			QMessageBox msg_box;
			msg_box.setText("Remove Folder");
			msg_box.setInformativeText("Do you want to keep all tissues\nand folders contained in this folder?");
			msg_box.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
			msg_box.setDefaultButton(QMessageBox::Yes);
			int ret = msg_box.exec();
			if (ret == QMessageBox::Yes)
			{
				m_TissueTreeWidget->RemoveItem(m_TissueTreeWidget->currentItem(), false);
			}
			else if (ret == QMessageBox::No)
			{
				Removeselected();
			}
		}
		else
		{
			m_TissueTreeWidget->RemoveItem(m_TissueTreeWidget->currentItem(), false);
		}
	}
	else
	{
		Removeselected();
	}
}

void MainWindow::Removeselected()
{
	const auto list = m_TissueTreeWidget->selectedItems();
	Removeselected(std::vector<QTreeWidgetItem*>(list.begin(), list.end()), true);
}

void MainWindow::Removeselected(const std::vector<QTreeWidgetItem*>& input, bool perform_checks)
{
	const auto list = m_TissueTreeWidget->Collect(input);
	std::vector<QTreeWidgetItem*> items_to_remove;
	std::set<tissues_size_t> tissues_to_remove;
	if (perform_checks)
	{
		// check if any tissue is locked
		for (const auto& item : list)
		{
			if (TissueInfos::GetTissueLocked(m_TissueTreeWidget->GetType(item)))
			{
				QMessageBox::warning(this, "iSeg", "Locked tissues can not be removed.", QMessageBox::Ok | QMessageBox::Default);
				return;
			}
		}

		// confirm that user really intends to remove tissues/delete segmentation
		int ret = QMessageBox::warning(this, "iSeg", "Do you really want to remove the selected tissues?", QMessageBox::Yes | QMessageBox::Default, QMessageBox::Cancel | QMessageBox::Escape);
		if (ret != QMessageBox::Yes)
		{
			return; // Cancel
		}

		// check if any tissues are in the hierarchy more than once
		auto all_items = m_TissueTreeWidget->GetAllItems();
		for (const auto& item : list)
		{
			items_to_remove.push_back(item);

			tissues_size_t curr_tissue_type = m_TissueTreeWidget->GetType(item);

			// if currTissueType==0 -> item is a folder
			bool remove_completely = curr_tissue_type != 0;
			if (remove_completely && m_TissueTreeWidget->GetTissueInstanceCount(curr_tissue_type) > 1)
			{
				// There are more than one instance of the same tissue in the tree
				int ret = QMessageBox::question(this, "iSeg", "There are multiple occurrences\nof a selected tissue in the "
																											"hierarchy.\nDo you want to remove them all?",
						QMessageBox::Yes | QMessageBox::Default, QMessageBox::No, QMessageBox::Cancel | QMessageBox::Escape);
				if (ret == QMessageBox::Yes)
				{
					remove_completely = true;
				}
				else if (ret == QMessageBox::No)
				{
					remove_completely = false;
				}
				else
				{
					return; // Cancel
				}
			}

			if (remove_completely)
			{
				tissues_to_remove.insert(curr_tissue_type);

				// find all tree items with that name and delete
				for (auto other : all_items)
				{
					if (other != item && m_TissueTreeWidget->GetType(other) == curr_tissue_type)
					{
						items_to_remove.push_back(other);
					}
				}
			}
		}
	}
	else
	{
		items_to_remove = list;
		for (const auto& item : list)
		{
			auto curr_tissue_type = m_TissueTreeWidget->GetType(item);
			if (curr_tissue_type != 0)
			{
				tissues_to_remove.insert(curr_tissue_type);
			}
		}
	}

	DataSelection data_selection;
	data_selection.allSlices = true;
	data_selection.tissues = true;
	emit BeginDatachange(data_selection, this, false);

	m_Handler3D->RemoveTissues(tissues_to_remove);
	m_TissueTreeWidget->RemoveItems(items_to_remove);

	emit EndDatachange(this, iseg::ClearUndo);

	if (TissueInfos::GetTissueCount() == 0)
	{
		TissueInfo tissue_info;
		tissue_info.m_Name = "Tissue1";
		tissue_info.m_Color = Color(1.0f, 0.0f, 0.1f);
		TissueInfos::AddTissue(tissue_info);

		// Insert new tissue in hierarchy
		m_TissueTreeWidget->InsertItem(false, ToQ(tissue_info.m_Name));
	}
	else
	{
		auto curr_tissue_type = m_TissueTreeWidget->GetCurrentType();
		TissuenrChanged(curr_tissue_type - 1);
	}
}

void MainWindow::RemoveTissueFolderAllPressed()
{
	// check if any tissue is locked
	for (auto item : m_TissueTreeWidget->GetAllItems())
	{
		if (TissueInfos::GetTissueLocked(m_TissueTreeWidget->GetType(item)))
		{
			QMessageBox::warning(this, "iSeg", "Locked tissues can not be removed.", QMessageBox::Ok | QMessageBox::Default);
			return;
		}
	}

	// confirm that user really intends to remove tissues/delete segmentation
	int ret = QMessageBox::warning(this, "iSeg", "Do you really want to remove all tissues and folders?", QMessageBox::Yes | QMessageBox::Default, QMessageBox::No, QMessageBox::Cancel | QMessageBox::Escape);
	if (ret == QMessageBox::Yes)
	{
		DataSelection data_selection;
		data_selection.allSlices = true;
		data_selection.tissues = true;
		emit BeginDatachange(data_selection, this, false);

		m_Handler3D->RemoveTissueall();
		m_TissueTreeWidget->RemoveAllFolders(false);
		m_TissueTreeWidget->UpdateTreeWidget();
		tissues_size_t curr_tissue_type = m_TissueTreeWidget->GetCurrentType();
		TissuenrChanged(curr_tissue_type - 1);

		emit EndDatachange(this, iseg::ClearUndo);
	}
}

// BL TODO replace by selectedtissue2work (SK)
void MainWindow::Tissue2work()
{
	DataSelection data_selection;
	data_selection.allSlices = m_Tissue3Dopt->isChecked();
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.work = true;
	emit BeginDatachange(data_selection, this);

	std::vector<tissues_size_t> tissue_list;
	tissue_list.push_back(m_TissueTreeWidget->GetCurrentType());

	if (m_Tissue3Dopt->isChecked())
	{
		m_Handler3D->Selectedtissue2work3D(tissue_list);
	}
	else
	{
		m_Handler3D->Selectedtissue2work(tissue_list);
	}

	emit EndDatachange(this);
}

void MainWindow::Selectedtissue2work()
{
	DataSelection data_selection;
	data_selection.allSlices = m_Tissue3Dopt->isChecked();
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.work = true;
	emit BeginDatachange(data_selection, this);

	m_Handler3D->ClearWork(); // resets work to 0.0f, then adds each tissue one-by-one

	std::vector<tissues_size_t> selected_tissues;
    const auto items = m_TissueTreeWidget->selectedItems();
	for (const auto& item : items)
	{
		selected_tissues.push_back(m_TissueTreeWidget->GetType(item));
	}

	try
	{
		if (m_Tissue3Dopt->isChecked())
		{
			m_Handler3D->Selectedtissue2work3D(selected_tissues);
		}
		else
		{
			m_Handler3D->Selectedtissue2work(selected_tissues);
		}
	}
	catch (std::exception&)
	{
		ISEG_ERROR_MSG("could not get tissue. Something might be wrong with tissue list.");
	}
	emit EndDatachange(this);
}

void MainWindow::Tissue2workall()
{
	DataSelection data_selection;
	data_selection.allSlices = m_Tissue3Dopt->isChecked();
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.work = true;
	emit BeginDatachange(data_selection, this);

	if (m_Tissue3Dopt->isChecked())
	{
		m_Handler3D->Tissue2workall3D();
	}
	else
	{
		m_Handler3D->Tissue2workall();
	}

	emit EndDatachange(this);
}

void MainWindow::Cleartissues()
{
	// check if any tissue is locked
	for (auto item : m_TissueTreeWidget->GetAllItems())
	{
		if (TissueInfos::GetTissueLocked(m_TissueTreeWidget->GetType(item)))
		{
			QMessageBox::warning(this, "iSeg", "Locked tissues can not be cleared.", QMessageBox::Ok | QMessageBox::Default);
			return;
		}
	}

	int ret = QMessageBox::warning(this, "iSeg", "Do you really want to clear all tissues?", QMessageBox::Yes | QMessageBox::Default, QMessageBox::No, QMessageBox::Cancel | QMessageBox::Escape);
	if (ret == QMessageBox::Yes)
	{
		DataSelection data_selection;
		data_selection.allSlices = m_Tissue3Dopt->isChecked();
		data_selection.sliceNr = m_Handler3D->ActiveSlice();
		data_selection.tissues = true;
		emit BeginDatachange(data_selection, this);

		if (m_Tissue3Dopt->isChecked())
		{
			m_Handler3D->Cleartissues3D();
		}
		else
		{
			m_Handler3D->Cleartissues();
		}

		emit EndDatachange(this);
	}
}

// BL TODO replace by clearselected (SK)
void MainWindow::Cleartissue()
{
	bool is_locked =
			TissueInfos::GetTissueLocked(m_TissueTreeWidget->GetCurrentType());
	if (is_locked)
	{
		QMessageBox::warning(this, "iSeg", "Locked tissue can not be removed.", QMessageBox::Ok | QMessageBox::Default);
		return;
	}

	int ret = QMessageBox::warning(this, "iSeg", "Do you really want to clear the tissue?", QMessageBox::Yes | QMessageBox::Default, QMessageBox::No, QMessageBox::Cancel | QMessageBox::Escape);
	if (ret == QMessageBox::Yes)
	{
		DataSelection data_selection;
		data_selection.allSlices = m_Tissue3Dopt->isChecked();
		data_selection.sliceNr = m_Handler3D->ActiveSlice();
		data_selection.tissues = true;
		emit BeginDatachange(data_selection, this);

		tissues_size_t nr = m_TissueTreeWidget->GetCurrentType();
		if (m_Tissue3Dopt->isChecked())
		{
			m_Handler3D->Cleartissue3D(nr);
		}
		else
		{
			m_Handler3D->Cleartissue(nr);
		}

		emit EndDatachange(this);
	}
}

void MainWindow::Clearselected()
{
	bool any_locked = false;
	const auto list = m_TissueTreeWidget->selectedItems();
	for (auto a = list.begin(); a != list.end() && !any_locked; ++a)
	{
		any_locked |= TissueInfos::GetTissueLocked(m_TissueTreeWidget->GetType(*a));
	}
	if (any_locked)
	{
		QMessageBox::warning(this, "iSeg", "Locked tissues can not be cleared.", QMessageBox::Ok | QMessageBox::Default);
		return;
	}

	int ret = QMessageBox::warning(this, "iSeg", "Do you really want to clear tissues?", QMessageBox::Yes | QMessageBox::Default, QMessageBox::No, QMessageBox::Cancel | QMessageBox::Escape);
	if (ret == QMessageBox::Yes)
	{
		DataSelection data_selection;
		data_selection.allSlices = m_Tissue3Dopt->isChecked();
		data_selection.sliceNr = m_Handler3D->ActiveSlice();
		data_selection.tissues = true;
		emit BeginDatachange(data_selection, this);

		const auto list = m_TissueTreeWidget->selectedItems();
		for (const auto& item : list)
		{
			tissues_size_t nr = m_TissueTreeWidget->GetType(item);
			if (m_Tissue3Dopt->isChecked())
			{
				m_Handler3D->Cleartissue3D(nr);
			}
			else
			{
				m_Handler3D->Cleartissue(nr);
			}
		}
		emit EndDatachange(this);
	}
}

void MainWindow::BmptissuevisibleChanged()
{
	if (m_CbBmptissuevisible->isChecked())
	{
		m_BmpShow->SetTissuevisible(true);
	}
	else
	{
		m_BmpShow->SetTissuevisible(false);
	}
}

void MainWindow::BmpoutlinevisibleChanged()
{
	if (m_CbBmpoutlinevisible->isChecked())
	{
		m_BmpShow->SetWorkbordervisible(true);
	}
	else
	{
		m_BmpShow->SetWorkbordervisible(false);
	}
}

void MainWindow::SetOutlineColor(QColor color)
{
	m_BmpShow->SetOutlineColor(color);
}

void MainWindow::WorktissuevisibleChanged()
{
	if (m_CbWorktissuevisible->isChecked())
	{
		m_WorkShow->SetTissuevisible(true);
	}
	else
	{
		m_WorkShow->SetTissuevisible(false);
	}
}

void MainWindow::WorkpicturevisibleChanged()
{
	if (m_CbWorkpicturevisible->isChecked())
	{
		m_WorkShow->SetPicturevisible(true);
	}
	else
	{
		m_WorkShow->SetPicturevisible(false);
	}
}

void MainWindow::SliceChanged()
{
	WidgetInterface* qw = (WidgetInterface*)m_MethodTab->currentWidget();
	qw->OnSlicenrChanged();

	unsigned short slicenr = m_Handler3D->ActiveSlice() + 1;
	QObject_disconnect(m_ScbSlicenr, SIGNAL(valueChanged(int)), this, SLOT(ScbSlicenrChanged()));
	m_ScbSlicenr->setValue(int(slicenr));
	QObject_connect(m_ScbSlicenr, SIGNAL(valueChanged(int)), this, SLOT(ScbSlicenrChanged()));
	QObject_disconnect(m_SbSlicenr, SIGNAL(valueChanged(int)), this, SLOT(SbSlicenrChanged()));
	m_SbSlicenr->setValue(int(slicenr));
	QObject_connect(m_SbSlicenr, SIGNAL(valueChanged(int)), this, SLOT(SbSlicenrChanged()));
	m_BmpShow->SlicenrChanged();
	m_WorkShow->SlicenrChanged();
	m_ScaleDialog->SlicenrChanged();
	m_ImagemathDialog->SlicenrChanged();
	m_ImageoverlayDialog->SlicenrChanged();
	m_OverlayWidget->SlicenrChanged();
	if (m_Handler3D->StartSlice() >= slicenr || m_Handler3D->EndSlice() + 1 <= slicenr)
	{
		m_LbInactivewarning->setText(QString("   3D Inactive Slice!   "));
	}
	else
	{
		m_LbInactivewarning->setText(QString(" "));
	}

	if (m_Xsliceshower != nullptr)
	{
		m_Xsliceshower->ZposChanged();
	}
	if (m_Ysliceshower != nullptr)
	{
		m_Ysliceshower->ZposChanged();
	}
}

void MainWindow::Slices3dChanged(bool new_bitstack)
{
	if (new_bitstack)
	{
		m_BitstackWidget->Newloaded();
	}
	m_OverlayWidget->Newloaded();
	m_ImageoverlayDialog->Newloaded();

	for (size_t i = 0; i < m_Tabwidgets.size(); i++)
	{
		((WidgetInterface*)(m_Tabwidgets[i]))->NewLoaded();
	}

	WidgetInterface* qw = (WidgetInterface*)m_MethodTab->currentWidget();

	if (m_Handler3D->NumSlices() != m_Nrslices)
	{
		QObject_disconnect(m_ScbSlicenr, SIGNAL(valueChanged(int)), this, SLOT(ScbSlicenrChanged()));
		m_ScbSlicenr->setMaxValue((int)m_Handler3D->NumSlices());
		QObject_connect(m_ScbSlicenr, SIGNAL(valueChanged(int)), this, SLOT(ScbSlicenrChanged()));
		QObject_disconnect(m_SbSlicenr, SIGNAL(valueChanged(int)), this, SLOT(SbSlicenrChanged()));
		m_SbSlicenr->setMaxValue((int)m_Handler3D->NumSlices());
		QObject_connect(m_SbSlicenr, SIGNAL(valueChanged(int)), this, SLOT(SbSlicenrChanged()));
		m_LbSlicenr->setText(QString(" of ") +
												 QString::number((int)m_Handler3D->NumSlices()));
		m_InterpolationWidget->Handler3DChanged();
		m_Nrslices = m_Handler3D->NumSlices();
	}

	unsigned short slicenr = m_Handler3D->ActiveSlice() + 1;
	QObject_disconnect(m_ScbSlicenr, SIGNAL(valueChanged(int)), this, SLOT(ScbSlicenrChanged()));
	m_ScbSlicenr->setValue(int(slicenr));
	QObject_connect(m_ScbSlicenr, SIGNAL(valueChanged(int)), this, SLOT(ScbSlicenrChanged()));
	QObject_disconnect(m_SbSlicenr, SIGNAL(valueChanged(int)), this, SLOT(SbSlicenrChanged()));
	m_SbSlicenr->setValue(int(slicenr));
	QObject_connect(m_SbSlicenr, SIGNAL(valueChanged(int)), this, SLOT(SbSlicenrChanged()));
	if (m_Handler3D->StartSlice() >= slicenr ||
			m_Handler3D->EndSlice() + 1 <= slicenr)
	{
		m_LbInactivewarning->setText(QString("   3D Inactive Slice!   "));
	}
	else
	{
		m_LbInactivewarning->setText(QString(" "));
	}

	if (m_Xsliceshower != nullptr)
		m_Xsliceshower->ZposChanged();
	if (m_Ysliceshower != nullptr)
		m_Ysliceshower->ZposChanged();

	qw->Init();
}

void MainWindow::ZoomIn()
{
	//	bmp_show->SetZoom(bmp_show->return_zoom()*2);
	//	work_show->SetZoom(work_show->return_zoom()*2);
	m_ZoomWidget->ZoomChanged(m_WorkShow->ReturnZoom() * 2);
}

void MainWindow::ZoomOut()
{
	//	bmp_show->SetZoom(bmp_show->return_zoom()/2);
	//	work_show->SetZoom(work_show->return_zoom()/2);
	//	zoom_widget->SetZoom(work_show->return_zoom());
	m_ZoomWidget->ZoomChanged(m_WorkShow->ReturnZoom() / 2);
}

void MainWindow::SlicenrUp()
{
	auto n = std::min(m_Handler3D->ActiveSlice() + m_SbStride->value(), m_Handler3D->NumSlices() - 1);
	m_Handler3D->SetActiveSlice(n);
	SliceChanged();
}

void MainWindow::SlicenrDown()
{
	auto n = std::max(static_cast<int>(m_Handler3D->ActiveSlice()) - m_SbStride->value(), 0);
	m_Handler3D->SetActiveSlice(n);
	SliceChanged();
}

void MainWindow::SbSlicenrChanged()
{
	m_Handler3D->SetActiveSlice(m_SbSlicenr->value() - 1);
	SliceChanged();
}

void MainWindow::ScbSlicenrChanged()
{
	m_Handler3D->SetActiveSlice(m_ScbSlicenr->value() - 1);
	SliceChanged();
}

void MainWindow::SbStrideChanged()
{
	m_SbSlicenr->setSingleStep(m_SbStride->value());
	m_ScbSlicenr->setSingleStep(m_SbStride->value());
}

void MainWindow::PbFirstPressed()
{
	m_Handler3D->SetActiveSlice(0);
	SliceChanged();
}

void MainWindow::PbLastPressed()
{
	m_Handler3D->SetActiveSlice(m_Handler3D->NumSlices() - 1);
	SliceChanged();
}

void MainWindow::SlicethicknessChanged()
{
	float thickness = m_Handler3D->GetSlicethickness();
	if (m_Xsliceshower != nullptr)
		m_Xsliceshower->ThicknessChanged(thickness);
	if (m_Ysliceshower != nullptr)
		m_Ysliceshower->ThicknessChanged(thickness);
	if (m_VV3D != nullptr)
		m_VV3D->ThicknessChanged(thickness);
	if (m_VV3Dbmp != nullptr)
		m_VV3Dbmp->ThicknessChanged(thickness);
	if (m_SurfaceViewer != nullptr)
		m_SurfaceViewer->ThicknessChanged(thickness);
}

void MainWindow::TissuenrChanged(int i)
{
	QWidget* qw = m_MethodTab->currentWidget();
	if (auto tool = dynamic_cast<WidgetInterface*>(qw))
	{
		tool->OnTissuenrChanged(i);
	}

	m_BmpShow->ColorChanged(i);
	m_WorkShow->ColorChanged(i);
	m_BitstackWidget->TissuenrChanged(i);

	m_CbTissuelock->setChecked(TissueInfos::GetTissueLocked(i + 1));
}

void MainWindow::TissueSelectionChanged()
{
	const auto list = m_TissueTreeWidget->selectedItems();
	if (list.size() > 1)
	{
		//showpb_tab[6] = false;
		m_CbBmpoutlinevisible->setChecked(false);
		BmpoutlinevisibleChanged();
		UpdateTabvisibility();
	}
	else
	{
		//showpb_tab[6] = true;
		//showpb_tab[14] = true;
		UpdateTabvisibility();
	}

	m_TissuesDock->setWindowTitle(QString("Tissues (%1)").arg(list.size()));

	if (!list.empty())
	{
		TissuenrChanged((int)m_TissueTreeWidget->GetCurrentType() - 1);
	}

	std::set<tissues_size_t> sel;
	for (const auto& item : list)
	{
		sel.insert(m_TissueTreeWidget->GetType(item));
	}
	TissueInfos::SetSelectedTissues(sel);
}

void MainWindow::Unselectall()
{
	const auto list = m_TissueTreeWidget->selectedItems();
	for (auto& item : list)
	{
		item->setSelected(false);
	}
}

void MainWindow::TreeWidgetDoubleclicked(QTreeWidgetItem* item, int column)
{
	ModifTissueFolderPressed();
}

void MainWindow::TreeWidgetContextmenu(const QPoint& pos)
{
	const auto list = m_TissueTreeWidget->selectedItems();

	QMenu context_menu("tissuetreemenu", m_TissueTreeWidget);
	if (list.size() <= 1) // single selection
	{
		if (m_TissueTreeWidget->GetCurrentIsFolder())
		{
			context_menu.addAction("Toggle Lock", m_CbTissuelock, SLOT(click()));
			context_menu.addSeparator();
			context_menu.addAction("New Tissue...", this, SLOT(NewTissuePressed()));
			context_menu.addAction("New Folder...", this, SLOT(NewFolderPressed()));
			context_menu.addAction("Mod. Folder...", this, SLOT(ModifTissueFolderPressed()));
			context_menu.addAction("Del. Folder...", this, SLOT(RemoveTissueFolderPressed()));
		}
		else
		{
			context_menu.addAction("Toggle Lock", m_CbTissuelock, SLOT(click()));
			context_menu.addSeparator();
			context_menu.addAction("New Tissue...", this, SLOT(NewTissuePressed()));
			context_menu.addAction("New Folder...", this, SLOT(NewFolderPressed()));
			context_menu.addAction("Mod. Tissue...", this, SLOT(ModifTissueFolderPressed()));
			context_menu.addAction("Del. Tissue...", this, SLOT(RemoveTissueFolderPressed()));
			context_menu.addSeparator();
			context_menu.addAction("Get Tissue", this, SLOT(Tissue2work()));
			context_menu.addAction("Clear Tissue", this, SLOT(Cleartissue()));
			context_menu.addSeparator();
			context_menu.addAction("Next Feat. Slice", this, SLOT(NextFeaturingSlice()));
		}
	}
	else // multi-selection
	{
		context_menu.addAction("Toggle Lock", m_CbTissuelock, SLOT(click()));
		context_menu.addSeparator();
		context_menu.addAction("Delete Selected", this, SLOT(Removeselected()));
		context_menu.addAction("Clear Selected", this, SLOT(Clearselected()));
		context_menu.addAction("Get Selected", this, SLOT(Selectedtissue2work()));
		context_menu.addAction("Merge", this, SLOT(Merge()));
	}

	if (!list.empty())
	{
		context_menu.addAction("Unselect All", this, SLOT(Unselectall()));
		context_menu.addAction("Assign Random Colors", this, SLOT(RandomizeColors()));
		context_menu.addAction("View Tissue Surface", this, SLOT(ExecuteSelectedtissueSurfaceviewer()));
	}
	context_menu.addSeparator();
	auto show_ind = context_menu.addAction("Show Tissue Indices", m_TissueTreeWidget, SLOT(ToggleShowTissueIndices()));
	show_ind->setChecked(!m_TissueTreeWidget->GetTissueIndicesHidden());
	// BL-ACTION context_menu.setItemChecked(item_id, !m_TissueTreeWidget->GetTissueIndicesHidden());
	context_menu.addAction("Sort By Name", m_TissueTreeWidget, SLOT(SortByTissueName()));
	context_menu.addAction("Sort By Index", m_TissueTreeWidget, SLOT(SortByTissueIndex()));
	context_menu.exec(m_TissueTreeWidget->viewport()->mapToGlobal(pos));
}

void MainWindow::TissuelockToggled()
{
	if (m_TissueTreeWidget->GetCurrentIsFolder())
	{
		// Set lock state of all child tissues
		std::map<tissues_size_t, unsigned short> child_tissues;
		m_TissueTreeWidget->GetCurrentChildTissues(child_tissues);
		for (std::map<tissues_size_t, unsigned short>::iterator iter =
						 child_tissues.begin();
				 iter != child_tissues.end(); ++iter)
		{
			TissueInfos::SetTissueLocked(iter->first, m_CbTissuelock->isChecked());
		}
	}
	else
	{
		const auto list = m_TissueTreeWidget->selectedItems();
		for (const auto& item : list)
		{
			tissues_size_t curr_tissue_type = m_TissueTreeWidget->GetType(item);
			TissueInfos::SetTissueLocked(curr_tissue_type, m_CbTissuelock->isChecked());
		}
	}
	m_TissueTreeWidget->UpdateTissueIcons();
	m_TissueTreeWidget->UpdateFolderIcons();

	if (!m_CbTissuelock->isChecked())
		m_LockTissues->setChecked(false);
}

void MainWindow::ExecuteUndo()
{
	if (m_Handler3D->ReturnNrundo() > 0)
	{
		DataSelection selected_data;
		if (m_MethodTab->currentWidget() == m_TransformWidget)
		{
			CancelTransformHelper();
			selected_data = m_Handler3D->Undo();
			m_TransformWidget->Init();
		}
		else
		{
			selected_data = m_Handler3D->Undo();
		}

		// Update ranges
		UpdateRangesHelper();

		//	if(undotype & )
		SliceChanged();

		if (selected_data.DataSelected())
		{
			m_Redonr->setEnabled(true);
			m_Undonr->setEnabled(m_Handler3D->ReturnNrundo() > 0);
		}
	}
}

void MainWindow::ExecuteRedo()
{
	DataSelection selected_data;
	if (m_MethodTab->currentWidget() == m_TransformWidget)
	{
		CancelTransformHelper();
		selected_data = m_Handler3D->Redo();
		m_TransformWidget->Init();
	}
	else
	{
		selected_data = m_Handler3D->Redo();
	}

	// Update ranges
	UpdateRangesHelper();

	//	if(undotype & )
	SliceChanged();

	if (selected_data.DataSelected())
	{
		m_Undonr->setEnabled(true);
		m_Redonr->setEnabled(m_Handler3D->ReturnNrredo() > 0);
	}
}

void MainWindow::ClearStack() { m_BitstackWidget->ClearStack(); }

void MainWindow::DoUndostepdone()
{
	m_Redonr->setEnabled(false);
	m_Undonr->setEnabled(m_Handler3D->ReturnNrundo() > 0);
}

void MainWindow::DoClearundo()
{
	m_Handler3D->ClearUndo();

	m_Redonr->setEnabled(false);
	m_Undonr->setEnabled(false);
}

void MainWindow::TabChanged(int idx)
{
	QWidget* qw = m_MethodTab->widget(idx);
	if (qw != m_TabOld)
	{
		ISEG_INFO("Starting widget: " << qw->metaObject()->className());

		// disconnect signal-slots of previous widget

		if (m_TabOld) // generic slots from WidgetInterface
		{
			m_TabOld->Cleanup();

			// always connect to bmp_show, but not always to work_show
			QObject_disconnect(m_TabOld, SIGNAL(VmChanged(std::vector<Mark>*)), m_BmpShow, SLOT(SetVm(std::vector<Mark>*)));
			QObject_disconnect(m_TabOld, SIGNAL(VpdynChanged(std::vector<Point>*)), m_BmpShow, SLOT(SetVpdyn(std::vector<Point>*)));

			QObject_disconnect(this, SIGNAL(BmpChanged()), m_TabOld, SLOT(BmpChanged()));
			QObject_disconnect(this, SIGNAL(WorkChanged()), m_TabOld, SLOT(WorkChanged()));
			QObject_disconnect(this, SIGNAL(TissuesChanged()), m_TabOld, SLOT(TissuesChanged()));
			QObject_disconnect(this, SIGNAL(MarksChanged()), m_TabOld, SLOT(MarksChanged()));

			QObject_disconnect(m_BmpShow, SIGNAL(MousepressedSign(Point)), m_TabOld, SLOT(MouseClicked(Point)));
			QObject_disconnect(m_BmpShow, SIGNAL(MousemovedSign(Point)), m_TabOld, SLOT(MouseMoved(Point)));
			QObject_disconnect(m_BmpShow, SIGNAL(MousereleasedSign(Point)), m_TabOld, SLOT(MouseReleased(Point)));
			QObject_disconnect(m_WorkShow, SIGNAL(MousepressedSign(Point)), m_TabOld, SLOT(MouseClicked(Point)));
			QObject_disconnect(m_WorkShow, SIGNAL(MousemovedSign(Point)), m_TabOld, SLOT(MouseMoved(Point)));
			QObject_disconnect(m_WorkShow, SIGNAL(MousereleasedSign(Point)), m_TabOld, SLOT(MouseReleased(Point)));
		}

		if (m_TabOld == m_MeasurementWidget)
		{
			QObject_disconnect(m_MeasurementWidget, SIGNAL(Vp1Changed(std::vector<Point>*)), m_BmpShow, SLOT(SetVp1(std::vector<Point>*)));
			QObject_disconnect(m_MeasurementWidget, SIGNAL(Vp1Changed(std::vector<Point>*)), m_WorkShow, SLOT(SetVp1(std::vector<Point>*)));
			QObject_disconnect(m_MeasurementWidget, SIGNAL(VpdynChanged(std::vector<Point>*)), m_WorkShow, SLOT(SetVpdyn(std::vector<Point>*)));
			QObject_disconnect(m_MeasurementWidget, SIGNAL(Vp1dynChanged(std::vector<Point>*,std::vector<Point>*,bool)), m_BmpShow, SLOT(SetVp1Dyn(std::vector<Point>*,std::vector<Point>*,bool)));
			QObject_disconnect(m_MeasurementWidget, SIGNAL(Vp1dynChanged(std::vector<Point>*,std::vector<Point>*,bool)), m_WorkShow, SLOT(SetVp1Dyn(std::vector<Point>*,std::vector<Point>*,bool)));

			m_WorkShow->setMouseTracking(false);
			m_BmpShow->setMouseTracking(false);
		}
		else if (m_TabOld == m_VesselextrWidget)
		{
			QObject_disconnect(m_VesselextrWidget, SIGNAL(Vp1Changed(std::vector<Point>*)), m_BmpShow, SLOT(SetVp1(std::vector<Point>*)));
		}
		else if (m_TabOld == m_HystWidget)
		{
			QObject_disconnect(m_HystWidget, SIGNAL(Vp1Changed(std::vector<Point>*)), m_BmpShow, SLOT(SetVp1(std::vector<Point>*)));
			QObject_disconnect(m_HystWidget, SIGNAL(Vp1dynChanged(std::vector<Point>*,std::vector<Point>*)), m_BmpShow, SLOT(SetVp1Dyn(std::vector<Point>*,std::vector<Point>*)));
		}
		else if (m_TabOld == m_LivewireWidget)
		{
			QObject_disconnect(m_BmpShow, SIGNAL(MousedoubleclickSign(Point)), m_LivewireWidget, SLOT(PtDoubleclicked(Point)));
			QObject_disconnect(m_BmpShow, SIGNAL(MousepressedmidSign(Point)), m_LivewireWidget, SLOT(PtMidclicked(Point)));
			QObject_disconnect(m_BmpShow, SIGNAL(MousedoubleclickmidSign(Point)), m_LivewireWidget, SLOT(PtDoubleclickedmid(Point)));
			QObject_disconnect(m_LivewireWidget, SIGNAL(Vp1Changed(std::vector<Point>*)), m_BmpShow, SLOT(SetVp1(std::vector<Point>*)));
			QObject_disconnect(m_LivewireWidget, SIGNAL(Vp1dynChanged(std::vector<Point>*,std::vector<Point>*)), m_BmpShow, SLOT(SetVp1Dyn(std::vector<Point>*,std::vector<Point>*)));

			m_BmpShow->setMouseTracking(false);
		}
		else if (m_TabOld == m_OlcWidget)
		{
			QObject_disconnect(m_OlcWidget, SIGNAL(VpdynChanged(std::vector<Point>*)), m_WorkShow, SLOT(SetVpdyn(std::vector<Point>*)));
		}
		else if (m_TabOld == m_FeatureWidget)
		{
			QObject_disconnect(m_FeatureWidget, SIGNAL(VpdynChanged(std::vector<Point>*)), m_WorkShow, SLOT(SetVpdyn(std::vector<Point>*)));

			m_WorkShow->setMouseTracking(false);
			m_BmpShow->setMouseTracking(false);
		}
		else if (m_TabOld == m_PickerWidget)
		{
			QObject_disconnect(m_PickerWidget, SIGNAL(Vp1Changed(std::vector<Point>*)), m_BmpShow, SLOT(SetVp1(std::vector<Point>*)));
			QObject_disconnect(m_PickerWidget, SIGNAL(Vp1Changed(std::vector<Point>*)), m_WorkShow, SLOT(SetVp1(std::vector<Point>*)));
		}

		// connect signal-slots new widget

		if (auto widget = dynamic_cast<WidgetInterface*>(qw)) // generic slots from WidgetInterface
		{
			QObject_connect(widget, SIGNAL(VmChanged(std::vector<Mark>*)), m_BmpShow, SLOT(SetVm(std::vector<Mark>*)));
			QObject_connect(widget, SIGNAL(VpdynChanged(std::vector<Point>*)), m_BmpShow, SLOT(SetVpdyn(std::vector<Point>*)));

			QObject_connect(this, SIGNAL(BmpChanged()), widget, SLOT(BmpChanged()));
			QObject_connect(this, SIGNAL(WorkChanged()), widget, SLOT(WorkChanged()));
			QObject_connect(this, SIGNAL(TissuesChanged()), widget, SLOT(TissuesChanged()));
			QObject_connect(this, SIGNAL(MarksChanged()), widget, SLOT(MarksChanged()));

			QObject_connect(m_BmpShow, SIGNAL(MousepressedSign(Point)), widget, SLOT(MouseClicked(Point)));
			QObject_connect(m_BmpShow, SIGNAL(MousemovedSign(Point)), widget, SLOT(MouseMoved(Point)));
			QObject_connect(m_BmpShow, SIGNAL(MousereleasedSign(Point)), widget, SLOT(MouseReleased(Point)));
			QObject_connect(m_WorkShow, SIGNAL(MousepressedSign(Point)), widget, SLOT(MouseClicked(Point)));
			QObject_connect(m_WorkShow, SIGNAL(MousemovedSign(Point)), widget, SLOT(MouseMoved(Point)));
			QObject_connect(m_WorkShow, SIGNAL(MousereleasedSign(Point)), widget, SLOT(MouseReleased(Point)));
		}

		if (qw == m_MeasurementWidget)
		{
			QObject_connect(m_MeasurementWidget, SIGNAL(Vp1Changed(std::vector<Point>*)), m_BmpShow, SLOT(SetVp1(std::vector<Point>*)));
			QObject_connect(m_MeasurementWidget, SIGNAL(Vp1Changed(std::vector<Point>*)), m_WorkShow, SLOT(SetVp1(std::vector<Point>*)));
			QObject_connect(m_MeasurementWidget, SIGNAL(VpdynChanged(std::vector<Point>*)), m_WorkShow, SLOT(SetVpdyn(std::vector<Point>*)));
			QObject_connect(m_MeasurementWidget, SIGNAL(Vp1dynChanged(std::vector<Point>*,std::vector<Point>*,bool)), m_BmpShow, SLOT(SetVp1Dyn(std::vector<Point>*,std::vector<Point>*,bool)));
			QObject_connect(m_MeasurementWidget, SIGNAL(Vp1dynChanged(std::vector<Point>*,std::vector<Point>*,bool)), m_WorkShow, SLOT(SetVp1Dyn(std::vector<Point>*,std::vector<Point>*,bool)));

			m_WorkShow->setMouseTracking(true);
			m_BmpShow->setMouseTracking(true);
		}
		else if (qw == m_VesselextrWidget)
		{
			QObject_connect(m_VesselextrWidget, SIGNAL(Vp1Changed(std::vector<Point>*)), m_BmpShow, SLOT(SetVp1(std::vector<Point>*)));
		}
		else if (qw == m_HystWidget)
		{
			QObject_connect(m_HystWidget, SIGNAL(Vp1Changed(std::vector<Point>*)), m_BmpShow, SLOT(SetVp1(std::vector<Point>*)));
			QObject_connect(m_HystWidget, SIGNAL(Vp1dynChanged(std::vector<Point>*,std::vector<Point>*)), m_BmpShow, SLOT(SetVp1Dyn(std::vector<Point>*,std::vector<Point>*)));
		}
		else if (qw == m_LivewireWidget)
		{
			QObject_connect(m_BmpShow, SIGNAL(MousedoubleclickSign(Point)), m_LivewireWidget, SLOT(PtDoubleclicked(Point)));
			QObject_connect(m_BmpShow, SIGNAL(MousepressedmidSign(Point)), m_LivewireWidget, SLOT(PtMidclicked(Point)));
			QObject_connect(m_BmpShow, SIGNAL(MousedoubleclickmidSign(Point)), m_LivewireWidget, SLOT(PtDoubleclickedmid(Point)));
			QObject_connect(m_LivewireWidget, SIGNAL(Vp1Changed(std::vector<Point>*)), m_BmpShow, SLOT(SetVp1(std::vector<Point>*)));
			QObject_connect(m_LivewireWidget, SIGNAL(Vp1dynChanged(std::vector<Point>*,std::vector<Point>*)), m_BmpShow, SLOT(SetVp1Dyn(std::vector<Point>*,std::vector<Point>*)));

			m_BmpShow->setMouseTracking(true);
		}
		else if (qw == m_OlcWidget)
		{
			QObject_connect(m_OlcWidget, SIGNAL(VpdynChanged(std::vector<Point>*)), m_WorkShow, SLOT(SetVpdyn(std::vector<Point>*)));
		}
		else if (qw == m_FeatureWidget)
		{
			QObject_connect(m_FeatureWidget, SIGNAL(VpdynChanged(std::vector<Point>*)), m_WorkShow, SLOT(SetVpdyn(std::vector<Point>*)));

			m_WorkShow->setMouseTracking(true);
			m_BmpShow->setMouseTracking(true);
		}
		else if (qw == m_PickerWidget)
		{
			QObject_connect(m_PickerWidget, SIGNAL(Vp1Changed(std::vector<Point>*)), m_BmpShow, SLOT(SetVp1(std::vector<Point>*)));
			QObject_connect(m_PickerWidget, SIGNAL(Vp1Changed(std::vector<Point>*)), m_WorkShow, SLOT(SetVp1(std::vector<Point>*)));
		}

		m_TabOld = static_cast<WidgetInterface*>(qw);
		m_TabOld->Init();
		m_TabOld->OnTissuenrChanged(m_TissueTreeWidget->GetCurrentType() - 1); // \todo BL: is this correct for all widgets?

		m_BmpShow->setCursor(*m_TabOld->GetCursor());
		m_WorkShow->setCursor(*m_TabOld->GetCursor());

		UpdateMethodButtonsPressed(m_TabOld);
	}
	else if (qw)
	{
		m_TabOld = static_cast<WidgetInterface*>(qw);
	}

	m_TabOld->setFocus();
}

void MainWindow::UpdateTabvisibility()
{
	auto nrtabbuttons = (unsigned short)m_Tabwidgets.size();
	unsigned short counter = 0;
	for (unsigned short i = 0; i < nrtabbuttons; i++)
	{
		if (m_ShowpbTab[i])
			counter++;
	}
	unsigned short counter1 = 0;
	for (unsigned short i = 0; i < nrtabbuttons; i++)
	{
		if (m_ShowpbTab[i])
		{
			m_PbTab[counter1]->setIconSet(m_Tabwidgets[i]->GetIcon(m_MPicpath));
			m_PbTab[counter1]->setText(m_Tabwidgets[i]->GetName().c_str());
			m_PbTab[counter1]->setToolTip(m_Tabwidgets[i]->toolTip());
			m_PbTab[counter1]->show();
			counter1++;
			if (counter1 == (counter + 1) / 2)
			{
				while (counter1 < (nrtabbuttons + 1) / 2)
				{
					m_PbTab[counter1]->hide();
					counter1++;
				}
			}
		}
	}

	while (counter1 < nrtabbuttons)
	{
		m_PbTab[counter1]->hide();
		counter1++;
	}

	WidgetInterface* qw = static_cast<WidgetInterface*>(m_MethodTab->currentWidget());
	UpdateMethodButtonsPressed(qw);
}

void MainWindow::UpdateMethodButtonsPressed(WidgetInterface* qw)
{
	auto nrtabbuttons = (unsigned short)m_Tabwidgets.size();
	unsigned short counter = 0;
	unsigned short pos = nrtabbuttons;
	for (unsigned short i = 0; i < nrtabbuttons; i++)
	{
		if (m_ShowpbTab[i])
		{
			if (m_Tabwidgets[i] == qw)
				pos = counter;
			counter++;
		}
	}

	unsigned short counter1 = 0;
	for (unsigned short i = 0; i < (counter + 1) / 2; i++, counter1++)
	{
		if (counter1 == pos)
			m_PbTab[i]->setChecked(true);
		else
			m_PbTab[i]->setChecked(false);
	}

	for (unsigned short i = (nrtabbuttons + 1) / 2; counter1 < counter;
			 i++, counter1++)
	{
		if (counter1 == pos)
			m_PbTab[i]->setChecked(true);
		else
			m_PbTab[i]->setChecked(false);
	}
}

void MainWindow::LoadAtlas(const QDir& path1)
{
	m_MAtlasfilename.m_MAtlasdir = path1;
	QStringList filters;
	filters << "*.atl";
	QStringList names1 = path1.entryList(filters);
	m_MAtlasfilename.m_Nratlases = (int)names1.size();
	if (m_MAtlasfilename.m_Nratlases > m_MAtlasfilename.maxnr)
		m_MAtlasfilename.m_Nratlases = m_MAtlasfilename.maxnr;
	for (int i = 0; i < m_MAtlasfilename.m_Nratlases; i++)
	{
		m_MAtlasfilename.m_MAtlasfilename[i] = names1[i];
		QFileInfo names1fi(names1[i]);
		m_MAtlasfilename.m_Atlasnr[i]->setText(names1fi.completeBaseName());
		m_MAtlasfilename.m_Atlasnr[i]->setVisible(true);
	}
	for (int i = m_MAtlasfilename.m_Nratlases; i < m_MAtlasfilename.maxnr; i++)
	{
		m_MAtlasfilename.m_Atlasnr[i]->setVisible(false);
	}

	if (!names1.empty())
	{
		m_MAtlasfilename.m_Separator->setVisible(true);
	}
}

void MainWindow::LoadLoadProj(const QString& path1)
{
	unsigned short projcounter = 0;
	FILE* fplatestproj = fopen(path1.toAscii(), "r");
	char c;
	m_MLoadprojfilename.m_CurrentFilename = path1;
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
	if (fplatestproj != nullptr)
		fclose(fplatestproj);
}

void MainWindow::AddLoadProj(const QString& path1)
{
	m_MLoadprojfilename.InsertProjectFileName(path1);
	
	for (int i = 0; i < 4; ++i)
	{
		if (m_MLoadprojfilename.m_RecentProjectFileNames[i] != "")
		{
			const int pos = m_MLoadprojfilename.m_RecentProjectFileNames[i].findRev('/', -2);
			if (pos != -1 && (int)m_MLoadprojfilename.m_RecentProjectFileNames[i].length() > pos + 1)
			{
				QString recent_filename = m_MLoadprojfilename.m_RecentProjectFileNames[i].right(m_MLoadprojfilename.m_RecentProjectFileNames[i].length() - pos - 1);
				m_MLoadprojfilename.m_LoadRecentProjects[i]->setText(recent_filename);
				m_MLoadprojfilename.m_LoadRecentProjects[i]->setVisible(true);
			}
		}
	}

	m_MLoadprojfilename.m_Separator->setVisible(true);
}

void MainWindow::SaveLoadProj(const QString& latestprojpath) const
{
	if (latestprojpath == "")
		return;
	FILE* fplatestproj = fopen(latestprojpath.toAscii(), "w");
	if (fplatestproj != nullptr)
	{
		if (m_MLoadprojfilename.m_RecentProjectFileNames[3] != "")
		{
			fprintf(fplatestproj, "%s\n", m_MLoadprojfilename.m_RecentProjectFileNames[3].toStdString().c_str());
		}
		if (m_MLoadprojfilename.m_RecentProjectFileNames[2] != "")
		{
			fprintf(fplatestproj, "%s\n", m_MLoadprojfilename.m_RecentProjectFileNames[2].toStdString().c_str());
		}
		if (m_MLoadprojfilename.m_RecentProjectFileNames[1] != "")
		{
			fprintf(fplatestproj, "%s\n", m_MLoadprojfilename.m_RecentProjectFileNames[1].toStdString().c_str());
		}
		if (m_MLoadprojfilename.m_RecentProjectFileNames[0] != "")
		{
			fprintf(fplatestproj, "%s\n", m_MLoadprojfilename.m_RecentProjectFileNames[0].toStdString().c_str());
		}

		fclose(fplatestproj);
	}
}

void MainWindow::PbTabPressed(int nr)
{
	auto nrtabbuttons = (unsigned short)m_Tabwidgets.size();
	unsigned short tabnr = nr + 1;
	for (unsigned short tabnr1 = 0; tabnr1 < m_PbTab.size(); tabnr1++)
	{
		m_PbTab[tabnr1]->setChecked(tabnr == tabnr1 + 1);
	}
	unsigned short pos1 = 0;
	for (unsigned short i = 0; i < nrtabbuttons; i++)
		if (m_ShowpbTab[i])
			pos1++;
	if ((tabnr > (nrtabbuttons + 1) / 2))
		tabnr = tabnr + (pos1 + 1) / 2 - (nrtabbuttons + 1) / 2;
	if (tabnr > pos1)
		return;
	pos1 = 0;
	unsigned short pos2 = 0;
	while (pos1 < tabnr)
	{
		if (m_ShowpbTab[pos2])
			pos1++;
		pos2++;
	}
	pos2--;
	m_MethodTab->setCurrentWidget(m_Tabwidgets[pos2]);
}

void MainWindow::BmpcrosshairvisibleChanged()
{
	if (m_CbBmpcrosshairvisible->isChecked())
	{
		if (m_Xsliceshower != nullptr)
		{
			m_BmpShow->CrosshairyChanged(m_Xsliceshower->GetSlicenr());
			m_BmpShow->SetCrosshairyvisible(true);
		}
		if (m_Ysliceshower != nullptr)
		{
			m_BmpShow->CrosshairxChanged(m_Ysliceshower->GetSlicenr());
			m_BmpShow->SetCrosshairxvisible(true);
		}
	}
	else
	{
		m_BmpShow->SetCrosshairxvisible(false);
		m_BmpShow->SetCrosshairyvisible(false);
	}
}

void MainWindow::WorkcrosshairvisibleChanged()
{
	if (m_CbWorkcrosshairvisible->isChecked())
	{
		if (m_Xsliceshower != nullptr)
		{
			m_WorkShow->CrosshairyChanged(m_Xsliceshower->GetSlicenr());
			m_WorkShow->SetCrosshairyvisible(true);
		}
		if (m_Ysliceshower != nullptr)
		{
			m_WorkShow->CrosshairxChanged(m_Ysliceshower->GetSlicenr());
			m_WorkShow->SetCrosshairxvisible(true);
		}
	}
	else
	{
		m_WorkShow->SetCrosshairxvisible(false);
		m_WorkShow->SetCrosshairyvisible(false);
	}
}

void MainWindow::ExecuteInversesliceorder()
{
	DataSelection data_selection;
	data_selection.allSlices = true;
	data_selection.bmp = true;
	data_selection.work = true;
	data_selection.tissues = true;
	emit BeginDatachange(data_selection, this, false);

	m_Handler3D->Inversesliceorder();

	if (QMessageBox::question(this, "iSeg", "Would you like the first slice to remain in same position in space?", QMessageBox::Yes, QMessageBox::No | QMessageBox::Default) == QMessageBox::Yes)
	{
		auto tr = m_Handler3D->ImageTransform();
		int shift[3] = {0, 0, -(m_Handler3D->NumSlices() - 1)};
		tr.PaddingUpdateTransform(shift, m_Handler3D->Spacing());
		m_Handler3D->SetTransform(tr);
	}

	emit EndDatachange(this, iseg::NoUndo);
}

void MainWindow::ExecuteSmoothsteps()
{
	DataSelection data_selection;
	data_selection.allSlices = true;
	data_selection.tissues = true;
	emit BeginDatachange(data_selection, this);

	m_Handler3D->StepsmoothZ(5);

	emit EndDatachange(this);
}

void MainWindow::ExecuteSmoothtissues()
{
	DataSelection data_selection;
	data_selection.allSlices = true;
	data_selection.tissues = true;
	emit BeginDatachange(data_selection, this);

	m_Handler3D->SmoothTissues(3);

	emit EndDatachange(this);
}
void MainWindow::ExecuteRemoveUnusedTissues()
{
	// get all unused tissue ids
	auto unused = m_Handler3D->FindUnusedTissues();

	if (unused.empty())
	{
		QMessageBox::information(this, "iSeg", "No unused tissues found");
		return;
	}
	else
	{
		int ret = QMessageBox::warning(this, "iSeg", "Found " + QString::number(unused.size()) + " unused tissues. Do you want to remove them?", QMessageBox::Yes | QMessageBox::Default, QMessageBox::Cancel | QMessageBox::Escape);
		if (ret != QMessageBox::Yes)
		{
			return;
		}
	}

	auto all = m_TissueTreeWidget->GetAllItems(true);

	// collect tree items matching the list of tissue ids
	std::vector<QTreeWidgetItem*> list;
	for (auto item : all)
	{
		auto type = m_TissueTreeWidget->GetType(item);
		if (std::find(unused.begin(), unused.end(), type) != unused.end())
		{
			list.push_back(item);
		}
	}
	Removeselected(list, false);
}

void MainWindow::ExecuteCleanup()
{
	tissues_size_t** slices = new tissues_size_t*[m_Handler3D->EndSlice() - m_Handler3D->StartSlice()];
	tissuelayers_size_t activelayer = m_Handler3D->ActiveTissuelayer();
	for (unsigned short i = m_Handler3D->StartSlice(); i < m_Handler3D->EndSlice(); i++)
	{
		slices[i - m_Handler3D->StartSlice()] = m_Handler3D->ReturnTissues(activelayer, i);
	}
	TissueCleaner tc(slices, m_Handler3D->EndSlice() - m_Handler3D->StartSlice(), m_Handler3D->Width(), m_Handler3D->Height());
	if (!tc.Allocate())
	{
		QMessageBox::information(this, "iSeg", "Not enough memory.\nThis operation cannot be performed.\n");
	}
	else
	{
		int rate, minsize;
		CleanerParams cp(&rate, &minsize);
		cp.exec();
		if (rate == 0 && minsize == 0)
			return;

		DataSelection data_selection;
		data_selection.allSlices = true;
		data_selection.tissues = true;
		emit BeginDatachange(data_selection, this);

		tc.ConnectedComponents();
		tc.MakeStat();
		tc.Clean(1.0f / rate, minsize);

		emit EndDatachange(this);
	}
}

void MainWindow::Wheelrotated(int delta)
{
	m_ZoomWidget->ZoomChanged(m_WorkShow->ReturnZoom() * pow(1.2, delta / 120.0));
}

void MainWindow::MousePosZoomChanged(const QPoint& point)
{
	m_BmpShow->SetMousePosZoom(point);
	m_WorkShow->SetMousePosZoom(point);
	//mousePosZoom = point;
}

FILE* MainWindow::SaveNotes(FILE* fp, unsigned short version)
{
	if (version >= 7)
	{
		QString text = m_MNotes->toPlainText();
		int dummy = (int)text.length();
		fwrite(&(dummy), 1, sizeof(int), fp);
		fwrite(text.toAscii(), 1, dummy, fp);
	}
	return fp;
}

FILE* MainWindow::LoadNotes(FILE* fp, unsigned short version)
{
	m_MNotes->clear();
	if (version >= 7)
	{
		int dummy = 0;
		fread(&dummy, sizeof(int), 1, fp);
		char* text = new char[dummy + 1];
		text[dummy] = '\0';
		fread(text, dummy, 1, fp);
		m_MNotes->setPlainText(QString(text));
		free(text);
	}
	return fp;
}

void MainWindow::DisconnectMouseclick()
{
	if (m_TabOld)
	{
		QObject_disconnect(m_BmpShow, SIGNAL(MousepressedSign(Point)), m_TabOld, SLOT(MouseClicked(Point)));
		QObject_disconnect(m_BmpShow, SIGNAL(MousereleasedSign(Point)), m_TabOld, SLOT(MouseReleased(Point)));
		QObject_disconnect(m_BmpShow, SIGNAL(MousemovedSign(Point)), m_TabOld, SLOT(MouseMoved(Point)));

		QObject_disconnect(m_WorkShow, SIGNAL(MousepressedSign(Point)), m_TabOld, SLOT(MouseClicked(Point)));
		QObject_disconnect(m_WorkShow, SIGNAL(MousereleasedSign(Point)), m_TabOld, SLOT(MouseReleased(Point)));
		QObject_disconnect(m_WorkShow, SIGNAL(MousemovedSign(Point)), m_TabOld, SLOT(MouseMoved(Point)));
	}
}

void MainWindow::ConnectMouseclick()
{
	if (m_TabOld)
	{
		QObject_connect(m_BmpShow, SIGNAL(MousepressedSign(Point)), m_TabOld, SLOT(MouseClicked(Point)));
		QObject_connect(m_BmpShow, SIGNAL(MousereleasedSign(Point)), m_TabOld, SLOT(MouseReleased(Point)));
		QObject_connect(m_BmpShow, SIGNAL(MousemovedSign(Point)), m_TabOld, SLOT(MouseMoved(Point)));
		QObject_connect(m_WorkShow, SIGNAL(MousepressedSign(Point)), m_TabOld, SLOT(MouseClicked(Point)));
		QObject_connect(m_WorkShow, SIGNAL(MousereleasedSign(Point)), m_TabOld, SLOT(MouseReleased(Point)));
		QObject_connect(m_WorkShow, SIGNAL(MousemovedSign(Point)), m_TabOld, SLOT(MouseMoved(Point)));
	}
}

void MainWindow::ReconnectmouseAfterrelease(Point)
{
	QObject_disconnect(m_WorkShow, SIGNAL(MousereleasedSign(Point)), this, SLOT(ReconnectmouseAfterrelease(Point)));
	ConnectMouseclick();
}

void MainWindow::HandleBeginDatachange(DataSelection& dataSelection, QWidget* sender, bool beginUndo)
{
	m_UndoStarted = beginUndo || m_UndoStarted;
	m_ChangeData = dataSelection;

	// Handle pending transforms
	if (m_MethodTab->currentWidget() == m_TransformWidget && sender != m_TransformWidget)
	{
		CancelTransformHelper();
	}

	// Begin undo
	if (beginUndo)
	{
		if (m_ChangeData.allSlices)
		{
			// Start undo for all slices
			m_CanUndo3D = m_Handler3D->ReturnUndo3D() && m_Handler3D->StartUndoall(m_ChangeData);
		}
		else
		{
			// Start undo for single slice
			m_Handler3D->StartUndo(m_ChangeData);
		}
	}
}

void MainWindow::EndUndoHelper(eEndUndoAction undoAction)
{
	if (m_UndoStarted)
	{
		if (undoAction == iseg::EndUndo)
		{
			if (m_ChangeData.allSlices)
			{
				// End undo for all slices
				if (m_CanUndo3D)
				{
					m_Handler3D->EndUndo();
					DoUndostepdone();
				}
				else
				{
					DoClearundo();
				}
			}
			else
			{
				// End undo for single slice
				m_Handler3D->EndUndo();
				DoUndostepdone();
			}
			m_UndoStarted = false;
		}
		else if (undoAction == iseg::MergeUndo)
		{
			m_Handler3D->MergeUndo();
		}
		else if (undoAction == iseg::AbortUndo)
		{
			m_Handler3D->AbortUndo();
			m_UndoStarted = false;
		}
	}
	else if (undoAction == iseg::ClearUndo)
	{
		DoClearundo();
	}
}

void MainWindow::HandleEndDatachange(QWidget* sender, eEndUndoAction undoAction)
{
	ISEG_DEBUG("MainWindow::handle_end_datachange (tissues:" << m_ChangeData.tissues << ", work:" << m_ChangeData.work << ")");

	// End undo
	EndUndoHelper(undoAction);

	// Handle 3d data change
	if (m_ChangeData.allSlices)
	{
		Slices3dChanged(sender != m_BitstackWidget);
	}

	// Update ranges
	UpdateRangesHelper();

	// Block changed data signals for visible widget
	if (sender == m_MethodTab->currentWidget())
	{
		QObject_disconnect(this, SIGNAL(BmpChanged()), sender, SLOT(BmpChanged()));
		QObject_disconnect(this, SIGNAL(WorkChanged()), sender, SLOT(WorkChanged()));
		QObject_disconnect(this, SIGNAL(TissuesChanged()), sender, SLOT(TissuesChanged()));
		QObject_disconnect(this, SIGNAL(MarksChanged()), sender, SLOT(MarksChanged()));
	}

	// Signal changed data
	if (m_ChangeData.bmp)
	{
		emit BmpChanged();
	}
	if (m_ChangeData.work)
	{
		emit WorkChanged();
	}
	if (m_ChangeData.tissues)
	{
		emit TissuesChanged();
	}
	if (m_ChangeData.marks)
	{
		emit MarksChanged();
	}

	// Signal changed data
	// \hack BL this looks like a hack, fixme
	if (m_ChangeData.bmp && m_ChangeData.work && m_ChangeData.allSlices)
	{
		// Do not reinitialize m_MultiDataset_widget after swapping
		if (m_NewDataAfterSwap)
		{
			m_NewDataAfterSwap = false;
		}
		else
		{
			m_MultidatasetWidget->NewLoaded();
		}
	}

	if (sender == m_MethodTab->currentWidget())
	{
		QObject_connect(this, SIGNAL(BmpChanged()), sender, SLOT(BmpChanged()));
		QObject_connect(this, SIGNAL(WorkChanged()), sender, SLOT(WorkChanged()));
		QObject_connect(this, SIGNAL(TissuesChanged()), sender, SLOT(TissuesChanged()));
		QObject_connect(this, SIGNAL(MarksChanged()), sender, SLOT(MarksChanged()));
	}
}

void MainWindow::DatasetChanged()
{
	emit BmpChanged();

	ResetBrightnesscontrast();
}

void MainWindow::UpdateRangesHelper()
{
	if (m_ChangeData.bmp)
	{
		if (m_ChangeData.allSlices)
		{
			m_BmpShow->UpdateRange();
		}
		else
		{
			m_BmpShow->UpdateRange(m_ChangeData.sliceNr);
		}
		UpdateBrightnesscontrast(true, false);
	}
	if (m_ChangeData.work)
	{
		if (m_ChangeData.allSlices)
		{
			m_WorkShow->UpdateRange();
		}
		else
		{
			m_WorkShow->UpdateRange(m_ChangeData.sliceNr);
		}
		UpdateBrightnesscontrast(false, false);
	}
}

void MainWindow::CancelTransformHelper()
{
	QObject_disconnect(m_TransformWidget, SIGNAL(BeginDatachange(DataSelection&,QWidget*,bool)), this, SLOT(HandleBeginDatachange(DataSelection&,QWidget*,bool)));
	QObject_disconnect(m_TransformWidget, SIGNAL(EndDatachange(QWidget*,eEndUndoAction)), this, SLOT(HandleEndDatachange(QWidget*,eEndUndoAction)));
	m_TransformWidget->CancelTransform();
	QObject_connect(m_TransformWidget, SIGNAL(BeginDatachange(DataSelection&,QWidget*,bool)), this, SLOT(HandleBeginDatachange(DataSelection&,QWidget*,bool)));
	QObject_connect(m_TransformWidget, SIGNAL(EndDatachange(QWidget*,eEndUndoAction)), this, SLOT(HandleEndDatachange(QWidget*,eEndUndoAction)));

	// Signal changed data
	bool bmp, work, tissues;
	m_TransformWidget->GetDataSelection(bmp, work, tissues);
	if (bmp)
	{
		emit BmpChanged();
	}
	if (work)
	{
		emit WorkChanged();
	}
	if (tissues)
	{
		emit TissuesChanged();
	}
}

void MainWindow::HandleBeginDataexport(DataSelection& dataSelection, QWidget* sender)
{
	// Handle pending transforms
	if (m_MethodTab->currentWidget() == m_TransformWidget && (dataSelection.bmp || dataSelection.work || dataSelection.tissues))
	{
		CancelTransformHelper();
	}

	// Handle pending tissue hierarchy modifications
	if (dataSelection.tissueHierarchy)
	{
		m_TissueHierarchyWidget->HandleChangedHierarchy();
	}
}

void MainWindow::HandleEndDataexport(QWidget* sender) {}

void MainWindow::ExecuteSavecolorlookup()
{
	if (!m_Handler3D->GetColorLookupTable())
	{
		QMessageBox::warning(this, "iSeg", "No color lookup table to export\n", QMessageBox::Ok | QMessageBox::Default);
		return;
	}

	QString savefilename = RecentPlaces::GetSaveFileName(this, "Save as", QString::null, "iSEG Color Lookup Table (*.lut)");
	if (!savefilename.endsWith(QString(".lut")))
		savefilename.append(".lut");

	XdmfImageWriter writer(savefilename.toStdString().c_str());
	writer.SetCompression(m_Handler3D->GetCompression());
	if (!writer.WriteColorLookup(m_Handler3D->GetColorLookupTable().get(), true))
	{
		QMessageBox::warning(this, "iSeg", "ERROR: occurred while exporting color lookup table\n", QMessageBox::Ok | QMessageBox::Default);
	}
}

void MainWindow::ExecuteVotingReplaceLabels()
{
	auto sel = m_Handler3D->TissueSelection();
	if (sel.size() == 1 && !m_Handler3D->TissueLocks().at(sel.at(0)))
	{
		tissues_size_t fg = sel.front();
		std::array<unsigned int, 3> radius = {1, 1, 1};

		DataSelection data_selection;
		data_selection.allSlices = true;
		data_selection.tissues = true;
		emit BeginDatachange(data_selection, this);

		auto remaining_voxels = VotingReplaceLabel(m_Handler3D, fg, 0, radius, 1, 10);
		if (remaining_voxels != 0)
		{
			remaining_voxels = VotingReplaceLabel(m_Handler3D, fg, 0, radius, 0, 1);
		}

		if (remaining_voxels != 0)
		{
			ISEG_INFO("Remaining voxels after relabeling: " << remaining_voxels);
		}

		emit EndDatachange(this, iseg::EndUndo);
	}
	else
	{
		QMessageBox::warning(this, "iSeg", "Please select only one non-locked tissue\n", QMessageBox::Ok | QMessageBox::Default);
	}
}

void MainWindow::ExecuteTargetConnectedComponents()
{
	ProgressDialog progress("Connected component analysis", this);

	DataSelection data_selection;
	data_selection.allSlices = true;
	data_selection.work = true;
	emit BeginDatachange(data_selection, this);

	bool ok = m_Handler3D->ComputeTargetConnectivity(&progress);

	emit EndDatachange(this, ok ? iseg::EndUndo : iseg::AbortUndo);
}

void MainWindow::ExecuteSplitTissue()
{
	auto sel = m_Handler3D->TissueSelection();
	if (sel.size() == 1 && !m_Handler3D->TissueLocks().at(sel.at(0)))
	{
		ProgressDialog progress("Connected component analysis", this);

		DataSelection data_selection;
		data_selection.allSlices = true;
		data_selection.tissues = true;
		emit BeginDatachange(data_selection, this);

		bool ok = m_Handler3D->ComputeSplitTissues(sel.front(), &progress);

		emit EndDatachange(this, ok ? iseg::EndUndo : iseg::AbortUndo);

		// update tree view after adding new tissues
		m_TissueTreeWidget->UpdateTreeWidget();
		m_TissueTreeWidget->UpdateTissueIcons();
		m_TissueTreeWidget->UpdateFolderIcons();
	}
	else
	{
		QMessageBox::warning(this, "iSeg", "Please select only one non-locked tissue\n", QMessageBox::Ok | QMessageBox::Default);
	}
}

} // namespace iseg
