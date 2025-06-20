cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(as7341_raw_reading)


# Add source files
target_sources(app PRIVATE 
    src/main.c
    src/as7341.c   
)






