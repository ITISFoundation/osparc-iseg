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

#include "StdStringToQString.h"
#include "TissueHierarchy.h"
#include "TissueInfos.h"
#include "TissueTreeWidget.h"

#include "Interface/QtConnect.h"

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
		algo::split(search_tokens, search_text, algo::is_space(), algo::token_compress_on);

		for (const auto& tok : search_tokens)
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

TissueTreeWidget::TissueTreeWidget(TissueHiearchy* hierarchy, QDir picpath, QWidget* parent)
		: QTreeWidget(parent), m_Hierarchies(hierarchy)
{
	m_PicturePath = picpath;
	m_SortByNameAscending = true;
	m_SortByTypeAscending = true;

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
	QObject_connect(this, SIGNAL(itemExpanded(QTreeWidgetItem*)), this, SLOT(ResizeColumnsToContents(QTreeWidgetItem*)));
	QObject_connect(this, SIGNAL(itemCollapsed(QTreeWidgetItem*)), this, SLOT(ResizeColumnsToContents(QTreeWidgetItem*)));
	Initialize();
}

TissueTreeWidget::~TissueTreeWidget() = default;

void TissueTreeWidget::Initialize()
{
	// Clear internal representations
	m_Hierarchies->Initialize();

	emit HierarchyListChanged();
}

void TissueTreeWidget::SetTissueFilter(const QString& filter)
{
	if (filter.toStdString() != m_TissueFilter)
	{
		m_TissueFilter = filter.toStdString();

		// set visibility on items
		UpdateVisibility();
	}
}

bool TissueTreeWidget::IsVisible(tissues_size_t type) const
{
	if (type > 0)
	{
		if (auto item = FindTissueItem(type))
		{
			return !item->isHidden();
		}
	}
	return false;
}

void TissueTreeWidget::UpdateVisibility()
{
	for (int i = 0; i < topLevelItemCount(); ++i)
	{
		UpdateVisibilityRecursive(topLevelItem(i));
	}
}

