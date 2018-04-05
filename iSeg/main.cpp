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

#include "config.h"

#include "DicomReader.h"
#include "MainWindow.h"
#include "SlicesHandler.h"
#include "TissueInfos.h"
#include "bmp_read_1.h"

#include "Core/BranchItem.h"
#include "Core/Log.h"
#include "Core/Pair.h"
#include "Core/Point.h"

#include <QPlastiqueStyle.h>
#include <QSplashScreen>
#include <qapplication.h>
#include <qdatetime.h>
#include <qlabel.h>
#include <qmessagebox.h>

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <list>
#include <stack>
#include <string>
#include <vector>

#include <vtkObjectFactory.h>
#include <vtkOutputWindow.h>

#pragma warning(disable : 4100)

#define SHOWSPLASH

using namespace std;
using namespace iseg;

namespace {
// \brief Redirect VTK errors/warnings to file
class vtkCustomOutputWindow : public vtkOutputWindow
{
public:
	static vtkCustomOutputWindow* New();
	vtkTypeMacro(vtkCustomOutputWindow, vtkOutputWindow);
	virtual void PrintSelf(ostream& os, vtkIndent indent) {}

	// Put the text into the output stream.
	virtual void DisplayText(const char* msg) { std::cerr << msg << std::endl; }

protected:
	vtkCustomOutputWindow() {}

private:
	vtkCustomOutputWindow(const vtkCustomOutputWindow&); // Not implemented.
	void operator=(const vtkCustomOutputWindow&);		 // Not implemented.
};

vtkStandardNewMacro(vtkCustomOutputWindow);
} // namespace

