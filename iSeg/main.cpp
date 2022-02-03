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

#include "config.h"

#include "MainWindow.h"
#include "SlicesHandler.h"
#include "TissueInfos.h"
//#include "bmp_read_1.h"
#include "PlastiqueStyle.h"
#include "UserDir.h"
#include "vtkCustomOutputWindow.h"

#include "Interface/PropertyWidget.h"

#include "Core/BranchItem.h"
#include "Core/Log.h"
#include "Core/Pair.h"

#include "Data/Logger.h"
#include "Data/Property.h"

#include "Thirdparty/QDarkStyleSheet/dark.h"

#include <QApplication>
#include <QDateTime>
#include <QLabel>
#include <QMessageBox>
#include <QSplashScreen>
#include <QVBoxLayout>

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

#pragma warning(disable : 4100)

#define SHOWSPLASH

namespace {

std::string timestamped(const std::string prefix, const std::string& suffix)
{
	auto time_local = boost::posix_time::second_clock::local_time();
	auto tod = boost::posix_time::to_iso_string(time_local);
	return prefix + "-" + tod + suffix;
}

std::pair<std::string, std::string> custom_parser(const std::string& s)
{
	if (s.find("S4Llink") == 0)
	{
		return std::make_pair(std::string("s4l"), std::string());
	}
	return std::make_pair(std::string(), std::string());
}

} // namespace


int main(int argc, char** argv)
{
	using namespace iseg;
	namespace po = boost::program_options;
	namespace fs = boost::filesystem;

	// parse program arguments
	po::options_description desc("Allowed options");
	desc.add_options()
		("help", "produce help message")
		("input-file,I", po::value<std::string>(), "open iSEG project (*.prj) file")
		("s4l", po::value<std::string>(), "start iSEG in S4L link mode")
		("debug", po::bool_switch()->default_value(false), "show debug log messages")
		("plugin-dirs,D", po::value<std::vector<std::string>>(), "additional directories to search for plugins");
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

	// start logging
	QDir tmpdir = TempDir();
	bool debug = vm.count("debug") ? vm["debug"].as<bool>() : false;
	auto log_file_name = timestamped(tmpdir.absoluteFilePath("iSeg").toStdString(), ".log");
	init_logging(log_file_name, true, debug);

	QFileInfo fileinfo(argv[0]);
	QDir file_directory = fileinfo.dir();
	ISEG_INFO("fileDirectory = " << file_directory.absolutePath().toStdString());

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

	QString splashpicpath = picpath.absoluteFilePath(QString("splash.png"));
	QString locationpath = file_directory.absolutePath();
	QString latestprojpath = tmpdir.absoluteFilePath(QString("latestproj.txt"));
	QString settingspath = tmpdir.absoluteFilePath(QString("settings.bin"));

	TissueInfos::InitTissues();
	BranchItem::InitAvailablelabels();

	SlicesHandler slice_handler;

	QApplication app(argc, argv);
#ifdef PLASTIQUE
	SetPlastiqueStyle(&app);
#elif 1
	dark::setStyleSheet(&app);
#endif

#ifdef SHOWSPLASH
	QSplashScreen splash_screen(QPixmap(splashpicpath).scaledToHeight(600, Qt::SmoothTransformation));
	splash_screen.show();
	QTime now = QTime::currentTime();
#endif
	app.processEvents();

	MainWindow* main_window = new MainWindow(&slice_handler, locationpath, picpath, tmpdir, vm.count("s4l"), plugin_dirs);

	main_window->LoadLoadProj(latestprojpath);
	main_window->LoadAtlas(atlasdir);
	main_window->LoadSettings(settingspath.toStdString());
	if (vm.count("s4l"))
	{
		main_window->LoadS4Llink(QString::fromStdString(vm["s4l"].as<std::string>()));
	}
	else if (vm.count("input-file"))
	{
		main_window->Loadproj(QString::fromStdString(vm["input-file"].as<std::string>()));
	}

#ifdef SHOWSPLASH
	while (now.msecsTo(QTime::currentTime()) < 1000)
	{
	}
#endif

//#define TEST_PROPERTY
#ifdef TEST_PROPERTY
	auto group = PropertyGroup::Create("Settings");
	auto pb1 = group->Add("Enable", PropertyBool::Create(true));
	auto pb2 = group->Add("Visible", PropertyBool::Create(true));
	auto child_group = group->Add("Child", PropertyGroup::Create("Child Settings"));
	auto pi = child_group->Add("Int", PropertyEnum::Create({"A", "B", "C"}, 0));
	pi->SetItemEnabled(1, false);

	auto pbtn1 = group->Add("Toggle Enable", PropertyButton::Create([&]() {
		child_group->SetEnabled(!child_group->Enabled());
	}));
	auto pbtn2 = group->Add("Toggle Visible", PropertyButton::Create([&]() {
		child_group->SetVisible(!child_group->Visible());
	}));

	iseg::Connect(pb1->onModified, pi, [&](Property_ptr p, Property::eChangeType type) {
		pi->SetEnabled(pb1->Value());
	});
	iseg::Connect(pb2->onModified, pi, [&](Property_ptr p, Property::eChangeType type) {
		pi->SetVisible(pb2->Value());
	});

	auto dialog = new QDialog(main_window);
	auto layout = new QVBoxLayout;
	layout->addWidget(new PropertyWidget(group));
	dialog->setLayout(layout);
	dialog->setMinimumWidth(450);
	dialog->setMinimumHeight(400);
	dialog->raise();
	dialog->exec();
	exit(EXIT_SUCCESS);
#endif

	main_window->showMaximized();
	main_window->show();

#ifdef SHOWSPLASH
	splash_screen.finish(main_window);
#endif

	QObject_connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));

	return app.exec();
}
