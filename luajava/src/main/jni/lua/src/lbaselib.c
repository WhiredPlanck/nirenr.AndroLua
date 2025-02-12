/*
** $Id: lbaselib.c,v 1.313 2016/04/11 19:18:40 roberto Exp $
** Basic library
** See Copyright Notice in lua.h
*/

#define lbaselib_c
#define LUA_LIB

#include "lprefix.h"


#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lua.h"

#include "lauxlib.h"
#include "lualib.h"


//#ifdef __ANDROID__
//
//#include <android/log.h>
//
//#define LOG_TAG "lua"
//#define LOGD(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
//#endif

static int luaB_print(lua_State *L) {
    int n = lua_gettop(L);  /* number of arguments */
    int i;
    lua_getglobal(L, "tostring");
    for (i = 1; i <= n; i++) {
        const char *s;
        size_t l;
        lua_pushvalue(L, -1);  /* function to be called */
        lua_pushvalue(L, i);   /* value to print */
        lua_call(L, 1, 1);
        s = lua_tolstring(L, -1, &l);  /* get result */
        if (s == NULL)
            return luaL_error(L, "'tostring' must return a string to 'print'");
        if (i > 1) lua_writestring("\t", 1);
        lua_writestring(s, l);
//#ifdef __ANDROID__
//        LOGD("%s", s);
//#endif
        lua_pop(L, 1);  /* pop result */
    }
    lua_writeline();
    return 0;
}


#define SPACECHARS    " \f\n\r\t\v"

static const char *b_str2int(const char *s, int base, lua_Integer *pn) {
    lua_Unsigned n = 0;
    int neg = 0;
    s += strspn(s, SPACECHARS);  /* skip initial spaces */
    if (*s == '-') {
        s++;
        neg = 1;
    }  /* handle signal */
    else if (*s == '+') s++;
    if (!isalnum((unsigned char) *s))  /* no digit? */
        return NULL;
    do {
        int digit = (isdigit((unsigned char) *s)) ? *s - '0'
                                                  : (toupper((unsigned char) *s) - 'A') + 10;
        if (digit >= base) return NULL;  /* invalid numeral */
        n = n * base + digit;
        s++;
    } while (isalnum((unsigned char) *s));
    s += strspn(s, SPACECHARS);  /* skip trailing spaces */
    *pn = (lua_Integer) ((neg) ? (0u - n) : n);
    return s;
}


static int luaB_tonumber(lua_State *L) {
    if (lua_isnoneornil(L, 2)) {  /* standard conversion? */
        luaL_checkany(L, 1);
        if (lua_type(L, 1) == LUA_TNUMBER) {  /* already a number? */
            lua_settop(L, 1);  /* yes; return it */
            return 1;
        } else {
            size_t l;
            const char *s = lua_tolstring(L, 1, &l);
            if (s != NULL && lua_stringtonumber(L, s) == l + 1)
                return 1;  /* successful conversion to number */
            /* else not a number */
        }
    } else {
        size_t l;
        const char *s;
        lua_Integer n = 0;  /* to avoid warnings */
        lua_Integer base = luaL_checkinteger(L, 2);
        luaL_checktype(L, 1, LUA_TSTRING);  /* no numbers as strings */
        s = lua_tolstring(L, 1, &l);
        luaL_argcheck(L, 2 <= base && base <= 36, 2, "base out of range");
        if (b_str2int(s, (int) base, &n) == s + l) {
            lua_pushinteger(L, n);
            return 1;
        }  /* else not a number */
    }  /* else not a number */
    lua_pushnil(L);  /* not a number */
    return 1;
}


//mod by nirenr
static int luaB_tointeger(lua_State *L) {
    if (lua_type(L, 1) == LUA_TNUMBER) {
        if (lua_isinteger(L, 1)) {
            lua_settop(L, 1);
            return 1;
        } else {
            lua_Number n = lua_tonumber(L, 1);
            lua_pushinteger(L, (lua_Integer) n);
            return 1;
        }
    } else {
        size_t l;
        const char *s = luaL_tolstring(L, 1, &l);
        if (s != NULL && lua_stringtonumber(L, s) == l + 1) {
            lua_Number n = lua_tonumber(L, 1);
            lua_pushinteger(L, (lua_Integer) n);
            return 1;
        }
    }
    lua_pushnil(L);
    return 1;
}


