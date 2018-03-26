/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */

#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <cmath>
#include <string>
#include <cassert>

#include <Vector.h>
#include <Matrix.h>

#include "HDF5Reader.h"
#include "HDF5Writer.h"

using namespace std;
using namespace Math;
using namespace HDF5;

const double eps = 1e-15;

int main(int argc, char **argv){

/*	hid_t file = H5Fcreate("test.h5", H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
	herr_t status = H5Fclose(file);
	return 0;*/
	
	int DIM1 = 10;
	vector<hsize_t> dims1(2);
	dims1[0] = 5, dims1[1] = 2;
	
	int DIM2 = 30;
	vector<hsize_t> dims2(3);
	dims2[0] = 5, dims2[1] = 3, dims2[2] = 2;
	
	int DIM3 = 50;
	vector<hsize_t> dims3(2);
	dims3[0] = 10, dims3[1] = 5;

	vector<double> x1(DIM1);
	for(int i=0; i<DIM1; i++)
		x1[i]=i+0.001;
	
	vector<int> x2(DIM2);
	for(int i=0; i<DIM2; i++)
		x2[i]=i;

	vector<unsigned char> x3(DIM3);
	for(int i=0; i<DIM3; i++)
		x3[i]=255;
	
	Vector<double> V1(10);
	for(int i=1; i<=V1.size(); i++)
		V1(i)=1.*i;
	
	Vector<int> V2(20);
	for(int i=1; i<=V2.size(); i++)
		V2(i)=i;
	
	Vector<unsigned char> V3(30);
	for(int i=1; i<=V3.size(); i++)
		V3(i)=i;
	
	Matrix<double> A1(3,2);
	for(int j=1; j<=A1.columns(); j++)
		for(int i=1; i<=A1.rows(); i++)
			A1(i,j)=10.*i+j;
	
	Matrix<int> A2(4,3);
	for(int j=1; j<=A2.columns(); j++)
		for(int i=1; i<=A2.rows(); i++)
			A2(i,j)=10*i+j;

	Matrix<unsigned char> A3(4,3);
	for(int j=1; j<=A3.columns(); j++)
		for(int i=1; i<=A3.rows(); i++)
			A3(i,j)=10*i+j;

// 	for(int i=0; i<DIM1; i++)
// 		cerr << x1[i] << " ";
// 	cerr << endl;
// 	for(int i=0; i<DIM2; i++)
// 		cerr << x2[i] << " ";
// 	cerr << endl;
// 	for(int i=0; i<DIM3; i++)
// 		cerr << int(x3[i]) << " ";
// 	cerr << endl;

	//cerr << "testing the tokenizer" << endl;
	
	HDF5Writer writer;
	writer.loud=1;
	
	// test the tokenizer
	string test;
	test = "/path/to/data";
	cerr << test << endl;
	HDF5Writer::tokenize(test);
	test = "/data";
	cerr << test << endl;
	HDF5Writer::tokenize(test);
	test = "data";
	cerr << test << endl;
	HDF5Writer::tokenize(test);
	test = "/";
	cerr << test << endl;
	HDF5Writer::tokenize(test);

	// test primitive datatypes
	cerr << "writing data" << endl;
	writer.open("test.h5");
	//	writer.open("test.h5","append");

// 	writer.compression = 9;
	writer.write(&x1[0], dims1, "x1");
	writer.createGroup("test");
	writer.write(&x2[0], dims2, "test/x2");
	writer.write(&x3[0], dims3, "test/x3");
	writer.write(V1,"V1");
	writer.write(A1,"A1");
	writer.write(V2,"V2");
	writer.write(A2,"A2");
	writer.write(V3,"V3");
	writer.write(A3,"A3");
	writer.close();
	
	
	cerr << "***   now reading back and checking   ***" << endl;
	vector<double> x11;
	vector<int> x22;
	vector<unsigned char> x33;
	vector<hsize_t> dims;
	int size = 0;
	
	HDF5Reader reader;
	reader.loud = 1;
	reader.open("test.h5");
	vector<string> names = reader.getGroupInfo("/");
	for(int i=0; i<names.size(); i++)
		cerr << "main : names = " << names[i] << endl;
	
	names = reader.getGroupInfo("/test");
	for(int i=0; i<names.size(); i++)
		cerr << "main : names = " << names[i] << endl;

	string dtype;
	
// 	if(!reader.getDatasetInfo("IdoNotExist",dtype, dims));
// 			cerr << "this should throw a warning of non-existent dataset" << endl;

	reader.getDatasetInfo(dtype, dims, "x1");
	cerr << "datatype = " << dtype << endl;
	for(int i=0; i<dims.size(); i++)
		cerr << dims[i] << endl;
	size = HDF5Reader::totalSize(dims);
	cerr << "total size = " << size << endl;
	x11.resize(size);
	reader.read(&x11[0], "x1");
	
	reader.getDatasetInfo(dtype, dims,"test/x2");
	cerr << "datatype = " << dtype << endl;
	for(int i=0; i<dims.size(); i++)
		cerr << dims[i] << endl;
	size = HDF5Reader::totalSize(dims);
	cerr << "total size = " << size << endl;
	x22.resize(size);
	reader.read(&x22[0],"test/x2");
	
	reader.getDatasetInfo(dtype, dims, "test/x3");
	cerr << "datatype = " << dtype << endl;
	for(int i=0; i<dims.size(); i++)
		cerr << dims[i] << endl;
	size = HDF5Reader::totalSize(dims);
	cerr << "total size = " << size << endl;
	x33.resize(size);
	reader.read(&x33[0],"test/x3");
// 	for(int i=0; i<size; i++)
// 		cerr << int(x33[i]) << endl;
	
	//cerr << reader.isHdf5(argv[1]) << endl;
	
	Vector<double> V11;
	Matrix<double> A11;
	Vector<int> V22;
	Matrix<int> A22;
	Vector<unsigned char> V33;
	Matrix<unsigned char> A33;
	reader.read(V11,"V1");
	reader.read(A11,"A1");
	reader.read(V22,"V2");
	reader.read(A22,"A2");
	reader.read(V33,"V3");
	reader.read(A33,"A3");
	reader.close();
	
	for(int i=0; i<DIM1; i++)
		if(fabs(double(x1[i]-x11[i]))>eps)
			return 1;
	for(int i=0; i<DIM2; i++)
		if(fabs(double(x2[i]-x22[i]))>eps)
			return 1;
	for(int i=0; i<DIM3; i++)
		if(fabs(double(x3[i]-x33[i]))>eps)
			return 1;
	for(int i=0; i<V1.size(); i++)
		if(fabs(double(V1[i]-V11[i]))>eps)
			return 1;
	for(int i=0; i<A1.size(); i++)
		if(fabs(double(A1[i]-A11[i]))>eps)
			return 1;
	for(int i=0; i<V2.size(); i++)
		if(fabs(double(V2[i]-V22[i]))>eps)
			return 1;
	for(int i=0; i<A2.size(); i++)
		if(fabs(double(A2[i]-A22[i]))>eps)
			return 1;
	for(int i=0; i<V3.size(); i++)
		if(fabs(double(V3[i]-V33[i]))>eps)
			return 1;
	for(int i=0; i<A3.size(); i++)
		if(fabs(double(A3[i]-A33[i]))>eps)
			return 1;

	cerr << "test passed" << endl;
	
// // 	Matrix<double> points;
// 	Matrix<int> cells;
// // 	
// 	HDF5Reader reader2;
// 	reader2.loud = 1;
// 	reader2.open("/home/domel/data/sphereInCylinder/crash.h5");
// // 	reader2.read(points,"points");
// 	reader2.read(cells,"triangles");
// 	reader2.close();
// 	cerr << "test2 passed" << endl;
	
	return 0;

}
