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

#include "LoaderWidgets.h"
#include "SlicesHandler.h"
#include "StdStringToQString.h"
#include "TissueHierarchy.h"
#include "TissueInfos.h"
#include "WidgetCollection.h"
#include "bmp_read_1.h"
#include "config.h"

#include "Interface/FormatTooltip.h"

#include <Q3HBoxLayout>
#include <Q3VBoxLayout>
#include <QHeaderView>
#include <QPaintEvent>
#include <QProgressDialog>
#include <QXmlInputSource>
#include <q3hbox.h>
#include <q3listbox.h>
#include <q3mimefactory.h>
#include <q3vbox.h>
#include <qapplication.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qdialog.h>
#include <qfile.h>
#include <qfiledialog.h>
#include <qimage.h>
#include <qinputdialog.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qmessagebox.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qslider.h>
#include <qspinbox.h>
#include <qwidget.h>

#include <boost/algorithm/string.hpp>

#include <algorithm>
#include <fstream>

#define UNREFERENCED_PARAMETER(P) (P)

#define TISSUETREEWIDGET_COLUMN_COUNT 2
#define TISSUETREEWIDGET_COLUMN_NAME 0
#define TISSUETREEWIDGET_COLUMN_TYPE 1
#define TISSUETREEWIDGET_COLUMN_FOLDER 2

using namespace std;
using namespace iseg;

namespace {
inline bool SearchFilter(const std::string& text, const std::string& filter)
{
	namespace algo = boost::algorithm;

	if (!filter.empty())
	{
		auto search_text = filter;
		algo::trim(search_text);

		std::vector<std::string> search_tokens;
		algo::split(search_tokens, search_text, algo::is_space(),
				algo::token_compress_on);

		for (auto tok : search_tokens)
		{
			if (!tok.empty() && !algo::icontains(text, tok))
			{
				return false;
			}
		}
	}
	return true;
}
} // namespace

ScaleWork::ScaleWork(SlicesHandler* hand3D, QDir picpath, QWidget* parent,
		const char* name, Qt::WindowFlags wFlags)
		//  : QWidget( parent, name, wFlags ),handler3D(hand3D)
		: QDialog(parent, name, TRUE, wFlags), handler3D(hand3D)
{
	activeslice = handler3D->active_slice();
	bmphand = handler3D->get_activebmphandler();

	vbox1 = new Q3VBox(this);
	hbox1 = new Q3HBox(vbox1);
	lL = new QLabel(QString("Low: "), hbox1);
	lL->show();
	limitLow = new QLineEdit(QString::number((int)0), hbox1);
	limitLow->show();
	hbox1->show();
	hbox2 = new Q3HBox(vbox1);
	lH = new QLabel(QString("High: "), hbox2);
	lH->show();
	limitHigh = new QLineEdit(QString::number((int)255), hbox2);
	limitHigh->show();
	hbox2->show();
	allslices = new QCheckBox(QString("3D"), vbox1);
	getRange = new QPushButton("Get Range", vbox1);
	getRange->show();
	doScale = new QPushButton("Scale", vbox1);
	doScale->show();
	doCrop = new QPushButton("Clamp", vbox1);
	doCrop->show();
	hbox3 = new Q3HBox(vbox1);
	lb_brightness = new QLabel(QString("B: "), hbox3);
	lb_brightness->show();
	lb_brightness->setPixmap(
			QIcon(picpath.absFilePath(QString("icon-brightness.png")).ascii())
					.pixmap());
	sl_brighness = new QSlider(Qt::Horizontal, hbox3);
	sl_brighness->setRange(0, 100);
	sl_brighness->setValue(30);
	hbox3->show();
	hbox4 = new Q3HBox(vbox1);
	lb_contrast = new QLabel(QString("C: "), hbox4);
	lb_contrast->show();
	lb_contrast->setPixmap(
			QIcon(picpath.absFilePath(QString("icon-contrast.png")).ascii())
					.pixmap());
	sl_contrast = new QSlider(Qt::Horizontal, hbox4);
	sl_contrast->setRange(0, 99);
	sl_contrast->setValue(30);
	hbox4->show();

	closeButton = new QPushButton("Close", vbox1);

	vbox1->setFixedSize(vbox1->sizeHint());
	setFixedSize(vbox1->size());

	QObject::connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));
	QObject::connect(getRange, SIGNAL(clicked()), this,
			SLOT(getrange_pushed()));
	QObject::connect(doScale, SIGNAL(clicked()), this, SLOT(scale_pushed()));
	QObject::connect(doCrop, SIGNAL(clicked()), this, SLOT(crop_pushed()));
	QObject::connect(sl_brighness, SIGNAL(sliderMoved(int)), this,
			SLOT(slider_changed(int)));
	QObject::connect(sl_brighness, SIGNAL(sliderPressed()), this,
			SLOT(slider_pressed()));
	QObject::connect(sl_brighness, SIGNAL(sliderReleased()), this,
			SLOT(slider_released()));
	QObject::connect(sl_contrast, SIGNAL(sliderMoved(int)), this,
			SLOT(slider_changed(int)));
	QObject::connect(sl_contrast, SIGNAL(sliderPressed()), this,
			SLOT(slider_pressed()));
	QObject::connect(sl_contrast, SIGNAL(sliderReleased()), this,
			SLOT(slider_released()));

	getrange_pushed();

	return;
}

ScaleWork::~ScaleWork() { delete vbox1; }

void ScaleWork::getrange_pushed()
{
	activeslice = handler3D->active_slice();
	bmphand = handler3D->get_activebmphandler();
	Pair p;
	if (allslices->isChecked())
	{
		handler3D->get_range(&p);
	}
	else
	{
		bmphand->get_range(&p);
	}
	minval = minval1 = p.low;
	maxval = maxval1 = p.high;
	limitLow->setText(QString::number((double)p.low, 'f', 6));
	limitHigh->setText(QString::number((double)p.high, 'f', 6));

	QObject::disconnect(sl_brighness, SIGNAL(sliderMoved(int)), this,
			SLOT(slider_changed(int)));
	QObject::disconnect(sl_contrast, SIGNAL(sliderMoved(int)), this,
			SLOT(slider_changed(int)));
	int brightnessint = (int)(100 * (maxval - 127.5f) / (maxval - minval));
	if (brightnessint < 0)
	{
		brightnessint = 0;
	}
	if (brightnessint > 100)
	{
		brightnessint = 100;
	}
	sl_brighness->setValue(brightnessint);
	int contrastint = 100 - (int)(25500.0f / (maxval - minval));
	if (contrastint < 0)
	{
		contrastint = 0;
	}
	if (contrastint > 99)
	{
		contrastint = 99;
	}
	sl_contrast->setValue(contrastint);
	QObject::connect(sl_brighness, SIGNAL(sliderMoved(int)), this,
			SLOT(slider_changed(int)));
	QObject::connect(sl_contrast, SIGNAL(sliderMoved(int)), this,
			SLOT(slider_changed(int)));

	if (allslices->isChecked())
	{
		bmphand->get_range(&p);
		minval1 = p.low;
		maxval1 = p.high;
	}
}

void ScaleWork::scale_pushed()
{
	activeslice = handler3D->active_slice();
	bmphand = handler3D->get_activebmphandler();

	Pair p;
	bool b1, b2;
	p.low = limitLow->text().toFloat(&b1);
	p.high = limitHigh->text().toFloat(&b2);
	if (b1 && b2)
	{
		iseg::DataSelection dataSelection;
		dataSelection.allSlices = allslices->isChecked();
		dataSelection.sliceNr = handler3D->active_slice();
		dataSelection.work = true;
		emit begin_datachange(dataSelection, this);

		if (allslices->isChecked())
		{
			handler3D->scale_colors(p);
		}
		else
		{
			bmphand->scale_colors(p);
		}

		emit end_datachange(this);
	}
}

void ScaleWork::crop_pushed()
{
	activeslice = handler3D->active_slice();
	bmphand = handler3D->get_activebmphandler();

	iseg::DataSelection dataSelection;
	dataSelection.allSlices = allslices->isChecked();
	dataSelection.sliceNr = handler3D->active_slice();
	dataSelection.work = true;
	emit begin_datachange(dataSelection, this);

	if (allslices->isChecked())
	{
		handler3D->crop_colors();
	}
	else
	{
		bmphand->crop_colors();
	}

	emit end_datachange(this);
}

void ScaleWork::slicenr_changed()
{
	//	if(activeslice!=handler3D->get_activeslice()){
	activeslice = handler3D->active_slice();
	bmphand_changed(handler3D->get_activebmphandler());
	//	}
}

void ScaleWork::bmphand_changed(bmphandler* bmph)
{
	bmphand = bmph;
	return;
}

void ScaleWork::slider_changed(int newval)
{
	UNREFERENCED_PARAMETER(newval);
	if (minval1 < maxval1)
	{
		bmphand = handler3D->get_activebmphandler();
		Pair p, p1;
		bmphand->get_range(&p1);
		float subpos1, subpos2;
		subpos1 = ((1.0f - 0.01f * sl_brighness->value()) * (maxval - minval) -
									(minval1 - minval)) /
							(maxval1 - minval1);
		subpos2 = (1.0f - 0.01f * sl_contrast->value()) * (maxval - minval) *
							0.5f / (maxval1 - minval1);
		p.low = p1.low + (p1.high - p1.low) * (subpos1 - subpos2);
		p.high = p1.low + (p1.high - p1.low) * (subpos1 + subpos2);

		//		if(allslices->isChecked()){
		//			handler3D->scale_colors(p);
		//		} else {
		//			bmphand=handler3D->get_activebmphandler();
		bmphand->scale_colors(p);
		//		}
	}
	emit end_datachange(this, iseg::NoUndo);
}

void ScaleWork::slider_pressed()
{
	getrange_pushed();

	iseg::DataSelection dataSelection;
	dataSelection.allSlices = allslices->isChecked();
	dataSelection.sliceNr = handler3D->active_slice();
	dataSelection.work = true;
	emit begin_datachange(dataSelection, this);
}

void ScaleWork::slider_released()
{
	if (allslices->isChecked())
	{
		bmphand = handler3D->get_activebmphandler();
		Pair p, p1, p2;
		bmphand->get_range(&p);
		float subpos1, subpos2;
		subpos1 = -minval1 / (maxval1 - minval1);
		subpos2 = (255.0f - minval) / (maxval1 - minval1);
		p1.low = p.low + (p.high - p.low) * subpos1;
		p1.high = p.low + (p.high - p.low) * subpos2;
		bmphand->scale_colors(p1);
		subpos1 = 1.0f - 0.01f * sl_brighness->value();
		subpos2 = 0.5f - 0.005f * sl_contrast->value();
		p2.low = minval + (maxval - minval) * (subpos1 - subpos2);
		p2.high = minval + (maxval - minval) * (subpos1 + subpos2);
		handler3D->scale_colors(p2);
	}

	emit end_datachange(this);
}

HistoWin::HistoWin(unsigned int* histo1, QWidget* parent, const char* name,
		Qt::WindowFlags wFlags)
		: QWidget(parent, name, wFlags)
{
	histo = histo1;
	image.create(258, 258, 8);
	image.setNumColors(256);
	for (int i = 0; i < 256; i++)
	{
		image.setColor(i, qRgb(i, i, i));
	}
	setFixedSize(258, 258);
	update();
	show();
	return;
}

void HistoWin::update()
{
	unsigned int maxim = 1;
	//unsigned int maxim1=0;
	for (int i = 0; i < 256; i++)
	{
		maxim = max(maxim, histo[i]);
	}

	image.fill(0);

	for (int i = 0; i < 256; i++)
	{
		for (int j = 255; j > (int)(255 - ((histo[i] * 255) / maxim)); j--)
		{
			image.setPixel(i + 1, j + 1, 255);
		}
	}

	repaint();

	return;
}

void HistoWin::histo_changed(unsigned int* histo1)
{
	histo = histo1;

	return;
}

void HistoWin::paintEvent(QPaintEvent* e)
{
	if (image.size() != QSize(0, 0)) // is an image loaded?
	{
		QPainter painter(this);
		painter.setClipRect(e->rect());
		painter.drawImage(0, 0, image);
	}
}

ShowHisto::ShowHisto(SlicesHandler* hand3D, QWidget* parent, const char* name,
		Qt::WindowFlags wFlags)
		: QDialog(parent, name, TRUE, wFlags), handler3D(hand3D)
{
	activeslice = handler3D->active_slice();
	bmphand = handler3D->get_activebmphandler();

	vbox1 = new Q3VBox(this);
	bmphand->make_histogram(true);
	histwindow = new HistoWin(bmphand->return_histogram(), vbox1, name, wFlags);
	histwindow->setFixedSize(258, 258);

	hbox1 = new Q3HBox(vbox1);
	pictselect = new QButtonGroup(this);
	//	pictselect->hide();
	bmppict = new QRadioButton(QString("Source"), hbox1);
	workpict = new QRadioButton(QString("Target"), hbox1);
	pictselect->insert(bmppict);
	pictselect->insert(workpict);
	workpict->setChecked(TRUE);
	workpict->show();
	bmppict->show();
	hbox1->show();

	hbox2 = new Q3HBox(vbox1);
	subsect = new QCheckBox(QString("Subsection "), hbox2);
	subsect->setChecked(TRUE);
	subsect->show();
	vbox2 = new Q3VBox(hbox2);
	hbox3 = new Q3HBox(vbox2);
	xoffs = new QLabel(QString("x-Offset: "), hbox3);
	xoffset = new QSpinBox(0, (int)bmphand->return_width(), 1, hbox3);
	xoffset->setValue(0);
	xoffset->show();
	yoffs = new QLabel(QString("y-Offset: "), hbox3);
	yoffset = new QSpinBox(0, (int)bmphand->return_height(), 1, hbox3);
	yoffset->show();
	xoffset->setValue(0);
	hbox3->show();
	hbox4 = new Q3HBox(vbox2);
	xl = new QLabel(QString("x-Length: "), hbox4);
	xlength = new QSpinBox(0, (int)bmphand->return_width(), 1, hbox4);
	xlength->show();
	xlength->setValue((int)bmphand->return_width());
	yl = new QLabel(QString("y-Length: "), hbox4);
	ylength = new QSpinBox(0, (int)bmphand->return_height(), 1, hbox4);
	ylength->show();
	ylength->setValue((int)bmphand->return_height());
	hbox4->show();
	updateSubsect = new QPushButton("Update Subsection", vbox2);
	vbox2->show();
	hbox2->show();
	closeButton = new QPushButton("Close", vbox1);

	draw_histo();
	histwindow->show();

	vbox1->show();

	QObject::connect(updateSubsect, SIGNAL(clicked()), this,
			SLOT(subsect_update()));
	QObject::connect(workpict, SIGNAL(toggled(bool)), this,
			SLOT(pict_toggled(bool)));
	QObject::connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));
	QObject::connect(subsect, SIGNAL(clicked()), this, SLOT(subsect_toggled()));

	vbox1->setFixedSize(vbox1->sizeHint());
	setFixedSize(vbox1->size());
}

ShowHisto::~ShowHisto()
{
	delete vbox1;
	delete pictselect;
}

void ShowHisto::subsect_toggled()
{
	bool isset = subsect->isOn();
	if (isset)
	{
		vbox2->show();
	}
	else
	{
		vbox2->hide();
	}
	draw_histo();

	return;
}

void ShowHisto::pict_toggled(bool on)
{
	UNREFERENCED_PARAMETER(on);
	draw_histo();
	return;
}

void ShowHisto::subsect_update()
{
	draw_histo();
	return;
}

void ShowHisto::draw_histo()
{
	//Point p;
	if (bmppict->isOn())
	{
		bmphand->swap_bmpwork();
		if (subsect->isOn())
		{
			Point p;
			p.px = xoffset->value();
			p.py = yoffset->value();
			bmphand->make_histogram(
					p,
					min((int)bmphand->return_width() - xoffset->value(),
							xlength->value()),
					min((int)bmphand->return_height() - yoffset->value(),
							ylength->value()),
					true);
		}
		else
		{
			bmphand->make_histogram(true);
		}
		bmphand->swap_bmpwork();
	}
	else
	{
		if (subsect->isOn())
		{
			Point p;
			p.px = xoffset->value();
			p.py = yoffset->value();
			bmphand->make_histogram(
					p,
					min((int)bmphand->return_width() - xoffset->value(),
							xlength->value()),
					min((int)bmphand->return_height() - yoffset->value(),
							ylength->value()),
					true);
		}
		else
		{
			bmphand->make_histogram(true);
		}
	}

	histwindow->update();
	return;
}

void ShowHisto::slicenr_changed()
{
	//	if(activeslice!=handler3D->get_activeslice()){
	activeslice = handler3D->active_slice();
	bmphand_changed(handler3D->get_activebmphandler());
	//	}
}

void ShowHisto::bmphand_changed(bmphandler* bmph)
{
	bmphand = bmph;
	histwindow->histo_changed(bmphand->return_histogram());
	draw_histo();
	return;
}

void ShowHisto::newloaded()
{
	activeslice = handler3D->active_slice();
	bmphand_changed(handler3D->get_activebmphandler());
}

colorshower::colorshower(int lx1, int ly1, QWidget* parent, const char* name,
		Qt::WindowFlags wFlags)
		: QWidget(parent, name, wFlags)
{
	lx = lx1;
	ly = ly1;
	fr = 0;
	fg = 0;
	fb = 0;
	opac = 0.5f;
	setFixedSize(lx, ly);

	repaint();
	return;
}

void colorshower::color_changed(float fr1, float fg1, float fb1, float opac1)
{
	fr = fr1;
	fg = fg1;
	fb = fb1;
	opac = opac1;

	repaint();
}

void colorshower::paintEvent(QPaintEvent* e)
{
	QColor color;
	color.setRgb((int)(opac * fr * 255), (int)(opac * fg * 255),
			(int)(opac * fb * 255));
	QPainter painter(this);
	painter.setClipRect(e->rect());
	painter.fillRect(0, 0, lx, ly, color);
	color.setRgb((int)((1 - opac + opac * fr) * 255),
			(int)((1 - opac + opac * fg) * 255),
			(int)((1 - opac + opac * fb) * 255));
	painter.setClipRect(e->rect());
	painter.fillRect(lx / 4, ly / 4, lx / 2, ly / 2, color);

	return;
}

TissueTreeWidget::TissueTreeWidget(TissueHiearchy* hierarchy, QDir picpath,
		QWidget* parent)
		: QTreeWidget(parent), hierarchies(hierarchy)
{
	picturePath = picpath;
	sortByNameAscending = true;
	sortByTypeAscending = true;

	setColumnCount(TISSUETREEWIDGET_COLUMN_COUNT);
	hideColumn(TISSUETREEWIDGET_COLUMN_TYPE);
	hideColumn(TISSUETREEWIDGET_COLUMN_FOLDER);
	setAcceptDrops(true);
	setDragDropMode(QTreeWidget::InternalMove);
	setExpandsOnDoubleClick(false);
	setHeaderHidden(true);
	//setIndentation(20);
	setItemsExpandable(true);
	setRootIsDecorated(true);
	//xxxb	setFixedHeight(250);
	//xxxb	setFixedWidth(110);
	QObject::connect(this, SIGNAL(itemExpanded(QTreeWidgetItem*)), this,
			SLOT(resize_columns_to_contents(QTreeWidgetItem*)));
	QObject::connect(this, SIGNAL(itemCollapsed(QTreeWidgetItem*)), this,
			SLOT(resize_columns_to_contents(QTreeWidgetItem*)));
	initialize();
}

TissueTreeWidget::~TissueTreeWidget() {}

void TissueTreeWidget::initialize()
{
	// Clear internal representations
	hierarchies->initialize();

	emit hierarchy_list_changed();
}

void TissueTreeWidget::set_tissue_filter(const QString& filter)
{
	if (filter.toStdString() != tissue_filter)
	{
		tissue_filter = filter.toStdString();

		// set visibility on items
		update_visibility();
	}
}

