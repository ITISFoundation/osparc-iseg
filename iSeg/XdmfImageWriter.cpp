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

#include "XdmfImageWriter.h"

#include "Data/ScopedTimer.h"

#include "Core/ColorLookupTable.h"
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

#include <boost/format.hpp>

#include <stdexcept>
#include <vector>

namespace iseg {

XdmfImageWriter::XdmfImageWriter()
{
	this->NumberOfSlices = 0;
	this->Width = 0;
	this->Height = 0;
	this->Compression = 1;
	this->PixelSize = nullptr;
	this->ImageSlices = nullptr;
	this->WorkSlices = nullptr;
	this->TissueSlices = nullptr;
	this->FileName = nullptr;
	this->CopyToContiguousMemory = false;
}

XdmfImageWriter::XdmfImageWriter(const char* filepath) : XdmfImageWriter()
{
	if (filepath && strlen(filepath) > 0)
	{
		SetFileName(filepath);
	}
}

XdmfImageWriter::~XdmfImageWriter() { delete[] this->FileName; }

bool XdmfImageWriter::Write(bool naked)
{
	return this->InternalWrite(FileName, ImageSlices, WorkSlices, TissueSlices,
			NumberOfSlices, Width, Height, PixelSize,
			ImageTransform, Compression, naked);
}

bool XdmfImageWriter::WriteColorLookup(const ColorLookupTable* lut, bool naked)
{
	if (lut == nullptr)
		return true;

	ScopedTimer timer("WriteColorLookup");

	QString qFileName(this->FileName);
	QFileInfo fileInfo(qFileName);
	QString basename = fileInfo.completeBaseName();
	QString suffix = fileInfo.suffix();

	// save working directory
	QDir oldcwd = QDir::current();
	QDir::setCurrent(fileInfo.absolutePath());

	HDF5Writer writer;
	writer.compression = Compression;
	QString fname;
	if (naked)
		fname = basename + "." + suffix;
	else
		fname = basename + ".h5";
	if (!writer.open(fname.toAscii().data(), "append"))
	{
		ISEG_ERROR("opening " << fname.toAscii().data());
		return false;
	}
	writer.compression = Compression;

	// create top level group
	writer.createGroup("/Lut");

	// write size and version
	std::vector<HDF5Writer::size_type> dim_scalar(1, 1);
	int version = 3;
	if (!writer.write(&version, dim_scalar, "/Lut/version"))
	{
		ISEG_ERROR_MSG("writing LUT version");
		return false;
	}
	int num_colors = static_cast<int>(lut->NumberOfColors());
	if (!writer.write(&num_colors, dim_scalar, "/Lut/size"))
	{
		ISEG_ERROR_MSG("writing LUT size");
		return false;
	}

	// write colors
	std::vector<HDF5Writer::size_type> dim_rgb(1, 3 * num_colors);
	std::vector<unsigned char> colors(3 * num_colors);
	for (int i = 0; i < num_colors; ++i)
	{
		lut->GetColor(static_cast<size_t>(i), &colors[i * 3]);
	}

	if (!writer.write(colors.data(), dim_rgb, "/Lut/colors"))
	{
		ISEG_ERROR_MSG("writing LUT size");
		return false;
	}

	QDir::setCurrent(oldcwd.absolutePath());

	return true;
}

int XdmfImageWriter::InternalWrite(const char* filename, float** slicesbmp,
		float** sliceswork,
		tissues_size_t** slicestissue,
		unsigned nrslices, unsigned width,
		unsigned height, float* pixelsize,
		Transform& transform, int compression,
		bool naked)
{
	QString qFileName(filename);
	QFileInfo fileInfo(qFileName);
	QString basename = fileInfo.completeBaseName();
	QString suffix = fileInfo.suffix();

	// save working directory
	QDir oldcwd = QDir::current();

	std::string abc(fileInfo.absolutePath().toAscii().data());

	// enter the xmf file folder so relative names for hdf5 files work
	QDir::setCurrent(fileInfo.absolutePath());

	const size_t N = (size_t)width * (size_t)height * (size_t)nrslices;

	std::vector<HDF5Writer::size_type> dims(3);
	dims[0] = width;
	dims[1] = height;
	dims[2] = nrslices;

	ISEG_INFO("Writing " << filename << ": " << width << " x " << height << " x " << nrslices);

	HDF5Writer writer;
	writer.chunkSize.resize(1, width * height);
	QString fname;
	if (naked)
		fname = basename + "." + suffix;
	else
		fname = basename + ".h5";
	if (!writer.open(fname.toAscii().data()))
	{
		ISEG_ERROR("opening " << fname.toStdString());
	}
	writer.compression = compression;

	// The slices are not contiguous in memory so we need to copy.
	if (this->CopyToContiguousMemory)
	{
		// Source
		std::vector<float> bufferFloat;
		try
		{
			// vector throws a length_error if resized above max_size
			ISEG_INFO("N = " << N << ", bufferFloat.max_size() = " << bufferFloat.max_size());
			bufferFloat.resize(N);
		}
		catch (std::exception& le)
		{
			ISEG_ERROR_MSG(le.what());
			return 0;
		}

		for (size_t n = 0, k = 0; k < nrslices; k++)
		{
			size_t pos = 0;
			for (unsigned j = 0; j < height; j++)
			{
				for (unsigned i = 0; i < width; i++, pos++)
				{
					bufferFloat[n] = slicesbmp[k][pos];
					n++;
				}
			}
		}

		if (!writer.write(bufferFloat, "Source"))
		{
			ISEG_ERROR_MSG("writing Source");
		}

		// Target
        if (sliceswork != nullptr)
        {
            for (size_t n = 0, k = 0; k < nrslices; k++)
            {
                size_t pos = 0;
                for (unsigned j = 0; j < height; j++)
                {
                    for (unsigned i = 0; i < width; i++, pos++)
                    {
                        bufferFloat[n] = sliceswork[k][pos];
                        n++;
                    }
                }
            }

            if (!writer.write(bufferFloat, "Target"))
            {
                ISEG_ERROR_MSG("writing Target");
            }
        }

		bufferFloat.clear();

		// Tissue
		std::vector<tissues_size_t> bufferTissuesSizeT;
		try
		{
			// vector throws a length_error if resized above max_size
			ISEG_INFO("N = " << N << ", bufferTissuesSizeT.max_size() = " << bufferTissuesSizeT.max_size());
			bufferTissuesSizeT.resize(N);
		}
		catch (std::exception& le)
		{
			ISEG_ERROR_MSG(le.what());
			return 0;
		}

		for (size_t n = 0, k = 0; k < nrslices; k++)
		{
			size_t pos = 0;
			for (unsigned j = 0; j < height; j++)
			{
				for (unsigned i = 0; i < width; i++, pos++)
				{
					bufferTissuesSizeT[n] = slicestissue[k][pos];
					n++;
				}
			}
		}

		if (!writer.write(bufferTissuesSizeT, "Tissue"))
		{
			ISEG_ERROR_MSG("writing Tissue");
		}
	}
	else // write slice-by-slice
	{
		ScopedTimer timer("Write Source");
		if (!writer.write(slicesbmp, nrslices, dims[0] * dims[1], "Source"))
		{
			ISEG_ERROR_MSG("writing Source");
		}
		timer.new_scope("Write Target");
		if (!writer.write(sliceswork, nrslices, dims[0] * dims[1], "Target"))
		{
			ISEG_ERROR_MSG("writing Target");
		}
		timer.new_scope("Write Tissue");
		if (!writer.write(slicestissue, nrslices, dims[0] * dims[1], "Tissue"))
		{
			ISEG_ERROR_MSG("writing Tissue");
		}
	}

	float offset[3], dc[6];
	transform.getOffset(offset);
	for (unsigned short i = 0; i < 3; i++)
	{
		dc[i] = transform[i][0];
		dc[i + 3] = transform[i][1];
	}

	//if(naked)
	{
		//write dimensions, pixelsize, offset, dc,
		std::vector<HDF5Writer::size_type> shape;
		shape.resize(1);
		int dimension[3];
		dimension[0] = dims[0];
		dimension[1] = dims[1];
		dimension[2] = dims[2];
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

		float rotation[9];
		for (int k = 0; k < 3; ++k)
		{
			rotation[k * 3 + 0] = ImageTransform[k][0];
			rotation[k * 3 + 1] = ImageTransform[k][1];
			rotation[k * 3 + 2] = ImageTransform[k][2];
		}
		shape[0] = 9;
		if (!writer.write(rotation, shape, std::string("rotation")))
		{
			ISEG_ERROR_MSG("writing rotation");
		}
	}

	writer.close();

	if (!naked)
	{
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
												.arg(nrslices)
												.arg(height)
												.arg(width)
												.toAscii()
												.data();

		QString realName = basename;
		if (basename.right(4) == QString("Temp"))
			realName = realName.left(realName.length() - 4);

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
		text = doc.createTextNode((realName + ".h5:/Source").toAscii().data());
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
		text = doc.createTextNode((realName + ".h5:/Target").toAscii().data());
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
		default: std::cerr << "tissues_size_t not supported!" << endl; return 0;
		}
		dataitem.setAttribute("Format", "HDF");
		dataitem.setAttribute("Dimensions", qdims.toAscii().data());
		text = doc.createTextNode((realName + ".h5:/Tissue").toAscii().data());
		dataitem.appendChild(text);
		attribute.appendChild(dataitem);
		grid.appendChild(attribute);

		QFile file(qFileName);
		if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
			return 0;

		QTextStream out(&file);
		out << doc;
		file.close();
	}

	// restore working directory
	QDir::setCurrent(oldcwd.absolutePath());

	return 1;
}

}// namespace iseg