static int luaB_error(lua_State *L) {
    int level = (int) luaL_optinteger(L, 2, 1);
    lua_settop(L, 1);
    if (lua_type(L, 1) == LUA_TSTRING && level > 0) {
        luaL_where(L, level);   /* add extra information */
        lua_pushvalue(L, 1);
        lua_concat(L, 2);
    }
    return lua_error(L);
}


static int luaB_getmetatable(lua_State *L) {
    luaL_checkany(L, 1);
    if (!lua_getmetatable(L, 1)) {
        lua_pushnil(L);
        return 1;  /* no metatable */
    }
    luaL_getmetafield(L, 1, "__metatable");
    return 1;  /* returns either __metatable field (if present) or metatable */
}


static int luaB_setmetatable(lua_State *L) {
    int t = lua_type(L, 2);
    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_argcheck(L, t == LUA_TNIL || t == LUA_TTABLE, 2,
                  "nil or table expected");
    if (luaL_getmetafield(L, 1, "__metatable") != LUA_TNIL)
        return luaL_error(L, "cannot change a protected metatable");
    lua_settop(L, 2);
    lua_setmetatable(L, 1);
    return 1;
}

//--mod by nirenr
static int luaB_setmetamethod(lua_State *L) {
    int t = lua_type(L, 3);
    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_argcheck(L, t == LUA_TNIL || t == LUA_TTABLE || t == LUA_TFUNCTION, 2,
                  "table function or nil expected");
    if (luaL_getmetafield(L, 1, "__metatable") != LUA_TNIL)
        return luaL_error(L, "cannot change a protected metatable");
    lua_settop(L, 3);
    lua_getmetatable(L, 1);
    if (lua_type(L, 4) != LUA_TTABLE) {
        lua_settop(L, 3);
        lua_newtable(L);
        lua_setmetatable(L, 1);
        lua_getmetatable(L, 1);
    }
    lua_insert(L, 2);
    lua_settable(L, 2);
    lua_settop(L, 1);
    return 1;
}
//---

static int luaB_rawequal(lua_State *L) {
    luaL_checkany(L, 1);
    luaL_checkany(L, 2);
    lua_pushboolean(L, lua_rawequal(L, 1, 2));
    return 1;
}


static int luaB_rawlen(lua_State *L) {
    int t = lua_type(L, 1);
    luaL_argcheck(L, t == LUA_TTABLE || t == LUA_TSTRING, 1,
                  "table or string expected");
    lua_pushinteger(L, lua_rawlen(L, 1));
    return 1;
}


static int luaB_rawget(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_checkany(L, 2);
    lua_settop(L, 2);
    lua_rawget(L, 1);
    return 1;
}

static int luaB_rawset(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_checkany(L, 2);
    luaL_checkany(L, 3);
    lua_settop(L, 3);
    lua_rawset(L, 1);
    return 1;
}


static int luaB_collectgarbage(lua_State *L) {
    static const char *const opts[] = {"stop", "restart", "collect",
                                       "count", "step", "setpause", "setstepmul",
                                       "isrunning", NULL};
    static const int optsnum[] = {LUA_GCSTOP, LUA_GCRESTART, LUA_GCCOLLECT,
                                  LUA_GCCOUNT, LUA_GCSTEP, LUA_GCSETPAUSE, LUA_GCSETSTEPMUL,
                                  LUA_GCISRUNNING};
    int o = optsnum[luaL_checkoption(L, 1, "collect", opts)];
    int ex = (int) luaL_optinteger(L, 2, 0);
    int res = lua_gc(L, o, ex);
    switch (o) {
        case LUA_GCCOUNT: {
            int b = lua_gc(L, LUA_GCCOUNTB, 0);
            lua_pushnumber(L, (lua_Number) res + ((lua_Number) b / 1024));
            return 1;
        }
        case LUA_GCSTEP:
        case LUA_GCISRUNNING: {
            lua_pushboolean(L, res);
            return 1;
        }
        default: {
            lua_pushinteger(L, res);
            return 1;
        }
    }
}