bool TissueTreeWidget::is_visible(tissues_size_t type) const
{
	if (type > 0)
	{
		if (auto item = find_tissue_item(type))
		{
			return !item->isHidden();
		}
	}
	return false;
}

void TissueTreeWidget::update_visibility()
{
	for (unsigned int i = 0; i < topLevelItemCount(); ++i)
	{
		update_visibility_recursive(topLevelItem(i));
	}
}

void TissueTreeWidget::update_visibility_recursive(QTreeWidgetItem* current)
{
	// setHidden hides/shows recursively
	bool const matches = SearchFilter(get_name(current).toStdString(), tissue_filter);
	bool const is_folder = get_is_folder(current);
	if (matches || !is_folder)
	{
		current->setHidden(!matches);
		return;
	}

	// first make all visible
	if (get_is_folder(current))
	{
		current->setHidden(false);
	}

	// filter children
	bool any_child_visible = false;
	for (unsigned int i = 0; i < current->childCount(); ++i)
	{
		QTreeWidgetItem* child = current->child(i);
		update_visibility_recursive(child);

		if (!child->isHidden())
		{
			any_child_visible = true;
		}
	}

	// hide empty folders
	if (!any_child_visible)
	{
		current->setHidden(true);
	}
}

void TissueTreeWidget::update_hierarchy()
{
	modified = true;
	hierarchies->set_selected_hierarchy(create_current_hierarchy());
}

TissueHierarchyItem* TissueTreeWidget::create_current_hierarchy()
{
	// Create internal representation from current QTreeWidget
	TissueHierarchyItem* root = new TissueHierarchyItem(true, QString("root"));

	for (unsigned int i = 0; i < topLevelItemCount(); ++i)
	{
		// Add top-level child
		QTreeWidgetItem* currWidgetItem = topLevelItem(i);
		TissueHierarchyItem* newTreeItem = create_hierarchy_item(currWidgetItem);
		root->AddChild(newTreeItem);

		// Subtree of current child
		if (newTreeItem->GetIsFolder())
		{
			create_hierarchy_recursively(currWidgetItem, newTreeItem);
		}
	}

	return root;
}

void TissueTreeWidget::create_hierarchy_recursively(
		QTreeWidgetItem* parentIn, TissueHierarchyItem* parentOut)
{
	for (unsigned int i = 0; i < parentIn->childCount(); ++i)
	{
		// Add child
		QTreeWidgetItem* currWidgetItem = parentIn->child(i);
		TissueHierarchyItem* newTreeItem = create_hierarchy_item(currWidgetItem);
		parentOut->AddChild(newTreeItem);

		// Subtree of current child
		if (newTreeItem->GetIsFolder())
		{
			create_hierarchy_recursively(currWidgetItem, newTreeItem);
		}
	}
}

TissueHierarchyItem* TissueTreeWidget::create_hierarchy_item(QTreeWidgetItem* item)
{
	return new TissueHierarchyItem(get_is_folder(item), item->text(TISSUETREEWIDGET_COLUMN_NAME));
}

namespace {

QPixmap generatePixmap(tissues_size_t tissuenr)
{
	QPixmap abc(10, 10);
	if (tissuenr > TissueInfos::GetTissueCount())
	{
		abc.fill(QColor(0, 0, 0));
		return abc;
	}
	unsigned char r, g, b;
	TissueInfoStruct* tissueInfo = TissueInfos::GetTissueInfo(tissuenr);
	tissueInfo->GetColorRGB(r, g, b);
	abc.fill(QColor(r, g, b));
	if (tissueInfo->locked)
	{
		QPainter painter(&abc);
		//		painter.drawText(3,7,QString("L"));
		float r, g, b;
		r = (tissueInfo->color[0] + 0.5f);
		if (r >= 1.0f)
			r = r - 1.0f;
		g = (tissueInfo->color[1] + 0.5f);
		if (g >= 1.0f)
			g = g - 1.0f;
		b = (tissueInfo->color[2] + 0.5f);
		if (b >= 1.0f)
			b = b - 1.0f;
		painter.setPen(QColor(int(r * 255), int(g * 255), int(b * 255)));
		painter.drawLine(0, 0, 9, 9);
		painter.drawLine(0, 9, 9, 0);
		painter.drawRect(0, 0, 9, 9);
	}

	return abc;
}

} // namespace

QTreeWidgetItem* TissueTreeWidget::create_hierarchy_item(bool isFolder, const QString& name)
{
	if (isFolder)
	{
		QTreeWidgetItem* newFolder = new QTreeWidgetItem();
		newFolder->setIcon(TISSUETREEWIDGET_COLUMN_NAME, QIcon(picturePath.absFilePath(QString("fileopen.png")).ascii()));
		newFolder->setText(TISSUETREEWIDGET_COLUMN_NAME, name);
		newFolder->setText(TISSUETREEWIDGET_COLUMN_FOLDER, QString::number(isFolder));
		newFolder->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
		newFolder->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
		return newFolder;
	}
	else
	{
		tissues_size_t type = TissueInfos::GetTissueType(ToStd(name));
		QTreeWidgetItem* newTissue = new QTreeWidgetItem();
		newTissue->setIcon(TISSUETREEWIDGET_COLUMN_NAME, generatePixmap(type));
		newTissue->setText(TISSUETREEWIDGET_COLUMN_NAME, name);
		newTissue->setText(TISSUETREEWIDGET_COLUMN_TYPE, QString::number(type));
		newTissue->setText(TISSUETREEWIDGET_COLUMN_FOLDER,
				QString::number(isFolder));
		newTissue->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicator);
		newTissue->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled |
												Qt::ItemIsDragEnabled);
		return newTissue;
	}
}

void TissueTreeWidget::insert_item(bool isFolder, const QString& name)
{
	QTreeWidgetItem* newItem = create_hierarchy_item(isFolder, name);

	if (QTreeWidgetItem* currItem = currentItem())
	{
		if (get_is_folder(currItem) && currItem->isExpanded())
		{
			// Insert as first child of current folder
			currItem->insertChild(0, newItem);
		}
		else
		{
			// Insert at current position
			QTreeWidgetItem* currParent = currItem->parent();
			if (currParent == 0)
			{
				insertTopLevelItem(indexOfTopLevelItem(currItem), newItem);
			}
			else
			{
				currParent->insertChild(currParent->indexOfChild(currItem), newItem);
			}
		}
	}
	else // empty tissue list
	{
		invisibleRootItem()->insertChild(0, newItem);
	}

	setCurrentItem(newItem);

	// Update internal representation
	update_hierarchy();
}

void TissueTreeWidget::insert_item(bool isFolder, const QString& name,
		QTreeWidgetItem* insertAbove)
{
	if (insertAbove == 0)
	{
		return;
	}

	QTreeWidgetItem* newItem = create_hierarchy_item(isFolder, name);

	QTreeWidgetItem* parent = insertAbove->parent();
	if (parent == 0)
	{
		insertTopLevelItem(indexOfTopLevelItem(insertAbove), newItem);
	}
	else
	{
		parent->insertChild(parent->indexOfChild(insertAbove), newItem);
	}

	setCurrentItem(newItem);

	// Update internal representation
	update_hierarchy();
}

void TissueTreeWidget::insert_item(bool isFolder, const QString& name,
		QTreeWidgetItem* parent, unsigned int index)
{
	QTreeWidgetItem* newItem = create_hierarchy_item(isFolder, name);

	if (parent == 0)
	{
		insertTopLevelItem(index, newItem);
	}
	else
	{
		parent->insertChild(index, newItem);
	}

	setCurrentItem(newItem);

	// Update internal representation
	update_hierarchy();
}

void TissueTreeWidget::remove_tissue(const QString& name)
{
	// Removes a tissue completely from all hierarchies
	blockSignals(true);
	for (int i = 0; i < topLevelItemCount(); ++i)
	{
		QTreeWidgetItem* currItem = topLevelItem(i);
		if (get_is_folder(currItem))
		{
			remove_tissue_recursively(currItem, name);
		}
		else if (get_name(currItem).compare(name) == 0)
		{
			// Delete tissue item
			takeTopLevelItem(i);
			delete currItem;
			--i;
		}
		else
		{
			// Update tissue type
			tissues_size_t newType = TissueInfos::GetTissueType(ToStd(currItem->text(TISSUETREEWIDGET_COLUMN_NAME)));
			currItem->setText(TISSUETREEWIDGET_COLUMN_TYPE, QString::number(newType));
		}
	}
	blockSignals(false);

	// Update internal representation
	update_hierarchy();

	// Propagate tissue removal to other hierarchies
	hierarchies->update_hierarchies();
}

void TissueTreeWidget::remove_tissue_recursively(QTreeWidgetItem* parent, const QString& name)
{
	for (int i = 0; i < parent->childCount(); ++i)
	{
		QTreeWidgetItem* currItem = parent->child(i);
		if (get_is_folder(currItem))
		{
			remove_tissue_recursively(currItem, name);
		}
		else if (get_name(currItem).compare(name) == 0)
		{
			// Delete tissue item
			parent->takeChild(i);
			delete currItem;
			--i;
		}
		else
		{
			// Update tissue type
			tissues_size_t newType = TissueInfos::GetTissueType(ToStd(currItem->text(TISSUETREEWIDGET_COLUMN_NAME)));
			currItem->setText(TISSUETREEWIDGET_COLUMN_TYPE, QString::number(newType));
		}
	}
}

void TissueTreeWidget::remove_items(const std::vector<QTreeWidgetItem*>& items)
{
	std::set<QTreeWidgetItem*> deleted;
	for (auto currItem: items)
	{
		if (deleted.count(currItem) == 0)
		{
			remove_item(currItem, false, false);
			deleted.insert(currItem);
		}
		else
		{
			ISEG_WARNING("trying to delete item again");
		}
	}

	// Update internal representation
	update_hierarchy();

	// Propagate tissue removal to other hierarchies
	hierarchies->update_hierarchies();

	// Update tissue indices
	update_tissue_indices();
}

void TissueTreeWidget::remove_item(QTreeWidgetItem* currItem, bool removeChildren, bool updateRepresentation)
{
	// Removes item in QTreeWidget and internal representations
	QTreeWidgetItem* currParent = currItem->parent();
	bool updateTissues = false;

	if (get_is_folder(currItem))
	{
		// Delete folder
		if (removeChildren)
		{
			// Delete all children of current item
			QList<QTreeWidgetItem*> children;
			take_children_recursively(currItem, children);
			qDeleteAll(children);

			// Delete current item
			if (currParent == 0)
			{
				takeTopLevelItem(indexOfTopLevelItem(currItem));
				delete currItem;
			}
			else
			{
				currParent->removeChild(currItem);
				delete currItem;
			}

			updateTissues = true;
		}
		else
		{
			// Insert children into parent and delete current item
			if (currParent == 0)
			{
				insertTopLevelItems(indexOfTopLevelItem(currItem), currItem->takeChildren());
				takeTopLevelItem(indexOfTopLevelItem(currItem));
				delete currItem;
			}
			else
			{
				currParent->insertChildren(currParent->indexOfChild(currItem), currItem->takeChildren());
				currParent->removeChild(currItem);
				delete currItem;
			}
		}
	}
	else
	{
		// Delete tissue
		if (currParent == 0)
		{
			takeTopLevelItem(indexOfTopLevelItem(currItem));
			delete currItem;
		}
		else
		{
			currParent->removeChild(currItem);
			delete currItem;
		}

		updateTissues = true;
	}

	// Update internal representation
	if (updateRepresentation)
	{
		update_hierarchy();

		// Propagate tissue removal to other hierarchies
		if (updateTissues)
		{
			hierarchies->update_hierarchies();

			// Update tissue indices
			update_tissue_indices();
		}
	}
}

void TissueTreeWidget::remove_all_folders(bool removeChildren)
{
	// Removes all folders in QTreeWidget and internal representations

	bool updateTissues = false;
	blockSignals(true);
	if (removeChildren)
	{
		for (int i = 0; i < topLevelItemCount(); ++i)
		{
			QTreeWidgetItem* item = topLevelItem(i);
			if (get_is_folder(item))
			{
				// Delete all children of item
				QList<QTreeWidgetItem*> children;
				take_children_recursively(item, children);
				qDeleteAll(children);

				// Delete item
				takeTopLevelItem(i);
				delete item;
				updateTissues = true;
				--i;
			}
		}
	}
	else
	{
		for (int i = 0; i < topLevelItemCount(); ++i)
		{
			QTreeWidgetItem* item = topLevelItem(i);
			if (get_is_folder(item))
			{
				// Insert children into parent and delete item
				insertTopLevelItems(i, item->takeChildren());
				takeTopLevelItem(indexOfTopLevelItem(item));
				delete item;
				--i;
			}
		}
	}
	blockSignals(false);

	// Update internal representation
	update_hierarchy();

	// Propagate tissue removal to other hierarchies
	if (updateTissues)
	{
		hierarchies->update_hierarchies();
	}
}

void TissueTreeWidget::update_tissue_name(const QString& oldName,
		const QString& newName)
{
	if (oldName.compare(newName) == 0)
	{
		return;
	}

	// Update tissue name in internal representations
	auto& hierarchyTrees = hierarchies->hierarchies();
	for (unsigned int i = 0; i < hierarchyTrees.size(); ++i)
	{
		hierarchyTrees[i]->UpdateTissueNameRecursively(oldName, newName);
	}

	// Update tissue name in tree widget
	update_tissue_name_widget(oldName, newName);
}

void TissueTreeWidget::update_tissue_name_widget(const QString& oldName,
		const QString& newName,
		QTreeWidgetItem* parent)
{
	if (parent == 0)
	{
		// Recursion with top level children
		for (int i = 0; i < topLevelItemCount(); ++i)
		{
			update_tissue_name_widget(oldName, newName, topLevelItem(i));
		}
	}
	else
	{
		if (get_is_folder(parent))
		{
			// Recursion with children
			for (int i = 0; i < parent->childCount(); ++i)
			{
				update_tissue_name_widget(oldName, newName, parent->child(i));
			}
		}
		else
		{
			// Update tissue name
			if (parent->text(TISSUETREEWIDGET_COLUMN_NAME).compare(oldName) ==
					0)
			{
				parent->setText(TISSUETREEWIDGET_COLUMN_NAME, newName);
			}
		}
	}
}

void TissueTreeWidget::update_tissue_icons(QTreeWidgetItem* parent)
{
	if (parent == 0)
	{
		// Recursion with top level children
		for (int i = 0; i < topLevelItemCount(); ++i)
		{
			update_tissue_icons(topLevelItem(i));
		}
	}
	else
	{
		if (get_is_folder(parent))
		{
			// Recursion with children
			for (int i = 0; i < parent->childCount(); ++i)
			{
				update_tissue_icons(parent->child(i));
			}
		}
		else
		{
			// Update tissue icon
			parent->setIcon(TISSUETREEWIDGET_COLUMN_NAME,
					generatePixmap(get_type(parent)));
		}
	}
}

void TissueTreeWidget::update_folder_icons(QTreeWidgetItem* parent)
{
	// Updates the folder icons based on the lock state of the child tissues
	// This only works if the tree widget is completely built

	if (parent == 0)
	{
		// Recursion with top level children
		for (int i = 0; i < topLevelItemCount(); ++i)
		{
			update_folder_icons(topLevelItem(i));
		}
	}
	else
	{
		if (get_is_folder(parent))
		{
			// Update folder icon
			short lockStates = get_child_lockstates(parent);
			if (lockStates == 0)
			{
				// All child tissues unlocked
				parent->setIcon(
						TISSUETREEWIDGET_COLUMN_NAME,
						QIcon(picturePath.absFilePath(QString("fileopen.png"))
											.ascii()));
			}
			else if (lockStates == 1)
			{
				// All child tissues locked
				parent->setIcon(
						TISSUETREEWIDGET_COLUMN_NAME,
						QIcon(picturePath.absFilePath(QString("folderlock1.png"))
											.ascii()));
			}
			else
			{
				// Mixed locked/unlocked
				parent->setIcon(
						TISSUETREEWIDGET_COLUMN_NAME,
						QIcon(picturePath.absFilePath(QString("folderlock2.png"))
											.ascii()));
			}

			// Recursion with children
			for (int i = 0; i < parent->childCount(); ++i)
			{
				update_folder_icons(parent->child(i));
			}
		}
	}
}

// TODO: Optimize by introducing folder map / hidden flag
short TissueTreeWidget::get_child_lockstates(QTreeWidgetItem* folder)
{
	// Returns whether all child tissues (including subfolders) are
	// unlocked (return value 0),
	// locked (return value 1), or
	// mixed locked and unlocked (-1)

	// Get lock state of first child (non-empty folder or tissue)
	int i;
	short lockStates = 0; // Empty folders are considered to be unlocked
	for (i = 0; i < folder->childCount(); ++i)
	{
		QTreeWidgetItem* currChild = folder->child(i);
		if (get_is_folder(currChild))
		{
			if (currChild->childCount() > 0)
			{
				lockStates = get_child_lockstates(currChild);
			}
			else
			{
				continue;
			}
		}
		else if (TissueInfos::GetTissueLocked(get_type(currChild)))
		{
			lockStates = 1;
			break;
		}
		else
		{
			lockStates = 0;
			break;
		}
	}
	if (lockStates == -1)
	{
		return -1;
	}

	// Test against lock states of other children
	for (int i = 1; i < folder->childCount(); ++i)
	{
		QTreeWidgetItem* currChild = folder->child(i);
		if (get_is_folder(currChild))
		{
			// Skip empty folders
			if (currChild->childCount() > 0 &&
					(lockStates != get_child_lockstates(currChild)))
			{
				return -1;
			}
		}
		else if ((bool)lockStates !=
						 TissueInfos::GetTissueLocked(get_type(currChild)))
		{
			return -1;
		}
	}
	return lockStates;
}

void TissueTreeWidget::pad_tissue_indices()
{
	// Get maximal number of digits
	unsigned short digits = 0;
	tissues_size_t tmp = (TissueInfos::GetTissueCount() + 1);
	while (tmp > 0)
	{
		tmp = (tissues_size_t)(tmp / 10);
		digits++;
	}

	for (unsigned int i = 0; i < topLevelItemCount(); ++i)
	{
		QTreeWidgetItem* item = topLevelItem(i);
		if (get_is_folder(item))
		{
			pad_tissue_indices_recursively(item, digits);
		}
		else
		{
			// Pad with leading zeroes
			QString paddedText = item->text(TISSUETREEWIDGET_COLUMN_TYPE);
			while (paddedText.length() < digits)
			{
				paddedText = paddedText.prepend('0');
			}
			item->setText(TISSUETREEWIDGET_COLUMN_TYPE, paddedText);
		}
	}
}

void TissueTreeWidget::pad_tissue_indices_recursively(QTreeWidgetItem* parent,
		unsigned short digits)
{
	for (unsigned int i = 0; i < parent->childCount(); ++i)
	{
		QTreeWidgetItem* item = parent->child(i);
		if (get_is_folder(item))
		{
			pad_tissue_indices_recursively(item, digits);
		}
		else
		{
			// Pad with leading zeroes
			QString paddedText = item->text(TISSUETREEWIDGET_COLUMN_TYPE);
			while (paddedText.length() < digits)
			{
				paddedText = paddedText.prepend('0');
			}
			item->setText(TISSUETREEWIDGET_COLUMN_TYPE, paddedText);
		}
	}
}

void TissueTreeWidget::update_tissue_indices()
{
	for (unsigned int i = 0; i < topLevelItemCount(); ++i)
	{
		QTreeWidgetItem* item = topLevelItem(i);
		if (get_is_folder(item))
		{
			update_tissue_indices_recursively(item);
		}
		else
		{
			tissues_size_t type = TissueInfos::GetTissueType(ToStd(item->text(TISSUETREEWIDGET_COLUMN_NAME)));
			item->setText(TISSUETREEWIDGET_COLUMN_TYPE, QString::number(type));
		}
	}
}

