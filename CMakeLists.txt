cmake_minimum_required(VERSION 3.3)

project("FastMap")

FILE( GLOB_RECURSE sources *.c* )
FILE( GLOB_RECURSE headers *.h* )

add_executable( FastMap ${sources} ${headers} )