static int luaB_type(lua_State *L) {
    int t = lua_type(L, 1);
    luaL_argcheck(L, t != LUA_TNONE, 1, "value expected");
    if (luaL_callmeta(L, 1, "__type")) {
        //lua_pushstring(L, lua_typename(L, t));
        //lua_pushvalue(L,-2);
        return 1;
    }
    lua_pushstring(L, lua_typename(L, t));
    return 1;
}


static int pairsmeta(lua_State *L, const char *method, int iszero,
                     lua_CFunction iter) {
    if (luaL_getmetafield(L, 1, method) == LUA_TNIL) {  /* no metamethod? */
        luaL_checktype(L, 1, LUA_TTABLE);  /* argument must be a table */
        lua_pushcfunction(L, iter);  /* will return generator, */
        lua_pushvalue(L, 1);  /* state, */
        if (iszero) lua_pushinteger(L, 0);  /* and initial value */
        else lua_pushnil(L);
    } else {
        lua_pushvalue(L, 1);  /* argument 'self' to metamethod */
        lua_call(L, 1, 3);  /* get 3 values from metamethod */
    }
    return 3;
}


static int luaB_next(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_settop(L, 2);  /* create a 2nd argument if there isn't one */
    if (lua_next(L, 1))
        return 2;
    else {
        lua_pushnil(L);
        return 1;
    }
}


static int luaB_pairs(lua_State *L) {
    return pairsmeta(L, "__pairs", 0, luaB_next);
}


/*
** Traversal function for 'ipairs'
*/
static int ipairsaux(lua_State *L) {
    lua_Integer i = luaL_checkinteger(L, 2) + 1;
    lua_pushinteger(L, i);
    return (lua_geti(L, 1, i) == LUA_TNIL) ? 1 : 2;
}


/*
** 'ipairs' function. Returns 'ipairsaux', given "table", 0.
** (The given "table" may not be a table.)
*/
static int luaB_ipairs(lua_State *L) {
#if defined(LUA_COMPAT_IPAIRS)
    return pairsmeta(L, "__ipairs", 1, ipairsaux);
#else
    luaL_checkany(L, 1);
    lua_pushcfunction(L, ipairsaux);  /* iteration function */
    lua_pushvalue(L, 1);  /* state */
    lua_pushinteger(L, 0);  /* initial value */
    return 3;
#endif
}


static int load_aux(lua_State *L, int status, int envidx) {
    if (status == LUA_OK) {
        if (envidx != 0) {  /* 'env' parameter? */
            lua_pushvalue(L, envidx);  /* environment for loaded function */
            if (!lua_setupvalue(L, -2, 1))  /* set it as 1st upvalue */
                lua_pop(L, 1);  /* remove 'env' if not used by previous call */
        }
        return 1;
    } else {  /* error (message is on top of the stack) */
        lua_pushnil(L);
        lua_insert(L, -2);  /* put before error message */
        return 2;  /* return nil plus error message */
    }
}


static int luaB_loadfile(lua_State *L) {
    const char *fname = luaL_optstring(L, 1, NULL);
    const char *mode = luaL_optstring(L, 2, NULL);
    int env = (!lua_isnone(L, 3) ? 3 : 0);  /* 'env' index or 0 if no 'env' */
    int status = luaL_loadfilex(L, fname, mode);
    return load_aux(L, status, env);
}


/*
** {======================================================
** Generic Read function
** =======================================================
*/


/*
** reserved slot, above all arguments, to hold a copy of the returned
** string to avoid it being collected while parsed. 'load' has four
** optional arguments (chunk, source name, mode, and environment).
*/
#define RESERVEDSLOT    5


/*
** Reader for generic 'load' function: 'lua_load' uses the
** stack for internal stuff, so the reader cannot change the
** stack top. Instead, it keeps its resulting string in a
** reserved slot inside the stack.
*/
static const char *generic_reader(lua_State *L, void *ud, size_t *size) {
    (void) (ud);  /* not used */
    luaL_checkstack(L, 2, "too many nested functions");
    lua_pushvalue(L, 1);  /* get function */
    lua_call(L, 0, 1);  /* call it */
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);  /* pop result */
        *size = 0;
        return NULL;
    } else if (!lua_isstring(L, -1))
        luaL_error(L, "reader function must return a string");
    lua_replace(L, RESERVEDSLOT);  /* save string in reserved slot */
    return lua_tolstring(L, RESERVEDSLOT, size);
}


