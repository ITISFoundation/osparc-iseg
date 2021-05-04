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
	this->NumberOfSlices = 0;
	this->Width = 0;
	this->Height = 0;
	this->Compression = 1;
	this->PixelSize = 0;
	this->ImageSlices = 0;
	this->WorkSlices = 0;
	this->TissueSlices = 0;
	this->FileName = 0;
}

XdmfImageMerger::~XdmfImageMerger() { delete[] this->FileName; }

int XdmfImageMerger::Write()
{
	return this->InternalWrite(FileName, MergeFileNames, ImageSlices,
			WorkSlices, TissueSlices, NumberOfSlices,
			TotalNumberOfSlices, Width, Height, PixelSize,
			ImageTransform, Compression);
}

int XdmfImageMerger::InternalWrite(
		const char* filename, std::vector<QString>& mergefilenames,
		float** slicesbmp, float** sliceswork, tissues_size_t** slicestissue,
		unsigned nrslices, unsigned nrslicesTotal, unsigned width, unsigned height,
		float* pixelsize, const Transform& transform, int compression)
{
	// Parse xml files of merged projects
	std::vector<XdmfImageReader*> imageReaders;
	std::vector<QString>::iterator iterFilename;
	for (iterFilename = mergefilenames.begin(); iterFilename != mergefilenames.end(); ++iterFilename)
	{
		XdmfImageReader* reader = new XdmfImageReader();
		QString imageFilename = QFileInfo(*iterFilename).completeBaseName() + ".xmf";
		reader->SetFileName(QFileInfo(*iterFilename).dir().absFilePath(imageFilename).toAscii().data());
		if (reader->ParseXML() == 0)
		{
			ISEG_ERROR_MSG("XdmfImageMerger::InternalWrite while parsing xmls");
			return 0;
		}
		imageReaders.push_back(reader);
	}

	QString qFileName(filename);
	QFileInfo fileInfo(qFileName);
	QString basename = fileInfo.completeBaseName();
	QString suffix = fileInfo.suffix();

	// save working directory
	QDir oldcwd = QDir::current();

	// enter the xmf file folder so relative names for hdf5 files work
	QDir::setCurrent(fileInfo.absolutePath());

	const size_t slice_size = static_cast<size_t>(width) * height;

	float offset[3];
	transform.getOffset(offset);

	ISEG_INFO("Writing " << width << " x " << height << " x " << nrslicesTotal);

	HDF5Writer writer;
	const QString fname = basename + ".h5";
	if (!writer.open(fname.toStdString()))
	{
		ISEG_ERROR("opening " << fname.toStdString());
	}
	writer.compression = compression;

	{
		//write dimensions, pixelsize, offset, dc,
		std::vector<HDF5Writer::size_type> shape;
		shape.resize(1);
		int dimension[3];
		dimension[0] = width;
		dimension[1] = height;
		dimension[2] = nrslicesTotal;

		float dc[6];
		transform.getDirectionCosines(dc);

		float rotation[9];
		for (int k = 0; k < 3; ++k)
		{
			rotation[k * 3 + 0] = transform[k][0];
			rotation[k * 3 + 1] = transform[k][1];
			rotation[k * 3 + 2] = transform[k][2];
		}

		shape[0] = 3;
		if (!writer.write(dimension, shape, std::string("dimensions")))
		{
			ISEG_ERROR_MSG("writing dimensions");
		}
		if (!writer.write(offset, shape, std::string("offset")))
		{
			ISEG_ERROR_MSG("writing offset");
		}
		if (!writer.write(pixelsize, shape, std::string("pixelsize")))
		{
			ISEG_ERROR_MSG("writing pixelsize");
		}

		shape[0] = 6;
		if (!writer.write(dc, shape, std::string("dc")))
		{
			ISEG_ERROR_MSG("writing dc");
		}

		shape[0] = 9;
		if (!writer.write(rotation, shape, std::string("rotation")))
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
		if (writer.write(null, nrslicesTotal, slice_size, "Source"))
		{
			size_t offset = 0;

			// Current project
			writer.write(slicesbmp, nrslices, slice_size, "Source", offset);
			offset += nrslices * slice_size;

			// Merged projects
			std::vector<float> project_buffer;
			std::vector<XdmfImageReader*>::iterator iterReader;
			for (iterFilename = mergefilenames.begin(),
					iterReader = imageReaders.begin();
					 iterFilename != mergefilenames.end();
					 ++iterFilename, ++iterReader)
			{
				size_t junk = 0;
				size_t const project_slices =
						(*iterReader)->GetNumberOfSlices();
				project_buffer.resize(slice_size * project_slices);
				ReadSource(*iterReader, (*iterFilename).toAscii().data(),
						project_buffer, junk);

				std::vector<float*> slices(project_slices, nullptr);
				for (size_t i = 0; i < project_slices; ++i)
				{
					slices[i] = &project_buffer[i * slice_size];
				}
				writer.write(slices.data(), project_slices, slice_size,
						"Source", offset);
				offset += project_slices * slice_size;
			}
		}
	}

	// Target
	{
		ISEG_INFO_MSG("writing Target");
		// allocate in file
		float** const null = nullptr;
		if (writer.write(null, nrslicesTotal, slice_size, "Target"))
		{
			size_t offset = 0;

			// Current project
			writer.write(sliceswork, nrslices, slice_size, "Target", offset);
			offset += nrslices * slice_size;

			// Merged projects
			std::vector<float> project_buffer;
			std::vector<XdmfImageReader*>::iterator iterReader;
			for (iterFilename = mergefilenames.begin(),
					iterReader = imageReaders.begin();
					 iterFilename != mergefilenames.end();
					 ++iterFilename, ++iterReader)
			{
				size_t junk = 0;
				size_t const project_slices =
						(*iterReader)->GetNumberOfSlices();
				project_buffer.resize(slice_size * project_slices);
				ReadTarget(*iterReader, (*iterFilename).toAscii().data(),
						project_buffer, junk);

				std::vector<float*> slices(project_slices, nullptr);
				for (size_t i = 0; i < project_slices; ++i)
				{
					slices[i] = &project_buffer[i * slice_size];
				}
				writer.write(slices.data(), project_slices, slice_size,
						"Target", offset);
				offset += project_slices * slice_size;
			}
		}
	}

	// Tissue
	{
		ISEG_INFO_MSG("writing Tissue");
		// allocate in file
		tissues_size_t** const null = nullptr;
		if (writer.write(null, nrslicesTotal, slice_size, "Tissue"))
		{
			size_t offset = 0;

			// Current project
			writer.write(slicestissue, nrslices, slice_size, "Tissue", offset);
			offset += nrslices * slice_size;

			// Merged projects
			std::vector<tissues_size_t> project_buffer;
			std::vector<XdmfImageReader*>::iterator iterReader;
			for (iterFilename = mergefilenames.begin(),
					iterReader = imageReaders.begin();
					 iterFilename != mergefilenames.end();
					 ++iterFilename, ++iterReader)
			{
				size_t junk = 0;
				size_t const project_slices =
						(*iterReader)->GetNumberOfSlices();
				project_buffer.resize(slice_size * project_slices);
				ReadTissues(*iterReader, (*iterFilename).toAscii().data(),
						project_buffer, junk);

				std::vector<tissues_size_t*> slices(project_slices, nullptr);
				for (size_t i = 0; i < project_slices; ++i)
				{
					slices[i] = &project_buffer[i * slice_size];
				}
				writer.write(slices.data(), project_slices, slice_size,
						"Tissue", offset);
				offset += project_slices * slice_size;
			}
		}
	}

	writer.close();

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

	QFile file(qFileName);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
		return 0;

	QTextStream out(&file);
	out << doc;
	file.close();

	// restore working directory
	QDir::setCurrent(oldcwd.absolutePath());

	for (XdmfImageReader* r : imageReaders)
	{
		delete r;
	}

	return 1;
}

