cmake_minimum_required(VERSION 3.10)
project(Lift LANGUAGES CXX C)
set(CMAKE_CXX_STANDARD 20)
#
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
include(cmake/configure_build_type.cmake)
include(cmake/configure_optix.cmake)

mark_as_advanced(CUDA_SDK_ROOT_DIR)
set(optix_LIBRARY "")

add_compile_definitions(LF_DEBUG)

set(CMAKE_DEBUG_POSTFIX d)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

foreach (OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${PROJECT_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${PROJECT_BINARY_DIR}/bin)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${PROJECT_BINARY_DIR}/lib)
endforeach ()

if (WIN32)
    add_definitions(-DVK_USE_PLATFORM_WIN32_KHR)
    add_definitions(-DUNICODE -D_UNICODE)
    add_definitions(-DWIN32_LEAN_AND_MEAN)

    foreach (flag_var CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
        if (${flag_var} MATCHES "/MD")
            string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
        endif ()
    endforeach ()

    #    add_compile_options("/std:c++17")
    #    add_compile_options("/MP")

endif ()

if (UNIX)

    # GCC 8 or higher is needed for <filesystem>
    set(CMAKE_CXX_COMPILER "/usr/bin/g++-8")

    add_definitions(-DUNIX)
    add_compile_options("-std=c++1z")
    add_compile_options("-Wall")
    add_compile_options("-fvisibility=hidden")

endif ()
if (MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif ()

find_package(Boost REQUIRED COMPONENTS program_options)
find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)
find_package(OptiX REQUIRED)

IF (NOT Vulkan_FOUND)
    message(FATAL_ERROR "Could not find Vulkan library!")
ELSE ()
    message(STATUS "Vulkan library: " ${Vulkan_LIBRARY})
ENDIF ()

find_program(Vulkan_GLSLANG_VALIDATOR
        NAMES glslangValidator
        HINTS ENV VULKAN_SDK
        PATH_SUFFIXES bin)

if (NOT Vulkan_GLSLANG_VALIDATOR)
    message(FATAL_ERROR "glslangValidator not found!")
endif ()


set(MAIN_PROJECT "Lift")
add_subdirectory(resources)
add_subdirectory(src)
add_subdirectory(external/spdlog)
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${MAIN_PROJECT})


#add_compile_definitions(_CRT_SECURE_NO_WARNINGS)


