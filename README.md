#iSEG

![iSEG logo](iSeg/images/isegicon.png)

## Description

The Medical Image Segmentation Tool Set (iSEG) is a fully integrated segmentation (including pre- and postprocessing) toolbox for the efficient, fast, and flexible generation of anatomical models from various types of imaging data. iSEG includes a variety of semi-automatic segmentation methods. The key strengths of iSEG are a) a powerful set of tools for manual segmentation (correction), b) support for massive image sizes, c) suport for segmenting complex models with a large number of tissues. iSEG features a plugin mechanism, which allows users to easily extend the application with custom widgets. The development of iSEG has been driven by the [Virtual Population](https://www.itis.ethz.ch/virtual-population/) project at the [IT'IS Foundation](https://www.itis.ethz.ch/), due to a lack of alternatives amongst open source and commercial tools.

## Compilation instructions

A defined set of 3rd party libraries are required in order to compile iSEG.
Instructions as to how these libraries must be installed and compiled are provided below.

### Required applications

The application below must be installed on the system in order to compile iSEG and its dependencies.

- [CMake:](https://cmake.org/) v3 minimum
- Visual Studio 2015 with *x64* C++ compiler
- [Git](https://git-scm.com/)

### Required 3rd party libraries

The libraries below are required to be compiled and installed on the system in order to compile iSEG. Instructions for each library follows hereafter.

- Boost, [Boost license](http://www.boost.org/users/license.html)
- HDF5, [BSD-like license](https://support.hdfgroup.org/products/licenses.html)
- zlib, [ZLIB license](http://zlib.net/zlib_license.html)
- ITK, [Apache 2.0 license](https://itk.org/ITK/project/licenseversion1.html)
- VTK, [BSD license](https://www.vtk.org/licensing/)
- Qt 4.8, [GPL3 and LGPL2.1 license](https://www1.qt.io/licensing/)
- GraphCut, [LGPL license](http://cbia.fi.muni.cz/user_dirs/gc_doc/)
- GPU-Marching-Cubes, [Permissive license](https://github.com/smistad/GPU-Marching-Cubes/blob/master/LICENSE)

### Optional 3rd party libraries

- OpenCL (necessary to use GPU computations, usually available from the graphics hardware manufacturers)

### 3rd Party Libraries compilation instructions

- [Boost installer](https://dl.bintray.com/boostorg/release/1.64.0/binaries): boost_1_64_0-msvc-14.0

  1. Download the installer _boost_1_64_0_msvc-14.0-64.exe_
  2. Run the installer and follow the default instructions

- [HDF5 installer](https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.8/hdf5-1.8.18/bin/windows): v1.8.18

  1. Download the installer _hdf5-1.8.18-win64-vs2013-shared.zip_
  2. Unzip the installer
  3. Run the installer and follow the default instructions

- [ITK source code](https://sourceforge.net/projects/itk/files/itk/4.12/InsightToolkit-4.12.2.tar.gz/download): v4.12.2

  1. Download and unzip the ITK folder
  2. CMake (source ITK folder, build in separate folder), press _Configure_
  3. Press configure and select VS Studio 14 2015 Win64
  4. Press _Advanced_ checkbox and select _BUILD_SHARED_LIBS_ , press configure again (CMake will find the ZLib library installed with the HDF5 package above)
  5. Press _Generate_
  6. Press _Open Project_ and build the solution in Debug and Release

- [VTK source code](https://www.vtk.org/files/release/7.1/VTK-7.1.0.tar.gz): v7.1.0

  1. Download and unzip the VTK folder
  2. CMake (source VTK folder, build in separate folder), press _Configure_
  3. Select _CMAKE_CXX_MP_FLAG_, change _VTK_RENDERING_BACKEND_ to _OpenGL_ and press _Configure_ again
  4. Press _Generate_
  5. Press _Open Project_ and build the solution in Debug and Release

- [Qt](https://download.qt.io/official_releases/qt/4.8/4.8.7/qt-everywhere-opensource-src-4.8.7.tar.gz): v4.8.7 (min 4.8.6)

  1. Download and unzip the Qt folder
  2. Apply the patch located in the [repo root folder](02-fix_build_with_msvc2015-45e8f4ee.diff)(02-fix_build_with_msvc2015-45e8f4ee.diff) ([based on these instructions](https://stackoverflow.com/questions/32848962/how-to-build-qt-4-8-6-with-visual-studio-2015-without-official-support))
  3. Open a VisualStudio 2015 x64 prompt
  4. Go to downloaded Qt folder (cd _%QTFOLDER%_)
  5. Input _set Path=%PATH%;%QTFOLDER%\bin_
  6. Input _configure -debug-and-release -opensource -confirm-license opengl desktop -nomake examples -nomake tests_ in the command window
  7. Compile Qt by inputing _nmake_

### Compiling iSEG

  1. Clone the repository
  2. CMake (source root folder, build in separate folder), press _Configure_
  3. Define _BOOST_ROOT_ as the folder where Boost library was installed (e.g. _c:\boost_1_64_0_), press _Configure_
  4. Define _HDF5_ROOT_ as the folder where HDF5 was installed (e.g. _C:\Program Files\HDF_Group\HDF5\1.8.18_), press _Configure_
  5. Define _ITK_DIR_ as the folder where ITK was built (e.g. _ITK_BUILD_DIR_), press _Configure_
  6. Define _QT_QMAKE_EXECUTABLE_ by selecting _qmake.exe_ in the folder where Qt was compiled/Installed (e.g. _QT_DIR/bin/qmake.exe_), press _Configure_
  7. Define _VTK_DIR_ as the folder where VTK was built (e.g. _VTK_BUILD_DIR_), press _Configure_
  8. Press _Generate_, then _Open Project_
  9. In Visual Studio, build the solution (Debug/Release)