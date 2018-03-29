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

#include "SlicesHandler.h"
#include "bmp_read_1.h"
#include "bmpshower.h"
#include "TissueInfos.h"

#include "Core/ColorLookupTable.h"
#include "Core/Point.h"

#include <QCloseEvent>
#include <QContextMenuEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QWheelEvent>
#include <algorithm>
#include <q3filedialog.h>
#include <q3popupmenu.h>
#include <qapplication.h>
#include <qcolor.h>
#include <qevent.h>
#include <qimage.h>
#include <qinputdialog.h>
#include <qlineedit.h>
#include <qpainter.h>
#include <qpen.h>
#include <qwidget.h>

#include "vtkAutoInit.h"
#ifdef ISEG_VTK_OPENGL2
VTK_MODULE_INIT(vtkRenderingOpenGL2);
VTK_MODULE_INIT(vtkRenderingVolumeOpenGL2);
#else
VTK_MODULE_INIT(vtkRenderingOpenGL); // VTK was built with vtkRenderingOpenGL
VTK_MODULE_INIT(vtkRenderingVolumeOpenGL);
#endif
VTK_MODULE_INIT(vtkInteractionStyle);

#include <QVTKWidget.h>
#include <vtkActor.h>
#include <vtkActor2D.h>
#include <vtkCamera.h>
#include <vtkCellPicker.h>
#include <vtkColorTransferFunction.h>
#include <vtkCommand.h>
#include <vtkCutter.h>
#include <vtkDataSetMapper.h>
#include <vtkDiscreteMarchingCubes.h>
#include <vtkFloatArray.h>
#include <vtkGeometryFilter.h>
#include <vtkImageAccumulate.h>
#include <vtkImageCast.h>
#include <vtkImageData.h>
#include <vtkImageMapToColors.h>
#include <vtkImageShiftScale.h>
#include <vtkImplicitPlaneWidget.h>
#include <vtkInformation.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkLODActor.h>
#include <vtkLookupTable.h>
#include <vtkMaskFields.h>
#include <vtkOutlineFilter.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPlane.h>
#include <vtkPlaneWidget.h>
#include <vtkPointData.h>
#include <vtkPointPicker.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRectilinearGrid.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkScalarBarActor.h>
#include <vtkSmartPointer.h>
#include <vtkSmartVolumeMapper.h>
#include <vtkTextMapper.h>
#include <vtkTextProperty.h>
#include <vtkThreshold.h>
#include <vtkUnstructuredGrid.h>
#include <vtkUnstructuredGridReader.h>
#include <vtkUnstructuredGridWriter.h>
#include <vtkVectorNorm.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>
#include <vtkWindowedSincPolyDataFilter.h>
#include <vtkXMLImageDataReader.h>

#include <vtkDataSetSurfaceFilter.h>
#include <vtkExtractEdges.h>
#include <vtkExtractGeometry.h>
#include <vtkLookupTable.h>

#include <vtkBoxWidget.h>
#include <vtkIdList.h>
#include <vtkPlanes.h>

#include <vtkCell.h>
#include <vtkCellData.h>
#include <vtkDataObject.h>
#include <vtkDataSet.h>
#include <vtkDataSetReader.h>
#include <vtkGenericCell.h>
#include <vtkPointData.h>
// #include <vtkSmartPointer.h>

//#include <vtkGL2PSExporter.h>
#include <vtkPNGWriter.h>
#include <vtkWindowToImageFilter.h>

#include <vtkFloatArray.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkShepardMethod.h>

//#include <vtkDataArray.h>
#include <vtkDoubleArray.h>

#ifdef USE_FFMPEG
// #include <vtkAVIWriter.h>
#	include <vtkFFMPEGWriter.h>
//#include <vtkMPEG2Writer.h>
#endif

#include <vtksys/SystemTools.hxx>

#include <cassert>
#include <cmath>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
using namespace iseg;

/*float tissuecolor[15][3]=
{{1,1,1},{0,0,1},{0,1,0},{1,0,0},{1,1,0}};*/

bmpshower::bmpshower(QWidget* parent, const char* name, Qt::WindowFlags wFlags)
	: QWidget(parent, name, wFlags)
{
}

void bmpshower::init(float** bmpbits1, unsigned short w, unsigned short h)
{
	bmpbits = bmpbits1;
	width = w;
	height = h;
	image.create(int(w), int(h), 32);

	//	image.setNumColors(256) ;
	/*	for(int i=0;i<256;i++){
	image.setColor(i, qRgb(i,i,i));
	}*/

	setMinimumSize((int)w, (int)h);
	setMaximumSize((int)w, (int)h);
	show();

	reload_bits();
	return;
}

void bmpshower::reload_bits()
{
	float* bmpbits1 = *bmpbits;
	unsigned pos = 0;
	int f;

	for (int y = height - 1; y >= 0; y--)
	{
		for (int x = 0; x < width; x++)
		{
			f = (int)max(0.0f, min(255.0f, (bmpbits1)[pos]));
			image.setPixel(x, y, qRgb(f, f, f));
			//			image.setPixel(x,y,(uint)max(0.0f,min(255.0f,(bmpbits1)[pos])));
			pos++;
		}
	}
	//	image.setPixel(0,0,1);
	//	image.save("D:\\Development\\segmentation\\sample images\\TCCT.bmp","BMP");
	/*	FILE *fp=fopen("D:\\Development\\segmentation\\sample images\\TCCT-MONO2-8-abdo.txt","w");
	fprintf(fp,"hallo %u %i %i %i %i\n%u %i %i %i %i\ndepth: %i, %u\ncolornr %i",f,qRed(qrgb),qGreen(qrgb),qBlue(qrgb),qGray(qrgb),(unsigned)max(0.0f,min(255.0f,(*bmpbits)[0])),qRed(image.pixel(0,0)),qGreen(image.pixel(0,0)),qBlue(image.pixel(0,0)),qGray(image.pixel(0,0)),image.depth(),pos1,image.numColors());
	fclose(fp);*/
	return;
}

void bmpshower::paintEvent(QPaintEvent* e)
{
	if (image.size() != QSize(0, 0)) // is an image loaded?
	{
		QPainter painter(this);
		painter.setClipRect(e->rect());
		painter.drawImage(0, 0, image);
	}
}

void bmpshower::bmp_changed() { update(); }

void bmpshower::size_changed(unsigned short w, unsigned short h)
{
	update(w, h);
}

void bmpshower::update()
{
	reload_bits();
	repaint();
}

void bmpshower::update(unsigned short w, unsigned short h)
{
	if (w != width || h != height)
	{
		width = w;
		height = h;
		setMinimumSize((int)w, (int)h);
		setMaximumSize((int)w, (int)h);
	}
	reload_bits();
	repaint();
}

bmptissueshower::bmptissueshower(QWidget* parent, const char* name,
								 Qt::WindowFlags wFlags)
	: QWidget(parent, name, wFlags), tissuevisible(true)
{
}

void bmptissueshower::paintEvent(QPaintEvent* e)
{
	if (image.size() != QSize(0, 0)) // is an image loaded?
	{
		QPainter painter(this);
		painter.setClipRect(e->rect());
		painter.drawImage(0, 0, image);
	}
}

void bmptissueshower::bmp_changed() { update(); }

void bmptissueshower::size_changed(unsigned short w, unsigned short h)
{
	update(w, h);
}

void bmptissueshower::update()
{
	reload_bits();
	repaint();
}

void bmptissueshower::update(unsigned short w, unsigned short h)
{
	if (w != width || h != height)
	{
		width = w;
		height = h;
		image.create(int(w), int(h), 32);
		setFixedSize((int)w, (int)h);
	}
	reload_bits();
	repaint();
}

void bmptissueshower::init(float** bmpbits1, tissues_size_t** tissue1,
						   unsigned short w, unsigned short h)
{
	bmpbits = bmpbits1;
	tissue = tissue1;
	width = w;
	height = h;
	image.create(int(w), int(h), 32);

	//	image.setNumColors(256) ;
	/*	for(int i=0;i<256;i++){
	image.setColor(i, qRgb(i,i,i));
	}*/

	setFixedSize((int)w, (int)h);
	setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
	show();

	reload_bits();
	return;
}

void bmptissueshower::reload_bits()
{
	float* bmpbits1 = *bmpbits;
	tissues_size_t* tissue1 = *tissue;
	unsigned pos = 0;
	int f;
	unsigned char r, g, b;
	if (tissuevisible)
	{
		for (int y = height - 1; y >= 0; y--)
		{
			for (int x = 0; x < width; x++)
			{
				f = (int)max(0.0f, min(255.0f, (bmpbits1)[pos]));
				if (tissue1[pos] == 0)
				{
					image.setPixel(x, y, qRgb(int(f), int(f), int(f)));
				}
				else
				{
					TissueInfos::GetTissueColorBlendedRGB(tissue1[pos], r, g, b,
														  f);
					image.setPixel(x, y, qRgb(r, g, b));
				}
				pos++;
			}
		}
	}
	else // only target/source
	{
		for (int y = height - 1; y >= 0; y--)
		{
			for (int x = 0; x < width; x++)
			{
				f = (int)max(0.0f, min(255.0f, (bmpbits1)[pos]));
				image.setPixel(x, y, qRgb(f, f, f));
				pos++;
			}
		}
	}

	return;
}

void bmptissueshower::tissue_changed()
{
	reload_bits();
	repaint();
}

bool bmptissueshower::toggle_tissuevisible()
{
	tissuevisible = !tissuevisible;
	update();
	return tissuevisible;
}

void bmptissueshower::set_tissuevisible(bool on)
{
	tissuevisible = on;
	update();
	return;
}

bmptissuemarkshower::bmptissuemarkshower(QWidget* parent, const char* name,
										 Qt::WindowFlags wFlags)
	: QWidget(parent, name, wFlags), tissuevisible(true), markvisible(true)
{
	addmark = new Q3Action("&Add Mark", 0, this);
	addlabel = new Q3Action("Add &Label", 0, this);
	removemark = new Q3Action("&Remove Mark", 0, this);
	addtissue = new Q3Action("Add &Tissue", 0, this);
	addtissueconnected = new Q3Action("Add Tissue &Conn.", 0, this);
	subtissue = new Q3Action("&Subtract Tissue", 0, this);
	addtissuelarger = new Q3Action("Add Tissue &Larger", 0, this);
	connect(addmark, SIGNAL(activated()), this, SLOT(add_mark()));
	connect(addlabel, SIGNAL(activated()), this, SLOT(add_label()));
	connect(removemark, SIGNAL(activated()), this, SLOT(remove_mark()));
	connect(addtissue, SIGNAL(activated()), this, SLOT(add_tissue()));
	connect(addtissueconnected, SIGNAL(activated()), this,
			SLOT(add_tissue_connected()));
	connect(subtissue, SIGNAL(activated()), this, SLOT(sub_tissue()));
	connect(addtissuelarger, SIGNAL(activated()), this,
			SLOT(add_tissuelarger()));

	return;
}

bmptissuemarkshower::~bmptissuemarkshower()
{
	delete addmark;
	delete addlabel;
	delete removemark;
	delete addtissue;
	delete addtissueconnected;
	delete subtissue;
	delete addtissuelarger;
}

void bmptissuemarkshower::paintEvent(QPaintEvent* e)
{
	if (image.size() != QSize(0, 0)) // is an image loaded?
	{
		QPainter painter(this);
		painter.setClipRect(e->rect());
		painter.drawImage(0, 0, image);
		QPen pen1;
		unsigned char r, g, b;
		for (vector<Mark>::iterator it = marks->begin(); it != marks->end();
			 it++)
		{
			TissueInfos::GetTissueColorRGB(it->mark, r, g, b);
			QColor qc(r, g, b);
			painter.setPen(QPen(qc));
			//			const QPen pen1=painter.pen();
			/*			pen1.setColor(qc);
			pen1.setWidth(1);
			painter.setPen(pen1);*/
			painter.drawLine(int(it->p.px) - 2, int(height - it->p.py) - 3,
							 int(it->p.px) + 2, int(height - it->p.py) + 1);
			painter.drawLine(int(it->p.px) - 2, int(height - it->p.py) + 1,
							 int(it->p.px) + 2, int(height - it->p.py) - 3);
			if (it->name != std::string(""))
			{
				painter.drawText(int(it->p.px) + 3, int(height - it->p.py) + 1,
								 QString(it->name.c_str()));
			}
		}
	}
}

void bmptissuemarkshower::bmp_changed() { update(); }

void bmptissuemarkshower::update()
{
	if (bmphand->return_width() != width || bmphand->return_height() != height)
	{
		width = bmphand->return_width();
		height = bmphand->return_height();
		image.create(int(width), int(height), 32);
		setFixedSize((int)width, (int)height);
	}
	reload_bits();
	repaint();
}

void bmptissuemarkshower::init(bmphandler* bmph1, tissuelayers_size_t layeridx,
							   bool bmporwork1)
{
	bmphand = bmph1;
	bmporwork = bmporwork1;
	if (bmporwork)
		bmpbits = bmph1->return_bmpfield();
	else
		bmpbits = bmph1->return_workfield();
	tissue = bmph1->return_tissuefield(layeridx);
	width = bmph1->return_width();
	height = bmph1->return_height();

	marks = bmph1->return_marks();
	image.create(int(width), int(height), 32);

	//	image.setNumColors(256) ;
	/*	for(int i=0;i<256;i++){
	image.setColor(i, qRgb(i,i,i));
	}*/

	setFixedSize((int)width, (int)height);
	setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));

	reload_bits();
	show();
	return;
}

void bmptissuemarkshower::reload_bits()
{
	float* bmpbits1 = *bmpbits;
	tissues_size_t* tissue1 = *tissue;
	unsigned pos = 0;
	int f;
	unsigned char r, g, b;
	if (tissuevisible)
	{
		for (int y = height - 1; y >= 0; y--)
		{
			for (int x = 0; x < width; x++)
			{
				f = (int)max(0.0f, min(255.0f, (bmpbits1)[pos]));
				//				image.setPixel(x,y,qRgb(f*(tissuecolor[tissue1[pos]][0]),f*tissuecolor[tissue1[pos]][1],f*tissuecolor[tissue1[pos]][2]));
				if (tissue1[pos] == 0)
				{
					image.setPixel(x, y, qRgb(int(f), int(f), int(f)));
				}
				else
				{
					//image.setPixel(x,y,qRgb(int(f/2+127.5f*(tissuecolor[tissue1[pos]][0])),int(f/2+127.5f*tissuecolor[tissue1[pos]][1]),int(f/2+127.5f*tissuecolor[tissue1[pos]][2])));
					TissueInfos::GetTissueColorBlendedRGB(tissue1[pos], r, g, b,
														  f);
					image.setPixel(x, y, qRgb(r, g, b));
				}
				pos++;
			}
		}
	}
	else
	{
		for (int y = height - 1; y >= 0; y--)
		{
			for (int x = 0; x < width; x++)
			{
				f = (int)max(0.0f, min(255.0f, (bmpbits1)[pos]));
				image.setPixel(x, y, qRgb(f, f, f));
				pos++;
			}
		}
	}

	return;
}

void bmptissuemarkshower::tissue_changed()
{
	reload_bits();
	repaint();
}