void TissueTreeWidget::update_tissue_indices_recursively(
		QTreeWidgetItem* parent)
{
	for (unsigned int i = 0; i < parent->childCount(); ++i)
	{
		QTreeWidgetItem* item = parent->child(i);
		if (get_is_folder(item))
		{
			update_tissue_indices_recursively(item);
		}
		else
		{
			tissues_size_t type = TissueInfos::GetTissueType(ToStd(item->text(TISSUETREEWIDGET_COLUMN_NAME)));
			item->setText(TISSUETREEWIDGET_COLUMN_TYPE, QString::number(type));
		}
	}
}

void TissueTreeWidget::set_current_folder_name(const QString& name)
{
	if (get_current_is_folder())
	{
		currentItem()->setText(TISSUETREEWIDGET_COLUMN_NAME, name);

		// Update internal representation
		update_hierarchy();
	}
}

void TissueTreeWidget::set_current_item(QTreeWidgetItem* item)
{
	if (item)
	{
		setCurrentItem(item);
	}
	else
	{
		setCurrentItem(topLevelItem(0));
	}
}

void TissueTreeWidget::set_current_tissue(tissues_size_t type)
{
	if (type > 0 && type <= TissueInfos::GetTissueCount())
	{
		QTreeWidgetItem* item = find_tissue_item(type);
		setCurrentItem(item);
		QTreeWidgetItem* parent = item->parent();
		if (parent != 0)
		{
			parent->setExpanded(true);
		}
	}
	else if (topLevelItemCount() > 0 && type > 0)
	{
		setCurrentItem(topLevelItem(0));
	}
}

QTreeWidgetItem* TissueTreeWidget::find_tissue_item(tissues_size_t type, QTreeWidgetItem* parent) const
{
	if (parent == 0)
	{
		// Recursion with top level children
		for (int i = 0; i < topLevelItemCount(); ++i)
		{
			QTreeWidgetItem* recursiveRes = find_tissue_item(type, topLevelItem(i));
			if (recursiveRes != 0)
			{
				return recursiveRes;
			}
		}
	}
	else
	{
		// Check if parent is searched item
		if (type == get_type(parent))
		{
			return parent;
		}
		// Recursion with children
		for (int i = 0; i < parent->childCount(); ++i)
		{
			QTreeWidgetItem* recursiveRes = find_tissue_item(type, parent->child(i));
			if (recursiveRes != 0)
			{
				return recursiveRes;
			}
		}
	}
	return (QTreeWidgetItem*)0;
}

tissues_size_t TissueTreeWidget::get_current_type() const
{
	return get_type(currentItem());
}

QString TissueTreeWidget::get_current_name() const { return get_name(currentItem()); }

bool TissueTreeWidget::get_current_is_folder() const
{
	return get_is_folder(currentItem());
}

bool TissueTreeWidget::get_is_folder(QTreeWidgetItem* item) const
{
	if (item)
	{
		return item->text(TISSUETREEWIDGET_COLUMN_FOLDER).toUShort();
	}
	return true;
}

tissues_size_t TissueTreeWidget::get_type(QTreeWidgetItem* item) const
{
	if (item && !get_is_folder(item))
	{
		return (tissues_size_t)item->text(TISSUETREEWIDGET_COLUMN_TYPE)
				.toUInt();
	}
	return 0;
}

QString TissueTreeWidget::get_name(QTreeWidgetItem* item) const
{
	if (item)
	{
		return item->text(TISSUETREEWIDGET_COLUMN_NAME);
	}
	return "";
}

bool TissueTreeWidget::get_current_has_children() const
{
	if (currentItem() == 0)
	{
		return false;
	}
	else
	{
		return currentItem()->childCount() > 0;
	}
}

void TissueTreeWidget::get_current_child_tissues(std::map<tissues_size_t, unsigned short>& types) const
{
	types.clear();
	if (currentItem() != 0)
	{
		get_child_tissues_recursively(currentItem(), types);
	}
}

void TissueTreeWidget::get_sublevel_child_tissues(std::map<tissues_size_t, unsigned short>& types) const
{
	types.clear();
	for (unsigned int i = 0; i < topLevelItemCount(); ++i)
	{
		QTreeWidgetItem* item = topLevelItem(i);
		if (get_is_folder(item))
		{
			get_child_tissues_recursively(item, types);
		}
	}
}

void TissueTreeWidget::get_child_tissues_recursively(QTreeWidgetItem* parent, std::map<tissues_size_t, unsigned short>& types) const
{
	for (int i = 0; i < parent->childCount(); ++i)
	{
		tissues_size_t currType = (tissues_size_t)parent->child(i)
																	->text(TISSUETREEWIDGET_COLUMN_TYPE)
																	.toUInt();
		if (currType > 0)
		{
			// Insert tissue
			if (types.find(currType) != types.end())
			{
				types[currType]++;
			}
			else
			{
				types[currType] = 1;
			}
		}
		else
		{
			// Get tissues in subfolder
			get_child_tissues_recursively(parent->child(i), types);
		}
	}
}

unsigned short TissueTreeWidget::get_selected_hierarchy() const
{
	return hierarchies->get_selected_hierarchy();
}

unsigned short TissueTreeWidget::get_hierarchy_count() const
{
	return hierarchies->get_hierarchy_count();
}

std::vector<QString>* TissueTreeWidget::get_hierarchy_names_ptr() const
{
	return hierarchies->get_hierarchy_names_ptr();
}

QString TissueTreeWidget::get_current_hierarchy_name() const
{
	return hierarchies->get_current_hierarchy_name();
}

void TissueTreeWidget::reset_default_hierarchy()
{
	hierarchies->reset_default_hierarchy();
}

void TissueTreeWidget::set_hierarchy(unsigned short index)
{
	if (!hierarchies->set_hierarchy(index))
	{
		return;
	}

	tissues_size_t currentType = get_current_type();

	// Build QTreeWidgetItems from TissueTreeItems
	blockSignals(true);
	build_tree_widget(hierarchies->selected_hierarchy());
	blockSignals(false);

	// Restore item selection
	if (currentType > 0)
	{
		set_current_tissue(currentType);
	}
	else
	{
		setCurrentItem(topLevelItem(0));
	}

	// Resize columns
	resize_columns_to_contents();
}

void TissueTreeWidget::build_tree_widget(TissueHierarchyItem* root)
{
	clear();

	// Create pool of existing tissues
	std::set<tissues_size_t> tissueTypes;
	for (tissues_size_t type = 1; type <= TissueInfos::GetTissueCount(); ++type)
	{
		tissueTypes.insert(type);
	}

	vector<TissueHierarchyItem*>* children = root->GetChildren();
	for (vector<TissueHierarchyItem*>::iterator iter = children->begin();
			 iter != children->end(); ++iter)
	{
		// Add top-level item
		TissueHierarchyItem* currItem = *iter;
		if (currItem->GetIsFolder())
		{
			QTreeWidgetItem* newFolder =
					create_hierarchy_item(true, currItem->GetName());
			addTopLevelItem(newFolder);
			build_tree_widget_recursively(currItem, newFolder, &tissueTypes);
		}
		else
		{
			tissues_size_t type = TissueInfos::GetTissueType(ToStd(currItem->GetName()));
			if (type > 0)
			{
				QTreeWidgetItem* newTissue =
						create_hierarchy_item(false, currItem->GetName());
				addTopLevelItem(newTissue);
				std::set<tissues_size_t>::iterator iter =
						tissueTypes.find(type);
				if (iter != tissueTypes.end())
				{
					tissueTypes.erase(iter);
				}
			}
		}
	}

	// Add all missing tissues at top level
	for (std::set<tissues_size_t>::iterator iter = tissueTypes.begin();
			 iter != tissueTypes.end(); ++iter)
	{
		addTopLevelItem(create_hierarchy_item(false, ToQ(TissueInfos::GetTissueName(*iter))));
	}

	// Update folder icons
	update_folder_icons();

	modified = false;
}

void TissueTreeWidget::build_tree_widget_recursively(
		TissueHierarchyItem* parentIn, QTreeWidgetItem* parentOut,
		std::set<tissues_size_t>* tissueTypes)
{
	vector<TissueHierarchyItem*>* children = parentIn->GetChildren();
	for (vector<TissueHierarchyItem*>::iterator iter = children->begin();
			 iter != children->end(); ++iter)
	{
		// Add item to parentOut
		TissueHierarchyItem* currItem = *iter;
		if (currItem->GetIsFolder())
		{
			QTreeWidgetItem* newFolder =
					create_hierarchy_item(true, currItem->GetName());
			parentOut->addChild(newFolder);
			build_tree_widget_recursively(currItem, newFolder, tissueTypes);
		}
		else
		{
			tissues_size_t type = TissueInfos::GetTissueType(ToStd(currItem->GetName()));
			if (type > 0)
			{
				QTreeWidgetItem* newTissue = create_hierarchy_item(false, currItem->GetName());
				parentOut->addChild(newTissue);
				std::set<tissues_size_t>::iterator iter = tissueTypes->find(type);
				if (iter != tissueTypes->end())
				{
					tissueTypes->erase(iter);
				}
			}
		}
	}
}

void TissueTreeWidget::add_new_hierarchy(const QString& name)
{
	// Create and select default hierarchy
	hierarchies->add_new_hierarchy(name);

	emit hierarchy_list_changed();
}

bool TissueTreeWidget::load_hierarchy(const QString& path)
{
	if (!hierarchies->load_hierarchy(path))
	{
		return false;
	}

	emit hierarchy_list_changed();

	return true;
}

void TissueTreeWidget::take_children_recursively(
		QTreeWidgetItem* parent, QList<QTreeWidgetItem*>& appendTo)
{
	// Recursion with children
	for (int i = 0; i < parent->childCount(); ++i)
	{
		take_children_recursively(parent->child(i), appendTo);
	}
	// Append children to list
	appendTo.append(parent->takeChildren());
}

void TissueTreeWidget::update_tree_widget()
{
	reset_default_hierarchy();
	set_hierarchy(hierarchies->get_selected_hierarchy());
}

bool TissueTreeWidget::save_hierarchy_as(const QString& name,
		const QString& path)
{
	if (hierarchies->get_selected_hierarchy() == 0)
	{
		// Do not override default hierarchy: Append to list
		hierarchies->add_new_hierarchy(name, create_current_hierarchy());

		// Reset default hierarchy
		hierarchies->reset_default_hierarchy();
		emit hierarchy_list_changed();
	}
	else
	{
		// Update hierarchy name and internal representation
		hierarchies->selected_hierarchy_name() = name;
		emit hierarchy_list_changed();
		update_hierarchy();
	}

	modified = false;

	return hierarchies->save_hierarchy_as(name, path);
}

void TissueTreeWidget::remove_current_hierarchy()
{
	if (hierarchies->remove_current_hierarchy())
	{
		modified = true;
	}

	// \bug it seems the folders are not refreshed properly
	set_hierarchy(hierarchies->get_selected_hierarchy());

	emit hierarchy_list_changed();
}

void TissueTreeWidget::toggle_show_tissue_indices()
{
	setColumnHidden(TISSUETREEWIDGET_COLUMN_TYPE,
			!isColumnHidden(TISSUETREEWIDGET_COLUMN_TYPE));
	resize_columns_to_contents();
}

void TissueTreeWidget::sort_by_tissue_name()
{
	if (sortByNameAscending)
	{
		sortItems(TISSUETREEWIDGET_COLUMN_NAME, Qt::AscendingOrder);
	}
	else
	{
		sortItems(TISSUETREEWIDGET_COLUMN_NAME, Qt::DescendingOrder);
	}
	sortByNameAscending = !sortByNameAscending;

	// Update internal representation
	update_hierarchy();
}

void TissueTreeWidget::sort_by_tissue_index()
{
	// Pad tissue indices with zeroes because of lexical sorting
	pad_tissue_indices();

	if (sortByTypeAscending)
	{
		sortItems(TISSUETREEWIDGET_COLUMN_TYPE, Qt::AscendingOrder);
	}
	else
	{
		sortItems(TISSUETREEWIDGET_COLUMN_TYPE, Qt::DescendingOrder);
	}
	sortByTypeAscending = !sortByTypeAscending;

	// Update tissue indices to undo padding
	update_tissue_indices();

	// Update internal representation
	update_hierarchy();
}

bool TissueTreeWidget::get_tissue_indices_hidden() const
{
	return isColumnHidden(TISSUETREEWIDGET_COLUMN_TYPE);
}

std::vector<QTreeWidgetItem*> TissueTreeWidget::collect(const std::vector<QTreeWidgetItem*>& list) const
{
	std::vector<QTreeWidgetItem*> my_children;

	for (auto item: list)
	{
		if (item != invisibleRootItem())
		{
			my_children.push_back(item);
		}

		for (int i = 0; i < item->childCount(); ++i)
		{
			auto extra = collect({item->child(i)});
			my_children.insert(my_children.end(), extra.begin(), extra.end());
		}
	}

	return my_children;
}

std::vector<QTreeWidgetItem*> TissueTreeWidget::get_all_items() const
{
	return collect({invisibleRootItem()});
}

void TissueTreeWidget::resize_columns_to_contents()
{
	for (int col = 0; col < TISSUETREEWIDGET_COLUMN_COUNT; ++col)
	{
		resizeColumnToContents(col);
	}
}

void TissueTreeWidget::resize_columns_to_contents(QTreeWidgetItem* item)
{
	resize_columns_to_contents();
}

bool TissueTreeWidget::get_hierarchy_modified() const { return modified; }

void TissueTreeWidget::set_hierarchy_modified(bool val) { modified = val; }

unsigned short TissueTreeWidget::get_tissue_instance_count(tissues_size_t type) const
{
	// Get number of items in tissue tree corresponding to the same tissue type
	unsigned short res = 0;
	for (unsigned int i = 0; i < topLevelItemCount(); ++i)
	{
		QTreeWidgetItem* item = topLevelItem(i);
		if (get_is_folder(item))
		{
			res += get_tissue_instance_count_recursively(item, type);
		}
		else if (get_type(item) == type)
		{
			res++;
		}
	}
	return res;
}

unsigned short TissueTreeWidget::get_tissue_instance_count_recursively(
		QTreeWidgetItem* parent, tissues_size_t type) const
{
	unsigned short res = 0;
	for (unsigned int i = 0; i < parent->childCount(); ++i)
	{
		QTreeWidgetItem* item = parent->child(i);
		if (get_is_folder(item))
		{
			res += get_tissue_instance_count_recursively(item, type);
		}
		else if (get_type(item) == type)
		{
			res++;
		}
	}
	return res;
}

FILE* TissueTreeWidget::SaveParams(FILE* fp, int version)
{
	return hierarchies->SaveParams(fp, version);
}

FILE* TissueTreeWidget::LoadParams(FILE* fp, int version)
{
	fp = hierarchies->LoadParams(fp, version);

	// rebuild of tree view
	set_hierarchy(hierarchies->get_selected_hierarchy());

	emit hierarchy_list_changed();

	return fp;
}

FILE* TissueTreeWidget::save_hierarchy(FILE* fp, unsigned short idx)
{
	return hierarchies->save_hierarchy(fp, idx);
}

FILE* TissueTreeWidget::load_hierarchy(FILE* fp)
{
	fp = hierarchies->load_hierarchy(fp);

	// Update folder icons
	update_folder_icons();

	// rebuild of tree view
	set_hierarchy(hierarchies->get_selected_hierarchy());

	emit hierarchy_list_changed();

	return fp;
}

void TissueTreeWidget::dropEvent(QDropEvent* de)
{
	// Only accept internal move actions
	if (!(de->source() == this && (de->possibleActions() & Qt::MoveAction)))
	{
		return;
	}

	if (!get_current_is_folder() &&
			(de->keyboardModifiers() & Qt::ShiftModifier))
	{
		// Move the item and insert a duplicate at the original position
		QTreeWidgetItem* currItem = currentItem();
		QTreeWidgetItem* currParent = currItem->parent();

		// Get original index
		unsigned int oldIdx = 0;
		if (currParent == 0)
		{
			oldIdx = indexOfTopLevelItem(currItem);
		}
		else
		{
			oldIdx = currParent->indexOfChild(currItem);
		}

		// Move item
		QTreeWidget::dropEvent(de);
		setCurrentItem(currItem);

		// Get new index
		unsigned int newIdx = 0;
		if (currParent == 0)
		{
			newIdx = indexOfTopLevelItem(currItem);
		}
		else
		{
			newIdx = currParent->indexOfChild(currItem);
		}

		// Insert copy
		if (newIdx < oldIdx)
		{
			oldIdx++;
		}
		insert_item(false, get_current_name(), currParent, oldIdx);
	}
	else
	{
		// Move the item
		QTreeWidget::dropEvent(de);
	}

	// Update internal representation
	update_hierarchy();

	// Update folder icons
	update_folder_icons();
}

void TissueTreeWidget::selectAll()
{
	for (auto item : get_all_items())
	{
		if (!get_is_folder(item))
		{
			item->setSelected(!item->isHidden());
		}
	}
}

void TissueTreeWidget::scrollToItem(QTreeWidgetItem* item)
{
	if (item)
	{
		scrollTo(indexFromItem(item));
	}
}

