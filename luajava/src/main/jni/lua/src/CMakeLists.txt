set(LUA_LIB_SRCS 
    "lapi.c"
    "lauxlib.c"
    "lbaselib.c"
    "lbitlib.c"
    "lcode.c"
    "lcorolib.c"
    "lctype.c"
    "ldblib.c"
    "ldebug.c"
    "ldo.c"
    "ldump.c"
    "lfunc.c"
    "lgc.c"
    "linit.c"
    "liolib.c"
    "llex.c"
    "lmathlib.c"
    "lmem.c"
    "loadlib.c"
    "lobject.c"
    "lopcodes.c"
    "loslib.c"
    "lparser.c"
    "lstate.c"
    "lstring.c"
    "lstrlib.c"
    "ltable.c"
    "ltablib.c"
    "ltm.c"
    "lundump.c"
    "lutf8lib.c"
    "lvm.c"
    "lzio.c"
)

set(TARGETS_TO_INSTALL lua_internal lua_include)

set(LUA_LINKED_LIBRARIES)

if(LUA_BUILD_AS_CXX)
	set_source_files_properties(${LUA_LIB_SRCS} "lua.c" "luac.c" PROPERTIES LANGUAGE CXX )
endif()

add_library(lua_internal INTERFACE)

add_library(lua_include INTERFACE)

target_include_directories(lua_include INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(lua_internal INTERFACE lua_include)

if(LUA_ENABLE_SHARED)
    add_library(lua_shared SHARED ${LUA_LIB_SRCS})
    target_link_libraries(lua_shared PRIVATE lua_internal PUBLIC lua_include)
    set_target_properties(lua_shared PROPERTIES
        VERSION "${PACKAGE_VERSION}"
    )
    if(WIN32)
        target_compile_definitions(lua_shared PRIVATE LUA_BUILD_AS_DLL)
    endif()
    list(APPEND TARGETS_TO_INSTALL lua_shared)
    if(BUILD_SHARED_LIBS)
        add_library(Lua::Library ALIAS lua_shared)
    elseif(NOT TOP_LEVEL)
        set_target_properties(lua_shared PROPERTIES
            EXCLUDE_FROM_ALL ON
        )
    endif()
endif()

add_library(lua_static STATIC ${LUA_LIB_SRCS})
target_link_libraries(lua_static PRIVATE lua_internal PUBLIC lua_include)
set_target_properties(lua_static PROPERTIES
    VERSION "${PACKAGE_VERSION}"
)
list(APPEND TARGETS_TO_INSTALL lua_static)
if(NOT BUILD_SHARED_LIBS OR NOT LUA_ENABLE_SHARED)
    add_library(Lua::Library ALIAS lua_static)
endif()


if(UNIX)
    if(NOT EMSCRIPTEN)
        find_library(LIBM m)
        #TODO: Redo this with find_package
        if(NOT LIBM)
            message(FATAL_ERROR "libm not found and is required by lua")
        endif()
        target_compile_definitions(lua_internal INTERFACE "LUA_USE_POSIX")
        target_link_libraries(lua_internal INTERFACE m)
        list(APPEND LUA_LINKED_LIBRARIES m)
        if(LUA_SUPPORT_DL)
            find_library(LIBDL "${CMAKE_DL_LIBS}")
            if(NOT LIBDL)
                message(FATAL_ERROR "libdl not found and is required by lua")
            endif()
            target_compile_definitions(lua_internal INTERFACE "LUA_USE_DLOPEN")
            target_link_libraries(lua_internal INTERFACE "${CMAKE_DL_LIBS}")
            list(APPEND LUA_LINKED_LIBRARIES "${CMAKE_DL_LIBS}")
        endif()
    endif()

	target_compile_options(lua_internal
		INTERFACE "-Wall" "-Wextra"
	)
elseif(Win32)
    target_compile_options(lua_internal
		INTERFACE "/Wall"
	)
endif()

if(LUA_BUILD_BINARY)
    include(CheckIncludeFile)
    CHECK_INCLUDE_FILE("readline/readline.h" HAVE_READLINE_READLINE_H)

    add_executable(lua "lua.c")
    # Can not use lua_shared because some symbols are not exported
    target_link_libraries(lua PRIVATE lua_static)
    set_target_properties(lua PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}
    )
    if (HAVE_READLINE_READLINE_H)
        target_compile_definitions(lua PRIVATE "LUA_USE_READLINE")
        target_link_libraries(lua PUBLIC readline)
    endif()
    list(APPEND TARGETS_TO_INSTALL lua)
endif()

if(LUA_BUILD_COMPILER)
    add_executable(luac "luac.c")
    target_link_libraries(luac PRIVATE lua_static)
    set_target_properties(luac PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}
    )
    list(APPEND TARGETS_TO_INSTALL luac)
endif()

install(TARGETS ${TARGETS_TO_INSTALL}
        EXPORT LuaTargets
)

install(DIRECTORY include/ TYPE INCLUDE)
