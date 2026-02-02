
<a name="readme-top"></a>





<!-- [![Contributors][contributors-shield]][contributors-url] -->
<!-- [![Forks][forks-shield]][forks-url] -->
<!-- [![Stargazers][stars-shield]][stars-url] -->
<!-- [![Issues][issues-shield]][issues-url] -->

<!-- PROJECT LOGO -->
<br />
<div align="center">


<h1 align="center">Distributed Key Value Store</h1>
<h2 align="center">An attempt to create a highly available distributed key-value store</h2>
  <p align="center">
  </p>
</div>



<!-- TABLE OF CONTENTS -->
<details>
  <summary>Table of Contents</summary>
  <ol>
    <li>
      <a href="#about-the-project">About The Project</a>
    </li>
    <li>
     <a href="#getting-started">Getting Started</a>
    <li><a href="#contact">Contact</a></li>
  </ol>
</details>



<!-- ABOUT THE PROJECT -->
## About The Project


A high availability distributed key-value store built with rocksdb as base key-value store with raft inspired consensus and leader election algorithm   
<p align="right">(<a href="#readme-top">back to top</a>)</p>





### 🚀 Features

### Core Raft Functionality
- [x] **Leader Election**
- [x] **Heartbeat Mechanism (AppendEntries with empty logs)**
- [x] **Custom Raft Consensus Implementation**
- [x] **Failure Detection & Handling**
- [x] **Log Replication**

### Storage & Recovery
- [x] **Persistent Log Storage**
- [x] **Crash Recovery via Log Replay**
- [ ] **Snapshotting / Log Compaction**
- [ ] **Fast Startup (Snapshot-based Recovery)**

### System Properties
- [x] **Replicated In-Memory Key-Value Store**
- [x] **gRPC-based RPC Layer**
- [x] **Thread-safe Log Management**
<p align="right">(<a href="#readme-top">back to top</a>)</p>


### Built With
* [![gRPC-shield][gRPC-shield]][gRPC-link]
* [![C++-shield][C++-shield]][C++-link]
* [![GTest-shield][GTest-shield]][GTest-link]
* [![Docker][Docker]][Docker-url]
* [![Hugging-face.com][Hugging-face.com]][Hugging-face-url]


<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- GETTING STARTED -->
## Getting Started

Follow the below installations to setup.

### Installation

1. Build the docker file 
   ```sh
   cd Distributed_Key_Value_Store
   docker image build -t distributed_key_value_store .
   ```
3. Start the docker container in exec mode
    ```sh
    docker container run -it distributed_key_value_store bash
    ```
4. Change directory to project directory in the container 
    ```sh
    cd project
    ```
5. Create a build directory and build the project
    ```sh
    mkdir -p build && cd build
    cmake .. -DCMAKE_BUILD_TYPE=Debug
    cmake --build .
    ```
6) Try to start the application with
   ```sh
   ./distributed_kv_store 300 600 50 3 null /tmp
   ./distributed_kv_store 300 600 50 3 172.17.0.2:5556 /tmp
   ```
   This will prompt you with how to start and run the application 

NOTE: All the steps above are assuming that the there is only one terminal session and on a unix platform with docker installed. 
<p align="right">(<a href="#readme-top">back to top</a>)</p>



## Contact

Mahesh Ghumare [LinkedIn](https://www.linkedin.com/in/mahesh-ghumare-37894a200/)


<p align="right">(<a href="#readme-top">back to top</a>)</p>


[contributors-shield]: https://img.shields.io/github/contributors/MaheshG11/E-commerce-Chat-agent.svg?style=for-the-badge
[contributors-url]: https://github.com/MaheshG11/E-commerce-Chat-agent/graphs/contributors
[forks-shield]: https://img.shields.io/github/forks/MaheshG11/E-commerce-Chat-agent.svg?style=for-the-badge
[forks-url]: https://github.com/MaheshG11/E-commerce-Chat-agent/network/members
[stars-shield]: https://img.shields.io/github/stars/MaheshG11/E-commerce-Chat-agent.svg?style=for-the-badge
[stars-url]: https://github.com/MaheshG11/E-commerce-Chat-agent/stargazers
[issues-shield]: https://img.shields.io/github/issues/MaheshG11/E-commerce-Chat-agent.svg?style=for-the-badge
[issues-url]: https://github.com/MaheshG11/E-commerce-Chat-agent/issues
[linkedin-shield]: https://img.shields.io/badge/-LinkedIn-black.svg?style=for-the-badge&logo=linkedin&colorB=555
[linkedin-url]: https://linkedin.com/in/mahesh-ghumare-37894a200
[product-screenshot]: images/screenshot.png


[Docker]:https://img.shields.io/badge/docker-%230db7ed.svg?style=for-the-badge&logo=docker&logoColor=white
[Docker-url]:https://www.docker.com/


[Hugging-face.com]:https://img.shields.io/badge/-RocksDB-FDEE21?style=for-the-badge&logo=RocksDB&logoColor=black
[Hugging-face-url]:https://rocksdb.org/
[C++-shield]:https://img.shields.io/badge/C++-00599C?style=flat-square&logo=C%2B%2B&logoColor=white
[C++-link]:https://isocpp.org/
[gRPC-shield]:https://img.shields.io/badge/gRPC-blue?logo=grpc
[gRPC-link]:https://grpc.io/
[GTest-shield]:https://img.shields.io/badge/GoogleTest-blue
[GTest-link]:https://google.github.io/googletest/
