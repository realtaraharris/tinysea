cmake_minimum_required(VERSION 3.20)
project(identifier_shortener)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)

include_directories(SYSTEM ${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS} ${CLANG_LIBRARY_DIRS})

# Precompiled header setup for GCC
set(PCH_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/stdafx.h")
set(PCH_FILE "${CMAKE_CURRENT_BINARY_DIR}/stdafx.h.gch")

add_custom_command(
    OUTPUT ${PCH_FILE}
    COMMAND ${CMAKE_CXX_COMPILER}
            -x c++-header
            ${CMAKE_CXX_FLAGS}
            -I${CMAKE_CURRENT_SOURCE_DIR}
            -I${LLVM_INCLUDE_DIRS}
            -I${CLANG_INCLUDE_DIRS}
            -o ${PCH_FILE}
            ${PCH_HEADER}
    DEPENDS ${PCH_HEADER}
    COMMENT "Generating precompiled header"
)

add_executable(tinysea
    main.cpp
)

target_compile_options(tinysea PRIVATE
    -Winvalid-pch
    -include ${PCH_HEADER}
)

add_custom_target(tinysea_pch DEPENDS ${PCH_FILE})
add_dependencies(tinysea tinysea_pch)

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
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/*.h
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