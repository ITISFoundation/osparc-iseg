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
	TissueTreeWidget(TissueHiearchy* hierarchy, QDir picpath,
			QWidget* parent = 0);
	~TissueTreeWidget();

public:
	void initialize();

	void set_tissue_filter(const QString& filter);
	bool is_visible(tissues_size_t type) const;
	void update_visibility();
	void update_visibility_recursive(QTreeWidgetItem* item);

	// Add tree items
	void insert_item(bool isFolder, const QString& name);

	// Delete tree items
	void remove_tissue(const QString& name);
	void remove_item(QTreeWidgetItem* currItem, bool removeChildren = false, bool updateRepresentation = true);
	void remove_items(const std::vector<QTreeWidgetItem*>& items);
	void remove_all_folders(bool removeChildren = false);

	// Update items
	void update_tissue_name(const QString& oldName, const QString& newName);
	void update_tissue_icons(QTreeWidgetItem* parent = 0);
	void update_folder_icons(QTreeWidgetItem* parent = 0);
	void set_current_folder_name(const QString& name);

	// Current item
	void set_current_item(QTreeWidgetItem* item);
	void set_current_tissue(tissues_size_t type);
	bool get_current_is_folder() const;
	tissues_size_t get_current_type() const;
	QString get_current_name() const;
	bool get_current_has_children() const;
	void get_current_child_tissues(std::map<tissues_size_t, unsigned short>& types) const;

	void scrollToItem(QTreeWidgetItem* item);

	// Hierarchy
	void update_tree_widget(); // Updates QTreeWidget from internal representation
	void update_hierarchy();	 // Updates internal representation from QTreeWidget
	unsigned short get_selected_hierarchy() const;
	unsigned short get_hierarchy_count() const;
	std::vector<QString>* get_hierarchy_names_ptr() const;
	QString get_current_hierarchy_name() const;
	void reset_default_hierarchy();
	void set_hierarchy(unsigned short index);
	void add_new_hierarchy(const QString& name);
	void remove_current_hierarchy();
	bool get_hierarchy_modified() const;
	void set_hierarchy_modified(bool val);
	unsigned short get_tissue_instance_count(tissues_size_t type) const;
	void get_sublevel_child_tissues(std::map<tissues_size_t, unsigned short>& types) const;

	// File IO
	FILE* SaveParams(FILE* fp, int version);
	FILE* LoadParams(FILE* fp, int version);
	bool load_hierarchy(const QString& path);
	bool save_hierarchy_as(const QString& name, const QString& path);

	// Display
	bool get_tissue_indices_hidden() const;

	std::vector<QTreeWidgetItem*> get_all_items(bool leaves_only = false) const;

	std::vector<QTreeWidgetItem*> collect(const std::vector<QTreeWidgetItem*>& list) const;

public slots:
	void toggle_show_tissue_indices();
	void sort_by_tissue_name();
	void sort_by_tissue_index();
	tissues_size_t get_type(QTreeWidgetItem* item) const;
	QString get_name(QTreeWidgetItem* item) const;

protected:
	// Drag & drop
	void dropEvent(QDropEvent* de) override;
	void selectAll() override;

private:
	void resize_columns_to_contents();
	bool get_is_folder(QTreeWidgetItem* item) const;

	QTreeWidgetItem* find_tissue_item(tissues_size_t type, QTreeWidgetItem* parent = 0) const;
	void take_children_recursively(QTreeWidgetItem* parent, QList<QTreeWidgetItem*>& appendTo);
	void get_child_tissues_recursively(QTreeWidgetItem* parent, std::map<tissues_size_t, unsigned short>& types) const;
	unsigned short get_tissue_instance_count_recursively(QTreeWidgetItem* parent, tissues_size_t type) const;
	void insert_item(bool isFolder, const QString& name, QTreeWidgetItem* insertAbove);
	void insert_item(bool isFolder, const QString& name, QTreeWidgetItem* parent, unsigned int index);
	void remove_tissue_recursively(QTreeWidgetItem* parent, const QString& name);
	void update_tissue_name_widget(const QString& oldName, const QString& newName, QTreeWidgetItem* parent = 0);
	short get_child_lockstates(QTreeWidgetItem* folder);
	void pad_tissue_indices();
	void pad_tissue_indices_recursively(QTreeWidgetItem* parent, unsigned short digits);
	void update_tissue_indices();
	void update_tissue_indices_recursively(QTreeWidgetItem* parent);

	// Convert QTreeWidget to internal representation
	TissueHierarchyItem* create_current_hierarchy();
	void create_hierarchy_recursively(QTreeWidgetItem* parentIn, TissueHierarchyItem* parentOut);
	TissueHierarchyItem* create_hierarchy_item(QTreeWidgetItem* item);

	// Convert internal representation to QTreeWidget
	void build_tree_widget(TissueHierarchyItem* root);
	void build_tree_widget_recursively(TissueHierarchyItem* parentIn, QTreeWidgetItem* parentOut, std::set<tissues_size_t>* tissueTypes);
	QTreeWidgetItem* create_hierarchy_item(bool isFolder, const QString& name);

	// SaveParams
	FILE* save_hierarchy(FILE* fp, unsigned short idx);
	FILE* load_hierarchy(FILE* fp);

private:
	TissueHiearchy* hierarchies;
	std::string tissue_filter;
	QDir picturePath;
	bool modified;
	bool sortByNameAscending;
	bool sortByTypeAscending;

signals:
	void hierarchy_list_changed(void);

private slots:
	void resize_columns_to_contents(QTreeWidgetItem* item);
};

} // namespace iseg
