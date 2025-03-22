cmake_minimum_required(VERSION 3.10)
project(SteganoTUI)

# Set C++ version
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Enable compile_commands.json generation
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Include header files from "include" directory
include_directories(include)

# Define the source files
set(SOURCES
    src/main.cpp
    src/stegano.cpp
    src/vigenere.cpp
)

set (ANOTHER
src/video.cpp)

# Find ncurses library
find_package(Curses REQUIRED)
find_package(OpenCV REQUIRED)
find_package(Threads REQUIRED)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OpenCV_INCLUDE_DIRS}
    ${CURSES_INCLUDE_DIR}
)

# Define the executable
add_executable(SteganoImgLsb ${SOURCES})

add_executable(SteganoVid ${ANOTHER})

# Link libraries
target_link_libraries(SteganoImgLsb
    ${OpenCV_LIBS}
    ${CURSES_LIBRARIES}
    Threads::Threads
)


target_link_libraries(SteganoVid
    ${OpenCV_LIBS}
    ${CURSES_LIBRARIES}
    Threads::Threads
)
