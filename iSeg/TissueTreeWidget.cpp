/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "Precompiled.h"

#include "StdStringToQString.h"
#include "TissueHierarchy.h"
#include "TissueInfos.h"
#include "TissueTreeWidget.h"

#include <QDropEvent>

#include <boost/algorithm/string.hpp>

#define TISSUETREEWIDGET_COLUMN_COUNT 2
#define TISSUETREEWIDGET_COLUMN_NAME 0
#define TISSUETREEWIDGET_COLUMN_TYPE 1
#define TISSUETREEWIDGET_COLUMN_FOLDER 2

namespace iseg {

namespace {
inline bool SearchFilter(const std::string& text, const std::string& filter)
{
	namespace algo = boost::algorithm;

	if (!filter.empty())
	{
		auto search_text = filter;
		algo::trim(search_text);

		std::vector<std::string> search_tokens;
		algo::split(search_tokens, search_text, algo::is_space(),
				algo::token_compress_on);

		for (auto tok : search_tokens)
		{
			if (!tok.empty() && !algo::icontains(text, tok))
			{
				return false;
			}
		}
	}
	return true;
}
} // namespace

TissueTreeWidget::TissueTreeWidget(TissueHiearchy* hierarchy, QDir picpath,
		QWidget* parent)
		: QTreeWidget(parent), hierarchies(hierarchy)
{
	picturePath = picpath;
	sortByNameAscending = true;
	sortByTypeAscending = true;

	setColumnCount(TISSUETREEWIDGET_COLUMN_COUNT);
	hideColumn(TISSUETREEWIDGET_COLUMN_TYPE);
	hideColumn(TISSUETREEWIDGET_COLUMN_FOLDER);
	setAcceptDrops(true);
	setDragDropMode(QTreeWidget::InternalMove);
	setExpandsOnDoubleClick(false);
	setHeaderHidden(true);
	//setIndentation(20);
	setItemsExpandable(true);
	setRootIsDecorated(true);
	//xxxb	setFixedHeight(250);
	//xxxb	setFixedWidth(110);
	QObject::connect(this, SIGNAL(itemExpanded(QTreeWidgetItem*)), this,
			SLOT(resize_columns_to_contents(QTreeWidgetItem*)));
	QObject::connect(this, SIGNAL(itemCollapsed(QTreeWidgetItem*)), this,
			SLOT(resize_columns_to_contents(QTreeWidgetItem*)));
	initialize();
}

TissueTreeWidget::~TissueTreeWidget() {}

void TissueTreeWidget::initialize()
{
	// Clear internal representations
	hierarchies->initialize();

	emit hierarchy_list_changed();
}

void TissueTreeWidget::set_tissue_filter(const QString& filter)
{
	if (filter.toStdString() != tissue_filter)
	{
		tissue_filter = filter.toStdString();

		// set visibility on items
		update_visibility();
	}
}

bool TissueTreeWidget::is_visible(tissues_size_t type) const
{
	if (type > 0)
	{
		if (auto item = find_tissue_item(type))
		{
			return !item->isHidden();
		}
	}
	return false;
}

void TissueTreeWidget::update_visibility()
{
	for (int i = 0; i < topLevelItemCount(); ++i)
	{
		update_visibility_recursive(topLevelItem(i));
	}
}

void TissueTreeWidget::update_visibility_recursive(QTreeWidgetItem* current)
{
	// setHidden hides/shows recursively
	bool const matches = SearchFilter(get_name(current).toStdString(), tissue_filter);
	bool const is_folder = get_is_folder(current);
	if (matches || !is_folder)
	{
		current->setHidden(!matches);
		return;
	}

	// first make all visible
	if (get_is_folder(current))
	{
		current->setHidden(false);
	}

	// filter children
	bool any_child_visible = false;
	for (int i = 0; i < current->childCount(); ++i)
	{
		QTreeWidgetItem* child = current->child(i);
		update_visibility_recursive(child);

		if (!child->isHidden())
		{
			any_child_visible = true;
		}
	}

	// hide empty folders
	if (!any_child_visible)
	{
		current->setHidden(true);
	}
}

void TissueTreeWidget::update_hierarchy()
{
	modified = true;
	hierarchies->set_selected_hierarchy(create_current_hierarchy());
}

TissueHierarchyItem* TissueTreeWidget::create_current_hierarchy()
{
	// Create internal representation from current QTreeWidget
	TissueHierarchyItem* root = new TissueHierarchyItem(true, QString("root"));

	for (int i = 0; i < topLevelItemCount(); ++i)
	{
		// Add top-level child
		QTreeWidgetItem* currWidgetItem = topLevelItem(i);
		TissueHierarchyItem* newTreeItem = create_hierarchy_item(currWidgetItem);
		root->AddChild(newTreeItem);

		// Subtree of current child
		if (newTreeItem->GetIsFolder())
		{
			create_hierarchy_recursively(currWidgetItem, newTreeItem);
		}
	}

	return root;
}

void TissueTreeWidget::create_hierarchy_recursively(
		QTreeWidgetItem* parentIn, TissueHierarchyItem* parentOut)
{
	for (int i = 0; i < parentIn->childCount(); ++i)
	{
		// Add child
		QTreeWidgetItem* currWidgetItem = parentIn->child(i);
		TissueHierarchyItem* newTreeItem = create_hierarchy_item(currWidgetItem);
		parentOut->AddChild(newTreeItem);

		// Subtree of current child
		if (newTreeItem->GetIsFolder())
		{
			create_hierarchy_recursively(currWidgetItem, newTreeItem);
		}
	}
}

TissueHierarchyItem* TissueTreeWidget::create_hierarchy_item(QTreeWidgetItem* item)
{
	return new TissueHierarchyItem(get_is_folder(item), item->text(TISSUETREEWIDGET_COLUMN_NAME));
}

namespace {

QPixmap generatePixmap(tissues_size_t tissuenr)
{
	QPixmap abc(10, 10);
	if (tissuenr > TissueInfos::GetTissueCount())
	{
		abc.fill(QColor(0, 0, 0));
		return abc;
	}
	unsigned char r, g, b;
	TissueInfo* tissueInfo = TissueInfos::GetTissueInfo(tissuenr);
	std::tie(r, g, b) = tissueInfo->color.toUChar();
	abc.fill(QColor(r, g, b));
	if (tissueInfo->locked)
	{
		QPainter painter(&abc);
		float r, g, b;
		r = (tissueInfo->color[0] + 0.5f);
		if (r >= 1.0f)
			r = r - 1.0f;
		g = (tissueInfo->color[1] + 0.5f);
		if (g >= 1.0f)
			g = g - 1.0f;
		b = (tissueInfo->color[2] + 0.5f);
		if (b >= 1.0f)
			b = b - 1.0f;
		painter.setPen(QColor(int(r * 255), int(g * 255), int(b * 255)));
		painter.drawLine(0, 0, 9, 9);
		painter.drawLine(0, 9, 9, 0);
		painter.drawRect(0, 0, 9, 9);
	}

	return abc;
}

} // namespace

QTreeWidgetItem* TissueTreeWidget::create_hierarchy_item(bool isFolder, const QString& name)
{
	if (isFolder)
	{
		QTreeWidgetItem* newFolder = new QTreeWidgetItem();
		newFolder->setIcon(TISSUETREEWIDGET_COLUMN_NAME, QIcon(picturePath.absFilePath(QString("fileopen.png")).ascii()));
		newFolder->setText(TISSUETREEWIDGET_COLUMN_NAME, name);
		newFolder->setText(TISSUETREEWIDGET_COLUMN_FOLDER, QString::number(isFolder));
		newFolder->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
		newFolder->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
		return newFolder;
	}
	else
	{
		tissues_size_t type = TissueInfos::GetTissueType(ToStd(name));
		QTreeWidgetItem* newTissue = new QTreeWidgetItem();
		newTissue->setIcon(TISSUETREEWIDGET_COLUMN_NAME, generatePixmap(type));
		newTissue->setText(TISSUETREEWIDGET_COLUMN_NAME, name);
		newTissue->setText(TISSUETREEWIDGET_COLUMN_TYPE, QString::number(type));
		newTissue->setText(TISSUETREEWIDGET_COLUMN_FOLDER,
				QString::number(isFolder));
		newTissue->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicator);
		newTissue->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled |
												Qt::ItemIsDragEnabled);
		return newTissue;
	}
}

