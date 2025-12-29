cmake_minimum_required(VERSION 3.14)
project(datetime_utils_ext LANGUAGES CXX)

find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
find_package(pybind11 REQUIRED)

# Ensure position-independent code for static lib
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Create the Python extension module
pybind11_add_module(datetime_utils_ext MODULE datetime_utils_pybind.cpp)

# Link with your datetime_utils library
target_link_libraries(datetime_utils_ext PRIVATE datetime_utils)

# Include path for datetime_utils headers
target_include_directories(datetime_utils_ext PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
