cmake_minimum_required(VERSION 3.15)
project(TestNetwordC)

set(CMAKE_CXX_STANDARD 14)

add_executable(TestNetwordC main.cpp)
add_executable(MyServer MyServer.cpp)
add_executable(MyClient MyClient.cpp)
add_executable(Client socket/selectversion/Client.cpp)
add_executable(Server socket/selectversion/Server.cpp)
