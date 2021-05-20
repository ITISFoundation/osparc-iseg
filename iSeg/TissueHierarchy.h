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

#include <vector>

#include <QString>

class QXmlStreamWriter;

namespace iseg {

class TissueHierarchyItem
{
public:
	TissueHierarchyItem(bool folderItem, const QString& itemName);
	~TissueHierarchyItem();

	// Accessors
	bool GetIsFolder() const;
	QString GetName();
	TissueHierarchyItem* GetParent();
	unsigned int GetChildCount();
	std::vector<TissueHierarchyItem*>* GetChildren();

	// Modifiers
	void AddChild(TissueHierarchyItem* child);
	void UpdateTissuesRecursively();
	void UpdateTissueNameRecursively(const QString& oldName, const QString& newName);

private:
	void SetParent(TissueHierarchyItem* parentItem);

private:
	bool m_IsFolder;
	QString m_Name;
	TissueHierarchyItem* m_Parent;
	std::vector<TissueHierarchyItem*> m_Children;
};

class TissueHiearchy
{
public:
	TissueHiearchy() { Initialize(); }
	void Initialize();

	TissueHierarchyItem* CreateDefaultHierarchy();

	std::vector<TissueHierarchyItem*>& Hierarchies() { return m_HierarchyTrees; }

	TissueHierarchyItem* SelectedHierarchy();
	void SetSelectedHierarchy(TissueHierarchyItem* h);

	QString& SelectedHierarchyName()
	{
		return m_HierarchyNames.at(m_SelectedHierarchy);
	}

	bool RemoveCurrentHierarchy();

	bool SetHierarchy(unsigned short index);
	unsigned short GetSelectedHierarchy() const;
	unsigned short GetHierarchyCount();
	std::vector<QString>* GetHierarchyNamesPtr();
	QString GetCurrentHierarchyName();
	void ResetDefaultHierarchy();

	void AddNewHierarchy(const QString& name);
	void AddNewHierarchy(const QString& name, TissueHierarchyItem* hierarchy);

	void UpdateHierarchies();

	// File IO
	FILE* SaveParams(FILE* fp, int version);
	FILE* LoadParams(FILE* fp, int version);

	FILE* LoadHierarchy(FILE* fp);
	FILE* SaveHierarchy(FILE* fp, unsigned short idx);

	bool LoadHierarchy(const QString& path);
	bool SaveHierarchyAs(const QString& name, const QString& path);

protected:
	void WriteChildrenRecursively(TissueHierarchyItem* parent, QXmlStreamWriter* xmlStream);

	FILE* SaveHierarchyItem(FILE* fp, TissueHierarchyItem* item);
	FILE* LoadHierarchyItem(FILE* fp, TissueHierarchyItem* parent);

private:
	unsigned short m_SelectedHierarchy;
	std::vector<QString> m_HierarchyNames;
	std::vector<TissueHierarchyItem*> m_HierarchyTrees;
};

} // namespace iseg
