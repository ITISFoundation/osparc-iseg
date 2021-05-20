sudo apt-get update
sudo apt-get install cmake
sudo apt-get install libboost-date-time-dev
sudo apt-get install libboost-filesystem-dev
sudo apt-get install libboost-program-options-dev
sudo apt-get install libboost-random-dev
sudo apt-get install libboost-test-dev
sudo apt-get install libboost-thread-dev
sudo apt-get install libboost-timer-dev
sudo apt-get install libhdf5-dev
sudo apt-get install libeigen3-dev
sudo apt-get install libqt4-opengl-dev

export ISEG_DIR=/home/lloyd/src/iSeg
export DEPS_DIR=${ISEG_DIR}/deps

mkdir ${ISEG_DIR}
mkdir ${ISEG_DIR}/build
git clone https://github.com/ITISFoundation/osparc-iseg.git ${ISEG_DIR}/src

docker pull itisfoundation/iseg-ubuntu-vtk:8.2
docker create --name vtkcontainer itisfoundation/iseg-ubuntu-vtk:8.2 bash
docker cp vtkcontainer:/work ${DEPS_DIR}/vtk
docker pull itisfoundation/iseg-ubuntu-itk:5.0.1
docker create --name itkcontainer itisfoundation/iseg-ubuntu-itk:5.0.1 bash
docker cp itkcontainer:/work ${DEPS_DIR}/itk
cmake -S${ISEG_DIR}/src -B${ISEG_DIR}/build -DCMAKE_BUILD_TYPE=Release \
    -DHDF5_ROOT=/usr -DVTK_DIR=`ls -d -1 ${DEPS_DIR}/vtk/lib/cmake/vtk*` \
    -DITK_DIR=`ls -d -1 ${DEPS_DIR}/itk/lib/cmake/ITK*` \
    -DQT_QMAKE_EXECUTABLE=/usr/bin/qmake-qt4 -Wdev
