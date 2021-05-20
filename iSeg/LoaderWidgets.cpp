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

#include <q3hbox.h>
#include <q3vbox.h>
#include <qbuttongroup.h>
#include <qfiledialog.h>
#include <qgridlayout.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qmessagebox.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qstring.h>

#include <QMouseEvent>
#include <QPainter>

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

ExportImg::ExportImg(SlicesHandler* h, QWidget* p, const char* n, Qt::WindowFlags f)
		: QDialog(p, n, f), m_Handler3D(h)
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

LoaderDicom::LoaderDicom(SlicesHandler* hand3D, QStringList* lname, bool breload, QWidget* parent, const char* name, Qt::WindowFlags wFlags)
		: QDialog(parent, name, TRUE, wFlags), m_Handler3D(hand3D), m_Reload(breload), m_Lnames(lname)
{
	m_Vbox1 = new Q3VBox(this);
	m_Hbox1 = new Q3HBox(m_Vbox1);
	m_CbSubsect = new QCheckBox(QString("Subsection "), m_Hbox1);
	m_CbSubsect->setChecked(false);
	m_CbSubsect->show();
	m_Hbox2 = new Q3HBox(m_Hbox1);

	m_Vbox2 = new Q3VBox(m_Hbox2);
	m_Vbox3 = new Q3VBox(m_Hbox2);
	m_Xoffs = new QLabel(QString("x-Offset: "), m_Vbox2);
	m_Xoffset = new QSpinBox(0, 2000, 1, m_Vbox3);
	m_Xoffset->setValue(0);
	m_Xoffset->show();
	m_Yoffs = new QLabel(QString("y-Offset: "), m_Vbox2);
	m_Yoffset = new QSpinBox(0, 2000, 1, m_Vbox3);
	m_Yoffset->show();
	m_Xoffset->setValue(0);
	m_Vbox2->show();
	m_Vbox3->show();

	m_Vbox4 = new Q3VBox(m_Hbox2);
	m_Vbox5 = new Q3VBox(m_Hbox2);
	m_Xl = new QLabel(QString("x-Length: "), m_Vbox4);
	m_Xlength = new QSpinBox(0, 2000, 1, m_Vbox5);
	m_Xlength->show();
	m_Xlength->setValue(512);
	m_Yl = new QLabel(QString("y-Length: "), m_Vbox4);
	m_Ylength = new QSpinBox(0, 2000, 1, m_Vbox5);
	m_Ylength->show();
	m_Ylength->setValue(512);
	m_Vbox4->show();
	m_Vbox5->show();

	m_Hbox2->show();
	m_Hbox1->show();

	m_Hbox3 = new Q3HBox(m_Vbox1);
	m_CbCt = new QCheckBox(QString("CT weight "), m_Hbox3);
	m_CbCt->setChecked(false);
	m_CbCt->show();
	m_Vbox6 = new Q3VBox(m_Hbox3);
	m_Hbox4 = new Q3HBox(m_Vbox6);
	m_BgWeight = new QButtonGroup(this);
	//	bg_weight->hide();
	m_RbBone = new QRadioButton(QString("Bone"), m_Hbox4);
	m_RbMuscle = new QRadioButton(QString("Muscle"), m_Hbox4);
	m_BgWeight->insert(m_RbBone);
	m_BgWeight->insert(m_RbMuscle);
	m_RbMuscle->setChecked(TRUE);
	m_RbMuscle->show();
	m_RbBone->show();
	m_Hbox4->show();
	m_CbCrop = new QCheckBox(QString("crop "), m_Vbox6);
	m_CbCrop->setChecked(true);
	m_CbCrop->show();
	m_Vbox6->show();
	m_Hbox3->show();
	//	hbox3->hide();

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
			m_Hbox6 = new Q3HBox(m_Vbox1);

			m_LbTitle = new QLabel(QString("Series-Nr.: "), m_Hbox6);
			m_Seriesnrselection = new QComboBox(m_Hbox6);
			for (unsigned i = 0; i < (unsigned)m_Dicomseriesnr.size(); i++)
			{
				QString str;
				m_Seriesnrselection->insertItem(str = str.setNum((int)m_Dicomseriesnr.at(i)));
			}
			m_Seriesnrselection->setCurrentItem(0);
			m_Hbox6->show();
		}
	}

	m_Hbox5 = new Q3HBox(m_Vbox1);
	m_LoadFile = new QPushButton("Open", m_Hbox5);
	m_CancelBut = new QPushButton("Cancel", m_Hbox5);
	m_Hbox5->show();
	m_Vbox1->show();

	/*	vbox1->setSizePolicy(QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed));
	vbox2->setSizePolicy(QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed));
	hbox1->setSizePolicy(QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed));
	hbox2->setSizePolicy(QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed));
	hbox3->setSizePolicy(QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed));
	hbox4->setSizePolicy(QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed));
	hbox5->setSizePolicy(QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed));*/
	setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
	m_Vbox1->setFixedSize(m_Vbox1->sizeHint());

	SubsectToggled();
	CtToggled();

	QObject_connect(m_LoadFile, SIGNAL(clicked()), this, SLOT(LoadPushed()));
	QObject_connect(m_CancelBut, SIGNAL(clicked()), this, SLOT(close()));
	QObject_connect(m_CbSubsect, SIGNAL(clicked()), this, SLOT(SubsectToggled()));
	QObject_connect(m_CbCt, SIGNAL(clicked()), this, SLOT(CtToggled()));
}

