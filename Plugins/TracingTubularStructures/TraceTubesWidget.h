/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#pragma once

#include "Data/SlicesHandlerInterface.h"

#include "Interface/WidgetInterface.h"

#include <itkImage.h>

#include <vector>

class QCheckBox;
class QLineEdit;
class QComboBox;
class QPushButton;
class QStackedWidget;

namespace itk {
class ProcessObject;
}

namespace iseg {
struct Point3D
{
	union {
		struct
		{
			short px, py;
		};
		Point p;
	};
	short pz;
};
} // namespace iseg

class TraceTubesWidget : public iseg::WidgetInterface
{
	Q_OBJECT
public:
	TraceTubesWidget(iseg::SlicesHandlerInterface* hand3D, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~TraceTubesWidget() override = default;

	void Init() override;
	void NewLoaded() override;
	void Cleanup() override;
	std::string GetName() override;
	QIcon GetIcon(QDir picdir) override;

private:
	void OnSlicenrChanged() override;
	void OnMouseReleased(iseg::Point p) override;

	void UpdatePoints();

	int GetPadding() const;

	enum eMetric {
		kIntensity = 0,
		kHessian2D,
		kHessian3D,
		kTarget
	};
	itk::Image<float, 3>::Pointer ComputeVesselness(const itk::ImageBase<3>::RegionType& requested_region) const;

	itk::Image<float, 3>::Pointer ComputeBlobiness(const itk::ImageBase<3>::RegionType& requested_region) const;

	itk::Image<float, 3>::Pointer ComputeObjectSdf(const itk::ImageBase<3>::RegionType& requested_region) const;

	template<class TSpeedImage>
	void DoWorkTemplate(TSpeedImage* speed_image, const itk::ImageBase<3>::RegionType& requested_region);

	iseg::SlicesHandlerInterface* m_Handler;
	std::vector<iseg::Point3D> m_Points;

	QWidget* m_MainOptions;
	QComboBox* m_Metric;
	QLineEdit* m_IntensityValue;
	QLineEdit* m_IntensityWeight;
	QLineEdit* m_AngleWeight;
	QLineEdit* m_LineRadius;
	QLineEdit* m_ActiveRegionPadding;
	QLineEdit* m_DebugMetricFilePath;
	QPushButton* m_ClearPoints;
	QPushButton* m_EstimateIntensity;
	QPushButton* m_ExecuteButton;

	QWidget* m_EmptyOptions;

	QWidget* m_VesselnessOptions;
	QLineEdit* m_Sigma;
	QCheckBox* m_DarkObjects;
	QLineEdit* m_Alpha;
	QLineEdit* m_Beta;
	QLineEdit* m_Gamma;

	QWidget* m_TargetOptions;
	QLineEdit* m_PathFileName;

	QStackedWidget* m_OptionsStack;

private slots:
	void DoWork();
	void ClearPoints();
	void EstimateIntensity();
	void OnMetricChanged();
};