void TissueTreeWidget::insert_item(bool isFolder, const QString& name)
{
	QTreeWidgetItem* newItem = create_hierarchy_item(isFolder, name);

	if (QTreeWidgetItem* currItem = currentItem())
	{
		if (get_is_folder(currItem) && currItem->isExpanded())
		{
			// Insert as first child of current folder
			currItem->insertChild(0, newItem);
		}
		else
		{
			// Insert at current position
			QTreeWidgetItem* currParent = currItem->parent();
			if (currParent == 0)
			{
				insertTopLevelItem(indexOfTopLevelItem(currItem), newItem);
			}
			else
			{
				currParent->insertChild(currParent->indexOfChild(currItem), newItem);
			}
		}
	}
	else // empty tissue list
	{
		invisibleRootItem()->insertChild(0, newItem);
	}

	setCurrentItem(newItem);

	// Update internal representation
	update_hierarchy();
}

void TissueTreeWidget::insert_item(bool isFolder, const QString& name,
		QTreeWidgetItem* insertAbove)
{
	if (insertAbove == 0)
	{
		return;
	}

	QTreeWidgetItem* newItem = create_hierarchy_item(isFolder, name);

	QTreeWidgetItem* parent = insertAbove->parent();
	if (parent == 0)
	{
		insertTopLevelItem(indexOfTopLevelItem(insertAbove), newItem);
	}
	else
	{
		parent->insertChild(parent->indexOfChild(insertAbove), newItem);
	}

	setCurrentItem(newItem);

	// Update internal representation
	update_hierarchy();
}

void TissueTreeWidget::insert_item(bool isFolder, const QString& name,
		QTreeWidgetItem* parent, unsigned int index)
{
	QTreeWidgetItem* newItem = create_hierarchy_item(isFolder, name);

	if (parent == 0)
	{
		insertTopLevelItem(index, newItem);
	}
	else
	{
		parent->insertChild(index, newItem);
	}

	setCurrentItem(newItem);

	// Update internal representation
	update_hierarchy();
}