LoaderDicom::~LoaderDicom()
{
	delete m_Vbox1;
	delete m_BgWeight;
}

void LoaderDicom::SubsectToggled()
{
	if (m_CbSubsect->isChecked())
	{
		m_Hbox2->show();
	}
	else
	{
		m_Hbox2->hide();
	}
}

void LoaderDicom::CtToggled()
{
	if (m_CbCt->isChecked())
	{
		m_Vbox6->show();
	}
	else
	{
		m_Vbox6->hide();
	}
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
				if (m_Dicomseriesnrlist[pos++] == m_Dicomseriesnr[m_Seriesnrselection->currentItem()])
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

LoaderRaw::LoaderRaw(SlicesHandler* hand3D, QWidget* parent, const char* name, Qt::WindowFlags wFlags)
		: QDialog(parent, name, TRUE, wFlags), m_Handler3D(hand3D)
{
	m_Vbox1 = new Q3VBox(this);
	m_Hbox1 = new Q3HBox(m_Vbox1);
	m_FileName = new QLabel(QString("File Name: "), m_Hbox1);
	m_NameEdit = new QLineEdit(m_Hbox1);
	m_NameEdit->show();
	m_SelectFile = new QPushButton("Select", m_Hbox1);
	m_SelectFile->show();
	m_Hbox1->show();
	m_Hbox6 = new Q3HBox(m_Vbox1);
	m_Xl1 = new QLabel(QString("Total x-Length: "), m_Hbox6);
	m_Xlength1 = new QSpinBox(0, 9999, 1, m_Hbox6);
	m_Xlength1->show();
	m_Xlength1->setValue(512);
	m_Yl1 = new QLabel(QString("Total y-Length: "), m_Hbox6);
	m_Ylength1 = new QSpinBox(0, 9999, 1, m_Hbox6);
	m_Ylength1->show();
	m_Ylength1->setValue(512);
	/*	nrslice = new QLabel(QString("Slice Nr.: "),hbox6);
	slicenrbox=new QSpinBox(0,200,1,hbox6);
	slicenrbox->show();
	slicenrbox->setValue(0);*/
	m_Hbox6->show();
	m_Hbox8 = new Q3HBox(m_Vbox1);
	m_Nrslice = new QLabel(QString("Start Nr.: "), m_Hbox8);
	m_Slicenrbox = new QSpinBox(0, 9999, 1, m_Hbox8);
	m_Slicenrbox->show();
	m_Slicenrbox->setValue(0);
	m_LbNrslices = new QLabel("#Slices: ", m_Hbox8);
	m_SbNrslices = new QSpinBox(1, 9999, 1, m_Hbox8);
	m_SbNrslices->show();
	m_SbNrslices->setValue(10);
	m_Hbox8->show();
	m_Hbox2 = new Q3HBox(m_Vbox1);
	m_Subsect = new QCheckBox(QString("Subsection "), m_Hbox2);
	m_Subsect->setChecked(false);
	m_Subsect->show();
	m_Vbox2 = new Q3VBox(m_Hbox2);
	m_Hbox3 = new Q3HBox(m_Vbox2);
	m_Xoffs = new QLabel(QString("x-Offset: "), m_Hbox3);
	m_Xoffset = new QSpinBox(0, 2000, 1, m_Hbox3);
	m_Xoffset->setValue(0);
	m_Xoffset->show();
	m_Yoffs = new QLabel(QString("y-Offset: "), m_Hbox3);
	m_Yoffset = new QSpinBox(0, 2000, 1, m_Hbox3);
	m_Yoffset->show();
	m_Xoffset->setValue(0);
	m_Hbox3->show();
	m_Hbox4 = new Q3HBox(m_Vbox2);
	m_Xl = new QLabel(QString("x-Length: "), m_Hbox4);
	m_Xlength = new QSpinBox(0, 2000, 1, m_Hbox4);
	m_Xlength->show();
	m_Xlength->setValue(256);
	m_Yl = new QLabel(QString("y-Length: "), m_Hbox4);
	m_Ylength = new QSpinBox(0, 2000, 1, m_Hbox4);
	m_Ylength->show();
	m_Ylength->setValue(256);
	m_Hbox4->show();
	m_Vbox2->show();
	m_Hbox2->show();
	m_Hbox7 = new Q3HBox(m_Vbox1);
	m_Bitselect = new QButtonGroup(this);
	//	bitselect->hide();
	/*	bit8=new QRadioButton(QString("8-bit"),bitselect);
	bit16=new QRadioButton(QString("16-bit"),bitselect);*/
	m_Bit8 = new QRadioButton(QString("8-bit"), m_Hbox7);
	m_Bit16 = new QRadioButton(QString("16-bit"), m_Hbox7);
	m_Bitselect->insert(m_Bit8);
	m_Bitselect->insert(m_Bit16);
	m_Bit16->show();
	m_Bit8->setChecked(TRUE);
	m_Bit8->show();
	m_Hbox7->show();
	/*	bitselect->insert(bit8);
	bitselect->insert(bit16);*/

	m_Hbox5 = new Q3HBox(m_Vbox1);
	m_LoadFile = new QPushButton("Open", m_Hbox5);
	m_CancelBut = new QPushButton("Cancel", m_Hbox5);
	m_Hbox5->show();

	/*	vbox1->setSizePolicy(QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed));
	vbox2->setSizePolicy(QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed));
	hbox1->setSizePolicy(QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed));
	hbox2->setSizePolicy(QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed));
	hbox3->setSizePolicy(QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed));
	hbox4->setSizePolicy(QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed));
	hbox5->setSizePolicy(QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed));*/
	m_Vbox1->show();
	setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));

	m_Vbox1->setFixedSize(m_Vbox1->sizeHint());

	QObject_connect(m_SelectFile, SIGNAL(clicked()), this, SLOT(SelectPushed()));
	QObject_connect(m_LoadFile, SIGNAL(clicked()), this, SLOT(LoadPushed()));
	QObject_connect(m_CancelBut, SIGNAL(clicked()), this, SLOT(close()));
	//	QObject_connect(subsect,SIGNAL(toggled(bool on)),this,SLOT(SubsectToggled(bool on)));
	QObject_connect(m_Subsect, SIGNAL(clicked()), this, SLOT(SubsectToggled()));
	//	QObject_connect(subsect,SIGNAL(toggled(bool on)),vbox2,SLOT(setShown(bool on)));

	SubsectToggled();
}

