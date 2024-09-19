cmake_minimum_required(VERSION 3.15)

# set the project name
project(ssiimg)

# set the output directories
set (ProdDir "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/${CMAKE_SYSTEM_NAME}/${CMAKE_SYSTEM_PROCESSOR}")
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${ProdDir}/lib/static")
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY "${ProdDir}/lib/shared")
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${ProdDir}/bin")

include_directories("include")

set (sources 
    "src/ssi-img.c"
    "src/planar.c"
    "src/interlaced.c"
)

# add our project library
add_library (${PROJECT_NAME} ${sources})

