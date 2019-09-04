cmake_minimum_required(VERSION 3.10)
project(connect4)

MATH(EXPR stack_size "1200 * 1024 * 1024")
set(CMAKE_EXE_LINKER_FLAGS "-Wl,--stack,${stack_size}")

set(CMAKE_CXX_STANDARD 17)
#-fwhole-program -flto -Wall -Wextra
set(CMAKE_CXX_FLAGS "-fprofile-use -g -fno-strict-aliasing -Ofast -march=native -funroll-loops -mtune=native -fomit-frame-pointer --param max-inline-insns-single=10000 --param max-inline-insns-auto=10000")

add_executable(connect4 main.cpp AlphaBeta.h State.h Piece.h Util.h Time.h)
