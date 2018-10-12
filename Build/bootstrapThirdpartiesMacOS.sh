#!/usr/bin/env bash

# TODO: passing directory names as argument would help to sync script with .travis.yml

# this is intentional, we want to install relative to calling directory,
# not inside script directory.
DEPS_DIR="${PWD}"

# lets define some travis stuff, so this script can run wo travis locally
`travis_retry` || alias travis_retry=''

# install via brew
which cmake || brew install cmake
brew ls --versions boost || brew install boost
brew ls --versions hdf5 || brew install hdf5
brew ls --versions vtk || brew install vtk
brew tap cartr/qt4 && brew tap-pin cartr/qt4 && (brew ls --versions qt@4 || brew install qt@4)

# install from source (could not find ITK tap)
ITK_URL="http://downloads.sourceforge.net/project/itk/itk/4.13/InsightToolkit-4.13.1.tar.gz"
test -d itk/lib || ((test -d itk/src || mkdir -p itk/src) && travis_retry wget --no-check-certificate --quiet -O - ${ITK_URL} | tar --strip-components=1 -xz -C itk/src && mkdir -p itk/build && cd itk/build && cmake ../src -DCMAKE_INSTALL_PREFIX=$PWD/.. -DModule_ITKReview=ON -DCMAKE_RULE_MESSAGES=OFF && make -j4 && make install && cd ${DEPS_DIR})