void bmptissuemarkshower::mark_changed()
{
	marks = bmphand->return_marks();
	repaint();
}

bool bmptissuemarkshower::toggle_tissuevisible()
{
	tissuevisible = !tissuevisible;
	update();
	return tissuevisible;
}

bool bmptissuemarkshower::toggle_markvisible()
{
	markvisible = !markvisible;
	repaint();
	return markvisible;
}

void bmptissuemarkshower::set_tissuevisible(bool on)
{
	tissuevisible = on;
	update();
	return;
}

void bmptissuemarkshower::set_markvisible(bool on)
{
	markvisible = on;
	repaint();
	return;
}

void bmptissuemarkshower::add_mark()
{
	Point p;
	p.px = (unsigned short)eventx;
	p.py = (unsigned short)eventy;
	emit addmark_sign(p);
}

void bmptissuemarkshower::add_label()
{
	bool ok;
	QString newText = QInputDialog::getText(
		"Label", "Enter a name for the label:", QLineEdit::Normal, "", &ok,
		this);
	if (ok)
	{
		Point p;
		p.px = (unsigned short)eventx;
		p.py = (unsigned short)eventy;
		emit addlabel_sign(p, newText.ascii());
	}
}

void bmptissuemarkshower::remove_mark()
{
	Point p;
	p.px = (unsigned short)eventx;
	p.py = (unsigned short)eventy;
	emit removemark_sign(p);
}

void bmptissuemarkshower::add_tissue()
{
	Point p;
	p.px = (unsigned short)eventx;
	p.py = (unsigned short)eventy;
	emit addtissue_sign(p);
}

void bmptissuemarkshower::add_tissue_connected()
{
	Point p;
	p.px = (unsigned short)eventx;
	p.py = (unsigned short)eventy;
	emit addtissueconnected_sign(p);
}

void bmptissuemarkshower::sub_tissue()
{
	Point p;
	p.px = (unsigned short)eventx;
	p.py = (unsigned short)eventy;
	emit subtissue_sign(p);
}

void bmptissuemarkshower::add_tissuelarger()
{
	Point p;
	p.px = (unsigned short)eventx;
	p.py = (unsigned short)eventy;
	emit addtissuelarger_sign(p);
}

void bmptissuemarkshower::contextMenuEvent(QContextMenuEvent* event)
{
	eventx = event->x();
	eventy = height - 1 - event->y();
	Q3PopupMenu contextMenu(this);
	addmark->addTo(&contextMenu);
	addlabel->addTo(&contextMenu);
	removemark->addTo(&contextMenu);
	if (!bmporwork)
	{
		addtissue->addTo(&contextMenu);
		subtissue->addTo(&contextMenu);
		addtissueconnected->addTo(&contextMenu);
		addtissuelarger->addTo(&contextMenu);
	}
	contextMenu.exec(event->globalPos());
}

bmptissuemarklineshower::bmptissuemarklineshower(QWidget* parent,
												 const char* name,
												 Qt::WindowFlags wFlags)
	: QWidget(parent, name, wFlags), tissuevisible(true), picturevisible(true),
	  markvisible(true), overlayvisible(false), workborder(false),
	  crosshairxvisible(false), crosshairyvisible(false) //,showvp(false)
{
	brightness = scaleoffset = 0.0f;
	contrast = scalefactor = 1.0f;
	mode = 1;
	zoom = 1.0;
	pixelsize.high = pixelsize.low = 1.0f;
	workborderlimit = true;
	actual_color.setRgb(255, 255, 255);
	crosshairxpos = 0;
	crosshairypos = 0;
	marks = NULL;
	overlayalpha = 0.0f;
	//	vp=new vector<Point>;
	//	vp_old=new vector<Point>;
	selecttissue = new Q3Action("Select Tissue", 0, this);
	addmark = new Q3Action("&Add Mark", 0, this);
	addlabel = new Q3Action("Add &Label", 0, this);
	removemark = new Q3Action("&Remove Mark", 0, this);
	clearmarks = new Q3Action("&Clear Marks", 0, this);
	addtissue = new Q3Action("Add &Tissue", 0, this);
	addtissueconnected = new Q3Action("Add Tissue &Conn", 0, this);
	addtissue3D = new Q3Action("Add Tissue 3&D", 0, this);
	subtissue = new Q3Action("&Subtract Tissue", 0, this);
	addtissuelarger = new Q3Action("Add Tissue &Larger", 0, this);
	connect(addmark, SIGNAL(activated()), this, SLOT(add_mark()));
	connect(addlabel, SIGNAL(activated()), this, SLOT(add_label()));
	connect(clearmarks, SIGNAL(activated()), this, SLOT(clear_marks()));
	connect(removemark, SIGNAL(activated()), this, SLOT(remove_mark()));
	connect(addtissue, SIGNAL(activated()), this, SLOT(add_tissue()));
	connect(addtissueconnected, SIGNAL(activated()), this,
			SLOT(add_tissue_connected()));
	connect(subtissue, SIGNAL(activated()), this, SLOT(sub_tissue()));
	connect(addtissue3D, SIGNAL(activated()), this, SLOT(add_tissue_3D()));
	connect(addtissuelarger, SIGNAL(activated()), this,
			SLOT(add_tissuelarger()));
	connect(selecttissue, SIGNAL(activated()), this, SLOT(select_tissue()));

	return;
}

bmptissuemarklineshower::~bmptissuemarklineshower()
{
	delete addmark;
	delete addlabel;
	delete removemark;
	delete clearmarks;
	delete addtissue;
	delete addtissueconnected;
	delete addtissue3D;
	delete subtissue;
	delete addtissuelarger;
	delete selecttissue;
}

void bmptissuemarklineshower::mode_changed(unsigned char newmode,
										   bool updatescale)
{
	if (newmode != 0 && mode != newmode)
	{
		mode = newmode;
		if (updatescale)
		{
			update_scaleoffsetfactor();
		}
	}
}

void bmptissuemarklineshower::get_scaleoffsetfactor(float& offset1,
													float& factor1)
{
	offset1 = scaleoffset;
	factor1 = scalefactor;
}

void bmptissuemarklineshower::set_zoom(double z)
{
	if (z != zoom)
	{
		QPoint oldCenter = visibleRegion().boundingRect().center();

		QPoint newCenter;
		if (mousePosZoom.x() == 0 && mousePosZoom.y() == 0)
			newCenter =
				QPoint(z * oldCenter.x() / zoom, z * oldCenter.y() / zoom);
		else
		{
			QPoint oldDiff;
			oldDiff = oldCenter - mousePosZoom;
			newCenter = z * mousePosZoom / zoom + oldDiff;
		}

		zoom = z;
		int w = (int)width * (zoom * pixelsize.high);
		int h = (int)height * (zoom * pixelsize.low);
		setFixedSize(w, h);
		if (mousePosZoom.x() != 0 && mousePosZoom.y() != 0)
			//if( isBmp )
			emit setcenter_sign(newCenter.x(), newCenter.y());
	}
}

void bmptissuemarklineshower::pixelsize_changed(Pair pixelsize1)
{
	if (pixelsize1.high != pixelsize.high || pixelsize1.low != pixelsize.low)
	{
		pixelsize = pixelsize1;
		setFixedSize((int)width * zoom * pixelsize.high,
					 (int)height * zoom * pixelsize.low);
		repaint();
	}
}

void bmptissuemarklineshower::paintEvent(QPaintEvent* e)
{
	marks = handler3D->get_activebmphandler()->return_marks();
	if (image.size() != QSize(0, 0)) // is an image loaded?
	{
		{
			QPainter painter(this);
			painter.setClipRect(e->rect());

			painter.scale(zoom * pixelsize.high, zoom * pixelsize.low);

			//		if(showvp)
			painter.drawImage(0, 0, image_decorated);

			//		else
			//			painter.drawImage(0, 0, image);
			//		QColor qc(int(255),int(255),int(255));
			//		painter.setPen(QPen(qc));
			painter.setPen(QPen(actual_color));

			if (marks != NULL)
			{
				//		int d=(int)(2.0/zoom+0.5);
				unsigned char r, g, b;
				for (vector<Mark>::iterator it = marks->begin();
					 it != marks->end(); it++)
				{
					TissueInfos::GetTissueColorRGB(it->mark, r, g, b);
					QColor qc1(r, g, b);
					painter.setPen(QPen(qc1));
					//			const QPen pen1=painter.pen();
					/*			pen1.setColor(qc);
					pen1.setWidth(1);
					painter.setPen(pen1);*/
					painter.drawLine(
						int(it->p.px) - 2, int(height - it->p.py) - 3,
						int(it->p.px) + 2, int(height - it->p.py) + 1);
					painter.drawLine(
						int(it->p.px) - 2, int(height - it->p.py) + 1,
						int(it->p.px) + 2, int(height - it->p.py) - 3);
					if (it->name != std::string(""))
					{
						painter.drawText(int(it->p.px) + 3,
										 int(height - it->p.py) + 1,
										 QString(it->name.c_str()));
					}

					//			painter.drawLine( int(it->p.px)-d,int(height-it->p.py-1)-d,int(it->p.px)+d,int(height-it->p.py-1)+d);
					//			painter.drawLine( int(it->p.px)-d,int(height-it->p.py-1)+d,int(it->p.px)+d,int(height-it->p.py-1)-d);
				}
			}
		}
		{
			QPainter painter1(this);

			float dx = zoom * pixelsize.high;
			float dy = zoom * pixelsize.low;

			for (vector<Point>::iterator it = vpdyn.begin(); it != vpdyn.end();
				 it++)
			{
				//			painter.drawPoint(int(it->px),int(height-it->py-1));
				//			painter.fillRect(int(it->px),int(height-it->py-1),2,2,actual_color);
				painter1.fillRect(
					int(dx * it->px), int(dy * (height - it->py - 1)),
					int(dx + 0.999f), int(dy + 0.999f), actual_color);
				/*			int x1=int(dx*it->px);
				do {
				int y1=int(height-it->py-1);
				do {
				painter1.drawPoint(x1,y1);
				y1++;
				} while(y1<int(dy*(height-it->py)));
				x1++;
				} while(x1<int(dx*(it->px+1)));*/
				/*			for(int x1=int(zoom*it->px);x1<int(zoom*(it->px+1));x1++){
				for(int y1=int(zoom*(height-it->py-1));y1<int(zoom*(height-it->py));y1++){
				painter1.drawPoint(x1,y1);
				}
				}*/
			}
			/*		if(showvp&&bmporwork){
			for(vector<Point>::iterator it=vp.begin();it!=vp.end();it++){
			painter.drawPoint(int(it->px),int(height-it->py-1));
			}
			}*/
		}
	}
}

void bmptissuemarklineshower::bmp_changed() { update(); }

void bmptissuemarklineshower::slicenr_changed()
{
	//	if(activeslice!=handler3D->get_activeslice()){
	activeslice = handler3D->get_activeslice();
	bmphand_changed(handler3D->get_activebmphandler());
	//	}
}

void bmptissuemarklineshower::bmphand_changed(bmphandler* bmph)
{
	bmphand = bmph;
	if (bmporwork)
		bmpbits = bmph->return_bmpfield();
	else
		bmpbits = bmph->return_workfield();
	tissue = bmph->return_tissuefield(handler3D->get_active_tissuelayer());
	marks = bmph->return_marks();

	mode_changed(bmph->return_mode(bmporwork), false);
	update_scaleoffsetfactor();

	reload_bits();
	if (workborder)
	{
		if (bmporwork)
			workborder_changed();
	}
	else
		repaint();
	return;
}

void bmptissuemarklineshower::overlay_changed()
{
	reload_bits();
	repaint();
}

void bmptissuemarklineshower::overlay_changed(QRect rect)
{
	reload_bits();
	repaint((int)(rect.left() * zoom * pixelsize.high),
			(int)((height - 1 - rect.bottom()) * zoom * pixelsize.low),
			(int)ceil(rect.width() * zoom * pixelsize.high),
			(int)ceil(rect.height() * zoom * pixelsize.low));
}

void bmptissuemarklineshower::update()
{
	QRect rect;
	rect.setLeft(0);
	rect.setTop(0);
	rect.setRight(width - 1);
	rect.setBottom(height - 1);
	update(rect);
}

void bmptissuemarklineshower::update(QRect rect)
{
	bmphand = handler3D->get_activebmphandler();
	overlaybits = handler3D->return_overlay();
	mode_changed(bmphand->return_mode(bmporwork), false);
	update_scaleoffsetfactor();
	if (bmporwork)
		bmpbits = bmphand->return_bmpfield();
	else
		bmpbits = bmphand->return_workfield();
	tissue = bmphand->return_tissuefield(handler3D->get_active_tissuelayer());
	marks = bmphand->return_marks();

	if (bmphand->return_width() != width || bmphand->return_height() != height)
	{
		//		marks=bmphand->return_marks();
		vp.clear();
		vp_old.clear();
		vp1.clear();
		vp1_old.clear();
		vpdyn.clear();
		vpdyn_old.clear();
		vm.clear();
		vm_old.clear();
		width = bmphand->return_width();
		height = bmphand->return_height();
		image.create(int(width), int(height), 32);
		image_decorated.create(int(width), int(height), 32);
		setFixedSize((int)width * zoom * pixelsize.high,
					 (int)height * zoom * pixelsize.low);

		if (bmporwork && workborder)
		{
			reload_bits();
			workborder_changed();
			//			repaint();
			return;
		}
	}

	reload_bits();
	repaint((int)(rect.left() * zoom * pixelsize.high),
			(int)((height - 1 - rect.bottom()) * zoom * pixelsize.low),
			(int)ceil(rect.width() * zoom * pixelsize.high),
			(int)ceil(rect.height() * zoom * pixelsize.low));
}

void bmptissuemarklineshower::init(SlicesHandler* hand3D, bool bmporwork1)
{
	handler3D = hand3D;
	activeslice = handler3D->get_activeslice();
	bmphand = handler3D->get_activebmphandler();
	overlaybits = handler3D->return_overlay();
	bmporwork = bmporwork1;
	if (bmporwork)
		bmpbits = bmphand->return_bmpfield();
	else
		bmpbits = bmphand->return_workfield();
	tissue = bmphand->return_tissuefield(hand3D->get_active_tissuelayer());
	width = bmphand->return_width();
	height = bmphand->return_height();
	marks = bmphand->return_marks();
	image.create(int(width), int(height), 32);
	image_decorated.create(int(width), int(height), 32);

	//	image.setNumColors(256) ;
	/*	for(int i=0;i<256;i++){
	image.setColor(i, qRgb(i,i,i));
	}*/

	setFixedSize((int)width * zoom * pixelsize.high,
				 (int)height * zoom * pixelsize.low);
	setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));

	mode_changed(bmphand->return_mode(bmporwork), false);
	update_range();
	update_scaleoffsetfactor();

	reload_bits();
	if (workborder)
		if (bmporwork)
			workborder_changed();
		else
			repaint();
	show();
	return;
}

void bmptissuemarklineshower::update_range()
{
	// Recompute ranges for all slices
	if (bmporwork)
	{
		handler3D->compute_bmprange_mode1(&range_mode1);
	}
	else
	{
		handler3D->compute_range_mode1(&range_mode1);
	}
}

