#!/bin/bash

# http://redsymbol.net/articles/unofficial-bash-strict-mode/
set -euo pipefail
IFS=$'\n\t'

mkdir -p ${BUILD_DIR}
pushd ${BUILD_DIR}

cmake -H. \
    -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
    -DBOOST_ROOT=/usr \
    -DHDF5_ROOT=/usr \
    -DVTK_DIR=`ls -d -1 /data/deps/vtk/lib/cmake/vtk*` \
    -DITK_DIR=`ls -d -1 /data/deps/itk/lib/cmake/ITK*` \
    -DQT_QMAKE_EXECUTABLE=/usr/bin/qmake-qt4 \
    -Wdev \
    /data/osparc-iseg

make -j

cp /data/deps/vtk/lib/*.* ${BUILD_DIR}/bin
cp -r /data/deps/itk/lib/*.* ${BUILD_DIR}/bin

popd