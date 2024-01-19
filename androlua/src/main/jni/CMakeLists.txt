cmake_minimum_required(VERSION 3.22.1)
project(luajava LANGUAGES C)

option(LUA_ENABLE_SHARED "Build dynamic liblua" OFF)
option(LUA_BUILD_BINARY "Build lua binary" OFF)
option(LUA_BUILD_COMPILER "Build luac compiler" OFF)
add_subdirectory(lua)

add_subdirectory(luajava)
