#! /bin/bash
apt-get update
apt-get install -y libgtest-dev

# test and debugging tools
apt-get update && apt-get install -y libboost-all-dev net-tools gdb libspdlog-dev systemd-coredump
