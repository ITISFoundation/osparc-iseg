### Compiler

- iSEG has been successfully built using Visual Studio 2015 with *x64* C++ compiler

### 3rd Party Libraries compilation instructions

- [Boost installer](https://dl.bintray.com/boostorg/release/1.64.0/binaries): boost_1_64_0-msvc-14.0, or newer

  1. Download the installer _boost_1_64_0_msvc-14.0-64.exe_
  2. Run the installer and follow the default instructions

- [HDF5 installer](https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.8/hdf5-1.8.18/bin/windows): v1.8.18, or newer

  1. Download the installer _hdf5-1.8.18-win64-vs2013-shared.zip_
  2. Unzip the installer
  3. Run the installer and follow the default instructions

- [ITK source code](https://sourceforge.net/projects/itk/files/itk/4.12/InsightToolkit-4.12.2.tar.gz/download): v4.12.2, or newer

  1. Download and unzip the ITK folder
  2. CMake (source ITK folder, build in separate folder), press _Configure_
  3. Press configure and select VS Studio 14 2015 Win64
  4. Press _Advanced_ checkbox and select _BUILD_SHARED_LIBS_ , press configure again (CMake will find the ZLib library installed with the HDF5 package above)
  5. Press _Generate_
  6. Press _Open Project_ and build the solution in Debug and Release

- [VTK source code](https://www.vtk.org/files/release/7.1/VTK-7.1.0.tar.gz): v7.1.0, or newer

  1. Download and unzip the VTK folder
  2. CMake (source VTK folder, build in separate folder), press _Configure_
  3. Set _ISEG_VTK_OPENGL2_ depending on the value of _VTK_RENDERING_BACKEND_ (_OpenGL_ or _OpenGL2_)
  4. Press _Generate_
  5. Press _Open Project_ and build the solution in Debug and Release

- [Qt](https://download.qt.io/official_releases/qt/4.8/4.8.7/qt-everywhere-opensource-src-4.8.7.tar.gz): v4.8.7 (min 4.8.6)

  1. Download and unzip the Qt folder
  2. Apply the patch located in the [repo root folder](Thirdparty/Qt/02-fix_build_with_msvc2015-45e8f4ee.diff)(02-fix_build_with_msvc2015-45e8f4ee.diff) ([based on these instructions](https://stackoverflow.com/questions/32848962/how-to-build-qt-4-8-6-with-visual-studio-2015-without-official-support))
  3. Open a VisualStudio 2015 x64 prompt
  4. Go to downloaded Qt folder (cd _%QTFOLDER%_)
  5. Input _set Path=%PATH%;%QTFOLDER%\bin_
  6. Input _configure -debug-and-release -opensource -confirm-license -opengl desktop -nomake examples -nomake tests_ in the command window
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