LoaderRaw::~LoaderRaw()
{
	delete m_Vbox1;
	delete m_Bitselect;
}

QString LoaderRaw::GetFileName() const { return m_NameEdit->text(); }

std::array<unsigned int, 2> LoaderRaw::GetDimensions() const
{
	return {
			static_cast<unsigned>(m_Xlength1->value()), static_cast<unsigned>(m_Ylength1->value())};
}

std::array<unsigned int, 3> LoaderRaw::GetSubregionStart() const
{
	return {
			static_cast<unsigned>(m_Xoffset->value()), static_cast<unsigned>(m_Yoffset->value()), static_cast<unsigned>(m_Slicenrbox->value())};
}

std::array<unsigned int, 3> LoaderRaw::GetSubregionSize() const
{
	return {
			static_cast<unsigned>(m_Subsect->isChecked() ? m_Xlength->value() : m_Xlength1->value()), static_cast<unsigned>(m_Subsect->isChecked() ? m_Ylength->value() : m_Ylength1->value()), static_cast<unsigned>(m_SbNrslices->value())};
}

int LoaderRaw::GetBits() const
{
	return (m_Bit8->isChecked() ? 8 : 16);
}

void LoaderRaw::SubsectToggled()
{
	bool isset = m_Subsect->isChecked();
	;
	if (isset)
	{
		m_Vbox2->show();
		//		nameEdit->setText(QString("ABC"));
	}
	else
	{
		m_Vbox2->hide();
		//		nameEdit->setText(QString("DEF"));
	}

	//	vbox1->setFixedSize(vbox1->sizeHint());
}

void LoaderRaw::LoadPushed()
{
	unsigned bitdepth;
	if (m_Bit8->isChecked())
		bitdepth = 8;
	else
		bitdepth = 16;
	if (!(m_NameEdit->text()).isEmpty())
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
			m_Handler3D->ReadRaw(m_NameEdit->text().ascii(), (unsigned short)m_Xlength1->value(), (unsigned short)m_Ylength1->value(), (unsigned)bitdepth, (unsigned short)m_Slicenrbox->value(), (unsigned short)m_SbNrslices->value(), p, (unsigned short)m_Xlength->value(), (unsigned short)m_Ylength->value());
		}
		else
		{
			m_Handler3D->ReadRaw(m_NameEdit->text().ascii(), (unsigned short)m_Xlength1->value(), (unsigned short)m_Ylength1->value(), (unsigned)bitdepth, (unsigned short)m_Slicenrbox->value(), (unsigned short)m_SbNrslices->value());
		}
		close();
		return;
	}
}

void LoaderRaw::SelectPushed()
{
	QString loadfilename = RecentPlaces::GetOpenFileName(this, "Open file", QString::null, QString::null);
	m_NameEdit->setText(loadfilename);
}