TissueAdder::TissueAdder(bool modifyTissue, TissueTreeWidget* tissueTree,
		QWidget* parent, const char* name,
		Qt::WindowFlags wFlags)
		: QDialog(parent, name, TRUE, wFlags), tissueTreeWidget(tissueTree)
{
	modify = modifyTissue;

	vbox1 = new Q3VBoxLayout(this);
	hbox1 = new Q3HBoxLayout(this);
	vbox1->addLayout(hbox1);
	tissuename = new QLabel(QString("Tissue Name: "), this);
	nameField = new QLineEdit(this);
	hbox1->addWidget(tissuename);
	hbox1->addWidget(nameField);
	hbox2 = new Q3HBoxLayout(this);
	cs = new colorshower(50, 50, this);
	hbox2->addWidget(cs);
	vbox2 = new Q3VBoxLayout(this);
	vbox3 = new Q3VBoxLayout(this);
	vbox4 = new Q3VBoxLayout(this);

	r = new QSlider(Qt::Horizontal, this);
	r->setRange(0, 255);
	red = new QLabel(QString("Red: "), this);
	//	hboxr=new QHBoxLayout(this);
	//	hboxr->addWidget(red);
	//	hboxr->addWidget(r);
	g = new QSlider(Qt::Horizontal, this);
	g->setRange(0, 255);
	green = new QLabel(QString("Green: "), this);
	//	hboxg=new QHBoxLayout(this);
	//	hboxg->addWidget(green);
	//	hboxg->addWidget(g);
	b = new QSlider(Qt::Horizontal, this);
	b->setRange(0, 255);
	blue = new QLabel(QString("Blue: "), this);
	sl_transp = new QSlider(Qt::Horizontal, this);
	sl_transp->setRange(0, 100);
	opac = new QLabel(QString("Transp.: "), this);
	sb_r = new QSpinBox(0, 255, 1, this);
	sb_g = new QSpinBox(0, 255, 1, this);
	sb_b = new QSpinBox(0, 255, 1, this);
	sb_transp = new QSpinBox(0, 100, 1, this);

	//	hboxb=new QHBoxLayout(this);
	//	hboxb->addWidget(blue);
	//	hboxb->addWidget(b);
	/*	vbox2->addLayout(hboxr);
			vbox2->addLayout(hboxg);
			vbox2->addLayout(hboxb);*/
	vbox2->addWidget(red);
	vbox2->addWidget(green);
	vbox2->addWidget(blue);
	vbox2->addWidget(opac);
	vbox3->addWidget(r);
	vbox3->addWidget(g);
	vbox3->addWidget(b);
	vbox3->addWidget(sl_transp);
	vbox4->addWidget(sb_r);
	vbox4->addWidget(sb_g);
	vbox4->addWidget(sb_b);
	vbox4->addWidget(sb_transp);
	hbox2->addLayout(vbox2);
	hbox2->addLayout(vbox3);
	hbox2->addLayout(vbox4);
	vbox1->addLayout(hbox2);
	hbox3 = new Q3HBoxLayout(this);

	addTissue = new QPushButton("", this);
	closeButton = new QPushButton("Close", this);
	hbox3->addWidget(addTissue);
	hbox3->addWidget(closeButton);
	vbox1->addLayout(hbox3);

	if (modify)
	{
		addTissue->setText("Modify Tissue");

		TissueInfoStruct* tissueInfo = TissueInfos::GetTissueInfo(tissueTreeWidget->get_current_type());
		nameField->setText(ToQ(tissueInfo->name));
		r->setValue(int(tissueInfo->color[0] * 255));
		g->setValue(int(tissueInfo->color[1] * 255));
		b->setValue(int(tissueInfo->color[2] * 255));
		sb_r->setValue(int(tissueInfo->color[0] * 255));
		sb_g->setValue(int(tissueInfo->color[1] * 255));
		sb_b->setValue(int(tissueInfo->color[2] * 255));
		sl_transp->setValue(int(100 - tissueInfo->opac * 100));
		sb_transp->setValue(int(100 - tissueInfo->opac * 100));

		fr1 = float(r->value()) / 255;
		fg1 = float(g->value()) / 255;
		fb1 = float(b->value()) / 255;
		transp1 = float(sl_transp->value()) / 100;

		QObject::connect(r, SIGNAL(valueChanged(int)), this,
				SLOT(update_color_r(int)));
		QObject::connect(g, SIGNAL(valueChanged(int)), this,
				SLOT(update_color_g(int)));
		QObject::connect(b, SIGNAL(valueChanged(int)), this,
				SLOT(update_color_b(int)));
		QObject::connect(sl_transp, SIGNAL(valueChanged(int)), this,
				SLOT(update_opac(int)));
		QObject::connect(sb_r, SIGNAL(valueChanged(int)), this,
				SLOT(update_color_rsb(int)));
		QObject::connect(sb_g, SIGNAL(valueChanged(int)), this,
				SLOT(update_color_gsb(int)));
		QObject::connect(sb_b, SIGNAL(valueChanged(int)), this,
				SLOT(update_color_bsb(int)));
		QObject::connect(sb_transp, SIGNAL(valueChanged(int)), this,
				SLOT(update_opacsb(int)));
		QObject::connect(addTissue, SIGNAL(clicked()), this,
				SLOT(add_pressed()));
		QObject::connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));
		QObject::connect(this,
				SIGNAL(color_changed(float, float, float, float)), cs,
				SLOT(color_changed(float, float, float, float)));

		emit color_changed(fr1, fg1, fb1, 1.0 - transp1);
	}
	else
	{
		addTissue->setText("Add Tissue");

		r->setValue(0);
		g->setValue(0);
		b->setValue(0);
		sb_r->setValue(0);
		sb_g->setValue(0);
		sb_b->setValue(0);
		sl_transp->setValue(50);
		sb_transp->setValue(50);

		fr1 = float(r->value()) / 255;
		fg1 = float(g->value()) / 255;
		fb1 = float(b->value()) / 255;
		transp1 = float(sl_transp->value()) / 100;

		QObject::connect(r, SIGNAL(valueChanged(int)), this,
				SLOT(update_color_r(int)));
		QObject::connect(g, SIGNAL(valueChanged(int)), this,
				SLOT(update_color_g(int)));
		QObject::connect(b, SIGNAL(valueChanged(int)), this,
				SLOT(update_color_b(int)));
		QObject::connect(sl_transp, SIGNAL(valueChanged(int)), this,
				SLOT(update_opac(int)));
		QObject::connect(addTissue, SIGNAL(clicked()), this,
				SLOT(add_pressed()));
		QObject::connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));
		QObject::connect(this,
				SIGNAL(color_changed(float, float, float, float)), cs,
				SLOT(color_changed(float, float, float, float)));
	}

	return;
}

TissueAdder::~TissueAdder()
{
	delete cs;
	delete vbox1;
	delete closeButton;
	delete addTissue;
	delete red;
	delete green;
	delete blue;
	delete opac;
	delete tissuename;
	delete nameField;
	delete r;
	delete g;
	delete b;
	delete sl_transp;
	delete sb_r;
	delete sb_g;
	delete sb_b;
	delete sb_transp;
}

void TissueAdder::update_color_r(int v)
{
	QObject::disconnect(sb_r, SIGNAL(valueChanged(int)), this,
			SLOT(update_color_rsb(int)));
	sb_r->setValue(v);
	QObject::connect(sb_r, SIGNAL(valueChanged(int)), this,
			SLOT(update_color_rsb(int)));
	fr1 = float(v) / 255;
	emit color_changed(fr1, fg1, fb1, 1.0f - transp1);
}

void TissueAdder::update_color_g(int v)
{
	QObject::disconnect(sb_g, SIGNAL(valueChanged(int)), this,
			SLOT(update_color_gsb(int)));
	sb_g->setValue(v);
	QObject::connect(sb_g, SIGNAL(valueChanged(int)), this,
			SLOT(update_color_gsb(int)));
	fg1 = float(v) / 255;
	emit color_changed(fr1, fg1, fb1, 1.0f - transp1);
}

void TissueAdder::update_color_b(int v)
{
	QObject::disconnect(sb_b, SIGNAL(valueChanged(int)), this,
			SLOT(update_color_bsb(int)));
	sb_b->setValue(v);
	QObject::connect(sb_b, SIGNAL(valueChanged(int)), this,
			SLOT(update_color_bsb(int)));
	fb1 = float(v) / 255;
	emit color_changed(fr1, fg1, fb1, 1.0f - transp1);
}

void TissueAdder::update_opac(int v)
{
	QObject::disconnect(sb_transp, SIGNAL(valueChanged(int)), this,
			SLOT(update_opacsb(int)));
	sb_transp->setValue(v);
	QObject::connect(sb_transp, SIGNAL(valueChanged(int)), this,
			SLOT(update_opacsb(int)));
	transp1 = float(v) / 100;
	emit color_changed(fr1, fg1, fb1, 1.0f - transp1);
}

void TissueAdder::update_color_rsb(int v)
{
	QObject::disconnect(r, SIGNAL(valueChanged(int)), this,
			SLOT(update_color_r(int)));
	r->setValue(v);
	QObject::connect(r, SIGNAL(valueChanged(int)), this,
			SLOT(update_color_r(int)));
	fr1 = float(v) / 255;
	emit color_changed(fr1, fg1, fb1, 1.0f - transp1);
}

void TissueAdder::update_color_gsb(int v)
{
	QObject::disconnect(g, SIGNAL(valueChanged(int)), this,
			SLOT(update_color_g(int)));
	g->setValue(v);
	QObject::connect(g, SIGNAL(valueChanged(int)), this,
			SLOT(update_color_g(int)));
	fg1 = float(v) / 255;
	emit color_changed(fr1, fg1, fb1, 1.0f - transp1);
}

void TissueAdder::update_color_bsb(int v)
{
	QObject::disconnect(b, SIGNAL(valueChanged(int)), this,
			SLOT(update_color_b(int)));
	b->setValue(v);
	QObject::connect(b, SIGNAL(valueChanged(int)), this,
			SLOT(update_color_b(int)));
	fb1 = float(v) / 255;
	emit color_changed(fr1, fg1, fb1, 1.0f - transp1);
}

void TissueAdder::update_opacsb(int v)
{
	QObject::disconnect(sl_transp, SIGNAL(valueChanged(int)), this,
			SLOT(update_opac(int)));
	sl_transp->setValue(v);
	QObject::connect(sl_transp, SIGNAL(valueChanged(int)), this,
			SLOT(update_opac(int)));
	transp1 = float(v) / 100;
	emit color_changed(fr1, fg1, fb1, 1.0f - transp1);
}

void TissueAdder::add_pressed()
{
	if (modify)
	{
		if (!nameField->text().isEmpty())
		{
			tissues_size_t type = tissueTreeWidget->get_current_type();
			TissueInfoStruct* tissueInfo = TissueInfos::GetTissueInfo(type);
			QString oldName = ToQ(tissueInfo->name);
			if (oldName.compare(nameField->text(), Qt::CaseInsensitive) != 0 &&
					TissueInfos::GetTissueType(ToStd(nameField->text())) > 0)
			{
				QMessageBox::information(
						this, "iSeg",
						"A tissue with the same name already exists.");
				return;
			}
			TissueInfos::SetTissueName(type, ToStd(nameField->text()));
			tissueInfo->opac = 1.0f - transp1;
			tissueInfo->color[0] = fr1;
			tissueInfo->color[1] = fg1;
			tissueInfo->color[2] = fb1;
			// Update tissue name and icon in hierarchy
			tissueTreeWidget->update_tissue_name(oldName, ToQ(tissueInfo->name));
			tissueTreeWidget->update_tissue_icons();
			close();
		}
		return;
	}
	else
	{
		if (TissueInfos::GetTissueCount() >= TISSUES_SIZE_MAX)
		{
			close();
		}
		else if (!nameField->text().isEmpty())
		{
			if (TissueInfos::GetTissueType(ToStd(nameField->text())) > 0)
			{
				QMessageBox::information(
						this, "iSeg",
						"A tissue with the same name already exists.");
				return;
			}
			TissueInfoStruct tissueInfo;
			tissueInfo.name = ToStd(nameField->text());
			tissueInfo.locked = false;
			tissueInfo.opac = 1.0f - transp1;
			tissueInfo.color[0] = fr1;
			tissueInfo.color[1] = fg1;
			tissueInfo.color[2] = fb1;
			TissueInfos::AddTissue(tissueInfo);
			// Insert new tissue in hierarchy
			tissueTreeWidget->insert_item(false, ToQ(tissueInfo.name));
			close();
		}
	}

	//	fprintf(fp,"%f %f %f",tissuecolor[1][0],tissuecolor[1][1],tissuecolor[1][2]);
	//	fclose(fp);

	return;
}

TissueFolderAdder::TissueFolderAdder(TissueTreeWidget* tissueTree,
		QWidget* parent, const char* name,
		Qt::WindowFlags wFlags)
		: QDialog(parent, name, TRUE, wFlags), tissueTreeWidget(tissueTree)
{
	setFixedWidth(235);
	setFixedHeight(161);

	hboxOverall = new Q3HBoxLayout(this);
	vboxOverall = new Q3VBoxLayout(this);
	hboxFolderName = new Q3HBoxLayout(this);
	hboxPushButtons = new Q3HBoxLayout(this);
	hboxOverall->addLayout(vboxOverall);

	// Folder name line edit
	nameLabel = new QLabel(QString("Folder Name: "), this);
	nameLineEdit = new QLineEdit(this);
	nameLineEdit->setText("New folder");
	hboxFolderName->addWidget(nameLabel);
	hboxFolderName->addWidget(nameLineEdit);
	vboxOverall->addLayout(hboxFolderName);

	// Buttons
	addButton = new QPushButton("Add", this);
	closeButton = new QPushButton("Close", this);
	hboxPushButtons->addWidget(addButton);
	hboxPushButtons->addWidget(closeButton);
	vboxOverall->addLayout(hboxPushButtons);

	QObject::connect(addButton, SIGNAL(clicked()), this, SLOT(add_pressed()));
	QObject::connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));
}

TissueFolderAdder::~TissueFolderAdder() { delete hboxOverall; }

void TissueFolderAdder::add_pressed()
{
	tissueTreeWidget->insert_item(true, nameLineEdit->text());
	close();
}

TissueHierarchyWidget::TissueHierarchyWidget(TissueTreeWidget* tissueTree,
		QWidget* parent,
		Qt::WindowFlags wFlags)
		: QWidget(parent, wFlags), tissueTreeWidget(tissueTree)
{
	hboxOverall = new Q3HBoxLayout(this);
	vboxOverall = new Q3VBoxLayout();
	vboxHierarchyButtons = new Q3VBoxLayout();
	hboxOverall->addLayout(vboxOverall);

	// Hierarchy selection combo box
	hierarchyComboBox = new QComboBox(this);
	vboxOverall->addWidget(hierarchyComboBox);

	// Hierarchy buttons
	newHierarchyButton = new QPushButton("New Hierarchy...", this);
	loadHierarchyButton = new QPushButton("Load Hierarchy...", this);
	saveHierarchyAsButton = new QPushButton("Save Hierarchy As...", this);
	removeHierarchyButton = new QPushButton("Remove Hierarchy", this);
	vboxHierarchyButtons->addWidget(newHierarchyButton);
	vboxHierarchyButtons->addWidget(loadHierarchyButton);
	vboxHierarchyButtons->addWidget(saveHierarchyAsButton);
	vboxHierarchyButtons->addWidget(removeHierarchyButton);
	vboxOverall->addLayout(vboxHierarchyButtons);

	QObject::connect(hierarchyComboBox, SIGNAL(currentIndexChanged(int)), this,
			SLOT(hierarchy_changed(int)));

	QObject::connect(newHierarchyButton, SIGNAL(clicked()), this,
			SLOT(new_hierarchy_pressed()));
	QObject::connect(loadHierarchyButton, SIGNAL(clicked()), this,
			SLOT(load_hierarchy_pressed()));
	QObject::connect(saveHierarchyAsButton, SIGNAL(clicked()), this,
			SLOT(save_hierarchy_as_pressed()));
	QObject::connect(removeHierarchyButton, SIGNAL(clicked()), this,
			SLOT(remove_hierarchy_pressed()));

	QObject::connect(tissueTreeWidget, SIGNAL(hierarchy_list_changed()), this,
			SLOT(update_hierarchy_combo_box()));

	update_hierarchy_combo_box();

	setFixedHeight(hboxOverall->sizeHint().height());
}

void TissueHierarchyWidget::update_hierarchy_combo_box()
{
	hierarchyComboBox->blockSignals(true);
	hierarchyComboBox->clear();
	auto hierarchyNames = tissueTreeWidget->get_hierarchy_names_ptr();
	for (auto name : *hierarchyNames)
	{
		hierarchyComboBox->addItem(name);
	}
	hierarchyComboBox->setCurrentItem(
			tissueTreeWidget->get_selected_hierarchy());
	hierarchyComboBox->blockSignals(false);
}

TissueHierarchyWidget::~TissueHierarchyWidget() { delete hboxOverall; }

bool TissueHierarchyWidget::handle_changed_hierarchy()
{
#if 0 // Version: Ask user whether to save xml
	if (tissueTreeWidget->get_hierarchy_modified())
	{
		int ret = QMessageBox::warning(this, "iSeg",
			QString("Do you want to save changes to hierarchy %1?").arg(tissueTreeWidget->get_current_hierarchy_name()),
			QMessageBox::Yes | QMessageBox::Default, QMessageBox::No, QMessageBox::Cancel | QMessageBox::Escape);
		if (ret == QMessageBox::Yes)
		{
			// Save hierarchy as...
			save_hierarchy_as_pressed();
		}
		else if (ret == QMessageBox::Cancel)
		{
			// Update internal representation of current hierarchy
			tissueTreeWidget->update_hierarchy();
			return false;
		}
		tissueTreeWidget->set_hierarchy_modified(false);
	}
	return true;
#else // Version: Commit change, but only save to xml if default hierarchy changed
	if (tissueTreeWidget->get_selected_hierarchy() == 0 &&
			tissueTreeWidget->get_hierarchy_modified())
	{
		int ret = QMessageBox::warning(
				this, "iSeg",
				QString("Do you want to save changes to hierarchy %1?")
						.arg(tissueTreeWidget->get_current_hierarchy_name()),
				QMessageBox::Yes | QMessageBox::Default, QMessageBox::No,
				QMessageBox::Cancel | QMessageBox::Escape);
		if (ret == QMessageBox::Yes)
		{
			// Save hierarchy as...
			if (!save_hierarchy_as_pressed())
			{
				return false;
			}
		}
		else if (ret == QMessageBox::Cancel)
		{
			// Update internal representation of current hierarchy
			tissueTreeWidget->update_hierarchy();
			return false;
		}
		else
		{
			// Reset default hierarchy
			tissueTreeWidget->reset_default_hierarchy();
		}
		tissueTreeWidget->update_tree_widget();
	}
	else
	{
		// Update internal representation of current hierarchy
		tissueTreeWidget->update_hierarchy();
		tissueTreeWidget->set_hierarchy_modified(false);
	}
	return true;
#endif
}

void TissueHierarchyWidget::hierarchy_changed(int index)
{
	if ((int)tissueTreeWidget->get_selected_hierarchy() == index)
	{
		return;
	}

	// Save changes to current hierarchy
	if (!handle_changed_hierarchy())
	{
		// Select default hierarchy
		hierarchyComboBox->blockSignals(true);
		hierarchyComboBox->setCurrentItem(0);
		hierarchyComboBox->blockSignals(false);
		return;
	}

	// Set selected hierarchy
	tissueTreeWidget->set_hierarchy(index);

	// Set selected item in combo box (may have changed during save hierarchy)
	hierarchyComboBox->blockSignals(true);
	hierarchyComboBox->setCurrentItem(index);
	hierarchyComboBox->blockSignals(false);
}

void TissueHierarchyWidget::new_hierarchy_pressed()
{
	// Save changes to current hierarchy
	if (!handle_changed_hierarchy())
	{
		return;
	}

	// Get hierarchy name
	bool ok = false;
	QString newHierarchyName = QInputDialog::getText(
			"Hierarchy name", "Enter a name for the hierarchy:", QLineEdit::Normal,
			"New Hierarchy", &ok, this);
	if (!ok)
	{
		return;
	}

	// Create new hierarchy
	tissueTreeWidget->add_new_hierarchy(newHierarchyName);
}

void TissueHierarchyWidget::load_hierarchy_pressed()
{
	// Save changes to current hierarchy
	if (!handle_changed_hierarchy())
	{
		return;
	}

	// Get file name
	QString fileName = QFileDialog::getOpenFileName(this, tr("Load File"), "",
			tr("XML files (*.xml)"));
	if (fileName.isNull())
	{
		return;
	}

	// Load hierarchy
	tissueTreeWidget->load_hierarchy(fileName);

	// Set loaded hierarchy selected
	tissueTreeWidget->set_hierarchy(tissueTreeWidget->get_hierarchy_count() -
																	1);
	update_hierarchy_combo_box();
}

bool TissueHierarchyWidget::save_hierarchy_as_pressed()
{
	// Get file name
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), "",
			tr("XML files (*.xml)"));
	if (fileName.isNull())
	{
		return false;
	}

	// Get hierarchy name
	bool ok = false;
	QString newHierarchyName = QInputDialog::getText(
			"Hierarchy name", "Enter a name for the hierarchy:", QLineEdit::Normal,
			tissueTreeWidget->get_current_hierarchy_name(), &ok, this);
	if (!ok)
	{
		return false;
	}

	// Save hierarchy
	return tissueTreeWidget->save_hierarchy_as(newHierarchyName, fileName);
}

void TissueHierarchyWidget::remove_hierarchy_pressed()
{
	// Remove current hierarchy
	tissueTreeWidget->remove_current_hierarchy();
}

