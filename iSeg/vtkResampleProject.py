#!/usr/bin/env python
##
## Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
## 
## This file is part of iSEG
## (see https://github.com/ITISFoundation/osparc-iseg).
## 
## This software is released under the MIT License.
##  https://opensource.org/licenses/MIT
##


# Title: vtkResampleProject
#
# Description: Resample iSeg projects.
#
# Bugs: Unlike when using vtkXMLImageDataReader to read the input image, GetScalarType() is unspecified after reading using the vtkXdmfReader. This is either a bug or a consequence that XDMF does not have a concept of image scalars. Therefore, prior to any processing both the active scalars as well as the image scalar type must be set explicitly. The 3 internal arrays (Source, Target and Tissue) must be processed separately due to the limitations in the vtkImageResample, which can not process multiple arrays at once. Final multi-scalar image must also be assembled by hand by data copying, which has adverse effect on performance (memory usage).
#



import os,sys,string
from optparse import OptionParser
# import xml.dom.minidom as minidom
# import tables
# from Xdmf import *
# from libvtkXdmfPython import *
# from numpy import *
from vtk import *
from vtkmy import *

usage = "usage: %prog [options] input output"
parser = OptionParser(usage=usage)
parser.add_option("--spacing", default="1,1,1")
parser.add_option("--transform", type=int, default="0")
parser.add_option("--translation", default="0,0,0")
parser.add_option("--rotation_angles", default="0,0,0")
parser.add_option("--rotation_center", default="0,0,0")
parser.add_option("--crop", type=int, default=0)
# parser.add_option("--interpolation", default="NearestNeighbor")

(opts, args) = parser.parse_args()
if len(args)<2:
	parser.error("incorrect number of arguments")

infile = args[0]
outfile = args[1]

str = string.split(opts.spacing,',')
assert(len(str)==3)
sx = float(str[0])
sy = float(str[1])
sz = float(str[2])
print "spacing", sx,sy,sz

print "crop", opts.crop
print "transform", opts.transform

if opts.transform:
	str = string.split(opts.translation,',')
	assert(len(str)==3)
	tx = float(str[0])
	ty = float(str[1])
	tz = float(str[2])
	print "translation", tx,ty,tz

	str = string.split(opts.rotation_angles,',')
	assert(len(str)==3)
	rx = float(str[0])
	ry = float(str[1])
	rz = float(str[2])
	print "rotation angles", rx,ry,rz

	str = string.split(opts.rotation_center,',')
	assert(len(str)==3)
	cx = float(str[0])
	cy = float(str[1])
	cz = float(str[2])
	print "rotation center", cx,cy,cz

reader = vtkXdmfReader()
reader.SetFileName(infile)
reader.UpdateInformation()
if not reader.IsFileStructuredPoints:
	print reader.GetClassName()
	exit(0)

# FIXME TODO how to disable reading all attributes??
# reader.SetReadAllColorScalars(0)
# reader.SetReadAllFields(0)
# reader.SetReadAllNormals(0)
# reader.SetReadAllScalars(0)
# reader.SetReadAllTCoords(0)
# reader.SetReadAllTensors(0)
# reader.SetReadAllVectors(0)
# reader.ReadAllFieldsOff()
# reader.ReadAllScalarsOff()
reader.SetPointArrayStatus("Source",1)
reader.SetPointArrayStatus("Target",0)
reader.SetPointArrayStatus("Tissue",1)
reader.Update()
dataset = reader.GetOutputDataObject(0)

# reader = vtkXMLImageDataReader()
# reader.SetFileName(infile)
# reader.Update()
# dataset = reader.GetOutput()

print dataset.GetExtent()
print dataset.GetOrigin()
print dataset.GetSpacing()



print "resampling Source..."
dataset.GetPointData().SetActiveScalars("Source")
dataset.SetScalarTypeToFloat()
print dataset.GetPointData().GetScalars().GetName()
print dataset.GetPointData().GetScalars().GetDataType()
print dataset.GetScalarType()

if opts.transform:
	# Axis order compatible with ParaView's Display Tab.
	# Translation and rotation must have opposite signs. Center of rotation must not. The order of transforms is: translate to the center of rotation, rotate along X, then Y, then Z, translate back, translate to the final requested position.
	transform = vtkTransform()
	# transform.Translate(tx,ty,tz)
	transform.Translate(cx,cy,cz)
	transform.RotateX(rx)
	transform.RotateY(ry)
	transform.RotateZ(rz)
	transform.Translate(-cx,-cy,-cz)
	transform.Translate(tx,ty,tz)
	transform.Update()

reslice = vtkImageReslice()
reslice.SetInput(dataset)
if opts.transform:
	reslice.SetResliceTransform(transform)
reslice.SetInputArrayToProcess(0,0,0,0,"Source")
reslice.SetOutputSpacing(sx,sy,sz)
reslice.SetInterpolationModeToCubic()
reslice.SetAutoCropOutput(opts.crop)
reslice.Update()
source = vtkImageData()
source.DeepCopy(reslice.GetOutput())
source.GetPointData().GetScalars().SetName("Source")


"""
print "resampling Target..."
dataset.GetPointData().SetActiveScalars("Target")
dataset.SetScalarTypeToFloat()
print dataset.GetPointData().GetScalars().GetName()
print dataset.GetPointData().GetScalars().GetDataType()
print dataset.GetScalarType()

reslice.SetInputArrayToProcess(0,0,0,0,"Target")
reslice.SetOutputSpacing(sx,sy,sz)
reslice.SetInterpolationModeToCubic()
reslice.Update()
target = vtkImageData()
target.DeepCopy(reslice.GetOutput())
target.GetPointData().GetScalars().SetName("Target")
"""


print "resampling Tissue..."
dataset.GetPointData().SetActiveScalars("Tissue")
dataset.SetScalarTypeToUnsignedChar()
print dataset.GetPointData().GetScalars().GetName()
print dataset.GetPointData().GetScalars().GetDataType()
print dataset.GetScalarType()

reslice.SetInputArrayToProcess(0,0,0,0,"Tissue")
reslice.SetOutputSpacing(sx,sy,sz)
reslice.SetInterpolationModeToNearestNeighbor()
reslice.Update()
tissue = vtkImageData()
tissue.DeepCopy(reslice.GetOutput())
tissue.GetPointData().GetScalars().SetName("Tissue")

# writer = vtkXMLImageDataWriter()
# writer.SetInput(source)
# writer.SetFileName("Source.vti")
# writer.Write()
# writer.SetInput(target)
# writer.SetFileName("Target.vti")
# writer.Write()
# writer.SetInput(tissue)
# writer.SetFileName("Tissue.vti")
# writer.Write()

ext1 = source.GetExtent()
# ext2 = target.GetExtent()
ext3 = tissue.GetExtent()
# assert(ext1==ext2)
# assert(ext1==ext2)
assert(ext1==ext3)



print "creating output with extent", ext1
output = vtkImageData()
output.SetExtent(ext1)
output.SetSpacing(source.GetSpacing())
output.SetOrigin(source.GetOrigin())
output.GetPointData().AddArray(source.GetPointData().GetArray("Source"))
# output.GetPointData().AddArray(target.GetPointData().GetArray("Target"))
output.GetPointData().AddArray(tissue.GetPointData().GetArray("Tissue"))



# writer = vtkXMLImageDataWriter()
# writer.SetInput(output)
# writer.SetFileName("output.vti")
# writer.Write()



print "writing output file", outfile
writer = vtkXdmfWriter()
writer.SetInput(output)
writer.SetFileName(outfile)
writer.Write()