static int luaB_load(lua_State *L) {
    int status;
    size_t l;
    const char *s = lua_tolstring(L, 1, &l);
    const char *mode = luaL_optstring(L, 3, "bt");
    int env = (!lua_isnone(L, 4) ? 4 : 0);  /* 'env' index or 0 if no 'env' */
    if (s != NULL) {  /* loading a string? */
        const char *chunkname = luaL_optstring(L, 2, s);
        status = luaL_loadbufferx(L, s, l, chunkname, mode);
    } else {  /* loading from a reader function */
        const char *chunkname = luaL_optstring(L, 2, "=(load)");
        luaL_checktype(L, 1, LUA_TFUNCTION);
        lua_settop(L, RESERVEDSLOT);  /* create reserved slot */
        status = lua_load(L, generic_reader, NULL, chunkname, mode);
    }
    return load_aux(L, status, env);
}

static int luaB_loadstring(lua_State *L) {
    int status;
    size_t l;
    const char *s = luaL_checklstring(L, 1, &l);
    const char *mode = luaL_optstring(L, 3, "bt");
    int env = (!lua_isnone(L, 4) ? 4 : 0);  /* 'env' index or 0 if no 'env' */
    const char *chunkname = luaL_optstring(L, 2, s);
    status = luaL_loadbufferx(L, s, l, chunkname, mode);
    return load_aux(L, status, env);
}

/* }====================================================== */


static int dofilecont(lua_State *L, int d1, lua_KContext d2) {
    (void) d1;
    (void) d2;  /* only to match 'lua_Kfunction' prototype */
    return lua_gettop(L) - 1;
}


static int luaB_dofile(lua_State *L) {
    const char *fname = luaL_optstring(L, 1, NULL);
    lua_settop(L, 1);
    if (luaL_loadfile(L, fname) != LUA_OK)
        return lua_error(L);
    lua_callk(L, 0, LUA_MULTRET, 0, dofilecont);
    return dofilecont(L, 0, 0);
}


static int luaB_assert(lua_State *L) {
    if (lua_toboolean(L, 1))  /* condition is true? */
        return lua_gettop(L);  /* return all arguments */
    else {  /* error */
        luaL_checkany(L, 1);  /* there must be a condition */
        lua_remove(L, 1);  /* remove it */
        lua_pushliteral(L, "assertion failed!");  /* default message */
        lua_settop(L, 1);  /* leave only message (default if no other one) */
        return luaB_error(L);  /* call 'error' */
    }
}


static int luaB_select(lua_State *L) {
    int n = lua_gettop(L);
    if (lua_type(L, 1) == LUA_TSTRING && *lua_tostring(L, 1) == '#') {
        lua_pushinteger(L, n - 1);
        return 1;
    } else {
        lua_Integer i = luaL_checkinteger(L, 1);
        if (i < 0) i = n + i;
        else if (i > n) i = n;
        luaL_argcheck(L, 1 <= i, 1, "index out of range");
        return n - (int) i;
    }
}


/*
** Continuation function for 'pcall' and 'xpcall'. Both functions
** already pushed a 'true' before doing the call, so in case of success
** 'finishpcall' only has to return everything in the stack minus
** 'extra' values (where 'extra' is exactly the number of items to be
** ignored).
*/
static int finishpcall(lua_State *L, int status, lua_KContext extra) {
    if (status != LUA_OK && status != LUA_YIELD) {  /* error? */
        lua_pushboolean(L, 0);  /* first result (false) */
        lua_pushvalue(L, -2);  /* error message */
        return 2;  /* return false, msg */
    } else
        return lua_gettop(L) - (int) extra;  /* return all results */
}


static int luaB_pcall(lua_State *L) {
    int status;
    luaL_checkany(L, 1);
    lua_pushboolean(L, 1);  /* first result if no errors */
    lua_insert(L, 1);  /* put it in place */
    status = lua_pcallk(L, lua_gettop(L) - 2, LUA_MULTRET, 0, 0, finishpcall);
    return finishpcall(L, status, 0);
}


