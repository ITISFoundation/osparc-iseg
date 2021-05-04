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

#include "XdmfImageReader.h"

#include "Data/Transform.h"
#include "Data/ScopedTimer.h"

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
int _Read(HDF5Reader& reader, size_t NumberOfSlices, size_t Width,
		size_t Height, bool ReadContiguousMemory,
		const std::string& source_dname, float** ImageSlices,
		const std::string& target_dname, float** WorkSlices,
		const std::string& tissue_dname, tissues_size_t** TissueSlices)
{
	if (!reader.exists(source_dname))
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
	if (!reader.exists(target_dname))
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
	if (!reader.exists(tissue_dname))
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
		size_t const N = NumberOfSlices * Width * Height;
		std::vector<float> bufferFloat;
		try
		{
			ISEG_DEBUG("N = " << N << ", bufferFloat.max_size() = " << bufferFloat.max_size());
			bufferFloat.resize(N);
		}
		catch (std::length_error& le)
		{
			std::cerr << "bufferFloat length error: " << le.what();
			return 0;
		}

		// Source
		if (reader.exists(source_dname))
		{
			ScopedTimer timer("Read Source");
			if (!reader.read(&bufferFloat[0], source_dname))
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
							ImageSlices[k][pos] = bufferFloat[n];
							n++;
						}
					}
				}
			}
		}

		// Target
		if (reader.exists(target_dname))
		{
			ScopedTimer timer("Read Target");
			if (!reader.read(&bufferFloat[0], target_dname))
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
							WorkSlices[k][pos] = bufferFloat[n];
							n++;
						}
					}
				}
			}
		}
		bufferFloat.clear();

		// Tissue
		if (reader.exists(tissue_dname))
		{
			ScopedTimer timer("Read Tissue");
			std::string type;
			std::vector<HDF5Reader::size_type> dims;
			if (!reader.getDatasetInfo(type, dims, tissue_dname))
			{
				ISEG_ERROR_MSG("reading Tissue data type...");
				return 0;
			}

			if (type.compare("unsigned char") == 0)
			{
				std::vector<unsigned char> bufferUChar;
				try
				{
					// vector throws a length_error if resized above max_size
					// vector<float> bufferFloat(N);
					ISEG_DEBUG("N = " << N << ", bufferUChar.max_size() = " << bufferUChar.max_size());
					bufferUChar.resize(N);
				}
				catch (std::length_error& le)
				{
					ISEG_ERROR("bufferUChar length error: " << le.what());
					return 0;
				}
				if (!reader.read(&bufferUChar[0], tissue_dname))
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
									(tissues_size_t)bufferUChar[n];
							n++;
						}
					}
				}
				bufferUChar.clear();
			}
			else if (type.compare("unsigned short") == 0)
			{
				std::vector<unsigned short> bufferUShort;
				try
				{
					// vector throws a length_error if resized above max_size
					// vector<float> bufferFloat(N);
					ISEG_DEBUG("N = " << N << ", bufferUShort.max_size() = " << bufferUShort.max_size());
					bufferUShort.resize(N);
				}
				catch (std::length_error& le)
				{
					ISEG_ERROR("bufferUShort length error: " << le.what());
					return 0;
				}
				if (!reader.read(&bufferUShort[0], tissue_dname))
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
									(tissues_size_t)bufferUShort[n];
							n++;
						}
					}
				}
				bufferUShort.clear();
			}
			else if (type.compare("unsigned int") == 0)
			{
				std::vector<unsigned int> bufferUInt;
				try
				{
					// vector throws a length_error if resized above max_size
					// vector<float> bufferFloat(N);
					ISEG_DEBUG("N = " << N << ", bufferUInt.max_size() = " << bufferUInt.max_size());
					bufferUInt.resize(N);
				}
				catch (std::length_error& le)
				{
					ISEG_ERROR("bufferUInt length error: " << le.what());
					return 0;
				}
				if (!reader.read(&bufferUInt[0], tissue_dname))
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
									(tissues_size_t)bufferUInt[n];
							n++;
						}
					}
				}
				bufferUInt.clear();
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

		if (reader.exists(source_dname))
		{
			ScopedTimer timer("Read Source");
			bool ok = true;
			size_t offset = 0;
			for (int k = 0; k < NumberOfSlices; k++, offset += slice_size)
			{
				ok = ok && reader.read(ImageSlices[k], offset, slice_size,
											 source_dname);
			}
		}
		if (reader.exists(target_dname))
		{
			ScopedTimer timer("Read Target");
			bool ok = true;
			size_t offset = 0;
			for (int k = 0; k < NumberOfSlices; k++, offset += slice_size)
			{
				ok = ok && reader.read(WorkSlices[k], offset, slice_size,
											 target_dname);
			}
		}
		if (reader.exists(tissue_dname))
		{
			ScopedTimer timer("Read Tissue");
			bool ok = true;
			size_t offset = 0;
			for (int k = 0; k < NumberOfSlices; k++, offset += slice_size)
			{
				ok = ok && reader.read(TissueSlices[k], offset, slice_size,
											 tissue_dname);
			}
		}
	}

	return 1;
}
} // namespace