void bmptissuemarklineshower::update_range(unsigned short slicenr)
{
	// Recompute range only for single slice
	if (bmporwork)
	{
		handler3D->compute_bmprange_mode1(slicenr, &range_mode1);
	}
	else
	{
		handler3D->compute_range_mode1(slicenr, &range_mode1);
	}
}

void bmptissuemarklineshower::reload_bits()
{
	auto color_lut = handler3D->GetColorLookupTable();

	float* bmpbits1 = *bmpbits;
	tissues_size_t* tissue1 = *tissue;
	unsigned pos = 0;
	int f;
	unsigned char r, g, b;

	for (int y = height - 1; y >= 0; y--)
	{
		for (int x = 0; x < width; x++)
		{
			if (picturevisible)
			{
				if (color_lut && bmporwork)
				{
					// \todo not sure if we should allow to 'scale & offset & clamp' when a color lut is available
					//f = max(0.0f, min(255.0f, scaleoffset + scalefactor * (bmpbits1)[pos]));
					color_lut->GetColor((bmpbits1)[pos], r, g, b);
				}
				else
				{
					r = g = b = (int)max(
						0.0f, min(255.0f,
								  scaleoffset + scalefactor * (bmpbits1)[pos]));
				}

				// overlay only visible if picture is visible
				if (overlayvisible)
				{
					f = max(0.0f,
							min(255.0f, scaleoffset +
											scalefactor * (overlaybits)[pos]));

					r = (1.0f - overlayalpha) * r + overlayalpha * f;
					g = (1.0f - overlayalpha) * g + overlayalpha * f;
					b = (1.0f - overlayalpha) * b + overlayalpha * f;
				}
			}
			else
			{
				r = g = b = 0;
			}

			if (tissuevisible && tissue1[pos] != 0)
			{
				// blend with tissue color
				float* rgbo = TissueInfos::GetTissueColor(tissue1[pos]);
				float alpha = 0.5f; // rgbo[3];
				r = static_cast<unsigned char>(r +
											   alpha * (255.0f * rgbo[0] - r));
				g = static_cast<unsigned char>(g +
											   alpha * (255.0f * rgbo[1] - g));
				b = static_cast<unsigned char>(b +
											   alpha * (255.0f * rgbo[2] - b));
				image.setPixel(x, y, qRgb(r, g, b));
			}
			else // no tissue
			{
				image.setPixel(x, y, qRgb(r, g, b));
			}
			pos++;
		}
	}

	// copy to decorated image
	image_decorated = image;

	// now decorate
	QRgb color_used = actual_color.rgb();
	QRgb color_dim = (actual_color.light(30)).rgb();

	if (workborder && bmporwork &&
		((!workborderlimit) ||
		 (unsigned)vp.size() < unsigned(width) * height / 5))
	{
		for (vector<Point>::iterator it = vp.begin(); it != vp.end(); it++)
		{
			image_decorated.setPixel(int(it->px), int(height - it->py - 1),
									 color_dim);
		}
	}

	for (vector<Point>::iterator it = vp1.begin(); it != vp1.end(); it++)
	{
		image_decorated.setPixel(int(it->px), int(height - it->py - 1),
								 color_used);
	}

	for (vector<Mark>::iterator it = vm.begin(); it != vm.end(); it++)
	{
		TissueInfos::GetTissueColorRGB(it->mark, r, g, b);
		image_decorated.setPixel(int(it->p.px), int(height - it->p.py - 1),
								 qRgb(r, g, b));
	}

	if (crosshairxvisible)
	{
		for (int x = 0; x < width; x++)
		{
			image_decorated.setPixel(x, height - 1 - crosshairxpos,
									 qRgb(0, 255, 0));
			image.setPixel(x, height - 1 - crosshairxpos, qRgb(0, 255, 0));
		}
	}

	if (crosshairyvisible)
	{
		for (int y = 0; y < height; y++)
		{
			image_decorated.setPixel(crosshairypos, y, qRgb(0, 255, 0));
			image.setPixel(crosshairypos, y, qRgb(0, 255, 0));
		}
	}

	return;
}

void bmptissuemarklineshower::tissue_changed()
{
	reload_bits();
	repaint();
}

void bmptissuemarklineshower::tissue_changed(QRect rect)
{
	reload_bits();
	repaint((int)(rect.left() * zoom * pixelsize.high),
			(int)((height - 1 - rect.bottom()) * zoom * pixelsize.low),
			(int)ceil(rect.width() * zoom * pixelsize.high),
			(int)ceil(rect.height() * zoom * pixelsize.low));
}

void bmptissuemarklineshower::mark_changed()
{
	marks = bmphand->return_marks();
	repaint();
}

bool bmptissuemarklineshower::toggle_tissuevisible()
{
	tissuevisible = !tissuevisible;
	update();
	return tissuevisible;
}

bool bmptissuemarklineshower::toggle_picturevisible()
{
	picturevisible = !picturevisible;
	update();
	return picturevisible;
}

bool bmptissuemarklineshower::toggle_markvisible()
{
	markvisible = !markvisible;
	repaint();
	return markvisible;
}

bool bmptissuemarklineshower::toggle_overlayvisible()
{
	overlayvisible = !overlayvisible;
	update();
	return overlayvisible;
}

/*bool bmptissuemarklineshower::toggle_outlinevisible()
{
xxxxxxxxxxxxxxxx
}*/

void bmptissuemarklineshower::set_tissuevisible(bool on)
{
	tissuevisible = on;
	update();
	return;
}

void bmptissuemarklineshower::set_picturevisible(bool on)
{
	picturevisible = on;
	update();
	return;
}

void bmptissuemarklineshower::set_markvisible(bool on)
{
	markvisible = on;
	repaint();
	return;
}

void bmptissuemarklineshower::set_overlayvisible(bool on)
{
	overlayvisible = on;
	update();
	return;
}

void bmptissuemarklineshower::set_overlayalpha(float alpha)
{
	overlayalpha = alpha;
	update();
	return;
}

/*void bmptissuemarklineshower::set_outlinevisible(bool on){
xxxxxxxxxxxxx
}*/

void bmptissuemarklineshower::add_mark()
{
	Point p;
	p.px = (unsigned short)eventx;
	p.py = (unsigned short)eventy;

	emit addmark_sign(p);
}

void bmptissuemarklineshower::add_label()
{
	bool ok;
	QString newText = QInputDialog::getText(
		"Label", "Enter a name for the label:", QLineEdit::Normal, "", &ok,
		this);
	if (ok)
	{
		Point p;
		p.px = (unsigned short)eventx;
		p.py = (unsigned short)eventy;
		emit addlabel_sign(p, newText.ascii());
	}
}

void bmptissuemarklineshower::clear_marks() { emit clearmarks_sign(); }

void bmptissuemarklineshower::remove_mark()
{
	Point p;
	p.px = (unsigned short)eventx;
	p.py = (unsigned short)eventy;
	emit removemark_sign(p);
}

void bmptissuemarklineshower::add_tissue()
{
	Point p;
	p.px = (unsigned short)eventx;
	p.py = (unsigned short)eventy;
	emit addtissue_sign(p);
}

void bmptissuemarklineshower::add_tissue_connected()
{
	Point p;
	p.px = (unsigned short)eventx;
	p.py = (unsigned short)eventy;
	emit addtissueconnected_sign(p);
}

void bmptissuemarklineshower::add_tissue_3D()
{
	Point p;
	p.px = (unsigned short)eventx;
	p.py = (unsigned short)eventy;
	emit addtissue3D_sign(p);
}

void bmptissuemarklineshower::sub_tissue()
{
	Point p;
	p.px = (unsigned short)eventx;
	p.py = (unsigned short)eventy;
	emit subtissue_sign(p);
}

void bmptissuemarklineshower::add_tissuelarger()
{
	Point p;
	p.px = (unsigned short)eventx;
	p.py = (unsigned short)eventy;
	emit addtissuelarger_sign(p);
}

void bmptissuemarklineshower::select_tissue()
{
	Point p;
	p.px = (unsigned short)eventx;
	p.py = (unsigned short)eventy;
	emit selecttissue_sign(p);
}

void bmptissuemarklineshower::zoom_in() { set_zoom(2 * zoom); }

void bmptissuemarklineshower::zoom_out() { set_zoom(0.5 * zoom); }

void bmptissuemarklineshower::unzoom() { set_zoom(1.0); }

double bmptissuemarklineshower::return_zoom() { return zoom; }

void bmptissuemarklineshower::contextMenuEvent(QContextMenuEvent* event)
{
	//	eventx=(event->x()/(zoom*pixelsize.high));
	//	eventy=height-1-(event->y()/(zoom*pixelsize.low));
	eventx =
		(int)max(min(width - 1.0, (event->x() / (zoom * pixelsize.high))), 0.0);
	eventy = (int)max(
		min(height - 1.0, height - 1 - (event->y() / (zoom * pixelsize.low))),
		0.0);

	Q3PopupMenu contextMenu(this);
	addmark->addTo(&contextMenu);
	addlabel->addTo(&contextMenu);
	removemark->addTo(&contextMenu);
	clearmarks->addTo(&contextMenu);
	selecttissue->addTo(&contextMenu);
	if (!bmporwork)
	{
		contextMenu.insertSeparator();
		addtissue->addTo(&contextMenu);
		subtissue->addTo(&contextMenu);
		addtissue3D->addTo(&contextMenu);
		addtissueconnected->addTo(&contextMenu);
		addtissuelarger->addTo(&contextMenu);
	}
	contextMenu.exec(event->globalPos());
}

void bmptissuemarklineshower::set_brightnesscontrast(float bright, float contr,
													 bool paint)
{
	brightness = bright;
	contrast = contr;
	update_scaleoffsetfactor();
	if (paint)
	{
		reload_bits();
		repaint();
	}
}

void bmptissuemarklineshower::update_scaleoffsetfactor()
{
	bmphandler* bmphand = handler3D->get_activebmphandler();
	if (bmporwork && handler3D->GetColorLookupTable())
	{
		// Disable scaling/offset for color mapped images, since it would break the lookup table
		scalefactor = 1.0f;
		scaleoffset = 0.0f;
	}
	else if (bmphand->return_mode(bmporwork) == 2)
	{
		// Mode 2 assumes the range [0, 255]
		scalefactor = contrast;
		scaleoffset = (127.5f - 255 * scalefactor) * (1.0f - brightness) +
					  127.5f * brightness;
	}
	else if (bmphand->return_mode(bmporwork) == 1)
	{
		// Mode 1 assumes an arbitrary range --> scale to range [0, 255]

		auto r = range_mode1;
		if (r.high == r.low)
		{
			r.high = r.low + 1.f;
		}
		scalefactor = 255.0f * contrast / (r.high - r.low);
		scaleoffset = (127.5f - r.high * scalefactor) * (1.0f - brightness) +
					  (127.5f - r.low * scalefactor) * brightness;
	}
	emit scaleoffsetfactor_changed(scaleoffset, scalefactor, bmporwork);
}

/*void bmptissuemarklineshower::mP(QPaintEvent?*e)
{
e;
return;
}*/

void bmptissuemarklineshower::mousePressEvent(QMouseEvent* e)
{
	Point p;
	//	p.px=(unsigned short)(e->x()/(zoom*pixelsize.high));
	//	p.py=(unsigned short)height-1-(e->y()/(zoom*pixelsize.low));
	p.px = (unsigned short)max(
		min(width - 1.0, (e->x() / (zoom * pixelsize.high))), 0.0);
	p.py = (unsigned short)max(
		min(height - 1.0, height - ((e->y() + 1) / (zoom * pixelsize.low))),
		0.0);

	if (e->button() == Qt::LeftButton)
	{
		emit mousepressed_sign(p);
	}
	else if (e->button() == Qt::MidButton)
	{
		emit mousepressedmid_sign(p);
	}
}

void bmptissuemarklineshower::mouseReleaseEvent(QMouseEvent* e)
{
	if (e->button() == Qt::LeftButton)
	{
		Point p;
		//		p.px=(unsigned short)(e->x()/(zoom*pixelsize.high));
		//		p.py=(unsigned short)height-1-(e->y()/(zoom*pixelsize.low));
		p.px = (unsigned short)max(
			min(width - 1.0, (e->x() / (zoom * pixelsize.high))), 0.0);
		p.py = (unsigned short)max(
			min(height - 1.0, height - ((e->y() + 1) / (zoom * pixelsize.low))),
			0.0);

		emit mousereleased_sign(p);
	}
}

void bmptissuemarklineshower::mouseDoubleClickEvent(QMouseEvent* e)
{
	//	if(e->button() == Qt::LeftButton){
	Point p;
	//		p.px=(unsigned short)(e->x()/(zoom*pixelsize.high));
	//		p.py=(unsigned short)height-1-(e->y()/(zoom*pixelsize.low));
	p.px = (unsigned short)max(
		min(width - 1.0, (e->x() / (zoom * pixelsize.high))), 0.0);
	p.py = (unsigned short)max(
		min(height - 1.0, height - ((e->y() + 1) / (zoom * pixelsize.low))),
		0.0);

	if (e->button() == Qt::LeftButton)
	{
		emit mousedoubleclick_sign(p);
	}
	else if (e->button() == Qt::MidButton)
	{
		emit mousedoubleclickmid_sign(p);
	}
	//	}
}

void bmptissuemarklineshower::mouseMoveEvent(QMouseEvent* e)
{
	Point p;
	p.px = (unsigned short)max(
		min(width - 1.0, (e->x() / (zoom * pixelsize.high))), 0.0);
	p.py = (unsigned short)max(
		min(height - 1.0, height - ((e->y() + 1) / (zoom * pixelsize.low))),
		0.0);

	emit mousemoved_sign(p);
}

void bmptissuemarklineshower::wheelEvent(QWheelEvent* e)
{
	int delta = e->delta();

	if (e->state() & Qt::ControlModifier)
	{
		mousePosZoom = e->pos();
		emit mousePosZoom_sign(mousePosZoom);
		emit wheelrotatedctrl_sign(delta);
	}
	else
		e->ignore();
}