/*
** Do a protected call with error handling. After 'lua_rotate', the
** stack will have <f, err, true, f, [args...]>; so, the function passes
** 2 to 'finishpcall' to skip the 2 first values when returning results.
*/
static int luaB_xpcall(lua_State *L) {
    int status;
    int n = lua_gettop(L);
    luaL_checktype(L, 2, LUA_TFUNCTION);  /* check error function */
    lua_pushboolean(L, 1);  /* first result */
    lua_pushvalue(L, 1);  /* function */
    lua_rotate(L, 3, 2);  /* move them below function's arguments */
    status = lua_pcallk(L, n - 2, LUA_MULTRET, 2, 2, finishpcall);
    return finishpcall(L, status, 2);
}


static int luaB_tostring(lua_State *L) {
    luaL_checkany(L, 1);
    luaL_tolstring(L, 1, NULL);
    return 1;
}


/* compatibility with old module system */
#if defined(LUA_COMPAT_MODULE)

static int findtable(lua_State *L) {
    if (lua_gettop(L) == 1) {
        lua_pushglobaltable(L);
        lua_insert(L, 1);
    }
    luaL_checktype(L, 1, LUA_TTABLE);
    const char *name = luaL_checklstring(L, 2, 0);
    lua_pushstring(L, luaL_findtable(L, 1, name, 0));
    return 2;
}

#endif


#ifdef LUA_COMPAT_DUMP

static lua_CFunction lc_table_concat = NULL;
static lua_CFunction lc_table_insert = NULL;
static lua_CFunction lc_string_rep = NULL;
static lua_CFunction lc_string_format = NULL;

/* gets upvalue with ID varid by consulting upvalue table at index
 * tidx for the upvalue table at given nesting level. */
static void lc_getupvalue(lua_State *L, int tidx, int level, int varid) {
    if (level == 0) {
        lua_rawgeti(L, tidx, varid);
    } else {
        lua_pushvalue(L, tidx);
        while (--level >= 0) {
            lua_rawgeti(L, tidx, 0); /* 0 links to parent table */
            lua_remove(L, -2);
            tidx = -1;
        }
        lua_rawgeti(L, -1, varid);
        lua_remove(L, -2);
    }
}

static void lc_setupvalue(lua_State *L, int tidx, int level, int varid) {
    if (level == 0) {
        lua_rawseti(L, tidx, varid);
    } else {
        lua_pushvalue(L, tidx);
        while (--level >= 0) {
            lua_rawgeti(L, tidx, 0); /* 0 links to parent table */
            lua_remove(L, -2);
            tidx = -1;
        }
        lua_insert(L, -2);
        lua_rawseti(L, -2, varid);
        lua_pop(L, 1);
    }
}

