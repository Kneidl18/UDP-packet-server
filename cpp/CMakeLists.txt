cmake_minimum_required(VERSION 3.27)
project(PP)

set(CMAKE_CXX_STANDARD 17)

add_executable(PP main.cpp
        PacketStructure.h
        configuration.h
        SocketHelper.cpp
        SocketHelper.h
        md5.cpp
        md5.h
        md5_loc.h)
