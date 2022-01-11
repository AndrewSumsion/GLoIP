cmake_minimum_required(VERSION 3.10)

project(GloipClient VERSION 1.0)

add_library(gloipclient SHARED gloip_client_generated.cpp
                               gloip_client.cpp
                               IOHandler.cpp
                               TcpIOHandler.cpp
)

target_include_directories(gloipclient PUBLIC ../protocols/${GLOIP_PROTOCOL})