void TissueTreeWidget::remove_tissue(const QString& name)
{
	// Removes a tissue completely from all hierarchies
	blockSignals(true);
	for (int i = 0; i < topLevelItemCount(); ++i)
	{
		QTreeWidgetItem* currItem = topLevelItem(i);
		if (get_is_folder(currItem))
		{
			remove_tissue_recursively(currItem, name);
		}
		else if (get_name(currItem).compare(name) == 0)
		{
			// Delete tissue item
			takeTopLevelItem(i);
			delete currItem;
			--i;
		}
		else
		{
			// Update tissue type
			tissues_size_t newType = TissueInfos::GetTissueType(ToStd(currItem->text(TISSUETREEWIDGET_COLUMN_NAME)));
			currItem->setText(TISSUETREEWIDGET_COLUMN_TYPE, QString::number(newType));
		}
	}
	blockSignals(false);

	// Update internal representation
	update_hierarchy();

	// Propagate tissue removal to other hierarchies
	hierarchies->update_hierarchies();
}

void TissueTreeWidget::remove_tissue_recursively(QTreeWidgetItem* parent, const QString& name)
{
	for (int i = 0; i < parent->childCount(); ++i)
	{
		QTreeWidgetItem* currItem = parent->child(i);
		if (get_is_folder(currItem))
		{
			remove_tissue_recursively(currItem, name);
		}
		else if (get_name(currItem).compare(name) == 0)
		{
			// Delete tissue item
			parent->takeChild(i);
			delete currItem;
			--i;
		}
		else
		{
			// Update tissue type
			tissues_size_t newType = TissueInfos::GetTissueType(ToStd(currItem->text(TISSUETREEWIDGET_COLUMN_NAME)));
			currItem->setText(TISSUETREEWIDGET_COLUMN_TYPE, QString::number(newType));
		}
	}
}

void TissueTreeWidget::remove_items(const std::vector<QTreeWidgetItem*>& items)
{
	std::set<QTreeWidgetItem*> deleted;
	for (auto currItem : items)
	{
		if (deleted.count(currItem) == 0)
		{
			remove_item(currItem, false, false);
			deleted.insert(currItem);
		}
		else
		{
			ISEG_WARNING("trying to delete item again");
		}
	}

	// Update internal representation
	update_hierarchy();

	// Propagate tissue removal to other hierarchies
	hierarchies->update_hierarchies();

	// Update tissue indices
	update_tissue_indices();
}

void TissueTreeWidget::remove_item(QTreeWidgetItem* currItem, bool removeChildren, bool updateRepresentation)
{
	// Removes item in QTreeWidget and internal representations
	QTreeWidgetItem* currParent = currItem->parent();
	bool updateTissues = false;

	if (get_is_folder(currItem))
	{
		// Delete folder
		if (removeChildren)
		{
			// Delete all children of current item
			QList<QTreeWidgetItem*> children;
			take_children_recursively(currItem, children);
			qDeleteAll(children);

			// Delete current item
			if (currParent == 0)
			{
				takeTopLevelItem(indexOfTopLevelItem(currItem));
				delete currItem;
			}
			else
			{
				currParent->removeChild(currItem);
				delete currItem;
			}

			updateTissues = true;
		}
		else
		{
			// Insert children into parent and delete current item
			if (currParent == 0)
			{
				insertTopLevelItems(indexOfTopLevelItem(currItem), currItem->takeChildren());
				takeTopLevelItem(indexOfTopLevelItem(currItem));
				delete currItem;
			}
			else
			{
				currParent->insertChildren(currParent->indexOfChild(currItem), currItem->takeChildren());
				currParent->removeChild(currItem);
				delete currItem;
			}
		}
	}
	else
	{
		// Delete tissue
		if (currParent == 0)
		{
			takeTopLevelItem(indexOfTopLevelItem(currItem));
			delete currItem;
		}
		else
		{
			currParent->removeChild(currItem);
			delete currItem;
		}

		updateTissues = true;
	}

	// Update internal representation
	if (updateRepresentation)
	{
		update_hierarchy();

		// Propagate tissue removal to other hierarchies
		if (updateTissues)
		{
			hierarchies->update_hierarchies();

			// Update tissue indices
			update_tissue_indices();
		}
	}
}

void TissueTreeWidget::remove_all_folders(bool removeChildren)
{
	// Removes all folders in QTreeWidget and internal representations

	bool updateTissues = false;
	blockSignals(true);
	if (removeChildren)
	{
		for (int i = 0; i < topLevelItemCount(); ++i)
		{
			QTreeWidgetItem* item = topLevelItem(i);
			if (get_is_folder(item))
			{
				// Delete all children of item
				QList<QTreeWidgetItem*> children;
				take_children_recursively(item, children);
				qDeleteAll(children);

				// Delete item
				takeTopLevelItem(i);
				delete item;
				updateTissues = true;
				--i;
			}
		}
	}
	else
	{
		for (int i = 0; i < topLevelItemCount(); ++i)
		{
			QTreeWidgetItem* item = topLevelItem(i);
			if (get_is_folder(item))
			{
				// Insert children into parent and delete item
				insertTopLevelItems(i, item->takeChildren());
				takeTopLevelItem(indexOfTopLevelItem(item));
				delete item;
				--i;
			}
		}
	}
	blockSignals(false);

	// Update internal representation
	update_hierarchy();

	// Propagate tissue removal to other hierarchies
	if (updateTissues)
	{
		hierarchies->update_hierarchies();
	}
}

