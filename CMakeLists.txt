cmake_minimum_required(VERSION 2.8)

project(gserver) 

if (CMAKE_BUILD_TYPE STREQUAL Debug)
        add_definitions(
         -D_DEBUG
        )
endif()


set (INC_DIR ${gserver_SOURCE_DIR}/engine/)
set (LINK_DIR ${gserver_SOURCE_DIR}/engine/)


include_directories(
        ${INC_DIR}
)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
set (CMAKE_POSITION_INDEPENDENT_CODE ON)

add_subdirectory(engine)
add_subdirectory(gtw)
add_subdirectory(test)

