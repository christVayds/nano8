#ifndef LUA_API_H
#define LUA_API_H

#include <stdbool.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

typedef enum{
  BUTTON_A,       //0
  BUTTON_S,       // 1
  BUTTON_Z,       // 2
  BUTTON_X,       // 3
  BUTTON_LEFT,    // 4
  BUTTON_RIGHT,   // 5
  BUTTON_UP,      // 6
  BUTTON_DOWN,    // 7
  BUTTON_COUNT    // 8
} Button;

typedef struct{
  int current;
  int prev;
  int holdFrames;
} ButtonState;

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
int l_btn(lua_State *L);
int l_btnp(lua_State *L);
int l_spr(lua_State *L);
int l_map(lua_State *L);
int l_mget(lua_State *L);
int l_mset(lua_State *L);
int l_fget(lua_State *L);
int l_fset(lua_State *L);
int l_camera(lua_State *L);
int l_cursor(lua_State *L);
int l_pal(lua_State *L);
int l_palt(lua_State *L);
int l_sspr(lua_State *L);

int l_flr(lua_State *L);
int l_abs(lua_State *L);
int l_ciel(lua_State *L);
int l_sqrt(lua_State *L);
int l_sin(lua_State *L);
int l_cos(lua_State *L);
int l_rand(lua_State *L);
int l_srand(lua_State *L);
int l_min(lua_State *L);
int l_max(lua_State *L);

void UpdateButton(Button btn, int isDown);
void PoolInput(void);

#endif