void TissueTreeWidget::update_tissue_name(const QString& oldName,
		const QString& newName)
{
	if (oldName.compare(newName) == 0)
	{
		return;
	}

	// Update tissue name in internal representations
	auto& hierarchyTrees = hierarchies->hierarchies();
	for (unsigned int i = 0; i < hierarchyTrees.size(); ++i)
	{
		hierarchyTrees[i]->UpdateTissueNameRecursively(oldName, newName);
	}

	// Update tissue name in tree widget
	update_tissue_name_widget(oldName, newName);
}

void TissueTreeWidget::update_tissue_name_widget(const QString& oldName,
		const QString& newName,
		QTreeWidgetItem* parent)
{
	if (parent == 0)
	{
		// Recursion with top level children
		for (int i = 0; i < topLevelItemCount(); ++i)
		{
			update_tissue_name_widget(oldName, newName, topLevelItem(i));
		}
	}
	else
	{
		if (get_is_folder(parent))
		{
			// Recursion with children
			for (int i = 0; i < parent->childCount(); ++i)
			{
				update_tissue_name_widget(oldName, newName, parent->child(i));
			}
		}
		else
		{
			// Update tissue name
			if (parent->text(TISSUETREEWIDGET_COLUMN_NAME).compare(oldName) ==
					0)
			{
				parent->setText(TISSUETREEWIDGET_COLUMN_NAME, newName);
			}
		}
	}
}

void TissueTreeWidget::update_tissue_icons(QTreeWidgetItem* parent)
{
	if (parent == 0)
	{
		// Recursion with top level children
		for (int i = 0; i < topLevelItemCount(); ++i)
		{
			update_tissue_icons(topLevelItem(i));
		}
	}
	else
	{
		if (get_is_folder(parent))
		{
			// Recursion with children
			for (int i = 0; i < parent->childCount(); ++i)
			{
				update_tissue_icons(parent->child(i));
			}
		}
		else
		{
			// Update tissue icon
			parent->setIcon(TISSUETREEWIDGET_COLUMN_NAME,
					generatePixmap(get_type(parent)));
		}
	}
}

void TissueTreeWidget::update_folder_icons(QTreeWidgetItem* parent)
{
	// Updates the folder icons based on the lock state of the child tissues
	// This only works if the tree widget is completely built

	if (parent == 0)
	{
		// Recursion with top level children
		for (int i = 0; i < topLevelItemCount(); ++i)
		{
			update_folder_icons(topLevelItem(i));
		}
	}
	else
	{
		if (get_is_folder(parent))
		{
			// Update folder icon
			short lockStates = get_child_lockstates(parent);
			if (lockStates == 0)
			{
				// All child tissues unlocked
				parent->setIcon(
						TISSUETREEWIDGET_COLUMN_NAME,
						QIcon(picturePath.absFilePath(QString("fileopen.png"))
											.ascii()));
			}
			else if (lockStates == 1)
			{
				// All child tissues locked
				parent->setIcon(
						TISSUETREEWIDGET_COLUMN_NAME,
						QIcon(picturePath.absFilePath(QString("folderlock1.png"))
											.ascii()));
			}
			else
			{
				// Mixed locked/unlocked
				parent->setIcon(
						TISSUETREEWIDGET_COLUMN_NAME,
						QIcon(picturePath.absFilePath(QString("folderlock2.png"))
											.ascii()));
			}

			// Recursion with children
			for (int i = 0; i < parent->childCount(); ++i)
			{
				update_folder_icons(parent->child(i));
			}
		}
	}
}

// TODO: Optimize by introducing folder map / hidden flag
short TissueTreeWidget::get_child_lockstates(QTreeWidgetItem* folder)
{
	// Returns whether all child tissues (including subfolders) are
	// unlocked (return value 0),
	// locked (return value 1), or
	// mixed locked and unlocked (-1)

	// Get lock state of first child (non-empty folder or tissue)
	int i;
	short lockStates = 0; // Empty folders are considered to be unlocked
	for (i = 0; i < folder->childCount(); ++i)
	{
		QTreeWidgetItem* currChild = folder->child(i);
		if (get_is_folder(currChild))
		{
			if (currChild->childCount() > 0)
			{
				lockStates = get_child_lockstates(currChild);
			}
			else
			{
				continue;
			}
		}
		else if (TissueInfos::GetTissueLocked(get_type(currChild)))
		{
			lockStates = 1;
			break;
		}
		else
		{
			lockStates = 0;
			break;
		}
	}
	if (lockStates == -1)
	{
		return -1;
	}

	// Test against lock states of other children
	for (int i = 1; i < folder->childCount(); ++i)
	{
		QTreeWidgetItem* currChild = folder->child(i);
		if (get_is_folder(currChild))
		{
			// Skip empty folders
			if (currChild->childCount() > 0 &&
					(lockStates != get_child_lockstates(currChild)))
			{
				return -1;
			}
		}
		else if ((bool)lockStates !=
						 TissueInfos::GetTissueLocked(get_type(currChild)))
		{
			return -1;
		}
	}
	return lockStates;
}

