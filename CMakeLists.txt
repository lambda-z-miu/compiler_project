cmake_minimum_required(VERSION 3.16)
project(compiler_project LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(USE_RDKIT "Use RDKit C++ for SVG rendering when available" ON)
option(REQUIRE_RDKIT "Fail configuration if USE_RDKIT is ON but RDKit is unavailable" OFF)

add_library(compiler_core STATIC
    Parser.cpp
    tokenizer.cpp
    Translation.cpp
    Keyword.cpp
    SvgRenderer.cpp
    ChemRenderer.cpp
)

add_executable(tokenizer tokenizer_main.cpp)
target_link_libraries(tokenizer PRIVATE compiler_core)

add_executable(parser parser.cpp)
target_link_libraries(parser PRIVATE compiler_core)

add_executable(chem_native_host native_host.cpp)
target_link_libraries(chem_native_host PRIVATE compiler_core)

add_executable(chem_http_server http_server.cpp)
target_link_libraries(chem_http_server PRIVATE compiler_core)

if(USE_RDKIT)
    if(NOT Eigen3_DIR AND EXISTS "/usr/share/eigen3/cmake/Eigen3Config.cmake")
        set(Eigen3_DIR "/usr/share/eigen3/cmake" CACHE PATH "Eigen3 CMake package directory")
    endif()
    find_package(Eigen3 CONFIG QUIET)

    set(COMPILER_PROJECT_RDKIT_DEPS_READY ${Eigen3_FOUND})
    if(NOT Eigen3_FOUND)
        if(REQUIRE_RDKIT)
            message(FATAL_ERROR "RDKit C++ was requested, but Eigen3 was not found. Install it with: sudo apt-get install -y libeigen3-dev")
        else()
            message(WARNING "Eigen3 was not found; RDKit C++ renderer will be skipped. Install libeigen3-dev to enable it.")
        endif()
    endif()
endif()

if(USE_RDKIT AND COMPILER_PROJECT_RDKIT_DEPS_READY)
    # The Ubuntu 24.04 RDKit package exports a CMake config that can reference
    # missing NumPy/Cairo targets. Manual discovery is more robust for this app.
    find_path(RDKIT_INCLUDE_DIR
        NAMES GraphMol/RDKitBase.h
        PATH_SUFFIXES rdkit
    )
    find_library(RDKIT_MOLDRAW2D_LIBRARY NAMES RDKitMolDraw2D)
    find_library(RDKIT_DEPICTOR_LIBRARY NAMES RDKitDepictor)
    find_library(RDKIT_GRAPHMOL_LIBRARY NAMES RDKitGraphMol)
    find_library(RDKIT_RDGENERAL_LIBRARY NAMES RDKitRDGeneral)
    find_library(RDKIT_DATASTRUCTS_LIBRARY NAMES RDKitDataStructs)
    find_library(CAIRO_LIBRARY NAMES cairo)
    find_path(CAIRO_INCLUDE_DIR
        NAMES cairo.h
        PATH_SUFFIXES cairo
    )

    if(RDKIT_INCLUDE_DIR
            AND RDKIT_MOLDRAW2D_LIBRARY
            AND RDKIT_DEPICTOR_LIBRARY
            AND RDKIT_GRAPHMOL_LIBRARY
            AND RDKIT_RDGENERAL_LIBRARY
            AND RDKIT_DATASTRUCTS_LIBRARY)
        target_compile_definitions(compiler_core PRIVATE COMPILER_PROJECT_HAVE_RDKIT=1)
        target_include_directories(compiler_core PRIVATE
            ${RDKIT_INCLUDE_DIR}
            ${CAIRO_INCLUDE_DIR}
        )
        target_link_libraries(compiler_core PRIVATE
            ${RDKIT_MOLDRAW2D_LIBRARY}
            ${RDKIT_DEPICTOR_LIBRARY}
            ${RDKIT_GRAPHMOL_LIBRARY}
            ${RDKIT_RDGENERAL_LIBRARY}
            ${RDKIT_DATASTRUCTS_LIBRARY}
            ${CAIRO_LIBRARY}
        )
        message(STATUS "RDKit C++ renderer enabled via manual library discovery")
    elseif(REQUIRE_RDKIT)
        message(FATAL_ERROR "RDKit C++ was requested but not found. Install librdkit-dev and libeigen3-dev, or set CMAKE_PREFIX_PATH/CMAKE_LIBRARY_PATH/CMAKE_INCLUDE_PATH to your RDKit installation.")
    else()
        message(WARNING "RDKit C++ was not found; parser will use the built-in SVG renderer fallback.")
    endif()
endif()