void bmptissuemarklineshower::recompute_workborder()
{
	bmphand = handler3D->get_activebmphandler();
	vp.clear();
	Point p;

	float* bits = bmphand->return_work();
	unsigned pos = 0;

	if (bits[pos] != bits[pos + 1] || bits[pos] != bits[pos + width])
	{
		p.px = 0;
		p.py = 0;
		if (bits[pos] != 0)
			vp.push_back(p);
	}
	pos++;
	for (unsigned short j = 1; j + 1 < width; j++)
	{
		if (bits[pos] != bits[pos + 1] || bits[pos] != bits[pos - 1] ||
			bits[pos] != bits[pos + width])
		{
			p.px = j;
			p.py = 0;
			if (bits[pos] != 0)
				vp.push_back(p);
		}
		pos++;
	}
	if (bits[pos] != bits[pos - 1] || bits[pos] != bits[pos + width])
	{
		p.px = width - 1;
		p.py = 0;
		if (bits[pos] != 0)
			vp.push_back(p);
	}
	pos++;

	for (unsigned short i = 1; i + 1 < height; i++)
	{
		if (bits[pos] != bits[pos + 1] || bits[pos] != bits[pos + width] ||
			bits[pos] != bits[pos - width])
		{
			p.px = 0;
			p.py = i;
			if (bits[pos] != 0)
				vp.push_back(p);
		}
		pos++;
		for (unsigned short j = 1; j + 1 < width; j++)
		{
			if (bits[pos] != bits[pos + 1] || bits[pos] != bits[pos - 1] ||
				bits[pos] != bits[pos + width] ||
				bits[pos] != bits[pos - width])
			{
				p.px = j;
				p.py = i;
				if (bits[pos] != 0)
					vp.push_back(p);
			}
			pos++;
		}
		if (bits[pos] != bits[pos - 1] || bits[pos] != bits[pos + width] ||
			bits[pos] != bits[pos - width])
		{
			p.px = width - 1;
			p.py = i;
			if (bits[pos] != 0)
				vp.push_back(p);
		}
		pos++;
	}
	if (bits[pos] != bits[pos + 1] || bits[pos] != bits[pos - width])
	{
		p.px = 0;
		p.py = height - 1;
		if (bits[pos] != 0)
			vp.push_back(p);
	}
	pos++;
	for (unsigned short j = 1; j + 1 < width; j++)
	{
		if (bits[pos] != bits[pos + 1] || bits[pos] != bits[pos - 1] ||
			bits[pos] != bits[pos - width])
		{
			p.px = j;
			p.py = height - 1;
			if (bits[pos] != 0)
				vp.push_back(p);
		}
		pos++;
	}
	if (bits[pos] != bits[pos - 1] || bits[pos] != bits[pos - width])
	{
		p.px = width - 1;
		p.py = height - 1;
		if (bits[pos] != 0)
			vp.push_back(p);
	}
}

void bmptissuemarklineshower::workborder_changed()
{
	if (workborder)
	{
		recompute_workborder();
		vp_changed();
	}
}

void bmptissuemarklineshower::workborder_changed(QRect rect)
{
	if (workborder)
	{
		recompute_workborder();
		vp_changed(rect);
	}
}

void bmptissuemarklineshower::vp_to_image_decorator()
{
	if ((!workborderlimit) ||
		((unsigned)vp_old.size() < unsigned(width) * height / 5))
	{
		for (vector<Point>::iterator it = vp_old.begin(); it != vp_old.end();
			 it++)
		{
			image_decorated.setPixel(
				int(it->px), int(height - it->py - 1),
				image.pixel(int(it->px), int(height - it->py - 1)));
		}
	}

	QRgb color_used = actual_color.rgb();
	QRgb color_dim = (actual_color.light(30)).rgb();
	if ((!workborderlimit) ||
		((unsigned)vp.size() < unsigned(width) * height / 5))
	{
		for (vector<Point>::iterator it = vp.begin(); it != vp.end(); it++)
		{
			image_decorated.setPixel(int(it->px), int(height - it->py - 1),
									 color_dim);
		}
	}

	for (vector<Point>::iterator it = vp1_old.begin(); it != vp1_old.end();
		 it++)
	{
		image_decorated.setPixel(
			int(it->px), int(height - it->py - 1),
			image.pixel(int(it->px), int(height - it->py - 1)));
	}

	for (vector<Point>::iterator it = vp1.begin(); it != vp1.end(); it++)
	{
		image_decorated.setPixel(int(it->px), int(height - it->py - 1),
								 color_used);
	}

	for (vector<Mark>::iterator it = vm_old.begin(); it != vm_old.end(); it++)
	{
		image_decorated.setPixel(
			int(it->p.px), int(height - it->p.py - 1),
			image.pixel(int(it->p.px), int(height - it->p.py - 1)));
	}

	unsigned char r, g, b;
	for (vector<Mark>::iterator it = vm.begin(); it != vm.end(); it++)
	{
		TissueInfos::GetTissueColorRGB(it->mark, r, g, b);
		image_decorated.setPixel(int(it->p.px), int(height - it->p.py - 1),
								 qRgb(r, g, b));
	}
}

void bmptissuemarklineshower::vp_changed()
{
	vp_to_image_decorator();

	repaint();

	vp_old.clear();
	vp_old.insert(vp_old.begin(), vp.begin(), vp.end());

	vp1_old.clear();
	vp1_old.insert(vp1_old.begin(), vp1.begin(), vp1.end());

	vm_old.clear();
	vm_old.insert(vm_old.begin(), vm.begin(), vm.end());
}

void bmptissuemarklineshower::vp_changed(QRect rect)
{
	vp_to_image_decorator();

	if (rect.left() > 0)
		rect.setLeft(rect.left() - 1);
	if (rect.top() > 0)
		rect.setTop(rect.top() - 1);
	if (rect.right() + 1 < width)
		rect.setRight(rect.right() + 1);
	if (rect.bottom() + 1 < height)
		rect.setBottom(rect.bottom() + 1);
	repaint((int)(rect.left() * zoom * pixelsize.high),
			(int)((height - 1 - rect.bottom()) * zoom * pixelsize.low),
			(int)ceil(rect.width() * zoom * pixelsize.high),
			(int)ceil(rect.height() * zoom * pixelsize.low));

	vp_old.clear();
	vp_old.insert(vp_old.begin(), vp.begin(), vp.end());

	vp1_old.clear();
	vp1_old.insert(vp1_old.begin(), vp1.begin(), vp1.end());

	vm_old.clear();
	vm_old.insert(vm_old.begin(), vm.begin(), vm.end());
}

void bmptissuemarklineshower::vp1dyn_changed()
{
	if ((!workborderlimit) ||
		((unsigned)vp_old.size() < unsigned(width) * height / 5))
	{
		for (vector<Point>::iterator it = vp_old.begin(); it != vp_old.end();
			 it++)
		{
			image_decorated.setPixel(
				int(it->px), int(height - it->py - 1),
				image.pixel(int(it->px), int(height - it->py - 1)));
		}
	}

	QRgb color_used = actual_color.rgb();
	QRgb color_dim = (actual_color.light(30)).rgb();
	QRgb color_highlight = (actual_color.lighter(60)).rgb();
	if ((!workborderlimit) ||
		((unsigned)vp.size() < unsigned(width) * height / 5))
	{
		for (vector<Point>::iterator it = vp.begin(); it != vp.end(); it++)
		{
			image_decorated.setPixel(int(it->px), int(height - it->py - 1),
									 color_dim);
		}
	}

	for (vector<Point>::iterator it = vp1_old.begin(); it != vp1_old.end();
		 it++)
	{
		image_decorated.setPixel(
			int(it->px), int(height - it->py - 1),
			image.pixel(int(it->px), int(height - it->py - 1)));
	}

	for (vector<Point>::iterator it = vp1.begin(); it != vp1.end(); it++)
	{
		image_decorated.setPixel(int(it->px), int(height - it->py - 1),
								 color_used);
	}

	for (vector<Point>::iterator it = limit_points.begin();
		 it != limit_points.end(); it++)
	{
		image_decorated.setPixel(int(it->px), int(height - it->py - 1),
								 color_highlight);
	}

	for (vector<Mark>::iterator it = vm_old.begin(); it != vm_old.end(); it++)
	{
		image_decorated.setPixel(
			int(it->p.px), int(height - it->p.py - 1),
			image.pixel(int(it->p.px), int(height - it->p.py - 1)));
	}

	unsigned char r, g, b;
	for (vector<Mark>::iterator it = vm.begin(); it != vm.end(); it++)
	{
		TissueInfos::GetTissueColorRGB(it->mark, r, g, b);
		image_decorated.setPixel(int(it->p.px), int(height - it->p.py - 1),
								 qRgb(r, g, b));
	}

	repaint();

	vpdyn_old.clear();
	vpdyn_old.insert(vpdyn_old.begin(), vpdyn.begin(), vpdyn.end());

	vp_old.clear();
	vp_old.insert(vp_old.begin(), vp.begin(), vp.end());

	vp1_old.clear();
	vp1_old.insert(vp1_old.begin(), vp1.begin(), vp1.end());

	vm_old.clear();
	vm_old.insert(vm_old.begin(), vm.begin(), vm.end());
}

void bmptissuemarklineshower::vpdyn_changed()
{
	repaint();
	return;
	{
		QPainter painter1(this);

		float dx = zoom * pixelsize.high;
		float dy = zoom * pixelsize.low;

		if (dx >= 1.002f || dy >= 1.002f)
		{
			for (vector<Point>::iterator it = vpdyn_old.begin();
				 it != vpdyn_old.end(); it++)
			{
				QColor qc(image_decorated.pixel(int(it->px),
												int(height - it->py - 1)));
				painter1.fillRect(int(dx * it->px),
								  int(dy * (height - it->py - 1)),
								  int(dx + 0.999f), int(dy + 0.999f), qc);
			}

			for (vector<Point>::iterator it = vpdyn.begin(); it != vpdyn.end();
				 it++)
			{
				painter1.fillRect(
					int(dx * it->px), int(dy * (height - it->py - 1)),
					int(dx + 0.999f), int(dy + 0.999f), actual_color);
			}
		}
		else
		{
			for (vector<Point>::iterator it = vpdyn_old.begin();
				 it != vpdyn_old.end(); it++)
			{
				QColor qc(image_decorated.pixel(int(it->px),
												int(height - it->py - 1)));
				painter1.setPen(QPen(qc));
				painter1.drawPoint(int(dx * it->px),
								   int(dy * (height - it->py - 1)));
			}
			painter1.setPen(QPen(actual_color));
			for (vector<Point>::iterator it = vpdyn.begin(); it != vpdyn.end();
				 it++)
			{
				painter1.drawPoint(int(dx * it->px),
								   int(dy * (height - it->py - 1)));
			}
		}
	}

	vpdyn_old.clear();
	vpdyn_old.insert(vpdyn_old.begin(), vpdyn.begin(), vpdyn.end());
}

void bmptissuemarklineshower::set_workbordervisible(bool on)
{
	if (on)
	{
		if (bmporwork && !workborder)
		{
			workborder = true;
			workborder_changed();
		}
	}
	else
	{
		if (workborder && bmporwork)
		{
			vp.clear();
			workborder = false;
			reload_bits();
			repaint();
		}
	}
}

bool bmptissuemarklineshower::toggle_workbordervisible()
{
	if (workborder)
	{
		if (bmporwork)
		{
			vp.clear();
			workborder = false;
			reload_bits();
			repaint();
		}
	}
	else
	{
		if (bmporwork)
		{
			workborder = true;
			workborder_changed();
		}
	}

	return workborder;
}

bool bmptissuemarklineshower::return_workbordervisible() { return workborder; }

void bmptissuemarklineshower::set_vp1(vector<Point>* vp1_arg)
{
	vp1.clear();
	vp1.insert(vp1.begin(), vp1_arg->begin(), vp1_arg->end());
	vp_changed();
}

void bmptissuemarklineshower::clear_vp1()
{
	vp1.clear();
	vp_changed();
}

void bmptissuemarklineshower::set_vm(vector<Mark>* vm_arg)
{
	vm.clear();
	vm.insert(vm.begin(), vm_arg->begin(), vm_arg->end());
	vp_changed();
}

void bmptissuemarklineshower::clear_vm()
{
	vm.clear();
	vp_changed();
}

void bmptissuemarklineshower::set_vpdyn(vector<Point>* vpdyn_arg)
{
	vpdyn.clear();
	vpdyn.insert(vpdyn.begin(), vpdyn_arg->begin(), vpdyn_arg->end());
	vpdyn_changed();
}

void bmptissuemarklineshower::clear_vpdyn()
{
	vpdyn.clear();
	vpdyn_changed();
}

void bmptissuemarklineshower::set_vp1_dyn(vector<Point>* vp1_arg,
										  vector<Point>* vpdyn_arg,
										  const bool also_points /*= false*/)
{
	vp1.clear();
	vp1.insert(vp1.begin(), vp1_arg->begin(), vp1_arg->end());
	vpdyn.clear();
	vpdyn.insert(vpdyn.begin(), vpdyn_arg->begin(), vpdyn_arg->end());
	limit_points.clear();
	if (also_points && vp1.size() > 1)
	{
		limit_points.push_back(vp1.front());
		limit_points.push_back(vp1.back());
	}
	vp1dyn_changed();
}

void bmptissuemarklineshower::color_changed(int tissue)
{
	unsigned char r, g, b;
	TissueInfos::GetTissueColorRGB(tissue + 1, r, g, b);
	actual_color.setRgb(r, g, b);
	vp_changed();

	return;
}

void bmptissuemarklineshower::crosshairx_changed(int i)
{
	if (i < height)
	{
		crosshairxpos = i;

		if (crosshairxvisible)
		{
			reload_bits();
			repaint();
		}
	}
}

void bmptissuemarklineshower::crosshairy_changed(int i)
{
	if (i < width)
	{
		crosshairypos = i;

		if (crosshairyvisible)
		{
			reload_bits();
			repaint();
		}
	}
}

void bmptissuemarklineshower::set_crosshairxvisible(bool on)
{
	if (crosshairxvisible != on)
	{
		crosshairxvisible = on;
		reload_bits();
		repaint();
	}
}

void bmptissuemarklineshower::set_crosshairyvisible(bool on)
{
	if (crosshairyvisible != on)
	{
		crosshairyvisible = on;
		reload_bits();
		repaint();
	}
}

