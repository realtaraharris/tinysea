cmake_minimum_required(VERSION 3.20)
project(identifier_shortener)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)

include_directories(SYSTEM ${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS} "${CMAKE_CURRENT_SOURCE_DIR}/include")
link_directories(${LLVM_LIBRARY_DIRS} ${CLANG_LIBRARY_DIRS})

add_executable(tinysea
    src/main.cpp
    src/renamer.cpp
    src/ASTVisitor.cpp
    src/PPCallbacks.cpp
)

target_precompile_headers(tinysea PRIVATE include/stdafx.h)

target_include_directories(tinysea SYSTEM PRIVATE
    ${LLVM_INCLUDE_DIRS}
    ${CLANG_INCLUDE_DIRS}
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_directories(tinysea PRIVATE
    ${LLVM_LIBRARY_DIRS}
    ${CLANG_LIBRARY_DIRS}
)

target_link_libraries(tinysea
    PRIVATE
    clangTooling
    clangRewrite
    clangBasic
)

find_program(CLANG_FORMAT NAMES clang-format)

if(CLANG_FORMAT)
    # TODO: get this from the files passed to add_executable instead of using file(GLOB_RECURSE ...)
    file(GLOB_RECURSE ALL_SOURCE_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
    )
    list(FILTER ALL_SOURCE_FILES EXCLUDE REGEX "CMakeCXXCompilerId.cpp")

    list(LENGTH ALL_SOURCE_FILES NUM_SOURCE_FILES)
    if(NUM_SOURCE_FILES GREATER 0)
        add_custom_target("pretty"
            COMMAND ${CLANG_FORMAT} -i --style=file ${ALL_SOURCE_FILES}
            COMMENT "Formatting ${NUM_SOURCE_FILES} source files..."
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
        message(STATUS "Formatting target will process ${NUM_SOURCE_FILES} files")
    else()
        message(WARNING "No source files found for formatting")
    endif()
else()
    message(STATUS "clang-format not found - formatting disabled")
endif()