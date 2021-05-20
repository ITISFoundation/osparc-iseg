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

#include "EdgeWidget.h"
#include "SlicesHandler.h"
#include "bmp_read_1.h"

#include "Data/ItkUtils.h"
#include "Data/Point.h"
#include "Data/ScopedTimer.h"
#include "Data/SlicesHandlerITKInterface.h"

#include "Core/BinaryThinningImageFilter.h"
#include "Core/ImageConnectivtyGraph.h"

#include <q3vbox.h>
#include <qbuttongroup.h>
#include <qdialog.h>
#include <qfiledialog.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qsize.h>
#include <qslider.h>
#include <qspinbox.h>
#include <qwidget.h>

#include <vtkCellArray.h>
#include <vtkDataSetWriter.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>

namespace iseg {

EdgeWidget::EdgeWidget(SlicesHandler* hand3D, QWidget* parent, const char* name, Qt::WindowFlags wFlags)
		: WidgetInterface(parent, name, wFlags), m_Handler3D(hand3D)
{
	setToolTip(Format("Various edge extraction routines. These are mostly useful "
										"as part of other segmentation techniques."));

	m_Activeslice = m_Handler3D->ActiveSlice();
	m_Bmphand = m_Handler3D->GetActivebmphandler();

	m_Hboxoverall = new Q3HBox(this);
	m_Hboxoverall->setMargin(8);
	m_Vboxmethods = new Q3VBox(m_Hboxoverall);
	m_Vbox1 = new Q3VBox(m_Hboxoverall);
	m_Hbox1 = new Q3HBox(m_Vbox1);
	m_Hbox2 = new Q3HBox(m_Vbox1);
	m_Hbox3 = new Q3HBox(m_Vbox1);
	m_Hbox4 = new Q3HBox(m_Vbox1);
	m_BtnExec = new QPushButton("Execute", m_Vbox1);

	m_TxtSigmal = new QLabel("Sigma: 0 ", m_Hbox1);
	m_SlSigma = new QSlider(Qt::Horizontal, m_Hbox1);
	m_SlSigma->setRange(1, 100);
	m_SlSigma->setValue(30);
	m_TxtSigma2 = new QLabel(" 5", m_Hbox1);

	m_TxtThresh11 = new QLabel("Thresh: 0 ", m_Hbox2);
	m_SlThresh1 = new QSlider(Qt::Horizontal, m_Hbox2);
	m_SlThresh1->setRange(1, 100);
	m_SlThresh1->setValue(20);
	m_TxtThresh12 = new QLabel(" 150", m_Hbox2);

	m_TxtThresh21 = new QLabel("Thresh high: 0 ", m_Hbox3);
	m_SlThresh2 = new QSlider(Qt::Horizontal, m_Hbox3);
	m_SlThresh2->setRange(1, 100);
	m_SlThresh2->setValue(80);
	m_TxtThresh22 = new QLabel(" 150", m_Hbox3);

	m_Cb3d = new QCheckBox(QString("3D thinning"), m_Hbox4);
	m_BtnExportCenterlines = new QPushButton(QString("Export"), m_Hbox4);

	m_RbSobel = new QRadioButton(QString("Sobel"), m_Vboxmethods);
	m_RbLaplacian = new QRadioButton(QString("Laplacian"), m_Vboxmethods);
	m_RbInterquartile = new QRadioButton(QString("Interquart."), m_Vboxmethods);
	m_RbMomentline = new QRadioButton(QString("Moment"), m_Vboxmethods);
	m_RbGaussline = new QRadioButton(QString("Gauss"), m_Vboxmethods);
	m_RbCanny = new QRadioButton(QString("Canny"), m_Vboxmethods);
	m_RbLaplacianzero = new QRadioButton(QString("Lapl. Zero"), m_Vboxmethods);
	m_RbCenterlines = new QRadioButton(QString("Centerlines"), m_Vboxmethods);

	m_Modegroup = new QButtonGroup(this);
	//	modegroup->hide();
	m_Modegroup->insert(m_RbSobel);
	m_Modegroup->insert(m_RbLaplacian);
	m_Modegroup->insert(m_RbInterquartile);
	m_Modegroup->insert(m_RbMomentline);
	m_Modegroup->insert(m_RbGaussline);
	m_Modegroup->insert(m_RbCanny);
	m_Modegroup->insert(m_RbLaplacianzero);
	m_Modegroup->insert(m_RbCenterlines);
	m_RbSobel->setChecked(TRUE);

	// 2018.02.14: fix options view is too narrow
	m_Vbox1->setMinimumWidth(std::max(300, m_Vbox1->sizeHint().width()));

	m_Vboxmethods->setMargin(5);
	m_Vbox1->setMargin(5);
	m_Vboxmethods->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
	m_Vboxmethods->setLineWidth(1);

	// 	vboxmethods->setFixedSize(vboxmethods->sizeHint());
	// 	hboxoverall->setFixedSize(hboxoverall->sizeHint());
	//	setFixedSize(vbox1->size());

	MethodChanged(0);

	QObject_connect(m_Modegroup, SIGNAL(buttonClicked(int)), this, SLOT(MethodChanged(int)));
	QObject_connect(m_BtnExec, SIGNAL(clicked()), this, SLOT(Execute()));
	QObject_connect(m_SlSigma, SIGNAL(valueChanged(int)), this, SLOT(SliderChanged(int)));
	QObject_connect(m_SlThresh1, SIGNAL(valueChanged(int)), this, SLOT(SliderChanged(int)));
	QObject_connect(m_SlThresh2, SIGNAL(valueChanged(int)), this, SLOT(SliderChanged(int)));

	QObject_connect(m_BtnExportCenterlines, SIGNAL(clicked()), this, SLOT(ExportCenterlines()));
}

EdgeWidget::~EdgeWidget()
= default;

void EdgeWidget::OnSlicenrChanged()
{
	m_Activeslice = m_Handler3D->ActiveSlice();
	BmphandChanged(m_Handler3D->GetActivebmphandler());
}

void EdgeWidget::BmphandChanged(Bmphandler* bmph)
{
	m_Bmphand = bmph;
}

void EdgeWidget::ExportCenterlines()
{
	QString savefilename = QFileDialog::getSaveFileName(QString::null, "VTK legacy file (*.vtk)\n", this);
	if (!savefilename.isEmpty())
	{
		SlicesHandlerITKInterface wrapper(m_Handler3D);
		auto target = wrapper.GetTarget(true);

		using input_type = itk::SliceContiguousImage<float>;
		auto edges = ImageConnectivityGraph<input_type>(target, target->GetBufferedRegion());

		auto points = vtkSmartPointer<vtkPoints>::New();
		auto lines = vtkSmartPointer<vtkCellArray>::New();

		std::vector<vtkIdType> node_map(target->GetBufferedRegion().GetNumberOfPixels(), -1);
		itk::Point<double, 3> p;
		for (auto e : edges)
		{
			if (node_map[e[0]] == -1)
			{
				target->TransformIndexToPhysicalPoint(target->ComputeIndex(e[0]), p);
				node_map[e[0]] = points->InsertNextPoint(p[0], p[1], p[2]);
			}
			if (node_map[e[1]] == -1)
			{
				target->TransformIndexToPhysicalPoint(target->ComputeIndex(e[1]), p);
				node_map[e[1]] = points->InsertNextPoint(p[0], p[1], p[2]);
			}
			lines->InsertNextCell(2);
			lines->InsertCellPoint(node_map[e[0]]);
			lines->InsertCellPoint(node_map[e[1]]);
		}

		auto pd = vtkSmartPointer<vtkPolyData>::New();
		pd->SetLines(lines);
		pd->SetPoints(points);

		auto writer = vtkSmartPointer<vtkDataSetWriter>::New();
		writer->SetInputData(pd);
		writer->SetFileTypeToBinary();
		writer->SetFileName(savefilename.toStdString().c_str());
		writer->Write();
	}
}

void EdgeWidget::Execute()
{
	DataSelection data_selection;
	data_selection.sliceNr = m_Handler3D->ActiveSlice();
	data_selection.work = true;
	emit BeginDatachange(data_selection, this);

	if (m_RbSobel->isChecked())
	{
		m_Bmphand->Sobel();
	}
	else if (m_RbLaplacian->isChecked())
	{
		m_Bmphand->Laplacian1();
	}
	else if (m_RbInterquartile->isChecked())
	{
		m_Bmphand->MedianInterquartile(false);
	}
	else if (m_RbMomentline->isChecked())
	{
		m_Bmphand->MomentLine();
	}
	else if (m_RbGaussline->isChecked())
	{
		m_Bmphand->GaussLine(m_SlSigma->value() * 0.05f);
	}
	else if (m_RbCanny->isChecked())
	{
		m_Bmphand->CannyLine(m_SlSigma->value() * 0.05f, m_SlThresh1->value() * 1.5f, m_SlThresh2->value() * 1.5f);
	}
	else if (m_RbCenterlines->isChecked())
	{
		try
		{
			ScopedTimer timer("Skeletonization");
			if (m_Cb3d->isChecked())
			{
				using input_type = itk::SliceContiguousImage<float>;
				using output_type = itk::Image<unsigned char, 3>;

				SlicesHandlerITKInterface wrapper(m_Handler3D);
				auto target = wrapper.GetTarget(true);
				auto skeleton = BinaryThinning<input_type, output_type>(target, 0.001f);

				DataSelection data_selection;
				data_selection.allSlices = true;
				data_selection.work = true;
				emit BeginDatachange(data_selection, this);

				iseg::Paste<output_type, input_type>(skeleton, target);

				emit EndDatachange(this);
			}
			else
			{
				using input_type = itk::Image<float, 2>;
				using output_type = itk::Image<unsigned char, 2>;

				SlicesHandlerITKInterface wrapper(m_Handler3D);
				auto target = wrapper.GetTargetSlice();
				auto skeleton = BinaryThinning<input_type, output_type>(target, 0.001f);

				DataSelection data_selection;
				data_selection.sliceNr = m_Handler3D->ActiveSlice();
				data_selection.work = true;
				emit BeginDatachange(data_selection, this);

				iseg::Paste<output_type, input_type>(skeleton, target);

				emit EndDatachange(this);
			}
		}
		catch (itk::ExceptionObject& e)
		{
			ISEG_ERROR("Thinning failed: " << e.what());
		}
		catch (std::exception& e)
		{
			ISEG_ERROR("Thinning failed: " << e.what());
		}
	}
	else
	{
		m_Bmphand->LaplacianZero(m_SlSigma->value() * 0.05f, m_SlThresh1->value() * 0.5f, false);
	}

	emit EndDatachange(this);
}

void EdgeWidget::MethodChanged(int)
{
	if (hideparams)
	{
		if (!m_RbLaplacian->isChecked())
		{
			m_RbLaplacian->hide();
		}
		if (!m_RbInterquartile->isChecked())
		{
			m_RbInterquartile->hide();
		}
		if (!m_RbMomentline->isChecked())
		{
			m_RbMomentline->hide();
		}
		if (!m_RbGaussline->isChecked())
		{
			m_RbGaussline->hide();
		}
		if (!m_RbGaussline->isChecked())
		{
			m_RbLaplacianzero->hide();
		}
	}
	else
	{
		m_RbLaplacian->show();
		m_RbInterquartile->show();
		m_RbMomentline->show();
		m_RbGaussline->show();
		m_RbLaplacianzero->show();
	}

	if (m_RbSobel->isChecked())
	{
		m_Hbox1->hide();
		m_Hbox2->hide();
		m_Hbox3->hide();
		m_Hbox4->hide();
		m_BtnExec->show();
	}
	else if (m_RbLaplacian->isChecked())
	{
		m_Hbox1->hide();
		m_Hbox2->hide();
		m_Hbox3->hide();
		m_Hbox4->hide();
		m_BtnExec->show();
	}
	else if (m_RbInterquartile->isChecked())
	{
		m_Hbox1->hide();
		m_Hbox2->hide();
		m_Hbox3->hide();
		m_Hbox4->hide();
		m_BtnExec->show();
	}
	else if (m_RbMomentline->isChecked())
	{
		m_Hbox1->hide();
		m_Hbox2->hide();
		m_Hbox3->hide();
		m_Hbox4->hide();
		m_BtnExec->show();
	}
	else if (m_RbGaussline->isChecked())
	{
		if (hideparams)
			m_Hbox1->hide();
		else
			m_Hbox1->show();
		m_Hbox2->hide();
		m_Hbox3->hide();
		m_Hbox4->hide();
		m_BtnExec->show();
	}
	else if (m_RbCanny->isChecked())
	{
		m_TxtThresh11->setText("Thresh low:  0 ");
		m_TxtThresh12->setText(" 150");
		if (hideparams)
			m_Hbox1->hide();
		else
			m_Hbox1->show();
		if (hideparams)
			m_Hbox2->hide();
		else
			m_Hbox2->show();
		if (hideparams)
			m_Hbox3->hide();
		else
			m_Hbox3->show();
		m_Hbox4->hide();
		m_BtnExec->show();
	}
	else if (m_RbCenterlines->isChecked())
	{
		m_Hbox1->hide();
		m_Hbox2->hide();
		m_Hbox3->hide();
		m_Hbox4->show();
		m_BtnExec->show();
	}
	else
	{
		m_TxtThresh11->setText("Thresh: 0 ");
		m_TxtThresh12->setText(" 50");
		if (hideparams)
			m_Hbox1->hide();
		else
			m_Hbox1->show();
		if (hideparams)
			m_Hbox2->hide();
		else
			m_Hbox2->show();
		m_Hbox3->hide();
		m_Hbox4->hide();
		m_BtnExec->show();
	}
}

void EdgeWidget::SliderChanged(int newval)
{
	Execute();
}

QSize EdgeWidget::sizeHint() const { return m_Vbox1->sizeHint(); }

void EdgeWidget::NewLoaded()
{
	m_Activeslice = m_Handler3D->ActiveSlice();
	m_Bmphand = m_Handler3D->GetActivebmphandler();
}

void EdgeWidget::Init()
{
	OnSlicenrChanged();
	HideParamsChanged();
}

FILE* EdgeWidget::SaveParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		int dummy;
		dummy = m_SlSigma->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = m_SlThresh1->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = m_SlThresh2->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_RbSobel->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_RbLaplacian->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_RbInterquartile->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_RbMomentline->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_RbGaussline->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_RbCanny->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(m_RbLaplacianzero->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
	}