ReloaderRaw::ReloaderRaw(SlicesHandler* hand3D, QWidget* parent, const char* name, Qt::WindowFlags wFlags)
		: QDialog(parent, name, TRUE, wFlags)
{
	m_Handler3D = hand3D;

	m_Vbox1 = new Q3VBox(this);
	m_Hbox1 = new Q3HBox(m_Vbox1);
	m_FileName = new QLabel(QString("File Name: "), m_Hbox1);
	m_NameEdit = new QLineEdit(m_Hbox1);
	m_NameEdit->show();
	m_SelectFile = new QPushButton("Select", m_Hbox1);
	m_SelectFile->show();
	m_Hbox1->show();
	m_Hbox2 = new Q3HBox(m_Vbox1);
	m_Bitselect = new QButtonGroup(this);
	//	bitselect->hide();
	m_Bit8 = new QRadioButton(QString("8-bit"), m_Hbox2);
	m_Bit16 = new QRadioButton(QString("16-bit"), m_Hbox2);
	m_Bitselect->insert(m_Bit8);
	m_Bitselect->insert(m_Bit16);
	m_Bit16->show();
	m_Bit8->setChecked(TRUE);
	m_Bit8->show();
	m_Nrslice = new QLabel(QString("Slice Nr.: "), m_Hbox2);
	m_Slicenrbox = new QSpinBox(0, 200, 1, m_Hbox2);
	m_Slicenrbox->show();
	m_Slicenrbox->setValue(0);
	m_Hbox2->show();
	m_Hbox5 = new Q3HBox(m_Vbox1);
	m_Subsect = new QCheckBox(QString("Subsection "), m_Hbox5);
	m_Subsect->setChecked(false);
	m_Subsect->show();
	m_Vbox2 = new Q3VBox(m_Hbox5);
	m_Hbox4 = new Q3HBox(m_Vbox2);
	m_Xl1 = new QLabel(QString("Total x-Length: "), m_Hbox4);
	m_Xlength1 = new QSpinBox(0, 2000, 1, m_Hbox4);
	m_Xlength1->show();
	m_Xlength1->setValue(512);
	m_Yl1 = new QLabel(QString("Total  y-Length: "), m_Hbox4);
	m_Ylength1 = new QSpinBox(0, 2000, 1, m_Hbox4);
	m_Ylength1->show();
	m_Ylength1->setValue(512);
	m_Hbox4->show();
	m_Hbox3 = new Q3HBox(m_Vbox2);
	m_Xoffs = new QLabel(QString("x-Offset: "), m_Hbox3);
	m_Xoffset = new QSpinBox(0, 2000, 1, m_Hbox3);
	m_Xoffset->setValue(0);
	m_Xoffset->show();
	m_Yoffs = new QLabel(QString("y-Offset: "), m_Hbox3);
	m_Yoffset = new QSpinBox(0, 2000, 1, m_Hbox3);
	m_Yoffset->show();
	m_Xoffset->setValue(0);
	m_Hbox3->show();
	m_Vbox2->show();
	m_Hbox5->show();

	/*	bitselect->insert(bit8);
	bitselect->insert(bit16);*/

	/*	hbox7=new QHBox(vbox1);
	lb_startnr = new QLabel(QString("Start Nr.: "),hbox7);
	sb_startnr=new QSpinBox(0,2000,1,hbox7);
	sb_startnr->setValue(0);
	sb_startnr->show();
	hbox7->show();*/

	m_Hbox6 = new Q3HBox(m_Vbox1);
	m_LoadFile = new QPushButton("Open", m_Hbox6);
	m_CancelBut = new QPushButton("Cancel", m_Hbox6);
	m_Hbox6->show();

	/*	vbox1->setSizePolicy(QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed));
	vbox2->setSizePolicy(QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed));
	hbox1->setSizePolicy(QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed));
	hbox2->setSizePolicy(QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed));
	hbox3->setSizePolicy(QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed));
	hbox4->setSizePolicy(QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed));
	hbox5->setSizePolicy(QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed));*/
	m_Vbox1->show();
	setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));

	m_Vbox1->setFixedSize(m_Vbox1->sizeHint());

	SubsectToggled();

	QObject_connect(m_SelectFile, SIGNAL(clicked()), this, SLOT(SelectPushed()));
	QObject_connect(m_LoadFile, SIGNAL(clicked()), this, SLOT(LoadPushed()));
	QObject_connect(m_CancelBut, SIGNAL(clicked()), this, SLOT(close()));
	QObject_connect(m_Subsect, SIGNAL(clicked()), this, SLOT(SubsectToggled()));
}

ReloaderRaw::~ReloaderRaw()
{
	delete m_Vbox1;
	delete m_Bitselect;
}

void ReloaderRaw::SubsectToggled()
{
	bool isset = m_Subsect->isChecked();
	;
	if (isset)
	{
		m_Vbox2->show();
		//		nameEdit->setText(QString("ABC"));
	}
	else
	{
		m_Vbox2->hide();
		//		nameEdit->setText(QString("DEF"));
	}

	//	vbox1->setFixedSize(vbox1->sizeHint());
}

