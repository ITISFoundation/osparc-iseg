/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
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

class TissueHierarchyItem
{
public:
	TissueHierarchyItem(bool folderItem, const QString &itemName);
	~TissueHierarchyItem();

	// Accessors
	bool GetIsFolder();
	QString GetName();
	TissueHierarchyItem *GetParent();
	unsigned int GetChildCount();
	std::vector<TissueHierarchyItem *> *GetChildren();

	// Modifiers
	void AddChild(TissueHierarchyItem *child);
	void UpdateTissuesRecursively();
	void UpdateTissueNameRecursively(const QString &oldName,
																	 const QString &newName);

private:
	void SetParent(TissueHierarchyItem *parentItem);

private:
	bool isFolder;
	QString name;
	TissueHierarchyItem *parent;
	std::vector<TissueHierarchyItem *> children;
};

class TissueHiearchy
{
public:
	TissueHiearchy() { initialize(); }
	void initialize();

	TissueHierarchyItem *create_default_hierarchy();

	std::vector<TissueHierarchyItem *> &hierarchies() { return hierarchyTrees; }

	TissueHierarchyItem *selected_hierarchy();
	void set_selected_hierarchy(TissueHierarchyItem *h);

	QString &selected_hierarchy_name()
	{
		return hierarchyNames.at(selectedHierarchy);
	}

	bool remove_current_hierarchy();

	bool set_hierarchy(unsigned short index);
	unsigned short get_selected_hierarchy();
	unsigned short get_hierarchy_count();
	std::vector<QString> *get_hierarchy_names_ptr();
	QString get_current_hierarchy_name();
	void reset_default_hierarchy();

	void add_new_hierarchy(const QString &name);
	void add_new_hierarchy(const QString &name, TissueHierarchyItem *hierarchy);

	void update_hierarchies();

	// File IO
	FILE *SaveParams(FILE *fp, int version);
	FILE *LoadParams(FILE *fp, int version);

	FILE *load_hierarchy(FILE *fp);
	FILE *save_hierarchy(FILE *fp, unsigned short idx);

	bool load_hierarchy(const QString &path);
	bool save_hierarchy_as(const QString &name, const QString &path);

protected:
	void write_children_recursively(TissueHierarchyItem *parent,
																	QXmlStreamWriter *xmlStream);

	FILE *save_hierarchy_item(FILE *fp, TissueHierarchyItem *item);
	FILE *load_hierarchy_item(FILE *fp, TissueHierarchyItem *parent);

private:
	unsigned short selectedHierarchy;
	std::vector<QString> hierarchyNames;
	std::vector<TissueHierarchyItem *> hierarchyTrees;
};