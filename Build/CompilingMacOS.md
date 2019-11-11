### Compiler

- iSEG has been successfully built using clang++ version 9.1.0

### 3rd Party Libraries compilation instructions

- [Boost](https://www.boost.org/): Boost 1.64, or newer

  1. Install using homebrew: `brew install boost`
  
- [HDF5](https://support.hdfgroup.org/HDF5/): v1.10.1, or newer

  1. Install using homebrew: `brew install hdf5`

- [ITK source code](https://github.com/InsightSoftwareConsortium/ITK/releases/download/v5.0.1/InsightToolkit-5.0.1.tar.gz): v5.0, or newer

  1. Download and unzip the ITK folder
  2. CMake (source ITK folder, build in separate folder), press _Configure_
    - "-DCMAKE_CXX_FLAGS=-std=c++11"
  3. Press configure and select the `Unix Makefile` generator
  4. Press _Advanced_ checkbox and select _BUILD_SHARED_LIBS_ , press configure again (CMake will find the ZLib library installed with the HDF5 package above)
  5. Press _Generate_
  6. In build directory type `make`

- [VTK source code](https://www.vtk.org/files/release/8.2/VTK-8.2.0.tar.gz): v7.1.0, or newer

  1. Install using homebrew: `brew install vtk`

- [Qt](https://download.qt.io/official_releases/qt/4.8/4.8.7/qt-everywhere-opensource-src-4.8.7.tar.gz): v4.8.7 (min 4.8.6)

  1. Qt4 is not a standard package in homebrew
  2. To install it via homebrew follow these [instructions](https://github.com/cartr/homebrew-qt4)
    ```
    brew tap cartr/qt4
    brew tap-pin cartr/qt4
    brew install qt@4
    ```

### Compiling iSEG

  1. Clone the repository
  2. Using CMake GUI (source root folder, build in separate folder), press _Configure_
  3. Select the generator, e.g. `Xcode` or `Unix Makefile`
  4. Define _BOOST_ROOT_ as the folder where Boost library was installed (e.g. _/usr/local/Cellar/boost/1.67.0_1_), press _Configure_
  5. Define _HDF5_ROOT_ as the folder where HDF5 was installed (e.g. _/usr/local/Cellar/hdf5/1.10.2_1_), press _Configure_
  6. Define _ITK_DIR_ as the folder where ITK was built (e.g. _ITK_BUILD_DIR_), press _Configure_
  7. Define _QT_QMAKE_EXECUTABLE_ by selecting _qmake.exe_ in the folder where Qt was compiled/Installed (e.g. _/usr/local/Cellar/qt@4/4.8.7_3_), press _Configure_
  8. Define _VTK_DIR_ as the folder where VTK was built (e.g. _VTK_BUILD_DIR_), press _Configure_
  9. Press _Generate_, then _Open Project_
  10. Build the solution (e.g. type `make` if you are using the `Unix Makefiles` generator)
