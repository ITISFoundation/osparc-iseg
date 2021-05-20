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

#include "InterpolationWidget.h"
#include "SlicesHandler.h"

#include "Data/BrushInteraction.h"

#include <q3vbox.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qdialog.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qspinbox.h>
#include <qwidget.h>

#include <algorithm>

namespace iseg {

InterpolationWidget::InterpolationWidget(SlicesHandler* hand3D, QWidget* parent, const char* name, Qt::WindowFlags wFlags)
		: WidgetInterface(parent, name, wFlags), m_Handler3D(hand3D)
{
	setToolTip(Format("Interpolate/extrapolate between segmented slices."));

	m_Brush = nullptr;

	m_Nrslices = m_Handler3D->NumSlices();

	m_Hboxoverall = new Q3HBox(this);
	m_Hboxoverall->setMargin(8);
	m_Vboxmethods = new Q3VBox(m_Hboxoverall);
	m_Vboxdataselect = new Q3VBox(m_Hboxoverall);
	m_Vboxparams = new Q3VBox(m_Hboxoverall);
	m_Vboxexecute = new Q3VBox(m_Hboxoverall);

	// Methods
	m_Vboxmethods->layout()->setAlignment(Qt::AlignTop);
	m_RbInter = new QRadioButton(QString("Interpolation"), m_Vboxmethods);
	m_RbExtra = new QRadioButton(QString("Extrapolation"), m_Vboxmethods);
	m_RbBatchinter = new QRadioButton(QString("Batch Interpol."), m_Vboxmethods);
	m_RbBatchinter->setToolTip(Format("Select the stride of (distance between) segmented slices and the "
																		"start slice to batch segment multiple slices."));
	m_Modegroup = new QButtonGroup(this);
	m_Modegroup->insert(m_RbInter);
	m_Modegroup->insert(m_RbExtra);
	m_Modegroup->insert(m_RbBatchinter);
	m_RbInter->setChecked(TRUE);

	// Data selection
	m_Vboxdataselect->layout()->setAlignment(Qt::AlignTop);
	m_RbTissue = new QRadioButton(QString("Selected Tissue"), m_Vboxdataselect);
	m_RbTissue->setToolTip(Format("Works on the tissue distribution of the currently selected tissue."));
	m_RbTissueall = new QRadioButton(QString("All Tissues"), m_Vboxdataselect);
	m_RbTissueall->setToolTip(Format("Works on all tissue at once (not available for extrapolation)."));
	m_RbWork = new QRadioButton(QString("TargetPict"), m_Vboxdataselect);
	m_RbWork->setToolTip(Format("Works on the Target image."));

	m_Sourcegroup = new QButtonGroup(this);
	m_Sourcegroup->insert(m_RbTissue);
	m_Sourcegroup->insert(m_RbTissueall);
	m_Sourcegroup->insert(m_RbWork);
	m_RbTissue->setChecked(TRUE);

	// Parameters
	m_Vboxparams->layout()->setAlignment(Qt::AlignTop);

	m_Hboxextra = new Q3HBox(m_Vboxparams);
	m_TxtSlicenr = new QLabel("Target Slice: ", m_Hboxextra);
	m_SbSlicenr = new QSpinBox(1, m_Nrslices, 1, m_Hboxextra);
	m_SbSlicenr->setValue(1);
	m_SbSlicenr->setToolTip(Format("Set the slice index where the Tissue/Target "
																 "distribution will be extrapolated."));

	m_Hboxbatch = new Q3HBox(m_Vboxparams);
	m_TxtBatchstride = new QLabel("Stride: ", m_Hboxbatch);
	m_SbBatchstride = new QSpinBox(2, m_Nrslices - 1, 1, m_Hboxbatch);
	m_SbBatchstride->setValue(2);
	m_SbBatchstride->setToolTip(Format("The stride is the distance between segmented slices. For example, "
																		 "you may segment every fifth slice (stride=5), then interpolate in "
																		 "between."));

	m_CbConnectedshapebased = new QCheckBox(QString("Connected Shape-Based"), m_Vboxparams);
	m_CbConnectedshapebased->setToolTip(Format("Align corresponding foreground objects by their center of mass, "
																						 "to ensure shape-based interpolation connects these objects"));

	m_CbMedianset = new QCheckBox(QString("Median Set"), m_Vboxparams);
	m_CbMedianset->setChecked(false);
	m_CbMedianset->setToolTip(Format("If Median Set is ON, the algorithm described in [1] "
																	 "is employed. Otherwise shape-based interpolation [2] is used. "
																	 "Multiple "
																	 "objects (with "
																	 "different gray values or tissue assignments, respectively) can be "
																	 "interpolated jointly "
																	 "without introducing gaps or overlap.<br>"
																	 "[1] S. Beucher. Sets, partitions and functions interpolations. "
																	 "1998.<br>"
																	 "[2] S. P. Raya and J. K. Udupa. Shape-based interpolation of "
																	 "multidimensional objects. 1990."));
	m_Rb4connectivity = new QRadioButton(QString("4-connectivity"), m_Vboxparams);
	m_Rb8connectivity = new QRadioButton(QString("8-connectivity"), m_Vboxparams);
	m_Connectivitygroup = new QButtonGroup(this);
	m_Connectivitygroup->insert(m_Rb4connectivity);
	m_Connectivitygroup->insert(m_Rb8connectivity);
	m_Rb8connectivity->setChecked(TRUE);

	m_CbBrush = new QCheckBox("Enable brush");
	m_CbBrush->setChecked(false);

	m_BrushRadius = new QLineEdit(QString::number(1));
	m_BrushRadius->setValidator(new QDoubleValidator);
	m_BrushRadius->setToolTip(Format("Set the radius of the brush in physical units, i.e. typically mm."));

	auto brush_param = new QWidget(m_Vboxparams);
	auto brush_layout = new QHBoxLayout;
	brush_layout->addWidget(m_CbBrush);
	brush_layout->addWidget(new QLabel("Brush radius"));
	brush_layout->addWidget(m_BrushRadius);
	brush_param->setLayout(brush_layout);

	// Execute
	m_Vboxexecute->layout()->setAlignment(Qt::AlignTop);
	m_Pushstart = new QPushButton("Start Slice", m_Vboxexecute);
	m_Pushstart->setToolTip(Format("Interpolation/extrapolation is based on 2 slices. Click start to "
																 "select the first slice and Execute to select the second slice. Interpolation "
																 "automatically interpolates the intermediate slices."
																 "<br>"
																 "Note:<br>The result is displayed in the Target but is not directly "
																 "added to the tissue distribution. "
																 "The user can add it with Adder function. The 'All Tissues' option "
																 "adds the result directly to the tissue."));
	m_Pushexec = new QPushButton("Execute", m_Vboxexecute);
	m_Pushexec->setEnabled(false);

	m_Vboxmethods->setMargin(5);
	m_Vboxdataselect->setMargin(5);
	m_Vboxparams->setMargin(5);
	m_Vboxexecute->setMargin(5);
	m_Vboxmethods->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
	m_Vboxmethods->setLineWidth(1);

	m_Vboxmethods->setFixedSize(m_Vboxparams->sizeHint());
	m_Vboxdataselect->setFixedSize(m_Vboxparams->sizeHint());
	m_Vboxparams->setFixedSize(m_Vboxparams->sizeHint());
	m_Vboxexecute->setFixedSize(m_Vboxparams->sizeHint());

	QObject_connect(m_BrushRadius, SIGNAL(textEdited(QString)), this, SLOT(BrushChanged()));
	QObject_connect(m_Modegroup, SIGNAL(buttonClicked(int)), this, SLOT(BrushChanged()));

	QObject_connect(m_Pushstart, SIGNAL(clicked()), this, SLOT(StartslicePressed()));
	QObject_connect(m_CbMedianset, SIGNAL(stateChanged(int)), this, SLOT(MethodChanged()));
	QObject_connect(m_CbConnectedshapebased, SIGNAL(stateChanged(int)), this, SLOT(MethodChanged()));
	QObject_connect(m_Modegroup, SIGNAL(buttonClicked(int)), this, SLOT(MethodChanged()));
	QObject_connect(m_Sourcegroup, SIGNAL(buttonClicked(int)), this, SLOT(SourceChanged()));

	QObject_connect(m_Pushexec, SIGNAL(clicked()), this, SLOT(Execute()));

	MethodChanged();
	SourceChanged();
}

InterpolationWidget::~InterpolationWidget()
{
	delete m_Sourcegroup;
	delete m_Modegroup;
	delete m_Hboxoverall;
}

QSize InterpolationWidget::sizeHint() const { return m_Hboxoverall->sizeHint(); }

void InterpolationWidget::Init()
{
	if (m_Handler3D->NumSlices() != m_Nrslices)
	{
		m_Nrslices = m_Handler3D->NumSlices();
		m_SbSlicenr->setMaxValue((int)m_Nrslices);
		m_SbBatchstride->setMaxValue((int)m_Nrslices - 1);
		m_Pushexec->setEnabled(false);
	}

	if (!m_Brush)
	{
		m_Brush = new BrushInteraction(
				m_Handler3D,
				[this](DataSelection sel) { emit BeginDatachange(sel, this); },
				[this](iseg::eEndUndoAction a) { emit EndDatachange(this, a); },
				[this](std::vector<Point>* vp) { emit VpdynChanged(vp); });
		BrushChanged();
	}
	else
	{
		m_Brush->Init(m_Handler3D);
	}
}

void InterpolationWidget::NewLoaded() {}

void InterpolationWidget::OnSlicenrChanged() {}

void InterpolationWidget::OnTissuenrChanged(int tissuetype)
{
	m_Tissuenr = (tissues_size_t)tissuetype + 1;
}

void InterpolationWidget::Handler3DChanged()
{
	if (m_Handler3D->NumSlices() != m_Nrslices)
	{
		m_Nrslices = m_Handler3D->NumSlices();
		m_SbSlicenr->setMaxValue((int)m_Nrslices);
		m_SbBatchstride->setMaxValue((int)m_Nrslices - 1);
	}
	m_Pushexec->setEnabled(false);
}

void InterpolationWidget::StartslicePressed()
{
	m_Startnr = m_Handler3D->ActiveSlice();
	m_Pushexec->setEnabled(true);
}

void InterpolationWidget::OnMouseClicked(Point p)
{
	if (!m_CbBrush->isChecked())
	{
		WidgetInterface::OnMouseClicked(p);
	}
	else if (m_RbWork->isChecked() || m_RbTissue->isChecked())
	{
		m_Brush->SetTissueValue(m_Tissuenr);
		m_Brush->OnMouseClicked(p);
	}
}

void InterpolationWidget::OnMouseReleased(Point p)
{
	if (!m_CbBrush->isChecked())
	{
		WidgetInterface::OnMouseReleased(p);
	}
	else if (m_RbWork->isChecked() || m_RbTissue->isChecked())
	{
		m_Brush->OnMouseReleased(p);
	}
}

void InterpolationWidget::OnMouseMoved(Point p)
{
	if (!m_CbBrush->isChecked())
	{
		WidgetInterface::OnMouseMoved(p);
	}
	else if (m_RbWork->isChecked() || m_RbTissue->isChecked())
	{
		m_Brush->OnMouseMoved(p);
	}
}

void InterpolationWidget::Execute()
{
	unsigned short batchstride = (unsigned short)m_SbBatchstride->value();
	bool const connected = m_CbConnectedshapebased->isVisible() && m_CbConnectedshapebased->isChecked();

	unsigned short current = m_Handler3D->ActiveSlice();
	if (current != m_Startnr)
	{
		DataSelection data_selection;
		if (m_RbExtra->isChecked())
		{
			data_selection.sliceNr = (unsigned short)m_SbSlicenr->value() - 1;
			data_selection.work = true;
			emit BeginDatachange(data_selection, this);
			if (m_RbWork->isChecked())
			{
				m_Handler3D->Extrapolatework(m_Startnr, current, (unsigned short)m_SbSlicenr->value() - 1);
			}
			else
			{
				m_Handler3D->Extrapolatetissue(m_Startnr, current, (unsigned short)m_SbSlicenr->value() - 1, m_Tissuenr);
			}
			emit EndDatachange(this);
		}
		else if (m_RbInter->isChecked())
		{
			data_selection.allSlices = true;

			if (m_RbWork->isChecked())
			{
				data_selection.work = true;
				emit BeginDatachange(data_selection, this);
				if (m_CbMedianset->isChecked())
				{
					m_Handler3D->InterpolateworkgreyMedianset(m_Startnr, current, m_Rb8connectivity->isChecked());
				}
				else
				{
					m_Handler3D->Interpolateworkgrey(m_Startnr, current, connected);
				}
			}
			else if (m_RbTissue->isChecked())
			{
				data_selection.work = true;
				emit BeginDatachange(data_selection, this);
				if (m_CbMedianset->isChecked())
				{
					m_Handler3D->InterpolatetissueMedianset(m_Startnr, current, m_Tissuenr, m_Rb8connectivity->isChecked());
				}
				else
				{
					m_Handler3D->Interpolatetissue(m_Startnr, current, m_Tissuenr, connected);
				}
			}
			else if (m_RbTissueall->isChecked())
			{
				data_selection.tissues = true;
				emit BeginDatachange(data_selection, this);
				if (m_CbMedianset->isChecked())
				{
					m_Handler3D->InterpolatetissuegreyMedianset(m_Startnr, current, m_Rb8connectivity->isChecked());
				}
				else
				{
					m_Handler3D->Interpolatetissuegrey(m_Startnr, current);
				}
			}
			emit EndDatachange(this);
		}
		else if (m_RbBatchinter->isChecked())
		{
			data_selection.allSlices = true;

			if (m_RbWork->isChecked())
			{
				data_selection.work = true;
				emit BeginDatachange(data_selection, this);

				if (m_CbMedianset->isChecked())
				{
					unsigned short batchstart;
					for (batchstart = m_Startnr;
							 batchstart <= current - batchstride;
							 batchstart += batchstride)
					{
						m_Handler3D->InterpolateworkgreyMedianset(batchstart, batchstart + batchstride, m_Rb8connectivity->isChecked());
					}
					// Last batch with smaller stride
					if (batchstart > current && current - (batchstart - batchstride) >= 2)
					{
						m_Handler3D->InterpolateworkgreyMedianset(batchstart - batchstride, current, m_Rb8connectivity->isChecked());
					}
				}
				else
				{
					unsigned short batchstart;
					for (batchstart = m_Startnr;
							 batchstart <= current - batchstride;
							 batchstart += batchstride)
					{
						m_Handler3D->Interpolateworkgrey(batchstart, batchstart + batchstride, connected);
					}
					// Last batch with smaller stride
					if (batchstart > current && current - (batchstart - batchstride) >= 2)
					{
						m_Handler3D->Interpolateworkgrey(batchstart - batchstride, current, connected);
					}
				}
			}
			else if (m_RbTissue->isChecked())
			{
				data_selection.work = true;
				emit BeginDatachange(data_selection, this);

				if (m_CbMedianset->isChecked())
				{
					unsigned short batchstart;
					for (batchstart = m_Startnr;
							 batchstart <= current - batchstride;
							 batchstart += batchstride)
					{
						m_Handler3D->InterpolatetissueMedianset(batchstart, batchstart + batchstride, m_Tissuenr, m_Rb8connectivity->isChecked());
					}
					// Last batch with smaller stride
					if (batchstart > current && current - (batchstart - batchstride) >= 2)
					{
						m_Handler3D->InterpolatetissueMedianset(batchstart - batchstride, current, m_Tissuenr, m_Rb8connectivity->isChecked());
					}
				}
				else
				{
					unsigned short batchstart;
					for (batchstart = m_Startnr;
							 batchstart <= current - batchstride;
							 batchstart += batchstride)
					{
						m_Handler3D->Interpolatetissue(batchstart, batchstart + batchstride, m_Tissuenr, connected);
					}
					// Last batch with smaller stride
					if (batchstart > current && current - (batchstart - batchstride) >= 2)
					{
						m_Handler3D->Interpolatetissue(batchstart - batchstride, current, m_Tissuenr, connected);
					}
				}
			}
			else if (m_RbTissueall->isChecked())
			{
				data_selection.tissues = true;
				emit BeginDatachange(data_selection, this);

				if (m_CbMedianset->isChecked())
				{
					unsigned short batchstart;
					for (batchstart = m_Startnr;
							 batchstart <= current - batchstride;
							 batchstart += batchstride)
					{
						m_Handler3D->InterpolatetissuegreyMedianset(batchstart, batchstart + batchstride, m_Rb8connectivity->isChecked());
					}
					// Last batch with smaller stride
					if (batchstart > current &&
							current - (batchstart - batchstride) >= 2)
					{
						m_Handler3D->InterpolatetissuegreyMedianset(batchstart - batchstride, current, m_Rb8connectivity->isChecked());
					}
				}
				else
				{
					unsigned short batchstart;
					for (batchstart = m_Startnr;
							 batchstart <= current - batchstride;
							 batchstart += batchstride)
					{
						m_Handler3D->Interpolatetissuegrey(batchstart, batchstart + batchstride);
					}
					// Last batch with smaller stride
					if (batchstart > current &&
							current - (batchstart - batchstride) >= 2)
					{
						m_Handler3D->Interpolatetissuegrey(batchstart - batchstride, current);
					}
				}
			}
			emit EndDatachange(this);
		}
		m_Pushexec->setEnabled(false);
	}
}

void InterpolationWidget::BrushChanged()
{
	if (m_Brush)
	{
		m_Brush->SetBrushTarget(m_RbWork->isChecked());
		m_Brush->SetRadius(m_BrushRadius->text().toDouble());
	}
}

void InterpolationWidget::MethodChanged()
{
	if (m_RbExtra->isChecked())
	{
		m_Hboxextra->show();
		m_Hboxbatch->hide();
		m_CbMedianset->hide();
		m_CbConnectedshapebased->hide();
		m_Rb4connectivity->hide();
		m_Rb8connectivity->hide();
		if (m_RbTissueall->isChecked())
		{
			m_RbTissue->setChecked(true);
		}
		m_RbTissueall->setEnabled(false);
	}
	else if (m_RbInter->isChecked())
	{
		m_Hboxextra->hide();
		m_Hboxbatch->hide();
		m_RbTissueall->setEnabled(true);
		m_CbMedianset->show();
		if (m_CbMedianset->isChecked())
		{
			m_CbConnectedshapebased->hide();
			m_Rb4connectivity->show();
			m_Rb8connectivity->show();
		}
		else
		{
			m_CbConnectedshapebased->setVisible(!m_RbTissueall->isChecked());
			m_Rb4connectivity->hide();
			m_Rb8connectivity->hide();
		}
	}
	else if (m_RbBatchinter->isChecked())
	{
		m_Hboxextra->hide();
		m_Hboxbatch->show();
		m_RbTissueall->setEnabled(true);
		m_CbMedianset->show();
		if (m_CbMedianset->isChecked())
		{
			m_Rb4connectivity->show();
			m_Rb8connectivity->show();
		}
		else
		{
			m_Rb4connectivity->hide();
			m_Rb8connectivity->hide();
		}
	}
}

void InterpolationWidget::SourceChanged()
{
	MethodChanged();

	if (m_Brush)
	{
		m_Brush->SetBrushTarget(m_RbWork->isChecked());
	}
}

FILE* InterpolationWidget::SaveParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		int dummy;
		dummy = m_SbSlicenr->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_RbTissue->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_RbWork->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_RbInter->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_RbExtra->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		if (version >= 11)
		{
			dummy = (int)(m_RbTissueall->isChecked());
			fwrite(&(dummy), 1, sizeof(int), fp);
			dummy = (int)(m_RbBatchinter->isChecked());
			fwrite(&(dummy), 1, sizeof(int), fp);
			if (version >= 12)
			{
				dummy = (int)(m_CbMedianset->isChecked());
				fwrite(&(dummy), 1, sizeof(int), fp);
				dummy = (int)(m_Rb4connectivity->isChecked());
				fwrite(&(dummy), 1, sizeof(int), fp);
				dummy = (int)(m_Rb8connectivity->isChecked());
				fwrite(&(dummy), 1, sizeof(int), fp);
			}
		}
	}

	return fp;
}

