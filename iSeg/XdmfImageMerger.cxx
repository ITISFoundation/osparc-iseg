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
#include <vector>
#include <stdexcept>

#include <QString>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QDomDocument>
#include <QDomElement>
#include <QDomText>

#include <vtkSmartPointer.h>

#include <HDF5IO/HDF5Writer.h>
#include <HDF5IO/HDF5Reader.h>

#include "XdmfImageMerger.h"

using namespace HDF5;

XdmfImageMerger::XdmfImageMerger()
{
	this->NumberOfSlices = 0;
	this->Width = 0;
	this->Height = 0;
	this->Compression = 1;
	this->PixelSize = 0;
	this->Offset = 0;
	this->ImageSlices = 0;
	this->WorkSlices = 0;
	this->TissueSlices = 0;
	this->FileName = 0;
}

XdmfImageMerger::~XdmfImageMerger()
{
	delete[] this->FileName;
}

int XdmfImageMerger::Write()
{
	return this->InternalWrite(FileName, MergeFileNames, ImageSlices, WorkSlices, TissueSlices, NumberOfSlices, TotalNumberOfSlices, Width, Height, PixelSize, Offset, Compression);
}

int XdmfImageMerger::InternalWrite(const char *filename, std::vector<QString> &mergefilenames, float **slicesbmp, float **sliceswork, tissues_size_t **slicestissue, unsigned nrslices, unsigned nrslicesTotal, unsigned width, unsigned height, float *pixelsize, float *offset, int compression)
{
	// Parse xml files of merged projects
	std::vector<XdmfImageReader*> imageReaders;
	std::vector<QString>::iterator iterFilename;
	for (iterFilename = mergefilenames.begin(); iterFilename != mergefilenames.end(); ++iterFilename) {
		XdmfImageReader *reader = new XdmfImageReader();
		QString imageFilename = QFileInfo(*iterFilename).completeBaseName() + ".xmf";
		reader->SetFileName(QFileInfo(*iterFilename).dir().absFilePath(imageFilename).toAscii().data());
		if (reader->ParseXML() == 0) {
			std::cerr << "error in XdmfImageMerger::InternalWrite while parsing xmls" << std::endl;
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
	std::cerr << "storing current folder " << oldcwd.absolutePath().toAscii().data() << std::endl;

	// enter the xmf file folder so relative names for hdf5 files work
	QDir::setCurrent(fileInfo.absolutePath());
	std::cerr << "changing current folder to " << fileInfo.absolutePath().toAscii().data() << std::endl;

	const size_t N = (size_t)width*(size_t)height*(size_t)nrslicesTotal;
	const size_t slice_size = static_cast<size_t>(width) * height;

	std::cerr << "XdmfImageReader::InternalWrite()" << std::endl;
	std::cerr << "Width = " << width << std::endl;
	std::cerr << "Height = " << height << std::endl;
	std::cerr << "NumberOfSlices = " << nrslicesTotal << std::endl;
	std::cerr << "Total size = " << N << std::endl;

	HDF5Writer writer;
	const QString fname = basename + ".h5";
	if (!writer.open(fname.toAscii().data()))
	{
		std::cerr << "error opening " << fname.toAscii().data() << std::endl;
	}
	writer.compression = compression;

	// The slices are not contiguous in memory so we need to copy.

	// Source
	{
		std::cerr << "writing Source" << std::endl;
		// allocate in file
		float** const null = nullptr;
		if (writer.write(null, nrslicesTotal, slice_size, "Source"))
		{
			size_t offset = 0;

			// Current project
			writer.write(slicesbmp, nrslices, slice_size, "Source", offset);
			offset += nrslices*slice_size;

			// Merged projects
			std::vector<float> project_buffer;
			std::vector<XdmfImageReader*>::iterator iterReader;
			for (iterFilename = mergefilenames.begin(), iterReader = imageReaders.begin();
				iterFilename != mergefilenames.end(); ++iterFilename, ++iterReader) 
			{
				size_t junk = 0;
				size_t const project_slices = (*iterReader)->GetNumberOfSlices();
				project_buffer.resize(slice_size * project_slices);
				ReadSource(*iterReader, (*iterFilename).toAscii().data(), project_buffer, junk);

				std::vector<float*> slices(project_slices, nullptr);
				for (size_t i=0; i<project_slices; ++i)
				{
					slices[i] = &project_buffer[i*slice_size];
				}
				writer.write(slices.data(), project_slices, slice_size, "Source", offset);
				offset += project_slices*slice_size;
			}
		}
	}

	// Target
	{
		std::cerr << "writing Target" << std::endl;
		// allocate in file
		float** const null = nullptr;
		if (writer.write(null, nrslicesTotal, slice_size, "Target"))
		{
			size_t offset = 0;

			// Current project
			writer.write(sliceswork, nrslices, slice_size, "Target", offset);
			offset += nrslices*slice_size;

			// Merged projects
			std::vector<float> project_buffer;
			std::vector<XdmfImageReader*>::iterator iterReader;
			for (iterFilename = mergefilenames.begin(), iterReader = imageReaders.begin();
				iterFilename != mergefilenames.end(); ++iterFilename, ++iterReader)
			{
				size_t junk = 0;
				size_t const project_slices = (*iterReader)->GetNumberOfSlices();
				project_buffer.resize(slice_size * project_slices);
				ReadTarget(*iterReader, (*iterFilename).toAscii().data(), project_buffer, junk);

				std::vector<float*> slices(project_slices, nullptr);
				for (size_t i = 0; i < project_slices; ++i)
				{
					slices[i] = &project_buffer[i*slice_size];
				}
				writer.write(slices.data(), project_slices, slice_size, "Target", offset);
				offset += project_slices*slice_size;
			}
		}
	}

	// Tissue
	{
		std::cerr << "writing Tissue" << std::endl;
		// allocate in file
		tissues_size_t** const null = nullptr;
		if (writer.write(null, nrslicesTotal, slice_size, "Tissue"))
		{
			size_t offset = 0;

			// Current project
			writer.write(slicestissue, nrslices, slice_size, "Tissue", offset);
			offset += nrslices*slice_size;

			// Merged projects
			std::vector<tissues_size_t> project_buffer;
			std::vector<XdmfImageReader*>::iterator iterReader;
			for (iterFilename = mergefilenames.begin(), iterReader = imageReaders.begin();
				iterFilename != mergefilenames.end(); ++iterFilename, ++iterReader)
			{
				size_t junk = 0;
				size_t const project_slices = (*iterReader)->GetNumberOfSlices();
				project_buffer.resize(slice_size * project_slices);
				ReadTissues(*iterReader, (*iterFilename).toAscii().data(), project_buffer, junk);

				std::vector<tissues_size_t*> slices(project_slices, nullptr);
				for (size_t i = 0; i < project_slices; ++i)
				{
					slices[i] = &project_buffer[i*slice_size];
				}
				writer.write(slices.data(), project_slices, slice_size, "Tissue", offset);
				offset += project_slices*slice_size;
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
	text = doc.createTextNode(QString("%1 %2 %3").arg(offset[2]).arg(offset[1]).arg(offset[0]).toAscii().data());
	dataitem.appendChild(text);
	geometry.appendChild(dataitem);

	dataitem = doc.createElement("DataItem");
	dataitem.setAttribute("Name", "Spacing");
	dataitem.setAttribute("Format", "XML");
	dataitem.setAttribute("NumberType", "Float");
	dataitem.setAttribute("Precision", 4);
	dataitem.setAttribute("Dimensions", 3);
	text = doc.createTextNode(QString("%1 %2 %3").arg(pixelsize[2]).arg(pixelsize[1]).arg(pixelsize[0]).toAscii().data());
	dataitem.appendChild(text);
	geometry.appendChild(dataitem);

	grid.appendChild(geometry);

	QString qdims = QString("%1 %2 %3").arg(nrslicesTotal).arg(height).arg(width).toAscii().data();

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
	switch (sizeof(tissues_size_t)) {
	case 1:
		dataitem.setAttribute("NumberType", "UChar");
		dataitem.setAttribute("Precision", 1);
		break;
	case 2:
		dataitem.setAttribute("NumberType", "UInt");
		dataitem.setAttribute("Precision", 4);
		break;
	default:
		std::cerr << "tissues_size_t not supported!" << std::endl;
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
	std::cerr << "restored current folder " << QDir::current().absolutePath().toAscii().data() << std::endl;

	for (XdmfImageReader* r: imageReaders)
	{
		delete r;
	}

	return 1;
}

int XdmfImageMerger::ReadSource(XdmfImageReader *imageReader, const char *filename, std::vector<float> &bufferFloat, size_t &sliceoffset)
{
	QFileInfo fileInfo(filename);
	QString basename = fileInfo.completeBaseName();
	QString suffix = fileInfo.suffix();

	// save working directory
	QDir oldcwd = QDir::current();
	std::cerr << "storing current folder " << oldcwd.absolutePath().toAscii().data() << std::endl;

	// enter the xmf file folder so relative names for hdf5 files work
	QDir::setCurrent(fileInfo.absolutePath());
	std::cerr << "changing current folder to " << fileInfo.absolutePath().toAscii().data() << std::endl;

	HDF5Reader reader;
	reader.loud = 1;
	const QString fname = basename + ".h5";
	if (!reader.open(fname.toAscii().data()))
	{
		std::cerr << "error opening " << fname.toAscii().data() << std::endl;
		return 0;
	}

	// Source
	QString mapSourceName = imageReader->GetMapArrayNames()["Source"];
	if (mapSourceName.isEmpty())
	{
		std::cerr << "Error, no Source array..." << std::endl;
		return 0;
	}
	if (!reader.read(&bufferFloat[sliceoffset*this->Width*this->Height], mapSourceName.toAscii().data()))
	{
		std::cerr << "Error reading Source dataset..." << std::endl;
		return 0;
	}
	sliceoffset += imageReader->GetNumberOfSlices();

	// restore working directory
	QDir::setCurrent(oldcwd.absolutePath());
	std::cerr << "restored current folder " << QDir::current().absolutePath().toAscii().data() << std::endl;

	return 1;
}

int XdmfImageMerger::ReadTarget(XdmfImageReader *imageReader, const char *filename, std::vector<float> &bufferFloat, size_t &sliceoffset)
{
	QFileInfo fileInfo(filename);
	QString basename = fileInfo.completeBaseName();
	QString suffix = fileInfo.suffix();

	// save working directory
	QDir oldcwd = QDir::current();
	std::cerr << "storing current folder " << oldcwd.absolutePath().toAscii().data() << std::endl;

	// enter the xmf file folder so relative names for hdf5 files work
	QDir::setCurrent(fileInfo.absolutePath());
	std::cerr << "changing current folder to " << fileInfo.absolutePath().toAscii().data() << std::endl;

	HDF5Reader reader;
	reader.loud = 1;
	const QString fname = basename + ".h5";
	if (!reader.open(fname.toAscii().data()))
	{
		std::cerr << "error opening " << fname.toAscii().data() << std::endl;
		return 0;
	}

	// Target
	QString mapTargetName = imageReader->GetMapArrayNames()["Target"];
	if (mapTargetName.isEmpty())
	{
		std::cerr << "Warning, no Target array, will initialize to 0..." << std::endl;
		bufferFloat.assign(bufferFloat.size(), 0.0f);
	}
	else
	{
		if (!reader.read(&bufferFloat[sliceoffset*this->Width*this->Height], mapTargetName.toAscii().data()))
		{
			std::cerr << "Error reading Target dataset..." << std::endl;
			return 0;
		}
	}
	sliceoffset += imageReader->GetNumberOfSlices();

	// restore working directory
	QDir::setCurrent(oldcwd.absolutePath());
	std::cerr << "restored current folder " << QDir::current().absolutePath().toAscii().data() << std::endl;

	return 1;
}

int XdmfImageMerger::ReadTissues(XdmfImageReader *imageReader, const char *filename, std::vector<tissues_size_t> &bufferTissuesSizeT, size_t &sliceoffset)
{
	QFileInfo fileInfo(filename);
	QString basename = fileInfo.completeBaseName();
	QString suffix = fileInfo.suffix();

	// save working directory
	QDir oldcwd = QDir::current();
	std::cerr << "storing current folder " << oldcwd.absolutePath().toAscii().data() << std::endl;

	// enter the xmf file folder so relative names for hdf5 files work
	QDir::setCurrent(fileInfo.absolutePath());
	std::cerr << "changing current folder to " << fileInfo.absolutePath().toAscii().data() << std::endl;

	HDF5Reader reader;
	const QString fname = basename + ".h5";
	if (!reader.open(fname.toAscii().data()))
	{
		std::cerr << "error opening " << fname.toAscii().data() << std::endl;
		return 0;
	}

	// Tissue
	QString mapTissueName = imageReader->GetMapArrayNames()["Tissue"];
	std::string type;
	std::vector<HDF5::HDF5Reader::size_type> dims;
	if (!reader.getDatasetInfo(type, dims, mapTissueName.toAscii().data())) {
		std::cerr << "Error reading Tissue data type..." << std::endl;
		return 0;
	}

	size_t const currNrslices = imageReader->GetNumberOfSlices();
	size_t const N = (currNrslices*this->Width)*this->Height;
	std::cerr << "N = " << N << std::endl;
	if (type.compare("unsigned char") == 0) {
		std::vector<unsigned char> bufferUChar;
		try {
			bufferUChar.resize(N);
		}
		catch (std::length_error& le) {
			std::cerr << "bufferUChar length error: " << le.what() << std::endl;
			return 0;
		}
		if (mapTissueName.isEmpty()) {
			std::cerr << "Error, no Tissue array..." << std::endl;
			return 0;
		}
		if (!reader.read(&bufferUChar[0], mapTissueName.toAscii().data())) {
			std::cerr << "Error reading Tissue dataset..." << std::endl;
			return 0;
		}
		std::vector<unsigned char>::iterator iterFrom = bufferUChar.begin();
		std::vector<tissues_size_t>::iterator iterTo = bufferTissuesSizeT.begin() + (sliceoffset*this->Width)*this->Height;
		for (unsigned int k = 0; k < currNrslices; k++) {
			for (unsigned int j = 0; j < this->Height; j++) {
				for (unsigned int i = 0; i < this->Width; i++) {
					(*iterTo++) = (tissues_size_t)(*iterFrom++);
				}
			}
		}
		bufferUChar.clear();

	}
	else if (type.compare("unsigned short") == 0) {
		static_assert(std::is_same<unsigned short, tissues_size_t>::value, "Special case we read directly into the buffer.");
		if (mapTissueName.isEmpty()) {
			std::cerr << "Error, no Tissue array..." << std::endl;
			return 0;
		}
		if (!reader.read(&bufferTissuesSizeT[(sliceoffset*this->Width)*this->Height], mapTissueName.toAscii().data())) {
			std::cerr << "Error reading Tissue dataset..." << std::endl;
			return 0;
		}
	}
	else if (type.compare("unsigned int") == 0) {
		std::vector<unsigned int> bufferUInt;
		try {
			bufferUInt.resize(N);
		}
		catch (std::length_error& le) {
			std::cerr << "bufferUInt length error: " << le.what() << std::endl;
			return 0;
		}
		if (mapTissueName.isEmpty()) {
			std::cerr << "Error, no Tissue array..." << std::endl;
			return 0;
		}
		if (!reader.read(&bufferUInt[0], mapTissueName.toAscii().data())) {
			std::cerr << "Error reading Tissue dataset..." << std::endl;
			return 0;
		}
		std::vector<unsigned int>::iterator iterFrom = bufferUInt.begin();
		std::vector<tissues_size_t>::iterator iterTo = bufferTissuesSizeT.begin() + sliceoffset*this->Width*this->Height;
		for (unsigned int k = 0; k < currNrslices; k++) {
			for (unsigned int j = 0; j < this->Height; j++) {
				for (unsigned int i = 0; i < this->Width; i++) {
					(*iterTo++) = (tissues_size_t)(*iterFrom++);
				}
			}
		}
		bufferUInt.clear();
	}
	else {
		std::cerr << "Error, Tissue data type not supported..." << std::endl;
		return 0;
	}
	sliceoffset += imageReader->GetNumberOfSlices();

	// restore working directory
	QDir::setCurrent(oldcwd.absolutePath());
	std::cerr << "restored current folder " << QDir::current().absolutePath().toAscii().data() << std::endl;

	return 1;
}