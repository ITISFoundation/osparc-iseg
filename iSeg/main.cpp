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

#include "MainWindow.h"
#include "SlicesHandler.h"
#include "TissueInfos.h"
#include "bmp_read_1.h"

#include "Data/Logger.h"

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
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>

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

std::pair<std::string, std::string> custom_parser(const std::string& s)
{
	if (s.find("S4Llink") == 0)
	{
		return std::make_pair(std::string("s4l"), std::string());
	}
	else
	{
		return std::make_pair(std::string(), std::string());
	}
}

// \brief Redirect VTK errors/warnings to file
class vtkCustomOutputWindow : public vtkOutputWindow
{
public:
	static vtkCustomOutputWindow* New();
	vtkTypeMacro(vtkCustomOutputWindow, vtkOutputWindow);
	virtual void PrintSelf(ostream& os, vtkIndent indent) override {}

	// Put the text into the output stream.
	virtual void DisplayText(const char* msg) override { std::cerr << msg << std::endl; }

protected:
	vtkCustomOutputWindow() {}

private:
	vtkCustomOutputWindow(const vtkCustomOutputWindow&); // Not implemented.
	void operator=(const vtkCustomOutputWindow&);				 // Not implemented.
};

vtkStandardNewMacro(vtkCustomOutputWindow);

} // namespace

int main(int argc, char** argv)
{
	// parse program arguments
	namespace po = boost::program_options;
	namespace fs = boost::filesystem;

	po::options_description desc("Allowed options");
	desc.add_options()("help", "produce help message")("input-file,I", po::value<std::string>(), "open iSEG project (*.prj) file")("s4l", po::value<std::string>(), "start iSEG in S4L link mode")("plugin-dirs,D", po::value<std::vector<std::string>>(), "additional directories to search for plugins");
	po::variables_map vm;
	po::store(po::command_line_parser(argc, argv).options(desc).extra_parser(custom_parser).run(), vm);
	po::notify(vm);

	if (vm.count("help"))
	{
		cout << desc << "\n";
		return 1;
	}
	std::vector<std::string> plugin_dirs;
	plugin_dirs.push_back(fs::path(argv[0]).parent_path().string());
	if (vm.count("plugin-dirs"))
	{
		auto extra_dirs = vm["plugin-dirs"].as<std::vector<std::string>>();
		plugin_dirs.insert(plugin_dirs.end(), extra_dirs.begin(), extra_dirs.end());
	}

	// install vtk error handler to avoid stupid popup windows
	auto eow = vtkCustomOutputWindow::New();
	eow->SetInstance(eow);
	eow->Delete();

	QFileInfo fileinfo(argv[0]);

	QDir tmpdir = QDir::home();
	if (!tmpdir.exists("iSeg"))
	{
		std::cerr << "iSeg folder does not exist, creating..." << endl;
		if (!tmpdir.mkdir(QString("iSeg")))
		{
			std::cerr << "failed to create iSeg folder, exiting..." << endl;
			exit(EXIT_FAILURE);
		}
	}
	if (!tmpdir.cd("iSeg"))
	{
		std::cerr << "failed to enter iSeg folder, exiting..." << endl;
		exit(EXIT_FAILURE);
	}

	if (!tmpdir.exists("tmp"))
	{
		std::cerr << "tmp folder does not exist, creating..." << endl;
		if (!tmpdir.mkdir(QString("tmp")))
		{
			std::cerr << "failed to create tmp folder, exiting..." << endl;
			exit(EXIT_FAILURE);
		}
	}
	if (!tmpdir.cd("tmp"))
	{
		std::cerr << "failed to enter tmp folder, exiting..." << endl;
		exit(EXIT_FAILURE);
	}

	auto log_file_name = timestamped(tmpdir.absFilePath("iSeg").toStdString(), ".log");
	init_logging(log_file_name, true);

	QDir fileDirectory = fileinfo.dir();
	ISEG_INFO("fileDirectory = " << fileDirectory.absolutePath().toStdString());

	QDir picpath = fileinfo.dir();
	ISEG_INFO("picture path = " << picpath.absolutePath().toStdString());
	if (!picpath.cd("images"))
	{
		ISEG_WARNING_MSG("images folder does not exist");
	}

	QDir atlasdir = fileinfo.dir();
	if (!atlasdir.cd("atlas"))
	{
		ISEG_WARNING_MSG("atlas folder does not exist");
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

	MainWindow* mainWindow = new MainWindow(
			&slice_handler, locationpath, picpath, tmpdir,
			vm.count("s4l"), nullptr, "MainWindow",
			Qt::WDestructiveClose | Qt::WResizeNoErase, plugin_dirs);

	// needed on MacOS, but not WIN32?
	mainWindow->setStyleSheet(
			"QWidget { color: white; }"
			"QPushButton:checked { background-color: rgb(150,150,150); font: bold }"
			"QPushButton:disabled  { background-color: rgb(40,40,40); color: rgb(128,128,128) }"
			"QCheckBox:disabled  { background-color: rgb(40,40,40); color: rgb(128,128,128) }");

	mainWindow->LoadLoadProj(latestprojpath);
	mainWindow->LoadAtlas(atlasdir);
	mainWindow->LoadSettings(settingspath.toAscii().data());
	if (vm.count("s4l"))
	{
		mainWindow->loadS4Llink(QString::fromStdString(vm["s4l"].as<std::string>()));
	}
	else if (vm.count("input-file"))
	{
		mainWindow->loadproj(QString::fromStdString(vm["input-file"].as<std::string>()));
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
