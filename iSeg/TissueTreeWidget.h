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

#include "../Data/Types.h"

#include <qdir.h>
#include <qtreewidget.h>

#include <set>

namespace iseg {

class TissueHierarchyItem;
class TissueHiearchy;

class TissueTreeWidget : public QTreeWidget
{
	Q_OBJECT
public:
	TissueTreeWidget(TissueHiearchy* hierarchy, QDir picpath, QWidget* parent = nullptr);
	~TissueTreeWidget() override;

public:
	void Initialize();

	void SetTissueFilter(const QString& filter);
	bool IsVisible(tissues_size_t type) const;
	void UpdateVisibility();
	void UpdateVisibilityRecursive(QTreeWidgetItem* item);

	// Add tree items
	void InsertItem(bool isFolder, const QString& name);

	// Delete tree items
	void RemoveTissue(const QString& name);
	void RemoveItem(QTreeWidgetItem* currItem, bool removeChildren = false, bool updateRepresentation = true);
	void RemoveItems(const std::vector<QTreeWidgetItem*>& items);
	void RemoveAllFolders(bool removeChildren = false);

	// Update items
	void UpdateTissueName(const QString& oldName, const QString& newName);
	void UpdateTissueIcons(QTreeWidgetItem* parent = nullptr);
	void UpdateFolderIcons(QTreeWidgetItem* parent = nullptr);
	void SetCurrentFolderName(const QString& name);

	// Current item
	void SetCurrentItem(QTreeWidgetItem* item);
	void SetCurrentTissue(tissues_size_t type);
	bool GetCurrentIsFolder() const;
	tissues_size_t GetCurrentType() const;
	QString GetCurrentName() const;
	bool GetCurrentHasChildren() const;
	void GetCurrentChildTissues(std::map<tissues_size_t, unsigned short>& types) const;

	void scrollToItem(QTreeWidgetItem* item);

	// Hierarchy
	void UpdateTreeWidget(); // Updates QTreeWidget from internal representation
	void UpdateHierarchy();	 // Updates internal representation from QTreeWidget
	unsigned short GetSelectedHierarchy() const;
	unsigned short GetHierarchyCount() const;
	std::vector<QString>* GetHierarchyNamesPtr() const;
	QString GetCurrentHierarchyName() const;
	void ResetDefaultHierarchy();
	void SetHierarchy(unsigned short index);
	void AddNewHierarchy(const QString& name);
	void RemoveCurrentHierarchy();
	bool GetHierarchyModified() const;
	void SetHierarchyModified(bool val);
	unsigned short GetTissueInstanceCount(tissues_size_t type) const;
	void GetSublevelChildTissues(std::map<tissues_size_t, unsigned short>& types) const;

	// File IO
	FILE* SaveParams(FILE* fp, int version);
	FILE* LoadParams(FILE* fp, int version);
	bool LoadHierarchy(const QString& path);
	bool SaveHierarchyAs(const QString& name, const QString& path);

	// Display
	bool GetTissueIndicesHidden() const;

	std::vector<QTreeWidgetItem*> GetAllItems(bool leaves_only = false) const;

	std::vector<QTreeWidgetItem*> Collect(const std::vector<QTreeWidgetItem*>& list) const;

	tissues_size_t GetType(const QTreeWidgetItem* item) const;

	QString GetName(const QTreeWidgetItem* item) const;

public slots:
	void ToggleShowTissueIndices();
	void SortByTissueName();
	void SortByTissueIndex();

protected:
	// Drag & drop
	void dropEvent(QDropEvent* de) override;
	void selectAll() override;

private:
	void ResizeColumnsToContents();
	bool GetIsFolder(const QTreeWidgetItem* item) const;

	QTreeWidgetItem* FindTissueItem(tissues_size_t type, QTreeWidgetItem* parent = nullptr) const;
	void TakeChildrenRecursively(QTreeWidgetItem* parent, QList<QTreeWidgetItem*>& appendTo);
	void GetChildTissuesRecursively(QTreeWidgetItem* parent, std::map<tissues_size_t, unsigned short>& types) const;
	unsigned short GetTissueInstanceCountRecursively(QTreeWidgetItem* parent, tissues_size_t type) const;
	void InsertItem(bool isFolder, const QString& name, QTreeWidgetItem* insertAbove);
	void InsertItem(bool isFolder, const QString& name, QTreeWidgetItem* parent, unsigned int index);
	void RemoveTissueRecursively(QTreeWidgetItem* parent, const QString& name);
	void UpdateTissueNameWidget(const QString& oldName, const QString& newName, QTreeWidgetItem* parent = nullptr);
	short GetChildLockstates(QTreeWidgetItem* folder);
	void PadTissueIndices();
	void PadTissueIndicesRecursively(QTreeWidgetItem* parent, unsigned short digits);
	void UpdateTissueIndices();
	void UpdateTissueIndicesRecursively(QTreeWidgetItem* parent);

	// Convert QTreeWidget to internal representation
	TissueHierarchyItem* CreateCurrentHierarchy();
	void CreateHierarchyRecursively(QTreeWidgetItem* parentIn, TissueHierarchyItem* parentOut);
	TissueHierarchyItem* CreateHierarchyItem(QTreeWidgetItem* item);

	// Convert internal representation to QTreeWidget
	void BuildTreeWidget(TissueHierarchyItem* root);
	void BuildTreeWidgetRecursively(TissueHierarchyItem* parentIn, QTreeWidgetItem* parentOut, std::set<tissues_size_t>* tissueTypes);
	QTreeWidgetItem* CreateHierarchyItem(bool isFolder, const QString& name);

	// SaveParams
	FILE* SaveHierarchy(FILE* fp, unsigned short idx);
	FILE* LoadHierarchy(FILE* fp);

private:
	TissueHiearchy* m_Hierarchies;
	std::string m_TissueFilter;
	QDir m_PicturePath;
	bool m_Modified;
	bool m_SortByNameAscending;
	bool m_SortByTypeAscending;

signals:
	void HierarchyListChanged();

private slots:
	void ResizeColumnsToContents(QTreeWidgetItem* item);
};

} // namespace iseg
