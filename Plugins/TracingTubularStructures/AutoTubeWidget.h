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

#include <qcheckbox.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qspinbox.h>

#include <itkImage.h>

class AutoTubeWidget : public iseg::WidgetInterface
{
	Q_OBJECT
public:
	AutoTubeWidget(iseg::SlicesHandlerInterface* hand3D);
	~AutoTubeWidget() override = default;
	void Init() override;
	void NewLoaded() override;
	void Cleanup() override;
	std::string GetName() override { return std::string("Auto-Tubes"); };
	QIcon GetIcon(QDir picdir) override { return QIcon(picdir.absFilePath(QString("LevelSet.png"))); };

private:
	void OnSlicenrChanged() override;
	void OnMouseClicked(iseg::Point p) override;

	template<class TInput, class TTissue, class TTarget>
	void DoWorkNd(TInput* source, TTissue* tissues, TTarget* target);

	template<class TInput, class TImage>
	typename TImage::Pointer ComputeFeatureImage(TInput* source) const;

	template<class TInput, class TImage>
	typename TImage::Pointer ComputeFeatureImage2d(TInput* source) const;

	iseg::SlicesHandlerInterface* m_Handler3D;

	QLineEdit* m_SigmaLow;
	QLineEdit* m_SigmaHi;
	QLineEdit* m_NumberSigmaLevels;
	QLineEdit* m_Threshold;
	QCheckBox* m_Metric2d;
	QCheckBox* m_NonMaxSuppression;
	QCheckBox* m_Skeletonize;
	QLineEdit* m_MaxRadius;
	QLineEdit* m_MinObjectSize;
	QPushButton* m_SelectObjectsButton;
	QLineEdit* m_SelectedObjects;
	QPushButton* m_ExecuteButton;

	template<typename T>
	struct Cache
	{
		using params_type = std::vector<double>;

		void Store(typename itk::Image<T, 2>::Pointer&, const params_type& p = params_type()) {}
		void Store(typename itk::Image<T, 3>::Pointer& i, const params_type& p = params_type())
		{
			params = p;
			img = i;
		}

		bool Get(typename itk::Image<T, 2>::Pointer&, const params_type& p = params_type()) const { return false; }
		bool Get(typename itk::Image<T, 3>::Pointer& r, const params_type& p = params_type()) const
		{
			if (p == params)
			{
				r = img;
			}
			return r != nullptr;
		}

		params_type params;
		typename itk::Image<T, 3>::Pointer img;
	};

	Cache<float> m_CachedFeatureImage;
	Cache<unsigned char> m_CachedSkeleton;

private slots:
	void SelectObjects();
	void DoWork();
};