XdmfImageReader::XdmfImageReader()
{
	this->NumberOfSlices = 0;
	this->Width = 0;
	this->Height = 0;
	this->Compression = 1;
	this->ImageTransform.setIdentity();
	this->ImageSlices = nullptr;
	this->WorkSlices = nullptr;
	this->TissueSlices = nullptr;
	this->FileName = nullptr;
	this->ReadContiguousMemory = false;
}

XdmfImageReader::~XdmfImageReader() { delete[] this->FileName; }

int XdmfImageReader::ParseXML()
{
	QFile file(FileName);
	if (!file.open(QIODevice::ReadOnly))
	{
		ISEG_ERROR("cannot open " << FileName);
		return 0;
	}

	QDomDocument inputDocument("Slices");
	QByteArray inputContent = file.readAll();
	file.close();

	QString msg;
	if (!inputDocument.setContent(inputContent, false,
					&msg)) // This function is not reentrant.
	{
		ISEG_ERROR("assigning content of " << FileName << ": " << msg.toStdString());
	}

	QDomElement root = inputDocument.documentElement();
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
					QStringList textList = text.split(QString(" "));
					if (textList.size() != 3)
					{
						ISEG_ERROR_MSG("invalid origin...");
						return 0;
					}
					float offset[3];
					for (int i3 = 0; i3 < textList.size(); ++i3)
						offset[2 - i3] = textList[i3].toFloat();
					ImageTransform.setOffset(offset);
				}
				else if (name == QString("Spacing"))
				{
					QString text = e2.text().trimmed();
					QStringList textList = text.split(QString(" "));
					if (textList.size() != 3)
					{
						ISEG_ERROR_MSG("invalid spacing...");
						return 0;
					}
					for (int i3 = 0; i3 < textList.size(); ++i3)
						PixelSize[2 - i3] = textList[i3].toFloat();
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
			QStringList textList =
					topology.attribute(QString("Dimensions")).split(QString(" "));
			if (textList.size() != 3)
			{
				ISEG_ERROR_MSG("invalid dimensions...");
				return 0;
			}
			this->Width = textList[2].toInt();
			this->Height = textList[1].toInt();
			this->NumberOfSlices = textList[0].toInt();

			// Attributes
			QDomNodeList attributeList = e.elementsByTagName("Attribute");
			for (int i2 = 0; i2 < attributeList.size(); ++i2)
			{
				const QDomElement scalar = attributeList.item(i2).toElement();
				QString name = scalar.attribute(QString("Name"));
				this->ArrayNames.append(name);
				QDomElement dataitem =
						scalar.firstChildElement(QString("DataItem"));
				QString text = dataitem.text().trimmed();
				int idx = text.indexOf(":");
				QString datasetname = text.remove(0, idx + 1);
				this->mapArrayNames[name] = datasetname;
			}

		} // if(e.tagName()=="Grid")

		n = n.nextSibling();
	}

	return 1;
}

// BL TODO read rotation & origin from HDF5 part, if available
int XdmfImageReader::Read()
{
	ISEG_INFO("Reading " << this->FileName << ": " << Width << " x " << Height << " x " << NumberOfSlices);

	QString qFileName(this->FileName);
	QFileInfo fileInfo(qFileName);
	QString basename = fileInfo.completeBaseName();
	QString suffix = fileInfo.suffix();

	// save working directory
	QDir oldcwd = QDir::current();

	// enter the xmf file folder so relative names for hdf5 files work
	QDir::setCurrent(fileInfo.absolutePath());

	HDF5Reader reader;
	const QString fname = basename + ".h5";
	if (!reader.open(fname.toStdString()))
	{
		ISEG_ERROR("opening " << fname.toStdString());
		return 0;
	}

	int r = _Read(reader, NumberOfSlices, Width, Height, ReadContiguousMemory,
			this->mapArrayNames["Source"].toAscii().data(), ImageSlices,
			this->mapArrayNames["Target"].toAscii().data(), WorkSlices,
			this->mapArrayNames["Tissue"].toAscii().data(), TissueSlices);

	reader.close();

	// restore working directory
	QDir::setCurrent(oldcwd.absolutePath());

	return r;
}

std::shared_ptr<ColorLookupTable> XdmfImageReader::ReadColorLookup() const
{
	std::string fname(this->FileName);
	boost::replace_last(fname, ".xmf", ".h5");

	HDFImageReader reader;
	reader.SetFileName(fname.c_str());

	return reader.ReadColorLookup();
}

HDFImageReader::HDFImageReader()
{
	this->NumberOfSlices = 0;
	this->Width = 0;
	this->Height = 0;
	this->Compression = 1;
	this->ImageSlices = nullptr;
	this->WorkSlices = nullptr;
	this->TissueSlices = nullptr;
	this->FileName = nullptr;
	this->ReadContiguousMemory = false;
}

HDFImageReader::~HDFImageReader() { delete[] this->FileName; }