void ReloaderRaw::LoadPushed()
{
	unsigned bitdepth;
	if (m_Bit8->isChecked())
		bitdepth = 8;
	else
		bitdepth = 16;

	if (!(m_NameEdit->text()).isEmpty())
	{
		if (m_Subsect->isChecked())
		{
			Point p;
			p.px = m_Xoffset->value();
			p.py = m_Yoffset->value();
			m_Handler3D->ReloadRaw(m_NameEdit->text().ascii(), m_Xlength1->value(), m_Ylength1->value(), bitdepth, m_Slicenrbox->value(), p);
		}
		else
		{
			m_Handler3D->ReloadRaw(m_NameEdit->text().ascii(), bitdepth, m_Slicenrbox->value());
		}
		close();
	}
}

void ReloaderRaw::SelectPushed()
{
	// null filter?
	QString loadfilename = RecentPlaces::GetOpenFileName(this, QString::null, QString::null, QString::null);
	m_NameEdit->setText(loadfilename);
}

NewImg::NewImg(SlicesHandler* hand3D, QWidget* parent, const char* name, Qt::WindowFlags wFlags)
		: QDialog(parent, name, TRUE, wFlags)
{
	m_Handler3D = hand3D;

	m_Vbox1 = new Q3VBox(this);
	m_Hbox2 = new Q3HBox(m_Vbox1);
	m_Xl = new QLabel(QString("Total x-Length: "), m_Hbox2);
	m_Xlength = new QSpinBox(1, 2000, 1, m_Hbox2);
	m_Xlength->show();
	m_Xlength->setValue(512);
	m_Yl = new QLabel(QString("Total  y-Length: "), m_Hbox2);
	m_Ylength = new QSpinBox(1, 2000, 1, m_Hbox2);
	m_Ylength->show();
	m_Ylength->setValue(512);
	m_Hbox2->show();
	m_Hbox1 = new Q3HBox(m_Vbox1);
	m_LbNrslices = new QLabel(QString("# Slices: "), m_Hbox1);
	m_SbNrslices = new QSpinBox(1, 2000, 1, m_Hbox1);
	m_SbNrslices->show();
	m_SbNrslices->setValue(10);
	m_Hbox1->show();
	m_Hbox3 = new Q3HBox(m_Vbox1);
	m_NewFile = new QPushButton("New", m_Hbox3);
	m_CancelBut = new QPushButton("Cancel", m_Hbox3);
	m_Hbox3->show();

	m_Vbox1->show();
	setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));

	m_Vbox1->setFixedSize(m_Vbox1->sizeHint());

	QObject_connect(m_NewFile, SIGNAL(clicked()), this, SLOT(NewPushed()));
	QObject_connect(m_CancelBut, SIGNAL(clicked()), this, SLOT(OnClose()));

	m_NewPressed = false;
}

NewImg::~NewImg() { delete m_Vbox1; }

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