void TissueTreeWidget::pad_tissue_indices()
{
	// Get maximal number of digits
	unsigned short digits = 0;
	tissues_size_t tmp = (TissueInfos::GetTissueCount() + 1);
	while (tmp > 0)
	{
		tmp = (tissues_size_t)(tmp / 10);
		digits++;
	}

	for (int i = 0; i < topLevelItemCount(); ++i)
	{
		QTreeWidgetItem* item = topLevelItem(i);
		if (get_is_folder(item))
		{
			pad_tissue_indices_recursively(item, digits);
		}
		else
		{
			// Pad with leading zeroes
			QString paddedText = item->text(TISSUETREEWIDGET_COLUMN_TYPE);
			while (paddedText.length() < digits)
			{
				paddedText = paddedText.prepend('0');
			}
			item->setText(TISSUETREEWIDGET_COLUMN_TYPE, paddedText);
		}
	}
}

void TissueTreeWidget::pad_tissue_indices_recursively(QTreeWidgetItem* parent,
		unsigned short digits)
{
	for (int i = 0; i < parent->childCount(); ++i)
	{
		QTreeWidgetItem* item = parent->child(i);
		if (get_is_folder(item))
		{
			pad_tissue_indices_recursively(item, digits);
		}
		else
		{
			// Pad with leading zeroes
			QString paddedText = item->text(TISSUETREEWIDGET_COLUMN_TYPE);
			while (paddedText.length() < digits)
			{
				paddedText = paddedText.prepend('0');
			}
			item->setText(TISSUETREEWIDGET_COLUMN_TYPE, paddedText);
		}
	}
}

void TissueTreeWidget::update_tissue_indices()
{
	for (int i = 0; i < topLevelItemCount(); ++i)
	{
		QTreeWidgetItem* item = topLevelItem(i);
		if (get_is_folder(item))
		{
			update_tissue_indices_recursively(item);
		}
		else
		{
			tissues_size_t type = TissueInfos::GetTissueType(ToStd(item->text(TISSUETREEWIDGET_COLUMN_NAME)));
			item->setText(TISSUETREEWIDGET_COLUMN_TYPE, QString::number(type));
		}
	}
}

void TissueTreeWidget::update_tissue_indices_recursively(
		QTreeWidgetItem* parent)
{
	for (int i = 0; i < parent->childCount(); ++i)
	{
		QTreeWidgetItem* item = parent->child(i);
		if (get_is_folder(item))
		{
			update_tissue_indices_recursively(item);
		}
		else
		{
			tissues_size_t type = TissueInfos::GetTissueType(ToStd(item->text(TISSUETREEWIDGET_COLUMN_NAME)));
			item->setText(TISSUETREEWIDGET_COLUMN_TYPE, QString::number(type));
		}
	}
}

void TissueTreeWidget::set_current_folder_name(const QString& name)
{
	if (get_current_is_folder())
	{
		currentItem()->setText(TISSUETREEWIDGET_COLUMN_NAME, name);

		// Update internal representation
		update_hierarchy();
	}
}

void TissueTreeWidget::set_current_item(QTreeWidgetItem* item)
{
	if (item)
	{
		setCurrentItem(item);
	}
	else
	{
		setCurrentItem(topLevelItem(0));
	}
}

void TissueTreeWidget::set_current_tissue(tissues_size_t type)
{
	if (type > 0 && type <= TissueInfos::GetTissueCount())
	{
		QTreeWidgetItem* item = find_tissue_item(type);
		setCurrentItem(item);
		QTreeWidgetItem* parent = item->parent();
		if (parent != 0)
		{
			parent->setExpanded(true);
		}
	}
	else if (topLevelItemCount() > 0 && type > 0)
	{
		setCurrentItem(topLevelItem(0));
	}
}

QTreeWidgetItem* TissueTreeWidget::find_tissue_item(tissues_size_t type, QTreeWidgetItem* parent) const
{
	if (parent == 0)
	{
		// Recursion with top level children
		for (int i = 0; i < topLevelItemCount(); ++i)
		{
			QTreeWidgetItem* recursiveRes = find_tissue_item(type, topLevelItem(i));
			if (recursiveRes != 0)
			{
				return recursiveRes;
			}
		}
	}
	else
	{
		// Check if parent is searched item
		if (type == get_type(parent))
		{
			return parent;
		}
		// Recursion with children
		for (int i = 0; i < parent->childCount(); ++i)
		{
			QTreeWidgetItem* recursiveRes = find_tissue_item(type, parent->child(i));
			if (recursiveRes != 0)
			{
				return recursiveRes;
			}
		}
	}
	return (QTreeWidgetItem*)0;
}

