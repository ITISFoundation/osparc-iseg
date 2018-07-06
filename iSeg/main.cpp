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

#include <QPlastiqueStyle>
#include <QSplashScreen>
#include <qapplication.h>
#include <qdatetime.h>
#include <qlabel.h>
#include <qmessagebox.h>

#include <boost/date_time.hpp>
#include <boost/format.hpp>

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

std::string timestamped(const std::string prefix, const std::string& suffix)
{
	auto timeLocal = boost::posix_time::second_clock::local_time();
	auto tod = boost::posix_time::to_iso_string(timeLocal);
	return prefix + "-" + tod + suffix;
}

// \brief Redirect VTK errors/warnings to file
class vtkCustomOutputWindow : public vtkOutputWindow
{
public:
	static vtkCustomOutputWindow *New();
	vtkTypeMacro(vtkCustomOutputWindow, vtkOutputWindow);
	virtual void PrintSelf(ostream &os, vtkIndent indent) override {}

	// Put the text into the output stream.
	virtual void DisplayText(const char *msg) override { std::cerr << msg << std::endl; }

protected:
	vtkCustomOutputWindow() {}

private:
	vtkCustomOutputWindow(const vtkCustomOutputWindow &); // Not implemented.
	void operator=(const vtkCustomOutputWindow &);				// Not implemented.
};

vtkStandardNewMacro(vtkCustomOutputWindow);
} // namespace

int main(int argc, char **argv)
{
	auto eow = vtkCustomOutputWindow::New();
	eow->SetInstance(eow);
	eow->Delete();

	cerr << "starting iSeg..." << endl;
	QFileInfo fileinfo(argv[0]);

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
	auto log_file_name = timestamped(tmpdir.absFilePath("iSeg").toStdString(), ".log");
	if (!interceptOutput(log_file_name))
	{
		error("intercepting output failed");
	}

	QDir fileDirectory = fileinfo.dir();
	cerr << "fileDirectory = " << fileDirectory.absolutePath().toStdString() << endl;

	QDir picpath = fileinfo.dir();
	cerr << "picture path = " << picpath.absolutePath().toStdString() << endl;
	if (!picpath.cd("images"))
	{
		cerr << "images folder does not exist" << endl;
	}

	QDir atlasdir = fileinfo.dir();
	if (!atlasdir.cd("atlas"))
	{
		cerr << "atlas folder does not exist" << endl;
	}

	QString splashpicpath = picpath.absFilePath(QString("splash.png"));
	QString locationpath = fileDirectory.absPath();
	QString latestprojpath = tmpdir.absFilePath(QString("latestproj.txt"));
	QString settingspath = tmpdir.absFilePath(QString("settings.bin"));

	TissueInfos::InitTissues();
	BranchItem::init_availablelabels();

	SlicesHandler slice_handler;

	QApplication app(argc, argv);
	QApplication::setStyle(new QPlastiqueStyle);

	QPalette palette;
	palette = app.palette();
	palette.setColor(QPalette::Window, QColor(40, 40, 40));
	palette.setColor(QPalette::Base, QColor(70, 70, 70));
	palette.setColor(QPalette::WindowText, QColor(255, 255, 255));
	palette.setColor(QPalette::Text, QColor(255, 255, 255));
	palette.setColor(QPalette::Button, QColor(70, 70, 70));
	palette.setColor(QPalette::Highlight, QColor(150, 150, 150));
	palette.setColor(QPalette::ButtonText, QColor(255, 255, 255));
	palette.setColor(QPalette::ToolTipText, QColor(0, 0, 0));
	qApp->setPalette(palette);
	app.setPalette(palette);

#ifdef SHOWSPLASH
	QString pixmapstr = QString(splashpicpath.toAscii().data());
	QPixmap pixmap(pixmapstr);

	QSplashScreen splash_screen(pixmap.scaledToHeight(600, Qt::SmoothTransformation));
	splash_screen.show();
	QTime now = QTime::currentTime();
#endif
	app.processEvents();

	MainWindow *mainWindow = nullptr;
	if (argc > 1)
	{
		QString qstr(argv[1]);
		if (qstr.compare(QString("S4Llink")) == 0)
		{
			mainWindow = new MainWindow(
					&slice_handler, locationpath, picpath, tmpdir,
					true, nullptr, "MainWindow",
					Qt::WDestructiveClose | Qt::WResizeNoErase, argv);
		}
	}
	if (mainWindow == nullptr)
	{
		mainWindow = new MainWindow(
				&slice_handler, locationpath, picpath, tmpdir,
				false, nullptr, "MainWindow",
				Qt::WDestructiveClose | Qt::WResizeNoErase, argv);
	}

	// needed on MacOS, but not WIN32?
	mainWindow->setStyleSheet("QWidget { color: white; }");

	mainWindow->LoadLoadProj(latestprojpath);
	mainWindow->LoadAtlas(atlasdir);
	mainWindow->LoadSettings(settingspath.toAscii().data());
	if (argc > 1)
	{
		QString qstr(argv[1]);
		if (qstr.compare(QString("S4Llink")) != 0)
		{
			mainWindow->loadproj(qstr);
		}
		else if (argc > 2)
		{
			mainWindow->loadS4Llink(QString(argv[2]));
		}
	}

#ifdef SHOWSPLASH
	while (now.msecsTo(QTime::currentTime()) < 1000)
	{
	}
#endif

	mainWindow->showMaximized();
	mainWindow->show();

#ifdef SHOWSPLASH
	splash_screen.finish(mainWindow);
#endif

	QObject::connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));

	return app.exec();
}