static int luaB_dump_ToString(lua_State *L) {
    lua_settop(L, 2);
    int otype = lua_type(L, 1);
    switch (otype) {
        case LUA_TNUMBER: {
            lua_pushcfunction(L, lc_table_insert);
            lc_getupvalue(L, lua_upvalueindex(1), 3, 1);
            lua_pushvalue(L, 1);
            lua_call(L, 2, 0);
            break;
        }
        case LUA_TSTRING: {
            lua_pushcfunction(L, lc_table_insert);
            const int lc11 = lua_gettop(L);
            lc_getupvalue(L, lua_upvalueindex(1), 3, 1);
            lua_pushcfunction(L, lc_string_format);
            lua_pushliteral(L, "%q");
            lua_pushvalue(L, 1);
            lua_call(L, 2, LUA_MULTRET);
            lua_call(L, (lua_gettop(L) - lc11), 0);
            break;
        }
        case LUA_TTABLE: {
            lua_pushcfunction(L, luaB_getmetatable);
            lua_pushvalue(L, 1);
            lua_call(L, 1, 1);
            lua_pushvalue(L, 3);
            if (lua_toboolean(L, -1)) {
                lua_pop(L, 1);
                lua_pushliteral(L, "__tostring");
                lua_gettable(L, 3);
            }
            const int lc16 = lua_toboolean(L, -1);
            lua_pop(L, 1);
            if (lc16) {
                lua_pushcfunction(L, lc_table_insert);
                const int lc17 = lua_gettop(L);
                lc_getupvalue(L, lua_upvalueindex(1), 3, 1);
                lua_pushcfunction(L, luaB_tostring);
                lua_pushvalue(L, 1);
                lua_call(L, 1, LUA_MULTRET);
                lua_call(L, (lua_gettop(L) - lc17), 0);
            } else {
                lc_getupvalue(L, lua_upvalueindex(1), 1, 3);
                lua_pushinteger(L, 2);
                lua_arith(L, LUA_OPADD);
                lc_setupvalue(L, lua_upvalueindex(1), 1, 3);
                lua_pushcfunction(L, lc_table_insert);
                lc_getupvalue(L, lua_upvalueindex(1), 3, 1);
                lua_pushliteral(L, "{");
                lua_call(L, 2, 0);
                lua_pushcfunction(L, luaB_pairs);
                lua_pushvalue(L, 1);
                lua_call(L, 1, 3);
                while (1) {
                    lua_pushvalue(L, -3);
                    lua_pushvalue(L, -3);
                    lua_pushvalue(L, -3);
                    lua_call(L, 2, 2);
                    if (lua_isnil(L, -2)) {
                        break;
                    }
                    lua_pushvalue(L, -2);
                    lua_replace(L, -4);
                    lua_pushvalue(L, LUA_RIDX_GLOBALS);
                    const int lc20 = lua_equal(L, 8, -1);
                    lua_pop(L, 1);
                    lua_pushboolean(L, lc20);
                    const int lc21 = lua_toboolean(L, -1);
                    lua_pop(L, 1);
                    if (lc21) {
                        lua_pushcfunction(L, lc_table_insert);
                        const int lc22 = lua_gettop(L);
                        lc_getupvalue(L, lua_upvalueindex(1), 3, 1);
                        lua_pushcfunction(L, lc_string_format);
                        lua_pushliteral(L, "\r\n%s%s\t=%s ;");
                        lua_pushcfunction(L, lc_string_rep);
                        lc_getupvalue(L, lua_upvalueindex(1), 1, 4);
                        lc_getupvalue(L, lua_upvalueindex(1), 1, 3);
                        lua_pushinteger(L, 1);
                        lua_arith(L, LUA_OPSUB);
                        lua_call(L, 2, 1);
                        lua_pushvalue(L, 7);
                        lua_pushliteral(L, "_G");
                        lua_call(L, 4, LUA_MULTRET);
                        lua_call(L, (lua_gettop(L) - lc22), 0);
                    } else {
                        luaL_getsubtable(L, LUA_REGISTRYINDEX, "_LOADED");
                        if (!lua_equal(L, 8, -1)) {
                            lua_pushcfunction(L, luaB_tonumber);
                            lua_pushvalue(L, 7);
                            lua_call(L, 1, 1);
                            const int lc27 = lua_toboolean(L, -1);
                            lua_pop(L, 1);
                            if (lc27) {
                                lua_pushcfunction(L, lc_string_format);
                                lua_pushliteral(L, "[%s]");
                                lua_pushvalue(L, 7);
                                lua_call(L, 2, 1);
                                lua_replace(L, 7);
                            } else {
                                lua_pushcfunction(L, lc_string_format);
                                lua_pushliteral(L, "[\"%s\"]");
                                lua_pushvalue(L, 7);
                                lua_call(L, 2, 1);
                                lua_replace(L, 7);
                            }
                            lua_settop(L, 8);
                            lua_pushcfunction(L, lc_table_insert);
                            lc_getupvalue(L, lua_upvalueindex(1), 3, 1);
                            lua_pushcfunction(L, lc_string_format);
                            lua_pushliteral(L, "\r\n%s%s\t= ");
                            lua_pushcfunction(L, lc_string_rep);
                            lc_getupvalue(L, lua_upvalueindex(1), 1, 4);
                            lc_getupvalue(L, lua_upvalueindex(1), 1, 3);
                            lua_pushinteger(L, 1);
                            lua_arith(L, LUA_OPSUB);
                            lua_call(L, 2, 1);
                            lua_pushvalue(L, 7);
                            lua_call(L, 3, LUA_MULTRET);
                            lua_call(L, (lua_gettop(L) - 9), 0);
                            if (lua_istable(L, 8)) {
                                lc_getupvalue(L, lua_upvalueindex(1), 2, 2);
                                lua_pushcfunction(L, luaB_tostring);
                                lua_pushvalue(L, 8);
                                lua_call(L, 1, 1);
                                lua_gettable(L, -2);
                                lua_remove(L, -2);
                                lua_pushnil(L);
                                const int lc37 = lua_equal(L, -2, -1);
                                lua_pop(L, 2);
                                if (lc37) {
                                    lua_pushvalue(L, 8);
                                    lc_getupvalue(L, lua_upvalueindex(1), 2, 2);
                                    lua_insert(L, -2);
                                    lua_pushcfunction(L, luaB_tostring);
                                    lua_pushvalue(L, 8);
                                    lua_call(L, 1, 1);
                                    lua_insert(L, -2);
                                    lua_settable(L, -3);
                                    lua_pop(L, 1);
                                    lua_pushvalue(L, 2);
                                    lua_pushvalue(L, 7);
                                    lua_concat(L, 2);
                                    lua_pushvalue(L, 9);
                                    lc_getupvalue(L, lua_upvalueindex(1), 2, 2);
                                    lua_insert(L, -2);
                                    lua_pushcfunction(L, luaB_tostring);
                                    lua_pushvalue(L, 8);
                                    lua_call(L, 1, 1);
                                    lua_insert(L, -2);
                                    lua_settable(L, -3);
                                    lua_pop(L, 1);
                                    lc_getupvalue(L, lua_upvalueindex(1), 0, 5);
                                    lua_pushvalue(L, 8);
                                    lua_pushvalue(L, 9);
                                    lua_call(L, 2, 0);
                                } else {
                                    lua_pushcfunction(L, lc_table_insert);
                                    const int lc39 = lua_gettop(L);
                                    lc_getupvalue(L, lua_upvalueindex(1), 3, 1);
                                    lua_pushcfunction(L, luaB_tostring);
                                    lc_getupvalue(L, lua_upvalueindex(1), 2, 2);
                                    lua_pushcfunction(L, luaB_tostring);
                                    lua_pushvalue(L, 8);
                                    lua_call(L, 1, 1);
                                    lua_gettable(L, -2);
                                    lua_remove(L, -2);
                                    lua_call(L, 1, LUA_MULTRET);
                                    lua_call(L, (lua_gettop(L) - lc39), 0);
                                    lua_pushcfunction(L, lc_table_insert);
                                    lc_getupvalue(L, lua_upvalueindex(1), 3, 1);
                                    lua_pushliteral(L, ";");
                                    lua_call(L, 2, 0);
                                }
                            } else {
                                lc_getupvalue(L, lua_upvalueindex(1), 0, 5);
                                lua_pushvalue(L, 8);
                                lua_pushvalue(L, 2);
                                lua_call(L, 2, 0);
                            }
                        }
                    }
                    lua_settop(L, 6);
                }
                lua_settop(L, 3);
                lua_pushcfunction(L, lc_table_insert);
                lc_getupvalue(L, lua_upvalueindex(1), 3, 1);
                lua_pushcfunction(L, lc_string_format);
                const int lc41 = lua_gettop(L);
                lua_pushliteral(L, "\r\n%s}");
                lua_pushcfunction(L, lc_string_rep);
                lc_getupvalue(L, lua_upvalueindex(1), 1, 4);
                lc_getupvalue(L, lua_upvalueindex(1), 1, 3);
                lua_pushinteger(L, 1);
                lua_arith(L, LUA_OPSUB);
                lua_call(L, 2, LUA_MULTRET);
                lua_call(L, (lua_gettop(L) - lc41), LUA_MULTRET);
                lua_call(L, (lua_gettop(L) - 4), 0);
                lc_getupvalue(L, lua_upvalueindex(1), 1, 3);
                lua_pushinteger(L, 2);
                lua_arith(L, LUA_OPSUB);
                lc_setupvalue(L, lua_upvalueindex(1), 1, 3);
            }
            lua_settop(L, 3);
            break;
        }
        default: {
            lua_pushcfunction(L, lc_table_insert);
            const int lc42 = lua_gettop(L);
            lc_getupvalue(L, lua_upvalueindex(1), 3, 1);
            lua_pushcfunction(L, luaB_tostring);
            lua_pushvalue(L, 1);
            lua_call(L, 1, LUA_MULTRET);
            lua_call(L, (lua_gettop(L) - lc42), 0);
            break;
        }
    }
    lua_settop(L, 2);
    lua_pushcfunction(L, lc_table_insert);
    lc_getupvalue(L, lua_upvalueindex(1), 3, 1);
    lua_pushliteral(L, " ;");
    lua_call(L, 2, 0);
    lc_getupvalue(L, lua_upvalueindex(1), 3, 1);
    return 1;
}

