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

#include <QDir>
#include <QFile>
#include <QXmlInputSource>
#include <QXmlStreamWriter>

namespace iseg {

namespace {

class TissueHierarchySaxHandler : public QXmlDefaultHandler
{
public:
	TissueHierarchySaxHandler(TissueHierarchyItem* root);

	bool startElement(const QString& namespaceURI, const QString& localName,
					  const QString& qName, const QXmlAttributes& attributes);

	bool endElement(const QString& namespaceURI, const QString& localName,
					const QString& qName);

	bool fatalError(const QXmlParseException& exception);

	QString getHierarchyName();

private:
	TissueHierarchyItem* currentFolder;
	QString currentText;
	QString loadedHierarchyName;
};

TissueHierarchySaxHandler::TissueHierarchySaxHandler(TissueHierarchyItem* root)
{
	currentFolder = root;
	loadedHierarchyName = QString();
}

bool TissueHierarchySaxHandler::startElement(const QString& /* namespaceURI */,
											 const QString& /* localName */,
											 const QString& qName,
											 const QXmlAttributes& attributes)
{
	if (qName == "hierarchy")
	{
		loadedHierarchyName = attributes.value("name");
	}
	else if (qName == "tissue")
	{
		// Add tissue with given name
		QString tissueName(attributes.value("name"));
		currentFolder->AddChild(new TissueHierarchyItem(false, tissueName));
	}
	else if (qName == "folder")
	{
		// Add as subfolder to last folder
		QString folderName(attributes.value("name"));
		TissueHierarchyItem* lastFolder = currentFolder;
		currentFolder = new TissueHierarchyItem(true, folderName);
		lastFolder->AddChild(currentFolder);
	}
	return true;
}

bool TissueHierarchySaxHandler::endElement(const QString& /* namespaceURI */,
										   const QString& /* localName */,
										   const QString& qName)
{
	if (qName == "folder")
	{
		// Set current folder to parent
		currentFolder = currentFolder->GetParent();
	}
	return true;
}

bool TissueHierarchySaxHandler::fatalError(const QXmlParseException& exception)
{
	// TODO: Handle error
	/*QMessageBox::warning(	0, QObject::tr("SAX Handler"),
		QObject::tr("Parse error at line %1, column %2:\n%3.")
		.arg(exception.lineNumber())
		.arg(exception.columnNumber())
		.arg(exception.message()));*/
	return false;
}

QString TissueHierarchySaxHandler::getHierarchyName()
{
	return loadedHierarchyName;
}
} // namespace

//////////////////////////////////////////////////////////////////////////

TissueHierarchyItem::TissueHierarchyItem(bool folderItem,
										 const QString& itemName)
{
	isFolder = folderItem;
	name = itemName;
	parent = 0;
}

TissueHierarchyItem::~TissueHierarchyItem() { children.clear(); }

bool TissueHierarchyItem::GetIsFolder() { return isFolder; }

QString TissueHierarchyItem::GetName() { return name; }

unsigned int TissueHierarchyItem::GetChildCount()
{
	return static_cast<unsigned int>(children.size());
}

TissueHierarchyItem* TissueHierarchyItem::GetParent() { return parent; }

std::vector<TissueHierarchyItem*>* TissueHierarchyItem::GetChildren()
{
	return &children;
}

void TissueHierarchyItem::AddChild(TissueHierarchyItem* child)
{
	child->SetParent(this);
	children.push_back(child);
}

void TissueHierarchyItem::UpdateTissuesRecursively()
{
	// Remove tissues which have been deleted from the tissue list
	for (std::vector<TissueHierarchyItem*>::iterator iter = children.begin();
		 iter != children.end();)
	{
		TissueHierarchyItem* currChild = *iter;
		if (currChild->GetIsFolder())
		{
			// Recursion with current folder child
			currChild->UpdateTissuesRecursively();
			++iter;
		}
		else if (!TissueInfos::GetTissueType(ToStd(currChild->GetName())))
		{
			// Delete tissue
			iter = children.erase(iter);
		}
		else
		{
			++iter;
		}
	}
}

void TissueHierarchyItem::UpdateTissueNameRecursively(const QString& oldName,
													  const QString& newName)
{
	// Update tissue name
	if (!isFolder && name.compare(oldName) == 0)
	{
		name = newName;
	}

	// Recursion with children
	for (std::vector<TissueHierarchyItem*>::iterator iter = children.begin();
		 iter != children.end(); ++iter)
	{
		(*iter)->UpdateTissueNameRecursively(oldName, newName);
	}
}

void TissueHierarchyItem::SetParent(TissueHierarchyItem* parentItem)
{
	parent = parentItem;
}

//////////////////////////////////////////////////////////////////////////

void TissueHiearchy::initialize()
{
	// Clear internal representations
	hierarchyNames.clear();
	for (unsigned int i = 0; i < hierarchyTrees.size(); ++i)
	{
		delete hierarchyTrees[i];
	}
	hierarchyTrees.clear();

	// Create and select default hierarchy
	hierarchyNames.push_back("(Default)");
	hierarchyTrees.push_back(create_default_hierarchy());
	set_hierarchy(0);
}

TissueHierarchyItem* TissueHiearchy::create_default_hierarchy()
{
	// Create internal representation of default hierarchy
	return new TissueHierarchyItem(true, QString("root"));
}

TissueHierarchyItem* TissueHiearchy::selected_hierarchy()
{
	if (selectedHierarchy < hierarchyTrees.size())
	{
		return hierarchyTrees.at(selectedHierarchy);
	}
	return nullptr;
}

void TissueHiearchy::set_selected_hierarchy(TissueHierarchyItem* h)
{
	if (h != hierarchyTrees[selectedHierarchy])
	{
		delete hierarchyTrees[selectedHierarchy];
		hierarchyTrees[selectedHierarchy] = h;
	}
}

bool TissueHiearchy::remove_current_hierarchy()
{
	if (selectedHierarchy == 0)
	{
		return false;
	}
	hierarchyNames.erase(hierarchyNames.begin() + selectedHierarchy);
	delete hierarchyTrees[selectedHierarchy];
	hierarchyTrees.erase(hierarchyTrees.begin() + selectedHierarchy);

	// Set default hierarchy
	set_hierarchy(0);
	return true;
}

bool TissueHiearchy::set_hierarchy(unsigned short index)
{
	if (index < 0 || index >= hierarchyTrees.size())
	{
		return false;
	}

	selectedHierarchy = index;
	return true;
}

unsigned short TissueHiearchy::get_selected_hierarchy()
{
	return selectedHierarchy;
}

unsigned short TissueHiearchy::get_hierarchy_count()
{
	return static_cast<unsigned short>(hierarchyNames.size());
}

std::vector<QString>* TissueHiearchy::get_hierarchy_names_ptr()
{
	return &hierarchyNames;
}

QString TissueHiearchy::get_current_hierarchy_name()
{
	return hierarchyNames[selectedHierarchy];
}

void TissueHiearchy::reset_default_hierarchy()
{
	delete hierarchyTrees[0];
	hierarchyTrees[0] = create_default_hierarchy();
}

void TissueHiearchy::add_new_hierarchy(const QString& name)
{
	add_new_hierarchy(name, create_default_hierarchy());
}

void TissueHiearchy::add_new_hierarchy(const QString& name,
									   TissueHierarchyItem* hierarchy)
{
	// Create and select default hierarchy
	hierarchyNames.push_back(name);
	hierarchyTrees.push_back(hierarchy);
	set_hierarchy(static_cast<unsigned short>(hierarchyTrees.size() - 1));
}

void TissueHiearchy::update_hierarchies()
{
	for (unsigned int i = 0; i < hierarchyTrees.size(); ++i)
	{
		if (i != selectedHierarchy)
		{
			hierarchyTrees[i]->UpdateTissuesRecursively();
		}
	}
}

bool TissueHiearchy::load_hierarchy(const QString& path)
{
	// Get existing tissues by creating default hierarchy
	TissueHierarchyItem* root = new TissueHierarchyItem(true, QString("root"));

	// Open xml file
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		delete root;
		return false;
	}
	QXmlSimpleReader xmlReader;
	QXmlInputSource* source = new QXmlInputSource(file);
	TissueHierarchySaxHandler handler(root);
	xmlReader.setContentHandler(&handler);
	xmlReader.setErrorHandler(&handler);
	if (!xmlReader.parse(source))
	{
		delete root;
		return false;
	}