int main(int argc, char** argv)
{
	auto eow = vtkCustomOutputWindow::New();
	eow->SetInstance(eow);
	eow->Delete();

	cerr << "starting iSeg..." << endl;
	QFileInfo fileinfo(argv[0]);

	cerr << "initializing tmp folder..." << endl;
	QDir tmpdir = QDir::home();

	if (!tmpdir.exists("iSeg"))
	{
		cerr << "iSeg folder does not exist, creating..." << endl;
		if (!tmpdir.mkdir(QString("iSeg")))
		{
			cerr << "failed to create iSeg folder, exiting..." << endl;
			exit(EXIT_FAILURE);
		}
	}
	if (!tmpdir.cd("iSeg"))
	{
		cerr << "failed to enter iSeg folder, exiting..." << endl;
		exit(EXIT_FAILURE);
	}

	if (!tmpdir.exists("tmp"))
	{
		cerr << "tmp folder does not exist, creating..." << endl;
		if (!tmpdir.mkdir(QString("tmp")))
		{
			cerr << "failed to create tmp folder, exiting..." << endl;
			exit(EXIT_FAILURE);
		}
	}
	if (!tmpdir.cd("tmp"))
	{
		cerr << "failed to enter tmp folder, exiting..." << endl;
		exit(EXIT_FAILURE);
	}

	cerr << "intercepting application's output to a log file..." << endl;
	if (!interceptOutput(tmpdir.absolutePath().toStdString() + "/iSeg.log"))
	{
		error("intercepting output failed");
	}

	QDir fileDirectory = fileinfo.dir();
	cerr << "fileDirectory = " << fileDirectory.absolutePath().toAscii().data()
		 << endl;

	QDir picpath = fileinfo.dir();
	cerr << "picture path = " << picpath.absolutePath().toAscii().data()
		 << endl;
	if (!picpath.cd("images"))
	{
		cerr << "images folder does not exist" << endl;
	}

	QDir atlasdir = fileinfo.dir();
	if (!atlasdir.cd("atlas"))
	{
		cerr << "atlas folder does not exist" << endl;
	}

	QDir stylesheetdir = fileinfo.dir();

#ifdef SHOWSPLASH
	QString splashpicpath = picpath.absFilePath(QString("splash.png"));
#endif
	QString stylesheetpath =
		stylesheetdir.absFilePath(QString("S4L.stylesheet"));
	QString locationpath = fileDirectory.absPath();
	QString latestprojpath = tmpdir.absFilePath(QString("latestproj.txt"));
	QString settingspath = tmpdir.absFilePath(QString("settings.bin"));

	TissueInfos::InitTissues();
	BranchItem::init_availablelabels();

	SlicesHandler h3Ds2;

	QApplication ab1(argc, argv);
	QApplication::setStyle(new QPlastiqueStyle);

	QPalette p;
	p = ab1.palette();
	p.setColor(QPalette::Window, QColor(40, 40, 40));
	p.setColor(QPalette::Base, QColor(70, 70, 70));
	p.setColor(QPalette::WindowText, QColor(255, 255, 255));
	p.setColor(QPalette::Text, QColor(255, 255, 255));
	p.setColor(QPalette::Button, QColor(70, 70, 70));
	p.setColor(QPalette::Highlight, QColor(150, 150, 150));
	p.setColor(QPalette::ButtonText, QColor(255, 255, 255));
	qApp->setPalette(p);

#ifdef SHOWSPLASH
	QString pixmapstr = QString(splashpicpath.toAscii().data());
	QPixmap pixmap1(pixmapstr);

	QSplashScreen splash1(
		pixmap1.scaledToHeight(600, Qt::SmoothTransformation));
	splash1.show();
#endif
	QTime now = QTime::currentTime();
	ab1.processEvents();

	MainWindow* mainWindow = NULL;
	if (argc > 1)
	{
		QString qstr(argv[1]);
		if (qstr.compare(QString("S4Llink")) == 0)
		{
			mainWindow = new MainWindow(
				&h3Ds2, QString(locationpath.toAscii().data()), picpath, tmpdir,
				true, 0, "new window",
				Qt::WDestructiveClose | Qt::WResizeNoErase, argv);
		}
	}
	if (mainWindow == NULL)
	{
		mainWindow =
			new MainWindow(&h3Ds2, QString(locationpath.toAscii().data()),
						   picpath, tmpdir, false, 0, "new window",
						   Qt::WDestructiveClose | Qt::WResizeNoErase, argv);
	}

	QFile qss(stylesheetpath.toAscii().data());
	qss.open(QFile::ReadOnly);
	//mainWindow->setStyleSheet(qss.readAll());
	cerr << qss.fileName().toStdString() << endl;
	mainWindow->setStyleSheet("color: white;");
	qss.close();
	//mainWindow->setStyleSheet("QDockWidget { color: black; } QDockWidget::title { background: lightgray; padding-left: 5px; padding-top: 3px;  } QDockWidget::close-button, QDockWidget::float-button { background: lightgray; } QMenuBar { background-color: gray; } QMenu::separator { background-color: lightgrey; height: 1px; margin-left: 10px; margin-right: 10px; } QMenu::item:disabled { color: darkgrey; }" );

	QString latestprojpath1;
	latestprojpath1.setAscii(latestprojpath.toAscii().data());
	mainWindow->LoadLoadProj(latestprojpath1);
	mainWindow->LoadAtlas(atlasdir);
	mainWindow->LoadSettings(settingspath.toAscii().data());
	bool s4llink = false;
	if (argc > 1)
	{
		QString qstr(argv[1]);
		if (qstr.compare(QString("S4Llink")) != 0)
			mainWindow->loadproj(qstr);
		else
		{
			s4llink = true;
			if (argc > 2)
				mainWindow->loadS4Llink(QString(argv[2]));
		}
		//mainWindow->loadproj(QString("F:\\applications\\bifurcation\\bifurcation.prj"));xxxa
	}

#ifdef SHOWSPLASH
	while (now.msecsTo(QTime::currentTime()) < 2000)
	{
	}
#endif

	mainWindow->showMaximized();
	mainWindow->show();

#ifdef NOSPLASH
	splash1.finish(mainWindow);
#endif

	QObject::connect(&ab1, SIGNAL(lastWindowClosed()), &ab1, SLOT(quit()));

	int ok1 = ab1.exec();

	return ok1;

	TissueInfos::InitTissues();

	//bmphandler image6;
	SlicesHandler h3Ds1;

	QApplication ab2(argc, argv);
	MainWindow* tbw2 = new MainWindow(
		&h3Ds1, locationpath, picpath, tmpdir, false, 0, "new window",
		Qt::WDestructiveClose | Qt::WResizeNoErase);
	tbw2->show();

	QObject::connect(&ab2, SIGNAL(lastWindowClosed()), &ab2, SLOT(quit()));

	return ab2.exec();

	return 1;
}
