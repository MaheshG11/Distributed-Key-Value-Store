#! /bin/bash

# These exports configures cmake path 
export MY_INSTALL_DIR=$HOME/.local
export PATH="$MY_INSTALL_DIR/bin:$PATH"


git clone https://github.com/eBay/NuRaft.git
cd NuRaft
sudo apt install -y libasio-dev libssl-dev libboost-all-dev
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release .. 
make -j$(nproc)
sudo make install