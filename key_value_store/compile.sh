#! /bin/bash

# These exports configures cmake path 
export MY_INSTALL_DIR=$HOME/.local
export PATH="$MY_INSTALL_DIR/bin:$PATH"

mkdir -p project/build
cd project/build
rm -rf *
cmake ..
make
