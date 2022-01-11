cmake_minimum_required(VERSION 3.10)

project(GloipServer VERSION 1.0)

add_library(gloipserver SHARED gloip_server_generated.cpp
                               gloip_server.cpp
                               TcpServer.cpp
                               TcpSocket.cpp
)

target_include_directories(gloipserver PUBLIC ../protocols/${GLOIP_PROTOCOL})

find_library(GLOIP_PROTOCOL_LIBRARY ${GLOIP_PROTOCOL} PATHS ../protocols/${GLOIP_PROTOCOL})
find_package(Threads REQUIRED)

target_link_libraries(gloipserver ${GLOIP_PROTOCOL_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS gloipserver DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/../out)