cmake_minimum_required(VERSION 3.15)
# set the project name and version
project(ssi-img VERSION 0.1.0)

# set up our binary dir to be a little more friendly
set (ProdDir "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/${CMAKE_SYSTEM_NAME}/${CMAKE_SYSTEM_PROCESSOR}")
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${ProdDir}/lib/static")
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY "${ProdDir}/lib/shared")
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${ProdDir}/bin")

# create a bin directory within the root of the project for convenience
file(MAKE_DIRECTORY "bin")

# generate a header file with the project version info
configure_file("${PROJECT_NAME}-version.in" "${PROJECT_NAME}-version.h")

# set-up the include paths
include_directories(
    "."
    "include"
    "${PROJECT_NAME}/include"
    "${CMAKE_BINARY_DIR}"
)

# include in our core code
if(NOT TARGET ${PROJECT_NAME})
    add_subdirectory(${PROJECT_NAME})
endif()

# additional code common for all executables
set (common_sources
    "tools/util.c"
)

# sources for our local BMP library
set (bmp_sources
    "tools/pal-tools.c"
    "quickbmp/bmp.c"
)

# build our BMP library
add_library(quickbmp ${bmp_sources})

# all our program executables
set (executables
    img2bmp
    bmp2img-ega
    bmp2img-cga
    bmp2bin
)

#build all our program executables
foreach(executable IN LISTS executables)
    add_executable(${executable} "src/${executable}.c" ${common_sources})
    target_link_libraries(${executable} "ssiimg" quickbmp)
    # for convenience copy the executables into the projects bin directory
    add_custom_command(TARGET ${executable} POST_BUILD
        COMMAND cp "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${executable}" "${CMAKE_SOURCE_DIR}/bin/${executable}"
    )
    install(TARGETS ${executable} DESTINATION ".")
endforeach(executable IN LISTS executables)

# make a more friendly package name
if("Darwin" STREQUAL ${CMAKE_HOST_SYSTEM_NAME})
    set (PACKAGE_HOST "Mac")
elseif("Windows" STREQUAL ${CMAKE_HOST_SYSTEM_NAME})
    set (PACKAGE_HOST "Win")
else()
    set (PACKAGE_HOST "${CMAKE_SYSTEM_NAME}")
endif()
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${PACKAGE_HOST}-${CMAKE_SYSTEM_PROCESSOR}")

# create a versioned zip package, and place it in the projects bin/package directory
set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/package")
include(CPack)