bits_stack::bits_stack(SlicesHandler* hand3D, QWidget* parent, const char* name,
		Qt::WindowFlags wFlags)
		: QWidget(parent, name, wFlags), handler3D(hand3D)
{
	bits_names = new QListWidget(this);
	hbox1 = new Q3HBoxLayout(this);
	hbox1->addWidget(bits_names);
	vbox1 = new Q3VBoxLayout(hbox1);
	pushwork = new QPushButton("Copy Target...", this);
	pushbmp = new QPushButton("Copy Source...", this);
	pushtissue = new QPushButton("Copy Tissue...", this);
	popwork = new QPushButton("Paste Target", this);
	popbmp = new QPushButton("Paste Source", this);
	poptissue = new QPushButton("Paste Tissue", this);
	deletebtn = new QPushButton("Delete", this);
	saveitem = new QPushButton("Save Item(s)", this);
	loaditem = new QPushButton("Open Item(s)", this);
	vbox1->addWidget(pushwork);
	vbox1->addWidget(pushbmp);
	vbox1->addWidget(pushtissue);
	vbox1->addWidget(popwork);
	vbox1->addWidget(popbmp);
	vbox1->addWidget(poptissue);
	vbox1->addWidget(deletebtn);
	vbox1->addWidget(saveitem);
	vbox1->addWidget(loaditem);

	bits_names->setSelectionMode(QAbstractItemView::ExtendedSelection);
	bits_names->setDragEnabled(true);
	bits_names->setDragDropMode(QAbstractItemView::InternalMove);
	bits_names->viewport()->setAcceptDrops(true);
	bits_names->setDropIndicatorShown(true);

	QObject::connect(pushwork, SIGNAL(clicked()), this,
			SLOT(pushwork_pressed()));
	QObject::connect(pushbmp, SIGNAL(clicked()), this, SLOT(pushbmp_pressed()));
	QObject::connect(pushtissue, SIGNAL(clicked()), this,
			SLOT(pushtissue_pressed()));
	QObject::connect(popwork, SIGNAL(clicked()), this, SLOT(popwork_pressed()));
	QObject::connect(popbmp, SIGNAL(clicked()), this, SLOT(popbmp_pressed()));
	QObject::connect(poptissue, SIGNAL(clicked()), this,
			SLOT(poptissue_pressed()));
	QObject::connect(deletebtn, SIGNAL(clicked()), this,
			SLOT(delete_pressed()));
	QObject::connect(saveitem, SIGNAL(clicked()), this,
			SLOT(saveitem_pressed()));
	QObject::connect(loaditem, SIGNAL(clicked()), this,
			SLOT(loaditem_pressed()));

	oldw = handler3D->width();
	oldh = handler3D->height();

	tissuenr = 0;
}

bits_stack::~bits_stack()
{
	delete bits_names;
	delete pushwork;
	delete pushbmp;
	delete pushtissue;
	delete popwork;
	delete popbmp;
	delete poptissue;
	delete deletebtn;
	delete saveitem;
	delete loaditem;
}

void bits_stack::tissuenr_changed(unsigned short i) { tissuenr = i + 1; }

void bits_stack::push_helper(bool source, bool target, bool tissue)
{
	if ((short)source + (short)target + (short)tissue != 1)
	{
		return;
	}

	QString dataName("");
	if (source)
	{
		dataName = QString("Source");
	}
	else if (target)
	{
		dataName = QString("Target");
	}
	else if (tissue)
	{
		dataName = QString("Tissue");
	}

	iseg::DataSelection dataSelection;
	dataSelection.bmp = source;
	dataSelection.work = target;
	dataSelection.tissues = tissue;
	emit begin_dataexport(dataSelection, this);

	bits_stack_pushdialog pushdialog(this, QString("Copy ") + dataName +
																						 QString("..."));
	if (pushdialog.exec() == QDialog::Rejected)
	{
		emit end_dataexport(this);
		return;
	}

	if (pushdialog.get_pushcurrentslice())
	{
		// Copy current slice
		bool ok;
		QString newText = QInputDialog::getText(
				"Name", "Enter a name for the picture:", QLineEdit::Normal, "", &ok,
				this);
		newText = newText + QString(" (") +
							QString::number(handler3D->active_slice() + 1) +
							QString(")");
		while (ok &&
					 bits_names->findItems(newText, Qt::MatchExactly).size() > 0)
		{
			newText = QInputDialog::getText(
					"Name",
					"Enter a !new! name for the picture:", QLineEdit::Normal, "",
					&ok, this);
			newText = newText + QString(" (") +
								QString::number(handler3D->active_slice() + 1) +
								QString(")");
		}
		if (ok)
		{
			unsigned dummy;
			if (source)
			{
				dummy = handler3D->pushstack_bmp();
			}
			else if (target)
			{
				dummy = handler3D->pushstack_work();
			}
			else if (tissue)
			{
				dummy = handler3D->pushstack_tissue(tissuenr);
			}
			bits_nr[newText] = dummy;
			bits_names->addItem(newText);
			emit stack_changed();
		}
	}
	else
	{
		// Copy slice range
		bool ok, matchfound, startok, endok;
		unsigned int startslice = pushdialog.get_startslice(&startok);
		unsigned int endslice = pushdialog.get_endslice(&endok);
		while (!startok || !endok || startslice > endslice || startslice < 1 ||
					 endslice > handler3D->num_slices())
		{
			QMessageBox::information(
					this, QString("Copy ") + dataName + QString("..."),
					"Please enter a valid slice range.\n");
			if (pushdialog.exec() == QDialog::Rejected)
			{
				emit end_dataexport(this);
				return;
			}
			startslice = pushdialog.get_startslice(&startok);
			endslice = pushdialog.get_endslice(&endok);
		}

		matchfound = true;
		QString newText = QInputDialog::getText(
				"Name", "Enter a name for the pictures:", QLineEdit::Normal, "",
				&ok, this);
		while (ok && matchfound)
		{
			matchfound = false;
			for (unsigned int slice = startslice; slice <= endslice; ++slice)
			{
				QString newTextExt = newText + QString(" (") +
														 QString::number(slice) + QString(")");
				if (bits_names->findItems(newTextExt, Qt::MatchExactly).size() >
						0)
				{
					matchfound = true;
					newText = QInputDialog::getText(
							"Name", "Enter a !new! name for the pictures:",
							QLineEdit::Normal, "", &ok, this);
					break;
				}
			}
		}
		if (ok)
		{
			for (unsigned int slice = startslice; slice <= endslice; ++slice)
			{
				QString newTextExt = newText + QString(" (") +
														 QString::number(slice) + QString(")");
				unsigned dummy;
				if (source)
				{
					dummy = handler3D->pushstack_bmp(slice - 1);
				}
				else if (target)
				{
					dummy = handler3D->pushstack_work(slice - 1);
				}
				else if (tissue)
				{
					dummy = handler3D->pushstack_tissue(tissuenr, slice - 1);
				}
				bits_nr[newTextExt] = dummy;
				bits_names->addItem(newTextExt);
			}
			emit stack_changed();
		}
	}

	emit end_dataexport(this);
}

void bits_stack::pushwork_pressed() { push_helper(false, true, false); }

void bits_stack::pushtissue_pressed() { push_helper(false, false, true); }

void bits_stack::pushbmp_pressed() { push_helper(true, false, false); }

void bits_stack::pop_helper(bool source, bool target, bool tissue)
{
	if ((short)source + (short)target + (short)tissue != 1)
	{
		return;
	}

	QString dataName("");
	if (source)
	{
		dataName = QString("Source");
	}
	else if (target)
	{
		dataName = QString("Target");
	}
	else if (tissue)
	{
		dataName = QString("Tissue");
	}

	QList<QListWidgetItem*> selectedItems = bits_names->selectedItems();
	if (selectedItems.size() <= 0)
	{
		return;
	}
	else if (handler3D->active_slice() + selectedItems.size() >
					 handler3D->num_slices())
	{
		QMessageBox::information(
				this, QString("Paste ") + dataName,
				"The number of images to be pasted starting at the\ncurrent slice "
				"would surpass the end of the data stack.\n");
		return;
	}

	iseg::DataSelection dataSelection;
	dataSelection.sliceNr = handler3D->active_slice();
	dataSelection.bmp = source;
	dataSelection.work = target;
	dataSelection.tissues = tissue;
	dataSelection.allSlices = selectedItems.size() > 1;
	emit begin_datachange(dataSelection, this);

	unsigned int slice = handler3D->active_slice();
	if (source)
	{
		for (QList<QListWidgetItem*>::iterator iter = selectedItems.begin();
				 iter != selectedItems.end(); ++iter)
		{
			handler3D->getstack_bmp(slice++, bits_nr[(*iter)->text()]);
		}
	}
	else if (target)
	{
		for (QList<QListWidgetItem*>::iterator iter = selectedItems.begin();
				 iter != selectedItems.end(); ++iter)
		{
			handler3D->getstack_work(slice++, bits_nr[(*iter)->text()]);
		}
	}
	else if (tissue)
	{
		for (QList<QListWidgetItem*>::iterator iter = selectedItems.begin();
				 iter != selectedItems.end(); ++iter)
		{
			handler3D->getstack_tissue(slice++, bits_nr[(*iter)->text()],
					tissuenr, true);
		}
	}

	emit end_datachange(this);
}

void bits_stack::popwork_pressed() { pop_helper(false, true, false); }

void bits_stack::popbmp_pressed() { pop_helper(true, false, false); }

void bits_stack::poptissue_pressed() { pop_helper(false, false, true); }

void bits_stack::loaditem_pressed()
{
	QStringList selectedFiles = QFileDialog::getOpenFileNames("Stackitems (*.stk)\nAll(*.*)",
			QString::null, this, "open file dialog", "Select on or more files to open");
	if (selectedFiles.isEmpty())
	{
		return;
	}

	if (selectedFiles.size() > 1)
	{
		// Load multiple items
		bool ok;
		bool matchfound = true;
		QString newText = QInputDialog::getText(
				"Name", "Enter a name for the pictures:", QLineEdit::Normal, "",
				&ok, this);
		while (ok && matchfound)
		{
			matchfound = false;
			unsigned int suffix = 0;
			for (QStringList::Iterator iter = selectedFiles.begin();
					 iter != selectedFiles.end(); ++iter)
			{
				QString newTextExt = newText + QString::number(suffix++);
				if (bits_names->findItems(newTextExt, Qt::MatchExactly).size() >
						0)
				{
					matchfound = true;
					newText = QInputDialog::getText(
							"Name", "Enter a !new! name for the pictures:",
							QLineEdit::Normal, "", &ok, this);
					break;
				}
			}
		}

		if (ok)
		{
			unsigned int suffix = 0;
			for (QStringList::Iterator iter = selectedFiles.begin();
					 iter != selectedFiles.end(); ++iter)
			{
				QString newTextExt = newText + QString::number(suffix++);
				unsigned dummy = handler3D->loadstack(iter->ascii());
				if (dummy != 123456)
				{
					bits_nr[newTextExt] = dummy;
					bits_names->addItem(newTextExt);
					emit stack_changed();
				}
			}
		}
	}
	else if (selectedFiles.size() > 0)
	{
		// Load single item
		bool ok;
		QString newText = QInputDialog::getText(
				"Name", "Enter a name for the picture:", QLineEdit::Normal, "", &ok,
				this);
		while (ok &&
					 bits_names->findItems(newText, Qt::MatchExactly).size() > 0)
		{
			newText = QInputDialog::getText(
					"Name",
					"Enter a !new! name for the picture:", QLineEdit::Normal, "",
					&ok, this);
		}

		if (ok)
		{
			unsigned dummy;
			dummy = handler3D->loadstack(selectedFiles[0].ascii());
			if (dummy != 123456)
			{
				bits_nr[newText] = dummy;
				bits_names->addItem(newText);
				emit stack_changed();
			}
		}
	}
}

void bits_stack::saveitem_pressed()
{
	QList<QListWidgetItem*> selectedItems = bits_names->selectedItems();
	if (selectedItems.size() <= 0)
	{
		return;
	}

	QString savefilename = QFileDialog::getSaveFileName(
			QString::null, "Stackitems (*.stk)\n", this); //, filename);
	if (savefilename.endsWith(QString(".stk")))
	{
		savefilename.remove(savefilename.length() - 4, 4);
	}
	if (savefilename.isEmpty())
	{
		return;
	}

	if (selectedItems.size() > 1)
	{
		unsigned int fieldWidth = 0;
		int tmp = selectedItems.size();
		while (tmp > 0)
		{
			tmp /= 10;
			fieldWidth++;
		}
		unsigned int suffix = 1;
		for (QList<QListWidgetItem*>::iterator iter = selectedItems.begin();
				 iter != selectedItems.end(); ++iter)
		{
			QString savefilenameExt =
					savefilename +
					QString("%1").arg(suffix++, fieldWidth, 10, QChar('0')) +
					QString(".stk");
			handler3D->savestack(bits_nr[(*iter)->text()],
					savefilenameExt.ascii());
		}
	}
	else
	{
		QString savefilenameExt = savefilename + QString(".stk");
		handler3D->savestack(bits_nr[selectedItems[0]->text()],
				savefilenameExt.ascii());
	}
}

void bits_stack::delete_pressed()
{
	QList<QListWidgetItem*> selectedItems = bits_names->selectedItems();
	if (selectedItems.size() <= 0)
	{
		return;
	}

	for (QList<QListWidgetItem*>::iterator iter = selectedItems.begin();
			 iter != selectedItems.end(); ++iter)
	{
		handler3D->removestack(bits_nr[(*iter)->text()]);
		bits_nr.erase((*iter)->text());
		delete bits_names->takeItem(bits_names->row(*iter));
		emit stack_changed();
	}
}

QMap<QString, unsigned int>* bits_stack::return_bitsnr() { return &bits_nr; }

void bits_stack::newloaded()
{
	if (oldw != handler3D->width() || oldh != handler3D->height())
	{
		bits_names->clear();
		bits_nr.clear();
	}
	oldw = handler3D->width();
	oldh = handler3D->height();
	emit stack_changed();
}

void bits_stack::clear_stack()
{
	handler3D->clear_stack();
	bits_names->clear();
	bits_nr.clear();
	emit stack_changed();
}

FILE* bits_stack::save_proj(FILE* fp)
{
	int size = int(bits_nr.size());
	fwrite(&size, 1, sizeof(int), fp);

	for (int j = 0; j < bits_names->count(); ++j)
	{
		QString itemName = bits_names->item(j)->text();
		int size1 = itemName.length();
		fwrite(&size1, 1, sizeof(int), fp);
		fwrite(itemName.ascii(), 1, sizeof(char) * size1, fp);
		unsigned int stackIdx = bits_nr[itemName];
		fwrite(&stackIdx, 1, sizeof(unsigned), fp);
	}

	return fp;
}

FILE* bits_stack::load_proj(FILE* fp)
{
	oldw = handler3D->width();
	oldh = handler3D->height();

	char name[100];
	int size;
	fread(&size, sizeof(int), 1, fp);

	bits_nr.clear();
	bits_names->clear();

	for (int j = 0; j < size; j++)
	{
		int size1;
		unsigned dummy;
		fread(&size1, sizeof(int), 1, fp);
		fread(name, sizeof(char) * size1, 1, fp);
		name[size1] = '\0';
		QString s(name);
		bits_names->addItem(s);

		fread(&dummy, sizeof(unsigned), 1, fp);
		bits_nr[s] = dummy;
	}
	emit stack_changed();

	return fp;
}

extoverlay_widget::extoverlay_widget(SlicesHandler* hand3D, QWidget* parent,
		const char* name, Qt::WindowFlags wFlags)
		: QWidget(parent, name, wFlags), handler3D(hand3D)
{
	activeslice = handler3D->active_slice();
	bmphand = handler3D->get_activebmphandler();

	alpha = 0.0f;
	sliderMax = 1000;

	sliderPrecision = 0;
	int tmp = sliderMax;
	while (tmp > 1)
	{
		tmp /= 10;
		sliderPrecision++;
	}

	hboxOverall = new Q3HBoxLayout(this);
	vboxOverall = new Q3VBoxLayout();
	hboxAlpha = new Q3HBoxLayout();
	hboxDisplaySrcTgt = new Q3HBoxLayout();
	hboxOverall->addLayout(vboxOverall);

	// Dataset selection combo box
	datasetComboBox = new QComboBox(this);
	vboxOverall->addWidget(datasetComboBox);

	// Dataset buttons
	loadDatasetButton = new QPushButton("Load Dataset...", this);
	vboxOverall->addWidget(loadDatasetButton);

	// Alpha value slider
	lbAlpha = new QLabel(QString("Alpha: "));
	slAlpha = new QSlider(Qt::Horizontal);
	slAlpha->setRange(0, sliderMax);
	slAlpha->setValue(alpha * sliderMax);
	leAlpha = new QLineEdit(QString::number((int)0));
	leAlpha->setAlignment(Qt::AlignRight);
	leAlpha->setText(QString::number(alpha, 'f', sliderPrecision));
	QString text = leAlpha->text();
	QFontMetrics fm = leAlpha->fontMetrics();
	QRect rect = fm.boundingRect(text);
	leAlpha->setFixedSize(rect.width() + 10, rect.height() + 4);
	hboxAlpha->addWidget(lbAlpha);
	hboxAlpha->addWidget(slAlpha);
	hboxAlpha->addWidget(leAlpha);
	vboxOverall->addLayout(hboxAlpha);

	// Display checkboxes
	srcCheckBox = new QCheckBox(QString("Source"));
	tgtCheckBox = new QCheckBox(QString("Target"));
	hboxDisplaySrcTgt->addWidget(srcCheckBox);
	hboxDisplaySrcTgt->addWidget(tgtCheckBox);
	vboxOverall->addLayout(hboxDisplaySrcTgt);

	setFixedHeight(hboxOverall->sizeHint().height());

	QObject::connect(datasetComboBox, SIGNAL(currentIndexChanged(int)), this,
			SLOT(dataset_changed(int)));
	QObject::connect(loadDatasetButton, SIGNAL(clicked()), this,
			SLOT(load_dataset_pressed()));
	QObject::connect(leAlpha, SIGNAL(editingFinished()), this,
			SLOT(alpha_changed()));
	QObject::connect(slAlpha, SIGNAL(sliderMoved(int)), this,
			SLOT(slider_changed(int)));
	QObject::connect(srcCheckBox, SIGNAL(clicked()), this,
			SLOT(source_toggled()));
	QObject::connect(tgtCheckBox, SIGNAL(clicked()), this,
			SLOT(target_toggled()));

	initialize();
}

void extoverlay_widget::initialize()
{
	datasetComboBox->blockSignals(true);
	datasetComboBox->clear();
	datasetComboBox->blockSignals(false);

	datasetNames.clear();
	datasetFilepaths.clear();
	add_dataset("(None)");
	selectedDataset = 0;

	alpha = 0.0f;
	leAlpha->setText(QString::number(alpha, 'f', sliderPrecision));
	slAlpha->setValue(alpha * sliderMax);
	emit overlayalpha_changed(alpha);
}

void extoverlay_widget::newloaded() { handler3D->new_overlay(); }

void extoverlay_widget::add_dataset(const QString& path)
{
	datasetNames.push_back(QFileInfo(path).fileName());
	datasetFilepaths.push_back(path);

	datasetComboBox->blockSignals(true);
	datasetComboBox->addItem(*datasetNames.rbegin());
	datasetComboBox->blockSignals(false);

	datasetComboBox->setCurrentIndex((datasetNames.size() - 1));
}

void extoverlay_widget::remove_dataset(unsigned short idx)
{
	datasetNames.erase(datasetNames.begin() + idx);
	datasetFilepaths.erase(datasetFilepaths.begin() + idx);

	datasetComboBox->blockSignals(true);
	datasetComboBox->removeItem(idx);
	datasetComboBox->blockSignals(false);

	datasetComboBox->setCurrentIndex(0);
}

void extoverlay_widget::reload_overlay()
{
	bool ok = true;
	if (selectedDataset == 0 && (srcCheckBox->isOn() || tgtCheckBox->isOn()))
	{
		handler3D->clear_overlay();
	}
	else if (datasetFilepaths[selectedDataset].endsWith(QString(".raw"),
							 Qt::CaseInsensitive))
	{
		ok = handler3D->ReadRawOverlay(datasetFilepaths[selectedDataset], 8,
				handler3D->active_slice());
	}
	else if (datasetFilepaths[selectedDataset].endsWith(QString(".vtk"),
							 Qt::CaseInsensitive) ||
					 datasetFilepaths[selectedDataset].endsWith(QString(".vti"),
							 Qt::CaseInsensitive))
	{
		ok = handler3D->ReadOverlay(datasetFilepaths[selectedDataset],
				handler3D->active_slice());
	}
	else if (datasetFilepaths[selectedDataset].endsWith(QString(".nii"),
							 Qt::CaseInsensitive) ||
					 datasetFilepaths[selectedDataset].endsWith(QString(".hdr"),
							 Qt::CaseInsensitive) ||
					 datasetFilepaths[selectedDataset].endsWith(QString(".img"),
							 Qt::CaseInsensitive) ||
					 datasetFilepaths[selectedDataset].endsWith(QString(".nii.gz"),
							 Qt::CaseInsensitive))
	{
		ok = handler3D->ReadOverlay(datasetFilepaths[selectedDataset],
				handler3D->active_slice());
	}

	if (!ok)
	{
		handler3D->clear_overlay();
	}

	emit overlay_changed();
}

