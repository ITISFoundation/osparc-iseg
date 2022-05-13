FROM ubuntu:18.04

RUN apt-get update -y && \
    apt-get install -y --no-install-recommends \
    gcc \
    gdb \
    g++ \
    libboost-chrono-dev \
    libboost-date-time-dev \
    libboost-filesystem-dev \
    libboost-program-options-dev \
    libboost-random-dev \
    libboost-test-dev \
    libboost-thread-dev \
    libboost-timer-dev \
    libeigen3-dev \
    libhdf5-dev \
    libqt4-opengl-dev \
    libsm-dev \
    libssl-dev \
    libxt-dev \
    software-properties-common \
    wget && \
    rm -rf /var/lib/apt/lists

RUN export CMAKE_VERSION=3.22.3 && export PREFIX=/tmp/cmake && \
    wget https://github.com/Kitware/CMake/releases/download/v3.22.3/cmake-3.22.3-linux-x86_64.sh && \
	chmod +x cmake-${CMAKE_VERSION}-linux-x86_64.sh && \
	mkdir -p ${PREFIX} && \
	./cmake-${CMAKE_VERSION}-linux-x86_64.sh --prefix=${PREFIX} --skip-license && \
	cp ${PREFIX}/bin/* /usr/local/bin && \
	cp -r ${PREFIX}/share/cmake-3.22 /usr/local/share && \
	rm cmake-${CMAKE_VERSION}-linux-x86_64.sh

WORKDIR /data

RUN mkdir -p /data/deps

COPY --from=itisfoundation/iseg-ubuntu-vtk:8.2 /work /data/deps/vtk
COPY --from=itisfoundation/iseg-ubuntu-itk:5.2.1 /work /data/deps/itk
