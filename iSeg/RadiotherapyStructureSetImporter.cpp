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

#include "RadiotherapyStructureSetImporter.h"
#include "StdStringToQString.h"
#include "TissueInfos.h"

#include "Interface/PropertyWidget.h"

#include "Core/fillcontour.h"

#include <QMessageBox>
#include <QString>
#include <QStringList>
#include <QBoxLayout>

namespace iseg {

RadiotherapyStructureSetImporter::RadiotherapyStructureSetImporter(QString loadfilename, SlicesHandler* hand3D, QWidget* parent, Qt::WindowFlags wFlags)
		: QDialog(parent, wFlags), m_Handler3D(hand3D)
{
	setModal(true);

	if (loadfilename.isEmpty())
	{
		close();
		return;
	}

	m_Tissues.clear();
	gdcmvtk_rtstruct::RequestData_RTStructureSetStorage(loadfilename.ascii(), m_Tissues);

	m_Vecignore.resize(m_Tissues.size());
	m_Vecpriorities.resize(m_Tissues.size());
	m_Vectissuenames.resize(m_Tissues.size());
	m_Vectissuenrs.resize(m_Tissues.size());
	m_Vecnew.resize(m_Tissues.size());

	tissues_size_t tissuenr;
	std::string namedummy = std::string("");
	tissues_size_t nrnew = 1;
	for (size_t i = 0; i < m_Tissues.size(); i++)
	{
		m_Vecignore[i] = false;
		m_Vecpriorities[i] = i + 1;
		if (m_Tissues[i]->name != namedummy)
		{
			m_Vectissuenames[i] = m_Tissues[i]->name;
		}
		else
		{
			QString sdummy;
			sdummy.setNum(nrnew);
			m_Vectissuenames[i] = std::string("tissue") + sdummy.toStdString();
			nrnew++;
		}

		for (tissuenr = 0;
				 tissuenr < TissueInfos::GetTissueCount() &&
				 m_Tissues[i]->name != TissueInfos::GetTissueName(tissuenr + 1);
				 tissuenr++)
		{}
		if (tissuenr == (tissues_size_t)TissueInfos::GetTissueCount())
		{
			m_Vecnew[i] = true;
			m_Vectissuenrs[i] = 0;
		}
		else
		{
			m_Vecnew[i] = false;
			m_Vectissuenrs[i] = tissuenr;
		}
	}

	// graphic user interface
	auto group = PropertyGroup::Create("Settings");

	PropertyEnum::descriptions_type solids;
	for (size_t i = 0; i < m_Tissues.size(); i++)
	{
		solids.push_back(m_Tissues[i]->name);
	}
	m_CbSolids = group->Add("Tissues", PropertyEnum::Create(solids, m_Currentitem = 0));
	m_CbIgnore = group->Add("Ignore", PropertyBool::Create(m_Vecignore[m_Currentitem]));

	m_SbPriority = group->Add("Priority", PropertyInt::Create(m_Vecpriorities[m_Currentitem], 1, m_Tissues.size()));
	m_SbPriority->SetToolTip(
			"1) Tissues have been sorted so that higher priority is given to the smallest tissues.<br>"
			"2) The higher number of priority, higher priority will have.");

	m_CbNew = group->Add("New Tissue", PropertyBool::Create(m_Vecnew[m_Currentitem]));
	m_LeName = group->Add("Name", PropertyString::Create(m_Vectissuenames[m_Currentitem]));

	PropertyEnum::descriptions_type names;
	for (size_t i = 0; i < m_Tissues.size(); i++)
	{
		names.push_back(TissueInfos::GetTissueName(i));
	}
	m_CbNames = group->Add("Name", PropertyEnum::Create(names, m_Vectissuenrs[m_Currentitem]));

	m_PbOk = group->Add("OK", PropertyButton::Create([this]() { OkPressed(); }));
	m_PbCancel = group->Add("Cancel", PropertyButton::Create([this]() { close(); }));

	// connections
	m_CbSolids->onModified.connect([this](Property_ptr p, Property::eChangeType type){
		if (type == Property::kValueChanged)
			SolidChanged(m_CbSolids->Value());
	});
	m_CbNew->onModified.connect([this](Property_ptr p, Property::eChangeType type) {
		if (type == Property::kValueChanged)
			NewChanged();
	});
	m_CbIgnore->onModified.connect([this](Property_ptr p, Property::eChangeType type) {
		if (type == Property::kValueChanged)
			IgnoreChanged();
	});

	// add widget and layout
	auto property_view = new PropertyWidget(group);
	property_view->setMinimumSize(300, 200);

	auto layout = new QHBoxLayout;
	layout->addWidget(property_view);
	setLayout(layout);

	// initalize
	Updatevisibility();
}

RadiotherapyStructureSetImporter::~RadiotherapyStructureSetImporter() = default;

void RadiotherapyStructureSetImporter::SolidChanged(int i)
{
	Storeparams();

	BlockPropertySignal block_new(m_CbNew);
	BlockPropertySignal block_ignore(m_CbIgnore);

	m_Currentitem = i;
	m_CbIgnore->SetValue(m_Vecignore[i]);
	m_SbPriority->SetValue(m_Vecpriorities[i]);
	m_CbNew->SetValue(m_Vecnew[i]);
	m_LeName->SetValue(m_Vectissuenames[i]);
	m_CbNames->SetValue(m_Vectissuenrs[i]);

	Updatevisibility();
}

void RadiotherapyStructureSetImporter::NewChanged()
{
	Storeparams();
	Updatevisibility();
}

void RadiotherapyStructureSetImporter::OkPressed()
{
	Storeparams();

	bool* mask = new bool[m_Handler3D->ReturnArea()];
	if (mask == nullptr)
	{
		QMessageBox::about(this, "Warning", "Not enough memory");
		return;
	}

	tissues_size_t nrnew = 0;
	for (size_t i = 0; i < m_Tissues.size(); i++)
	{
		if (m_Vecignore[i] == false && !m_Vectissuenames[i].empty() && m_Vecnew[i] == true)
			nrnew++;
	}
	if (nrnew >= TISSUES_SIZE_MAX - 1 - TissueInfos::GetTissueCount())
	{
		QMessageBox::about(this, "Warning", "Max number of solids exceeded");
		return;
	}

	Pair p;
	p = m_Handler3D->GetPixelsize();
	float thick = m_Handler3D->GetSlicethickness();
	float disp[3];
	m_Handler3D->GetDisplacement(disp);
	float dc[6];
	m_Handler3D->GetDirectionCosines(dc);
	unsigned short pixel_extents[2] = {m_Handler3D->Width(), m_Handler3D->Height()};
	float pixel_size[2] = {p.high, p.low};

	ISEG_INFO("RTStruct import: thickness = " << thick);

	if (std::abs(dc[0]) != 1.0f || std::abs(dc[4]) != 1.0f)
	{
		QMessageBox::warning(this, "Warning", "Arbitrary rotations of image orientation (patient) not supported");
		return;
	}

	DataSelection data_selection;
	data_selection.allSlices = true;
	data_selection.tissues = true;
	emit BeginDatachange(data_selection, this);

	bool error = false;
	tissues_size_t tissuenr;
	std::string namedummy;
	for (int i = 1; i <= m_Tissues.size(); i++) // i is priority
	{
		if (error)
			break;

		gdcmvtk_rtstruct::tissuevec::iterator it = m_Tissues.begin();
		int j = 0;
		while (m_Vecpriorities[j] != i) // j is tissue index, processed in order of priority
			j++, it++;

		if (m_Vecignore[j] == false && ((!m_Vecnew[j]) || (!m_Vectissuenames[j].empty())))
		{
			const auto& rtstruct_i = *it;

			if (m_Vecnew[j])
			{
				TissueInfo tissue_info;
				tissue_info.m_Name = m_Vectissuenames[j];
				tissue_info.m_Locked = false;
				tissue_info.m_Opac = 0.5f;
				tissue_info.m_Color[0] = rtstruct_i->color[0];
				tissue_info.m_Color[1] = rtstruct_i->color[1];
				tissue_info.m_Color[2] = rtstruct_i->color[2];
				TissueInfos::AddTissue(tissue_info);
				tissuenr = TissueInfos::GetTissueCount();
			}
			else
			{
				tissuenr = m_Vectissuenrs[j] + 1;
			}

			size_t pospoints = 0;
			size_t posoutlines = 0;
			std::vector<float*> points;
			bool clockwisefill = false;
			while (posoutlines < rtstruct_i->outlinelength.size())
			{
				points.clear();
				unsigned int* nrpoints = &(rtstruct_i->outlinelength[posoutlines]);
				float zcoord = rtstruct_i->points[pospoints + 2];
				points.push_back(&(rtstruct_i->points[pospoints]));
				pospoints += rtstruct_i->outlinelength[posoutlines] * 3;
				posoutlines++;
				while (posoutlines < rtstruct_i->outlinelength.size() && zcoord == rtstruct_i->points[pospoints + 2])
				{
					points.push_back(&(rtstruct_i->points[pospoints]));
					pospoints += rtstruct_i->outlinelength[posoutlines] * 3;
					posoutlines++;
				}

				const int start_sl = m_Handler3D->StartSlice();
				const int end_sl = m_Handler3D->EndSlice();

				float swap_z = dc[0] * dc[4];
				int slicenr = round(swap_z * (disp[2] - zcoord) / thick);

				if (slicenr < 0 && swap_z > 0.f)
				{
					ISEG_WARNING("RTStruct import: strange slice Value " << slicenr);
					slicenr = end_sl + slicenr; // TODO this is strange!
				}

				ISEG_INFO("RTStruct import: importing slice " << slicenr)

				if (slicenr >= start_sl && slicenr < end_sl)
				{
					try
					{
						fillcontours::fill_contour(mask, pixel_extents, disp, pixel_size, dc, &(points[0]), nrpoints, points.size(), clockwisefill);
					}
					catch (std::exception& e)
					{
						QMessageBox::warning(this, "An Exception Occurred", e.what());
						error = true;
						break;
					}
					m_Handler3D->Add2tissue(tissuenr, mask, static_cast<unsigned short>(slicenr), true);
				}
				else
				{
					ISEG_WARNING("RTStruct import: invalid slice " << slicenr);
				}
			}
		}
	}

	emit EndDatachange(this);

	delete[] mask;

	close();
}

void RadiotherapyStructureSetImporter::IgnoreChanged()
{
	Storeparams();
	Updatevisibility();
}

void RadiotherapyStructureSetImporter::Updatevisibility()
{
	m_CbNew->SetVisible(!m_Vecignore[m_CbSolids->Value()]);
	m_SbPriority->SetVisible(!m_Vecignore[m_CbSolids->Value()]);
	m_CbNew->SetVisible(!m_Vecignore[m_CbSolids->Value()]);

	bool new_solid = m_Vecnew[m_CbSolids->Value()];
	m_LeName->SetVisible(!m_Vecignore[m_CbSolids->Value()] && new_solid);
	m_CbNames->SetVisible(!m_Vecignore[m_CbSolids->Value()] && !new_solid);
}

void RadiotherapyStructureSetImporter::Storeparams()
{
	int dummy = m_Vecpriorities[m_Currentitem];

	m_Vecnew[m_Currentitem] = m_CbNew->Value();
	m_Vecignore[m_Currentitem] = m_CbIgnore->Value();
	m_Vectissuenrs[m_Currentitem] = m_CbNames->Value();
	m_Vectissuenames[m_Currentitem] = m_LeName->Value();

	if (dummy != m_SbPriority->Value())
	{
		m_Vecpriorities[m_Currentitem] = m_SbPriority->Value();
		int pos = 0;
		if (m_Currentitem == 0)
			pos = 1;

		while (m_Vecpriorities[pos] != m_Vecpriorities[m_Currentitem])
		{
			pos++;
			if (pos == m_Currentitem)
				pos++;
		}
		m_Vecpriorities[pos] = dummy;
	}
}

} // namespace iseg