tissues_size_t TissueTreeWidget::get_current_type() const
{
	return get_type(currentItem());
}

QString TissueTreeWidget::get_current_name() const { return get_name(currentItem()); }

bool TissueTreeWidget::get_current_is_folder() const
{
	return get_is_folder(currentItem());
}

bool TissueTreeWidget::get_is_folder(QTreeWidgetItem* item) const
{
	if (item)
	{
		return item->text(TISSUETREEWIDGET_COLUMN_FOLDER).toUShort();
	}
	return true;
}

tissues_size_t TissueTreeWidget::get_type(QTreeWidgetItem* item) const
{
	if (item && !get_is_folder(item))
	{
		return (tissues_size_t)item->text(TISSUETREEWIDGET_COLUMN_TYPE)
				.toUInt();
	}
	return 0;
}

QString TissueTreeWidget::get_name(QTreeWidgetItem* item) const
{
	if (item)
	{
		return item->text(TISSUETREEWIDGET_COLUMN_NAME);
	}
	return "";
}

bool TissueTreeWidget::get_current_has_children() const
{
	if (currentItem() == 0)
	{
		return false;
	}
	else
	{
		return currentItem()->childCount() > 0;
	}
}

void TissueTreeWidget::get_current_child_tissues(std::map<tissues_size_t, unsigned short>& types) const
{
	types.clear();
	if (currentItem() != 0)
	{
		get_child_tissues_recursively(currentItem(), types);
	}
}

void TissueTreeWidget::get_sublevel_child_tissues(std::map<tissues_size_t, unsigned short>& types) const
{
	types.clear();
	for (int i = 0; i < topLevelItemCount(); ++i)
	{
		QTreeWidgetItem* item = topLevelItem(i);
		if (get_is_folder(item))
		{
			get_child_tissues_recursively(item, types);
		}
	}
}

void TissueTreeWidget::get_child_tissues_recursively(QTreeWidgetItem* parent, std::map<tissues_size_t, unsigned short>& types) const
{
	for (int i = 0; i < parent->childCount(); ++i)
	{
		tissues_size_t currType = (tissues_size_t)parent->child(i)
																	->text(TISSUETREEWIDGET_COLUMN_TYPE)
																	.toUInt();
		if (currType > 0)
		{
			// Insert tissue
			if (types.find(currType) != types.end())
			{
				types[currType]++;
			}
			else
			{
				types[currType] = 1;
			}
		}
		else
		{
			// Get tissues in subfolder
			get_child_tissues_recursively(parent->child(i), types);
		}
	}
}

unsigned short TissueTreeWidget::get_selected_hierarchy() const
{
	return hierarchies->get_selected_hierarchy();
}

unsigned short TissueTreeWidget::get_hierarchy_count() const
{
	return hierarchies->get_hierarchy_count();
}

std::vector<QString>* TissueTreeWidget::get_hierarchy_names_ptr() const
{
	return hierarchies->get_hierarchy_names_ptr();
}

QString TissueTreeWidget::get_current_hierarchy_name() const
{
	return hierarchies->get_current_hierarchy_name();
}

void TissueTreeWidget::reset_default_hierarchy()
{
	hierarchies->reset_default_hierarchy();
}

void TissueTreeWidget::set_hierarchy(unsigned short index)
{
	if (!hierarchies->set_hierarchy(index))
	{
		return;
	}

	tissues_size_t currentType = get_current_type();

	// Build QTreeWidgetItems from TissueTreeItems
	blockSignals(true);
	build_tree_widget(hierarchies->selected_hierarchy());
	blockSignals(false);

	// Restore item selection
	if (currentType > 0)
	{
		set_current_tissue(currentType);
	}
	else
	{
		setCurrentItem(topLevelItem(0));
	}

	// Resize columns
	resize_columns_to_contents();
}

void TissueTreeWidget::build_tree_widget(TissueHierarchyItem* root)
{
	clear();

	// Create pool of existing tissues
	std::set<tissues_size_t> tissueTypes;
	for (tissues_size_t type = 1; type <= TissueInfos::GetTissueCount(); ++type)
	{
		tissueTypes.insert(type);
	}

	std::vector<TissueHierarchyItem*>* children = root->GetChildren();
	for (auto iter = children->begin(); iter != children->end(); ++iter)
	{
		// Add top-level item
		TissueHierarchyItem* currItem = *iter;
		if (currItem->GetIsFolder())
		{
			QTreeWidgetItem* newFolder = create_hierarchy_item(true, currItem->GetName());
			addTopLevelItem(newFolder);
			build_tree_widget_recursively(currItem, newFolder, &tissueTypes);
		}
		else
		{
			tissues_size_t type = TissueInfos::GetTissueType(ToStd(currItem->GetName()));
			if (type > 0)
			{
				QTreeWidgetItem* newTissue =
						create_hierarchy_item(false, currItem->GetName());
				addTopLevelItem(newTissue);
				std::set<tissues_size_t>::iterator iter =
						tissueTypes.find(type);
				if (iter != tissueTypes.end())
				{
					tissueTypes.erase(iter);
				}
			}
		}
	}

	// Add all missing tissues at top level
	for (auto iter = tissueTypes.begin(); iter != tissueTypes.end(); ++iter)
	{
		addTopLevelItem(create_hierarchy_item(false, ToQ(TissueInfos::GetTissueName(*iter))));
	}

	// Update folder icons
	update_folder_icons();

	modified = false;
}

