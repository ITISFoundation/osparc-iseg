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

	bool startElement(const QString& namespaceURI, const QString& localName, const QString& qName, const QXmlAttributes& attributes) override;

	bool endElement(const QString& namespaceURI, const QString& localName, const QString& qName) override;

	bool fatalError(const QXmlParseException& exception) override;

	QString GetHierarchyName();

private:
	TissueHierarchyItem* m_CurrentFolder;
	QString m_CurrentText;
	QString m_LoadedHierarchyName;
};

TissueHierarchySaxHandler::TissueHierarchySaxHandler(TissueHierarchyItem* root)
{
	m_CurrentFolder = root;
	m_LoadedHierarchyName = QString();
}

bool TissueHierarchySaxHandler::startElement(const QString& /* namespaceURI */, const QString& /* localName */, const QString& qName, const QXmlAttributes& attributes)
{
	if (qName == "hierarchy")
	{
		m_LoadedHierarchyName = attributes.value("name");
	}
	else if (qName == "tissue")
	{
		// Add tissue with given name
		QString tissue_name(attributes.value("name"));
		m_CurrentFolder->AddChild(new TissueHierarchyItem(false, tissue_name));
	}
	else if (qName == "folder")
	{
		// Add as subfolder to last folder
		QString folder_name(attributes.value("name"));
		TissueHierarchyItem* last_folder = m_CurrentFolder;
		m_CurrentFolder = new TissueHierarchyItem(true, folder_name);
		last_folder->AddChild(m_CurrentFolder);
	}
	return true;
}