	// Add hierarchy to list
	hierarchyNames.push_back(handler.getHierarchyName());
	hierarchyTrees.push_back(root);

	return true;
}

bool TissueHiearchy::save_hierarchy_as(const QString& name, const QString& path)
{
	// Open xml file
	QFile file(path);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		return false; // TODO: Display error message
	}

	// Write xml
	QXmlStreamWriter xmlStream(&file);
	xmlStream.setAutoFormatting(true);
	xmlStream.writeStartDocument();
	xmlStream.writeStartElement("hierarchy");
	xmlStream.writeAttribute("name", name);
	std::vector<TissueHierarchyItem*>* topLevelChildren =
		hierarchyTrees[selectedHierarchy]->GetChildren();
	for (auto item : *topLevelChildren)
	{
		write_children_recursively(item, &xmlStream);
	}
	xmlStream.writeEndElement(); // hierarchy
	xmlStream.writeEndDocument();

	return true;
}

void TissueHiearchy::write_children_recursively(TissueHierarchyItem* parent,
												QXmlStreamWriter* xmlStream)
{
	if (parent->GetIsFolder())
	{
		// Write folder start element
		xmlStream->writeStartElement("folder");
		xmlStream->writeAttribute("name", parent->GetName());

		// Write children
		std::vector<TissueHierarchyItem*>* children = parent->GetChildren();
		for (std::vector<TissueHierarchyItem*>::iterator iter =
				 children->begin();
			 iter != children->end(); ++iter)
		{
			write_children_recursively(*iter, xmlStream);
		}

		// Write folder end element
		xmlStream->writeEndElement(); // folder
	}
	else
	{
		// Write tissue element
		xmlStream->writeStartElement("tissue");
		xmlStream->writeAttribute("name", parent->GetName());
		xmlStream->writeEndElement(); // tissue
	}
}

