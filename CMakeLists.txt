cmake_minimum_required(VERSION 3.16)
project(compiler_project LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(tokenizer
    tokenizer_main.cpp
    tokenizer.cpp
)

add_executable(parser
    parser.cpp
    tokenizer.cpp
)
