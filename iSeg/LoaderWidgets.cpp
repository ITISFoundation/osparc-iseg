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

#define cimg_display 0
#include "CImg.h"
#include "LoaderWidgets.h"
#include "XdmfImageReader.h"

#include "Interface/LayoutTools.h"
#include "Interface/RecentPlaces.h"

#include "Data/Point.h"
#include "Data/ScopedTimer.h"

#include "Core/ColorLookupTable.h"
#include "Core/ImageReader.h"
#include "Core/ImageWriter.h"

#include "Thirdparty/nanoflann/nanoflann.hpp"

#include <QButtonGroup>
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QRadioButton>
#include <QString>

#include <boost/algorithm/string.hpp>
#include <boost/dll.hpp>
#include <boost/filesystem.hpp>

namespace {
template<class VectorOfVectorsType, typename num_t = double, int DIM = -1, class Distance = nanoflann::metric_L2, typename IndexType = size_t>
struct KDTreeVectorOfVectorsAdaptor
{
	using self_t = KDTreeVectorOfVectorsAdaptor<VectorOfVectorsType, num_t, DIM, Distance>;
	using metric_t = typename Distance::template traits<num_t, self_t>::distance_t;
	using index_t = nanoflann::KDTreeSingleIndexAdaptor<metric_t, self_t, DIM, IndexType>;

	index_t* m_Index; //! The kd-tree index for the user to call its methods as usual with any other FLANN index.

	/// Constructor: takes a const ref to the vector of vectors object with the data points
	KDTreeVectorOfVectorsAdaptor(const size_t /* dimensionality */, const VectorOfVectorsType& mat, const int leaf_max_size = 10) : m_MData(mat)
	{
		assert(!mat.empty() && !mat[0].empty());
		const size_t dims = mat[0].size();
		if (DIM > 0 && static_cast<int>(dims) != DIM)
			throw std::runtime_error("Data set dimensionality does not match the 'DIM' template argument");
		m_Index = new index_t(static_cast<int>(dims), *this /* adaptor */, nanoflann::KDTreeSingleIndexAdaptorParams(leaf_max_size));
		m_Index->buildIndex();
	}

	~KDTreeVectorOfVectorsAdaptor()
	{
		delete m_Index;
	}

	const VectorOfVectorsType& m_MData;

	inline void Query(const num_t* query_point, const size_t num_closest, IndexType* out_indices, num_t* out_distances_sq, const int nChecks_IGNORED = 10) const
	{
		nanoflann::KNNResultSet<num_t, IndexType> result_set(num_closest);
		result_set.init(out_indices, out_distances_sq);
		m_Index->findNeighbors(result_set, query_point, nanoflann::SearchParams());
	}

	const self_t& derived() const { return *this; } // NOLINT
	self_t& derived() { return *this; }							// NOLINT

	inline size_t kdtree_get_point_count() const { return m_MData.size(); } // NOLINT

	inline num_t kdtree_get_pt(const size_t idx, const size_t dim) const // NOLINT
	{
		return m_MData[idx][dim];
	}

	template<class BBOX>
	bool kdtree_get_bbox(BBOX&) const // NOLINT
	{
		return false;
	}
};
} // namespace