int XdmfImageMerger::ReadSource(XdmfImageReader* imageReader,
		const char* filename,
		std::vector<float>& bufferFloat,
		size_t& sliceoffset)
{
	QFileInfo fileInfo(filename);
	QString basename = fileInfo.completeBaseName();
	QString suffix = fileInfo.suffix();

	// save working directory
	QDir oldcwd = QDir::current();

	// enter the xmf file folder so relative names for hdf5 files work
	QDir::setCurrent(fileInfo.absolutePath());

	HDF5Reader reader;
	reader.loud = 1;
	const QString fname = basename + ".h5";
	if (!reader.open(fname.toStdString()))
	{
		ISEG_ERROR("opening " << fname.toStdString());
		return 0;
	}

	// Source
	QString mapSourceName = imageReader->GetMapArrayNames()["Source"];
	if (mapSourceName.isEmpty())
	{
		ISEG_ERROR_MSG("no Source array...");
		return 0;
	}
	if (!reader.read(&bufferFloat[sliceoffset * this->Width * this->Height],
					mapSourceName.toAscii().data()))
	{
		ISEG_ERROR_MSG("reading Source dataset...");
		return 0;
	}
	sliceoffset += imageReader->GetNumberOfSlices();

	// restore working directory
	QDir::setCurrent(oldcwd.absolutePath());

	return 1;
}