	return fp;
}

FILE* EdgeWidget::LoadParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		QObject_disconnect(m_Modegroup, SIGNAL(buttonClicked(int)), this, SLOT(MethodChanged(int)));
		QObject_disconnect(m_SlSigma, SIGNAL(valueChanged(int)), this, SLOT(SliderChanged(int)));
		QObject_disconnect(m_SlThresh1, SIGNAL(valueChanged(int)), this, SLOT(SliderChanged(int)));
		QObject_disconnect(m_SlThresh2, SIGNAL(valueChanged(int)), this, SLOT(SliderChanged(int)));

		int dummy;
		fread(&dummy, sizeof(int), 1, fp);
		m_SlSigma->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		m_SlThresh1->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		m_SlThresh2->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		m_RbSobel->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		m_RbLaplacian->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		m_RbInterquartile->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		m_RbMomentline->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		m_RbGaussline->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		m_RbCanny->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		m_RbLaplacianzero->setChecked(dummy > 0);

		MethodChanged(0);

		QObject_connect(m_Modegroup, SIGNAL(buttonClicked(int)), this, SLOT(MethodChanged(int)));
		QObject_connect(m_SlSigma, SIGNAL(valueChanged(int)), this, SLOT(SliderChanged(int)));
		QObject_connect(m_SlThresh1, SIGNAL(valueChanged(int)), this, SLOT(SliderChanged(int)));
		QObject_connect(m_SlThresh2, SIGNAL(valueChanged(int)), this, SLOT(SliderChanged(int)));
	}
	return fp;
}

void EdgeWidget::HideParamsChanged() { MethodChanged(0); }

} // namespace iseg