namespace iseg {

namespace algo = boost::algorithm;
namespace fs = boost::filesystem;

ExportImg::ExportImg(SlicesHandler* h, QWidget* p, Qt::WindowFlags f)
		: QDialog(p, f), m_Handler3D(h)
{
	auto img_selection_hbox = new QHBoxLayout;
	m_ImgSelectionGroup = make_button_group({"Source", "Target", "Tissue"}, 0);
	for (auto b : m_ImgSelectionGroup->buttons())
	{
		img_selection_hbox->addWidget(b);
	}

	auto slice_selection_hbox = new QHBoxLayout;
	m_SliceSelectionGroup = make_button_group({"Current Slice", "Active Slices"}, 0);
	for (auto b : m_SliceSelectionGroup->buttons())
	{
		slice_selection_hbox->addWidget(b);
	}

	auto button_hbox = new QHBoxLayout;
	button_hbox->addWidget(m_PbSave = new QPushButton("OK"));
	button_hbox->addWidget(m_PbCancel = new QPushButton("Cancel"));

	auto top_layout = new QVBoxLayout;
	top_layout->addLayout(img_selection_hbox);
	top_layout->addLayout(slice_selection_hbox);
	top_layout->addLayout(button_hbox);
	setLayout(top_layout);

	QObject_connect(m_PbSave, SIGNAL(clicked()), this, SLOT(SavePushed()));
	QObject_connect(m_PbCancel, SIGNAL(clicked()), this, SLOT(close()));
}

void ExportImg::SavePushed()
{
	// todo: what to do about file series, e.g. select with some option (including base name), select directory (or save file name without extension, then append)
	QString filter =
			"Nifty file (*.nii.gz *nii.gz)\n"
			"Analyze file (*.hdr *.img)\n"
			"Nrrd file (*.nrrd)\n"
			"VTK file (*.vtk *vti)\n"
			"BMP file (*.bmp)\n"
			"PNG file (*.png)\n"
			"JPG file (*.jpg *.jpeg)";

	std::string file_path = RecentPlaces::GetSaveFileName(this, "Save As", QString::null, filter).toStdString();
	auto img_selection = static_cast<ImageWriter::eImageSelection>(m_ImgSelectionGroup->checkedId());
	auto slice_selection = static_cast<ImageWriter::eSliceSelection>(m_SliceSelectionGroup->checkedId());

	ImageWriter w(true);
	w.WriteVolume(file_path, m_Handler3D, img_selection, slice_selection);

	close();
}

LoaderDicom::LoaderDicom(SlicesHandler* hand3D, QStringList* lname, bool breload, QWidget* parent, Qt::WindowFlags wFlags)
		: QDialog(parent, wFlags), m_Handler3D(hand3D), m_Reload(breload), m_Lnames(lname)
{
	setModal(true);

	m_CbSubsect = new QCheckBox("Subsection ");
	m_CbSubsect->setChecked(false);
	m_SubsectArea = new QWidget;
	{
		m_Xoffset = new QSpinBox;
		m_Xoffset->setRange(0, 2000);
		m_Xoffset->setValue(0);
		m_Yoffset = new QSpinBox;
		m_Yoffset->setRange(0, 2000);
		m_Yoffset->setValue(0);

		m_Xlength = new QSpinBox;
		m_Xlength->setRange(0, 2000);
		m_Xlength->setValue(512);
		m_Ylength = new QSpinBox;
		m_Ylength->setRange(0, 2000);
		m_Ylength->setValue(512);

		auto col1 = new QVBoxLayout;
		col1->addWidget(new QLabel("x-Offset: "));
		col1->addWidget(new QLabel("y-Offset: "));
		auto col2 = new QVBoxLayout;
		col2->addWidget(m_Xoffset);
		col2->addWidget(m_Yoffset);
		auto col3 = new QVBoxLayout;
		col3->addWidget(new QLabel("x-Length: "));
		col3->addWidget(new QLabel("y-Length: "));
		auto col4 = new QVBoxLayout;
		col4->addWidget(m_Xlength);
		col4->addWidget(m_Ylength);

		auto hbox = new QHBoxLayout;
		hbox->addLayout(col1);
		hbox->addLayout(col2);
		hbox->addLayout(col3);
		hbox->addLayout(col4);
		m_SubsectArea->setLayout(hbox);
	}

	m_CbCt = new QCheckBox("CT weight ");
	m_CbCt->setChecked(false);
	m_CtWeightArea = new QWidget;
	{
		m_BgWeight = new QButtonGroup(this);
		m_RbBone = new QRadioButton("Bone");
		m_RbMuscle = new QRadioButton("Muscle");
		m_BgWeight->insert(m_RbBone);
		m_BgWeight->insert(m_RbMuscle);
		m_RbMuscle->setChecked(true);

		m_CbCrop = new QCheckBox("Clamp values ");
		m_CbCrop->setChecked(true);

		auto hbox = new QHBoxLayout;
		hbox->addWidget(m_RbBone);
		hbox->addWidget(m_RbMuscle);
		hbox->addWidget(m_CbCrop);

		m_CtWeightArea->setLayout(hbox);
	}

	m_Seriesnrselection = new QComboBox;
	if (!m_Lnames->empty())
	{
		std::vector<const char*> vnames;
		for (auto it = m_Lnames->begin(); it != m_Lnames->end(); it++)
		{
			vnames.push_back((*it).ascii());
		}
		m_Dicomseriesnr.clear();
		m_Handler3D->GetDICOMseriesnr(&vnames, &m_Dicomseriesnr, &m_Dicomseriesnrlist);
		if (m_Dicomseriesnr.size() > 1)
		{
			for (unsigned i = 0; i < (unsigned)m_Dicomseriesnr.size(); i++)
			{
				QString str;
				m_Seriesnrselection->addItem(str = str.setNum((int)m_Dicomseriesnr.at(i)));
			}
			m_Seriesnrselection->setCurrentIndex(0);
		}
	}

	m_LoadFile = new QPushButton("Open");
	m_CancelBut = new QPushButton("Cancel");

	// layout
	auto hbox6 = new QHBoxLayout;
	hbox6->addWidget(new QLabel("Series-Nr.: "));
	hbox6->addWidget(m_Seriesnrselection);

	auto hbox5 = new QHBoxLayout;
	hbox5->addWidget(m_LoadFile);
	hbox5->addWidget(m_CancelBut);

	auto layout = new QVBoxLayout;
	layout->addWidget(m_CbSubsect);
	layout->addWidget(m_SubsectArea);
	layout->addWidget(m_CbCt);
	layout->addWidget(m_CtWeightArea);
	layout->addLayout(hbox6);
	layout->addLayout(hbox5);
	setLayout(layout);

	SubsectToggled();
	CtToggled();

	QObject_connect(m_LoadFile, SIGNAL(clicked()), this, SLOT(LoadPushed()));
	QObject_connect(m_CancelBut, SIGNAL(clicked()), this, SLOT(close()));
	QObject_connect(m_CbSubsect, SIGNAL(clicked()), this, SLOT(SubsectToggled()));
	QObject_connect(m_CbCt, SIGNAL(clicked()), this, SLOT(CtToggled()));
}

LoaderDicom::~LoaderDicom()
{
	delete m_BgWeight;
}

void LoaderDicom::SubsectToggled()
{
	m_SubsectArea->setVisible(m_CbSubsect->isChecked());
}

void LoaderDicom::CtToggled()
{
	m_CtWeightArea->setVisible(m_CbCt->isChecked());
}

void LoaderDicom::LoadPushed()
{
	if (!m_Lnames->empty())
	{
		std::vector<const char*> vnames;
		if (m_Dicomseriesnr.size() > 1)
		{
			unsigned pos = 0;
			for (const auto& name : *m_Lnames)
			{
				if (m_Dicomseriesnrlist[pos++] == m_Dicomseriesnr[m_Seriesnrselection->currentIndex()])
				{
					vnames.push_back(name.ascii());
				}
			}
		}
		else
		{
			for (const auto& name : *m_Lnames)
			{
				vnames.push_back(name.ascii());
			}
		}

		if (m_CbSubsect->isChecked())
		{
			Point p;
			p.px = m_Xoffset->value();
			p.py = m_Yoffset->value();
			if (m_Reload)
				m_Handler3D->ReloadDICOM(vnames, p);
			else
				m_Handler3D->LoadDICOM(vnames, p, m_Xlength->value(), m_Ylength->value());
		}
		else
		{
			if (m_Reload)
				m_Handler3D->ReloadDICOM(vnames);
			else
				m_Handler3D->LoadDICOM(vnames);
		}

		if (m_CbCt->isChecked())
		{
			Pair p;
			if (m_RbMuscle->isChecked())
			{
				p.high = 1190;
				p.low = 890;
			}
			else if (m_RbBone->isChecked())
			{
				m_Handler3D->GetRange(&p);
			}
			m_Handler3D->ScaleColors(p);
			if (m_CbCrop->isChecked())
			{
				m_Handler3D->CropColors();
			}
			m_Handler3D->Work2bmpall();
		}

		close();
	}
	else
	{
		close();
	}
}

LoaderRaw::LoaderRaw(SlicesHandler* hand3D, const QString& file_path, QWidget* parent, Qt::WindowFlags wFlags)
		: QDialog(parent, wFlags), m_Handler3D(hand3D), m_FileName(file_path)
{
	setModal(true);

	m_Bit8 = new QRadioButton("8-bit");
	m_Bit16 = new QRadioButton("16-bit");
	m_Bitselect = new QButtonGroup(this);
	m_Bitselect->insert(m_Bit8);
	m_Bitselect->insert(m_Bit16);
	m_Bit8->setChecked(true);

	m_Slicenrbox = new QSpinBox;
	m_Slicenrbox->setRange(0, 2000);
	m_Slicenrbox->setValue(0);

	m_SbNrslices = new QSpinBox;
	m_Slicenrbox->setRange(1, 10000);
	m_SbNrslices->setValue(10);

	m_Subsect = new QCheckBox("Subsection ");
	m_Subsect->setChecked(false);

	m_Xlength1 = new QSpinBox;
	m_Xlength1->setRange(0, 2000);
	m_Xlength1->setValue(512);
	m_Ylength1 = new QSpinBox;
	m_Ylength1->setRange(0, 2000);
	m_Ylength1->setValue(512);
	
	m_Xlength = new QSpinBox;
	m_Xlength->setRange(0, 2000);
	m_Xlength->setValue(256);
	m_Ylength = new QSpinBox;
	m_Ylength->setRange(0, 2000);
	m_Ylength->setValue(256);

	m_Xoffset = new QSpinBox;
	m_Xoffset->setRange(0, 2000);
	m_Xoffset->setValue(0);
	m_Yoffset = new QSpinBox;
	m_Yoffset->setRange(0, 2000);
	m_Yoffset->setValue(0);

	m_LoadFile = new QPushButton("Open");
	m_CancelBut = new QPushButton("Cancel");

	// layout
	auto hbox_opt = new QHBoxLayout;
	hbox_opt->addWidget(m_Bit8);
	hbox_opt->addWidget(m_Bit16);

	auto hbox_size = new QHBoxLayout;
	hbox_size->addWidget(new QLabel("Total x-Length: "));
	hbox_size->addWidget(m_Xlength1);
	hbox_size->addWidget(new QLabel("Total y-Length: "));
	hbox_size->addWidget(m_Ylength1);

	auto hbox_slices = new QHBoxLayout;
	hbox_slices->addWidget(new QLabel("Start Slice: "));
	hbox_slices->addWidget(m_Slicenrbox);
	hbox_slices->addWidget(new QLabel("# Slices: "));
	hbox_slices->addWidget(m_SbNrslices);

	m_SubsectArea = new QWidget;
	{
		auto hbox_len = new QHBoxLayout;
		hbox_len->addWidget(new QLabel("x-Length: "));
		hbox_len->addWidget(m_Xlength);
		hbox_len->addWidget(new QLabel("y-Length: "));
		hbox_len->addWidget(m_Ylength);

		auto hbox_offset = new QHBoxLayout;
		hbox_offset->addWidget(new QLabel("x-Offset: "));
		hbox_offset->addWidget(m_Xoffset);
		hbox_offset->addWidget(new QLabel("y-Offset: "));
		hbox_offset->addWidget(m_Yoffset);

		auto vbox = new QVBoxLayout;
		vbox->addLayout(hbox_len);
		vbox->addLayout(hbox_offset);

		m_SubsectArea->setLayout(vbox);
	}

	auto hbox_buttons = new QHBoxLayout;
	hbox_buttons->addWidget(m_LoadFile);
	hbox_buttons->addWidget(m_CancelBut);

	auto layout = new QVBoxLayout;
	layout->addLayout(hbox_opt);
	layout->addLayout(hbox_size);
	layout->addLayout(hbox_slices);
	layout->addWidget(m_Subsect);
	layout->addWidget(m_SubsectArea);
	layout->addLayout(hbox_buttons);
	setLayout(layout);

	QObject_connect(m_LoadFile, SIGNAL(clicked()), this, SLOT(LoadPushed()));
	QObject_connect(m_CancelBut, SIGNAL(clicked()), this, SLOT(close()));
	QObject_connect(m_Subsect, SIGNAL(clicked()), this, SLOT(SubsectToggled()));

	SubsectToggled();
}

LoaderRaw::~LoaderRaw()
{
	delete m_Bitselect;
}

QString LoaderRaw::GetFileName() const { return m_FileName; }

std::array<unsigned int, 2> LoaderRaw::GetDimensions() const
{
	return {static_cast<unsigned>(m_Xlength1->value()), static_cast<unsigned>(m_Ylength1->value())};
}

std::array<unsigned int, 3> LoaderRaw::GetSubregionStart() const
{
	return {static_cast<unsigned>(m_Xoffset->value()), static_cast<unsigned>(m_Yoffset->value()), static_cast<unsigned>(m_Slicenrbox->value())};
}

std::array<unsigned int, 3> LoaderRaw::GetSubregionSize() const
{
	return {static_cast<unsigned>(m_Subsect->isChecked() ? m_Xlength->value() : m_Xlength1->value()), static_cast<unsigned>(m_Subsect->isChecked() ? m_Ylength->value() : m_Ylength1->value()), static_cast<unsigned>(m_SbNrslices->value())};
}

int LoaderRaw::GetBits() const
{
	return (m_Bit8->isChecked() ? 8 : 16);
}

void LoaderRaw::SubsectToggled()
{
	m_SubsectArea->setVisible(m_Subsect->isChecked());
}

void LoaderRaw::LoadPushed()
{
	unsigned bitdepth;
	if (m_Bit8->isChecked())
		bitdepth = 8;
	else
		bitdepth = 16;
	if (!m_FileName.isEmpty())
	{
		if (m_SkipReading)
		{
			// do nothing
		}
		else if (m_Subsect->isOn())
		{
			Point p;
			p.px = m_Xoffset->value();
			p.py = m_Yoffset->value();
			m_Handler3D->ReadRaw(m_FileName.toLatin1(), (unsigned short)m_Xlength1->value(), (unsigned short)m_Ylength1->value(), (unsigned)bitdepth, (unsigned short)m_Slicenrbox->value(), (unsigned short)m_SbNrslices->value(), p, (unsigned short)m_Xlength->value(), (unsigned short)m_Ylength->value());
		}
		else
		{
			m_Handler3D->ReadRaw(m_FileName.toLatin1(), (unsigned short)m_Xlength1->value(), (unsigned short)m_Ylength1->value(), (unsigned)bitdepth, (unsigned short)m_Slicenrbox->value(), (unsigned short)m_SbNrslices->value());
		}
		close();
	}
}

ReloaderRaw::ReloaderRaw(SlicesHandler* hand3D, const QString& file_path, QWidget* parent, Qt::WindowFlags wFlags)
		: QDialog(parent, wFlags), m_Handler3D(hand3D), m_FileName(file_path)
{
	setModal(true);

	m_Bit8 = new QRadioButton("8-bit");
	m_Bit16 = new QRadioButton("16-bit");
	m_Bitselect = new QButtonGroup(this);
	m_Bitselect->insert(m_Bit8);
	m_Bitselect->insert(m_Bit16);
	m_Bit8->setChecked(true);

	m_Slicenrbox = new QSpinBox;
	m_Slicenrbox->setRange(0, 2000);
	m_Slicenrbox->setValue(0);

	m_Subsect = new QCheckBox("Subsection ");
	m_Subsect->setChecked(false);

	m_Xlength1 = new QSpinBox;
	m_Xlength1->setRange(0, 2000);
	m_Xlength1->setValue(512);
	m_Ylength1 = new QSpinBox;
	m_Ylength1->setRange(0, 2000);
	m_Ylength1->setValue(512);

	m_Xoffset = new QSpinBox;
	m_Xoffset->setRange(0, 2000);
	m_Xoffset->setValue(0);
	m_Yoffset = new QSpinBox;
	m_Yoffset->setRange(0, 2000);
	m_Yoffset->setValue(0);

	m_LoadFile = new QPushButton("Open");
	m_CancelBut = new QPushButton("Cancel");

	// layout
	auto hbox_opt = new QHBoxLayout;
	hbox_opt->addWidget(m_Bit8);
	hbox_opt->addWidget(m_Bit16);
	hbox_opt->addWidget(new QLabel("Slice: "));
	hbox_opt->addWidget(m_Slicenrbox);

	m_SubsectArea = new QWidget;
	{
		auto hbox_size = new QHBoxLayout;
		hbox_size->addWidget(new QLabel("x-Length: "));
		hbox_size->addWidget(m_Xlength1);
		hbox_size->addWidget(new QLabel("y-Length: "));
		hbox_size->addWidget(m_Ylength1);

		auto hbox_offset = new QHBoxLayout;
		hbox_offset->addWidget(new QLabel("x-Offset: "));
		hbox_offset->addWidget(m_Xoffset);
		hbox_offset->addWidget(new QLabel("y-Offset: "));
		hbox_offset->addWidget(m_Yoffset);

		auto vbox = new QVBoxLayout;
		vbox->addLayout(hbox_size);
		vbox->addLayout(hbox_offset);

		m_SubsectArea->setLayout(vbox);
	}

	auto hbox_buttons = new QHBoxLayout;
	hbox_buttons->addWidget(m_LoadFile);
	hbox_buttons->addWidget(m_CancelBut);

	auto layout = new QVBoxLayout;
	layout->addLayout(hbox_opt);
	layout->addWidget(m_Subsect);
	layout->addWidget(m_SubsectArea);
	layout->addLayout(hbox_buttons);
	setLayout(layout);

	SubsectToggled();

	QObject_connect(m_CancelBut, SIGNAL(clicked()), this, SLOT(close()));
	QObject_connect(m_Subsect, SIGNAL(clicked()), this, SLOT(SubsectToggled()));
}

ReloaderRaw::~ReloaderRaw()
{
	delete m_Bitselect;
}

void ReloaderRaw::SubsectToggled()
{
	m_SubsectArea->setVisible(m_Subsect->isChecked());
}

void ReloaderRaw::LoadPushed()
{
	unsigned bitdepth;
	if (m_Bit8->isChecked())
		bitdepth = 8;
	else
		bitdepth = 16;

	if (!m_FileName.isEmpty())
	{
		if (m_Subsect->isChecked())
		{
			Point p;
			p.px = m_Xoffset->value();
			p.py = m_Yoffset->value();
			m_Handler3D->ReloadRaw(m_FileName.toLatin1(), m_Xlength1->value(), m_Ylength1->value(), bitdepth, m_Slicenrbox->value(), p);
		}
		else
		{
			m_Handler3D->ReloadRaw(m_FileName.toLatin1(), bitdepth, m_Slicenrbox->value());
		}
		close();
	}
}

NewImg::NewImg(SlicesHandler* hand3D, QWidget* parent, Qt::WindowFlags wFlags)
		: QDialog(parent, wFlags), m_Handler3D(hand3D)
{
	setModal(true);

	m_Xlength = new QSpinBox;
	m_Xlength->setRange(1, 5000);
	m_Xlength->setValue(512);

	m_Ylength = new QSpinBox;
	m_Ylength->setRange(1, 5000);
	m_Ylength->setValue(512);

	m_SbNrslices = new QSpinBox;
	m_SbNrslices->setRange(1, 5000);
	m_SbNrslices->setValue(10);

	m_NewFile = new QPushButton("New");
	m_CancelBut = new QPushButton("Cancel");

	auto hbox1 = new QHBoxLayout;
	hbox1->addWidget(new QLabel("Width: "));
	hbox1->addWidget(m_Xlength);
	hbox1->addWidget(new QLabel("Height: "));
	hbox1->addWidget(m_Ylength);

	auto hbox2 = new QHBoxLayout;
	hbox2->addWidget(new QLabel("Slices: "));
	hbox2->addWidget(m_SbNrslices);

	auto hbox3 = new QHBoxLayout;
	hbox3->addWidget(m_NewFile);
	hbox3->addWidget(m_CancelBut);

	auto layout = new QVBoxLayout;
	layout->addLayout(hbox1);
	layout->addLayout(hbox2);
	layout->addLayout(hbox3);
	setLayout(layout);

	QObject_connect(m_NewFile, SIGNAL(clicked()), this, SLOT(NewPushed()));
	QObject_connect(m_CancelBut, SIGNAL(clicked()), this, SLOT(OnClose()));

	m_NewPressed = false;
}

NewImg::~NewImg() {}

bool NewImg::NewPressed() const { return m_NewPressed; }

void NewImg::NewPushed()
{
	m_Handler3D->UpdateColorLookupTable(nullptr);

	m_Handler3D->Newbmp((unsigned short)m_Xlength->value(), (unsigned short)m_Ylength->value(), (unsigned short)m_SbNrslices->value());
	m_NewPressed = true;
	close();
}

void NewImg::OnClose()
{
	m_NewPressed = false;
	close();
}

LoaderColorImages::LoaderColorImages(SlicesHandler* hand3D, eImageType typ, std::vector<const char*> filenames, QWidget* parent, Qt::WindowFlags wFlags)
		: QDialog(parent, wFlags), m_Handler3D(hand3D), m_Type(typ), m_MFilenames(filenames)
{
	setModal(true);

	m_MapToLut = new QCheckBox("Map colors to lookup table");
	m_MapToLut->setChecked(true);
	if (typ == LoaderColorImages::kTIF)
	{
		m_MapToLut->setEnabled(false);
	}

	m_Subsect = new QCheckBox("Subsection");
	m_Subsect->setChecked(false);

	auto xoffs = new QLabel("x-Offset: ");
	m_Xoffset = new QSpinBox(0, 2000, 1, nullptr);
	m_Xoffset->setValue(0);

	auto yoffs = new QLabel("y-Offset: ");
	m_Yoffset = new QSpinBox(0, 2000, 1, nullptr);
	m_Xoffset->setValue(0);

	auto xl = new QLabel("x-Length: ");
	m_Xlength = new QSpinBox(0, 2000, 1, nullptr);
	m_Xlength->setValue(256);

	auto yl = new QLabel("y-Length: ");
	m_Ylength = new QSpinBox(0, 2000, 1, nullptr);
	m_Ylength->setValue(256);

	auto subsect_layout = new QGridLayout(2, 4);
	subsect_layout->addWidget(xoffs);
	subsect_layout->addWidget(m_Xoffset);
	subsect_layout->addWidget(xl);
	subsect_layout->addWidget(m_Xlength);
	subsect_layout->addWidget(yoffs);
	subsect_layout->addWidget(m_Yoffset);
	subsect_layout->addWidget(yl);
	subsect_layout->addWidget(m_Ylength);
	auto subsect_options = new QWidget;
	subsect_options->setLayout(subsect_layout);

	m_LoadFile = new QPushButton("Open");
	m_CancelBut = new QPushButton("Cancel");
	auto button_layout = new QHBoxLayout;
	button_layout->addWidget(m_LoadFile);
	button_layout->addWidget(m_CancelBut);
	auto button_row = new QWidget;
	button_row->setLayout(button_layout);

	auto top_layout = new QVBoxLayout;
	top_layout->addWidget(m_MapToLut);
	top_layout->addWidget(m_Subsect);
	top_layout->addWidget(subsect_options);
	top_layout->addWidget(button_row);
	setLayout(top_layout);
	setMinimumSize(150, 200);

	MapToLutToggled();

	QObject_connect(m_LoadFile, SIGNAL(clicked()), this, SLOT(LoadPushed()));
	QObject_connect(m_CancelBut, SIGNAL(clicked()), this, SLOT(close()));
	QObject_connect(m_MapToLut, SIGNAL(clicked()), this, SLOT(MapToLutToggled()));
}

LoaderColorImages::~LoaderColorImages() = default;

void LoaderColorImages::MapToLutToggled()
{
	// enable/disable
	m_Subsect->setEnabled(!m_MapToLut->isChecked());
}

void LoaderColorImages::LoadPushed()
{
	if (m_MapToLut->isChecked())
	{
		LoadQuantize();
	}
	else
	{
		LoadMixer();
	}
}

void LoaderColorImages::LoadQuantize()
{
	QString initial_dir = QString::null;

	auto lut_path = boost::dll::program_location().parent_path() / fs::path("lut");
	if (fs::exists(lut_path))
	{
		fs::directory_iterator dir_itr(lut_path);
		fs::directory_iterator end_iter;
		for (; dir_itr != end_iter; ++dir_itr)
		{
			fs::path lut_file(dir_itr->path());
			if (algo::iends_with(lut_file.string(), ".lut"))
			{
				initial_dir = QString::fromStdString(lut_file.parent_path().string());
				break;
			}
		}
	}

	QString filename = RecentPlaces::GetOpenFileName(this, "Open Lookup Table", initial_dir, "iSEG Color Lookup Table (*.lut *.h5)\nAll (*.*)");
	if (!filename.isEmpty())
	{
		XdmfImageReader reader;
		reader.SetFileName(filename.toStdString().c_str());
		std::shared_ptr<ColorLookupTable> lut;
		{
			ScopedTimer t("Load LUT");
			lut = reader.ReadColorLookup();
		}
		if (lut)
		{
			const auto n = lut->NumberOfColors();

			using color_type = std::array<unsigned char, 3>;
			using color_vector_type = std::vector<color_type>;

			color_vector_type points(n);
			for (size_t i = 0; i < n; ++i)
			{
				lut->GetColor(i, points[i].data());
			}

			using distance_type = float;
			using kd_tree_type = KDTreeVectorOfVectorsAdaptor<color_vector_type, distance_type>;
			kd_tree_type tree(3, points, 10 /* max leaf */);
			{
				ScopedTimer t("Build kd-tree for colors");
				tree.m_Index->buildIndex();
			}

			unsigned w, h;
			if (ImageReader::GetInfo2D(m_MFilenames[0], w, h))
			{
				const auto map_colors = [&tree](unsigned char r, unsigned char g, unsigned char b) -> float {
					size_t id;
					distance_type sqr_dist;
					std::array<distance_type, 3> query_pt = {
							static_cast<distance_type>(r), static_cast<distance_type>(g), static_cast<distance_type>(b)};
					tree.Query(&query_pt[0], 1, &id, &sqr_dist);
					return static_cast<float>(id);
				};

				auto load = [&, this](float** slices) {
					ScopedTimer t("Load and map image stack");
					ImageReader::GetImageStack(m_MFilenames, slices, w, h, map_colors);
				};

				m_Handler3D->Newbmp(w, h, static_cast<unsigned short>(m_MFilenames.size()), load);
				m_Handler3D->UpdateColorLookupTable(lut);
			}
		}
		else
		{
			QMessageBox::warning(this, "iSeg", "ERROR: occurred while reading color lookup table\n", QMessageBox::Ok | QMessageBox::Default);
		}
	}

	close();
}

void LoaderColorImages::LoadMixer()
{
	if ((m_Type == eImageType::kBMP && Bmphandler::CheckBMPDepth(m_MFilenames[0]) > 8) ||
			(m_Type == eImageType::kPNG && Bmphandler::CheckPNGDepth(m_MFilenames[0]) > 8))
	{
		ChannelMixer channel_mixer(m_MFilenames, nullptr);
		channel_mixer.move(QCursor::pos());
		if (!channel_mixer.exec())
		{
			close();
			return;
		}

		int red_factor = channel_mixer.GetRedFactor();
		int green_factor = channel_mixer.GetGreenFactor();
		int blue_factor = channel_mixer.GetBlueFactor();
		m_Handler3D->SetRgbFactors(red_factor, green_factor, blue_factor);
	}
	else
	{
		m_Handler3D->SetRgbFactors(30, 59, 11);
	}

	if (m_Subsect->isChecked())
	{
		Point p;
		p.px = m_Xoffset->value();
		p.py = m_Yoffset->value();

		switch (m_Type)
		{
		case eImageType::kPNG:
			m_Handler3D->LoadPng(m_MFilenames, p, m_Xlength->value(), m_Ylength->value());
			break;
		case eImageType::kBMP:
			m_Handler3D->LoadDIBitmap(m_MFilenames, p, m_Xlength->value(), m_Ylength->value());
			break;
		case eImageType::kJPG:
			m_Handler3D->LoadDIJpg(m_MFilenames, p, m_Xlength->value(), m_Ylength->value());
			break;
		default:
			break;
		}
	}
	else
	{
		switch (m_Type)
		{
		case eImageType::kPNG:
			m_Handler3D->LoadPng(m_MFilenames);
			break;
		case eImageType::kBMP:
			m_Handler3D->LoadDIBitmap(m_MFilenames);
			break;
		case eImageType::kJPG:
			m_Handler3D->LoadDIJpg(m_MFilenames);
			break;
		default:
			break;
		}
	}
	close();
}

ClickableLabel::ClickableLabel(QWidget* parent, Qt::WindowFlags f)
		: QLabel(parent, f)
{
	m_CenterX = width() / 2;
	m_CenterY = height() / 2;
	m_SquareWidth = 24;
	m_SquareHeight = 24;
}

ClickableLabel::ClickableLabel(const QString& text, QWidget* parent, Qt::WindowFlags f)
		: QLabel(text, parent, f)
{
}

void ClickableLabel::SetSquareWidth(int width) { m_SquareWidth = width; }

void ClickableLabel::SetSquareHeight(int height) { m_SquareHeight = height; }

void ClickableLabel::SetCenter(QPoint newCenter)
{
	m_CenterX = newCenter.x();
	m_CenterY = newCenter.y();
	emit NewCenterPreview(QPoint(m_CenterX, m_CenterY));
}

void ClickableLabel::mousePressEvent(QMouseEvent* ev)
{
	m_CenterX = ev->pos().x();
	m_CenterY = ev->pos().y();
	emit NewCenterPreview(QPoint(m_CenterX, m_CenterY));
}

void ClickableLabel::paintEvent(QPaintEvent* e)
{
	QLabel::paintEvent(e);

	QPainter painter(this);

	QPen paintpen(Qt::yellow);
	paintpen.setWidth(1);
	painter.setPen(paintpen);

	QPainterPath square;
	square.addRect(m_CenterX - m_SquareHeight / 2, m_CenterY - m_SquareHeight / 2, m_SquareWidth, m_SquareHeight);
	painter.drawPath(square);
}

ChannelMixer::ChannelMixer(std::vector<const char*> filenames, QWidget* parent, Qt::WindowFlags wFlags)
		: QDialog(parent, wFlags), m_MFilenames(filenames)
{
	setModal(true);

	m_PreviewCenter.setX(0);
	m_PreviewCenter.setY(0);
	QString file_name = QString::fromUtf8(m_MFilenames[0]);
	if (!file_name.isEmpty())
	{
		m_SourceImage = QImage(file_name);
		if (m_SourceImage.isNull())
		{
			QMessageBox::information(this, tr("Image Viewer"), tr("Cannot load %1.").arg(file_name));
			return;
		}
		m_PreviewCenter.setX(m_SourceImage.width());
		m_PreviewCenter.setY(m_SourceImage.height());
	}

	QSize standard_box_size;
	standard_box_size.setWidth(m_ScaleX);
	standard_box_size.setHeight(m_ScaleY);

	m_ImageSourceLabel = new ClickableLabel;
	m_ImageSourceLabel->setFixedSize(standard_box_size);
	m_ImageSourceLabel->SetSquareWidth(25);
	m_ImageSourceLabel->SetSquareHeight(25);
	m_ImageSourceLabel->setAlignment(Qt::AlignCenter);

	m_ImageLabel = new QLabel;
	m_ImageLabel->setFixedSize(standard_box_size);

	m_SliderRed = new QSlider(Qt::Horizontal);
	m_SliderRed->setRange(0, 100);
	m_SliderRed->setValue(m_RedFactor);
	m_LabelRedValue = new QLineEdit(QString::number(m_SliderRed->value()));

	m_SliderGreen = new QSlider(Qt::Horizontal);
	m_SliderGreen->setRange(0, 100);
	m_SliderGreen->setValue(m_GreenFactor);
	m_LabelGreenValue = new QLineEdit(QString::number(m_SliderGreen->value()));

	m_SliderBlue = new QSlider(Qt::Horizontal);
	m_SliderBlue->setRange(0, 100);
	m_SliderBlue->setValue(m_BlueFactor);
	m_LabelBlueValue = new QLineEdit(QString::number(m_SliderBlue->value()));

	m_LabelPreviewAlgorithm = new QLabel;

	m_SpinSlice = new QSpinBox;
	m_SpinSlice->setRange(0, static_cast<int>(filenames.size()) - 1);
	m_SpinSlice->setValue(0);
	m_SelectedSlice = m_SpinSlice->value();

	m_LoadFile = new QPushButton("Open");
	m_CancelBut = new QPushButton("Cancel");

	// layout
	auto hbox_preview = new QHBoxLayout;
	hbox_preview->addWidget(m_ImageSourceLabel);
	hbox_preview->addWidget(m_ImageLabel);

	auto hbox_red = new QHBoxLayout;
	hbox_red->addWidget(new QLabel("Red"));
	hbox_red->addWidget(m_SliderRed);
	hbox_red->addWidget(m_LabelRedValue);

	auto hbox_green = new QHBoxLayout;
	hbox_green->addWidget(new QLabel("Green"));
	hbox_green->addWidget(m_SliderGreen);
	hbox_green->addWidget(m_LabelGreenValue);

	auto hbox_blue = new QHBoxLayout;
	hbox_blue->addWidget(new QLabel("Blue"));
	hbox_blue->addWidget(m_SliderBlue);
	hbox_blue->addWidget(m_LabelBlueValue);

	auto hbox_slice = new QHBoxLayout;
	hbox_slice->addWidget(new QLabel("Slice"));
	hbox_slice->addWidget(m_SpinSlice);

	auto hbox_buttons = new QHBoxLayout;
	hbox_buttons->addWidget(m_LoadFile);
	hbox_buttons->addWidget(m_CancelBut);

	auto layout = new QVBoxLayout;
	layout->addLayout(hbox_preview);
	layout->addLayout(hbox_red);
	layout->addLayout(hbox_green);
	layout->addLayout(hbox_blue);
	layout->addWidget(m_LabelPreviewAlgorithm);
	layout->addLayout(hbox_slice);
	layout->addLayout(hbox_buttons);
	setLayout(layout);

	// connections
	QObject_connect(m_SliderRed, SIGNAL(valueChanged(int)), this, SLOT(SliderRedValueChanged(int)));
	QObject_connect(m_SliderGreen, SIGNAL(valueChanged(int)), this, SLOT(SliderGreenValueChanged(int)));
	QObject_connect(m_SliderBlue, SIGNAL(valueChanged(int)), this, SLOT(SliderBlueValueChanged(int)));

	QObject_connect(m_LabelRedValue, SIGNAL(textEdited(QString)), this, SLOT(LabelRedValueChanged(QString)));
	QObject_connect(m_LabelGreenValue, SIGNAL(textEdited(QString)), this, SLOT(LabelGreenValueChanged(QString)));
	QObject_connect(m_LabelBlueValue, SIGNAL(textEdited(QString)), this, SLOT(LabelBlueValueChanged(QString)));

	QObject_connect(m_SpinSlice, SIGNAL(valueChanged(int)), this, SLOT(SliceValueChanged(int)));

	QObject_connect(m_LoadFile, SIGNAL(clicked()), this, SLOT(LoadPushed()));
	QObject_connect(m_CancelBut, SIGNAL(clicked()), this, SLOT(close()));

	QObject_connect(m_ImageSourceLabel, SIGNAL(NewCenterPreview(QPoint)), this, SLOT(NewCenterPreview(QPoint)));

	m_FirstTime = true;

	RefreshSourceImage();
	ChangePreview();
}

ChannelMixer::~ChannelMixer() {}

void ChannelMixer::SliderRedValueChanged(int value)
{
	m_RedFactor = value;
	m_LabelRedValue->setText(QString::number(value));

	ChangePreview();
}

void ChannelMixer::SliderGreenValueChanged(int value)
{
	m_GreenFactor = value;
	m_LabelGreenValue->setText(QString::number(value));

	ChangePreview();
}

void ChannelMixer::SliderBlueValueChanged(int value)
{
	m_BlueFactor = value;
	m_LabelBlueValue->setText(QString::number(value));

	ChangePreview();
}

void ChannelMixer::LabelRedValueChanged(QString text)
{
	m_RedFactor = text.toInt();
	m_SliderRed->setValue(m_RedFactor);

	ChangePreview();
}

void ChannelMixer::LabelGreenValueChanged(QString text)
{
	m_GreenFactor = text.toInt();
	m_SliderGreen->setValue(m_GreenFactor);

	ChangePreview();
}

void ChannelMixer::LabelBlueValueChanged(QString text)
{
	m_BlueFactor = text.toInt();
	m_SliderBlue->setValue(m_BlueFactor);

	ChangePreview();
}


void ChannelMixer::SliceValueChanged(int value)
{
	m_SelectedSlice = value;

	RefreshSourceImage();
}

void ChannelMixer::NewCenterPreview(QPoint newCenter)
{
	double image_width = m_SourceImage.width();
	double image_height = m_SourceImage.height();

	bool fixed_in_height = true;
	double scaled_factor = 1;
	if (image_height / image_width < m_ScaleY / m_ScaleX)
		fixed_in_height = false;

	int correction_x = 0;
	int correction_y = 0;
	if (fixed_in_height)
	{
		scaled_factor = image_height / m_ScaleY;
		correction_x = (image_width - m_ScaleX * scaled_factor) / 2;
	}
	else
	{
		scaled_factor = image_width / m_ScaleX;
		correction_y = (m_ScaleY * scaled_factor - image_height) / 2;
	}

	m_PreviewCenter.setX(scaled_factor * newCenter.x() + correction_x);
	m_PreviewCenter.setY(image_height -
											 (scaled_factor * newCenter.y() - correction_y));

	RefreshSourceImage();
}

void ChannelMixer::ChangePreview()
{
	int total_factor = m_RedFactor + m_GreenFactor + m_BlueFactor;
	double normalize = total_factor / 100.0;

	if (normalize == 0)
	{
		m_RedFactorPv = 33;
		m_GreenFactorPv = 33;
		m_BlueFactorPv = 33;
	}
	else
	{
		m_RedFactorPv = m_RedFactor / normalize;
		m_GreenFactorPv = m_GreenFactor / normalize;
		m_BlueFactorPv = m_BlueFactor / normalize;
	}

	UpdateText();
	RefreshImage();
}

void ChannelMixer::RefreshSourceImage()
{
	QString file_name = QString::fromUtf8(m_MFilenames[m_SelectedSlice]);
	QImage small_image;
	if (!file_name.isEmpty())
	{
		m_SourceImage = QImage(file_name);
		if (m_SourceImage.isNull())
		{
			QMessageBox::information(this, tr("Image Viewer"), tr("Cannot load %1.").arg(file_name));
			return;
		}

		small_image = m_SourceImage.scaled(m_ScaleX, m_ScaleY, Qt::KeepAspectRatio);

		m_ImageSourceLabel->setPixmap(QPixmap::fromImage(small_image));
		m_ImageSourceLabel->update();
	}

	if (m_FirstTime)
	{
		m_FirstTime = false;

		double image_source_width = m_SourceImage.width();
		double image_source_height = m_SourceImage.height();

		m_PreviewCenter.setX(image_source_width / 2);
		m_PreviewCenter.setY(image_source_height / 2);

		double small_image_width = small_image.width();
		double small_image_height = small_image.height();

		bool fixed_in_height = true;
		double scaled_factor = 1;
		if (image_source_height / image_source_width < m_ScaleY / m_ScaleX)
			fixed_in_height = false;

		if (fixed_in_height)
			scaled_factor = image_source_height / m_ScaleY;
		else
			scaled_factor = image_source_width / m_ScaleX;

		int square_width;
		if (image_source_width > 900 || image_source_height > 900)
			square_width = 300;
		else
		{
			square_width = std::min(image_source_width / 3, image_source_height / 3);
		}

		m_WidthPv = m_HeightPv = square_width;

		m_ImageSourceLabel->SetSquareWidth(square_width / scaled_factor);
		m_ImageSourceLabel->SetSquareHeight(square_width / scaled_factor);

		int small_image_center_x = small_image_width / 2;
		int small_image_center_y = small_image_height / 2;
		int small_image_center_square_half_side = square_width / scaled_factor / 2;
		small_image_center_square_half_side = 0;
		m_ImageSourceLabel->SetCenter(QPoint(small_image_center_x, small_image_center_y + small_image_center_square_half_side));
	}

	RefreshImage();
}

void ChannelMixer::RefreshImage()
{
	QString file_name = QString::fromUtf8(m_MFilenames[m_SelectedSlice]);
	if (!file_name.isEmpty())
	{
		QImage image(file_name);
		if (image.isNull())
		{
			QMessageBox::information(this, tr("Image Viewer"), tr("Cannot load %1.").arg(file_name));
			return;
		}

		QImage converted = ConvertImageTo8BitBMP(image, m_WidthPv, m_HeightPv);
		m_ImageLabel->clear();
		m_ImageLabel->setPixmap(QPixmap::fromImage(converted.scaled(m_ScaleX, m_ScaleY, Qt::KeepAspectRatio)));
		m_ImageLabel->update();
	}
}

QImage ChannelMixer::ConvertImageTo8BitBMP(QImage image, int width, int height)
{
	/* Convert RGB image to grayscale image */
	QImage converted_image(width, height, QImage::Format::Format_Indexed8);

	QVector<QRgb> table(256);
	for (int h = 0; h < 256; ++h)
	{
		table[h] = qRgb(h, h, h);
	}
	converted_image.setColorTable(table);

	int start_x = m_PreviewCenter.x() - (width / 2);
	int start_y = m_SourceImage.height() - (m_PreviewCenter.y() + (height / 2));

	QRect rect(start_x, start_y, width, height);
	QImage cropped = image.copy(rect);

	for (int j = 2; j < height - 2; j++)
	{
		for (int i = 2; i < width - 2; i++)
		{
			QRgb rgb = cropped.pixel(i, j);
			int gray_value = qRed(rgb) * (m_RedFactorPv / 100.00) +
											 qGreen(rgb) * (m_GreenFactorPv / 100.00) +
											 qBlue(rgb) * (m_BlueFactorPv / 100.00);
			converted_image.setPixel(i, j, gray_value);
		}
	}

	return converted_image;
}

void ChannelMixer::UpdateText()
{
	m_LabelPreviewAlgorithm->setText(
			"GrayScale = " +
			QString::number(m_RedFactorPv) + "*R + " +
			QString::number(m_GreenFactorPv) + "*G + " +
			QString::number(m_BlueFactorPv) + "*B");
}

void ChannelMixer::CancelToggled()
{
	m_RedFactorPv = 30;
	m_GreenFactorPv = 59;
	m_BlueFactorPv = 11;

	close();
}

int ChannelMixer::GetRedFactor() const { return m_RedFactorPv; }

int ChannelMixer::GetGreenFactor() const { return m_GreenFactorPv; }

int ChannelMixer::GetBlueFactor() const { return m_BlueFactorPv; }

void ChannelMixer::LoadPushed()
{
	accept();
}

ReloaderBmp2::ReloaderBmp2(SlicesHandler* hand3D, std::vector<const char*> filenames, QWidget* parent, Qt::WindowFlags wFlags)
		: QDialog(parent, wFlags), m_Handler3D(hand3D), m_MFilenames(filenames)
{
	setModal(true);

	m_Subsect = new QCheckBox("Subsection ");
	m_Subsect->setChecked(false);

	m_Xoffset = new QSpinBox;
	m_Xoffset->setRange(0, 2000);
	m_Xoffset->setValue(0);

	m_Yoffset = new QSpinBox;
	m_Yoffset->setRange(0, 2000);
	m_Yoffset->setValue(0);

	m_LoadFile = new QPushButton("Open");
	m_CancelBut = new QPushButton("Cancel");

	// layout
	m_SubsectArea = new QWidget;
	{
		auto hbox = new QHBoxLayout;
		hbox->addWidget(new QLabel("x-Offset: "));
		hbox->addWidget(m_Xoffset);
		hbox->addWidget(new QLabel("y-Offset: "));
		hbox->addWidget(m_Yoffset);
		m_SubsectArea->setLayout(hbox);
	}

	auto hbox_buttons = new QHBoxLayout;
	hbox_buttons->addWidget(m_LoadFile);
	hbox_buttons->addWidget(m_CancelBut);

	auto vbox = new QVBoxLayout;
	vbox->addWidget(m_Subsect);
	vbox->addWidget(m_SubsectArea);
	vbox->addLayout(hbox_buttons);

	SubsectToggled();

	QObject_connect(m_LoadFile, SIGNAL(clicked()), this, SLOT(LoadPushed()));
	QObject_connect(m_CancelBut, SIGNAL(clicked()), this, SLOT(close()));
	QObject_connect(m_Subsect, SIGNAL(clicked()), this, SLOT(SubsectToggled()));
}

ReloaderBmp2::~ReloaderBmp2() {}

void ReloaderBmp2::SubsectToggled()
{
	m_SubsectArea->setVisible(m_Subsect->isChecked());
}

void ReloaderBmp2::LoadPushed()
{
	if (m_Subsect->isChecked())
	{
		Point p;
		p.px = m_Xoffset->value();
		p.py = m_Yoffset->value();
		m_Handler3D->ReloadDIBitmap(m_MFilenames, p);
	}
	else
	{
		m_Handler3D->ReloadDIBitmap(m_MFilenames);
	}
	close();
}

EditText::EditText(QWidget* parent, Qt::WindowFlags wFlags)
		: QDialog(parent, wFlags)
{
	setModal(true);

	m_TextEdit = new QLineEdit("");

	m_SaveBut = new QPushButton("Save");
	m_CancelBut = new QPushButton("Cancel");

	auto hbox1 = new QHBoxLayout;
	hbox1->addWidget(m_TextEdit);

	auto hbox2 = new QHBoxLayout;
	hbox2->addWidget(m_SaveBut);
	hbox2->addWidget(m_CancelBut);

	auto layout = new QVBoxLayout;
	layout->addLayout(hbox1);
	layout->addLayout(hbox2);
	setLayout(layout);

	QObject_connect(m_SaveBut, SIGNAL(clicked()), this, SLOT(accept()));
	QObject_connect(m_CancelBut, SIGNAL(clicked()), this, SLOT(reject()));
}

EditText::~EditText() {}

void EditText::SetEditableText(QString editable_text)
{
	m_TextEdit->setText(editable_text);
}

QString EditText::GetEditableText() { return m_TextEdit->text(); }

SupportedMultiDatasetTypes::SupportedMultiDatasetTypes(QWidget* parent, Qt::WindowFlags wFlags)
		: QDialog(parent, wFlags)
{
	setModal(true);

	// widgets
	auto radio_group_box = new QGroupBox("Supported types");
	for (int i = 0; i < eSupportedTypes::keSupportedTypesSize; i++)
	{
		m_RadioButs.push_back(new QRadioButton(ToQString(eSupportedTypes(i))));
	}

	m_SelectBut = new QPushButton("Select");
	m_CancelBut = new QPushButton("Cancel");

	// layout
	auto radio_layout = new QVBoxLayout;
	for (auto rb : m_RadioButs)
	{
		radio_layout->addWidget(rb);
	}
	radio_group_box->setLayout(radio_layout);

	auto buttons_layout = new QHBoxLayout;
	buttons_layout->addWidget(m_SelectBut);
	buttons_layout->addWidget(m_CancelBut);

	auto vbox_overall = new QVBoxLayout;
	vbox_overall->addWidget(radio_group_box);
	vbox_overall->addLayout(buttons_layout);

	setLayout(vbox_overall);

	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

	// connections
	QObject_connect(m_SelectBut, SIGNAL(clicked()), this, SLOT(accept()));
	QObject_connect(m_CancelBut, SIGNAL(clicked()), this, SLOT(reject()));
}

int SupportedMultiDatasetTypes::GetSelectedType()
{
	for (int i = 0; i < m_RadioButs.size(); i++)
	{
		if (m_RadioButs.at(i)->isChecked())
		{
			return i;
		}
	}
	return -1;
}

} // namespace iseg