FILE* TissueHiearchy::SaveParams(FILE* fp, int version)
{
	if (version >= 10)
	{
		// Write number of hierarchies (excluding default)
		unsigned char len =
			static_cast<unsigned char>(hierarchyNames.size() - 1);
		fwrite(&len, sizeof(unsigned char), 1, fp);

		// Write hierarchies (excluding default)
		for (unsigned short i = 1; i <= len; ++i)
		{
			fp = save_hierarchy(fp, i);
		}

		// Write selected hierarchy
		len = selectedHierarchy;
		fwrite(&len, sizeof(unsigned char), 1, fp);
	}

	return fp;
}

FILE* TissueHiearchy::LoadParams(FILE* fp, int version)
{
	// Delete all hierarchies
	hierarchyNames.clear();
	for (unsigned int i = 0; i < hierarchyTrees.size(); ++i)
	{
		delete hierarchyTrees[i];
	}
	hierarchyTrees.clear();

	// Add default hierarchy
	hierarchyNames.push_back("(Default)");
	hierarchyTrees.push_back(create_default_hierarchy());

	if (version >= 10)
	{
		// Read number of hierarchies (excluding default)
		unsigned char len;
		fread(&len, sizeof(unsigned char), 1, fp);

		// Read hierarchies (excluding default)
		for (unsigned short i = 1; i <= len; ++i)
		{
			fp = load_hierarchy(fp);
		}

		// Read selected hierarchy
		fread(&len, sizeof(unsigned char), 1, fp);
		set_hierarchy(len);
	}
	else
	{
		set_hierarchy(0);
	}

	return fp;
}

FILE* TissueHiearchy::save_hierarchy(FILE* fp, unsigned short idx)
{
	// Write hierarchy name
	QString name = hierarchyNames[idx].append('\0');
	unsigned char len = (unsigned char)name.size();
	fwrite(&len, sizeof(unsigned char), 1, fp);
	fwrite(name.constData(), sizeof(QChar), len, fp);

	// Write root item
	TissueHierarchyItem* hierarchy = hierarchyTrees[idx];
	fp = save_hierarchy_item(fp, hierarchy);

	return fp;
}

FILE* TissueHiearchy::save_hierarchy_item(FILE* fp, TissueHierarchyItem* item)
{
	// Write folder flag
	unsigned char flag = (unsigned char)item->GetIsFolder();
	fwrite(&flag, sizeof(unsigned char), 1, fp);

	// Write item name
	QString name = item->GetName().append('\0');
	unsigned char len = (unsigned char)name.size();
	fwrite(&len, sizeof(unsigned char), 1, fp);
	fwrite(name.constData(), sizeof(QChar), len, fp);

	if (item->GetIsFolder())
	{
		// Write number of children
		unsigned int count = item->GetChildCount();
		fwrite(&count, sizeof(unsigned int), 1, fp);

		// Write folder children
		for (auto child : *item->GetChildren())
		{
			fp = save_hierarchy_item(fp, child);
		}
	}

	return fp;
}

FILE* TissueHiearchy::load_hierarchy(FILE* fp)
{
	// Read hierarchy name
	QChar name[256];
	unsigned char len;
	fread(&len, sizeof(unsigned char), 1, fp);
	fread(name, sizeof(QChar), len, fp);
	hierarchyNames.push_back(QString(name));

	// Read root item
	fp = load_hierarchy_item(fp, 0);

	return fp;
}

FILE* TissueHiearchy::load_hierarchy_item(FILE* fp, TissueHierarchyItem* parent)
{
	// Read folder flag
	unsigned char flag;
	fread(&flag, sizeof(unsigned char), 1, fp);
	bool isFolder = (bool)flag;

	// Read item name
	QChar name[256];
	unsigned char len;
	fread(&len, sizeof(unsigned char), 1, fp);
	fread(name, sizeof(QChar), len, fp);

	// Create item
	TissueHierarchyItem* item = nullptr;
	if (isFolder)
	{
		item = new TissueHierarchyItem(true, QString(name));
	}
	else
	{
		tissues_size_t type = TissueInfos::GetTissueType(ToStd(QString(name)));
		if (type <= 0)
		{
			// The tissue does not exist
			assert(type > 0);
			return fp;
		}
		item = new TissueHierarchyItem(false, QString(name));
	}

	if (parent == 0)
	{
		// Root folder
		hierarchyTrees.push_back(item);
	}
	else
	{
		parent->AddChild(item);
	}

	if (isFolder)
	{
		// Read number of children
		unsigned int count;
		fread(&count, sizeof(unsigned int), 1, fp);

		// Read folder children
		for (unsigned int i = 0; i < count; ++i)
		{
			fp = load_hierarchy_item(fp, item);
		}
	}

	return fp;
}

} // namespace iseg