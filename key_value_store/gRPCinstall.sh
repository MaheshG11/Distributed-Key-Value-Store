#! /bin/bash

#Installations as mentioned on gRPC website for C++
export MY_INSTALL_DIR=$HOME/.local
mkdir -p $MY_INSTALL_DIR

export PATH="$MY_INSTALL_DIR/bin:$PATH"
apt update && apt-get install -y build-essential git protobuf-compiler autoconf libtool pkg-config sudo wget
wget -O cmake-linux.sh https://github.com/Kitware/CMake/releases/download/v3.30.3/cmake-3.30.3-linux-x86_64.sh
sh cmake-linux.sh -- --skip-license --prefix=$MY_INSTALL_DIR
rm cmake-linux.sh
git clone --recurse-submodules -b v1.66.0 --depth 1 --shallow-submodules https://github.com/grpc/grpc

cd grpc
mkdir -p cmake/build
pushd cmake/build
cmake -DgRPC_INSTALL=ON \
      -DgRPC_BUILD_TESTS=OFF \
      -DCMAKE_INSTALL_PREFIX=$MY_INSTALL_DIR \
      ../..
make -j 4
make install
popd