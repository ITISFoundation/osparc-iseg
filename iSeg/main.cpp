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
#include "bmp_read_1.h"

#include "Data/Logger.h"
#include "Data/Property.h"

#include "Core/BranchItem.h"
#include "Core/Log.h"
#include "Core/Pair.h"

#include "Interface/PropertyWidget.h"

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

using namespace iseg;

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
	else
	{
		return std::make_pair(std::string(), std::string());
	}
}

// \brief Redirect VTK errors/warnings to file
class VtkCustomOutputWindow : public vtkOutputWindow
{
public:
	static VtkCustomOutputWindow* New();
	vtkTypeMacro(VtkCustomOutputWindow, vtkOutputWindow);
	void PrintSelf(ostream& os, vtkIndent indent) override {}

	// Put the text into the output stream.
	void DisplayText(const char* msg) override { std::cerr << msg << std::endl; }

protected:
	VtkCustomOutputWindow() = default;

private:
	VtkCustomOutputWindow(const VtkCustomOutputWindow&) = delete; 
	void operator=(const VtkCustomOutputWindow&) = delete;				 
};

vtkStandardNewMacro(VtkCustomOutputWindow);

} // namespace

#ifdef NDEBUG
bool _print_debug_log = false;
#else
bool print_debug_log = true;
#endif

int main(int argc, char** argv)
{
	// parse program arguments
	namespace po = boost::program_options;
	namespace fs = boost::filesystem;

	po::options_description desc("Allowed options");
	desc.add_options()("help", "produce help message")("input-file,I", po::value<std::string>(), "open iSEG project (*.prj) file")("s4l", po::value<std::string>(), "start iSEG in S4L link mode")("debug", po::bool_switch()->default_value(false), "show debug log messages")("plugin-dirs,D", po::value<std::vector<std::string>>(), "additional directories to search for plugins");
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
	auto eow = VtkCustomOutputWindow::New();
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

	bool debug = vm.count("debug") ? vm["debug"].as<bool>() : false;
	auto log_file_name = timestamped(tmpdir.absFilePath("iSeg").toStdString(), ".log");
	init_logging(log_file_name, true, debug);

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

	QString splashpicpath = picpath.absFilePath(QString("splash.png"));
	QString locationpath = file_directory.absPath();
	QString latestprojpath = tmpdir.absFilePath(QString("latestproj.txt"));
	QString settingspath = tmpdir.absFilePath(QString("settings.bin"));

	TissueInfos::InitTissues();
	BranchItem::InitAvailablelabels();

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

	MainWindow* main_window = new MainWindow(&slice_handler, locationpath, picpath, tmpdir, vm.count("s4l"), nullptr, "MainWindow", Qt::WDestructiveClose | Qt::WResizeNoErase, plugin_dirs);

	// needed on MacOS, but not WIN32?
	app.setStyleSheet(
			"QWidget { color: white; }"
			"QPushButton:checked { background-color: rgb(150,150,150); font: bold }"
			"QPushButton:disabled  { background-color: rgb(40,40,40); color: rgb(128,128,128) }"
			"QComboBox:disabled  { background-color: rgb(40,40,40); color: rgb(128,128,128) }"
			"QCheckBox:disabled  { background-color: rgb(40,40,40); color: rgb(128,128,128) }");

	main_window->LoadLoadProj(latestprojpath);
	main_window->LoadAtlas(atlasdir);
	main_window->LoadSettings(settingspath.toAscii().data());
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

#ifdef TEST_PROPERTY
	auto group = PropertyGroup::Create();
	group->SetDescription("Settings");
	auto pi = group->Add("Iterations", PropertyInt::Create(5));
	pi->SetDescription("Number of iterations");
	auto pf = group->Add("Relaxation", PropertyReal::Create(0.5));
	auto child_group = group->Add("Advanced", PropertyGroup::Create());
	child_group->SetDescription("Advanced Settings");
	auto pi2 = child_group->Add("Internal Iterations", PropertyInt::Create(2));
	auto pb = child_group->Add("Enable", PropertyBool::Create(true));
	auto pb2 = child_group->Add("Visible", PropertyBool::Create(true));
	auto pbtn0 = child_group->Add("Update", PropertyButton::Create("Update", []() {}));
	auto ps = child_group->Add("Name", PropertyString::Create("Bar"));
	auto pe = child_group->Add("Method", PropertyEnum::Create({"Foo", "Bar", "Hello"}, 0));
	auto group_p = group.get();
	auto pbtn = group->Add("Update", PropertyButton::Create("Update", [group_p]() {
		std::cerr << "PropertyButton triggered\n";
		group_p->DumpTree();
	}));

	iseg::Connect(group->onChildModified, group, [&](Property_ptr p, Property::eChangeType type) {
		ISEG_INFO(p->Name() << " : " << p->StringValue());
	});
	iseg::Connect(pb->onModified, pbtn, [&](Property_ptr p, Property::eChangeType type) {
		pbtn->SetEnabled(pb->Value());
	});
	iseg::Connect(pb2->onModified, pbtn0, [&](Property_ptr p, Property::eChangeType type) {
		pbtn0->SetVisible(pb2->Value());
	});

	auto dialog = new QDialog(main_window);
	auto layout = new QVBoxLayout;
	layout->addWidget(new PropertyWidget(group));
	dialog->setLayout(layout);
	dialog->raise();
	dialog->exec();
	return 0;
#endif

	main_window->showMaximized();
	main_window->show();

#ifdef SHOWSPLASH
	splash_screen.finish(main_window);
#endif

	QObject_connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));

	return app.exec();
}