LoaderColorImages::LoaderColorImages(SlicesHandler* hand3D, eImageType typ, std::vector<const char*> filenames, QWidget* parent, const char* name, Qt::WindowFlags wFlags)
		: QDialog(parent, name, TRUE, wFlags), m_Handler3D(hand3D), m_Type(typ), m_MFilenames(filenames)
{
	m_MapToLut = new QCheckBox(QString("Map colors to lookup table"));
	m_MapToLut->setChecked(true);
	if (typ == LoaderColorImages::kTIF)
	{
		m_MapToLut->setEnabled(false);
	}

	m_Subsect = new QCheckBox(QString("Subsection"));
	m_Subsect->setChecked(false);

	auto xoffs = new QLabel(QString("x-Offset: "));
	m_Xoffset = new QSpinBox(0, 2000, 1, nullptr);
	m_Xoffset->setValue(0);

	auto yoffs = new QLabel(QString("y-Offset: "));
	m_Yoffset = new QSpinBox(0, 2000, 1, nullptr);
	m_Xoffset->setValue(0);

	auto xl = new QLabel(QString("x-Length: "));
	m_Xlength = new QSpinBox(0, 2000, 1, nullptr);
	m_Xlength->setValue(256);

	auto yl = new QLabel(QString("y-Length: "));
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
		m_Handler3D->SetRgbFactors(33, 33, 33);
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

ChannelMixer::ChannelMixer(std::vector<const char*> filenames, QWidget* parent, const char* name, Qt::WindowFlags wFlags)
		: QDialog(parent, name, TRUE, wFlags), m_MFilenames(filenames)
{
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

	m_RedFactorPv = 30;
	m_GreenFactorPv = 59;
	m_BlueFactorPv = 11;

	m_RedFactor = 30;
	m_GreenFactor = 59;
	m_BlueFactor = 11;

	m_ScaleX = 400;
	m_ScaleY = 500;

	m_VboxMain = new Q3VBox(this);

	m_HboxImageAndControl = new Q3HBox(m_VboxMain);

	QSize standard_box_size;
	standard_box_size.setWidth(m_ScaleX);
	standard_box_size.setHeight(m_ScaleY);

	m_HboxImageSource = new Q3HBox(m_HboxImageAndControl);
	m_HboxImageSource->setFixedSize(standard_box_size);
	m_HboxImageSource->show();
	m_ImageSourceLabel = new ClickableLabel(m_HboxImageSource);
	m_ImageSourceLabel->setFixedSize(standard_box_size);
	m_ImageSourceLabel->SetSquareWidth(25);
	m_ImageSourceLabel->SetSquareHeight(25);
	m_ImageSourceLabel->setAlignment(Qt::AlignCenter);

	m_HboxImage = new Q3HBox(m_HboxImageAndControl);
	m_HboxImage->setFixedSize(standard_box_size);
	m_HboxImage->show();
	m_ImageLabel = new QLabel(m_HboxImage);
	m_ImageLabel->setFixedSize(standard_box_size);

	m_HboxControl = new Q3VBox(m_HboxImageAndControl);

	QSize control_size;
	control_size.setHeight(m_ScaleY);
	control_size.setWidth(m_ScaleX / 2);
	m_HboxControl->setFixedSize(control_size);

	m_HboxChannelOptions = new Q3VBox(m_HboxControl);

	m_VboxRed = new Q3HBox(m_HboxChannelOptions);
	m_VboxGreen = new Q3HBox(m_HboxChannelOptions);
	m_VboxBlue = new Q3HBox(m_HboxChannelOptions);
	m_LabelPreviewAlgorithm = new QLabel(m_HboxChannelOptions);
	m_VboxSlice = new Q3HBox(m_HboxChannelOptions);
	m_HboxButtons = new Q3HBox(m_HboxChannelOptions);

	m_LabelRed = new QLabel(m_VboxRed);
	m_LabelRed->setText("Red");
	m_LabelRed->setFixedWidth(40);
	m_SliderRed = new QSlider(Qt::Horizontal, m_VboxRed);
	m_SliderRed->setMinValue(0);
	m_SliderRed->setMaxValue(100);
	m_SliderRed->setValue(m_RedFactor);
	m_SliderRed->setFixedWidth(80);
	m_LabelRedValue = new QLineEdit(m_VboxRed);
	m_LabelRedValue->setText(QString::number(m_SliderRed->value()));
	m_LabelRedValue->setFixedWidth(30);
	QLabel* label_pure_red = new QLabel(m_VboxRed);
	label_pure_red->setText(" Pure");
	label_pure_red->setFixedWidth(30);
	m_ButtonRed = new QRadioButton(m_VboxRed);
	m_ButtonRed->setChecked(false);

	m_LabelGreen = new QLabel(m_VboxGreen);
	m_LabelGreen->setText("Green");
	m_LabelGreen->setFixedWidth(40);
	m_SliderGreen = new QSlider(Qt::Horizontal, m_VboxGreen);
	m_SliderGreen->setMinValue(0);
	m_SliderGreen->setMaxValue(100);
	m_SliderGreen->setValue(m_GreenFactor);
	m_SliderGreen->setFixedWidth(80);
	m_LabelGreenValue = new QLineEdit(m_VboxGreen);
	m_LabelGreenValue->setText(QString::number(m_SliderGreen->value()));
	m_LabelGreenValue->setFixedWidth(30);
	QLabel* label_pure_green = new QLabel(m_VboxGreen);
	label_pure_green->setText(" Pure");
	label_pure_green->setFixedWidth(30);
	m_ButtonGreen = new QRadioButton(m_VboxGreen);
	m_ButtonGreen->setChecked(false);

	m_LabelBlue = new QLabel(m_VboxBlue);
	m_LabelBlue->setText("Blue");
	m_LabelBlue->setFixedWidth(40);
	m_SliderBlue = new QSlider(Qt::Horizontal, m_VboxBlue);
	m_SliderBlue->setMinValue(0);
	m_SliderBlue->setMaxValue(100);
	m_SliderBlue->setValue(m_BlueFactor);
	m_SliderBlue->setFixedWidth(80);
	m_LabelBlueValue = new QLineEdit(m_VboxBlue);
	m_LabelBlueValue->setText(QString::number(m_SliderBlue->value()));
	m_LabelBlueValue->setFixedWidth(30);
	QLabel* label_pure_blue = new QLabel(m_VboxBlue);
	label_pure_blue->setText(" Pure");
	label_pure_blue->setFixedWidth(30);
	m_ButtonBlue = new QRadioButton(m_VboxBlue);
	m_ButtonBlue->setChecked(false);

	m_LabelSliceValue = new QLabel(m_VboxSlice);
	m_LabelSliceValue->setText("Slice");
	m_LabelSliceValue->setFixedWidth(40);
	m_SpinSlice = new QSpinBox(m_VboxSlice);
	m_SpinSlice->setMinimum(0);
	m_SpinSlice->setMaximum(static_cast<int>(filenames.size()) - 1);
	m_SpinSlice->setValue(0);
	m_SelectedSlice = m_SpinSlice->value();

	m_LoadFile = new QPushButton("Open", m_HboxButtons);
	m_CancelBut = new QPushButton("Cancel", m_HboxButtons);

	m_HboxControl->show();
	m_HboxButtons->show();
	m_VboxMain->show();

	setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
	m_VboxMain->setFixedSize(m_VboxMain->sizeHint());

	QObject_connect(m_SliderRed, SIGNAL(valueChanged(int)), this, SLOT(SliderRedValueChanged(int)));
	QObject_connect(m_SliderGreen, SIGNAL(valueChanged(int)), this, SLOT(SliderGreenValueChanged(int)));
	QObject_connect(m_SliderBlue, SIGNAL(valueChanged(int)), this, SLOT(SliderBlueValueChanged(int)));

	QObject_connect(m_LabelRedValue, SIGNAL(textEdited(QString)), this, SLOT(LabelRedValueChanged(QString)));
	QObject_connect(m_LabelGreenValue, SIGNAL(textEdited(QString)), this, SLOT(LabelGreenValueChanged(QString)));
	QObject_connect(m_LabelBlueValue, SIGNAL(textEdited(QString)), this, SLOT(LabelBlueValueChanged(QString)));

	QObject_connect(m_ButtonRed, SIGNAL(toggled(bool)), this, SLOT(ButtonRedPushed(bool)));
	QObject_connect(m_ButtonGreen, SIGNAL(toggled(bool)), this, SLOT(ButtonGreenPushed(bool)));
	QObject_connect(m_ButtonBlue, SIGNAL(toggled(bool)), this, SLOT(ButtonBluePushed(bool)));

	QObject_connect(m_SpinSlice, SIGNAL(valueChanged(int)), this, SLOT(SliceValueChanged(int)));

	QObject_connect(m_LoadFile, SIGNAL(clicked()), this, SLOT(LoadPushed()));
	QObject_connect(m_CancelBut, SIGNAL(clicked()), this, SLOT(close()));

	QObject_connect(m_ImageSourceLabel, SIGNAL(newCenterPreview(QPoint)), this, SLOT(NewCenterPreview(QPoint)));

	m_FirstTime = true;

	RefreshSourceImage();
	ChangePreview();
}

ChannelMixer::~ChannelMixer() { delete m_VboxMain; }

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

void ChannelMixer::ButtonRedPushed(bool checked)
{
	if (checked)
	{
		m_SliderRed->setValue(100);
		m_SliderGreen->setValue(0);
		m_SliderBlue->setValue(0);

		m_ButtonGreen->setChecked(false);
		m_ButtonBlue->setChecked(false);
	}
}

void ChannelMixer::ButtonGreenPushed(bool checked)
{
	if (checked)
	{
		m_SliderRed->setValue(0);
		m_SliderGreen->setValue(100);
		m_SliderBlue->setValue(0);

		m_ButtonRed->setChecked(false);
		m_ButtonBlue->setChecked(false);
	}
}
void ChannelMixer::ButtonBluePushed(bool checked)
{
	if (checked)
	{
		m_SliderRed->setValue(0);
		m_SliderGreen->setValue(0);
		m_SliderBlue->setValue(100);

		m_ButtonRed->setChecked(false);
		m_ButtonGreen->setChecked(false);
	}
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
	//RefreshImage();
}

void ChannelMixer::ChangePreview()
{
	if ((m_RedFactor != 0) + (m_GreenFactor != 0) + (m_BlueFactor != 0) > 1)
	{
		m_ButtonRed->setChecked(false);
		m_ButtonGreen->setChecked(false);
		m_ButtonBlue->setChecked(false);
	}

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
	m_HboxImageSource->update();

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
	m_HboxImage->update();
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
	m_LabelPreviewAlgorithm->setText("GrayScale = " + QString::number(m_RedFactorPv) + "*R + " +
																	 QString::number(m_GreenFactorPv) + "*G + " +
																	 QString::number(m_BlueFactorPv) + "*B");
}

void ChannelMixer::CancelToggled()
{
	m_RedFactorPv = 30;
	m_GreenFactorPv = 59;
	m_BlueFactorPv = 11;

	m_VboxMain->hide();
}

int ChannelMixer::GetRedFactor() const { return m_RedFactorPv; }

int ChannelMixer::GetGreenFactor() const { return m_GreenFactorPv; }

int ChannelMixer::GetBlueFactor() const { return m_BlueFactorPv; }

void ChannelMixer::LoadPushed()
{
	close();
}

ReloaderBmp2::ReloaderBmp2(SlicesHandler* hand3D, std::vector<const char*> filenames, QWidget* parent, const char* name, Qt::WindowFlags wFlags)
		: QDialog(parent, name, TRUE, wFlags)
{
	m_Handler3D = hand3D;
	m_MFilenames = filenames;

	m_Vbox1 = new Q3VBox(this);
	m_Hbox2 = new Q3HBox(m_Vbox1);
	m_Subsect = new QCheckBox(QString("Subsection "), m_Hbox2);
	m_Subsect->setChecked(false);
	m_Subsect->show();
	m_Xoffs = new QLabel(QString("x-Offset: "), m_Hbox2);
	m_Xoffset = new QSpinBox(0, 2000, 1, m_Hbox2);
	m_Xoffset->setValue(0);
	m_Xoffset->show();
	m_Yoffs = new QLabel(QString("y-Offset: "), m_Hbox2);
	m_Yoffset = new QSpinBox(0, 2000, 1, m_Hbox2);
	m_Yoffset->show();
	m_Xoffset->setValue(0);
	m_Hbox2->show();
	m_Hbox3 = new Q3HBox(m_Vbox1);
	m_LoadFile = new QPushButton("Open", m_Hbox3);
	m_CancelBut = new QPushButton("Cancel", m_Hbox3);
	m_Hbox3->show();

	/*	vbox1->setSizePolicy(QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed));
	vbox2->setSizePolicy(QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed));
	hbox1->setSizePolicy(QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed));
	hbox2->setSizePolicy(QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed));
	hbox3->setSizePolicy(QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed));
	hbox4->setSizePolicy(QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed));
	hbox5->setSizePolicy(QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed));*/
	m_Vbox1->show();
	setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));

	m_Vbox1->setFixedSize(m_Vbox1->sizeHint());

	SubsectToggled();

	QObject_connect(m_LoadFile, SIGNAL(clicked()), this, SLOT(LoadPushed()));
	QObject_connect(m_CancelBut, SIGNAL(clicked()), this, SLOT(close()));
	QObject_connect(m_Subsect, SIGNAL(clicked()), this, SLOT(SubsectToggled()));
}

