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

#include "XdmfImageReader.h"

#include "Data/ScopedTimer.h"
#include "Data/Transform.h"

#include "Core/ColorLookupTable.h"
#include "Core/HDF5Reader.h"

#include <boost/algorithm/string/replace.hpp>

#include <QDir>
#include <QDomDocument>
#include <QDomElement>
#include <QDomText>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QTextStream>

#include <vtkSmartPointer.h>

#include <cassert>
#include <stdexcept>
#include <vector>

namespace iseg {

namespace {
int _Read(HDF5Reader& reader, size_t NumberOfSlices, size_t Width, size_t Height, bool ReadContiguousMemory, const std::string& source_dname, float** ImageSlices, const std::string& target_dname, float** WorkSlices, const std::string& tissue_dname, tissues_size_t** TissueSlices)
{
	if (!reader.Exists(source_dname))
	{
		ISEG_WARNING_MSG("no Source array, will initialize to 0...");
		for (int k = 0; k < NumberOfSlices; k++)
		{
			size_t pos = 0;
			for (int j = 0; j < Height; j++)
			{
				for (int i = 0; i < Width; i++, pos++)
				{
					ImageSlices[k][pos] = 0.0f;
				}
			}
		}
	}
	if (!reader.Exists(target_dname))
	{
		ISEG_WARNING_MSG("no Target array, will initialize to 0...");
		for (int k = 0; k < NumberOfSlices; k++)
		{
			size_t pos = 0;
			for (int j = 0; j < Height; j++)
			{
				for (int i = 0; i < Width; i++, pos++)
				{
					WorkSlices[k][pos] = 0.0f;
				}
			}
		}
	}
	if (!reader.Exists(tissue_dname))
	{
		ISEG_WARNING_MSG("no Tissue array, will initialize to 0...");
		for (int k = 0; k < NumberOfSlices; k++)
		{
			size_t pos = 0;
			for (int j = 0; j < Height; j++)
			{
				for (int i = 0; i < Width; i++, pos++)
				{
					TissueSlices[k][pos] = 0;
				}
			}
		}
	}

	if (ReadContiguousMemory)
	{
		// allocate
		size_t const n = NumberOfSlices * Width * Height;
		std::vector<float> buffer_float;
		try
		{
			ISEG_DEBUG("N = " << n << ", bufferFloat.max_size() = " << buffer_float.max_size());
			buffer_float.resize(n);
		}
		catch (std::length_error& le)
		{
			std::cerr << "bufferFloat length error: " << le.what();
			return 0;
		}

		// Source
		if (reader.Exists(source_dname))
		{
			ScopedTimer timer("Read Source");
			if (!reader.Read(&buffer_float[0], source_dname))
			{
				ISEG_ERROR_MSG("reading Source dataset...");
			}
			else
			{
				size_t n = 0;
				for (int k = 0; k < NumberOfSlices; k++)
				{
					size_t pos = 0;
					for (int j = 0; j < Height; j++)
					{
						for (int i = 0; i < Width; i++, pos++)
						{
							ImageSlices[k][pos] = buffer_float[n];
							n++;
						}
					}
				}
			}
		}

		// Target
		if (reader.Exists(target_dname))
		{
			ScopedTimer timer("Read Target");
			if (!reader.Read(&buffer_float[0], target_dname))
			{
				ISEG_ERROR_MSG("reading Target dataset...");
			}
			else
			{
				size_t n = 0;
				for (int k = 0; k < NumberOfSlices; k++)
				{
					size_t pos = 0;
					for (int j = 0; j < Height; j++)
					{
						for (int i = 0; i < Width; i++, pos++)
						{
							WorkSlices[k][pos] = buffer_float[n];
							n++;
						}
					}
				}
			}
		}
		buffer_float.clear();

		// Tissue
		if (reader.Exists(tissue_dname))
		{
			ScopedTimer timer("Read Tissue");
			std::string type;
			std::vector<HDF5Reader::size_type> dims;
			if (!reader.GetDatasetInfo(type, dims, tissue_dname))
			{
				ISEG_ERROR_MSG("reading Tissue data type...");
				return 0;
			}

			if (type == "unsigned char")
			{
				std::vector<unsigned char> buffer_u_char;
				try
				{
					// vector throws a length_error if resized above max_size
					// vector<float> bufferFloat(N);
					ISEG_DEBUG("N = " << n << ", bufferUChar.max_size() = " << buffer_u_char.max_size());
					buffer_u_char.resize(n);
				}
				catch (std::length_error& le)
				{
					ISEG_ERROR("bufferUChar length error: " << le.what());
					return 0;
				}
				if (!reader.Read(&buffer_u_char[0], tissue_dname))
				{
					ISEG_ERROR_MSG("reading Tissue dataset...");
					return 0;
				}
				size_t n = 0;
				for (int k = 0; k < NumberOfSlices; k++)
				{
					size_t pos = 0;
					for (int j = 0; j < Height; j++)
					{
						for (int i = 0; i < Width; i++, pos++)
						{
							TissueSlices[k][pos] =
									(tissues_size_t)buffer_u_char[n];
							n++;
						}
					}
				}
				buffer_u_char.clear();
			}
			else if (type == "unsigned short")
			{
				std::vector<unsigned short> buffer_u_short;
				try
				{
					// vector throws a length_error if resized above max_size
					// vector<float> bufferFloat(N);
					ISEG_DEBUG("N = " << n << ", bufferUShort.max_size() = " << buffer_u_short.max_size());
					buffer_u_short.resize(n);
				}
				catch (std::length_error& le)
				{
					ISEG_ERROR("bufferUShort length error: " << le.what());
					return 0;
				}
				if (!reader.Read(&buffer_u_short[0], tissue_dname))
				{
					ISEG_ERROR_MSG("reading Tissue dataset...");
					return 0;
				}
				size_t n = 0;
				for (int k = 0; k < NumberOfSlices; k++)
				{
					size_t pos = 0;
					for (int j = 0; j < Height; j++)
					{
						for (int i = 0; i < Width; i++, pos++)
						{
							TissueSlices[k][pos] =
									(tissues_size_t)buffer_u_short[n];
							n++;
						}
					}
				}
				buffer_u_short.clear();
			}
			else if (type == "unsigned int")
			{
				std::vector<unsigned int> buffer_u_int;
				try
				{
					// vector throws a length_error if resized above max_size
					// vector<float> bufferFloat(N);
					ISEG_DEBUG("N = " << n << ", bufferUInt.max_size() = " << buffer_u_int.max_size());
					buffer_u_int.resize(n);
				}
				catch (std::length_error& le)
				{
					ISEG_ERROR("bufferUInt length error: " << le.what());
					return 0;
				}
				if (!reader.Read(&buffer_u_int[0], tissue_dname))
				{
					ISEG_ERROR_MSG("reading Tissue dataset...");
					return 0;
				}
				size_t n = 0;
				for (int k = 0; k < NumberOfSlices; k++)
				{
					size_t pos = 0;
					for (int j = 0; j < Height; j++)
					{
						for (int i = 0; i < Width; i++, pos++)
						{
							TissueSlices[k][pos] =
									(tissues_size_t)buffer_u_int[n];
							n++;
						}
					}
				}
				buffer_u_int.clear();
			}
			else
			{
				ISEG_ERROR_MSG("Tissue data type not supported...");
				return 0;
			}
		}
	}
	else
	{
		size_t const slice_size = Width * Height;

		if (reader.Exists(source_dname))
		{
			ScopedTimer timer("Read Source");
			bool ok = true;
			size_t offset = 0;
			for (int k = 0; k < NumberOfSlices; k++, offset += slice_size)
			{
				ok = ok && reader.Read(ImageSlices[k], offset, slice_size, source_dname);
			}
		}
		if (reader.Exists(target_dname))
		{
			ScopedTimer timer("Read Target");
			bool ok = true;
			size_t offset = 0;
			for (int k = 0; k < NumberOfSlices; k++, offset += slice_size)
			{
				ok = ok && reader.Read(WorkSlices[k], offset, slice_size, target_dname);
			}
		}
		if (reader.Exists(tissue_dname))
		{
			ScopedTimer timer("Read Tissue");
			bool ok = true;
			size_t offset = 0;
			for (int k = 0; k < NumberOfSlices; k++, offset += slice_size)
			{
				ok = ok && reader.Read(TissueSlices[k], offset, slice_size, tissue_dname);
			}
		}
	}

	return 1;
}
} // namespace

XdmfImageReader::XdmfImageReader()
{
	this->m_NumberOfSlices = 0;
	this->m_Width = 0;
	this->m_Height = 0;
	this->m_Compression = 1;
	this->m_ImageTransform.SetIdentity();
	this->m_ImageSlices = nullptr;
	this->m_WorkSlices = nullptr;
	this->m_TissueSlices = nullptr;
	this->m_FileName = nullptr;
	this->m_ReadContiguousMemory = false;
}

XdmfImageReader::~XdmfImageReader() { delete[] this->m_FileName; }

int XdmfImageReader::ParseXML()
{
	QFile file(m_FileName);
	if (!file.open(QIODevice::ReadOnly))
	{
		ISEG_ERROR("cannot open " << m_FileName);
		return 0;
	}

	QDomDocument input_document("Slices");
	QByteArray input_content = file.readAll();
	file.close();

	QString msg;
	if (!input_document.setContent(input_content, false, &msg)) // This function is not reentrant.
	{
		ISEG_ERROR("assigning content of " << m_FileName << ": " << msg.toStdString());
	}

	QDomElement root = input_document.documentElement();
	if (root.tagName() != "Xdmf")
	{
		ISEG_ERROR("invalid root element: " << root.tagName().toStdString());
	}

	QDomElement domain = root.firstChildElement(QString("Domain"));
	QDomNode n = domain.firstChild();
	while (!n.isNull())
	{
		QDomElement e = n.toElement();
		if (e.tagName() == "Grid")
		{
			QString type = e.attribute(QString("Type"));
			if (type.isEmpty())
				type = e.attribute(QString("GridType"));
			if (type != QString("Uniform"))
			{
				ISEG_ERROR("unsupported grid type: " << type.toStdString());
				return 0;
			}

			// Geometry
			QDomElement geometry = e.firstChildElement(QString("Geometry"));
			if (geometry.attribute(QString("Type")) !=
							QString("ORIGIN_DXDYDZ") &&
					geometry.attribute(QString("GeometryType")) !=
							QString("ORIGIN_DXDYDZ"))
			{
				ISEG_ERROR_MSG("unsupported geometry type...");
				return 0;
			}
			QDomNodeList list2 = geometry.elementsByTagName("DataItem");
			for (int i2 = 0; i2 < list2.size(); ++i2)
			{
				const QDomElement e2 = list2.item(i2).toElement();
				QString name = e2.attribute(QString("Name"));
				if (name == QString("Origin"))
				{
					QString text = e2.text().trimmed();
					QStringList text_list = text.split(QString(" "));
					if (text_list.size() != 3)
					{
						ISEG_ERROR_MSG("invalid origin...");
						return 0;
					}
					float offset[3];
					for (int i3 = 0; i3 < text_list.size(); ++i3)
						offset[2 - i3] = text_list[i3].toFloat();
					m_ImageTransform.SetOffset(offset);
				}
				else if (name == QString("Spacing"))
				{
					QString text = e2.text().trimmed();
					QStringList text_list = text.split(QString(" "));
					if (text_list.size() != 3)
					{
						ISEG_ERROR_MSG("invalid spacing...");
						return 0;
					}
					for (int i3 = 0; i3 < text_list.size(); ++i3)
						m_PixelSize[2 - i3] = text_list[i3].toFloat();
				}
				else
				{
					ISEG_ERROR_MSG("expecting origin and spacing...");
					return 0;
				}
			}

			// Topology
			QDomElement topology = e.firstChildElement(QString("Topology"));
			if (topology.attribute(QString("Type")) !=
							QString("3DCORECTMesh") &&
					topology.attribute(QString("TopologyType")) !=
							QString("3DCORECTMesh"))
			{
				ISEG_ERROR_MSG("unsupported topology type...");
				return 0;
			}
			QStringList text_list =
					topology.attribute(QString("Dimensions")).split(QString(" "));
			if (text_list.size() != 3)
			{
				ISEG_ERROR_MSG("invalid dimensions...");
				return 0;
			}
			this->m_Width = text_list[2].toInt();
			this->m_Height = text_list[1].toInt();
			this->m_NumberOfSlices = text_list[0].toInt();

			// Attributes
			QDomNodeList attribute_list = e.elementsByTagName("Attribute");
			for (int i2 = 0; i2 < attribute_list.size(); ++i2)
			{
				const QDomElement scalar = attribute_list.item(i2).toElement();
				QString name = scalar.attribute(QString("Name"));
				this->m_ArrayNames.append(name);
				QDomElement dataitem =
						scalar.firstChildElement(QString("DataItem"));
				QString text = dataitem.text().trimmed();
				int idx = text.indexOf(":");
				QString datasetname = text.remove(0, idx + 1);
				this->m_MapArrayNames[name] = datasetname;
			}

		} // if(e.tagName()=="Grid")

		n = n.nextSibling();
	}

	return 1;
}

// BL TODO read rotation & origin from HDF5 part, if available
int XdmfImageReader::Read()
{
	ISEG_INFO("Reading " << this->m_FileName << ": " << m_Width << " x " << m_Height << " x " << m_NumberOfSlices);

	QString q_file_name(this->m_FileName);
	QFileInfo file_info(q_file_name);
	QString basename = file_info.completeBaseName();

	// save working directory
	QDir oldcwd = QDir::current();

	// enter the xmf file folder so relative names for hdf5 files work
	QDir::setCurrent(file_info.absolutePath());

	HDF5Reader reader;
	const QString fname = basename + ".h5";
	if (!reader.Open(fname.toStdString()))
	{
		ISEG_ERROR("opening " << fname.toStdString());
		return 0;
	}

	int r = _Read(reader, m_NumberOfSlices, m_Width, m_Height, m_ReadContiguousMemory, this->m_MapArrayNames["Source"].toStdString().c_str(), m_ImageSlices, this->m_MapArrayNames["Target"].toStdString().c_str(), m_WorkSlices, this->m_MapArrayNames["Tissue"].toStdString().c_str(), m_TissueSlices);

	reader.Close();

	// restore working directory
	QDir::setCurrent(oldcwd.absolutePath());

	return r;
}

std::shared_ptr<ColorLookupTable> XdmfImageReader::ReadColorLookup() const
{
	std::string fname(this->m_FileName);
	boost::replace_last(fname, ".xmf", ".h5");

	HDFImageReader reader;
	reader.SetFileName(fname.c_str());

	return reader.ReadColorLookup();
}

HDFImageReader::HDFImageReader()
{
	this->m_NumberOfSlices = 0;
	this->m_Width = 0;
	this->m_Height = 0;
	this->m_Compression = 1;
	this->m_ImageSlices = nullptr;
	this->m_WorkSlices = nullptr;
	this->m_TissueSlices = nullptr;
	this->m_FileName = nullptr;
	this->m_ReadContiguousMemory = false;
}

HDFImageReader::~HDFImageReader() { delete[] this->m_FileName; }

int HDFImageReader::ParseHDF()
{
	std::cerr << "parsing file " << this->m_FileName << endl;
	HDF5Reader reader;
	if (!reader.Open(m_FileName))
	{
		ISEG_ERROR("opening " << m_FileName);
		return 0;
	}

	bool ok = true;
	std::vector<int> dimensions;
	dimensions.resize(3);
	ok = ok && (reader.Read(dimensions, "/dimensions") != 0) &&
			 (dimensions.size() == 3);
	this->m_Width = dimensions[0];
	this->m_Height = dimensions[1];
	this->m_NumberOfSlices = dimensions[2];

	std::vector<float> pixelsize(3);
	ok = ok && (reader.Read(pixelsize, "/pixelsize") != 0) &&
			 (pixelsize.size() == 3);
	for (unsigned short i = 0; i < 3; i++)
		m_PixelSize[i] = pixelsize[i];

	std::vector<float> offset(3);
	ok = ok && (reader.Read(offset, "/offset") != 0) && (offset.size() == 3);

	if (reader.Exists("/rotation"))
	{
		std::vector<float> rotation(9);
		ok = ok && (reader.Read(rotation, "/rotation") != 0) &&
				 (rotation.size() == 9);

		m_ImageTransform.SetIdentity();
		m_ImageTransform.SetOffset(offset.data());

		for (int k = 0; k < 3; ++k)
		{
			m_ImageTransform[k][0] = rotation[k * 3 + 0];
			m_ImageTransform[k][1] = rotation[k * 3 + 1];
			m_ImageTransform[k][2] = rotation[k * 3 + 2];
		}
	}
	else
	{
		std::vector<float> dc(6);
		ok = ok && (reader.Read(dc, "/dc") != 0) && (dc.size() == 6);

		m_ImageTransform.SetTransform(offset.data(), dc.data());
	}

	//Transform

	if (reader.Exists("/Source"))
	{
		m_MapArrayNames["Source"] = QString("/Source");
		m_ArrayNames.append(QString("Source"));
	}
	if (reader.Exists("/Target"))
	{
		m_MapArrayNames["Target"] = QString("/Target");
		m_ArrayNames.append(QString("Target"));
	}
	if (reader.Exists("/Tissue"))
	{
		m_MapArrayNames["Tissue"] = QString("/Tissue");
		m_ArrayNames.append(QString("Tissue"));
	}

	reader.Close();

	assert(ok);
	return ok ? 1 : 0;
}

int HDFImageReader::Read()
{
	ISEG_INFO("Reading " << this->m_FileName << ": " << m_Width << " x " << m_Height << " x " << m_NumberOfSlices);

	//	vector<int> dims;

	QString q_file_name(this->m_FileName);
	QFileInfo file_info(q_file_name);
	QString basename = file_info.completeBaseName();
	QString suffix = file_info.suffix();

	// save working directory
	QDir oldcwd = QDir::current();

	// enter the xmf file folder so relative names for hdf5 files work
	QDir::setCurrent(file_info.absolutePath());

	HDF5Reader reader;
	const QString fname = basename + "." + suffix;
	if (!reader.Open(fname.toStdString().c_str()))
	{
		ISEG_ERROR("opening " << fname.toStdString());
		return 0;
	}

	_Read(reader, m_NumberOfSlices, m_Width, m_Height, m_ReadContiguousMemory, this->m_MapArrayNames["Source"].toStdString().c_str(), m_ImageSlices, this->m_MapArrayNames["Target"].toStdString().c_str(), m_WorkSlices, this->m_MapArrayNames["Tissue"].toStdString().c_str(), m_TissueSlices);

	reader.Close();

	// restore working directory
	QDir::setCurrent(oldcwd.absolutePath());

	return 1;
}

std::shared_ptr<ColorLookupTable> HDFImageReader::ReadColorLookup() const
{
	ScopedTimer timer("ReadColorLookup");

	std::shared_ptr<ColorLookupTable> color_lookup_table;

	QString q_file_name(this->m_FileName);
	QFileInfo file_info(q_file_name);
	QString basename = file_info.completeBaseName();
	QString suffix = file_info.suffix();

	// save working directory
	QDir oldcwd = QDir::current();
	QDir::setCurrent(file_info.absolutePath());

	HDF5Reader reader;
	const QString fname = basename + "." + suffix;
	if (!reader.Open(fname.toStdString()))
	{
		ISEG_ERROR("opening " << fname.toStdString());
		return nullptr;
	}

	bool ok = true;
	if (reader.Exists("/Lut"))
	{
		int version = -1, num_colors = 0;
		ok = ok && (reader.Read(&version, "/Lut/version") != 0);
		ok = ok && (reader.Read(&num_colors, "/Lut/size") != 0);

		if (version == 1)
		{
			std::vector<std::string> colors = reader.GetGroupInfo("/Lut");
			if (!colors.empty())
			{
				int index;
				float float_rgb[3];
				unsigned char rgb[3];

				ok = ok && (colors.size() == num_colors + 2);
				if (!ok)
				{
					ISEG_ERROR_MSG("could not load color lookup table");
					return nullptr;
				}
				if (version > 1)
				{
					ISEG_ERROR_MSG("Error: could not load color lookup table. The file "
												 "format is newer than this iSEG.");
					return nullptr;
				}

				color_lookup_table = std::make_shared<ColorLookupTable>();
				color_lookup_table->SetNumberOfColors(num_colors);

				for (const auto& name : colors)
				{
					if (ok && name.find("color") != std::string::npos)
					{
						std::string const folder_name = "/Lut/" + name;

						ok = ok &&
								 (reader.Read(&index, folder_name + "/index") != 0);
						ok = ok &&
								 (reader.Read(float_rgb, folder_name + "/rgb") != 0);

						rgb[0] = static_cast<unsigned char>(float_rgb[0] * 255.0);
						rgb[1] = static_cast<unsigned char>(float_rgb[1] * 255.0);
						rgb[2] = static_cast<unsigned char>(float_rgb[2] * 255.0);
						color_lookup_table->SetColor(index, rgb);
					}
				}
			}
		}
		else if (version == 2)
		{
			std::vector<float> colors(3 * num_colors);
			ok = ok && (reader.Read(colors.data(), "/Lut/colors") != 0);
			if (ok)
			{
				color_lookup_table = std::make_shared<ColorLookupTable>();
				color_lookup_table->SetNumberOfColors(num_colors);

				unsigned char rgb[3];
				for (int i = 0; i < num_colors; ++i)
				{
					rgb[0] = static_cast<unsigned char>(colors[i * 3 + 0] * 255.0);
					rgb[1] = static_cast<unsigned char>(colors[i * 3 + 1] * 255.0);
					rgb[2] = static_cast<unsigned char>(colors[i * 3 + 2] * 255.0);
					color_lookup_table->SetColor(static_cast<size_t>(i), rgb);
				}
			}
		}
		else if (version == 3)
		{
			std::vector<unsigned char> colors(3 * num_colors);
			ok = ok && (reader.Read(colors.data(), "/Lut/colors") != 0);
			if (ok)
			{
				color_lookup_table = std::make_shared<ColorLookupTable>();
				color_lookup_table->SetNumberOfColors(num_colors);

				for (int i = 0; i < num_colors; ++i)
				{
					color_lookup_table->SetColor(static_cast<size_t>(i), &colors[i * 3]);
				}
			}
		}
		else
		{
			ISEG_ERROR_MSG("color lookup table was written with a newer version of iSEG");
		}
	}

	QDir::setCurrent(oldcwd.absolutePath());

	return color_lookup_table;
}

} // namespace iseg