void TissueTreeWidget::build_tree_widget_recursively(
		TissueHierarchyItem* parentIn, QTreeWidgetItem* parentOut,
		std::set<tissues_size_t>* tissueTypes)
{
	std::vector<TissueHierarchyItem*>* children = parentIn->GetChildren();
	for (auto iter = children->begin(); iter != children->end(); ++iter)
	{
		// Add item to parentOut
		TissueHierarchyItem* currItem = *iter;
		if (currItem->GetIsFolder())
		{
			QTreeWidgetItem* newFolder =
					create_hierarchy_item(true, currItem->GetName());
			parentOut->addChild(newFolder);
			build_tree_widget_recursively(currItem, newFolder, tissueTypes);
		}
		else
		{
			tissues_size_t type = TissueInfos::GetTissueType(ToStd(currItem->GetName()));
			if (type > 0)
			{
				QTreeWidgetItem* newTissue = create_hierarchy_item(false, currItem->GetName());
				parentOut->addChild(newTissue);
				auto iter = tissueTypes->find(type);
				if (iter != tissueTypes->end())
				{
					tissueTypes->erase(iter);
				}
			}
		}
	}
}

void TissueTreeWidget::add_new_hierarchy(const QString& name)
{
	// Create and select default hierarchy
	hierarchies->add_new_hierarchy(name);

	emit hierarchy_list_changed();
}

bool TissueTreeWidget::load_hierarchy(const QString& path)
{
	if (!hierarchies->load_hierarchy(path))
	{
		return false;
	}

	emit hierarchy_list_changed();

	return true;
}

void TissueTreeWidget::take_children_recursively(
		QTreeWidgetItem* parent, QList<QTreeWidgetItem*>& appendTo)
{
	// Recursion with children
	for (int i = 0; i < parent->childCount(); ++i)
	{
		take_children_recursively(parent->child(i), appendTo);
	}
	// Append children to list
	appendTo.append(parent->takeChildren());
}

void TissueTreeWidget::update_tree_widget()
{
	reset_default_hierarchy();
	set_hierarchy(hierarchies->get_selected_hierarchy());
}

bool TissueTreeWidget::save_hierarchy_as(const QString& name,
		const QString& path)
{
	if (hierarchies->get_selected_hierarchy() == 0)
	{
		// Do not override default hierarchy: Append to list
		hierarchies->add_new_hierarchy(name, create_current_hierarchy());

		// Reset default hierarchy
		hierarchies->reset_default_hierarchy();
		emit hierarchy_list_changed();
	}
	else
	{
		// Update hierarchy name and internal representation
		hierarchies->selected_hierarchy_name() = name;
		emit hierarchy_list_changed();
		update_hierarchy();
	}

	modified = false;

	return hierarchies->save_hierarchy_as(name, path);
}

void TissueTreeWidget::remove_current_hierarchy()
{
	if (hierarchies->remove_current_hierarchy())
	{
		modified = true;
	}

	// \bug it seems the folders are not refreshed properly
	set_hierarchy(hierarchies->get_selected_hierarchy());

	emit hierarchy_list_changed();
}

void TissueTreeWidget::toggle_show_tissue_indices()
{
	setColumnHidden(TISSUETREEWIDGET_COLUMN_TYPE,
			!isColumnHidden(TISSUETREEWIDGET_COLUMN_TYPE));
	resize_columns_to_contents();
}

void TissueTreeWidget::sort_by_tissue_name()
{
	if (sortByNameAscending)
	{
		sortItems(TISSUETREEWIDGET_COLUMN_NAME, Qt::AscendingOrder);
	}
	else
	{
		sortItems(TISSUETREEWIDGET_COLUMN_NAME, Qt::DescendingOrder);
	}
	sortByNameAscending = !sortByNameAscending;

	// Update internal representation
	update_hierarchy();
}

void TissueTreeWidget::sort_by_tissue_index()
{
	// Pad tissue indices with zeroes because of lexical sorting
	pad_tissue_indices();

	if (sortByTypeAscending)
	{
		sortItems(TISSUETREEWIDGET_COLUMN_TYPE, Qt::AscendingOrder);
	}
	else
	{
		sortItems(TISSUETREEWIDGET_COLUMN_TYPE, Qt::DescendingOrder);
	}
	sortByTypeAscending = !sortByTypeAscending;

	// Update tissue indices to undo padding
	update_tissue_indices();

	// Update internal representation
	update_hierarchy();
}

bool TissueTreeWidget::get_tissue_indices_hidden() const
{
	return isColumnHidden(TISSUETREEWIDGET_COLUMN_TYPE);
}