ReloaderBmp2::~ReloaderBmp2() { delete m_Vbox1; }

void ReloaderBmp2::SubsectToggled()
{
	bool isset = m_Subsect->isChecked();
	;
	if (isset)
	{
		m_Xoffset->show();
		m_Yoffs->show();
		m_Yoffset->show();
		m_Yoffs->show();
	}
	else
	{
		m_Xoffset->hide();
		m_Xoffs->hide();
		m_Yoffset->hide();
		m_Yoffs->hide();
	}
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

EditText::EditText(QWidget* parent, const char* name, Qt::WindowFlags wFlags)
		: QDialog(parent, name, TRUE, wFlags)
{
	m_Vbox1 = new Q3VBox(this);

	m_Hbox1 = new Q3HBox(m_Vbox1);
	m_TextEdit = new QLineEdit(m_Hbox1);

	m_Hbox2 = new Q3HBox(m_Vbox1);
	m_SaveBut = new QPushButton("Save", m_Hbox2);
	m_CancelBut = new QPushButton("Cancel", m_Hbox2);

	setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));

	m_Vbox1->setFixedSize(200, 50);

	QObject_connect(m_SaveBut, SIGNAL(clicked()), this, SLOT(accept()));
	QObject_connect(m_CancelBut, SIGNAL(clicked()), this, SLOT(reject()));
}

