FROM ubuntu:latest
COPY gRPCinstall.sh ./
RUN chmod +x gRPCinstall.sh
RUN ./gRPCinstall.sh
COPY rocksdb_install.sh ./
RUN chmod +x rocksdb_install.sh
RUN ./rocksdb_install.sh
COPY test_installations.sh ./
RUN chmod +x test_installations.sh
RUN ./test_installations.sh
RUN mkdir project
COPY . ./project
COPY build.sh ./
RUN chmod +x build.sh
RUN ./build.sh
# CMD ["/bin/bash", "-c", "export PATH=\"$HOME/.local/bin:$PATH\" && /bin/bash"]

# RUN ./project/initialize.sh 
# This file will initialize docker container where the code is already installed 