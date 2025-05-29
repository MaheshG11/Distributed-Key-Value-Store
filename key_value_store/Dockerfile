FROM ubuntu:latest
COPY gRPCinstall.sh ./
RUN ./gRPCinstall.sh
COPY rocksdb_install.sh ./
RUN ./rocksdb_install.sh
COPY test_installations.sh ./
RUN ./test_installations.sh
RUN mkdir project
COPY . ./project
CMD ["/bin/bash", "-c", "export PATH=\"$HOME/.local/bin:$PATH\" && /bin/bash"]

# RUN ./project/initialize.sh 
# This file will initialize docker container where the code is already installed 