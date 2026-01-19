<!-- Improved compatibility of back to top link: See: https://github.com/othneildrew/Best-README-Template/pull/73 -->
<a name="readme-top"></a>
<!--
*** Thanks for checking out the Best-README-Template. If you have a suggestion
*** that would make this better, please fork the repo and create a pull request
*** or simply open an issue with the tag "enhancement".
*** Don't forget to give the project a star!
*** Thanks again! Now go create something AMAZING! :D
-->



<!-- PROJECT SHIELDS -->
<!--
*** I'm using markdown "reference style" links for readability.
*** Reference links are enclosed in brackets [ ] instead of parentheses ( ).
*** See the bottom of this document for the declaration of the reference variables
*** for contributors-url, forks-url, etc. This is an optional, concise syntax you may use.
*** https://www.markdownguide.org/basic-syntax/#reference-style-links
-->

[![Contributors][contributors-shield]][contributors-url]
[![Forks][forks-shield]][forks-url]
[![Stargazers][stars-shield]][stars-url]
[![Issues][issues-shield]][issues-url]

<!-- PROJECT LOGO -->
<br />
<div align="center">


<h1 align="center">Distributed Key Value Store</h1>
<h2 align="center">An attempt to create a high performance distributed key-value store</h2>
  <p align="center">
    <br />
    <br />
    <br />
 <!--   <a href="https://github.com/MaheshG11/E-commerce-Chat-agent">View Demo</a> -->
    
<!--     <a href="https://github.com/MaheshG11/E-commerce-Chat-agent/issues/new?labels=bug&template=bug-report---.md">Report Bug</a> -->
    
<!--     <a href="https://github.com/MaheshG11/E-commerce-Chat-agent/issues/new?labels=enhancement&template=feature-request---.md">Request Feature</a> -->
  </p>
</div>



<!-- TABLE OF CONTENTS -->
<details>
  <summary>Table of Contents</summary>
  <ol>
    <li>
      <a href="#about-the-project">About The Project</a>
      <ul>
        <li><a href="#built-with">Built With</a></li>
      </ul>
    </li>
    <li>
     <a href="#getting-started">Getting Started</a>
     <!--  <ul>
        <li><a href="#prerequisites">Prerequisites</a></li>
        <li><a href="#installation">Installation</a></li>
      </ul>-->
    </li>
 <!--    <li><a href="#usage">Usage</a></li>
    <li><a href="#roadmap">Roadmap</a></li>-->
<!--     <li><a href="#contributing">Contributing</a></li> -->
<!--     <li><a href="#license">License</a></li> -->
    <li><a href="#contact">Contact</a></li>
<!--     <li><a href="#acknowledgments">Acknowledgments</a></li> -->
  </ol>
</details>



<!-- ABOUT THE PROJECT -->
## About The Project

<!--[![Product Name Screen Shot][product-screenshot]](https://example.com)-->

A high performance distributed key-value store built with rocksdb as base key-value store with raft inspired consensus and leader election algorithm   
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
<!--
### Prerequisites

This is an example of how to list things you need to use the software and how to install them.
* npm
  ```sh
  npm install npm@latest -g
  ```-->

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



<!-- USAGE EXAMPLES -->
<!--
## Usage

Use this space to show useful examples of how a project can be used. Additional screenshots, code examples and demos work well in this space. You may also link to more resources.

_For more examples, please refer to the [Documentation](https://example.com)_

<p align="right">(<a href="#readme-top">back to top</a>)</p>

-->



<!-- CONTRIBUTING -->



<!-- LICENSE -->
<!-- CONTACT -->
## Contact

Mahesh Ghumare [LinkedIn](https://www.linkedin.com/in/mahesh-ghumare-37894a200/)


<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- ACKNOWLEDGMENTS -->

<!-- MARKDOWN LINKS & IMAGES -->
<!-- https://www.markdownguide.org/basic-syntax/#reference-style-links -->
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
[Next.js]: https://img.shields.io/badge/next.js-000000?style=for-the-badge&logo=nextdotjs&logoColor=white
[Next-url]: https://nextjs.org/
[Django]:https://img.shields.io/badge/Django-092E20?style=for-the-badge&logo=django&logoColor=green
[Django-url]:https://www.djangoproject.com/
[Docker]:https://img.shields.io/badge/docker-%230db7ed.svg?style=for-the-badge&logo=docker&logoColor=white
[Docker-url]:https://www.docker.com/
[FastAPI]:https://img.shields.io/badge/FastAPI-005571?style=for-the-badge&logo=fastapi
[FastAPI-url]:https://fastapi.tiangolo.com/
[MongoDB]:https://img.shields.io/badge/-MongoDB-13aa52?style=for-the-badge&logo=mongodb&logoColor=white
[MongoDB-url]:https://www.mongodb.com/
[Postgres]:https://img.shields.io/badge/postgresql-4169e1?style=for-the-badge&logo=postgresql&logoColor=white
[Postgres-url]:https://www.postgresql.org/
[React.js]: https://img.shields.io/badge/React-20232A?style=for-the-badge&logo=react&logoColor=61DAFB
[React-url]: https://reactjs.org/
[Vue.js]: https://img.shields.io/badge/Vue.js-35495E?style=for-the-badge&logo=vuedotjs&logoColor=4FC08D
[Vue-url]: https://vuejs.org/
[Angular.io]: https://img.shields.io/badge/Angular-DD0031?style=for-the-badge&logo=angular&logoColor=white
[Angular-url]: https://angular.io/
[Svelte.dev]: https://img.shields.io/badge/Svelte-4A4A55?style=for-the-badge&logo=svelte&logoColor=FF3E00
[Svelte-url]: https://svelte.dev/
[Laravel.com]: https://img.shields.io/badge/Laravel-FF2D20?style=for-the-badge&logo=laravel&logoColor=white
[Laravel-url]: https://laravel.com
[Bootstrap.com]: https://img.shields.io/badge/Bootstrap-563D7C?style=for-the-badge&logo=bootstrap&logoColor=white
[Bootstrap-url]: https://getbootstrap.com
[JQuery.com]: https://img.shields.io/badge/jQuery-0769AD?style=for-the-badge&logo=jquery&logoColor=white
[JQuery-url]: https://jquery.com 
[Hugging-face.com]:https://img.shields.io/badge/-RocksDB-FDEE21?style=for-the-badge&logo=RocksDB&logoColor=black
[Hugging-face-url]:https://rocksdb.org/
[C++-shield]:https://img.shields.io/badge/C++-00599C?style=flat-square&logo=C%2B%2B&logoColor=white
[C++-link]:https://isocpp.org/
[gRPC-shield]:https://img.shields.io/badge/gRPC-blue?logo=grpc
[gRPC-link]:https://grpc.io/
[GTest-shield]:https://img.shields.io/badge/GoogleTest-blue
[GTest-link]:https://google.github.io/googletest/
