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

#include "XdmfImageMerger.h"

#include "Core/HDF5Reader.h"
#include "Core/HDF5Writer.h"

#include <QDir>
#include <QDomDocument>
#include <QDomElement>
#include <QDomText>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QTextStream>

#include <vtkSmartPointer.h>

#include <stdexcept>
#include <vector>

namespace iseg {

XdmfImageMerger::XdmfImageMerger()
{
	this->m_NumberOfSlices = 0;
	this->m_Width = 0;
	this->m_Height = 0;
	this->m_Compression = 1;
	this->m_PixelSize = nullptr;
	this->m_ImageSlices = nullptr;
	this->m_WorkSlices = nullptr;
	this->m_TissueSlices = nullptr;
	this->m_FileName = nullptr;
}

XdmfImageMerger::~XdmfImageMerger() { delete[] this->m_FileName; }

int XdmfImageMerger::Write()
{
	return this->InternalWrite(m_FileName, m_MergeFileNames, m_ImageSlices, m_WorkSlices, m_TissueSlices, m_NumberOfSlices, m_TotalNumberOfSlices, m_Width, m_Height, m_PixelSize, m_ImageTransform, m_Compression);
}

int XdmfImageMerger::InternalWrite(const char* filename, std::vector<QString>& mergefilenames, float** slicesbmp, float** sliceswork, tissues_size_t** slicestissue, unsigned nrslices, unsigned nrslicesTotal, unsigned width, unsigned height, float* pixelsize, const Transform& transform, int compression)
{
	// Parse xml files of merged projects
	std::vector<XdmfImageReader*> image_readers;
	std::vector<QString>::iterator iter_filename;
	for (iter_filename = mergefilenames.begin(); iter_filename != mergefilenames.end(); ++iter_filename)
	{
		XdmfImageReader* reader = new XdmfImageReader();
		QString image_filename = QFileInfo(*iter_filename).completeBaseName() + ".xmf";
		reader->SetFileName(QFileInfo(*iter_filename).dir().absFilePath(image_filename).toAscii().data());
		if (reader->ParseXML() == 0)
		{
			ISEG_ERROR_MSG("XdmfImageMerger::InternalWrite while parsing xmls");
			return 0;
		}
		image_readers.push_back(reader);
	}

	QString q_file_name(filename);
	QFileInfo file_info(q_file_name);
	QString basename = file_info.completeBaseName();

	// save working directory
	QDir oldcwd = QDir::current();

	// enter the xmf file folder so relative names for hdf5 files work
	QDir::setCurrent(file_info.absolutePath());

	const size_t slice_size = static_cast<size_t>(width) * height;

	float offset[3];
	transform.GetOffset(offset);

	ISEG_INFO("Writing " << width << " x " << height << " x " << nrslicesTotal);

	HDF5Writer writer;
	const QString fname = basename + ".h5";
	if (!writer.Open(fname.toStdString()))
	{
		ISEG_ERROR("opening " << fname.toStdString());
	}
	writer.m_Compression = compression;

	{
		//write dimensions, pixelsize, offset, dc,
		std::vector<HDF5Writer::size_type> shape;
		shape.resize(1);
		int dimension[3];
		dimension[0] = width;
		dimension[1] = height;
		dimension[2] = nrslicesTotal;

		float dc[6];
		transform.GetDirectionCosines(dc);

		float rotation[9];
		for (int k = 0; k < 3; ++k)
		{
			rotation[k * 3 + 0] = transform[k][0];
			rotation[k * 3 + 1] = transform[k][1];
			rotation[k * 3 + 2] = transform[k][2];
		}

		shape[0] = 3;
		if (!writer.Write(dimension, shape, std::string("dimensions")))
		{
			ISEG_ERROR_MSG("writing dimensions");
		}
		if (!writer.Write(offset, shape, std::string("offset")))
		{
			ISEG_ERROR_MSG("writing offset");
		}
		if (!writer.Write(pixelsize, shape, std::string("pixelsize")))
		{
			ISEG_ERROR_MSG("writing pixelsize");
		}

		shape[0] = 6;
		if (!writer.Write(dc, shape, std::string("dc")))
		{
			ISEG_ERROR_MSG("writing dc");
		}

		shape[0] = 9;
		if (!writer.Write(rotation, shape, std::string("rotation")))
		{
			ISEG_ERROR_MSG("writing rotation");
		}
	}

	// The slices are not contiguous in memory so we need to copy.

	// Source
	{
		ISEG_INFO_MSG("writing Source");
		// allocate in file
		float** const null = nullptr;
		if (writer.Write(null, nrslicesTotal, slice_size, "Source"))
		{
			size_t offset = 0;

			// Current project
			writer.Write(slicesbmp, nrslices, slice_size, "Source", offset);
			offset += nrslices * slice_size;

			// Merged projects
			std::vector<float> project_buffer;
			std::vector<XdmfImageReader*>::iterator iter_reader;
			for (iter_filename = mergefilenames.begin(), iter_reader = image_readers.begin();
					 iter_filename != mergefilenames.end();
					 ++iter_filename, ++iter_reader)
			{
				size_t junk = 0;
				size_t const project_slices =
						(*iter_reader)->GetNumberOfSlices();
				project_buffer.resize(slice_size * project_slices);
				ReadSource(*iter_reader, (*iter_filename).toAscii().data(), project_buffer, junk);

				std::vector<float*> slices(project_slices, nullptr);
				for (size_t i = 0; i < project_slices; ++i)
				{
					slices[i] = &project_buffer[i * slice_size];
				}
				writer.Write(slices.data(), project_slices, slice_size, "Source", offset);
				offset += project_slices * slice_size;
			}
		}
	}

	// Target
	{
		ISEG_INFO_MSG("writing Target");
		// allocate in file
		float** const null = nullptr;
		if (writer.Write(null, nrslicesTotal, slice_size, "Target"))
		{
			size_t offset = 0;

			// Current project
			writer.Write(sliceswork, nrslices, slice_size, "Target", offset);
			offset += nrslices * slice_size;

			// Merged projects
			std::vector<float> project_buffer;
			std::vector<XdmfImageReader*>::iterator iter_reader;
			for (iter_filename = mergefilenames.begin(), iter_reader = image_readers.begin();
					 iter_filename != mergefilenames.end();
					 ++iter_filename, ++iter_reader)
			{
				size_t junk = 0;
				size_t const project_slices =
						(*iter_reader)->GetNumberOfSlices();
				project_buffer.resize(slice_size * project_slices);
				ReadTarget(*iter_reader, (*iter_filename).toAscii().data(), project_buffer, junk);

				std::vector<float*> slices(project_slices, nullptr);
				for (size_t i = 0; i < project_slices; ++i)
				{
					slices[i] = &project_buffer[i * slice_size];
				}
				writer.Write(slices.data(), project_slices, slice_size, "Target", offset);
				offset += project_slices * slice_size;
			}
		}
	}

	// Tissue
	{
		ISEG_INFO_MSG("writing Tissue");
		// allocate in file
		tissues_size_t** const null = nullptr;
		if (writer.Write(null, nrslicesTotal, slice_size, "Tissue"))
		{
			size_t offset = 0;

			// Current project
			writer.Write(slicestissue, nrslices, slice_size, "Tissue", offset);
			offset += nrslices * slice_size;

			// Merged projects
			std::vector<tissues_size_t> project_buffer;
			std::vector<XdmfImageReader*>::iterator iter_reader;
			for (iter_filename = mergefilenames.begin(), iter_reader = image_readers.begin();
					 iter_filename != mergefilenames.end();
					 ++iter_filename, ++iter_reader)
			{
				size_t junk = 0;
				size_t const project_slices =
						(*iter_reader)->GetNumberOfSlices();
				project_buffer.resize(slice_size * project_slices);
				ReadTissues(*iter_reader, (*iter_filename).toAscii().data(), project_buffer, junk);

				std::vector<tissues_size_t*> slices(project_slices, nullptr);
				for (size_t i = 0; i < project_slices; ++i)
				{
					slices[i] = &project_buffer[i * slice_size];
				}
				writer.Write(slices.data(), project_slices, slice_size, "Tissue", offset);
				offset += project_slices * slice_size;
			}
		}
	}

	writer.Close();

	// Write XML file
	QDomElement dataitem, attribute;
	QDomText text;

	QDomDocument doc("Xdmf");
	QDomElement root = doc.createElement("Xdmf");
	doc.appendChild(root);
	QDomElement domain = doc.createElement("Domain");
	domain.setAttribute("Name", "domain");
	root.appendChild(domain);

	QDomElement grid = doc.createElement("Grid");
	grid.setAttribute("Type", "Uniform");
	domain.appendChild(grid);

	QDomElement geometry = doc.createElement("Geometry");
	geometry.setAttribute("Type", "ORIGIN_DXDYDZ");

	dataitem = doc.createElement("DataItem");
	dataitem.setAttribute("Name", "Origin");
	dataitem.setAttribute("Format", "XML");
	dataitem.setAttribute("NumberType", "Float");
	dataitem.setAttribute("Precision", 4);
	dataitem.setAttribute("Dimensions", 3);
	text = doc.createTextNode(QString("%1 %2 %3")
																.arg(offset[2])
																.arg(offset[1])
																.arg(offset[0])
																.toAscii()
																.data());
	dataitem.appendChild(text);
	geometry.appendChild(dataitem);

	dataitem = doc.createElement("DataItem");
	dataitem.setAttribute("Name", "Spacing");
	dataitem.setAttribute("Format", "XML");
	dataitem.setAttribute("NumberType", "Float");
	dataitem.setAttribute("Precision", 4);
	dataitem.setAttribute("Dimensions", 3);
	text = doc.createTextNode(QString("%1 %2 %3")
																.arg(pixelsize[2])
																.arg(pixelsize[1])
																.arg(pixelsize[0])
																.toAscii()
																.data());
	dataitem.appendChild(text);
	geometry.appendChild(dataitem);

	grid.appendChild(geometry);

	QString qdims = QString("%1 %2 %3")
											.arg(nrslicesTotal)
											.arg(height)
											.arg(width)
											.toAscii()
											.data();

	QDomElement topology = doc.createElement("Topology");
	topology.setAttribute("Type", "3DCORECTMesh");
	topology.setAttribute("Dimensions", qdims.toAscii().data());
	grid.appendChild(topology);

	attribute = doc.createElement("Attribute");
	attribute.setAttribute("Name", "Source");
	attribute.setAttribute("AttributeType", "Scalar");
	attribute.setAttribute("Center", "Node");
	dataitem = doc.createElement("DataItem");
	dataitem.setAttribute("NumberType", "Float");
	dataitem.setAttribute("Precision", 4);
	dataitem.setAttribute("Format", "HDF");
	dataitem.setAttribute("Dimensions", qdims.toAscii().data());
	text = doc.createTextNode((basename + ".h5:/Source").toAscii().data());
	dataitem.appendChild(text);
	attribute.appendChild(dataitem);
	grid.appendChild(attribute);

	attribute = doc.createElement("Attribute");
	attribute.setAttribute("Name", "Target");
	attribute.setAttribute("AttributeType", "Scalar");
	attribute.setAttribute("Center", "Node");
	dataitem = doc.createElement("DataItem");
	dataitem.setAttribute("NumberType", "Float");
	dataitem.setAttribute("Precision", 4);
	dataitem.setAttribute("Format", "HDF");
	dataitem.setAttribute("Dimensions", qdims.toAscii().data());
	text = doc.createTextNode((basename + ".h5:/Target").toAscii().data());
	dataitem.appendChild(text);
	attribute.appendChild(dataitem);
	grid.appendChild(attribute);

	attribute = doc.createElement("Attribute");
	attribute.setAttribute("Name", "Tissue");
	attribute.setAttribute("AttributeType", "Scalar");
	attribute.setAttribute("Center", "Node");
	dataitem = doc.createElement("DataItem");
	switch (sizeof(tissues_size_t))
	{
	case 1:
		dataitem.setAttribute("NumberType", "UChar");
		dataitem.setAttribute("Precision", 1);
		break;
	case 2:
		dataitem.setAttribute("NumberType", "UInt");
		dataitem.setAttribute("Precision", 4);
		break;
	default:
		ISEG_ERROR_MSG("tissues_size_t not supported!");
		return 0;
	}
	dataitem.setAttribute("Format", "HDF");
	dataitem.setAttribute("Dimensions", qdims.toAscii().data());
	text = doc.createTextNode((basename + ".h5:/Tissue").toAscii().data());
	dataitem.appendChild(text);
	attribute.appendChild(dataitem);
	grid.appendChild(attribute);

	QFile file(q_file_name);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
		return 0;

	QTextStream out(&file);
	out << doc;
	file.close();

	// restore working directory
	QDir::setCurrent(oldcwd.absolutePath());

	for (XdmfImageReader* r : image_readers)
	{
		delete r;
	}

	return 1;
}

int XdmfImageMerger::ReadSource(XdmfImageReader* imageReader, const char* filename, std::vector<float>& bufferFloat, size_t& sliceoffset) const
{
	QFileInfo file_info(filename);
	QString basename = file_info.completeBaseName();

	// save working directory
	QDir oldcwd = QDir::current();

	// enter the xmf file folder so relative names for hdf5 files work
	QDir::setCurrent(file_info.absolutePath());

	HDF5Reader reader;
	reader.m_Loud = true;
	const QString fname = basename + ".h5";
	if (!reader.Open(fname.toStdString()))
	{
		ISEG_ERROR("opening " << fname.toStdString());
		return 0;
	}

	// Source
	QString map_source_name = imageReader->GetMapArrayNames()["Source"];
	if (map_source_name.isEmpty())
	{
		ISEG_ERROR_MSG("no Source array...");
		return 0;
	}
	if (!reader.Read(&bufferFloat[sliceoffset * this->m_Width * this->m_Height], map_source_name.toStdString()))
	{
		ISEG_ERROR_MSG("reading Source dataset...");
		return 0;
	}
	sliceoffset += imageReader->GetNumberOfSlices();

	// restore working directory
	QDir::setCurrent(oldcwd.absolutePath());

	return 1;
}

int XdmfImageMerger::ReadTarget(XdmfImageReader* imageReader, const char* filename, std::vector<float>& bufferFloat, size_t& sliceoffset) const
{
	QFileInfo file_info(filename);
	QString basename = file_info.completeBaseName();

	// save working directory
	QDir oldcwd = QDir::current();

	// enter the xmf file folder so relative names for hdf5 files work
	QDir::setCurrent(file_info.absolutePath());

	HDF5Reader reader;
	reader.m_Loud = true;
	const QString fname = basename + ".h5";
	if (!reader.Open(fname.toStdString()))
	{
		ISEG_ERROR("opening " << fname.toStdString());
		return 0;
	}

	// Target
	QString map_target_name = imageReader->GetMapArrayNames()["Target"];
	if (map_target_name.isEmpty())
	{
		ISEG_WARNING_MSG("no Target array, will initialize to 0...");
		bufferFloat.assign(bufferFloat.size(), 0.0f);
	}
	else
	{
		if (!reader.Read(&bufferFloat[sliceoffset * this->m_Width * this->m_Height], map_target_name.toAscii().data()))
		{
			ISEG_ERROR_MSG("reading Target dataset...");
			return 0;
		}
	}
	sliceoffset += imageReader->GetNumberOfSlices();

	// restore working directory
	QDir::setCurrent(oldcwd.absolutePath());

	return 1;
}

int XdmfImageMerger::ReadTissues(XdmfImageReader* imageReader, const char* filename, std::vector<tissues_size_t>& bufferTissuesSizeT, size_t& sliceoffset) const
{
	QFileInfo file_info(filename);
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

	// Tissue
	QString map_tissue_name = imageReader->GetMapArrayNames()["Tissue"];
	std::string type;
	std::vector<HDF5Reader::size_type> dims;
	if (!reader.GetDatasetInfo(type, dims, map_tissue_name.toAscii().data()))
	{
		ISEG_ERROR_MSG("reading Tissue data type...");
		return 0;
	}

	size_t const curr_nrslices = imageReader->GetNumberOfSlices();
	size_t const n = (curr_nrslices * this->m_Width) * this->m_Height;

	if (type == "unsigned char")
	{
		std::vector<unsigned char> buffer_u_char;
		try
		{
			buffer_u_char.resize(n);
		}
		catch (std::length_error& le)
		{
			std::cerr << "bufferUChar length error: " << le.what();
			return 0;
		}
		if (map_tissue_name.isEmpty())
		{
			ISEG_ERROR_MSG("no Tissue array...");
			return 0;
		}
		if (!reader.Read(&buffer_u_char[0], map_tissue_name.toAscii().data()))
		{
			ISEG_ERROR_MSG("reading Tissue dataset...");
			return 0;
		}
		std::vector<unsigned char>::iterator iter_from = buffer_u_char.begin();
		std::vector<tissues_size_t>::iterator iter_to =
				bufferTissuesSizeT.begin() +
				(sliceoffset * this->m_Width) * this->m_Height;
		for (unsigned int k = 0; k < curr_nrslices; k++)
		{
			for (unsigned int j = 0; j < this->m_Height; j++)
			{
				for (unsigned int i = 0; i < this->m_Width; i++)
				{
					(*iter_to++) = (tissues_size_t)(*iter_from++);
				}
			}
		}
		buffer_u_char.clear();
	}
	else if (type == "unsigned short")
	{
		static_assert(std::is_same<unsigned short, tissues_size_t>::value, "Special case we read directly into the buffer.");
		if (map_tissue_name.isEmpty())
		{
			ISEG_ERROR_MSG("no Tissue array...");
			return 0;
		}
		if (!reader.Read(&bufferTissuesSizeT[(sliceoffset * this->m_Width) * this->m_Height], map_tissue_name.toAscii().data()))
		{
			ISEG_ERROR_MSG("reading Tissue dataset...");
			return 0;
		}
	}
	else if (type == "unsigned int")
	{
		std::vector<unsigned int> buffer_u_int;
		try
		{
			buffer_u_int.resize(n);
		}
		catch (std::length_error& le)
		{
			ISEG_ERROR("bufferUInt length error: " << le.what());
			return 0;
		}
		if (map_tissue_name.isEmpty())
		{
			ISEG_ERROR_MSG("no Tissue array...");
			return 0;
		}
		if (!reader.Read(&buffer_u_int[0], map_tissue_name.toAscii().data()))
		{
			ISEG_ERROR_MSG("reading Tissue dataset...");
			return 0;
		}
		std::vector<unsigned int>::iterator iter_from = buffer_u_int.begin();
		std::vector<tissues_size_t>::iterator iter_to =
				bufferTissuesSizeT.begin() +
				sliceoffset * this->m_Width * this->m_Height;
		for (unsigned int k = 0; k < curr_nrslices; k++)
		{
			for (unsigned int j = 0; j < this->m_Height; j++)
			{
				for (unsigned int i = 0; i < this->m_Width; i++)
				{
					(*iter_to++) = (tissues_size_t)(*iter_from++);
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
	sliceoffset += imageReader->GetNumberOfSlices();

	// restore working directory
	QDir::setCurrent(oldcwd.absolutePath());

	return 1;
}

} // namespace iseg
