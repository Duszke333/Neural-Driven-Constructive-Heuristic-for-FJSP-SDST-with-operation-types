cmake_minimum_required(VERSION 3.22)
project(nd-ch-fjsp)

set(CMAKE_CXX_STANDARD 20)

#------------------------------
# Default build type: Release
#------------------------------
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif()

#------------------------------
# External Dependencies Paths
#------------------------------
set(CMAES_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/../libcmaes/include CACHE PATH "Path to cmaes include directory")
set(CMAES_LIB_DIR ${CMAKE_SOURCE_DIR}/../libcmaes/src/.libs CACHE PATH "Path to cmaes library directory")
set(EIGEN3_INCLUDE_DIR /usr/include/eigen3 CACHE PATH "Path to Eigen3 include directory")

find_package(Threads REQUIRED)
find_package(OpenMP REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")

#------------------------------
# Define Sources and Headers
#------------------------------
# Assuming you move files as requested:
# src/      <- *.cpp (including main.cpp)
# include/  <- *.h

add_executable(nd-ch-fjsp
        # Headers (in include/)
        include/BlackBoxEvaluatorConcept.h
        include/chof.h
        include/ClusteringConstructionHeuristic.h
        include/ConstructionHeuristicConcept.h
        include/DataConcept.h
        include/DataExport.h
        include/DataSetEvaluator.h
        include/Enums.h
        include/Err.h
        include/FFN.h
        include/Generator.h
        include/JobshopData.h
        include/JobshopDrawer.h
        include/learn.h
        include/ParallelDataSetEvaluator.h
        include/ParallelEvaluator.h
        include/ReadConfig.h
        include/Testing.h
        include/utils.h
        include/io.h
        include/options.h
        # SDST header files
        include/SetupTimeGenerator.h
        include/RandomSetupGenerator.h
        
        # Sources (in src/)
        src/DataExport.cpp
        src/FFN.cpp
        src/Generator.cpp
        src/JobshopDrawer.cpp
        src/main.cpp
        src/utils.cpp
        src/io.cpp
        src/options.cpp
        # SDST source files
        src/SetupTimeFactory.cpp
        src/RandomSetupGenerator.cpp
)

#------------------------------
# Include Directories
#------------------------------
target_include_directories(nd-ch-fjsp PRIVATE
        "${CMAKE_SOURCE_DIR}/include"          # <--- CRITICAL: Allows #include "header.h" to work
        "${EIGEN3_INCLUDE_DIR}"
        "${CMAES_INCLUDE_DIR}"
        "${CMAES_INCLUDE_DIR}/libcmaes"
        "/usr/include/eigen3"
)

#------------------------------
# Link Libraries & Directories
#------------------------------
target_link_directories(nd-ch-fjsp PRIVATE ${CMAES_LIB_DIR})

target_link_libraries(nd-ch-fjsp PRIVATE
        sfml-graphics
        sfml-window
        sfml-system
        boost_serialization
        boost_timer
        boost_program_options
        libcmaes.a
        Threads::Threads
)

#------------------------------
# Compiler Flags
#------------------------------
target_compile_options(nd-ch-fjsp PRIVATE
        $<$<CONFIG:Debug>:-O0 -g -march=native>
        $<$<CONFIG:Release>:-O3 -DNDEBUG -march=native>
)

#------------------------------
# Output Configuration
#------------------------------
# Forces the binary to be created in the root project directory
set_target_properties(nd-ch-fjsp PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

# Double-check copy command (useful for some IDEs that ignore RUNTIME_OUTPUT_DIRECTORY)
add_custom_command(TARGET nd-ch-fjsp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:nd-ch-fjsp>
        ${CMAKE_SOURCE_DIR}/$<TARGET_FILE_NAME:nd-ch-fjsp>
        COMMENT "Ensuring executable is in project root..."
)

