cmake_minimum_required(VERSION 4.0.0)

set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")

project(simulation_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_MODULE_STD ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find Dependencies
find_package(Arrow REQUIRED)
find_package(Parquet REQUIRED)

# Define the library
add_library(simulation_engine)

set_target_properties(simulation_engine PROPERTIES 
    POSITION_INDEPENDENT_CODE ON
    CXX_SCAN_FOR_MODULES ON
)

# Default to Release build for performance
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Define the sources
target_sources(simulation_engine
  PUBLIC
    FILE_SET CXX_MODULES FILES
        # Simulation engine
        include/simulation_engine/simulation_engine.cppm
        include/simulation_engine/template_instantiations.cppm
        include/simulation_engine/probability_distributions.cppm
        include/simulation_engine/types.cppm
        include/simulation_engine/quote.cppm
        include/simulation_engine/market_state.cppm
        include/simulation_engine/market_data.cppm
        include/simulation_engine/engine.cppm
        include/simulation_engine/portfolio.cppm
        include/simulation_engine/statistics.cppm
        include/simulation_engine/strategy_interface.cppm
        include/simulation_engine/order_placement.cppm
        include/simulation_engine/run_params.cppm

        # lib packages
        lib/datetime/include/datetime/datetime.cppm
        lib/datetime/include/datetime/datetime_date.cppm
        lib/datetime/include/datetime/datetime_datetime.cppm
        lib/datetime/include/datetime/datetime_duration.cppm
        lib/datetime/include/datetime/datetime_time.cppm
        lib/datetime/include/datetime/datetime_types.cppm
    PRIVATE
        # Simulation engine
        src/engine.cpp
        src/statistics.cpp
        src/portfolio.cpp
        src/market_data.cpp

        # lib packages
        lib/datetime/src/date.cpp
        lib/datetime/src/datetime.cpp
        lib/datetime/src/duration.cpp
        lib/datetime/src/time.cpp
)

# Set up include directories
target_include_directories(simulation_engine PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/datetime/include>
    $<INSTALL_INTERFACE:include>
)

# Link dependencies to simulation_engine
target_link_libraries(simulation_engine PRIVATE
    Arrow::arrow_shared
    Parquet::parquet_shared
)

# --- Example Strategy Executables ---
set(EXAMPLES market_order_example limit_order_example)

foreach(EXAMPLE ${EXAMPLES})
    add_executable(${EXAMPLE} examples/${EXAMPLE}.cpp)
    target_link_libraries(${EXAMPLE} PRIVATE simulation_engine)
endforeach()

# To build run: 
# cmake -B build -G Ninja
# cmake --build build
