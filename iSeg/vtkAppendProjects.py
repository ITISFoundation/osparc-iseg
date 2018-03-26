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
##




# Title: vtkAppendProjects
#
# Description: Append iSeg projects.
#
# Bugs:
#



import os,sys,string
from optparse import OptionParser
# import xml.dom.minidom as minidom
# import tables

from vtk import *
# from Xdmf import *
from libvtkXdmfPython import *
# from numpy import *
from libvtkmyFilteringPython import *
from libvtkmyImagingPython import *

usage = "usage: %prog [options] inputs output"
parser = OptionParser(usage=usage)
parser.add_option("--shift", type=int, default=0)
# parser.add_option("--background", type=int, default=0)
parser.add_option("--preserve", type=int, default=1)
parser.add_option("--axis", type=int, default=0)
# parser.add_option("--threads", type=int, default=1)

(opts, args) = parser.parse_args()
if len(args)<3:
	parser.error("incorrect number of arguments")

NINPUTS = len(args)-1
print "got", NINPUTS, "inputs"

# append = vtkImageAppend()
append = vtkImageAppend2()
# append.SetNumberOfThreads(1)
if opts.preserve:
	print "Preserving extents..."
	append.SetPreserveExtents(1)
else:
	print "Appending along axis..."
	append.SetPreserveExtents(0)
	append.SetAppendAxis(opts.axis)

append.SetUseTransparency(1)
# append.SetIntTransparentValue(opts.background)
# append.SetLongTransparentValueMin(0)
# append.SetLongTransparentValueMax(0)
# append.SetDoubleTransparentValueMin(-20.0)
# append.SetDoubleTransparentValueMax(20.0)

for i in range(NINPUTS):
	filename = args[i]
	print i, filename

	reader = vtkXdmfReader()
	reader.SetFileName(filename)
	reader.UpdateInformation()
	if not reader.IsFileStructuredPoints:
		print reader.GetClassName()
		exit(0)
	reader.SetPointArrayStatus("Source",1)
	reader.SetPointArrayStatus("Target",0)
	reader.SetPointArrayStatus("Tissue",1)
	reader.Update()

	# reader = vtkXMLImageDataReader()
	# reader.SetFileName(filename)
	# reader.Update()

	# info = vtkImageChangeInformation()
	# ext = reader.GetOutputDataObject(0).GetExtent()
	# info.Update()

	# dataset = reader.GetOutputDataObject(0)
	dataset = vtkImageData()
	dataset.DeepCopy(reader.GetOutputDataObject(0))
	# dataset.DeepCopy(reader.GetOutput())
	# dataset.DeepCopy(info.GetOutput())
	# dataset.GetPointData().SetActiveScalars("Source")


	org = dataset.GetOrigin()
	spc = dataset.GetSpacing()
	ext = dataset.GetExtent()

	print org
	print spc
	print ext

	if opts.shift:
		# The first dataset in the list becomes the reference.
		if i==0:
			org0 = org
			print "Reference origin:", org0
		else:
			dataset.SetOrigin(org0)
			print "new origin:", dataset.GetOrigin()
			dorg = [org[0]-org0[0], org[1]-org0[1], org[2]-org0[2]]
			dext = [int(dorg[0]/spc[0]), int(dorg[1]/spc[1]), int(dorg[2]/spc[2])]
			print "dext:", dext
			ext1 = [ext[0]+dext[0], ext[1]+dext[0], ext[2]+dext[1], ext[3]+dext[1], ext[4]+dext[2], ext[5]+dext[2]]
			print "new extent:", ext1
			dataset.SetExtent(ext1)

	# writer = vtkXMLImageDataWriter()
	# writer.SetInput(dataset)
	# writer.SetFileName("%s.vti" % filename)
	# writer.Write()

	dataset.SetScalarTypeToUnsignedChar()
	append.AddInput(dataset)



print "updating..."
append.Update()
output = append.GetOutput()
print output.GetOrigin()

"""
print "resampling..."
output.GetPointData().SetActiveScalars("Source")
output.SetScalarTypeToFloat()
resample = vtkImageReslice()
resample.SetInputArrayToProcess(0,0,0,0,"Source")
resample.SetInput(output)
resample.SetOutputSpacing(5,5,5)
resample.Update()

print "writing..."
writer = vtkXMLImageDataWriter()
# writer.SetInput(output)
writer.SetInput(resample.GetOutput())
writer.SetFileName("out-1.vti")
writer.Write()

output.GetPointData().SetActiveScalars("Tissue")
output.SetScalarTypeToUnsignedChar()
resample = vtkImageReslice()
resample.SetInputArrayToProcess(0,0,0,0,"Tissue")
resample.SetInput(output)
resample.SetOutputSpacing(5,5,5)
resample.Update()

print "writing..."
writer.SetInput(resample.GetOutput())
writer.SetFileName("out-2.vti")
writer.Write()
"""

print "writing..."
filename = args[NINPUTS]
print "writing output file", filename
writer = vtkXdmfWriter()
writer.SetInput(output)
writer.SetFileName(filename)
writer.Write()

print "END"
