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

#include "config.h"

#include "SlicesHandler.h"

#include "vtkMyGDCMPolyDataReader.h"

#include "Data/Property.h"

#include <QDialog>

#include <vector>

namespace iseg {

class RadiotherapyStructureSetImporter : public QDialog
{
	Q_OBJECT
public:
	RadiotherapyStructureSetImporter(QString filename, SlicesHandler* hand3D, QWidget* parent = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~RadiotherapyStructureSetImporter() override;

private:
	void Storeparams();
	void Updatevisibility();
	void SolidChanged(int);
	void NewChanged();
	void IgnoreChanged();
	void OkPressed();

	SlicesHandler* m_Handler3D;

	PropertyInt_ptr m_SbPriority;
	PropertyEnum_ptr m_CbSolids;
	PropertyEnum_ptr m_CbNames;
	PropertyString_ptr m_LeName;
	PropertyBool_ptr m_CbNew;
	PropertyBool_ptr m_CbIgnore;

	PropertyButton_ptr m_PbCancel;
	PropertyButton_ptr m_PbOk;

	gdcmvtk_rtstruct::tissuevec m_Tissues;
	std::vector<bool> m_Vecnew;
	std::vector<bool> m_Vecignore;
	std::vector<tissues_size_t> m_Vectissuenrs;
	std::vector<int> m_Vecpriorities;
	std::vector<std::string> m_Vectissuenames;
	int m_Currentitem;

signals:
	void BeginDatachange(DataSelection& dataSelection, QWidget* sender = nullptr, bool beginUndo = true);
	void EndDatachange(QWidget* sender = nullptr, eEndUndoAction undoAction = iseg::EndUndo);
};

} // namespace iseg
