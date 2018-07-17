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

using namespace std;
using namespace iseg;

XdmfImageWriter::XdmfImageWriter()
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
		return 1;

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
		cerr << "error opening " << fname.toAscii().data() << endl;
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
		cerr << "error writing LUT version" << endl;
		return false;
	}
	int num_colors = lut->NumberOfColors();
	if (!writer.write(&num_colors, dim_scalar, "/Lut/size"))
	{
		cerr << "error writing LUT size" << endl;
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
		cerr << "error writing LUT size" << endl;
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
	cerr << "storing current folder " << oldcwd.absolutePath().toAscii().data()
			 << endl;

	std::string abc(fileInfo.absolutePath().toAscii().data());

	// enter the xmf file folder so relative names for hdf5 files work
	QDir::setCurrent(fileInfo.absolutePath());
	cerr << "changing current folder to "
			 << fileInfo.absolutePath().toAscii().data() << endl;

	const size_t N = (size_t)width * (size_t)height * (size_t)nrslices;

	vector<HDF5Writer::size_type> dims(3);
	dims[0] = width;
	dims[1] = height;
	dims[2] = nrslices;

	cerr << "XdmfImageReader::InternalWrite()" << endl;
	cerr << "Width = " << width << endl;
	cerr << "Height = " << height << endl;
	cerr << "NumberOfSlices = " << nrslices << endl;
	cerr << "Total size = " << N << endl;

	HDF5Writer writer;
	writer.chunkSize.resize(1, width * height);
	QString fname;
	if (naked)
		fname = basename + "." + suffix;
	else
		fname = basename + ".h5";
	if (!writer.open(fname.toAscii().data()))
	{
		cerr << "error opening " << fname.toAscii().data() << endl;
	}
	writer.compression = compression;

	// The slices are not contiguous in memory so we need to copy.
	if (this->CopyToContiguousMemory)
	{
		// Source
		vector<float> bufferFloat;
		try
		{
			// vector throws a length_error if resized above max_size
			cerr << "N = " << N
					 << ", bufferFloat.max_size() = " << bufferFloat.max_size()
					 << endl;
			bufferFloat.resize(N);
		}
		catch (length_error& le)
		{
			cerr << "bufferFloat length error: " << le.what() << endl;
			return 0;
		}

		size_t n = 0;
		for (int k = 0; k < nrslices; k++)
		{
			size_t pos = 0;
			for (int j = 0; j < height; j++)
			{
				for (int i = 0; i < width; i++, pos++)
				{
					bufferFloat[n] = slicesbmp[k][pos];
					n++;
				}
			}
		}

		if (!writer.write(bufferFloat, "Source"))
		{
			cerr << "error writing Source" << endl;
		}

		// Target
		n = 0;
		for (int k = 0; k < nrslices; k++)
		{
			size_t pos = 0;
			for (int j = 0; j < height; j++)
			{
				for (int i = 0; i < width; i++, pos++)
				{
					bufferFloat[n] = sliceswork[k][pos];
					n++;
				}
			}
		}

		if (!writer.write(bufferFloat, "Target"))
		{
			cerr << "error writing Target" << endl;
		}

		bufferFloat.clear();

		// Tissue
		vector<tissues_size_t> bufferTissuesSizeT;
		try
		{
			// vector throws a length_error if resized above max_size
			cerr << "N = " << N << ", bufferTissuesSizeT.max_size() = "
					 << bufferTissuesSizeT.max_size() << endl;
			bufferTissuesSizeT.resize(N);
		}
		catch (length_error& le)
		{
			cerr << "bufferTissuesSizeT length error: " << le.what() << endl;
			return 0;
		}

		n = 0;
		for (int k = 0; k < nrslices; k++)
		{
			size_t pos = 0;
			for (int j = 0; j < height; j++)
			{
				for (int i = 0; i < width; i++, pos++)
				{
					bufferTissuesSizeT[n] = slicestissue[k][pos];
					n++;
				}
			}
		}

		if (!writer.write(bufferTissuesSizeT, "Tissue"))
		{
			cerr << "error writing Tissue" << endl;
		}
	}
	else // write slice-by-slice
	{
		if (!writer.write(slicesbmp, nrslices, dims[0] * dims[1], "Source"))
		{
			cerr << "error writing Source" << endl;
		}
		if (!writer.write(sliceswork, nrslices, dims[0] * dims[1], "Target"))
		{
			cerr << "error writing Target" << endl;
		}
		if (!writer.write(slicestissue, nrslices, dims[0] * dims[1], "Tissue"))
		{
			cerr << "error writing Tissue" << endl;
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
			cerr << "error writing dimensions" << endl;
		}
		if (!writer.write(offset, shape, std::string("offset")))
		{
			cerr << "error writing offset" << endl;
		}
		if (!writer.write(pixelsize, shape, std::string("pixelsize")))
		{
			cerr << "error writing pixelsize" << endl;
		}
		shape[0] = 6;

		if (!writer.write(dc, shape, std::string("dc")))
		{
			cerr << "error writing dc" << endl;
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
			cerr << "error writing rotation" << endl;
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
		default: cerr << "tissues_size_t not supported!" << endl; return 0;
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
	cerr << "restored current folder "
			 << QDir::current().absolutePath().toAscii().data() << endl;

	return 1;
}