void extoverlay_widget::slicenr_changed() { reload_overlay(); }

void extoverlay_widget::dataset_changed(int index)
{
	if (index < 0 || index >= datasetNames.size())
	{
		return;
	}
	selectedDataset = index;
	reload_overlay();
}

extoverlay_widget::~extoverlay_widget()
{
	datasetNames.clear();
	datasetFilepaths.clear();
	delete hboxOverall;
}

void extoverlay_widget::load_dataset_pressed()
{
	QString loadfilename =
			QFileDialog::getOpenFileName(QString::null,
					"VTK (*.vti *.vtk)\n"
					"Raw files (*.raw)\n"
					"NIFTI (*.nii *.hdr *.img *.nii.gz)\n"
					"All(*.*)",
					this); // TODO: Support other file types
	if (!loadfilename.isEmpty())
	{
		add_dataset(loadfilename);
	}
}

void extoverlay_widget::alpha_changed()
{
	bool b1;
	float value = leAlpha->text().toFloat(&b1);
	if (!b1)
	{
		leAlpha->setText(QString::number(alpha, 'f', sliderPrecision));
		slAlpha->setValue(alpha * sliderMax);
		QApplication::beep();
		return;
	}

	if (value < 0.0f)
	{
		value = 0.0f;
	}
	else if (value > 1.0f)
	{
		value = 1.0f;
	}
	alpha = value;
	leAlpha->setText(QString::number(alpha, 'f', sliderPrecision));
	slAlpha->setValue(alpha * sliderMax);
	emit overlayalpha_changed(alpha);
}

void extoverlay_widget::slider_changed(int newval)
{
	alpha = newval / (float)sliderMax;
	leAlpha->setText(QString::number(alpha, 'f', sliderPrecision));
	emit overlayalpha_changed(alpha);
}

void extoverlay_widget::source_toggled()
{
	bool isset = srcCheckBox->isOn();
	emit bmpoverlayvisible_changed(isset);
}

void extoverlay_widget::target_toggled()
{
	bool isset = tgtCheckBox->isOn();
	emit workoverlayvisible_changed(isset);
}

MultiDataset_widget::MultiDataset_widget(SlicesHandler* hand3D, QWidget* parent,
		const char* name,
		Qt::WindowFlags wFlags)
		: QWidget(parent, name, wFlags), m_Handler3D(hand3D)
{
	hboxOverall = new Q3HBoxLayout(this);
	vboxOverall = new Q3VBoxLayout();
	hboxOverall->addLayout(vboxOverall);

	// Add dataset button
	m_AddDatasetButton = new QPushButton("Add Dataset...", this);
	vboxOverall->addWidget(m_AddDatasetButton);

	// Dataset selection group box
	m_DatasetsGroupBox = new QGroupBox("- Available datasets -");
	m_VboxDatasets = new Q3VBoxLayout(m_DatasetsGroupBox);
	m_VboxDatasets->addStretch(1);
	m_DatasetsGroupBox->setLayout(m_VboxDatasets);
	vboxOverall->addWidget(m_DatasetsGroupBox);

	//Buttons
	QHBoxLayout* buttonsGrid = new QHBoxLayout();
	vboxOverall->addLayout(buttonsGrid);

	// Add dataset button
	m_LoadDatasetButton = new QPushButton("Load Dataset", this);
	buttonsGrid->addWidget(m_LoadDatasetButton);

	m_ChangeNameButton = new QPushButton("Change Name", this);
	buttonsGrid->addWidget(m_ChangeNameButton);

	m_RemoveDatasetButton = new QPushButton("Remove Dataset", this);
	buttonsGrid->addWidget(m_RemoveDatasetButton);
	m_RemoveDatasetButton->setEnabled(false);

	m_DatasetsGroupBox->setMinimumHeight(200);
	setFixedHeight(hboxOverall->sizeHint().height());

	QObject::connect(m_AddDatasetButton, SIGNAL(clicked()), this,
			SLOT(AddDatasetPressed()));
	QObject::connect(m_LoadDatasetButton, SIGNAL(clicked()), this,
			SLOT(SwitchDataset()));
	QObject::connect(m_ChangeNameButton, SIGNAL(clicked()), this,
			SLOT(ChangeDatasetName()));
	QObject::connect(m_RemoveDatasetButton, SIGNAL(clicked()), this,
			SLOT(RemoveDataset()));
	QObject::connect(m_DatasetsGroupBox, SIGNAL(clicked()), this,
			SLOT(DatasetSelectionChanged()));

	Initialize();
	InitializeMap();
}

MultiDataset_widget::~MultiDataset_widget() { m_RadioButtons.clear(); }

void MultiDataset_widget::InitializeMap()
{
	m_MapDatasetValues["DCM"] = DatasetTypeEnum::DCM;
	m_MapDatasetValues["DICOM"] = DatasetTypeEnum::DCM;
	m_MapDatasetValues["BMP"] = DatasetTypeEnum::BMP;
	m_MapDatasetValues["PNG"] = DatasetTypeEnum::PNG;
	m_MapDatasetValues["RAW"] = DatasetTypeEnum::RAW;
	m_MapDatasetValues["MHD"] = DatasetTypeEnum::MHD;
	m_MapDatasetValues["AVW"] = DatasetTypeEnum::AVW;
	m_MapDatasetValues["VTI"] = DatasetTypeEnum::VTK;
	m_MapDatasetValues["VTK"] = DatasetTypeEnum::VTK;
	m_MapDatasetValues["XMF"] = DatasetTypeEnum::XDMF;
	m_MapDatasetValues["NII"] = DatasetTypeEnum::NIFTI;
	m_MapDatasetValues["HDR"] = DatasetTypeEnum::NIFTI;
	m_MapDatasetValues["IMG"] = DatasetTypeEnum::NIFTI;
	m_MapDatasetValues["NIA"] = DatasetTypeEnum::NIFTI;
	m_MapDatasetValues["RTDOSE"] = DatasetTypeEnum::RTDOSE;
}

void MultiDataset_widget::Initialize()
{
	m_ItIsBeingLoaded = false;
	m_RadioButtons.clear();
}

void MultiDataset_widget::ClearRadioButtons()
{
	if (m_VboxDatasets->layout() != nullptr)
	{
		QLayoutItem* item;
		while ((item = m_VboxDatasets->layout()->takeAt(0)) != nullptr)
		{
			delete item->widget();
			delete item;
		}
	}

	m_VboxDatasets->update();
	m_VboxDatasets->layout()->update();

	m_RadioButtons.clear();
}

void MultiDataset_widget::NewLoaded()
{
	if (m_ItIsBeingLoaded)
	{
		m_ItIsBeingLoaded = false;
		return;
	}

	ClearRadioButtons();

	Initialize();

	const unsigned short w_loaded = m_Handler3D->width();
	const unsigned short h_loaded = m_Handler3D->height();
	const unsigned short nrofslices_loaded = m_Handler3D->num_slices();

	const bool checkMatch = true;
	if (checkMatch)
	{
		if (w_loaded == 512 && h_loaded == 512 && nrofslices_loaded == 1)
		{
			// empty default image
			return;
		}
	}

	SDatasetInfo newRadioButton;
	// Create the copy of the main dataset only when adding a second dataset
	//CopyImagesSlices(m_Handler3D, newRadioButton);

	QStringList paths;
	paths.append("main_Dataset");
	AddDatasetToList(newRadioButton, paths);

	QFont font(newRadioButton.m_RadioButton->font());
	font.setBold(true);
	newRadioButton.m_RadioButton->setFont(font);
	newRadioButton.m_RadioButton->setChecked(true);
	newRadioButton.m_IsActive = true;

	m_RadioButtons.push_back(newRadioButton);
}

void MultiDataset_widget::AddDatasetPressed()
{
	SupportedMultiDatasetTypes dlg;
	dlg.exec();

	int selectedType = dlg.GetSelectedType();

	if (selectedType != -1)
	{
		SlicesHandler* handler3D = new SlicesHandler();
		QStringList loadfilenames;
		unsigned short width, height, nrofslices;
		MultiDataset_widget::SDatasetInfo dataInfo;
		dataInfo.m_IsActive = false;
		bool success = false;

		switch (SupportedMultiDatasetTypes::supportedTypes(selectedType))
		{
		case SupportedMultiDatasetTypes::supportedTypes::bmp:
		{
			loadfilenames = QFileDialog::getOpenFileNames("Images (*.bmp)\nAll(*.*)",
					QString::null, this, "open files dialog",
					"Select one or more files to open");

			if (!loadfilenames.empty())
			{
				sort(loadfilenames.begin(), loadfilenames.end());
				short nrelem = loadfilenames.size();

				vector<const char*> vfilenames;
				for (short i = 0; i < nrelem; i++)
				{
					vfilenames.push_back(loadfilenames[i].ascii());
				}

				LoaderColorImages LB(handler3D, LoaderColorImages::kBMP, vfilenames, this);
				LB.move(QCursor::pos());
				LB.exec();

				width = handler3D->width();
				height = handler3D->height();
				nrofslices = handler3D->num_slices();

				success = CheckInfoAndAddToList(dataInfo, loadfilenames, width,
						height, nrofslices);
			}
		}
		break;

		case SupportedMultiDatasetTypes::supportedTypes::dcm:
		{
			loadfilenames = QFileDialog::getOpenFileNames(
					"Images (*.dcm *.dicom)\n"
					"All(*.*)",
					QString::null, this, "open files dialog",
					"Select one or more files to open");

			if (!loadfilenames.empty())
			{
				LoaderDicom LD(handler3D, &loadfilenames, false, this);
				LD.move(QCursor::pos());
				LD.exec();
			}

			width = handler3D->width();
			height = handler3D->height();
			nrofslices = handler3D->num_slices();

			success = CheckInfoAndAddToList(dataInfo, loadfilenames, width,
					height, nrofslices);
		}
		break;

		case SupportedMultiDatasetTypes::supportedTypes::raw:
		{
			LoaderRaw LR(handler3D, this);
			LR.move(QCursor::pos());
			LR.exec();

			QString loadfilename = LR.GetFileName();
			loadfilenames.append(loadfilename);
			width = handler3D->width();
			height = handler3D->height();
			nrofslices = handler3D->num_slices();

			success = CheckInfoAndAddToList(dataInfo, loadfilenames, width,
					height, nrofslices);
		}
		break;

		case SupportedMultiDatasetTypes::supportedTypes::nifti:
		case SupportedMultiDatasetTypes::supportedTypes::vtk:
		{
			bool res = true;
			QString loadfilename = QFileDialog::getOpenFileName(
					QString::null,
					"VTK (*.vti *.vtk)\n"
					"NIFTI (*.nii *.hdr *.img *.nia)\n"
					"All(*.*)",
					this);
			if (!loadfilename.isEmpty())
			{
				if (handler3D->ReadImage(loadfilename.ascii()))
				{
					loadfilenames.append(loadfilename);
					width = handler3D->width();
					height = handler3D->height();
					nrofslices = handler3D->num_slices();

					success = CheckInfoAndAddToList(dataInfo, loadfilenames,
							width, height, nrofslices);
				}
			}
		}
		break;

		default: break;
		}

		if (success)
		{
			// Create the copy of the main dataset only when adding a second dataset
			if (m_RadioButtons.size() == 1)
			{
				CopyImagesSlices(m_Handler3D, m_RadioButtons.at(0));
			}

			CopyImagesSlices(handler3D, dataInfo);
			m_RadioButtons.push_back(dataInfo);

			delete handler3D;		 //freed memory
			handler3D = nullptr; //pointed dangling ptr to nullptr
		}
	}
}

bool MultiDataset_widget::CheckInfoAndAddToList(
		MultiDataset_widget::SDatasetInfo& newRadioButton,
		QStringList loadfilenames, unsigned short width, unsigned short height,
		unsigned short nrofslices)
{
	// check whether the new dataset matches the dataset loaded
	const unsigned short w_loaded = m_Handler3D->width();
	const unsigned short h_loaded = m_Handler3D->height();
	const unsigned short nrofslices_loaded = m_Handler3D->num_slices();

	if (w_loaded == 512 && h_loaded == 512 && nrofslices_loaded == 1)
	{
		QMessageBox mb(
				"Loading failed", "The main dataset must be loaded first.",
				QMessageBox::Critical, QMessageBox::Ok | QMessageBox::Default,
				QMessageBox::NoButton, QMessageBox::NoButton);
		mb.exec();
	}

	if (w_loaded == width && h_loaded == height &&
			nrofslices_loaded == nrofslices)
	{
		return AddDatasetToList(newRadioButton, loadfilenames);
	}
	else
	{
		QMessageBox mb(
				"Loading failed",
				"The resolution of the dataset must match the one loaded.",
				QMessageBox::Critical, QMessageBox::Ok | QMessageBox::Default,
				QMessageBox::NoButton, QMessageBox::NoButton);
		mb.exec();
	}

	return false;
}

bool MultiDataset_widget::AddDatasetToList(
		MultiDataset_widget::SDatasetInfo& newRadioButton,
		QStringList loadfilenames)
{
	QString butText = loadfilenames[0];
	newRadioButton.m_RadioButton =
			new QRadioButton(QFileInfo(loadfilenames[0]).fileName());
	newRadioButton.m_DatasetFilepath = loadfilenames;
	newRadioButton.m_RadioButtonText = butText;

	m_VboxDatasets->addWidget(newRadioButton.m_RadioButton, 0, Qt::AlignTop);

	QObject::connect(newRadioButton.m_RadioButton, SIGNAL(clicked()), this,
			SLOT(DatasetSelectionChanged()));

	return !newRadioButton.m_DatasetFilepath.empty();
}

void MultiDataset_widget::CopyImagesSlices(
		SlicesHandler* handler3D, MultiDataset_widget::SDatasetInfo& newRadioButton,
		const bool saveOnlyWorkingBits /*= false*/)
{
	const int nrslices = handler3D->num_slices();
	const int width = handler3D->width();
	const int height = handler3D->height();
	const int size = width * height;

	newRadioButton.m_Width = width;
	newRadioButton.m_Height = height;

	if (!saveOnlyWorkingBits)
	{
		newRadioButton.m_BmpSlices.clear();
		for (int i = 0; i < nrslices; i++)
		{
			float* bmp_data = (float*)malloc(sizeof(float) * size);
			memcpy(bmp_data, handler3D->return_bmp(i), sizeof(float) * size);
			newRadioButton.m_BmpSlices.push_back(bmp_data);
		}
	}

	newRadioButton.m_WorkSlices.clear();
	for (int i = 0; i < nrslices; i++)
	{
		float* work_data = (float*)malloc(sizeof(float) * size);
		memcpy(work_data, handler3D->return_work(i), sizeof(float) * size);
		newRadioButton.m_WorkSlices.push_back(work_data);
	}
}

void MultiDataset_widget::SwitchDataset()
{
	for (auto& radioButton : m_RadioButtons)
	{
		if (radioButton.m_RadioButton->isChecked())
		{
			QFont font(radioButton.m_RadioButton->font());
			font.setBold(true);
			radioButton.m_RadioButton->setFont(font);

			if (!radioButton.m_BmpSlices.empty())
			{
				iseg::DataSelection dataSelection;
				dataSelection.allSlices = true;
				dataSelection.bmp = true;
				dataSelection.work = true;
				dataSelection.tissues = false;
				emit begin_datachange(dataSelection, this, false);

				float size = radioButton.m_Width * radioButton.m_Height;
				const int nSlices = radioButton.m_BmpSlices.size();
				assert(radioButton.m_BmpSlices.size() ==
							 m_Handler3D->num_slices());
				assert(radioButton.m_WorkSlices.size() ==
							 m_Handler3D->num_slices());

				for (int i = 0; i < nSlices; i++)
				{
					m_Handler3D->copy2bmp(i, radioButton.m_BmpSlices.at(i), 1);
					//m_Handler3D->set_bmp( i, radioButton.m_BmpSlices.at(i), 1 );
					//float* bmp_data = (float*) malloc(sizeof(float)*size);
					//memcpy(bmp_data, radioButton.m_BmpSlices.at(i), sizeof(float)*size);
					//m_Handler3D->set_bmp( i, bmp_data, 1 );
					////m_Handler3D->bmp2workall();
				}

				for (int i = 0; i < nSlices; i++)
				{
					m_Handler3D->copy2work(i, radioButton.m_WorkSlices.at(i),
							1);
					//m_Handler3D->set_work( i, radioButton.m_WorkSlices.at(i), 1 );
					//float* work_data = (float*) malloc(sizeof(float)*size);
					//memcpy(work_data, radioButton.m_WorkSlices.at(i), sizeof(float)*size);
					//m_Handler3D->set_work( i, work_data, 1 );
					//work_data = nullptr;
					//delete work_data;
				}
				m_ItIsBeingLoaded = true;
				radioButton.m_IsActive = true;

				emit end_datachange(this, iseg::ClearUndo);
				emit dataset_changed();
			}

			continue;
		}
		else
		{
			QFont font(radioButton.m_RadioButton->font());
			font.setBold(false);
			radioButton.m_RadioButton->setFont(font);
			radioButton.m_IsActive = false;
		}
	}

	DatasetSelectionChanged();
}

void MultiDataset_widget::ChangeDatasetName()
{
	for (auto& radioButton : m_RadioButtons)
	{
		if (radioButton.m_RadioButton->isChecked())
		{
			EditText edit_text_dlg(this->parentWidget(), "",
					this->windowFlags());
			edit_text_dlg.set_editable_text(radioButton.m_RadioButton->text());
			if (edit_text_dlg.exec())
			{
				QString returning_text = edit_text_dlg.get_editable_text();
				radioButton.m_RadioButtonText = returning_text;
				radioButton.m_RadioButton->setText(returning_text);
			}
			return;
		}
	}
}

void MultiDataset_widget::RemoveDataset()
{
	int index = 0;
	for (auto& radioButton : m_RadioButtons)
	{
		if (radioButton.m_RadioButton->isChecked())
		{
			m_VboxDatasets->removeWidget(radioButton.m_RadioButton);

			std::for_each(radioButton.m_BmpSlices.begin(),
					radioButton.m_BmpSlices.end(),
					[](float* element) { delete element; });
			radioButton.m_BmpSlices.clear();

			std::for_each(radioButton.m_WorkSlices.begin(),
					radioButton.m_WorkSlices.end(),
					[](float* element) { delete element; });
			radioButton.m_WorkSlices.clear();

			delete radioButton.m_RadioButton;

			m_RadioButtons.erase(m_RadioButtons.begin() + index);

			break;
		}
		index++;
	}

	// Automatically check the active dataset
	for (int i = 0; i < GetNumberOfDatasets(); i++)
	{
		if (IsActive(i))
		{
			m_RadioButtons.at(i).m_RadioButton->setChecked(true);
			return;
		}
	}
}

void MultiDataset_widget::DatasetSelectionChanged()
{
	// Do not allow removing active dataset
	for (int i = 0; i < GetNumberOfDatasets(); i++)
	{
		if (IsActive(i) && IsChecked(i))
		{
			m_RemoveDatasetButton->setEnabled(false);
			break;
		}
		else
		{
			m_RemoveDatasetButton->setEnabled(true);
		}
	}

	// Do not allow removing main dataset
	if (IsChecked(0))
	{
		m_RemoveDatasetButton->setEnabled(false);
	}
}

bool MultiDataset_widget::IsChecked(const int multiDS_index)
{
	if (multiDS_index < m_RadioButtons.size())
	{
		return m_RadioButtons.at(multiDS_index).m_RadioButton->isChecked();
	}
	return false;
}

int MultiDataset_widget::GetNumberOfDatasets() { return m_RadioButtons.size(); }