bool TissueHierarchySaxHandler::endElement(const QString& /* namespaceURI */, const QString& /* localName */, const QString& qName)
{
	if (qName == "folder")
	{
		// Set current folder to parent
		m_CurrentFolder = m_CurrentFolder->GetParent();
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

QString TissueHierarchySaxHandler::GetHierarchyName()
{
	return m_LoadedHierarchyName;
}
} // namespace

//////////////////////////////////////////////////////////////////////////

TissueHierarchyItem::TissueHierarchyItem(bool folderItem, const QString& itemName)
{
	m_IsFolder = folderItem;
	m_Name = itemName;
	m_Parent = nullptr;
}

TissueHierarchyItem::~TissueHierarchyItem() { m_Children.clear(); }

bool TissueHierarchyItem::GetIsFolder() const { return m_IsFolder; }

QString TissueHierarchyItem::GetName() { return m_Name; }

unsigned int TissueHierarchyItem::GetChildCount()
{
	return static_cast<unsigned int>(m_Children.size());
}

TissueHierarchyItem* TissueHierarchyItem::GetParent() { return m_Parent; }

std::vector<TissueHierarchyItem*>* TissueHierarchyItem::GetChildren()
{
	return &m_Children;
}

void TissueHierarchyItem::AddChild(TissueHierarchyItem* child)
{
	child->SetParent(this);
	m_Children.push_back(child);
}

void TissueHierarchyItem::UpdateTissuesRecursively()
{
	// Remove tissues which have been deleted from the tissue list
	for (std::vector<TissueHierarchyItem*>::iterator iter = m_Children.begin();
			 iter != m_Children.end();)
	{
		TissueHierarchyItem* curr_child = *iter;
		if (curr_child->GetIsFolder())
		{
			// Recursion with current folder child
			curr_child->UpdateTissuesRecursively();
			++iter;
		}
		else if (!TissueInfos::GetTissueType(ToStd(curr_child->GetName())))
		{
			// Delete tissue
			iter = m_Children.erase(iter);
		}
		else
		{
			++iter;
		}
	}
}

void TissueHierarchyItem::UpdateTissueNameRecursively(const QString& oldName, const QString& newName)
{
	// Update tissue name
	if (!m_IsFolder && m_Name.compare(oldName) == 0)
	{
		m_Name = newName;
	}

	// Recursion with children
	for (std::vector<TissueHierarchyItem*>::iterator iter = m_Children.begin();
			 iter != m_Children.end(); ++iter)
	{
		(*iter)->UpdateTissueNameRecursively(oldName, newName);
	}
}

void TissueHierarchyItem::SetParent(TissueHierarchyItem* parentItem)
{
	m_Parent = parentItem;
}

//////////////////////////////////////////////////////////////////////////

void TissueHiearchy::Initialize()
{
	// Clear internal representations
	m_HierarchyNames.clear();
	for (unsigned int i = 0; i < m_HierarchyTrees.size(); ++i)
	{
		delete m_HierarchyTrees[i];
	}
	m_HierarchyTrees.clear();

	// Create and select default hierarchy
	m_HierarchyNames.push_back("(Default)");
	m_HierarchyTrees.push_back(CreateDefaultHierarchy());
	SetHierarchy(0);
}

TissueHierarchyItem* TissueHiearchy::CreateDefaultHierarchy()
{
	// Create internal representation of default hierarchy
	return new TissueHierarchyItem(true, QString("root"));
}

TissueHierarchyItem* TissueHiearchy::SelectedHierarchy()
{
	if (m_SelectedHierarchy < m_HierarchyTrees.size())
	{
		return m_HierarchyTrees.at(m_SelectedHierarchy);
	}
	return nullptr;
}

void TissueHiearchy::SetSelectedHierarchy(TissueHierarchyItem* h)
{
	if (h != m_HierarchyTrees[m_SelectedHierarchy])
	{
		delete m_HierarchyTrees[m_SelectedHierarchy];
		m_HierarchyTrees[m_SelectedHierarchy] = h;
	}
}

bool TissueHiearchy::RemoveCurrentHierarchy()
{
	if (m_SelectedHierarchy == 0)
	{
		return false;
	}
	m_HierarchyNames.erase(m_HierarchyNames.begin() + m_SelectedHierarchy);
	delete m_HierarchyTrees[m_SelectedHierarchy];
	m_HierarchyTrees.erase(m_HierarchyTrees.begin() + m_SelectedHierarchy);

	// Set default hierarchy
	SetHierarchy(0);
	return true;
}

bool TissueHiearchy::SetHierarchy(unsigned short index)
{
	if (index < 0 || index >= m_HierarchyTrees.size())
	{
		return false;
	}

	m_SelectedHierarchy = index;
	return true;
}

unsigned short TissueHiearchy::GetSelectedHierarchy() const
{
	return m_SelectedHierarchy;
}

unsigned short TissueHiearchy::GetHierarchyCount()
{
	return static_cast<unsigned short>(m_HierarchyNames.size());
}

std::vector<QString>* TissueHiearchy::GetHierarchyNamesPtr()
{
	return &m_HierarchyNames;
}

QString TissueHiearchy::GetCurrentHierarchyName()
{
	return m_HierarchyNames[m_SelectedHierarchy];
}

void TissueHiearchy::ResetDefaultHierarchy()
{
	delete m_HierarchyTrees[0];
	m_HierarchyTrees[0] = CreateDefaultHierarchy();
}

void TissueHiearchy::AddNewHierarchy(const QString& name)
{
	AddNewHierarchy(name, CreateDefaultHierarchy());
}

void TissueHiearchy::AddNewHierarchy(const QString& name, TissueHierarchyItem* hierarchy)
{
	// Create and select default hierarchy
	m_HierarchyNames.push_back(name);
	m_HierarchyTrees.push_back(hierarchy);
	SetHierarchy(static_cast<unsigned short>(m_HierarchyTrees.size() - 1));
}

void TissueHiearchy::UpdateHierarchies()
{
	for (unsigned int i = 0; i < m_HierarchyTrees.size(); ++i)
	{
		if (i != m_SelectedHierarchy)
		{
			m_HierarchyTrees[i]->UpdateTissuesRecursively();
		}
	}
}

bool TissueHiearchy::LoadHierarchy(const QString& path)
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
	QXmlSimpleReader xml_reader;
	QXmlInputSource* source = new QXmlInputSource(file);
	TissueHierarchySaxHandler handler(root);
	xml_reader.setContentHandler(&handler);
	xml_reader.setErrorHandler(&handler);
	if (!xml_reader.parse(source))
	{
		delete root;
		return false;
	}

	// Add hierarchy to list
	m_HierarchyNames.push_back(handler.GetHierarchyName());
	m_HierarchyTrees.push_back(root);

	return true;
}

