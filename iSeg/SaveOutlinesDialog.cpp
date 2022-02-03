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

#include "SaveOutlinesDialog.h"
#include "SlicesHandler.h"
#include "StdStringToQString.h"
#include "TissueInfos.h"

#include "Interface/PropertyWidget.h"
#include "Interface/QtConnect.h"

#include "Interface/RecentPlaces.h"

#include <QFileDialog>
#include <QListWidget>
#include <QMessageBox>
#include <QBoxLayout>

namespace iseg {

SaveOutlinesDialog::SaveOutlinesDialog(SlicesHandler* hand3D, QWidget* parent, Qt::WindowFlags wFlags)
		: QDialog(parent, wFlags), m_Handler3D(hand3D)
{
	setModal(true);

	// properties
	auto group = PropertyGroup::Create("Options");

	m_Filetype = group->Add("Output Type", PropertyEnum::Create({"Outlines", "Triangle Mesh"}, eOutputType::kLine));
	m_Filetype->SetToolTip("Select the output format / type.");

	m_SimplifyLines = group->Add("Simplification", PropertyEnum::Create({"None", "Doug-Peucker", "Distance"}, eSimplifyLines::kNone));
	m_SimplifyLines->SetToolTip("Select simplification method to coarsen outlines.");

	m_SbDist = group->Add("Segm. Size", PropertyInt::Create(5, 5, 50));
	m_SlF = group->Add("Max. Distance", PropertySlider::Create(15, 0, 100));
	m_SbMinsize = group->Add("Min. Size", PropertyInt::Create(1, 1, 999));

	m_CbExtrusion = group->Add("Extrusions", PropertyBool::Create(false));
	m_SbTopextrusion = group->Add("Slices on top", PropertyInt::Create(10, 0, 100));
	m_SbBottomextrusion = group->Add("Slices on bottom", PropertyInt::Create(10, 0, 100));

	m_CbSmooth = group->Add("Smooth", PropertyBool::Create(false));
	m_CbSimplify = group->Add("Simplify", PropertyBool::Create(false));
	m_CbMarchingcubes = group->Add("Marching Cubes (faster)", PropertyBool::Create(true));
	m_SbBetween = group->Add("Interpol. Slices", PropertyInt::Create(0, 0, 10));

	m_PbExec = group->Add("Save", PropertyButton::Create([this]() { SavePushed(); }));
	m_PbClose = group->Add("Quit", PropertyButton::Create([this]() { close(); }));

	// connections
	m_Filetype->onModified.connect([this](Property_ptr p, Property::eChangeType type)
	{
		if (type == Property::kValueChanged)
			ModeChanged();
	});
	m_CbExtrusion->onModified.connect([this](Property_ptr p, Property::eChangeType type) {
		if (type == Property::kValueChanged)
			ModeChanged();
	});
	m_SimplifyLines->onModified.connect([this](Property_ptr p, Property::eChangeType type) {
		if (type == Property::kValueChanged)
			SimplifyChanged();
	});
	m_CbExtrusion->onModified.connect([this](Property_ptr p, Property::eChangeType type) {
		if (type == Property::kValueChanged)
			ExtrusionChanged();
	});

	// add widget and layout
	m_LboTissues = new QListWidget;
	for (tissues_size_t i = 1; i <= TissueInfos::GetTissueCount(); ++i)
	{
		m_LboTissues->addItem(ToQ(TissueInfos::GetTissueName(i)));
	}
	m_LboTissues->setSelectionMode(QAbstractItemView::ExtendedSelection);

	auto property_view = new PropertyWidget(group);

	auto layout = new QVBoxLayout;
	layout->addWidget(m_LboTissues, 2);
	layout->addWidget(property_view, 1);
	setLayout(layout);

	ModeChanged();
	SimplifyChanged();
	ExtrusionChanged();
}

void SaveOutlinesDialog::ModeChanged()
{
	const bool lines = m_Filetype->Value() == eOutputType::kLine;

	m_SimplifyLines->SetVisible(lines);
	m_SbMinsize->SetVisible(lines);
	m_SbBetween->SetVisible(lines);
	m_SlF->SetVisible(lines);
	m_CbExtrusion->SetVisible(lines);
	m_SbTopextrusion->SetVisible(lines);
	m_SbBottomextrusion->SetVisible(lines);

	m_CbSmooth->SetVisible(!lines);
	m_CbSimplify->SetVisible(!lines);
	m_CbMarchingcubes->SetVisible(!lines);
}

void SaveOutlinesDialog::SimplifyChanged()
{
	m_SbDist->SetVisible(m_SimplifyLines->Value() == eSimplifyLines::kDist);
	m_SlF->SetVisible(m_SimplifyLines->Value() == eSimplifyLines::kDougpeuck);
}

void SaveOutlinesDialog::ExtrusionChanged()
{
	m_SbTopextrusion->SetEnabled(m_CbExtrusion->Value());
	m_SbBottomextrusion->SetEnabled(m_CbExtrusion->Value());
}

void SaveOutlinesDialog::SavePushed()
{
	std::vector<tissues_size_t> vtissues;
	for (tissues_size_t i = 0; i < TissueInfos::GetTissueCount(); i++)
	{
		if (m_LboTissues->item((int)i)->isSelected())
		{
			vtissues.push_back((tissues_size_t)i + 1);
		}
	}

	if (vtissues.empty())
	{
		QMessageBox::warning(this, tr("Warning"), tr("No tissues have been selected."), QMessageBox::Ok, 0);
		return;
	}

	QString filter = (m_Filetype->Value() == eOutputType::kTriang) ? "Surface grids (*.vtp *.dat *.stl)" : "Files (*.txt)";
	QString loadfilename = RecentPlaces::GetSaveFileName(this, "Save as", QString::null, filter);
	if (!loadfilename.isEmpty())
	{
		if (m_Filetype->Value() == eOutputType::kTriang)
		{
			ISEG_INFO_MSG("triangulating...");
			// If no extension given, add a default one
			if (loadfilename.length() > 4 &&
					!loadfilename.endsWith(QString(".vtp"), Qt::CaseInsensitive) &&
					!loadfilename.endsWith(QString(".stl"), Qt::CaseInsensitive) &&
					!loadfilename.endsWith(QString(".dat"), Qt::CaseInsensitive))
				loadfilename.append(".dat");

			float ratio = m_SlF->Value() * 0.0099f + 0.01f;
			if (m_SimplifyLines->Value() == eSimplifyLines::kNone)
				ratio = 1.0;

			unsigned int smoothingiter = 0;
			if (m_CbSmooth->Value())
				smoothingiter = 9;

			bool usemc = false;
			if (m_CbMarchingcubes->Value())
				usemc = true;

			// Call method to extract surface, smooth, simplify and finally save it to file
			int number_of_errors = m_Handler3D->ExtractTissueSurfaces(loadfilename.toStdString(), vtissues, usemc, ratio, smoothingiter);

			if (number_of_errors < 0)
			{
				QMessageBox::warning(
					this, "iSeg", "Surface extraction failed.\n\nMaybe something is "
					"wrong the pixel type or label field.",
					QMessageBox::Ok | QMessageBox::Default);
			}
			else if (number_of_errors > 0)
			{
				QMessageBox::warning(
					this, "iSeg", "Surface extraction might have failed.\n\nPlease "
					"verify the tissue names and colors carefully.",
					QMessageBox::Ok | QMessageBox::Default);
			}
		}
		else // save lines
		{
			if (loadfilename.length() > 4 && !loadfilename.endsWith(QString(".txt")))
				loadfilename.append(".txt");

			if (m_SbBetween->Value() == 0)
			{
				Pair pair1 = m_Handler3D->GetPixelsize();
				m_Handler3D->SetPixelsize(pair1.high / 2, pair1.low / 2);
				if (m_SimplifyLines->Value() == eSimplifyLines::kDougpeuck)
				{
					m_Handler3D->ExtractContours2Xmirrored(m_SbMinsize->Value(), vtissues, m_SlF->Value() * 0.05f);
				}
				else if (m_SimplifyLines->Value() == eSimplifyLines::kDist)
				{
					m_Handler3D->ExtractContours2Xmirrored(m_SbMinsize->Value(), vtissues);
				}
				else if (m_SimplifyLines->Value() == eSimplifyLines::kNone)
				{
					m_Handler3D->ExtractContours2Xmirrored(m_SbMinsize->Value(), vtissues);
				}
				m_Handler3D->ShiftContours(-(int)m_Handler3D->Width(), -(int)m_Handler3D->Height());
				if (m_CbExtrusion->Value())
				{
					m_Handler3D->SetextrusionContours(m_SbTopextrusion->Value() - 1, m_SbBottomextrusion->Value() - 1);
				}
				m_Handler3D->SaveContours(loadfilename.toAscii());

#define ROTTERDAM
#ifdef ROTTERDAM
				FILE* fp = fopen(loadfilename.toAscii(), "a");
				float disp[3];
				m_Handler3D->GetDisplacement(disp);
				fprintf(fp, "V1\nX%i %i %i %i O%f %f %f\n", -(int)m_Handler3D->Width(), (int)m_Handler3D->Width(), -(int)m_Handler3D->Height(), (int)m_Handler3D->Height(), disp[0], disp[1], disp[2]); // TODO: rotation
				std::vector<AugmentedMark> labels;
				m_Handler3D->GetLabels(&labels);
				if (!labels.empty())
					fprintf(fp, "V1\n");
				for (size_t i = 0; i < labels.size(); i++)
				{
					fprintf(fp, "S%i 1\nL%i %i %s\n", (int)labels[i].slicenr, (int)m_Handler3D->Width() - 1 - (int)labels[i].p.px * 2, (int)labels[i].p.py * 2 - (int)m_Handler3D->Height() + 1, labels[i].name.c_str());
				}
				fclose(fp);
#endif

				m_Handler3D->SetPixelsize(pair1.high, pair1.low);
				if (m_CbExtrusion->Value())
				{
					m_Handler3D->ResetextrusionContours();
				}
			}
			else
			{
				m_Handler3D->ExtractinterpolatesaveContours2Xmirrored(m_SbMinsize->Value(), vtissues, m_SbBetween->Value(), m_SimplifyLines->Value()==kDougpeuck, m_SlF->Value() * 0.05f, loadfilename.toAscii());
			}
		}
		close();
	}
}

} // namespace iseg
