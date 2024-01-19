add_library(luajava SHARED luajava.c)
target_link_libraries(luajava android lua_static)
