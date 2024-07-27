cmake_minimum_required(VERSION 3.10)

# set the project name
project(ssi-img)

# add the executable
add_executable(bmp2img-cga bmp2img-cga.c)
add_executable(bmp2img-ega bmp2img-ega.c)
add_executable(img2bmp img2bmp.c)
