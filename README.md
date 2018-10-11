![iSEG logo](iSeg/images/isegicon.png)
[![Build Status](https://travis-ci.com/ITISFoundation/osparc-iseg.svg?branch=master)](https://travis-ci.com/ITISFoundation/osparc-iseg)

## Description

The Medical Image Segmentation Tool Set (iSEG) is a fully integrated segmentation (including pre- and postprocessing) toolbox for the efficient, fast, and flexible generation of anatomical models from various types of imaging data. iSEG includes a variety of semi-automatic segmentation methods. The key strengths of iSEG are a) a powerful set of tools for manual segmentation (correction), b) support for massive image sizes, c) suport for segmenting complex models with a large number of tissues. iSEG features a plugin mechanism, which allows users to easily extend the application with custom widgets. The development of iSEG has been driven by the [Virtual Population](https://www.itis.ethz.ch/virtual-population/) project at the [IT'IS Foundation](https://www.itis.ethz.ch/), due to a lack of alternatives amongst open source and commercial tools.

## Compilation instructions

A defined set of 3rd party libraries are required in order to compile iSEG.
Instructions as to how these libraries must be installed and compiled are provided below.

### Required applications

The applications below are needed in order to compile iSEG and its dependencies.

- [CMake:](https://cmake.org/) 3.6 minimum
- Visual Studio 2015 or Xcode/Clang on MacOS
- [Git](https://git-scm.com/)

### Required 3rd party libraries

The libraries below are required to be compiled and installed on the system in order to compile iSEG. Platform specific instructions for each library are given below.

- Boost, [Boost license](http://www.boost.org/users/license.html)
- HDF5, [BSD-like license](https://support.hdfgroup.org/products/licenses.html)
- ITK, [Apache 2.0 license](https://itk.org/ITK/project/licenseversion1.html)
- VTK, [BSD license](https://www.vtk.org/licensing/)
- Qt 4.8, [GPL3 and LGPL2.1 license](https://www1.qt.io/licensing/)


### Compiling iSEG

- Windows/Visual Studio [instructions](Documentation/CompilingWindows.md)
- MacOS/Clang++ [instructions](Documentation/CompilingMacOS.md)