int XdmfImageMerger::ReadTarget(XdmfImageReader* imageReader,
		const char* filename,
		std::vector<float>& bufferFloat,
		size_t& sliceoffset)
{
	QFileInfo fileInfo(filename);
	QString basename = fileInfo.completeBaseName();
	QString suffix = fileInfo.suffix();

	// save working directory
	QDir oldcwd = QDir::current();

	// enter the xmf file folder so relative names for hdf5 files work
	QDir::setCurrent(fileInfo.absolutePath());

	HDF5Reader reader;
	reader.loud = 1;
	const QString fname = basename + ".h5";
	if (!reader.open(fname.toStdString()))
	{
		ISEG_ERROR("opening " << fname.toStdString());
		return 0;
	}

	// Target
	QString mapTargetName = imageReader->GetMapArrayNames()["Target"];
	if (mapTargetName.isEmpty())
	{
		ISEG_WARNING_MSG("no Target array, will initialize to 0...");
		bufferFloat.assign(bufferFloat.size(), 0.0f);
	}
	else
	{
		if (!reader.read(&bufferFloat[sliceoffset * this->Width * this->Height],
						mapTargetName.toAscii().data()))
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

int XdmfImageMerger::ReadTissues(
		XdmfImageReader* imageReader, const char* filename,
		std::vector<tissues_size_t>& bufferTissuesSizeT, size_t& sliceoffset)
{
	QFileInfo fileInfo(filename);
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

	// Tissue
	QString mapTissueName = imageReader->GetMapArrayNames()["Tissue"];
	std::string type;
	std::vector<HDF5Reader::size_type> dims;
	if (!reader.getDatasetInfo(type, dims, mapTissueName.toAscii().data()))
	{
		ISEG_ERROR_MSG("reading Tissue data type...");
		return 0;
	}

	size_t const currNrslices = imageReader->GetNumberOfSlices();
	size_t const N = (currNrslices * this->Width) * this->Height;

	if (type.compare("unsigned char") == 0)
	{
		std::vector<unsigned char> bufferUChar;
		try
		{
			bufferUChar.resize(N);
		}
		catch (std::length_error& le)
		{
			std::cerr << "bufferUChar length error: " << le.what();
			return 0;
		}
		if (mapTissueName.isEmpty())
		{
			ISEG_ERROR_MSG("no Tissue array...");
			return 0;
		}
		if (!reader.read(&bufferUChar[0], mapTissueName.toAscii().data()))
		{
			ISEG_ERROR_MSG("reading Tissue dataset...");
			return 0;
		}
		std::vector<unsigned char>::iterator iterFrom = bufferUChar.begin();
		std::vector<tissues_size_t>::iterator iterTo =
				bufferTissuesSizeT.begin() +
				(sliceoffset * this->Width) * this->Height;
		for (unsigned int k = 0; k < currNrslices; k++)
		{
			for (unsigned int j = 0; j < this->Height; j++)
			{
				for (unsigned int i = 0; i < this->Width; i++)
				{
					(*iterTo++) = (tissues_size_t)(*iterFrom++);
				}
			}
		}
		bufferUChar.clear();
	}
	else if (type.compare("unsigned short") == 0)
	{
		static_assert(std::is_same<unsigned short, tissues_size_t>::value,
				"Special case we read directly into the buffer.");
		if (mapTissueName.isEmpty())
		{
			ISEG_ERROR_MSG("no Tissue array...");
			return 0;
		}
		if (!reader.read(
						&bufferTissuesSizeT[(sliceoffset * this->Width) * this->Height],
						mapTissueName.toAscii().data()))
		{
			ISEG_ERROR_MSG("reading Tissue dataset...");
			return 0;
		}
	}
	else if (type.compare("unsigned int") == 0)
	{
		std::vector<unsigned int> bufferUInt;
		try
		{
			bufferUInt.resize(N);
		}
		catch (std::length_error& le)
		{
			ISEG_ERROR("bufferUInt length error: " << le.what());
			return 0;
		}
		if (mapTissueName.isEmpty())
		{
			ISEG_ERROR_MSG("no Tissue array...");
			return 0;
		}
		if (!reader.read(&bufferUInt[0], mapTissueName.toAscii().data()))
		{
			ISEG_ERROR_MSG("reading Tissue dataset...");
			return 0;
		}
		std::vector<unsigned int>::iterator iterFrom = bufferUInt.begin();
		std::vector<tissues_size_t>::iterator iterTo =
				bufferTissuesSizeT.begin() +
				sliceoffset * this->Width * this->Height;
		for (unsigned int k = 0; k < currNrslices; k++)
		{
			for (unsigned int j = 0; j < this->Height; j++)
			{
				for (unsigned int i = 0; i < this->Width; i++)
				{
					(*iterTo++) = (tissues_size_t)(*iterFrom++);
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
	sliceoffset += imageReader->GetNumberOfSlices();

	// restore working directory
	QDir::setCurrent(oldcwd.absolutePath());

	return 1;
}

} // namespace iseg