int HDFImageReader::ParseHDF()
{
	std::cerr << "parsing file " << this->FileName << endl;
	HDF5Reader reader;
	if (!reader.open(FileName))
	{
		ISEG_ERROR("opening " << FileName);
		return 0;
	}

	bool ok = true;
	std::vector<int> dimensions;
	dimensions.resize(3);
	ok = ok && (reader.read(dimensions, "/dimensions") != 0) &&
			 (dimensions.size() == 3);
	this->Width = dimensions[0];
	this->Height = dimensions[1];
	this->NumberOfSlices = dimensions[2];

	std::vector<float> pixelsize(3);
	ok = ok && (reader.read(pixelsize, "/pixelsize") != 0) &&
			 (pixelsize.size() == 3);
	for (unsigned short i = 0; i < 3; i++)
		PixelSize[i] = pixelsize[i];

	std::vector<float> offset(3);
	ok = ok && (reader.read(offset, "/offset") != 0) && (offset.size() == 3);

	if (reader.exists("/rotation"))
	{
		std::vector<float> rotation(9);
		ok = ok && (reader.read(rotation, "/rotation") != 0) &&
				 (rotation.size() == 9);

		ImageTransform.setIdentity();
		ImageTransform.setOffset(offset.data());

		for (int k = 0; k < 3; ++k)
		{
			ImageTransform[k][0] = rotation[k * 3 + 0];
			ImageTransform[k][1] = rotation[k * 3 + 1];
			ImageTransform[k][2] = rotation[k * 3 + 2];
		}
	}
	else
	{
		std::vector<float> dc(6);
		ok = ok && (reader.read(dc, "/dc") != 0) && (dc.size() == 6);

		ImageTransform.setTransform(offset.data(), dc.data());
	}

	//Transform

	if (reader.exists("/Source"))
	{
		mapArrayNames["Source"] = QString("/Source");
		ArrayNames.append(QString("Source"));
	}
	if (reader.exists("/Target"))
	{
		mapArrayNames["Target"] = QString("/Target");
		ArrayNames.append(QString("Target"));
	}
	if (reader.exists("/Tissue"))
	{
		mapArrayNames["Tissue"] = QString("/Tissue");
		ArrayNames.append(QString("Tissue"));
	}

	reader.close();

	assert(ok);
	return ok ? 1 : 0;
}

int HDFImageReader::Read()
{
	ISEG_INFO("Reading " << this->FileName << ": " << Width << " x " << Height << " x " << NumberOfSlices);

	//	vector<int> dims;

	QString qFileName(this->FileName);
	QFileInfo fileInfo(qFileName);
	QString basename = fileInfo.completeBaseName();
	QString suffix = fileInfo.suffix();

	// save working directory
	QDir oldcwd = QDir::current();

	// enter the xmf file folder so relative names for hdf5 files work
	QDir::setCurrent(fileInfo.absolutePath());

	HDF5Reader reader;
	const QString fname = basename + "." + suffix;
	if (!reader.open(fname.toAscii().data()))
	{
		ISEG_ERROR("opening " << fname.toStdString());
		return 0;
	}

	_Read(reader, NumberOfSlices, Width, Height, ReadContiguousMemory,
			this->mapArrayNames["Source"].toAscii().data(), ImageSlices,
			this->mapArrayNames["Target"].toAscii().data(), WorkSlices,
			this->mapArrayNames["Tissue"].toAscii().data(), TissueSlices);

	reader.close();

	// restore working directory
	QDir::setCurrent(oldcwd.absolutePath());

	return 1;
}

std::shared_ptr<ColorLookupTable> HDFImageReader::ReadColorLookup() const
{
	ScopedTimer timer("ReadColorLookup");

	std::shared_ptr<ColorLookupTable> color_lookup_table;

	QString qFileName(this->FileName);
	QFileInfo fileInfo(qFileName);
	QString basename = fileInfo.completeBaseName();
	QString suffix = fileInfo.suffix();

	// save working directory
	QDir oldcwd = QDir::current();
	QDir::setCurrent(fileInfo.absolutePath());

	HDF5Reader reader;
	const QString fname = basename + "." + suffix;
	if (!reader.open(fname.toStdString()))
	{
		ISEG_ERROR("opening " << fname.toStdString());
		return nullptr;
	}

	bool ok = true;
	if (reader.exists("/Lut"))
	{
		int version = -1, num_colors = 0;
		ok = ok && (reader.read(&version, "/Lut/version") != 0);
		ok = ok && (reader.read(&num_colors, "/Lut/size") != 0);

		if (version == 1)
		{
			std::vector<std::string> colors = reader.getGroupInfo("/Lut");
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

				for (auto name : colors)
				{
					if (ok && name.find("color") != std::string::npos)
					{
						std::string const folder_name = "/Lut/" + name;

						ok = ok &&
								 (reader.read(&index, folder_name + "/index") != 0);
						ok = ok &&
								 (reader.read(float_rgb, folder_name + "/rgb") != 0);

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
			ok = ok && (reader.read(colors.data(), "/Lut/colors") != 0);
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
			ok = ok && (reader.read(colors.data(), "/Lut/colors") != 0);
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

}// namespace iseg