EditText::~EditText() { delete m_Vbox1; }

void EditText::SetEditableText(QString editable_text)
{
	m_TextEdit->setText(editable_text);
}

QString EditText::GetEditableText() { return m_TextEdit->text(); }

SupportedMultiDatasetTypes::SupportedMultiDatasetTypes(QWidget* parent, const char* name, Qt::WindowFlags wFlags)
		: QDialog(parent, name, TRUE, wFlags)
{
	m_Hboxoverall = new Q3HBoxLayout(this);
	m_Vboxoverall = new Q3VBoxLayout(this);
	m_Hboxoverall->addLayout(m_Vboxoverall);

	// Dataset selection group box
	QGroupBox* radio_group_box = new QGroupBox("Supported types");
	Q3VBoxLayout* radio_layout = new Q3VBoxLayout(this);
	for (int i = 0; i < eSupportedTypes::nrSupportedTypes; i++)
	{
		QString texted = ToQString(eSupportedTypes(i));
		QRadioButton* radio_but = new QRadioButton(texted);
		radio_layout->addWidget(radio_but);

		m_RadioButs.push_back(radio_but);
	}
	radio_group_box->setLayout(radio_layout);
	m_Vboxoverall->addWidget(radio_group_box);

	QHBoxLayout* buttons_layout = new QHBoxLayout();
	m_SelectBut = new QPushButton("Select", this);
	m_CancelBut = new QPushButton("Cancel", this);
	buttons_layout->addWidget(m_SelectBut);
	buttons_layout->addWidget(m_CancelBut);
	m_Vboxoverall->addLayout(buttons_layout);

	setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));

	QObject_connect(m_SelectBut, SIGNAL(clicked()), this, SLOT(accept()));
	QObject_connect(m_CancelBut, SIGNAL(clicked()), this, SLOT(reject()));
}

SupportedMultiDatasetTypes::~SupportedMultiDatasetTypes()
{
	delete m_Vboxoverall;
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
