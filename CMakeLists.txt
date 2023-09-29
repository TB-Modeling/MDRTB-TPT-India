#/* CMakeLists.txt
#   This is an alternative way to compile the project in addition to XCode or
#   the makefile. It is also useful for the CLion IDE and for Conan.
#
#   TODO list
#    - Go through src/makefile and make sure everything is here.
#    - WARN_DEFAULTS are currently unused
#    - Tests: Building doesn't work. Problem is with target_include_directories
#      , one or more line of which is wrong. Also project setup might need to
#      change. See: https://arne-mertz.de/2018/06/cmake-project-structure/
#*/

# Setup
# ...The minimum version of CMake that we want collaborators to be using
cmake_minimum_required(VERSION 3.15)

# Project name
project(MDRTB CXX)

# Defining project tree folders
add_subdirectory(src)
add_subdirectory(test)

# Variables
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_FLAGS "-std=c++11 -ftree-vectorize -msse2 -O3")

add_definitions(
    -Wfatal-errors
)

#set(WARN_DEFAULTS "-Wall -Wextra -Wpedantic")
#add_compile_options(-Wall -Wextra -pedantic -Werror)

#set(SRC_DIR src/)
#set(TEST_DIR test/)

#list(APPEND includes
#    "/usr/local/include"
#    "/usr/include")
#list(APPEND relativeIncludes
#    "env/include"
#    "env/catch/include")

## Build: Tests
#file(GLOB TEST_SOURCES "${TEST_DIR}*.cpp")
#add_executable(test ${TEST_SOURCES} ${SRC_DIR}Run.cpp)
#target_include_directories(test
##    PUBLIC "$<BUILD_INTERFACE:${includes}>"
#    PRIVATE "$<INSTALL_INTERFACE:${relativeIncludes}>"
##    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
#)

## Build: Main
## -Source files to compile from
#file(GLOB SOURCES "${SRC_DIR}*.cpp")
## -Binaries to compile to
#add_executable(mdrtb ${SOURCES})
## -Includes
#target_include_directories(mdrtb PUBLIC
#    "$<BUILD_INTERFACE:${includes}>"
#    "$<INSTALL_INTERFACE:${relativeIncludes}>")
## -Links
## ...FIND_PACKAGE uses: https://cmake.org/cmake/help/v3.2/module/FindGSL.html
#FIND_PACKAGE(GSL 2.6 COMPONENTS program_options REQUIRED)
#INCLUDE_DIRECTORIES(${GSL_INCLUDE_DIRS})
#TARGET_LINK_LIBRARIES(mdrtb LINK_PUBLIC
#    m  # Math library
#    GSL::gsl  # GNU Scientific Library
#    GSL::gslcblas)  # GSL C Basic Linear Algebra Subprograms

# Report
message(\nBuild info:\n
    - CMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}\n
    - CMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}\n
    - CMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}\n
    - SRC_DIR=${SRC_DIR}\n
    - TEST_DIR=${TEST_DIR}\n
    - includes=${includes}\n
    - relativeIncludes=${relativeIncludes}\n
    - GSL_INCLUDE_DIRS=${GSL_INCLUDE_DIRS}\n
    - GSL_FOUND=${GSL_FOUND}\n
    - GSL_LIBRARIES=${GSL_LIBRARIES}\n
    - GSL_VERSION=${GSL_VERSION}\n)