static int luaB_dump(lua_State *L) {
    if (lc_table_concat == NULL) {
        lua_State *L2 = luaL_newstate();
        luaL_openlibs(L2);

        lua_getglobal(L2, "table");
        lua_pushliteral(L2, "concat");
        lua_gettable(L2, -2);
        lc_table_concat = lua_tocfunction(L2, -1);
        lua_pop(L2, 1);
        lua_pushliteral(L2, "insert");
        lua_gettable(L2, -2);
        lc_table_insert = lua_tocfunction(L2, -1);
        lua_pop(L2, 2);
        lua_getglobal(L2, "string");
        lua_getfield(L2, -1, "rep");
        lc_string_rep = lua_tocfunction(L2, -1);
        lua_pop(L2, 1);
        lua_getfield(L2, -1, "format");
        lc_string_format = lua_tocfunction(L2, -1);
        lua_pop(L2, 2);
        lua_close(L2);
    }
    lua_settop(L, 1);
    lua_newtable(L);
    lua_pushvalue(L, lua_upvalueindex(1));
    lua_rawseti(L, -2, 0);
    lua_newtable(L);
    lua_rawseti(L, 2, 1);
    lua_newtable(L);
    lua_pushvalue(L, 2);
    lua_rawseti(L, -2, 0);
    lua_newtable(L);
    lua_rawseti(L, 3, 2);
    lua_newtable(L);
    lua_newtable(L);
    lua_pushvalue(L, 3);
    lua_rawseti(L, -2, 0);
    lua_pushliteral(L, "  ");
    lua_pushinteger(L, 0);
    lua_rawseti(L, 5, 3);
    lua_rawseti(L, 5, 4);
    lua_newtable(L);
    lua_pushvalue(L, 5);
    lua_rawseti(L, -2, 0);
    lua_pushvalue(L, 6);
    lua_pushcclosure(L, luaB_dump_ToString, 1);
    lua_rawseti(L, 6, 5);
    lc_getupvalue(L, 6, 0, 5);
    lua_pushvalue(L, 1);
    lua_pushliteral(L, "");
    lua_call(L, 2, 1);
    lc_setupvalue(L, 6, 3, 1);
    lua_pushcfunction(L, lc_table_concat);
    lc_getupvalue(L, 6, 3, 1);
    lua_call(L, 1, 1);
    return 1;
}