volumeviewer3D::volumeviewer3D(SlicesHandler* hand3D1, bool bmportissue1,
							   bool gpu_or_raycast, bool shade1,
							   QWidget* parent, const char* name,
							   Qt::WindowFlags wFlags)
	: QWidget(parent, name, wFlags)
{
	bmportissue = bmportissue1;
	hand3D = hand3D1;
	vbox1 = new Q3VBox(this);
	vtkWidget = new QVTKWidget(vbox1);
	vtkWidget->setFixedSize(600, 600);
	//	resize( QSize(600, 600).expandedTo(minimumSizeHint()) );

	cb_shade = new QCheckBox("Shade", vbox1);
	cb_shade->setChecked(shade1);
	cb_shade->show();
	cb_raytraceortexturemap = new QCheckBox("GPU / Software", vbox1);
	cb_raytraceortexturemap->setChecked(gpu_or_raycast);
	cb_raytraceortexturemap->show();

	cb_showslices = new QCheckBox("Show Slices", vbox1);
	cb_showslices->setChecked(true);
	cb_showslices->show();
	cb_showslice1 = new QCheckBox("Enable Slicer 1", vbox1);
	cb_showslice1->setChecked(false);
	cb_showslice1->show();
	cb_showslice2 = new QCheckBox("Enable Slicer 2", vbox1);
	cb_showslice2->setChecked(false);
	cb_showslice2->show();
	cb_showvolume = new QCheckBox("Show Volume", vbox1);
	cb_showvolume->setChecked(true);
	cb_showvolume->show();

	hbox1 = new Q3HBox(vbox1);
	lb_contr = new QLabel("Window:", hbox1);
	sl_contr = new QSlider(Qt::Horizontal, hbox1);
	sl_contr->setRange(0, 100);
	sl_contr->setValue(100);
	hbox1->setFixedHeight(hbox1->sizeHint().height());
	hbox2 = new Q3HBox(vbox1);
	lb_bright = new QLabel("Level:", hbox2);
	sl_bright = new QSlider(Qt::Horizontal, hbox2);
	sl_bright->setRange(0, 100);
	sl_bright->setValue(50);
	hbox2->setFixedHeight(hbox2->sizeHint().height());
	hbox3 = new Q3HBox(vbox1);
	lb_trans = new QLabel("Transp.:", hbox3);
	sl_trans = new QSlider(Qt::Horizontal, hbox3);
	sl_trans->setRange(0, 100);
	sl_trans->setValue(50);
	hbox3->setFixedHeight(hbox3->sizeHint().height());

	bt_update = new QPushButton("Update", vbox1);
	bt_update->show();

	if (bmportissue)
	{
		hbox3->hide();
		hbox1->show();
		hbox2->show();
	}
	else
	{
		hbox3->show();
		hbox1->hide();
		hbox2->hide();
	}

	vbox1->show();

	//	setSizePolicy(QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed));
	//	vbox1->setFixedSize(vbox1->sizeHint());
	resize(vbox1->sizeHint().expandedTo(minimumSizeHint()));

	QObject::connect(cb_shade, SIGNAL(clicked()), this, SLOT(shade_changed()));
	QObject::connect(cb_raytraceortexturemap, SIGNAL(clicked()), this,
					 SLOT(raytraceortexturemap_changed()));
	QObject::connect(bt_update, SIGNAL(clicked()), this, SLOT(reload()));
	QObject::connect(cb_showslices, SIGNAL(clicked()), this,
					 SLOT(showslices_changed()));
	QObject::connect(cb_showslice1, SIGNAL(clicked()), this,
					 SLOT(showslices_changed()));
	QObject::connect(cb_showslice2, SIGNAL(clicked()), this,
					 SLOT(showslices_changed()));
	QObject::connect(cb_showvolume, SIGNAL(clicked()), this,
					 SLOT(showvolume_changed()));

	QObject::connect(sl_bright, SIGNAL(sliderReleased()), this,
					 SLOT(contrbright_changed()));
	QObject::connect(sl_contr, SIGNAL(sliderReleased()), this,
					 SLOT(contrbright_changed()));
	QObject::connect(sl_trans, SIGNAL(sliderReleased()), this,
					 SLOT(transp_changed()));

	//   vtkStructuredPointsReader* reader = vtkStructuredPointsReader::New();
	//  reader= vtkSmartPointer<vtkXMLImageDataReader>::New();
	//  fnamei="D:\\Development\\bmp_read_QT\\bmp_read_3D_8\\qvtkViewer\\test2.vti";
	//  reader->SetFileName(fnamei.c_str());

	//  vtkImageData* input = (vtkImageData*)reader->GetOutput();
	//  input->Update();

	input = vtkSmartPointer<vtkImageData>::New();
	input->SetExtent(0, (int)hand3D->return_width() - 1, 0,
					 (int)hand3D->return_height() - 1, 0,
					 (int)hand3D->return_nrslices() - 1);
	Pair ps = hand3D->get_pixelsize();
	if (bmportissue)
	{
		input->SetSpacing(ps.high, ps.low, hand3D->get_slicethickness());
		input->AllocateScalars(VTK_FLOAT, 1);
		float* field = (float*)input->GetScalarPointer(0, 0, 0);
		for (unsigned short i = 0; i < hand3D->return_nrslices(); i++)
		{
			hand3D->copyfrombmp(
				i, &(field[i * (unsigned long long)hand3D->return_area()]));
		}
	}
	else
	{
		switch (sizeof(tissues_size_t))
		{
		case 1: input->AllocateScalars(VTK_UNSIGNED_CHAR, 1); break;
		case 2: input->AllocateScalars(VTK_UNSIGNED_SHORT, 1); break;
		default:
			cerr << "volumeviewer3D::volumeviewer3D: tissues_size_t not "
					"implemented."
				 << endl;
		}
		input->SetSpacing(ps.high, ps.low, hand3D->get_slicethickness());
		tissues_size_t* field =
			(tissues_size_t*)input->GetScalarPointer(0, 0, 0);
		for (unsigned short i = 0; i < hand3D->return_nrslices(); i++)
		{
			hand3D->copyfromtissue(
				i, &(field[i * (unsigned long long)hand3D->return_area()]));
		}
	}

	double bounds[6], center[3];

	input->GetBounds(bounds);
	input->GetCenter(center);
	input->GetScalarRange(range);
	cerr << "input range = " << range[0] << " " << range[1] << endl;

	double level = 0.5 * (range[1] + range[0]);
	double window = range[1] - range[0];
	if (range[1] > 250)
	{
		level = 125;
		window = 250;
		sl_contr->setValue((101 * window / (range[1] - range[0])) - 1);
		sl_bright->setValue(100 * (level - range[0]) / (range[1] - range[0]));
	}

	//
	// Put a bounding box in the scene to help orient the user.
	//
	outlineGrid = vtkSmartPointer<vtkOutlineFilter>::New();
	outlineGrid->SetInputData(input);
	outlineGrid->Update();
	outlineMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	outlineMapper->SetInputConnection(outlineGrid->GetOutputPort());
	outlineActor = vtkSmartPointer<vtkActor>::New();
	outlineActor->SetMapper(outlineMapper);

	// Add a PlaneWidget to interactively move the slice plane.
	planeWidgetY = vtkSmartPointer<vtkImplicitPlaneWidget>::New();
	planeWidgetY->SetInputData(input);
	planeWidgetY->NormalToYAxisOn();
	planeWidgetY->DrawPlaneOff();
	planeWidgetY->SetScaleEnabled(0);
	planeWidgetY->SetOriginTranslation(0);
	planeWidgetY->SetOutsideBounds(0);

	// Attach a Cutter to the slice plane to sample the function.
	sliceCutterY = vtkSmartPointer<vtkCutter>::New();
	sliceCutterY->SetInputData(input);
	slicePlaneY = vtkSmartPointer<vtkPlane>::New();
	planeWidgetY->GetPlane(slicePlaneY);
	sliceCutterY->SetCutFunction(slicePlaneY);

	// Add a PlaneWidget to interactively move the slice plane.
	planeWidgetZ = vtkSmartPointer<vtkImplicitPlaneWidget>::New();
	planeWidgetZ->SetInputData(input);
	planeWidgetZ->NormalToZAxisOn();
	planeWidgetZ->DrawPlaneOff();
	planeWidgetZ->SetScaleEnabled(0);
	planeWidgetZ->SetOriginTranslation(0);
	planeWidgetZ->SetOutsideBounds(0);

	// Attach a Cutter to the slice plane to sample the function.
	sliceCutterZ = vtkSmartPointer<vtkCutter>::New();
	sliceCutterZ->SetInputData(input);
	slicePlaneZ = vtkSmartPointer<vtkPlane>::New();
	planeWidgetZ->GetPlane(slicePlaneZ);
	sliceCutterZ->SetCutFunction(slicePlaneZ);

	//
	// Create a lookup table to map the velocity magnitudes.
	//
	if (bmportissue)
	{
		lut = vtkSmartPointer<vtkLookupTable>::New();
		lut->SetTableRange(range);
		lut->SetHueRange(0, 0);
		lut->SetSaturationRange(0, 0);
		lut->SetValueRange(0, 1);
		lut->Build(); //effective build
	}
	else
	{
		lut = vtkSmartPointer<vtkLookupTable>::New();
		tissues_size_t tissuecount = TissueInfos::GetTissueCount();
		lut->SetNumberOfColors(tissuecount + 1);
		lut->Build();
		lut->SetTableValue(0, 0.0, 0.0, 0.0, 0.0);
		float* tissuecolor;
		for (tissues_size_t i = 1; i <= tissuecount; i++)
		{
			tissuecolor = TissueInfos::GetTissueColor(i);
			lut->SetTableValue(i, tissuecolor[0], tissuecolor[1],
							   tissuecolor[2], 1.0);
		}
	}
	//
	// Map the slice plane and create the geometry to be rendered.
	//
	sliceY = vtkSmartPointer<vtkPolyDataMapper>::New();
	if (!bmportissue)
		sliceY->SetColorModeToMapScalars();
	sliceY->SetInputConnection(sliceCutterY->GetOutputPort());
	//   input->GetScalarRange( range );
	//range[1] *= 0.7; // reduce the upper range by 30%
	sliceY->SetScalarRange(range);
	sliceY->SetLookupTable(lut);
	//   sliceY->SelectColorArray(argv[2]);
	sliceY->Update();
	sliceActorY = vtkSmartPointer<vtkActor>::New();
	sliceActorY->SetMapper(sliceY);

	sliceZ = vtkSmartPointer<vtkPolyDataMapper>::New();
	if (!bmportissue)
		sliceZ->SetColorModeToMapScalars();
	sliceZ->SetInputConnection(sliceCutterZ->GetOutputPort());
	//   input->GetScalarRange( range );
	//range[1] *= 0.7; // reduce the upper range by 30%
	sliceZ->SetScalarRange(range);
	sliceZ->SetLookupTable(lut);
	sliceZ->Update();
	sliceActorZ = vtkSmartPointer<vtkActor>::New();
	sliceActorZ->SetMapper(sliceZ);

	if (!bmportissue)
	{
		opacityTransferFunction = vtkSmartPointer<vtkPiecewiseFunction>::New();
		opacityTransferFunction->AddPoint(0, 0);
		tissues_size_t tissuecount = TissueInfos::GetTissueCount();
		for (tissues_size_t i = 1; i <= tissuecount; i++)
		{
			float opac1 = TissueInfos::GetTissueOpac(i);
			if (sl_trans->value() > 50)
				opac1 = opac1 * (100 - sl_trans->value()) / 50;
			else
				opac1 = 1.0f - (1.0 - opac1) * sl_trans->value() / 50;
			opacityTransferFunction->AddPoint(i, opac1);
		}

		colorTransferFunction =
			vtkSmartPointer<vtkColorTransferFunction>::New();
		colorTransferFunction->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
		float* tissuecolor;
		for (tissues_size_t i = 1; i <= tissuecount; i++)
		{
			tissuecolor = TissueInfos::GetTissueColor(i);
			colorTransferFunction->AddRGBPoint((double)i - 0.1, tissuecolor[0],
											   tissuecolor[1], tissuecolor[2]);
		}
		tissuecolor = TissueInfos::GetTissueColor(tissuecount);
		colorTransferFunction->AddRGBPoint((double)tissuecount + 0.1,
										   tissuecolor[0], tissuecolor[1],
										   tissuecolor[2]);
	}
	else
	{
		opacityTransferFunction = vtkSmartPointer<vtkPiecewiseFunction>::New();
		opacityTransferFunction->AddPoint(level - window / 2, 0.0);
		opacityTransferFunction->AddPoint(level + window / 2, 1.0);
		colorTransferFunction =
			vtkSmartPointer<vtkColorTransferFunction>::New();
		colorTransferFunction->AddRGBPoint(level - window / 2, 0.0, 0.0, 0.0);
		colorTransferFunction->AddRGBPoint(level + window / 2, 1.0, 1.0, 1.0);
	}

	volumeProperty = vtkSmartPointer<vtkVolumeProperty>::New();
	volumeProperty->SetColor(colorTransferFunction);
	volumeProperty->SetScalarOpacity(opacityTransferFunction);
	if (shade1)
		volumeProperty->ShadeOn();
	else
		volumeProperty->ShadeOff();
	if (bmportissue)
		volumeProperty->SetInterpolationTypeToLinear();
	else
		volumeProperty->SetInterpolationTypeToNearest();

	volumeMapper = vtkSmartPointer<vtkSmartVolumeMapper>::New();
	volumeMapper->SetRequestedRenderModeToDefault();

	cast = vtkSmartPointer<vtkImageShiftScale>::New();
	if (bmportissue)
	{
		double slope = 255 / (range[1] - range[0]);
		double shift = -range[0];
		cast->SetInputData(input);
		cast->SetShift(shift);
		cast->SetScale(slope);
		cast->SetOutputScalarTypeToUnsignedShort();
		volumeMapper->SetInputConnection(cast->GetOutputPort());
	}
	else
	{
		volumeMapper->SetInputData(input);
	}

	volume = vtkSmartPointer<vtkVolume>::New();
	if (gpu_or_raycast)
		volumeMapper->SetRequestedRenderModeToDefault();
	else
		volumeMapper->SetRequestedRenderModeToRayCast();

	volume->SetMapper(volumeMapper);
	volume->SetProperty(volumeProperty);

	//
	// Create the renderers and assign actors to them.
	//
	ren3D = vtkSmartPointer<vtkRenderer>::New();
	ren3D->AddActor(sliceActorY);
	ren3D->AddActor(sliceActorZ);
	ren3D->AddVolume(volume);

	ren3D->AddActor(outlineActor);
	ren3D->SetBackground(0, 0, 0);
	ren3D->SetViewport(0.0, 0.0, 1.0, 1.0);

	renWin = vtkSmartPointer<vtkRenderWindow>::New();

	vtkWidget->GetRenderWindow()->AddRenderer(ren3D);
	vtkWidget->GetRenderWindow()->LineSmoothingOn();
	vtkWidget->GetRenderWindow()->SetSize(600, 600);

	style = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();

	iren = vtkSmartPointer<QVTKInteractor>::New();
	iren->SetInteractorStyle(style);

	iren->SetRenderWindow(vtkWidget->GetRenderWindow());

	my_sliceDataY = vtkSmartPointer<vtkMySliceCallbackY>::New();
	my_sliceDataY->slicePlaneY1 = slicePlaneY;
	my_sliceDataY->sliceCutterY1 = sliceCutterY;
	planeWidgetY->SetInteractor(iren);
	planeWidgetY->AddObserver(vtkCommand::InteractionEvent, my_sliceDataY);
	planeWidgetY->AddObserver(vtkCommand::EnableEvent, my_sliceDataY);
	planeWidgetY->AddObserver(vtkCommand::StartInteractionEvent, my_sliceDataY);
	planeWidgetY->AddObserver(vtkCommand::EndPickEvent, my_sliceDataY);
	planeWidgetY->SetPlaceFactor(1.0);
	planeWidgetY->PlaceWidget();
	planeWidgetY->SetOrigin(center);
	planeWidgetY->SetEnabled(cb_showslice1->isOn());
	planeWidgetY->PlaceWidget(bounds);

	my_sliceDataZ = vtkSmartPointer<vtkMySliceCallbackZ>::New();
	my_sliceDataZ->slicePlaneZ1 = slicePlaneZ;
	my_sliceDataZ->sliceCutterZ1 = sliceCutterZ;
	planeWidgetZ->SetInteractor(iren);
	planeWidgetZ->AddObserver(vtkCommand::InteractionEvent, my_sliceDataZ);
	planeWidgetZ->AddObserver(vtkCommand::EnableEvent, my_sliceDataZ);
	planeWidgetZ->AddObserver(vtkCommand::StartInteractionEvent, my_sliceDataZ);
	planeWidgetZ->AddObserver(vtkCommand::EndPickEvent, my_sliceDataZ);
	planeWidgetZ->SetPlaceFactor(1.0);
	planeWidgetZ->PlaceWidget();
	planeWidgetZ->SetOrigin(center);
	planeWidgetZ->SetEnabled(cb_showslice2->isOn());
	planeWidgetZ->PlaceWidget(bounds);

	//
	// Jump into the event loop and capture mouse and keyboard events.
	//
	//iren->Start();
	vtkWidget->GetRenderWindow()->Render();
}