std::vector<QTreeWidgetItem*> TissueTreeWidget::collect(const std::vector<QTreeWidgetItem*>& list) const
{
	std::vector<QTreeWidgetItem*> my_children;

	for (auto item : list)
	{
		if (item != invisibleRootItem())
		{
			my_children.push_back(item);
		}

		for (int i = 0; i < item->childCount(); ++i)
		{
			auto extra = collect({item->child(i)});
			my_children.insert(my_children.end(), extra.begin(), extra.end());
		}
	}

	return my_children;
}

std::vector<QTreeWidgetItem*> TissueTreeWidget::get_all_items(bool leaves_only) const
{
	auto all = collect({invisibleRootItem()});
	if (leaves_only)
	{
		std::vector<QTreeWidgetItem*> leaves;
		std::copy_if(all.begin(), all.end(), std::back_inserter(leaves),
				[this](QTreeWidgetItem* item) { return !get_is_folder(item); });
		return leaves;
	}
	return all;
}

void TissueTreeWidget::resize_columns_to_contents()
{
	for (int col = 0; col < TISSUETREEWIDGET_COLUMN_COUNT; ++col)
	{
		resizeColumnToContents(col);
	}
}

void TissueTreeWidget::resize_columns_to_contents(QTreeWidgetItem* item)
{
	resize_columns_to_contents();
}

bool TissueTreeWidget::get_hierarchy_modified() const { return modified; }

void TissueTreeWidget::set_hierarchy_modified(bool val) { modified = val; }

unsigned short TissueTreeWidget::get_tissue_instance_count(tissues_size_t type) const
{
	// Get number of items in tissue tree corresponding to the same tissue type
	unsigned short res = 0;
	for (int i = 0; i < topLevelItemCount(); ++i)
	{
		QTreeWidgetItem* item = topLevelItem(i);
		if (get_is_folder(item))
		{
			res += get_tissue_instance_count_recursively(item, type);
		}
		else if (get_type(item) == type)
		{
			res++;
		}
	}
	return res;
}

unsigned short TissueTreeWidget::get_tissue_instance_count_recursively(
		QTreeWidgetItem* parent, tissues_size_t type) const
{
	unsigned short res = 0;
	for (int i = 0; i < parent->childCount(); ++i)
	{
		QTreeWidgetItem* item = parent->child(i);
		if (get_is_folder(item))
		{
			res += get_tissue_instance_count_recursively(item, type);
		}
		else if (get_type(item) == type)
		{
			res++;
		}
	}
	return res;
}

FILE* TissueTreeWidget::SaveParams(FILE* fp, int version)
{
	return hierarchies->SaveParams(fp, version);
}

FILE* TissueTreeWidget::LoadParams(FILE* fp, int version)
{
	fp = hierarchies->LoadParams(fp, version);

	// rebuild of tree view
	set_hierarchy(hierarchies->get_selected_hierarchy());

	emit hierarchy_list_changed();

	return fp;
}

FILE* TissueTreeWidget::save_hierarchy(FILE* fp, unsigned short idx)
{
	return hierarchies->save_hierarchy(fp, idx);
}

FILE* TissueTreeWidget::load_hierarchy(FILE* fp)
{
	fp = hierarchies->load_hierarchy(fp);

	// Update folder icons
	update_folder_icons();

	// rebuild of tree view
	set_hierarchy(hierarchies->get_selected_hierarchy());

	emit hierarchy_list_changed();

	return fp;
}

void TissueTreeWidget::dropEvent(QDropEvent* de)
{
	// Only accept internal move actions
	if (!(de->source() == this && (de->possibleActions() & Qt::MoveAction)))
	{
		return;
	}

	if (!get_current_is_folder() &&
			(de->keyboardModifiers() & Qt::ShiftModifier))
	{
		// Move the item and insert a duplicate at the original position
		QTreeWidgetItem* currItem = currentItem();
		QTreeWidgetItem* currParent = currItem->parent();

		// Get original index
		unsigned int oldIdx = 0;
		if (currParent == 0)
		{
			oldIdx = indexOfTopLevelItem(currItem);
		}
		else
		{
			oldIdx = currParent->indexOfChild(currItem);
		}

		// Move item
		QTreeWidget::dropEvent(de);
		setCurrentItem(currItem);

		// Get new index
		unsigned int newIdx = 0;
		if (currParent == 0)
		{
			newIdx = indexOfTopLevelItem(currItem);
		}
		else
		{
			newIdx = currParent->indexOfChild(currItem);
		}

		// Insert copy
		if (newIdx < oldIdx)
		{
			oldIdx++;
		}
		insert_item(false, get_current_name(), currParent, oldIdx);
	}
	else
	{
		// Move the item
		QTreeWidget::dropEvent(de);
	}

	// Update internal representation
	update_hierarchy();

	// Update folder icons
	update_folder_icons();
}

void TissueTreeWidget::selectAll()
{
	bool wasBlocked = blockSignals(true);
	for (auto item : get_all_items())
	{
		if (!get_is_folder(item))
		{
			item->setSelected(!item->isHidden());
		}
	}
	blockSignals(wasBlocked);

	itemSelectionChanged();
}

void TissueTreeWidget::scrollToItem(QTreeWidgetItem* item)
{
	if (item)
	{
		scrollTo(indexFromItem(item));
	}
}

} // namespace iseg