#endif
static const luaL_Reg base_funcs[] = {
#ifdef LUA_COMPAT_DUMP
        {"dump", luaB_dump},
#endif
        {"assert", luaB_assert},
        {"collectgarbage", luaB_collectgarbage},
        {"dofile", luaB_dofile},
        {"error", luaB_error},
#if defined(LUA_COMPAT_MODULE)
        {"findtable", findtable},
#endif
        {"getmetatable", luaB_getmetatable},
        {"ipairs", luaB_ipairs},
        {"loadfile", luaB_loadfile},
        {"load", luaB_load},
#if defined(LUA_COMPAT_LOADSTRING)
        {"loadstring", luaB_loadstring},
#endif
        {"next", luaB_next},
        {"pairs", luaB_pairs},
        {"pcall", luaB_pcall},
        {"print", luaB_print},
        {"rawequal", luaB_rawequal},
        {"rawlen", luaB_rawlen},
        {"rawget", luaB_rawget},
        {"rawset", luaB_rawset},
        {"select", luaB_select},
        {"setmetatable", luaB_setmetatable},
        {"setmetamethod", luaB_setmetamethod},
        {"tointeger", luaB_tointeger},
        {"tonumber", luaB_tonumber},
        {"tostring", luaB_tostring},
        {"type", luaB_type},
        {"xpcall", luaB_xpcall},
        /* placeholders */
        {"_G", NULL},
        {"_VERSION", NULL},
        {NULL, NULL}
};


LUAMOD_API int luaopen_base(lua_State *L) {
    /* open lib into global table */
    lua_pushglobaltable(L);
    luaL_setfuncs(L, base_funcs, 0);
    /* set global _G */
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "_G");
    /* set global _VERSION */
    lua_pushliteral(L, LUA_VERSION);
    lua_setfield(L, -2, "_VERSION");
    return 1;
}

