cmake_minimum_required(VERSION 3.10)
# set the project name
project(ssi-img)

include_directories(
    "."
    "include"
)

# add the executable
add_executable(bmp2img-cga executables/bmp2img-cga.c)
add_executable(bmp2img-ega executables/bmp2img-ega.c)
add_executable(img2bmp executables/img2bmp.c)