FILE* InterpolationWidget::LoadParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		QObject_disconnect(m_Modegroup, SIGNAL(buttonClicked(int)), this, SLOT(MethodChanged()));
		QObject_disconnect(m_Sourcegroup, SIGNAL(buttonClicked(int)), this, SLOT(SourceChanged()));

		int dummy;
		fread(&dummy, sizeof(int), 1, fp);
		m_SbSlicenr->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		m_RbTissue->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		m_RbWork->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		m_RbInter->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		m_RbExtra->setChecked(dummy > 0);
		if (version >= 11)
		{
			fread(&dummy, sizeof(int), 1, fp);
			m_RbTissueall->setChecked(dummy > 0);
			fread(&dummy, sizeof(int), 1, fp);
			m_RbBatchinter->setChecked(dummy > 0);
			if (version >= 12)
			{
				fread(&dummy, sizeof(int), 1, fp);
				m_CbMedianset->setChecked(dummy > 0);
				fread(&dummy, sizeof(int), 1, fp);
				m_Rb4connectivity->setChecked(dummy > 0);
				fread(&dummy, sizeof(int), 1, fp);
				m_Rb8connectivity->setChecked(dummy > 0);
			}
		}

		MethodChanged();
		SourceChanged();

		QObject_connect(m_Modegroup, SIGNAL(buttonClicked(int)), this, SLOT(MethodChanged()));
		QObject_connect(m_Sourcegroup, SIGNAL(buttonClicked(int)), this, SLOT(SourceChanged()));
	}
	return fp;
}

} // namespace iseg