bool MultiDataset_widget::IsActive(const int multiDS_index)
{
	if (multiDS_index < m_RadioButtons.size())
	{
		return m_RadioButtons.at(multiDS_index).m_IsActive;
	}
	return false;
}

std::vector<float*> MultiDataset_widget::GetBmpData(const int multiDS_index)
{
	if (multiDS_index < m_RadioButtons.size())
	{
		return m_RadioButtons.at(multiDS_index).m_BmpSlices;
	}
	return std::vector<float*>();
}

void MultiDataset_widget::SetBmpData(const int multiDS_index,
		std::vector<float*> bmp_bits_vc)
{
	if (multiDS_index < m_RadioButtons.size())
	{
		m_RadioButtons.at(multiDS_index).m_BmpSlices = bmp_bits_vc;
	}
}

std::vector<float*> MultiDataset_widget::GetWorkingData(const int multiDS_index)
{
	if (multiDS_index < m_RadioButtons.size())
	{
		return m_RadioButtons.at(multiDS_index).m_WorkSlices;
	}
	return std::vector<float*>();
}

void MultiDataset_widget::SetWorkingData(const int multiDS_index,
		std::vector<float*> work_bits_vc)
{
	if (multiDS_index < m_RadioButtons.size())
	{
		m_RadioButtons.at(multiDS_index).m_WorkSlices = work_bits_vc;
	}
}

bits_stack_pushdialog::bits_stack_pushdialog(QWidget* parent, const char* name,
		Qt::WindowFlags wFlags)
		: QDialog(parent, name, false, wFlags)
{
	vboxoverall = new Q3VBox(this);
	hboxparams = new Q3HBox(vboxoverall);
	vboxsliceselection = new Q3VBox(hboxparams);
	vboxdelimiter = new Q3VBox(hboxparams);
	hboxslicerange = new Q3HBox(hboxparams);
	vboxslicerangelabels = new Q3VBox(hboxslicerange);
	vboxslicerangelineedits = new Q3VBox(hboxslicerange);
	hboxpushbuttons = new Q3HBox(vboxoverall);

	rb_currentslice =
			new QRadioButton(QString("Current slice"), vboxsliceselection);
	rb_multislices =
			new QRadioButton(QString("Slice range"), vboxsliceselection);
	slicegroup = new QButtonGroup(this);
	slicegroup->insert(rb_currentslice);
	slicegroup->insert(rb_multislices);
	rb_currentslice->setChecked(TRUE);

	lb_startslice = new QLabel(QString("Start slice:"), vboxslicerangelabels);
	lb_endslice = new QLabel(QString("End slice:"), vboxslicerangelabels);
	le_startslice = new QLineEdit(QString(""), vboxslicerangelineedits);
	le_endslice = new QLineEdit(QString(""), vboxslicerangelineedits);

	pushexec = new QPushButton(QString("OK"), hboxpushbuttons);
	pushcancel = new QPushButton(QString("Cancel"), hboxpushbuttons);

	vboxoverall->setMargin(5);
	hboxslicerange->setMargin(5);
	vboxsliceselection->setMargin(5);
	hboxpushbuttons->setMargin(5);
	vboxdelimiter->setMargin(5);
	vboxsliceselection->layout()->setAlignment(Qt::AlignTop);
	vboxdelimiter->setFixedSize(15, hboxslicerange->height());
	this->setFixedSize(vboxoverall->sizeHint());

	hboxslicerange->hide();

	QObject::connect(pushexec, SIGNAL(clicked()), this, SLOT(accept()));
	QObject::connect(pushcancel, SIGNAL(clicked()), this, SLOT(reject()));
	QObject::connect(slicegroup, SIGNAL(buttonClicked(int)), this,
			SLOT(sliceselection_changed()));
}

bits_stack_pushdialog::~bits_stack_pushdialog()
{
	delete slicegroup;
	delete vboxoverall;
}

bool bits_stack_pushdialog::get_pushcurrentslice()
{
	return rb_currentslice->isChecked();
}

unsigned int bits_stack_pushdialog::get_startslice(bool* ok)
{
	return le_startslice->text().toUInt(ok);
}

unsigned int bits_stack_pushdialog::get_endslice(bool* ok)
{
	return le_endslice->text().toUInt(ok);
}

void bits_stack_pushdialog::sliceselection_changed()
{
	if (rb_currentslice->isChecked())
	{
		hboxslicerange->hide();
	}
	else
	{
		hboxslicerange->show();
	}
}

//xxxxxxxxxxxxxxxxxxxxxx histo xxxxxxxxxxxxxxxxxxxxxxxxxxxx

ZoomWidget::ZoomWidget(double zoom1, QDir picpath, QWidget* parent,
		const char* name, Qt::WindowFlags wFlags)
		: QWidget(parent, name, wFlags)
{
	zoom = zoom1;
	vbox1 = new Q3VBoxLayout(this);
	pushzoomin = new QPushButton(
			QIcon(picpath.absFilePath(QString("zoomin.png")).ascii()), "Zoom in",
			this);
	pushzoomout = new QPushButton(
			QIcon(picpath.absFilePath(QString("zoomout.png")).ascii()), "Zoom out",
			this);
	pushunzoom = new QPushButton(
			QIcon(picpath.absFilePath(QString("unzoom.png")).ascii()), "Unzoom",
			this);
	zoom_f =
			new QLabel(QString("x"), this); //+QString::number(zoom,'g',4),this);
	le_zoom_f = new QLineEdit(QString::number(zoom, 'g', 4), this);
	le_zoom_f->setFixedWidth(80);
	vbox1->addWidget(pushzoomin);
	vbox1->addWidget(pushzoomout);
	vbox1->addWidget(pushunzoom);
	hbox1 = new Q3HBoxLayout(this);
	hbox1->addWidget(zoom_f);
	hbox1->addWidget(le_zoom_f);
	vbox1->addLayout(hbox1);
	//	vbox1->addWidget(zoom_f);
	//	vbox1->addWidget(le_zoom_f);
	setFixedHeight(vbox1->sizeHint().height());

	QObject::connect(pushzoomin, SIGNAL(clicked()), this,
			SLOT(zoomin_pushed()));
	QObject::connect(pushzoomout, SIGNAL(clicked()), this,
			SLOT(zoomout_pushed()));
	QObject::connect(pushunzoom, SIGNAL(clicked()), this,
			SLOT(unzoom_pushed()));
	QObject::connect(le_zoom_f, SIGNAL(editingFinished()), this,
			SLOT(le_zoom_changed()));
}

ZoomWidget::~ZoomWidget()
{
	delete vbox1;
	delete pushzoomin;
	delete pushzoomout;
	delete pushunzoom;
	delete le_zoom_f;
	delete zoom_f;
}

double ZoomWidget::get_zoom() { return zoom; }

void ZoomWidget::zoom_changed(double z)
{
	zoom = z;
	zoom_f->setText(QString("x")); //+QString::number(zoom,'g',4));
	le_zoom_f->setText(QString::number(zoom, 'g', 4));
	emit set_zoom(zoom);
}

void ZoomWidget::zoomin_pushed()
{
	zoom = 2 * zoom;
	zoom_f->setText(QString("x")); //+QString::number(zoom,'g',4));
	le_zoom_f->setText(QString::number(zoom, 'g', 4));
	emit set_zoom(zoom);
}

void ZoomWidget::zoomout_pushed()
{
	zoom = 0.5 * zoom;
	zoom_f->setText(QString("x")); //+QString::number(zoom,'g',4));
	le_zoom_f->setText(QString::number(zoom, 'g', 4));
	emit set_zoom(zoom);
}

void ZoomWidget::unzoom_pushed()
{
	zoom = 1.0;
	zoom_f->setText(QString("x")); //+QString::number(zoom,'g',4));
	le_zoom_f->setText(QString::number(zoom, 'g', 4));
	emit set_zoom(zoom);
}

void ZoomWidget::le_zoom_changed()
{
	bool b1;
	float zoom1 = le_zoom_f->text().toFloat(&b1);
	if (b1)
	{
		zoom = zoom1;
		zoom_f->setText(QString("x")); //+QString::number(zoom,'g',4));
		emit set_zoom(zoom);
	}
	else
	{
		if (le_zoom_f->text() != QString("."))
		{
			QApplication::beep();
		}
	}
}

//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

/*QHBoxLayout_fixedheight::QHBoxLayout_fixedheight(QWidget *parent,int margin,int spacing,const char *name)
	: QHBoxLayout(parent,margin,spacing,name)
{
}

QSizePolicy::ExpandData QHBoxLayout_fixedheight::expanding() const
{
	return QSizePolicy::Horizontal;
}

QSize QHBoxLayout_fixedheight::maximumSize() const
{
	QSize qs=sizeHint();
	qs.setHeight(15);
	return qs;
}*/

//--------------------------------------------------

ImageMath::ImageMath(SlicesHandler* hand3D, QWidget* parent, const char* name,
		Qt::WindowFlags wFlags)
		//  : QWidget( parent, name, wFlags ),handler3D(hand3D)
		: QDialog(parent, name, TRUE, wFlags), handler3D(hand3D)
{
	activeslice = handler3D->active_slice();
	bmphand = handler3D->get_activebmphandler();

	vbox1 = new Q3VBox(this);

	hbox1 = new Q3HBox(vbox1);
	imgorval = new QButtonGroup(this);
	//	imgorval->hide();
	rb_img = new QRadioButton(QString("Image"), hbox1);
	rb_val = new QRadioButton(QString("Value"), hbox1);
	imgorval->insert(rb_img);
	imgorval->insert(rb_val);
	rb_img->setChecked(TRUE);
	rb_img->show();
	rb_val->show();
	hbox1->show();

	hbox2 = new Q3HBox(vbox1);
	lb_val = new QLabel(QString("Value: "), hbox2);
	lb_val->show();
	le_val = new QLineEdit(QString::number((int)0), hbox2);
	le_val->show();
	val = 0;
	hbox2->show();

	allslices = new QCheckBox(QString("3D"), vbox1);

	hbox3 = new Q3HBox(vbox1);
	doAdd = new QPushButton("Add.", hbox3);
	doAdd->show();
	doSub = new QPushButton("Subt.", hbox3);
	doSub->show();
	doMult = new QPushButton("Mult.", hbox3);
	doMult->show();
	doNeg = new QPushButton("Neg.", hbox3);
	doNeg->show();
	hbox3->show();

	closeButton = new QPushButton("Close", vbox1);

	vbox1->setFixedSize(vbox1->sizeHint());
	setFixedSize(vbox1->size());

	imgorval_changed(0);

	QObject::connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));
	QObject::connect(doAdd, SIGNAL(clicked()), this, SLOT(add_pushed()));
	QObject::connect(doSub, SIGNAL(clicked()), this, SLOT(sub_pushed()));
	QObject::connect(doMult, SIGNAL(clicked()), this, SLOT(mult_pushed()));
	QObject::connect(doNeg, SIGNAL(clicked()), this, SLOT(neg_pushed()));
	QObject::connect(imgorval, SIGNAL(buttonClicked(int)), this,
			SLOT(imgorval_changed(int)));
	QObject::connect(le_val, SIGNAL(editingFinished()), this,
			SLOT(value_changed()));

	return;
}

ImageMath::~ImageMath() { delete vbox1; }

void ImageMath::imgorval_changed(int)
{
	if (rb_val->isOn())
	{
		hbox2->show();
	}
	else
	{
		hbox2->hide();
	}
}

void ImageMath::add_pushed()
{
	activeslice = handler3D->active_slice();
	bmphand = handler3D->get_activebmphandler();

	iseg::DataSelection dataSelection;
	dataSelection.allSlices = allslices->isChecked();
	dataSelection.sliceNr = handler3D->active_slice();
	dataSelection.work = true;
	emit begin_datachange(dataSelection, this);

	if (allslices->isChecked())
	{
		if (rb_val->isOn())
		{
			handler3D->bmp_add(val);
		}
		else
		{
			handler3D->bmp_sum();
		}
	}
	else
	{
		if (rb_val->isOn())
		{
			bmphand->bmp_add(val);
		}
		else
		{
			bmphand->bmp_sum();
		}
	}

	emit end_datachange(this);
}

void ImageMath::sub_pushed()
{
	activeslice = handler3D->active_slice();
	bmphand = handler3D->get_activebmphandler();

	iseg::DataSelection dataSelection;
	dataSelection.allSlices = allslices->isChecked();
	dataSelection.sliceNr = handler3D->active_slice();
	dataSelection.work = true;
	emit begin_datachange(dataSelection, this);

	if (allslices->isChecked())
	{
		if (rb_val->isOn())
		{
			handler3D->bmp_add(-val);
		}
		else
		{
			handler3D->bmp_diff();
		}
	}
	else
	{
		if (rb_val->isOn())
		{
			bmphand->bmp_add(-val);
		}
		else
		{
			bmphand->bmp_diff();
		}
	}

	emit end_datachange(this);
}

void ImageMath::mult_pushed()
{
	activeslice = handler3D->active_slice();
	bmphand = handler3D->get_activebmphandler();

	iseg::DataSelection dataSelection;
	dataSelection.allSlices = allslices->isChecked();
	dataSelection.sliceNr = handler3D->active_slice();
	dataSelection.work = true;
	emit begin_datachange(dataSelection, this);

	if (allslices->isChecked())
	{
		if (rb_val->isOn())
		{
			handler3D->bmp_mult(val);
		}
		else
		{
			handler3D->bmp_mult();
		}
	}
	else
	{
		if (rb_val->isOn())
		{
			bmphand->bmp_mult(val);
		}
		else
		{
			bmphand->bmp_mult();
		}
	}

	emit end_datachange(this);
}

void ImageMath::neg_pushed()
{
	activeslice = handler3D->active_slice();
	bmphand = handler3D->get_activebmphandler();

	iseg::DataSelection dataSelection;
	dataSelection.allSlices = allslices->isChecked();
	dataSelection.sliceNr = handler3D->active_slice();
	dataSelection.work = true;
	emit begin_datachange(dataSelection, this);

	if (allslices->isChecked())
	{
		handler3D->bmp_neg();
	}
	else
	{
		bmphand->bmp_neg();
	}

	emit end_datachange(this);
}

void ImageMath::slicenr_changed()
{
	activeslice = handler3D->active_slice();
	bmphand_changed(handler3D->get_activebmphandler());
}

void ImageMath::bmphand_changed(bmphandler* bmph)
{
	bmphand = bmph;
	return;
}

void ImageMath::value_changed()
{
	bool b1;
	float value = le_val->text().toFloat(&b1);
	if (b1)
	{
		val = value;
	}
	else
	{
		if (le_val->text() != QString("."))
		{
			QApplication::beep();
		}
	}
}

//--------------------------------------------------

ImageOverlay::ImageOverlay(SlicesHandler* hand3D, QWidget* parent,
		const char* name, Qt::WindowFlags wFlags)
		//  : QWidget( parent, name, wFlags ),handler3D(hand3D)
		: QDialog(parent, name, TRUE, wFlags), handler3D(hand3D)
{
	activeslice = handler3D->active_slice();
	bmphand = handler3D->get_activebmphandler();
	bkpWork = (float*)malloc(sizeof(float) * bmphand->return_area());
	bmphand->copyfromwork(bkpWork);

	alpha = 0.0f;
	sliderMax = 1000;

	sliderPrecision = 0;
	int tmp = sliderMax;
	while (tmp > 1)
	{
		tmp /= 10;
		sliderPrecision++;
	}

	vbox1 = new Q3VBox(this);
	hbox1 = new Q3HBox(vbox1);
	lbAlpha = new QLabel(QString("Alpha: "), hbox1);
	lbAlpha->show();
	slAlpha = new QSlider(Qt::Horizontal, hbox1);
	slAlpha->setRange(0, sliderMax);
	slAlpha->setValue(alpha * sliderMax);
	slAlpha->show();
	leAlpha = new QLineEdit(QString::number((int)0), hbox1);
	leAlpha->setAlignment(Qt::AlignRight);
	leAlpha->setText(QString::number(alpha, 'f', sliderPrecision));
	QString text = leAlpha->text();
	QFontMetrics fm = leAlpha->fontMetrics();
	QRect rect = fm.boundingRect(text);
	leAlpha->setFixedSize(rect.width() + 10, rect.height() + 4);
	leAlpha->show();
	hbox1->show();

	hbox2 = new Q3HBox(vbox1);
	allslices = new QCheckBox(QString("3D"), hbox2);
	allslices->show();
	hbox2->show();

	hbox3 = new Q3HBox(vbox1);
	closeButton = new QPushButton("Close", hbox3);
	closeButton->show();
	applyButton = new QPushButton("Apply", hbox3);
	applyButton->show();
	hbox3->show();

	vbox1->setFixedSize(vbox1->sizeHint());
	setFixedSize(vbox1->size());

	QObject::connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));
	QObject::connect(applyButton, SIGNAL(clicked()), this,
			SLOT(apply_pushed()));
	QObject::connect(leAlpha, SIGNAL(editingFinished()), this,
			SLOT(alpha_changed()));
	QObject::connect(slAlpha, SIGNAL(sliderMoved(int)), this,
			SLOT(slider_changed(int)));

	return;
}

ImageOverlay::~ImageOverlay()
{
	delete vbox1;
	free(bkpWork);
}

void ImageOverlay::closeEvent(QCloseEvent* e)
{
	QDialog::closeEvent(e);

	if (e->isAccepted())
	{
		// Undo overlay
		iseg::DataSelection dataSelection;
		dataSelection.allSlices = false;
		dataSelection.sliceNr = handler3D->active_slice();
		dataSelection.work = true;
		emit begin_datachange(dataSelection, this, false);

		bmphand->copy2work(bkpWork, bmphand->return_mode(false));

		emit end_datachange(this, iseg::NoUndo);
	}
}

void ImageOverlay::apply_pushed()
{
	// Swap modified work with original work backup for undo operation
	bkpWork = bmphand->swap_work_pointer(bkpWork);

	iseg::DataSelection dataSelection;
	dataSelection.allSlices = allslices->isChecked();
	dataSelection.sliceNr = handler3D->active_slice();
	dataSelection.work = true;
	emit begin_datachange(dataSelection, this);

	if (allslices->isChecked())
	{
		handler3D->bmp_overlay(alpha);
	}
	else
	{
		bmphand->bmp_overlay(alpha);
	}
	bmphand->copyfromwork(bkpWork);

	// Reset alpha
	alpha = 0.0f;
	slAlpha->setValue(alpha * sliderMax);
	leAlpha->setText(QString::number(alpha, 'f', sliderPrecision));

	emit end_datachange(this);
}

void ImageOverlay::slicenr_changed()
{
	activeslice = handler3D->active_slice();
	bmphand_changed(handler3D->get_activebmphandler());
}

void ImageOverlay::bmphand_changed(bmphandler* bmph)
{
	bmphand = bmph;
	bmphand->copyfromwork(bkpWork);
	return;
}

void ImageOverlay::newloaded()
{
	free(bkpWork);
	bkpWork = (float*)malloc(sizeof(float) *
													 handler3D->get_activebmphandler()->return_area());
}

void ImageOverlay::alpha_changed()
{
	bool b1;
	float value = leAlpha->text().toFloat(&b1);
	if (!b1)
	{
		leAlpha->setText(QString::number(alpha, 'f', sliderPrecision));
		slAlpha->setValue(alpha * sliderMax);
		QApplication::beep();
		return;
	}

	if (value < 0.0f)
	{
		value = 0.0f;
	}
	else if (value > 1.0f)
	{
		value = 1.0f;
	}
	alpha = value;
	leAlpha->setText(QString::number(alpha, 'f', sliderPrecision));
	slAlpha->setValue(alpha * sliderMax);

	iseg::DataSelection dataSelection;
	dataSelection.allSlices = false;
	dataSelection.sliceNr = handler3D->active_slice();
	dataSelection.work = true;
	emit begin_datachange(dataSelection, this, false);

	bmphand->copy2work(bkpWork, bmphand->return_mode(false));
	bmphand->bmp_overlay(alpha);

	emit end_datachange(this, iseg::NoUndo);
}