bool TissueHiearchy::SaveHierarchyAs(const QString& name, const QString& path)
{
	// Open xml file
	QFile file(path);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		return false; // TODO: Display error message
	}

	// Write xml
	QXmlStreamWriter xml_stream(&file);
	xml_stream.setAutoFormatting(true);
	xml_stream.writeStartDocument();
	xml_stream.writeStartElement("hierarchy");
	xml_stream.writeAttribute("name", name);
	std::vector<TissueHierarchyItem*>* top_level_children =
			m_HierarchyTrees[m_SelectedHierarchy]->GetChildren();
	for (auto item : *top_level_children)
	{
		WriteChildrenRecursively(item, &xml_stream);
	}
	xml_stream.writeEndElement(); // hierarchy
	xml_stream.writeEndDocument();

	return true;
}

void TissueHiearchy::WriteChildrenRecursively(TissueHierarchyItem* parent, QXmlStreamWriter* xmlStream)
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
			WriteChildrenRecursively(*iter, xmlStream);
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
				static_cast<unsigned char>(m_HierarchyNames.size() - 1);
		fwrite(&len, sizeof(unsigned char), 1, fp);

		// Write hierarchies (excluding default)
		for (unsigned short i = 1; i <= len; ++i)
		{
			fp = SaveHierarchy(fp, i);
		}

		// Write selected hierarchy
		len = m_SelectedHierarchy;
		fwrite(&len, sizeof(unsigned char), 1, fp);
	}

	return fp;
}

FILE* TissueHiearchy::LoadParams(FILE* fp, int version)
{
	// Delete all hierarchies
	m_HierarchyNames.clear();
	for (unsigned int i = 0; i < m_HierarchyTrees.size(); ++i)
	{
		delete m_HierarchyTrees[i];
	}
	m_HierarchyTrees.clear();

	// Add default hierarchy
	m_HierarchyNames.push_back("(Default)");
	m_HierarchyTrees.push_back(CreateDefaultHierarchy());

	if (version >= 10)
	{
		// Read number of hierarchies (excluding default)
		unsigned char len;
		fread(&len, sizeof(unsigned char), 1, fp);

		// Read hierarchies (excluding default)
		for (unsigned short i = 1; i <= len; ++i)
		{
			fp = LoadHierarchy(fp);
		}

		// Read selected hierarchy
		fread(&len, sizeof(unsigned char), 1, fp);
		SetHierarchy(len);
	}
	else
	{
		SetHierarchy(0);
	}

	return fp;
}

FILE* TissueHiearchy::SaveHierarchy(FILE* fp, unsigned short idx)
{
	// Write hierarchy name
	QString name = m_HierarchyNames[idx].append('\0');
	unsigned char len = (unsigned char)name.size();
	fwrite(&len, sizeof(unsigned char), 1, fp);
	fwrite(name.constData(), sizeof(QChar), len, fp);

	// Write root item
	TissueHierarchyItem* hierarchy = m_HierarchyTrees[idx];
	fp = SaveHierarchyItem(fp, hierarchy);

	return fp;
}

FILE* TissueHiearchy::SaveHierarchyItem(FILE* fp, TissueHierarchyItem* item)
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
			fp = SaveHierarchyItem(fp, child);
		}
	}

	return fp;
}

FILE* TissueHiearchy::LoadHierarchy(FILE* fp)
{
	// Read hierarchy name
	QChar name[256];
	unsigned char len;
	fread(&len, sizeof(unsigned char), 1, fp);
	fread(name, sizeof(QChar), len, fp);
	m_HierarchyNames.push_back(QString(name));

	// Read root item
	fp = LoadHierarchyItem(fp, nullptr);

	return fp;
}

FILE* TissueHiearchy::LoadHierarchyItem(FILE* fp, TissueHierarchyItem* parent)
{
	// Read folder flag
	unsigned char flag;
	fread(&flag, sizeof(unsigned char), 1, fp);
	bool is_folder = (bool)flag;

	// Read item name
	QChar name[256];
	unsigned char len;
	fread(&len, sizeof(unsigned char), 1, fp);
	fread(name, sizeof(QChar), len, fp);

	// Create item
	TissueHierarchyItem* item = nullptr;
	if (is_folder)
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

	if (parent == nullptr)
	{
		// Root folder
		m_HierarchyTrees.push_back(item);
	}
	else
	{
		parent->AddChild(item);
	}

	if (is_folder)
	{
		// Read number of children
		unsigned int count;
		fread(&count, sizeof(unsigned int), 1, fp);

		// Read folder children
		for (unsigned int i = 0; i < count; ++i)
		{
			fp = LoadHierarchyItem(fp, item);
		}
	}

	return fp;
}

} // namespace iseg
