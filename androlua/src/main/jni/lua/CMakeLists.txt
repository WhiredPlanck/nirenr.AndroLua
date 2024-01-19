cmake_minimum_required(VERSION 3.1)
project(lua LANGUAGES C VERSION 5.3.5)

option(LUA_SUPPORT_DL "Support dynamic loading of compiled modules" OFF)
option(LUA_BUILD_AS_CXX "Build lua as C++" OFF)
option(LUA_ENABLE_SHARED "Build dynamic liblua" ON)

if(${PROJECT_NAME} STREQUAL ${CMAKE_PROJECT_NAME})
    set(TOP_LEVEL TRUE)
else()
    set(TOP_LEVEL FALSE)
endif()

if(TOP_LEVEL)
    option(LUA_BUILD_BINARY "Build lua binary" ON)
    option(LUA_BUILD_COMPILER "Build luac compiler" ON)
else()
    option(LUA_BUILD_BINARY "Build lua binary" OFF)
    option(LUA_BUILD_COMPILER "Build luac compiler" ON)
endif()

add_subdirectory(src)

include(CMakePackageConfigHelpers)

get_target_property(LUA_EXPORT_LIBRARY Lua::Library ALIASED_TARGET)
write_basic_package_version_file(
    LuaConfigVersion.cmake
    VERSION ${PACKAGE_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(EXPORT LuaTargets
        FILE LuaTargets.cmake
        DESTINATION "share/cmake/Lua"
        NAMESPACE Lua::
)

configure_package_config_file(
    LuaConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/LuaConfig.cmake"
    INSTALL_DESTINATION "share/cmake/Lua"
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/LuaConfig.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/LuaConfigVersion.cmake"
        DESTINATION "share/cmake/Lua"
)