void TissueTreeWidget::UpdateVisibilityRecursive(QTreeWidgetItem* current)
{
	// setHidden hides/shows recursively
	bool const matches = SearchFilter(GetName(current).toStdString(), m_TissueFilter);
	bool const is_folder = GetIsFolder(current);
	if (matches || !is_folder)
	{
		current->setHidden(!matches);
		return;
	}

	// first make all visible
	if (GetIsFolder(current))
	{
		current->setHidden(false);
	}

	// filter children
	bool any_child_visible = false;
	for (int i = 0; i < current->childCount(); ++i)
	{
		QTreeWidgetItem* child = current->child(i);
		UpdateVisibilityRecursive(child);

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

void TissueTreeWidget::UpdateHierarchy()
{
	m_Modified = true;
	m_Hierarchies->SetSelectedHierarchy(CreateCurrentHierarchy());
}

TissueHierarchyItem* TissueTreeWidget::CreateCurrentHierarchy()
{
	// Create internal representation from current QTreeWidget
	TissueHierarchyItem* root = new TissueHierarchyItem(true, QString("root"));

	for (int i = 0; i < topLevelItemCount(); ++i)
	{
		// Add top-level child
		QTreeWidgetItem* curr_widget_item = topLevelItem(i);
		TissueHierarchyItem* new_tree_item = CreateHierarchyItem(curr_widget_item);
		root->AddChild(new_tree_item);

		// Subtree of current child
		if (new_tree_item->GetIsFolder())
		{
			CreateHierarchyRecursively(curr_widget_item, new_tree_item);
		}
	}

	return root;
}

void TissueTreeWidget::CreateHierarchyRecursively(QTreeWidgetItem* parentIn, TissueHierarchyItem* parentOut)
{
	for (int i = 0; i < parentIn->childCount(); ++i)
	{
		// Add child
		QTreeWidgetItem* curr_widget_item = parentIn->child(i);
		TissueHierarchyItem* new_tree_item = CreateHierarchyItem(curr_widget_item);
		parentOut->AddChild(new_tree_item);

		// Subtree of current child
		if (new_tree_item->GetIsFolder())
		{
			CreateHierarchyRecursively(curr_widget_item, new_tree_item);
		}
	}
}

TissueHierarchyItem* TissueTreeWidget::CreateHierarchyItem(QTreeWidgetItem* item)
{
	return new TissueHierarchyItem(GetIsFolder(item), item->text(TISSUETREEWIDGET_COLUMN_NAME));
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
	TissueInfo* tissue_info = TissueInfos::GetTissueInfo(tissuenr);
	std::tie(r, g, b) = tissue_info->m_Color.ToUChar();
	abc.fill(QColor(r, g, b));
	if (tissue_info->m_Locked)
	{
		QPainter painter(&abc);
		float r, g, b;
		r = (tissue_info->m_Color[0] + 0.5f);
		if (r >= 1.0f)
			r = r - 1.0f;
		g = (tissue_info->m_Color[1] + 0.5f);
		if (g >= 1.0f)
			g = g - 1.0f;
		b = (tissue_info->m_Color[2] + 0.5f);
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

QTreeWidgetItem* TissueTreeWidget::CreateHierarchyItem(bool isFolder, const QString& name)
{
	if (isFolder)
	{
		QTreeWidgetItem* new_folder = new QTreeWidgetItem();
		new_folder->setIcon(TISSUETREEWIDGET_COLUMN_NAME, QIcon(m_PicturePath.absFilePath(QString("fileopen.png")).ascii()));
		new_folder->setText(TISSUETREEWIDGET_COLUMN_NAME, name);
		new_folder->setText(TISSUETREEWIDGET_COLUMN_FOLDER, QString::number(isFolder));
		new_folder->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
		new_folder->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
		return new_folder;
	}
	else
	{
		tissues_size_t type = TissueInfos::GetTissueType(ToStd(name));
		QTreeWidgetItem* new_tissue = new QTreeWidgetItem();
		new_tissue->setIcon(TISSUETREEWIDGET_COLUMN_NAME, generatePixmap(type));
		new_tissue->setText(TISSUETREEWIDGET_COLUMN_NAME, name);
		new_tissue->setText(TISSUETREEWIDGET_COLUMN_TYPE, QString::number(type));
		new_tissue->setText(TISSUETREEWIDGET_COLUMN_FOLDER, QString::number(isFolder));
		new_tissue->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicator);
		new_tissue->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled |
												 Qt::ItemIsDragEnabled);
		return new_tissue;
	}
}

void TissueTreeWidget::InsertItem(bool isFolder, const QString& name)
{
	QTreeWidgetItem* new_item = CreateHierarchyItem(isFolder, name);

	if (QTreeWidgetItem* curr_item = currentItem())
	{
		if (GetIsFolder(curr_item) && curr_item->isExpanded())
		{
			// Insert as first child of current folder
			curr_item->insertChild(0, new_item);
		}
		else
		{
			// Insert at current position
			QTreeWidgetItem* curr_parent = curr_item->parent();
			if (curr_parent == nullptr)
			{
				insertTopLevelItem(indexOfTopLevelItem(curr_item), new_item);
			}
			else
			{
				curr_parent->insertChild(curr_parent->indexOfChild(curr_item), new_item);
			}
		}
	}
	else // empty tissue list
	{
		invisibleRootItem()->insertChild(0, new_item);
	}

	setCurrentItem(new_item);

	// Update internal representation
	UpdateHierarchy();
}

void TissueTreeWidget::InsertItem(bool isFolder, const QString& name, QTreeWidgetItem* insertAbove)
{
	if (insertAbove == nullptr)
	{
		return;
	}

	QTreeWidgetItem* new_item = CreateHierarchyItem(isFolder, name);

	QTreeWidgetItem* parent = insertAbove->parent();
	if (parent == nullptr)
	{
		insertTopLevelItem(indexOfTopLevelItem(insertAbove), new_item);
	}
	else
	{
		parent->insertChild(parent->indexOfChild(insertAbove), new_item);
	}

	setCurrentItem(new_item);

	// Update internal representation
	UpdateHierarchy();
}

void TissueTreeWidget::InsertItem(bool isFolder, const QString& name, QTreeWidgetItem* parent, unsigned int index)
{
	QTreeWidgetItem* new_item = CreateHierarchyItem(isFolder, name);

	if (parent == nullptr)
	{
		insertTopLevelItem(index, new_item);
	}
	else
	{
		parent->insertChild(index, new_item);
	}

	setCurrentItem(new_item);

	// Update internal representation
	UpdateHierarchy();
}

void TissueTreeWidget::RemoveTissue(const QString& name)
{
	// Removes a tissue completely from all hierarchies
	blockSignals(true);
	for (int i = 0; i < topLevelItemCount(); ++i)
	{
		QTreeWidgetItem* curr_item = topLevelItem(i);
		if (GetIsFolder(curr_item))
		{
			RemoveTissueRecursively(curr_item, name);
		}
		else if (GetName(curr_item).compare(name) == 0)
		{
			// Delete tissue item
			takeTopLevelItem(i);
			delete curr_item;
			--i;
		}
		else
		{
			// Update tissue type
			tissues_size_t new_type = TissueInfos::GetTissueType(ToStd(curr_item->text(TISSUETREEWIDGET_COLUMN_NAME)));
			curr_item->setText(TISSUETREEWIDGET_COLUMN_TYPE, QString::number(new_type));
		}
	}
	blockSignals(false);

	// Update internal representation
	UpdateHierarchy();

	// Propagate tissue removal to other hierarchies
	m_Hierarchies->UpdateHierarchies();
}

void TissueTreeWidget::RemoveTissueRecursively(QTreeWidgetItem* parent, const QString& name)
{
	for (int i = 0; i < parent->childCount(); ++i)
	{
		QTreeWidgetItem* curr_item = parent->child(i);
		if (GetIsFolder(curr_item))
		{
			RemoveTissueRecursively(curr_item, name);
		}
		else if (GetName(curr_item).compare(name) == 0)
		{
			// Delete tissue item
			parent->takeChild(i);
			delete curr_item;
			--i;
		}
		else
		{
			// Update tissue type
			tissues_size_t new_type = TissueInfos::GetTissueType(ToStd(curr_item->text(TISSUETREEWIDGET_COLUMN_NAME)));
			curr_item->setText(TISSUETREEWIDGET_COLUMN_TYPE, QString::number(new_type));
		}
	}
}

void TissueTreeWidget::RemoveItems(const std::vector<QTreeWidgetItem*>& items)
{
	std::set<QTreeWidgetItem*> deleted;
	for (auto curr_item : items)
	{
		if (deleted.count(curr_item) == 0)
		{
			RemoveItem(curr_item, false, false);
			deleted.insert(curr_item);
		}
		else
		{
			ISEG_WARNING("trying to delete item again");
		}
	}

	// Update internal representation
	UpdateHierarchy();

	// Propagate tissue removal to other hierarchies
	m_Hierarchies->UpdateHierarchies();

	// Update tissue indices
	UpdateTissueIndices();
}

void TissueTreeWidget::RemoveItem(QTreeWidgetItem* currItem, bool removeChildren, bool updateRepresentation)
{
	// Removes item in QTreeWidget and internal representations
	QTreeWidgetItem* curr_parent = currItem->parent();
	bool update_tissues = false;

	if (GetIsFolder(currItem))
	{
		// Delete folder
		if (removeChildren)
		{
			// Delete all children of current item
			QList<QTreeWidgetItem*> children;
			TakeChildrenRecursively(currItem, children);
			qDeleteAll(children);

			// Delete current item
			if (curr_parent == nullptr)
			{
				takeTopLevelItem(indexOfTopLevelItem(currItem));
				delete currItem;
			}
			else
			{
				curr_parent->removeChild(currItem);
				delete currItem;
			}

			update_tissues = true;
		}
		else
		{
			// Insert children into parent and delete current item
			if (curr_parent == nullptr)
			{
				insertTopLevelItems(indexOfTopLevelItem(currItem), currItem->takeChildren());
				takeTopLevelItem(indexOfTopLevelItem(currItem));
				delete currItem;
			}
			else
			{
				curr_parent->insertChildren(curr_parent->indexOfChild(currItem), currItem->takeChildren());
				curr_parent->removeChild(currItem);
				delete currItem;
			}
		}
	}
	else
	{
		// Delete tissue
		if (curr_parent == nullptr)
		{
			takeTopLevelItem(indexOfTopLevelItem(currItem));
			delete currItem;
		}
		else
		{
			curr_parent->removeChild(currItem);
			delete currItem;
		}

		update_tissues = true;
	}

	// Update internal representation
	if (updateRepresentation)
	{
		UpdateHierarchy();

		// Propagate tissue removal to other hierarchies
		if (update_tissues)
		{
			m_Hierarchies->UpdateHierarchies();

			// Update tissue indices
			UpdateTissueIndices();
		}
	}
}

void TissueTreeWidget::RemoveAllFolders(bool removeChildren)
{
	// Removes all folders in QTreeWidget and internal representations

	bool update_tissues = false;
	blockSignals(true);
	if (removeChildren)
	{
		for (int i = 0; i < topLevelItemCount(); ++i)
		{
			QTreeWidgetItem* item = topLevelItem(i);
			if (GetIsFolder(item))
			{
				// Delete all children of item
				QList<QTreeWidgetItem*> children;
				TakeChildrenRecursively(item, children);
				qDeleteAll(children);

				// Delete item
				takeTopLevelItem(i);
				delete item;
				update_tissues = true;
				--i;
			}
		}
	}
	else
	{
		for (int i = 0; i < topLevelItemCount(); ++i)
		{
			QTreeWidgetItem* item = topLevelItem(i);
			if (GetIsFolder(item))
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
	UpdateHierarchy();

	// Propagate tissue removal to other hierarchies
	if (update_tissues)
	{
		m_Hierarchies->UpdateHierarchies();
	}
}

void TissueTreeWidget::UpdateTissueName(const QString& oldName, const QString& newName)
{
	if (oldName.compare(newName) == 0)
	{
		return;
	}

	// Update tissue name in internal representations
	auto& hierarchy_trees = m_Hierarchies->Hierarchies();
	for (auto & hierarchy_tree : hierarchy_trees)
	{
		hierarchy_tree->UpdateTissueNameRecursively(oldName, newName);
	}

	// Update tissue name in tree widget
	UpdateTissueNameWidget(oldName, newName);
}

void TissueTreeWidget::UpdateTissueNameWidget(const QString& oldName, const QString& newName, QTreeWidgetItem* parent)
{
	if (parent == nullptr)
	{
		// Recursion with top level children
		for (int i = 0; i < topLevelItemCount(); ++i)
		{
			UpdateTissueNameWidget(oldName, newName, topLevelItem(i));
		}
	}
	else
	{
		if (GetIsFolder(parent))
		{
			// Recursion with children
			for (int i = 0; i < parent->childCount(); ++i)
			{
				UpdateTissueNameWidget(oldName, newName, parent->child(i));
			}
		}
		else
		{
			// Update tissue name
			if (parent->text(TISSUETREEWIDGET_COLUMN_NAME).compare(oldName) == 0)
			{
				parent->setText(TISSUETREEWIDGET_COLUMN_NAME, newName);
			}
		}
	}
}

void TissueTreeWidget::UpdateTissueIcons(QTreeWidgetItem* parent)
{
	if (parent == nullptr)
	{
		// Recursion with top level children
		for (int i = 0; i < topLevelItemCount(); ++i)
		{
			UpdateTissueIcons(topLevelItem(i));
		}
	}
	else
	{
		if (GetIsFolder(parent))
		{
			// Recursion with children
			for (int i = 0; i < parent->childCount(); ++i)
			{
				UpdateTissueIcons(parent->child(i));
			}
		}
		else
		{
			// Update tissue icon
			parent->setIcon(TISSUETREEWIDGET_COLUMN_NAME, generatePixmap(GetType(parent)));
		}
	}
}

void TissueTreeWidget::UpdateFolderIcons(QTreeWidgetItem* parent)
{
	// Updates the folder icons based on the lock state of the child tissues
	// This only works if the tree widget is completely built

	if (parent == nullptr)
	{
		// Recursion with top level children
		for (int i = 0; i < topLevelItemCount(); ++i)
		{
			UpdateFolderIcons(topLevelItem(i));
		}
	}
	else
	{
		if (GetIsFolder(parent))
		{
			// Update folder icon
			short lock_states = GetChildLockstates(parent);
			if (lock_states == 0)
			{
				// All child tissues unlocked
				parent->setIcon(TISSUETREEWIDGET_COLUMN_NAME, QIcon(m_PicturePath.absFilePath(QString("fileopen.png"))
																																.ascii()));
			}
			else if (lock_states == 1)
			{
				// All child tissues locked
				parent->setIcon(TISSUETREEWIDGET_COLUMN_NAME, QIcon(m_PicturePath.absFilePath(QString("folderlock1.png"))
																																.ascii()));
			}
			else
			{
				// Mixed locked/unlocked
				parent->setIcon(TISSUETREEWIDGET_COLUMN_NAME, QIcon(m_PicturePath.absFilePath(QString("folderlock2.png"))
																																.ascii()));
			}

			// Recursion with children
			for (int i = 0; i < parent->childCount(); ++i)
			{
				UpdateFolderIcons(parent->child(i));
			}
		}
	}
}

// TODO: Optimize by introducing folder map / hidden flag
short TissueTreeWidget::GetChildLockstates(QTreeWidgetItem* folder)
{
	// Returns whether all child tissues (including subfolders) are
	// unlocked (return value 0), // locked (return value 1), or
	// mixed locked and unlocked (-1)

	// Get lock state of first child (non-empty folder or tissue)
	int i;
	short lock_states = 0; // Empty folders are considered to be unlocked
	for (i = 0; i < folder->childCount(); ++i)
	{
		QTreeWidgetItem* curr_child = folder->child(i);
		if (GetIsFolder(curr_child))
		{
			if (curr_child->childCount() > 0)
			{
				lock_states = GetChildLockstates(curr_child);
			}
			else
			{
				continue;
			}
		}
		else if (TissueInfos::GetTissueLocked(GetType(curr_child)))
		{
			lock_states = 1;
			break;
		}
		else
		{
			lock_states = 0;
			break;
		}
	}
	if (lock_states == -1)
	{
		return -1;
	}

	// Test against lock states of other children
	for (int i = 1; i < folder->childCount(); ++i)
	{
		QTreeWidgetItem* curr_child = folder->child(i);
		if (GetIsFolder(curr_child))
		{
			// Skip empty folders
			if (curr_child->childCount() > 0 &&
					(lock_states != GetChildLockstates(curr_child)))
			{
				return -1;
			}
		}
		else if ((bool)lock_states !=
						 TissueInfos::GetTissueLocked(GetType(curr_child)))
		{
			return -1;
		}
	}
	return lock_states;
}

void TissueTreeWidget::PadTissueIndices()
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
		if (GetIsFolder(item))
		{
			PadTissueIndicesRecursively(item, digits);
		}
		else
		{
			// Pad with leading zeroes
			QString padded_text = item->text(TISSUETREEWIDGET_COLUMN_TYPE);
			while (padded_text.length() < digits)
			{
				padded_text = padded_text.prepend('0');
			}
			item->setText(TISSUETREEWIDGET_COLUMN_TYPE, padded_text);
		}
	}
}

void TissueTreeWidget::PadTissueIndicesRecursively(QTreeWidgetItem* parent, unsigned short digits)
{
	for (int i = 0; i < parent->childCount(); ++i)
	{
		QTreeWidgetItem* item = parent->child(i);
		if (GetIsFolder(item))
		{
			PadTissueIndicesRecursively(item, digits);
		}
		else
		{
			// Pad with leading zeroes
			QString padded_text = item->text(TISSUETREEWIDGET_COLUMN_TYPE);
			while (padded_text.length() < digits)
			{
				padded_text = padded_text.prepend('0');
			}
			item->setText(TISSUETREEWIDGET_COLUMN_TYPE, padded_text);
		}
	}
}

void TissueTreeWidget::UpdateTissueIndices()
{
	for (int i = 0; i < topLevelItemCount(); ++i)
	{
		QTreeWidgetItem* item = topLevelItem(i);
		if (GetIsFolder(item))
		{
			UpdateTissueIndicesRecursively(item);
		}
		else
		{
			tissues_size_t type = TissueInfos::GetTissueType(ToStd(item->text(TISSUETREEWIDGET_COLUMN_NAME)));
			item->setText(TISSUETREEWIDGET_COLUMN_TYPE, QString::number(type));
		}
	}
}

void TissueTreeWidget::UpdateTissueIndicesRecursively(QTreeWidgetItem* parent)
{
	for (int i = 0; i < parent->childCount(); ++i)
	{
		QTreeWidgetItem* item = parent->child(i);
		if (GetIsFolder(item))
		{
			UpdateTissueIndicesRecursively(item);
		}
		else
		{
			tissues_size_t type = TissueInfos::GetTissueType(ToStd(item->text(TISSUETREEWIDGET_COLUMN_NAME)));
			item->setText(TISSUETREEWIDGET_COLUMN_TYPE, QString::number(type));
		}
	}
}

void TissueTreeWidget::SetCurrentFolderName(const QString& name)
{
	if (GetCurrentIsFolder())
	{
		currentItem()->setText(TISSUETREEWIDGET_COLUMN_NAME, name);

		// Update internal representation
		UpdateHierarchy();
	}
}

void TissueTreeWidget::SetCurrentItem(QTreeWidgetItem* item)
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

void TissueTreeWidget::SetCurrentTissue(tissues_size_t type)
{
	if (type > 0 && type <= TissueInfos::GetTissueCount())
	{
		QTreeWidgetItem* item = FindTissueItem(type);
		setCurrentItem(item);
		QTreeWidgetItem* parent = item->parent();
		if (parent != nullptr)
		{
			parent->setExpanded(true);
		}
	}
	else if (topLevelItemCount() > 0 && type > 0)
	{
		setCurrentItem(topLevelItem(0));
	}
}

QTreeWidgetItem* TissueTreeWidget::FindTissueItem(tissues_size_t type, QTreeWidgetItem* parent) const
{
	if (parent == nullptr)
	{
		// Recursion with top level children
		for (int i = 0; i < topLevelItemCount(); ++i)
		{
			QTreeWidgetItem* recursive_res = FindTissueItem(type, topLevelItem(i));
			if (recursive_res != nullptr)
			{
				return recursive_res;
			}
		}
	}
	else
	{
		// Check if parent is searched item
		if (type == GetType(parent))
		{
			return parent;
		}
		// Recursion with children
		for (int i = 0; i < parent->childCount(); ++i)
		{
			QTreeWidgetItem* recursive_res = FindTissueItem(type, parent->child(i));
			if (recursive_res != nullptr)
			{
				return recursive_res;
			}
		}
	}
	return (QTreeWidgetItem*)nullptr;
}

tissues_size_t TissueTreeWidget::GetCurrentType() const
{
	return GetType(currentItem());
}

QString TissueTreeWidget::GetCurrentName() const { return GetName(currentItem()); }

bool TissueTreeWidget::GetCurrentIsFolder() const
{
	return GetIsFolder(currentItem());
}

bool TissueTreeWidget::GetIsFolder(const QTreeWidgetItem* item) const
{
	if (item)
	{
		return item->text(TISSUETREEWIDGET_COLUMN_FOLDER).toUShort() != 0;
	}
	return true;
}

tissues_size_t TissueTreeWidget::GetType(const QTreeWidgetItem* item) const
{
	if (item && !GetIsFolder(item))
	{
		return (tissues_size_t)item->text(TISSUETREEWIDGET_COLUMN_TYPE).toUInt();
	}
	return 0;
}

QString TissueTreeWidget::GetName(const QTreeWidgetItem* item) const
{
	if (item)
	{
		return item->text(TISSUETREEWIDGET_COLUMN_NAME);
	}
	return "";
}

bool TissueTreeWidget::GetCurrentHasChildren() const
{
	if (currentItem() == nullptr)
	{
		return false;
	}
	else
	{
		return currentItem()->childCount() > 0;
	}
}

void TissueTreeWidget::GetCurrentChildTissues(std::map<tissues_size_t, unsigned short>& types) const
{
	types.clear();
	if (currentItem() != nullptr)
	{
		GetChildTissuesRecursively(currentItem(), types);
	}
}

void TissueTreeWidget::GetSublevelChildTissues(std::map<tissues_size_t, unsigned short>& types) const
{
	types.clear();
	for (int i = 0; i < topLevelItemCount(); ++i)
	{
		QTreeWidgetItem* item = topLevelItem(i);
		if (GetIsFolder(item))
		{
			GetChildTissuesRecursively(item, types);
		}
	}
}

void TissueTreeWidget::GetChildTissuesRecursively(QTreeWidgetItem* parent, std::map<tissues_size_t, unsigned short>& types) const
{
	for (int i = 0; i < parent->childCount(); ++i)
	{
		tissues_size_t curr_type = (tissues_size_t)parent->child(i)
																	 ->text(TISSUETREEWIDGET_COLUMN_TYPE)
																	 .toUInt();
		if (curr_type > 0)
		{
			// Insert tissue
			if (types.find(curr_type) != types.end())
			{
				types[curr_type]++;
			}
			else
			{
				types[curr_type] = 1;
			}
		}
		else
		{
			// Get tissues in subfolder
			GetChildTissuesRecursively(parent->child(i), types);
		}
	}
}

unsigned short TissueTreeWidget::GetSelectedHierarchy() const
{
	return m_Hierarchies->GetSelectedHierarchy();
}

unsigned short TissueTreeWidget::GetHierarchyCount() const
{
	return m_Hierarchies->GetHierarchyCount();
}

std::vector<QString>* TissueTreeWidget::GetHierarchyNamesPtr() const
{
	return m_Hierarchies->GetHierarchyNamesPtr();
}

QString TissueTreeWidget::GetCurrentHierarchyName() const
{
	return m_Hierarchies->GetCurrentHierarchyName();
}

void TissueTreeWidget::ResetDefaultHierarchy()
{
	m_Hierarchies->ResetDefaultHierarchy();
}

void TissueTreeWidget::SetHierarchy(unsigned short index)
{
	if (!m_Hierarchies->SetHierarchy(index))
	{
		return;
	}

	tissues_size_t current_type = GetCurrentType();

	// Build QTreeWidgetItems from TissueTreeItems
	blockSignals(true);
	BuildTreeWidget(m_Hierarchies->SelectedHierarchy());
	blockSignals(false);

	// Restore item selection
	if (current_type > 0)
	{
		SetCurrentTissue(current_type);
	}
	else
	{
		setCurrentItem(topLevelItem(0));
	}

	// Resize columns
	ResizeColumnsToContents();
}

void TissueTreeWidget::BuildTreeWidget(TissueHierarchyItem* root)
{
	clear();

	// Create pool of existing tissues
	std::set<tissues_size_t> tissue_types;
	for (tissues_size_t type = 1; type <= TissueInfos::GetTissueCount(); ++type)
	{
		tissue_types.insert(type);
	}

	std::vector<TissueHierarchyItem*>* children = root->GetChildren();
	for (auto iter = children->begin(); iter != children->end(); ++iter)
	{
		// Add top-level item
		TissueHierarchyItem* curr_item = *iter;
		if (curr_item->GetIsFolder())
		{
			QTreeWidgetItem* new_folder = CreateHierarchyItem(true, curr_item->GetName());
			addTopLevelItem(new_folder);
			BuildTreeWidgetRecursively(curr_item, new_folder, &tissue_types);
		}
		else
		{
			tissues_size_t type = TissueInfos::GetTissueType(ToStd(curr_item->GetName()));
			if (type > 0)
			{
				QTreeWidgetItem* new_tissue =
						CreateHierarchyItem(false, curr_item->GetName());
				addTopLevelItem(new_tissue);
				std::set<tissues_size_t>::iterator iter =
						tissue_types.find(type);
				if (iter != tissue_types.end())
				{
					tissue_types.erase(iter);
				}
			}
		}
	}

	// Add all missing tissues at top level
	for (auto iter = tissue_types.begin(); iter != tissue_types.end(); ++iter)
	{
		addTopLevelItem(CreateHierarchyItem(false, ToQ(TissueInfos::GetTissueName(*iter))));
	}

	// Update folder icons
	UpdateFolderIcons();

	m_Modified = false;
}

void TissueTreeWidget::BuildTreeWidgetRecursively(TissueHierarchyItem* parentIn, QTreeWidgetItem* parentOut, std::set<tissues_size_t>* tissueTypes)
{
	std::vector<TissueHierarchyItem*>* children = parentIn->GetChildren();
	for (auto iter = children->begin(); iter != children->end(); ++iter)
	{
		// Add item to parentOut
		TissueHierarchyItem* curr_item = *iter;
		if (curr_item->GetIsFolder())
		{
			QTreeWidgetItem* new_folder =
					CreateHierarchyItem(true, curr_item->GetName());
			parentOut->addChild(new_folder);
			BuildTreeWidgetRecursively(curr_item, new_folder, tissueTypes);
		}
		else
		{
			tissues_size_t type = TissueInfos::GetTissueType(ToStd(curr_item->GetName()));
			if (type > 0)
			{
				QTreeWidgetItem* new_tissue = CreateHierarchyItem(false, curr_item->GetName());
				parentOut->addChild(new_tissue);
				auto iter = tissueTypes->find(type);
				if (iter != tissueTypes->end())
				{
					tissueTypes->erase(iter);
				}
			}
		}
	}
}

void TissueTreeWidget::AddNewHierarchy(const QString& name)
{
	// Create and select default hierarchy
	m_Hierarchies->AddNewHierarchy(name);

	emit HierarchyListChanged();
}

bool TissueTreeWidget::LoadHierarchy(const QString& path)
{
	if (!m_Hierarchies->LoadHierarchy(path))
	{
		return false;
	}

	emit HierarchyListChanged();

	return true;
}

void TissueTreeWidget::TakeChildrenRecursively(QTreeWidgetItem* parent, QList<QTreeWidgetItem*>& appendTo)
{
	// Recursion with children
	for (int i = 0; i < parent->childCount(); ++i)
	{
		TakeChildrenRecursively(parent->child(i), appendTo);
	}
	// Append children to list
	appendTo.append(parent->takeChildren());
}

void TissueTreeWidget::UpdateTreeWidget()
{
	ResetDefaultHierarchy();
	SetHierarchy(m_Hierarchies->GetSelectedHierarchy());
}

bool TissueTreeWidget::SaveHierarchyAs(const QString& name, const QString& path)
{
	if (m_Hierarchies->GetSelectedHierarchy() == 0)
	{
		// Do not override default hierarchy: Append to list
		m_Hierarchies->AddNewHierarchy(name, CreateCurrentHierarchy());

		// Reset default hierarchy
		m_Hierarchies->ResetDefaultHierarchy();
		emit HierarchyListChanged();
	}
	else
	{
		// Update hierarchy name and internal representation
		m_Hierarchies->SelectedHierarchyName() = name;
		emit HierarchyListChanged();
		UpdateHierarchy();
	}

	m_Modified = false;

	return m_Hierarchies->SaveHierarchyAs(name, path);
}

void TissueTreeWidget::RemoveCurrentHierarchy()
{
	if (m_Hierarchies->RemoveCurrentHierarchy())
	{
		m_Modified = true;
	}

	// \bug it seems the folders are not refreshed properly
	SetHierarchy(m_Hierarchies->GetSelectedHierarchy());

	emit HierarchyListChanged();
}

void TissueTreeWidget::ToggleShowTissueIndices()
{
	setColumnHidden(TISSUETREEWIDGET_COLUMN_TYPE, !isColumnHidden(TISSUETREEWIDGET_COLUMN_TYPE));
	ResizeColumnsToContents();
}

void TissueTreeWidget::SortByTissueName()
{
	if (m_SortByNameAscending)
	{
		sortItems(TISSUETREEWIDGET_COLUMN_NAME, Qt::AscendingOrder);
	}
	else
	{
		sortItems(TISSUETREEWIDGET_COLUMN_NAME, Qt::DescendingOrder);
	}
	m_SortByNameAscending = !m_SortByNameAscending;

	// Update internal representation
	UpdateHierarchy();
}

void TissueTreeWidget::SortByTissueIndex()
{
	// Pad tissue indices with zeroes because of lexical sorting
	PadTissueIndices();

	if (m_SortByTypeAscending)
	{
		sortItems(TISSUETREEWIDGET_COLUMN_TYPE, Qt::AscendingOrder);
	}
	else
	{
		sortItems(TISSUETREEWIDGET_COLUMN_TYPE, Qt::DescendingOrder);
	}
	m_SortByTypeAscending = !m_SortByTypeAscending;

	// Update tissue indices to undo padding
	UpdateTissueIndices();

	// Update internal representation
	UpdateHierarchy();
}

bool TissueTreeWidget::GetTissueIndicesHidden() const
{
	return isColumnHidden(TISSUETREEWIDGET_COLUMN_TYPE);
}

std::vector<QTreeWidgetItem*> TissueTreeWidget::Collect(const std::vector<QTreeWidgetItem*>& list) const
{
	std::vector<QTreeWidgetItem*> my_children;

	for (const auto& item : list)
	{
		if (item != invisibleRootItem())
		{
			my_children.push_back(item);
		}

		for (int i = 0; i < item->childCount(); ++i)
		{
			auto extra = Collect({item->child(i)});
			my_children.insert(my_children.end(), extra.begin(), extra.end());
		}
	}

	return my_children;
}

std::vector<QTreeWidgetItem*> TissueTreeWidget::GetAllItems(bool leaves_only) const
{
	auto all = Collect({invisibleRootItem()});
	if (leaves_only)
	{
		std::vector<QTreeWidgetItem*> leaves;
		std::copy_if(all.begin(), all.end(), std::back_inserter(leaves), [this](QTreeWidgetItem* item) { return !GetIsFolder(item); });
		return leaves;
	}
	return all;
}

void TissueTreeWidget::ResizeColumnsToContents()
{
	for (int col = 0; col < TISSUETREEWIDGET_COLUMN_COUNT; ++col)
	{
		resizeColumnToContents(col);
	}
}

void TissueTreeWidget::ResizeColumnsToContents(QTreeWidgetItem* item)
{
	ResizeColumnsToContents();
}

bool TissueTreeWidget::GetHierarchyModified() const { return m_Modified; }

void TissueTreeWidget::SetHierarchyModified(bool val) { m_Modified = val; }

unsigned short TissueTreeWidget::GetTissueInstanceCount(tissues_size_t type) const
{
	// Get number of items in tissue tree corresponding to the same tissue type
	unsigned short res = 0;
	for (int i = 0; i < topLevelItemCount(); ++i)
	{
		QTreeWidgetItem* item = topLevelItem(i);
		if (GetIsFolder(item))
		{
			res += GetTissueInstanceCountRecursively(item, type);
		}
		else if (GetType(item) == type)
		{
			res++;
		}
	}
	return res;
}

unsigned short TissueTreeWidget::GetTissueInstanceCountRecursively(QTreeWidgetItem* parent, tissues_size_t type) const
{
	unsigned short res = 0;
	for (int i = 0; i < parent->childCount(); ++i)
	{
		QTreeWidgetItem* item = parent->child(i);
		if (GetIsFolder(item))
		{
			res += GetTissueInstanceCountRecursively(item, type);
		}
		else if (GetType(item) == type)
		{
			res++;
		}
	}
	return res;
}

FILE* TissueTreeWidget::SaveParams(FILE* fp, int version)
{
	return m_Hierarchies->SaveParams(fp, version);
}

FILE* TissueTreeWidget::LoadParams(FILE* fp, int version)
{
	fp = m_Hierarchies->LoadParams(fp, version);

	// rebuild of tree view
	SetHierarchy(m_Hierarchies->GetSelectedHierarchy());

	emit HierarchyListChanged();

	return fp;
}

FILE* TissueTreeWidget::SaveHierarchy(FILE* fp, unsigned short idx)
{
	return m_Hierarchies->SaveHierarchy(fp, idx);
}

FILE* TissueTreeWidget::LoadHierarchy(FILE* fp)
{
	fp = m_Hierarchies->LoadHierarchy(fp);

	// Update folder icons
	UpdateFolderIcons();

	// rebuild of tree view
	SetHierarchy(m_Hierarchies->GetSelectedHierarchy());

	emit HierarchyListChanged();

	return fp;
}

void TissueTreeWidget::dropEvent(QDropEvent* de)
{
	// Only accept internal move actions
	if (!(de->source() == this && (de->possibleActions() & Qt::MoveAction)))
	{
		return;
	}

	if (!GetCurrentIsFolder() &&
			(de->keyboardModifiers() & Qt::ShiftModifier))
	{
		// Move the item and insert a duplicate at the original position
		QTreeWidgetItem* curr_item = currentItem();
		QTreeWidgetItem* curr_parent = curr_item->parent();

		// Get original index
		unsigned int old_idx = 0;
		if (curr_parent == nullptr)
		{
			old_idx = indexOfTopLevelItem(curr_item);
		}
		else
		{
			old_idx = curr_parent->indexOfChild(curr_item);
		}

		// Move item
		QTreeWidget::dropEvent(de);
		setCurrentItem(curr_item);

		// Get new index
		unsigned int new_idx = 0;
		if (curr_parent == nullptr)
		{
			new_idx = indexOfTopLevelItem(curr_item);
		}
		else
		{
			new_idx = curr_parent->indexOfChild(curr_item);
		}

		// Insert copy
		if (new_idx < old_idx)
		{
			old_idx++;
		}
		InsertItem(false, GetCurrentName(), curr_parent, old_idx);
	}
	else
	{
		// Move the item
		QTreeWidget::dropEvent(de);
	}

	// Update internal representation
	UpdateHierarchy();

	// Update folder icons
	UpdateFolderIcons();
}

void TissueTreeWidget::selectAll()
{
	bool was_blocked = blockSignals(true);
	for (auto item : GetAllItems())
	{
		if (!GetIsFolder(item))
		{
			item->setSelected(!item->isHidden());
		}
	}
	blockSignals(was_blocked);

	emit itemSelectionChanged();
}

void TissueTreeWidget::scrollToItem(QTreeWidgetItem* item)
{
	if (item)
	{
		scrollTo(indexFromItem(item));
	}
}

} // namespace iseg
