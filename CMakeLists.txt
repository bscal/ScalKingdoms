cmake_minimum_required(VERSION 3.10)

project(Game VERSION 1.0.0)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)


set(VENDOR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Game/vendor CACHE FILEPATH "Path to vendor directory.")
set(RAYLIB_DIR ${VENDOR_DIR}/raylib)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Configuration" FORCE)

set(OUTPUT_DIR ${CMAKE_CONFIGURATION_TYPES}-windows-x86-64)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY bin/${OUTPUT_DIR}/${PROJECT_NAME})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY bin/${OUTPUT_DIR}/${PROJECT_NAME})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin/${OUTPUT_DIR}/${PROJECT_NAME})

# Set this to the minimal version you want to support
find_package(raylib QUIET) # Let CMake search for a raylib-config.cmake
add_subdirectory("Game/vendor/raylib")
#find_library(RAYLIB raylib REQUIRED PATHS ${RAYLIB_DIR}/lib)

file(GLOB_RECURSE SRC_FILES Game/src/*.cpp)
file(GLOB_RECURSE HEADER_FILES Game/src/*.h)
add_executable(Game ${SRC_FILES} ${HEADER_FILES})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRC_FILES} ${HEADER_FILES})

target_include_directories(Game PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src ${RAYLIB_DIR}/include)
target_link_libraries(Game PUBLIC raylib)
target_link_directories(${CMAKE_CURRENT_SOURCE_DIR}/bin/${OUTPUT_DIR}/${PROJECT_NAME})

if("${CMAKE_CONFIGURATION_TYPES}" STREQUAL "Debug")
    add_compile_definitions(SCAL_DEBUG)
elseif("${CMAKE_CONFIGURATION_TYPES}" STREQUAL "Release")
    add_compile_definitions(SCAL_RELEASE)
endif()

if (MSVC)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Game)
endif()

# For moving assets folder around
#if(MSVC)
#    set_target_properties(Game PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
#else()
#    # Set a symbolic link so exe can find the /assets folder
#    set(link_src "${CMAKE_SOURCE_DIR}/assets")
#    if(UNIX AND NOT APPLE) 
#        set(link_dst "${CMAKE_CURRENT_BINARY_DIR}/assets") 
#    else() 
#        set(link_dst "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/assets") 
#    endif() 
#    add_custom_command( 
#        TARGET Game
#        POST_BUILD COMMAND ${CMAKE_COMMAND} 
#            -E create_symlink ${link_src} ${link_dst} 
#        DEPENDS ${link_dst} 
#        COMMENT "symbolic link ${link_src} => ${link_dst}" 
#    )
#endif()

#add_custom_target(copy_assets COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/assets ${CMAKE_CURRENT_BINARY_DIR}/assets)
#add_dependencies(Game copy_assets)