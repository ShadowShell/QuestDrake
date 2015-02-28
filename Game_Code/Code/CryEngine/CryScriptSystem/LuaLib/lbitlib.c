/* Bitwise operations library */
/* Reuben Thomas   Nov00-26Nov01 */

#include "lauxlib.h"
#include "lua.h"

#if defined(LINUX) || defined(APPLE) || defined(PS3) || defined(ORBIS)
typedef signed long long Integer;
typedef unsigned long long UInteger;
#else
typedef signed __int64 Integer;
typedef unsigned __int64 UInteger;
#endif

#pragma warning(disable: 4244)

static Integer s_luaL_check_number(lua_State *L,int index)
{
	return (Integer)lua_tonumber(L,index);
}

#define luaL_check_bit(L, n)  ((Integer)s_luaL_check_number(L, n))
#define luaL_check_ubit(L, n) ((UInteger)luaL_check_bit(L, n))

#define TDYADIC(name, op, t1, t2) \
  static int int_ ## name(lua_State* L) { \
    lua_pushnumber(L, \
      luaL_check_ ## t1 ## bit(L, 1) op luaL_check_ ## t2 ## bit(L, 2)); \
    return 1; \
  }

#define DYADIC(name, op) \
  TDYADIC(name, op, , )

#define MONADIC(name, op) \
  static int int_ ## name(lua_State* L) { \
    lua_pushnumber(L, op luaL_check_bit(L, 1)); \
    return 1; \
  }

#define VARIADIC(name, op) \
  static int int_ ## name(lua_State *L) { \
    int n = lua_gettop(L), i; \
    Integer w = luaL_check_bit(L, 1); \
    for (i = 2; i <= n; i++) \
      w op ## = luaL_check_bit(L, i); \
    lua_pushnumber(L, w); \
    return 1; \
  }
#pragma pack(push,1)
#pragma warning (disable : 4003)
MONADIC(not,     ~)
DYADIC(mod,      %)
VARIADIC(and,    &)
VARIADIC(or,     |)
VARIADIC(xor,    ^)
TDYADIC(lshift,  <<,, u)
TDYADIC(rshift,  >>, u, u)
TDYADIC(arshift, >>,, u)
#pragma pack(pop)

static const struct luaL_reg bitlib[] = {
	{"bnot",    int_not},
	{"imod",    int_mod},  /* "mod" already in Lua math library */
	{"band",    int_and},
	{"bor",     int_or},
	{"bxor",    int_xor},
	{"lshift",  int_lshift},
	{"rshift",  int_rshift},
	{"arshift", int_arshift},
	{NULL, NULL}
};

LUALIB_API void lua_bitlibopen (lua_State *L) {
	lua_pushvalue(L, LUA_GLOBALSINDEX);
	luaL_openlib(L, NULL, bitlib, 0);  /* open lib into global table */
  //luaL_openlib(L, "bitlib",bitlib,0);
}