void volumeviewer3D::resizeEvent(QResizeEvent* RE)
{
	QWidget::resizeEvent(RE);
	QSize size1 = RE->size();

	vbox1->setFixedSize(size1);

	if (size1.height() > 300)
		size1.setHeight(size1.height() - 300);
	vtkWidget->setFixedSize(size1);
	vtkWidget->GetRenderWindow()->SetSize(size1.width(), size1.height());
	vtkWidget->GetRenderWindow()->Render();
}

void volumeviewer3D::closeEvent(QCloseEvent* qce)
{
	emit hasbeenclosed();
	QWidget::closeEvent(qce);
}

volumeviewer3D::~volumeviewer3D() { delete vbox1; }

void volumeviewer3D::shade_changed()
{
	if (cb_shade->isOn())
	{
		volumeProperty->ShadeOn();
	}
	else
	{
		volumeProperty->ShadeOff();
	}

	vtkWidget->GetRenderWindow()->Render();
}

void volumeviewer3D::raytraceortexturemap_changed()
{
	if (cb_raytraceortexturemap->isOn())
	{
		volumeMapper->SetRequestedRenderModeToDefault();
	}
	else
	{
		volumeMapper->SetRequestedRenderModeToRayCast();
	}

	volume->Update();
	vtkWidget->GetRenderWindow()->Render();
}

void volumeviewer3D::showslices_changed()
{
	bool changed = false;
	if ((cb_showslices->isOn() ? 1 : 0) != sliceActorY->GetVisibility())
	{
		sliceActorY->SetVisibility(cb_showslices->isOn());
		sliceActorZ->SetVisibility(cb_showslices->isOn());
		changed = true;
	}
	if ((cb_showslice1->isOn() ? 1 : 0) != planeWidgetY->GetEnabled())
	{
		planeWidgetY->SetEnabled(cb_showslice1->isOn());
		changed = true;
	}
	if ((cb_showslice2->isOn() ? 1 : 0) != planeWidgetZ->GetEnabled())
	{
		planeWidgetZ->SetEnabled(cb_showslice2->isOn());
		changed = true;
	}

	if (changed)
	{
		vtkWidget->GetRenderWindow()->Render();
	}
}

void volumeviewer3D::showvolume_changed()
{
	if (cb_showvolume->isOn())
	{
		volume->SetVisibility(true);
		cb_shade->show();
		cb_raytraceortexturemap->show();
		if (bmportissue)
		{
			hbox3->hide();
			hbox1->show();
			hbox2->show();
		}
		else
		{
			hbox3->show();
			hbox1->hide();
			hbox2->show();
		}
	}
	else
	{
		volume->SetVisibility(false);
		cb_shade->hide();
		cb_raytraceortexturemap->hide();
		hbox1->hide();
		hbox2->hide();
		hbox3->hide();
	}
	vtkWidget->GetRenderWindow()->Render();
}

void volumeviewer3D::tissue_changed()
{
	if (!bmportissue)
	{
		colorTransferFunction->RemoveAllPoints();
		colorTransferFunction->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
		tissues_size_t tissuecount = TissueInfos::GetTissueCount();
		float* tissuecolor;
		for (tissues_size_t i = 1; i <= tissuecount; i++)
		{
			tissuecolor = TissueInfos::GetTissueColor(i);
			colorTransferFunction->AddRGBPoint(i, tissuecolor[0],
											   tissuecolor[1], tissuecolor[2]);
		}

		opacityTransferFunction->RemoveAllPoints();
		opacityTransferFunction->AddPoint(0, 0);
		for (tissues_size_t i = 1; i <= tissuecount; i++)
		{
			float opac1 = TissueInfos::GetTissueOpac(i);
			if (sl_trans->value() > 50)
				opac1 = opac1 * (100 - sl_trans->value()) / 50;
			else
				opac1 = 1.0f - (1.0 - opac1) * sl_trans->value() / 50;
			opacityTransferFunction->AddPoint(i, opac1);
		}

		vtkWidget->GetRenderWindow()->Render();
	}
}

void volumeviewer3D::transp_changed()
{
	if (!bmportissue)
	{
		opacityTransferFunction->RemoveAllPoints();
		opacityTransferFunction->AddPoint(0, 0);
		tissues_size_t tissuecount = TissueInfos::GetTissueCount();
		for (tissues_size_t i = 1; i <= tissuecount; i++)
		{
			float opac1 = TissueInfos::GetTissueOpac(i);
			if (sl_trans->value() > 50)
				opac1 = opac1 * (100 - sl_trans->value()) / 50;
			else
				opac1 = 1.0f - (1.0 - opac1) * sl_trans->value() / 50;
			opacityTransferFunction->AddPoint(i, opac1);
		}

		vtkWidget->GetRenderWindow()->Render();
	}
}

void volumeviewer3D::contrbright_changed()
{
	if (bmportissue)
	{
		double level =
			range[0] + (sl_bright->value() * (range[1] - range[0]) / 100);
		double window = (range[1] - range[0]) / (sl_contr->value() + 1);

		level = range[0] + (sl_bright->value() * (range[1] - range[0]) / 100);
		window = (range[1] - range[0]) * (sl_contr->value() + 1) / 101;

		opacityTransferFunction->RemoveAllPoints();
		opacityTransferFunction->AddPoint(level - window / 2, 0.0);
		opacityTransferFunction->AddPoint(level + window / 2, 1.0);
		colorTransferFunction->RemoveAllPoints();
		colorTransferFunction->AddRGBPoint(level - window / 2, 0.0, 0.0, 0.0);
		colorTransferFunction->AddRGBPoint(level + window / 2, 1.0, 1.0, 1.0);

		vtkWidget->GetRenderWindow()->Render();
	}
}

void volumeviewer3D::pixelsize_changed(Pair p)
{
	input->SetSpacing(p.high, p.low, hand3D->get_slicethickness());
	//BL?
	//vtkInformation* info = input->GetPipelineInformation();
	//double spc[3];
	//input->GetSpacing(spc);
	//info->Set(vtkDataObject::SPACING(), spc, 3);
	input->Modified();

	double bounds[6], center[3];

	input->GetBounds(bounds);
	input->GetCenter(center);

	planeWidgetY->SetOrigin(center);
	planeWidgetY->PlaceWidget(bounds);

	planeWidgetZ->SetOrigin(center);
	planeWidgetZ->PlaceWidget(bounds);

	vtkWidget->GetRenderWindow()->Render();
}

void volumeviewer3D::thickness_changed(float thick)
{
	Pair p = hand3D->get_pixelsize();
	input->SetSpacing(p.high, p.low, thick);
	//BL? what does this do?
	//vtkInformation* info = input->GetPipelineInformation();
	//double spc[3];
	//input->GetSpacing(spc);
	//info->Set(vtkDataObject::SPACING(), spc, 3);
	input->Modified();

	double bounds[6], center[3];
	input->GetBounds(bounds);
	input->GetCenter(center);

	planeWidgetY->SetOrigin(center);
	planeWidgetY->PlaceWidget(bounds);

	planeWidgetZ->SetOrigin(center);
	planeWidgetZ->PlaceWidget(bounds);

	//	outlineGrid->SetInput(input);
	//	outlineGrid->Update();
	//	outlineMapper->SetInput(outlineGrid->GetOutput());
	//	outlineMapper->Update();

	/*outlineMapper->Modified();
	outlineActor->Modified();
	ren3D->Modified();

	volume->Update();*/

	vtkWidget->GetRenderWindow()->Render();

	/*iSAR_ShepInterpolate iSI;
	int nx=16;
	int ny=8;
	float valin[128];
	{
	int pos=0;
	for(int pos=0;pos<nx*ny;pos++) {
	valin[pos]=0;
	}
	valin[19]=1;
	}
	float valout[256];

	{
	FILE *fp3=fopen("D:\\Development\\segmentation\\sample images\\test100.txt","w");

	int pos=0;
	for(int i=0;i<ny;i++) {
	for(int j=0;j<nx;j++) {
	if(i%2==1)
	fprintf(fp3,"- %f ",valin[j*ny+i]);
	else
	fprintf(fp3,"%f - ",valin[j*ny+i]);
	pos++;
	}
	fprintf(fp3,"\n");
	}
	fprintf(fp3,"\n\n");

	fclose(fp3);
	}

	iSI.interpolate(16,8,15,10,7.5,valin,valout);*/
}

void volumeviewer3D::reload()
{
	int xm, xp, ym, yp, zm, zp;				  //xxxa
	input->GetExtent(xm, xp, ym, yp, zm, zp); //xxxa
	int size1[3];
	input->GetDimensions(size1);
	int w = hand3D->return_width(); //xxxa
	if ((hand3D->return_width() != size1[0]) ||
		(hand3D->return_height() != size1[1]) ||
		(hand3D->return_nrslices() != size1[2]))
	{
		input->SetExtent(0, (int)hand3D->return_width() - 1, 0,
						 (int)hand3D->return_height() - 1, 0,
						 (int)hand3D->return_nrslices() - 1);
		input->AllocateScalars(input->GetScalarType(),
							   input->GetNumberOfScalarComponents());
		outlineGrid->SetInputData(input);
		planeWidgetY->SetInputData(input);
		sliceCutterY->SetInputData(input);
		planeWidgetZ->SetInputData(input);
		sliceCutterZ->SetInputData(input);

		double bounds[6], center[3];
		input->GetBounds(bounds);
		input->GetCenter(center);
		planeWidgetY->SetOrigin(center);
		planeWidgetY->PlaceWidget(bounds);
		planeWidgetZ->SetOrigin(center);
		planeWidgetZ->PlaceWidget(bounds);
		if (!bmportissue)
		{
			volumeMapper->SetInputData(input);
		}
		else
		{
			cast->SetInputData(input);
			volumeMapper->SetInputConnection(cast->GetOutputPort());
		}
	}
	input->GetExtent(xm, xp, ym, yp, zm, zp); //xxxa
	input->GetDimensions(size1);			  //xxxa
	Pair ps = hand3D->get_pixelsize();
	input->SetSpacing(ps.high, ps.low, hand3D->get_slicethickness());

	if (bmportissue)
	{
		float* field = (float*)input->GetScalarPointer(0, 0, 0);
		for (unsigned short i = 0; i < hand3D->return_nrslices(); i++)
		{
			hand3D->copyfrombmp(
				i, &(field[i * (unsigned long long)hand3D->return_area()]));
		}
	}
	else
	{
		tissues_size_t* field =
			(tissues_size_t*)input->GetScalarPointer(0, 0, 0);
		for (unsigned short i = 0; i < hand3D->return_nrslices(); i++)
		{
			hand3D->copyfromtissue(
				i, &(field[i * (unsigned long long)hand3D->return_area()]));
		}
	}

	/*vtkInformation* info = input->GetPipelineInformation();
	Pair p;
	hand3D->get_bmprange(&p);
	range[0]=p.low;
	range[1]=p.high;
	info->Set(vtkDataObject::FIELD_RANGE(),range,2);*/
	input->Modified();
	input->GetScalarRange(range);
	if (bmportissue)
	{
		Pair p;

		hand3D->get_bmprange(&p);
		range[0] = p.low;
		range[1] = p.high;
		lut->SetTableRange(range);
		lut->Build(); //effective build
		double slope = 255 / (range[1] - range[0]);
		double shift = -range[0];
		cast->SetShift(shift);
		cast->SetScale(slope);
		cast->SetOutputScalarTypeToUnsignedShort();
		cast->Update();
	}
	else
	{
		tissues_size_t tissuecount = TissueInfos::GetTissueCount();
		lut->SetNumberOfColors(tissuecount + 1);
		lut->Build();
		lut->SetTableValue(0, 0.0, 0.0, 0.0, 0.0);
		float* tissuecolor;
		for (tissues_size_t i = 1; i <= tissuecount; ++i)
		{
			tissuecolor = TissueInfos::GetTissueColor(i);
			lut->SetTableValue((double)i - 0.1, tissuecolor[0], tissuecolor[1],
							   tissuecolor[2], 1.0);
		}
		tissuecolor = TissueInfos::GetTissueColor(tissuecount + 1);
		colorTransferFunction->AddRGBPoint((double)tissuecount + 0.1,
										   tissuecolor[0], tissuecolor[1],
										   tissuecolor[2]);
	}
	sliceY->SetScalarRange(range);
	sliceY->SetLookupTable(lut);
	sliceY->Update();
	sliceZ->SetScalarRange(range);
	sliceZ->SetLookupTable(lut);
	sliceZ->Update();
	//		sliceY->Update();
	//		sliceZ->Update();
	//	} else {
	//		tissues_size_t *field=(tissues_size_t *)input->GetScalarPointer(0,0,0);
	//		for(unsigned short i=0;i<hand3D->return_nrslices();i++) {
	//			hand3D->copyfromtissue(i,&(field[i*(unsigned long)hand3D->return_area()]));
	//		}
	if (!bmportissue)
	{
		opacityTransferFunction->RemoveAllPoints();
		opacityTransferFunction->AddPoint(0, 0);
		tissues_size_t tissuecount = TissueInfos::GetTissueCount();
		for (tissues_size_t i = 1; i <= tissuecount; ++i)
		{
			float opac1 = TissueInfos::GetTissueOpac(i);
			if (sl_trans->value() > 50)
				opac1 = opac1 * (100 - sl_trans->value()) / 50;
			else
				opac1 = 1.0f - (1.0 - opac1) * sl_trans->value() / 50;
			opacityTransferFunction->AddPoint(i, opac1);
		}

		colorTransferFunction->RemoveAllPoints();
		colorTransferFunction->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
		float* tissuecolor;
		for (tissues_size_t i = 1; i <= tissuecount; ++i)
		{
			tissuecolor = TissueInfos::GetTissueColor(i);
			colorTransferFunction->AddRGBPoint(i, tissuecolor[0],
											   tissuecolor[1], tissuecolor[2]);
		}
	}

	input->Modified();
	outlineActor->Modified();

	vtkWidget->GetRenderWindow()->Render();
}

