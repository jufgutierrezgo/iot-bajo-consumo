cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(as7341_spectrometry)

target_sources(app PRIVATE 
src/main.c
src/as7341.c
)










