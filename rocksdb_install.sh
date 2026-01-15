#! /bin/bash

# These exports configures cmake path 
export MY_INSTALL_DIR=$HOME/.local
export PATH="$MY_INSTALL_DIR/bin:$PATH"


cd project
sudo apt-get install -y libgflags-dev libsnappy-dev zlib1g-dev libbz2-dev libzstd-dev
git clone https://github.com/facebook/rocksdb.git
cd rocksdb
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release .. 
make -j$(nproc)
sudo make install