//volumeviewer3D::volumeviewer3D(SlicesHandler *hand3D1, bool bmportissue1, bool raytraceortexturemap1, bool shade1, QWidget *parent, const char *name, Qt::WindowFlags wFlags )
//    : QWidget( parent, name, wFlags )
//{
//	hand3D=hand3D1;
//	bmportissue=bmportissue1;
//	vbox1=new QVBox(this);
//	vtkWidget = new QVTKWidget( vbox1, "vtkWidget");
//	vtkWidget->setFixedSize(600,600);
////	resize( QSize(600, 600).expandedTo(minimumSizeHint()) );
//
//	vbox1->show();
//
////	setSizePolicy(QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed));
//	vbox1->setFixedSize(vbox1->sizeHint());
//	resize( vbox1->sizeHint().expandedTo(minimumSizeHint()) );
//
//  input = vtkSmartPointer<vtkImageData>::New();
//  input->SetExtent(0,(int)hand3D->return_width()-1,0,(int)hand3D->return_height()-1,0,(int)hand3D->return_nrslices()-1);
//  {
//	  input->SetScalarTypeToUnsignedChar();
//	  Pair ps=hand3D->get_pixelsize();
//	  input->SetSpacing(ps.high,ps.low,hand3D->get_slicethickness());
//	  tissues_size_t *field=(tissues_size_t *)input->GetScalarPointer(0,0,0);
//	  for(unsigned short i=0;i<hand3D->return_nrslices();i++) {
//		  hand3D->copyfromtissue(i,&(field[i*(unsigned long)hand3D->return_area()]));
//	  }
//  }
//
//  double bounds[6], center[3];
//  double range[2];
//
//  input->GetBounds( bounds );
//  input->GetCenter( center );
//  input->GetScalarRange( range );
//
//  /*outlineGrid= vtkSmartPointer<vtkOutlineFilter>::New();
//  outlineGrid->SetInput( input );
//  outlineGrid->Update();
//  outlineMapper= vtkSmartPointer<vtkPolyDataMapper>::New();
//  outlineMapper->SetInput(outlineGrid->GetOutput());
//  outlineActor= vtkSmartPointer<vtkActor>::New();
//  outlineActor->SetMapper(outlineMapper);*/
//
//		opacityTransferFunction = vtkSmartPointer<vtkPiecewiseFunction>::New();
//		opacityTransferFunction->AddPoint(0, 0);
//		for(tissues_size_t i=1;i<=tissuecount;i++) {
//			opacityTransferFunction->AddPoint(i, tissueopac[i]);
//		}
//
//		colorTransferFunction = vtkSmartPointer<vtkColorTransferFunction>::New();
//		colorTransferFunction->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
//		for(tissues_size_t i=1;i<=tissuecount;i++) {
//			colorTransferFunction->AddRGBPoint(i, tissuecolor[i][0], tissuecolor[i][1], tissuecolor[i][2]);
//		}
//
//		volumeProperty = vtkSmartPointer<vtkVolumeProperty>::New();
//		volumeProperty->SetColor(colorTransferFunction);
//		volumeProperty->SetScalarOpacity(opacityTransferFunction);
//
//		compositeFunction = vtkSmartPointer<vtkVolumeRayCastCompositeFunction>::New();
//		volumeMapper = vtkSmartPointer<vtkVolumeRayCastMapper>::New();
////		volumeMapper->DebugOn();
//		volumeMapper->SetVolumeRayCastFunction(compositeFunction);
//		volumeMapper->SetInput(input);
//
//		volume = vtkSmartPointer<vtkVolume>::New();
//		volume->SetMapper(volumeMapper);
//		volume->SetProperty(volumeProperty);
//
//  //
//  // Create the renderers and assign actors to them.
//  //
//  ren3D= vtkSmartPointer<vtkRenderer>::New();
//  ren3D->AddVolume(volume);
//  //ren3D->AddActor( outlineActor );
//  ren3D->SetBackground( 0, 0, 0 );
//  ren3D->SetViewport(0.0, 0.0, 1.0, 1.0);
////  input->DebugOn();
////  ren3D->DebugOn();
//
//  /*renWin= vtkSmartPointer<vtkRenderWindow>::New();
//  renWin->AddRenderer( ren3D );
//  renWin->LineSmoothingOn();
//  renWin->SetSize( 600, 600 );*/
//
//  vtkWidget->GetRenderWindow()->AddRenderer(ren3D);
//  vtkWidget->GetRenderWindow()->LineSmoothingOn();
//  vtkWidget->GetRenderWindow()->SetSize( 600, 600 );
//
//  style= vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
//  iren = vtkSmartPointer<QVTKInteractor>::New();
//  iren->SetInteractorStyle(style);
//
//    iren->SetRenderWindow(vtkWidget->GetRenderWindow());
//
//  //
//  // Jump into the event loop and capture mouse and keyboard events.
//  //
//  vtkWidget->GetRenderWindow()->Render();
//}
//
//
//void volumeviewer3D::closeEvent(QCloseEvent* qce)
//{
//	emit hasbeenclosed();
//	QWidget::closeEvent(qce);
//}
//
//volumeviewer3D::~volumeviewer3D()
//{
//	delete vbox1;
//}
//
//void volumeviewer3D::shade_changed()
//{
//
//}
//
//void volumeviewer3D::raytraceortexturemap_changed()
//{
//
//}
//
//void volumeviewer3D::tissue_changed()
//{
//
//}
//
//void volumeviewer3D::pixelsize_changed(Pair p)
//{
//
//}
//
//void volumeviewer3D::thickness_changed(float thick)
//{
//	double a,b,c1,c2,c3,c4,c5,c6,c7,c8,c9;
//	input->GetSpacing(a,b,c1);
//	input->SetSpacing(1,1,0.5);
//	input->Modified();
//	input->GetSpacing(a,b,c2);
//
//
//	/*outlineGrid->SetInput(input);
//	outlineGrid->Update();
//	outlineMapper->SetInput(outlineGrid->GetOutput());
//	outlineMapper->Update();
//	outlineActor->SetMapper(outlineMapper);
//	outlineActor->Modified();*/
//
//	volumeMapper->SetInput(input);
//	input->GetSpacing(a,b,c3);
//	volumeMapper->Update();
//	input->GetSpacing(a,b,c4);
//	volume->SetMapper(volumeMapper);
//	input->GetSpacing(a,b,c5);
//	volume->Update();
//	input->GetSpacing(a,b,c6);
//	ren3D->Modified();
//	input->GetSpacing(a,b,c7);
////	renWin->Modified();
//	input->GetSpacing(a,b,c8);
//
//	vtkWidget->GetRenderWindow()->Render();
//	input->GetSpacing(a,b,c9);
//
//	FILE *fp3=fopen("D:\\Development\\segmentation\\sample images\\test100.txt","w");
//	fprintf(fp3,"a1 %f %f %f %f %f %f %f %f %f\n",c1,c2,c3,c4,c5,c6,c7,c8,c9);
//	fclose(fp3);
//
//	/*iSAR_ShepInterpolate iSI;
//	int nx=16;
//	int ny=8;
//	float valin[128];
//	{
//		int pos=0;
//		for(int pos=0;pos<nx*ny;pos++) {
//			valin[pos]=0;
//		}
//		valin[19]=1;
//	}
//	float valout[256];
//
//	{
//		FILE *fp3=fopen("D:\\Development\\segmentation\\sample images\\test100.txt","w");
//
//		int pos=0;
//		for(int i=0;i<ny;i++) {
//			for(int j=0;j<nx;j++) {
//				if(i%2==1)
//					fprintf(fp3,"- %f ",valin[j*ny+i]);
//				else
//					fprintf(fp3,"%f - ",valin[j*ny+i]);
//				pos++;
//			}
//			fprintf(fp3,"\n");
//		}
//		fprintf(fp3,"\n\n");
//
//		fclose(fp3);
//	}
//
//	iSI.interpolate(16,8,15,10,7.5,valin,valout);*/
//
//}
//
//void volumeviewer3D::reload()
//{
//
//}

void iSAR_ShepInterpolate::interpolate(int nx, int ny, double dx, double dy,
									   double offset, float* valin,
									   float* valout)
{
	vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
	{
		int pos = 0;
		for (int j = 0; j < nx; j++)
		{
			for (int i = 0; i < ny; i++)
			{
				points->InsertPoint(pos, j * dx + (i % 2) * offset, i * dy, 0);
				pos++;
			}
		}
		for (int j = 0; j < nx; j++)
		{
			for (int i = 0; i < ny; i++)
			{
				points->InsertPoint(pos, j * dx + (i % 2) * offset, i * dy, 1);
				pos++;
			}
		}
	}

	vtkSmartPointer<vtkDoubleArray> scalars =
		vtkSmartPointer<vtkDoubleArray>::New();
	{
		for (int pos = 0; pos < nx * ny; pos++)
		{
			scalars->InsertValue(pos, valin[pos]);
		}
		for (int pos = nx * ny; pos < 2 * nx * ny; pos++)
		{
			scalars->InsertValue(pos, valin[pos - (nx * ny)]);
		}
	}

	vtkSmartPointer<vtkPolyData> profile = vtkSmartPointer<vtkPolyData>::New();
	profile->SetPoints(points);
	profile->GetPointData()->SetScalars(scalars);

	vtkSmartPointer<vtkShepardMethod> shepard =
		vtkSmartPointer<vtkShepardMethod>::New();
	shepard->SetInputData(profile);
	shepard->SetModelBounds(0, (nx - 1) * dx + offset, 0, (ny - 1) * dy, 0,
							0.2);
	//	shepard->SetMaximumDistance(0.2);
	shepard->SetNullValue(1.5);
	shepard->SetSampleDimensions(2 * nx, ny, 2);
	shepard->Update();

	vtkSmartPointer<vtkImageData> output = vtkSmartPointer<vtkImageData>::New();
	output->ShallowCopy(shepard->GetOutput());
	double* field = (double*)output->GetScalarPointer(0, 0, 0);

	{
		FILE* fp3 = fopen(
			"D:\\Development\\segmentation\\sample images\\test100.txt", "a");

		{
			vtkDoubleArray* myarray =
				(vtkDoubleArray*)profile->GetPointData()->GetScalars();
			unsigned long long pos = 0;
			for (int j = 0; j < nx; j++)
			{
				for (int i = 0; i < ny; i++)
				{
					double pt[3];
					profile->GetPoint(pos, pt);
					myarray->GetValue(pos);
					fprintf(fp3, "%f %f %f %f \n", pt[0], pt[1], pt[2],
							myarray->GetValue(pos));
					pos++;
				}
			}
			fprintf(fp3, "\n");
			for (int j = 0; j < nx; j++)
			{
				for (int i = 0; i < ny; i++)
				{
					double pt[3];
					profile->GetPoint(pos, pt);
					myarray->GetValue(pos);
					fprintf(fp3, "%f %f %f %f \n", pt[0], pt[1], pt[2],
							myarray->GetValue(pos));
					pos++;
				}
			}
		}

		unsigned long long pos = 0;
		for (int i = 0; i < ny; i++)
		{
			for (int j = 0; j < nx * 2; j++)
			{
				fprintf(fp3, "%f ",
						*((double*)output->GetScalarPointer(j, i, 0)));
				pos++;
			}
			fprintf(fp3, "\n");
		}
		fprintf(fp3, "\n\n");

		for (int i = 0; i < ny; i++)
		{
			for (int j = 0; j < nx * 2; j++)
			{
				fprintf(fp3, "%f ",
						*((double*)output->GetScalarPointer(j, i, 1)));
				pos++;
			}
			fprintf(fp3, "\n");
		}
		fprintf(fp3, "\n\n");

		fclose(fp3);
	}
}