void ImageOverlay::slider_changed(int newval)
{
	alpha = newval / (float)sliderMax;
	leAlpha->setText(QString::number(alpha, 'f', sliderPrecision));

	iseg::DataSelection dataSelection;
	dataSelection.allSlices = false;
	dataSelection.sliceNr = handler3D->active_slice();
	dataSelection.work = true;
	emit begin_datachange(dataSelection, this, false);

	bmphand->copy2work(bkpWork, bmphand->return_mode(false));
	bmphand->bmp_overlay(alpha);

	emit end_datachange(this, iseg::NoUndo);
}

CleanerParams::CleanerParams(int* rate1, int* minsize1, QWidget* parent,
		const char* name, Qt::WindowFlags wFlags)
{
	rate = rate1;
	minsize = minsize1;
	hbox1 = new Q3HBox(this);
	vbox1 = new Q3VBox(hbox1);
	vbox2 = new Q3VBox(hbox1);
	lb_rate = new QLabel(QString("Rate: "), vbox1);
	lb_minsize = new QLabel(QString("Pixel Size: "), vbox1);
	pb_doit = new QPushButton("OK", vbox1);
	sb_rate = new QSpinBox(3, 10000, 1, vbox2);
	sb_rate->setValue(4);
	sb_rate->setToolTip(Format("1/rate is the percentage of the total (tissue) volume needed to force small regions to be kept (overrides Pixel Size criterion)."));
	sb_minsize = new QSpinBox(2, 10000, 1, vbox2);
	sb_minsize->setValue(10);
	sb_minsize->setToolTip(Format("Minimum number of pixels required by an island."));
	pb_dontdoit = new QPushButton("Cancel", vbox2);
	QObject::connect(pb_doit, SIGNAL(clicked()), this, SLOT(doit_pressed()));
	QObject::connect(pb_dontdoit, SIGNAL(clicked()), this,
			SLOT(dontdoit_pressed()));

	hbox1->setFixedSize(hbox1->sizeHint());
	setFixedSize(hbox1->size());
}

CleanerParams::~CleanerParams() { delete hbox1; }

void CleanerParams::doit_pressed()
{
	*rate = sb_rate->value();
	*minsize = sb_minsize->value();
	close();
}

void CleanerParams::dontdoit_pressed()
{
	*rate = 0;
	*minsize = 0;
	close();
}

MergeProjectsDialog::MergeProjectsDialog(QWidget* parent, const char* name,
		Qt::WindowFlags wFlags)
		: QDialog(parent, name, TRUE, wFlags)
{
	hboxOverall = new Q3HBoxLayout(this);
	vboxFileList = new Q3VBoxLayout(this);
	vboxButtons = new Q3VBoxLayout(this);
	vboxEditButtons = new Q3VBoxLayout(this);
	vboxExecuteButtons = new Q3VBoxLayout(this);

	fileListWidget = new QListWidget(this);
	fileListWidget->setSelectionMode(QAbstractItemView::ContiguousSelection);

	addButton = new QPushButton("Add...", this);
	removeButton = new QPushButton("Remove", this);
	moveUpButton = new QPushButton("Move up", this);
	moveDownButton = new QPushButton("Move down", this);
	executeButton = new QPushButton("Execute", this);
	cancelButton = new QPushButton("Cancel", this);

	hboxOverall->setMargin(10);

	vboxFileList->addWidget(fileListWidget);

	vboxEditButtons->setAlignment(Qt::AlignTop);
	vboxEditButtons->addWidget(addButton);
	vboxEditButtons->addWidget(removeButton);
	vboxEditButtons->addWidget(moveUpButton);
	vboxEditButtons->addWidget(moveDownButton);

	vboxExecuteButtons->setAlignment(Qt::AlignBottom);
	vboxExecuteButtons->addWidget(executeButton);
	vboxExecuteButtons->addWidget(cancelButton);

	vboxButtons->addLayout(vboxEditButtons);
	vboxButtons->addLayout(vboxExecuteButtons);

	hboxOverall->addLayout(vboxFileList);
	hboxOverall->addSpacing(10);
	hboxOverall->addLayout(vboxButtons);

	QObject::connect(addButton, SIGNAL(clicked()), this, SLOT(add_pressed()));
	QObject::connect(removeButton, SIGNAL(clicked()), this,
			SLOT(remove_pressed()));
	QObject::connect(moveUpButton, SIGNAL(clicked()), this,
			SLOT(move_up_pressed()));
	QObject::connect(moveDownButton, SIGNAL(clicked()), this,
			SLOT(move_down_pressed()));
	QObject::connect(executeButton, SIGNAL(clicked()), this, SLOT(accept()));
	QObject::connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
}

MergeProjectsDialog::~MergeProjectsDialog() { delete hboxOverall; }

void MergeProjectsDialog::add_pressed()
{
	QStringList openfilenames = QFileDialog::getOpenFileNames(
			"Projects (*.prj)", QString::null, this, "iSeg",
			"Select one or more files to add");
	fileListWidget->addItems(openfilenames);
}

void MergeProjectsDialog::remove_pressed()
{
	QList<QListWidgetItem*> removeItems = fileListWidget->selectedItems();
	for (QList<QListWidgetItem*>::const_iterator iter = removeItems.begin();
			 iter != removeItems.end(); ++iter)
	{
		delete fileListWidget->takeItem(fileListWidget->row(*iter));
	}
}

void MergeProjectsDialog::move_up_pressed()
{
	QList<QListWidgetItem*> moveItems = fileListWidget->selectedItems();
	if (moveItems.size() <= 0)
	{
		return;
	}

	int rowFirst = fileListWidget->row(*moveItems.begin());
	int rowLast = fileListWidget->row(*(moveItems.end() - 1));
	if (rowFirst <= 0)
	{
		return;
	}

	fileListWidget->insertItem(rowLast, fileListWidget->takeItem(rowFirst - 1));
}

void MergeProjectsDialog::move_down_pressed()
{
	QList<QListWidgetItem*> moveItems = fileListWidget->selectedItems();
	if (moveItems.size() <= 0)
	{
		return;
	}

	int rowFirst = fileListWidget->row(*moveItems.begin());
	int rowLast = fileListWidget->row(*(moveItems.end() - 1));
	if (rowLast >= fileListWidget->count() - 1)
	{
		return;
	}

	fileListWidget->insertItem(rowFirst, fileListWidget->takeItem(rowLast + 1));
}

void MergeProjectsDialog::get_filenames(std::vector<QString>& filenames)
{
	filenames.clear();
	for (int row = 0; row < fileListWidget->count(); ++row)
	{
		filenames.push_back(fileListWidget->item(row)->text());
	}
}

CheckBoneConnectivityDialog::CheckBoneConnectivityDialog(
		SlicesHandler* hand3D, const char* name, QWidget* parent /*=0*/,
		Qt::WindowFlags wFlags /*=0*/)
		: QWidget(parent, name, wFlags), handler3D(hand3D)
{
	mainBox = new Q3HBox(this);
	vbox1 = new Q3VBox(mainBox);
	hbox1 = new Q3HBox(vbox1);
	hbox2 = new Q3HBox(vbox1);
	hbox3 = new Q3HBox(vbox1);
	hbox4 = new Q3HBox(vbox1);

	vbox1->setMargin(3);
	hbox1->setMargin(3);
	hbox2->setMargin(3);
	hbox3->setMargin(3);
	hbox4->setMargin(3);

	mainBox->setFixedSize(QSize(420, 420));

	hbox1->setMaximumHeight(30);
	hbox2->setMaximumHeight(30);

	bonesFoundCB = new QCheckBox(QString("Bones Found"), hbox1);
	bonesFoundCB->setEnabled(false);

	executeButton = new QPushButton("Execute", hbox2);
	cancelButton = new QPushButton("Cancel", hbox2);

	foundConnectionsTable = new QTableWidget(hbox3);
	foundConnectionsTable->setColumnCount(BoneConnectionColumn::kColumnNumber);

	exportButton = new QPushButton("Export", hbox4);
	exportButton->setMaximumWidth(60);
	exportButton->setEnabled(false);
	progressText = new QLabel(hbox4);
	progressText->setAlignment(Qt::AlignRight);

	QStringList tableHeader;
	tableHeader << "Bone 1"
							<< "Bone 2"
							<< "Slice #";
	foundConnectionsTable->setHorizontalHeaderLabels(tableHeader);
	foundConnectionsTable->verticalHeader()->setVisible(false);
	foundConnectionsTable->setColumnWidth(BoneConnectionColumn::kTissue1, 160);
	foundConnectionsTable->setColumnWidth(BoneConnectionColumn::kTissue2, 160);
	foundConnectionsTable->setColumnWidth(BoneConnectionColumn::kSliceNumber,
			60);

	hbox1->show();
	hbox2->show();
	hbox3->show();
	hbox4->show();
	vbox1->show();

	mainBox->show();

	QObject::connect(executeButton, SIGNAL(clicked()), this,
			SLOT(execute_pressed()));
	QObject::connect(cancelButton, SIGNAL(clicked()), this,
			SLOT(cancel_pressed()));
	QObject::connect(exportButton, SIGNAL(clicked()), this,
			SLOT(export_pressed()));
	QObject::connect(foundConnectionsTable, SIGNAL(cellClicked(int, int)), this,
			SLOT(cellClicked(int, int)));

	CheckBoneExist();

	return;
}

CheckBoneConnectivityDialog::~CheckBoneConnectivityDialog() {}

void CheckBoneConnectivityDialog::ShowText(const std::string& text)
{
	progressText->setText(QString::fromStdString(text));
}

bool CheckBoneConnectivityDialog::IsBone(const std::string& label_name) const
{
	std::string name = label_name;
	std::transform(name.begin(), name.end(), name.begin(), ::tolower);

	if (name.find("cortical") != std::string::npos)
	{
		return true;
	}
	else if (name.find("cancellous") != std::string::npos)
	{
		return true;
	}
	else if (name.find("marrow") != std::string::npos)
	{
		return true;
	}
	else if (name.find("tooth") != std::string::npos)
	{
		return true;
	}
	else if (name.find("bone") != std::string::npos)
	{
		return true;
	}
	return false;
}

void CheckBoneConnectivityDialog::CheckBoneExist()
{
	int bonesFound = 0;

	tissues_size_t tissuecount = TissueInfos::GetTissueCount();
	for (tissues_size_t i = 0; i <= tissuecount; i++)
	{
		TissueInfoStruct* tissueInfo = TissueInfos::GetTissueInfo(i);
		if (IsBone(tissueInfo->name))
		{
			bonesFound++;
			if (bonesFound > 1)
			{
				break;
			}
		}
	}

	if (bonesFound > 1)
	{
		ShowText("Bones found. Press Execute to look for bone connections");
	}
	else if (bonesFound == 1)
	{
		ShowText("Only one bone found. No possible connections");
	}
	else
	{
		ShowText("One bone found. No possible connection");
	}

	bonesFoundCB->setChecked(bonesFound > 1);
	executeButton->setEnabled(bonesFound > 1);
}

void CheckBoneConnectivityDialog::LookForConnections()
{
	foundConnections.clear();

	unsigned short width = handler3D->width();
	unsigned short height = handler3D->height();
	unsigned short startSl = handler3D->start_slice();
	unsigned short endSl = handler3D->end_slice();

	int numTasks = endSl - startSl;
	QProgressDialog progress("Looking for connected bones...", "Cancel", 0,
			numTasks, mainBox);
	progress.show();
	progress.setWindowModality(Qt::WindowModal);
	progress.setModal(true);
	progress.setValue(0);

	std::vector<std::string> label_names;
	tissues_size_t tissuecount = TissueInfos::GetTissueCount();
	for (tissues_size_t i = 0; i <= tissuecount; i++)
	{
		TissueInfoStruct* tissueInfo = TissueInfos::GetTissueInfo(i);
		label_names.push_back(tissueInfo->name);
	}

	std::vector<int> same_bone_map(label_names.size(), -1);
	auto replace = [](std::string& str, const std::string& from,
										 const std::string& to) {
		size_t start_pos = str.find(from);
		if (start_pos == std::string::npos)
		{
			return false;
		}
		str.replace(start_pos, from.length(), to);
		return true;
	};

	for (size_t i = 0; i < label_names.size(); i++)
	{
		if (IsBone(label_names[i]))
		{
			std::string name = label_names[i];
			std::transform(name.begin(), name.end(), name.begin(), ::tolower);
			replace(name, "cancellous", "cortical");
			replace(name, "red_marrow", "cortical");
			replace(name, "marrow", "cortical");
			size_t idx = 0;
			for (; idx < label_names.size(); idx++)
			{
				std::string namei = label_names[idx];
				std::transform(namei.begin(), namei.end(), namei.begin(),
						::tolower);
				if (namei == name)
				{
					break;
				}
			}
			if (idx < label_names.size())
			{
				same_bone_map[i] = (int)idx;
			}
			else
			{
				same_bone_map[i] = (int)i;
			}
		}
	}

	for (int sliceN = startSl; sliceN < endSl - 1; sliceN++)
	{
		tissues_size_t* tissuesMain = handler3D->return_tissues(0, sliceN);
		unsigned pos = 0;
		//for( int y=height-1; y>=0; y-- )
		for (int y = height - 2; y >= 1; y--)
		{
			for (int x = 1; x < width - 1; x++)
			{
				pos = y * width + x;

				tissues_size_t tissue_value = tissuesMain[pos];
				if (std::find(same_bone_map.begin(), same_bone_map.end(),
								tissue_value) != same_bone_map.end())
				{
					// check neighbour connection
					std::vector<tissues_size_t> neighbour_tissues;

					//Same slice:
					neighbour_tissues.push_back(tissuesMain[pos - width - 1]);
					neighbour_tissues.push_back(tissuesMain[pos - width]);
					neighbour_tissues.push_back(tissuesMain[pos - width + 1]);

					neighbour_tissues.push_back(tissuesMain[pos - 1]);
					neighbour_tissues.push_back(tissuesMain[pos + 1]);

					neighbour_tissues.push_back(tissuesMain[pos + width - 1]);
					neighbour_tissues.push_back(tissuesMain[pos + width]);
					neighbour_tissues.push_back(tissuesMain[pos + width + 1]);

					//Not needed
					//-1 slice:
					//if( sliceN-1 >= startSl)
					//{
					//	tissues_size_t *tissues_minus1 = handler3D->return_tissues(0,sliceN-1);
					//
					//	neighbour_tissues.push_back( tissues_minus1[pos-width-1] );
					//	neighbour_tissues.push_back( tissues_minus1[pos-width] );
					//	neighbour_tissues.push_back( tissues_minus1[pos-width+1] );
					//
					//	neighbour_tissues.push_back( tissues_minus1[pos-1] );
					//	neighbour_tissues.push_back( tissues_minus1[pos] );
					//	neighbour_tissues.push_back( tissues_minus1[pos+1] );
					//
					//	neighbour_tissues.push_back( tissues_minus1[pos+width-1] );
					//	neighbour_tissues.push_back( tissues_minus1[pos+width] );
					//	neighbour_tissues.push_back( tissues_minus1[pos+width+1] );
					//}

					//+1 slice:
					if (sliceN + 1 < endSl)
					{
						tissues_size_t* tissues_plus1 =
								handler3D->return_tissues(0, sliceN + 1);

						neighbour_tissues.push_back(
								tissues_plus1[pos - width - 1]);
						neighbour_tissues.push_back(tissues_plus1[pos - width]);
						neighbour_tissues.push_back(
								tissues_plus1[pos - width + 1]);

						neighbour_tissues.push_back(tissues_plus1[pos - 1]);
						neighbour_tissues.push_back(tissues_plus1[pos]);
						neighbour_tissues.push_back(tissues_plus1[pos + 1]);

						neighbour_tissues.push_back(
								tissues_plus1[pos + width - 1]);
						neighbour_tissues.push_back(tissues_plus1[pos + width]);
						neighbour_tissues.push_back(
								tissues_plus1[pos + width + 1]);
					}

					//remove the same bone values
					std::sort(neighbour_tissues.begin(),
							neighbour_tissues.end());
					neighbour_tissues.erase(
							std::unique(neighbour_tissues.begin(),
									neighbour_tissues.end()),
							neighbour_tissues.end());
					neighbour_tissues.erase(
							std::remove(neighbour_tissues.begin(),
									neighbour_tissues.end(), tissue_value),
							neighbour_tissues.end());

					for (size_t k = 0; k < neighbour_tissues.size(); k++)
					{
						if (std::find(
										same_bone_map.begin(), same_bone_map.end(),
										neighbour_tissues[k]) != same_bone_map.end())
						{
							tissues_size_t tis1, tis2;
							if (tissue_value < neighbour_tissues[k])
							{
								tis1 = tissue_value;
								tis2 = neighbour_tissues[k];
							}
							else
							{
								tis1 = neighbour_tissues[k];
								tis2 = tissue_value;
							}
							BoneConnectionInfo matchFound(tis1, tis2, sliceN);
							foundConnections.push_back(matchFound);
						}
					}
				}
			}
		}

		progress.setValue(sliceN);

		if (progress.wasCanceled())
		{
			break;
		}
	}

	progress.setValue(numTasks);

	std::sort(foundConnections.begin(), foundConnections.end());
	foundConnections.erase(
			std::unique(foundConnections.begin(), foundConnections.end()),
			foundConnections.end());
}

void CheckBoneConnectivityDialog::FillConnectionsTable()
{
	while (foundConnectionsTable->rowCount() > 0)
	{
		foundConnectionsTable->removeRow(0);
	}

	for (size_t i = 0; i < foundConnections.size(); i++)
	{
		int row = foundConnectionsTable->rowCount();
		foundConnectionsTable->insertRow(row);

		BoneConnectionInfo newLineInfo = foundConnections.at(i);

		TissueInfoStruct* tissueInfo1 =
				TissueInfos::GetTissueInfo(newLineInfo.TissueID1);
		TissueInfoStruct* tissueInfo2 =
				TissueInfos::GetTissueInfo(newLineInfo.TissueID2);

		foundConnectionsTable->setItem(row, BoneConnectionColumn::kTissue1,
				new QTableWidgetItem(ToQ(tissueInfo1->name)));
		foundConnectionsTable->setItem(row, BoneConnectionColumn::kTissue2,
				new QTableWidgetItem(ToQ(tissueInfo2->name)));
		foundConnectionsTable->setItem(
				row, BoneConnectionColumn::kSliceNumber,
				new QTableWidgetItem(QString::number(newLineInfo.SliceNumber + 1)));
	}

	exportButton->setEnabled(foundConnections.size() > 0);
}

void CheckBoneConnectivityDialog::cellClicked(int row, int col)
{
	if (row < foundConnectionsTable->rowCount())
	{
		int sliceNumber =
				foundConnectionsTable->item(row, kSliceNumber)->text().toInt() - 1;
		handler3D->set_active_slice(sliceNumber);
		emit slice_changed();
	}
}

void CheckBoneConnectivityDialog::execute_pressed()
{
	executeButton->setEnabled(false);

	LookForConnections();
	FillConnectionsTable();

	ShowText(std::to_string(foundConnections.size()) + " connections found.");

	executeButton->setEnabled(true);
}

void CheckBoneConnectivityDialog::cancel_pressed() { close(); }

void CheckBoneConnectivityDialog::export_pressed()
{
	std::ofstream output_file("BoneConnections.txt");

	for (unsigned int i = 0; i < foundConnections.size(); i++)
	{
		TissueInfoStruct* tissueInfo1 =
				TissueInfos::GetTissueInfo(foundConnections[i].TissueID1);
		TissueInfoStruct* tissueInfo2 =
				TissueInfos::GetTissueInfo(foundConnections[i].TissueID2);
		output_file << tissueInfo1->name << " "
								<< tissueInfo2->name << " "
								<< foundConnections[i].SliceNumber + 1 << endl;
	}
	output_file.close();

	ShowText("Export finished to BoneConnections.txt");
}
