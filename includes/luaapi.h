#ifndef LUA_API_H
#define LUA_API_H

#include <stdbool.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

lua_State *InitLuaState();
int l_print(lua_State *L);
int l_cls(lua_State *L);
int l_pset(lua_State *L);
int l_pget(lua_State *L);
int l_line(lua_State *L);
int l_rect(lua_State *L);
int l_rectfill(lua_State *L);
int l_circfill(lua_State *L);
int l_circ(lua_State *L);

void CallLuaFunction(lua_State *L, const char* funcname, bool *funcExist, bool *catchErr);
#endif