surfaceviewer3D::surfaceviewer3D(SlicesHandler* hand3D1, bool bmportissue1,
								 QWidget* parent, const char* name,
								 Qt::WindowFlags wFlags)
	: QWidget(parent, name, wFlags)
{
	bmportissue = bmportissue1;
	hand3D = hand3D1;
	vbox1 = new Q3VBox(this);
	vtkWidget = new QVTKWidget(vbox1);
	vtkWidget->setFixedSize(600, 600);
	//	resize( QSize(600, 600).expandedTo(minimumSizeHint()) );

	hbox1 = new Q3HBox(vbox1);
	lb_trans = new QLabel("Transp.:", hbox1);
	sl_trans = new QSlider(Qt::Horizontal, hbox1);
	sl_trans->setRange(0, 100);
	sl_trans->setValue(50);
	hbox1->setFixedHeight(hbox1->sizeHint().height());

	QObject::connect(sl_trans, SIGNAL(sliderReleased()), this,
					 SLOT(transp_changed()));

	if (bmportissue)
	{
		hbox2 = new Q3HBox(vbox1);
		lb_thresh = new QLabel("Thresh.:", hbox2);
		sl_thresh = new QSlider(Qt::Horizontal, hbox2);
		sl_thresh->setRange(0, 100);
		sl_thresh->setValue(50);
		hbox2->setFixedHeight(hbox2->sizeHint().height());

		QObject::connect(sl_thresh, SIGNAL(sliderReleased()), this,
						 SLOT(thresh_changed()));
	}

	bt_update = new QPushButton("Update", vbox1);
	bt_update->show();

	vbox1->show();

	//	setSizePolicy(QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed));
	//	vbox1->setFixedSize(vbox1->sizeHint());
	resize(vbox1->sizeHint().expandedTo(minimumSizeHint()));

	QObject::connect(bt_update, SIGNAL(clicked()), this, SLOT(reload()));

	input = vtkSmartPointer<vtkImageData>::New();
	input->SetExtent(0, (int)hand3D->return_width() - 1, 0,
					 (int)hand3D->return_height() - 1, 0,
					 (int)hand3D->return_nrslices() - 1);
	Pair ps = hand3D->get_pixelsize();
	if (bmportissue)
	{
		input->SetSpacing(ps.high, ps.low, hand3D->get_slicethickness());
		input->AllocateScalars(VTK_FLOAT, 1);
		float* field = (float*)input->GetScalarPointer(0, 0, 0);
		for (unsigned short i = 0; i < hand3D->return_nrslices(); i++)
		{
			hand3D->copyfrombmp(
				i, &(field[i * (unsigned long long)hand3D->return_area()]));
		}
	}
	else
	{
		switch (sizeof(tissues_size_t))
		{
		case 1: input->AllocateScalars(VTK_UNSIGNED_CHAR, 1); break;
		case 2: input->AllocateScalars(VTK_UNSIGNED_SHORT, 1); break;
		default:
			cerr << "surfaceviewer3D::surfaceviewer3D: tissues_size_t not "
					"implemented."
				 << endl;
		}
		input->SetSpacing(ps.high, ps.low, hand3D->get_slicethickness());
		tissues_size_t* field =
			(tissues_size_t*)input->GetScalarPointer(0, 0, 0);
		for (unsigned short i = 0; i < hand3D->return_nrslices(); i++)
		{
			hand3D->copyfromtissue(
				i, &(field[i * (unsigned long long)hand3D->return_area()]));
		}
	}

	double bounds[6], center[3];

	input->GetBounds(bounds);
	input->GetCenter(center);
	input->GetScalarRange(range);

	double level = 0.5 * (range[1] + range[0]);
	double window = range[1] - range[0];

	smoother = vtkSmartPointer<vtkWindowedSincPolyDataFilter>::New();
	scalarsOff = vtkSmartPointer<vtkMaskFields>::New();

	// Define all of the variables
	startLabel = range[0];
	endLabel = range[1];
	startLabel = 1;
	unsigned int smoothingIterations = 15;
	double passBand = 0.001;
	double featureAngle = 120.0;

	// Generate models from labels
	// 1) Read the meta file
	// 2) Generate a histogram of the labels
	// 3) Generate models from the labeled volume
	// 4) Smooth the models
	// 5) Output each model into a separate file

	ren3D = vtkSmartPointer<vtkRenderer>::New();

	if (bmportissue)
	{
		cubes = vtkSmartPointer<vtkMarchingCubes>::New();
		cubes->SetValue(0, range[0] + 0.01 * (range[01] - range[0]) *
										  sl_thresh->value());
		cubes->SetInputData(input);

		PolyDataMapper.push_back(vtkSmartPointer<vtkPolyDataMapper>::New());
		(*PolyDataMapper.rbegin())->SetInputData(cubes->GetOutput());
		(*PolyDataMapper.rbegin())->ScalarVisibilityOff();

		Actor.push_back(vtkSmartPointer<vtkActor>::New());
		(*Actor.rbegin())->SetMapper((*PolyDataMapper.rbegin()));
		(*Actor.rbegin())
			->GetProperty()
			->SetOpacity(1.0 - 0.01 * sl_trans->value());
		//		(*Actor.rbegin())->GetProperty()->SetColor(1.0,1.0,1.0);
		ren3D->AddActor((*Actor.rbegin()));
	}
	else
	{
		discreteCubes = vtkSmartPointer<vtkDiscreteMarchingCubes>::New();
		discreteCubes->SetInputData(input);

		histogram = vtkSmartPointer<vtkImageAccumulate>::New();
		histogram->SetInputData(input);
		histogram->SetComponentExtent(0, endLabel, 0, 0, 0, 0);
		histogram->SetComponentOrigin(0, 0, 0);
		histogram->SetComponentSpacing(1, 1, 1);
		histogram->Update();

		discreteCubes->GenerateValues(endLabel - startLabel + 1, startLabel,
									  endLabel);
#ifdef TISSUES_SIZE_TYPEDEF
		if (startLabel > TISSUES_SIZE_MAX || endLabel > TISSUES_SIZE_MAX)
		{
			cerr << "surfaceviewer3D::surfaceviewer3D: Out of range "
					"tissues_size_t."
				 << endl;
		}
#endif // TISSUES_SIZE_TYPEDEF
		float* tissuecolor;
		for (unsigned int i = startLabel; i <= endLabel; i++)
		{
			geometry.push_back(vtkSmartPointer<vtkGeometryFilter>::New());

			selector.push_back(vtkSmartPointer<vtkThreshold>::New());

			//selector->SetInput(smoother->GetOutput());
			(*selector.rbegin())
				->SetInputConnection(discreteCubes->GetOutputPort());
			//selector->SetInputArrayToProcess(0, 0, 0,
			//	vtkDataObject::FIELD_ASSOCIATION_CELLS,
			//	vtkDataSetAttributes::SCALARS);

			//// Strip the scalars from the output
			//scalarsOff->SetInput(selector->GetOutput());
			//scalarsOff->CopyAttributeOff(vtkMaskFields::POINT_DATA,
			//	vtkDataSetAttributes::SCALARS);
			//scalarsOff->CopyAttributeOff(vtkMaskFields::CELL_DATA,
			//	vtkDataSetAttributes::SCALARS);

			//geometry->SetInput(scalarsOff->GetOutput());
			(*geometry.rbegin())
				->SetInputConnection((*selector.rbegin())->GetOutputPort());

			// see if the label exists, if not skip it
			double frequency =
				histogram->GetOutput()->GetPointData()->GetScalars()->GetTuple1(
					i);
			if (frequency == 0.0)
			{
				continue;
			}

			indices.push_back(i);

			// select the cells for a given label
			(*selector.rbegin())->ThresholdBetween(i, i);

			PolyDataMapper.push_back(vtkSmartPointer<vtkPolyDataMapper>::New());
			(*PolyDataMapper.rbegin())
				->SetInputConnection((*geometry.rbegin())->GetOutputPort());
			(*PolyDataMapper.rbegin())->ScalarVisibilityOff();

			Actor.push_back(vtkSmartPointer<vtkActor>::New());
			(*Actor.rbegin())->SetMapper((*PolyDataMapper.rbegin()));
			float opac1 = TissueInfos::GetTissueOpac(i);
			if (sl_trans->value() > 50)
				opac1 = opac1 * (100 - sl_trans->value()) / 50;
			else
				opac1 = 1.0f - (1.0 - opac1) * sl_trans->value() / 50;
			(*Actor.rbegin())->GetProperty()->SetOpacity(opac1);
			tissuecolor = TissueInfos::GetTissueColor(i);
			(*Actor.rbegin())
				->GetProperty()
				->SetColor(tissuecolor[0], tissuecolor[1], tissuecolor[2]);
			ren3D->AddActor((*Actor.rbegin()));
		}
	}

	//smoother->SetInput(discreteCubes->GetOutput());
	//smoother->SetNumberOfIterations(smoothingIterations);
	//smoother->BoundarySmoothingOff();
	//smoother->FeatureEdgeSmoothingOff();
	//smoother->SetFeatureAngle(featureAngle);
	//smoother->SetPassBand(passBand);
	//smoother->NonManifoldSmoothingOn();
	//smoother->NormalizeCoordinatesOn();
	//smoother->Update();

	ren3D->SetBackground(0, 0, 0);
	ren3D->SetViewport(0.0, 0.0, 1.0, 1.0);

	renWin = vtkSmartPointer<vtkRenderWindow>::New();

	vtkWidget->GetRenderWindow()->AddRenderer(ren3D);
	vtkWidget->GetRenderWindow()->LineSmoothingOn();
	vtkWidget->GetRenderWindow()->SetSize(600, 600);

	style = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
	iren = vtkSmartPointer<QVTKInteractor>::New();
	iren->SetInteractorStyle(style);

	iren->SetRenderWindow(vtkWidget->GetRenderWindow());

	//
	// Jump into the event loop and capture mouse and keyboard events.
	//
	//iren->Start();
	vtkWidget->GetRenderWindow()->Render();
}

surfaceviewer3D::~surfaceviewer3D() { delete vbox1; }

void surfaceviewer3D::tissue_changed()
{
	if (!bmportissue)
	{
#ifdef TISSUES_SIZE_TYPEDEF
		if (startLabel > TISSUES_SIZE_MAX || endLabel > TISSUES_SIZE_MAX)
		{
			cerr << "surfaceviewer3D::tissue_changed: Out of range "
					"tissues_size_t."
				 << endl;
		}
#endif // TISSUES_SIZE_TYPEDEF
		float* tissuecolor;
		for (unsigned int i = startLabel; i <= endLabel; i++)
		{
			size_t j = 0;
			while (j < indices.size() && indices[j] != i)
				j++;
			if (j == indices.size())
				continue;
			float opac1 = TissueInfos::GetTissueOpac(i);
			if (sl_trans->value() > 50)
				opac1 = opac1 * (100 - sl_trans->value()) / 50;
			else
				opac1 = 1.0f - (1.0 - opac1) * sl_trans->value() / 50;
			Actor[j]->GetProperty()->SetOpacity(opac1);
			tissuecolor = TissueInfos::GetTissueColor(i);
			Actor[j]->GetProperty()->SetColor(tissuecolor[0], tissuecolor[1],
											  tissuecolor[2]);
		}

		vtkWidget->GetRenderWindow()->Render();
	}
}

void surfaceviewer3D::pixelsize_changed(Pair p)
{
	input->SetSpacing(p.high, p.low, hand3D->get_slicethickness());
	//BL? what does this do?
	//vtkInformation* info = input->GetPipelineInformation();
	//double spc[3];
	//input->GetSpacing(spc);
	//info->Set(vtkDataObject::SPACING(), spc, 3);
	input->Modified();

	vtkWidget->GetRenderWindow()->Render();
}

void surfaceviewer3D::thickness_changed(float thick)
{
	Pair p = hand3D->get_pixelsize();
	input->SetSpacing(p.high, p.low, thick);
	//BL? what does this do?
	//vtkInformation* info = input->GetPipelineInformation();
	//double spc[3];
	//input->GetSpacing(spc);
	//info->Set(vtkDataObject::SPACING(), spc, 3);
	input->Modified();

	vtkWidget->GetRenderWindow()->Render();
}

void surfaceviewer3D::reload()
{
	if (!bmportissue)
	{
		for (size_t i = 0; i < Actor.size(); i++)
			ren3D->RemoveActor(Actor[i]);
		geometry.clear();
		selector.clear();
		indices.clear();
		PolyDataMapper.clear();
		Actor.clear();
	}

	int xm, xp, ym, yp, zm, zp;				  //xxxa
	input->GetExtent(xm, xp, ym, yp, zm, zp); //xxxa
	int size1[3];
	input->GetDimensions(size1);
	int w = hand3D->return_width(); //xxxa
	if ((hand3D->return_width() != size1[0]) ||
		(hand3D->return_height() != size1[1]) ||
		(hand3D->return_nrslices() != size1[2]))
	{
		input->SetExtent(0, (int)hand3D->return_width() - 1, 0,
						 (int)hand3D->return_height() - 1, 0,
						 (int)hand3D->return_nrslices() - 1);
		input->AllocateScalars(input->GetScalarType(),
							   input->GetNumberOfScalarComponents());
		if (bmportissue)
		{
			cubes->SetInputData(input);
		}
		else
		{
			discreteCubes->SetInputData(input);
		}
	}
	input->GetExtent(xm, xp, ym, yp, zm, zp); //xxxa
	input->GetDimensions(size1);			  //xxxa
	Pair ps = hand3D->get_pixelsize();
	input->SetSpacing(ps.high, ps.low, hand3D->get_slicethickness());

	if (bmportissue)
	{
		float* field = (float*)input->GetScalarPointer(0, 0, 0);
		for (unsigned short i = 0; i < hand3D->return_nrslices(); i++)
		{
			hand3D->copyfrombmp(
				i, &(field[i * (unsigned long long)hand3D->return_area()]));
		}
	}
	else
	{
		tissues_size_t* field =
			(tissues_size_t*)input->GetScalarPointer(0, 0, 0);
		for (unsigned short i = 0; i < hand3D->return_nrslices(); i++)
		{
			hand3D->copyfromtissue(
				i, &(field[i * (unsigned long long)hand3D->return_area()]));
		}
	}

	if (bmportissue)
	{
		Pair p;
		hand3D->get_bmprange(&p);
		range[0] = p.low;
		range[1] = p.high;
	}
	else
	{
		range[0] = 0;
		range[1] = TissueInfos::GetTissueCount();
	}

	startLabel = range[0];
	endLabel = range[1];
	startLabel = 1;

	if (bmportissue)
	{
		cubes->SetValue(0, range[0] + 0.01 * (range[01] - range[0]) *
										  sl_thresh->value());
	}
	else
	{
		histogram->SetInputData(input);
		histogram->SetComponentExtent(0, endLabel, 0, 0, 0, 0);
		histogram->Update();

		discreteCubes->GenerateValues(endLabel - startLabel + 1, startLabel,
									  endLabel);

#ifdef TISSUES_SIZE_TYPEDEF
		if (startLabel > TISSUES_SIZE_MAX || endLabel > TISSUES_SIZE_MAX)
		{
			cerr << "surfaceviewer3D::reload: Out of range tissues_size_t."
				 << endl;
		}
#endif // TISSUES_SIZE_TYPEDEF
		float* tissuecolor;
		for (unsigned int i = startLabel; i <= endLabel; i++)
		{
			geometry.push_back(vtkSmartPointer<vtkGeometryFilter>::New());
			selector.push_back(vtkSmartPointer<vtkThreshold>::New());

			//selector->SetInput(smoother->GetOutput());
			(*selector.rbegin())
				->SetInputConnection(discreteCubes->GetOutputPort());
			//selector->SetInputArrayToProcess(0, 0, 0,
			//	vtkDataObject::FIELD_ASSOCIATION_CELLS,
			//	vtkDataSetAttributes::SCALARS);

			//// Strip the scalars from the output
			//scalarsOff->SetInput(selector->GetOutput());
			//scalarsOff->CopyAttributeOff(vtkMaskFields::POINT_DATA,
			//	vtkDataSetAttributes::SCALARS);
			//scalarsOff->CopyAttributeOff(vtkMaskFields::CELL_DATA,
			//	vtkDataSetAttributes::SCALARS);

			//geometry->SetInput(scalarsOff->GetOutput());
			(*geometry.rbegin())
				->SetInputConnection((*selector.rbegin())->GetOutputPort());

			// see if the label exists, if not skip it
			double frequency =
				histogram->GetOutput()->GetPointData()->GetScalars()->GetTuple1(
					i);
			if (frequency == 0.0)
			{
				continue;
			}

			indices.push_back(i);

			// select the cells for a given label
			(*selector.rbegin())->ThresholdBetween(i, i);

			PolyDataMapper.push_back(vtkSmartPointer<vtkPolyDataMapper>::New());
			(*PolyDataMapper.rbegin())
				->SetInputConnection((*geometry.rbegin())->GetOutputPort());
			(*PolyDataMapper.rbegin())->ScalarVisibilityOff();

			Actor.push_back(vtkSmartPointer<vtkActor>::New());
			(*Actor.rbegin())->SetMapper((*PolyDataMapper.rbegin()));
			float opac1 = TissueInfos::GetTissueOpac(i);
			if (sl_trans->value() > 50)
				opac1 = opac1 * (100 - sl_trans->value()) / 50;
			else
				opac1 = 1.0f - (1.0 - opac1) * sl_trans->value() / 50;
			(*Actor.rbegin())->GetProperty()->SetOpacity(opac1);
			tissuecolor = TissueInfos::GetTissueColor(i);
			(*Actor.rbegin())
				->GetProperty()
				->SetColor(tissuecolor[0], tissuecolor[1], tissuecolor[2]);
			ren3D->AddActor((*Actor.rbegin()));
		}
	}

	input->Modified();

	vtkWidget->GetRenderWindow()->Render();
}

void surfaceviewer3D::closeEvent(QCloseEvent* qce)
{
	emit hasbeenclosed();
	QWidget::closeEvent(qce);
}

void surfaceviewer3D::resizeEvent(QResizeEvent* RE)
{
	QWidget::resizeEvent(RE);
	QSize size1 = RE->size();
	vbox1->setFixedSize(size1);
	if (size1.height() > 150)
		size1.setHeight(size1.height() - 150);
	vtkWidget->setFixedSize(size1);
	vtkWidget->GetRenderWindow()->SetSize(size1.width(), size1.height());
	vtkWidget->GetRenderWindow()->Render();
}

void surfaceviewer3D::transp_changed()
{
	if (bmportissue)
	{
		(*Actor.rbegin())
			->GetProperty()
			->SetOpacity(1.0 - 0.01 * sl_trans->value());
	}
	else
	{
#ifdef TISSUES_SIZE_TYPEDEF
		if (startLabel > TISSUES_SIZE_MAX || endLabel > TISSUES_SIZE_MAX)
		{
			cerr << "surfaceviewer3D::transp_changed: Out of range "
					"tissues_size_t."
				 << endl;
		}
#endif // TISSUES_SIZE_TYPEDEF
		for (unsigned int i = startLabel; i <= endLabel; i++)
		{
			size_t j = 0;
			while (j < indices.size() && indices[j] != i)
				j++;
			if (j == indices.size())
				continue;
			float opac1 = TissueInfos::GetTissueOpac(i);
			if (sl_trans->value() > 50)
				opac1 = opac1 * (100 - sl_trans->value()) / 50;
			else
				opac1 = 1.0f - (1.0 - opac1) * sl_trans->value() / 50;
			Actor[j]->GetProperty()->SetOpacity(opac1);
		}
	}
	vtkWidget->GetRenderWindow()->Render();
}

void surfaceviewer3D::thresh_changed()
{
	if (bmportissue)
	{
		cubes->SetValue(0, range[0] + 0.01 * (range[01] - range[0]) *
										  sl_thresh->value());

		vtkWidget->GetRenderWindow()->Render();
	}
}
