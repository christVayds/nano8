#include "luaapi.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <raylib.h>
#include <math.h>
#include "game.h"
#include "font.h"

#define MAX_LOG 1024

ButtonState buttons[BUTTON_COUNT];

lua_State *InitLuaState(){
  lua_State *L = luaL_newstate();
  luaL_openlibs(L);

  lua_register(L, "print", l_print);
  lua_register(L, "cls", l_cls);
  lua_register(L, "pset", l_pset);
  lua_register(L, "pget", l_pget);
  lua_register(L, "line", l_line);
  lua_register(L, "rect", l_rect);
  lua_register(L, "rectfill", l_rectfill);
  lua_register(L, "circfill", l_circfill);
  lua_register(L, "circ", l_circ);
  lua_register(L, "btn", l_btn);
  lua_register(L, "btnp", l_btnp);
  lua_register(L, "spr", l_spr);
  lua_register(L, "map", l_map);
  lua_register(L, "mset", l_mset);
  lua_register(L, "mget", l_mget);
  lua_register(L, "fget", l_fget);
  lua_register(L, "fset", l_fset);
  lua_register(L, "camera", l_camera);
  lua_register(L, "cursor", l_cursor);
  lua_register(L, "pal", l_pal);
  lua_register(L, "palt", l_palt);
  lua_register(L, "sspr", l_sspr);
  lua_register(L, "flr", l_flr);
  lua_register(L, "abs", l_abs);
  lua_register(L, "ciel", l_ciel);
  lua_register(L, "sqrt", l_sqrt);
  lua_register(L, "sin", l_sin);
  lua_register(L, "cos", l_cos);
  lua_register(L, "rand", l_rand);
  lua_register(L, "srand", l_srand);
  lua_register(L, "min", l_min);
  lua_register(L, "max", l_max);

  // GLOBAL VARIABLES

  // KEYS 
  lua_pushnumber(L, 0); lua_setglobal(L, "K_A");  // K_A = 0
  lua_pushnumber(L, 1); lua_setglobal(L, "K_S");  // K_S = 1 
  lua_pushnumber(L, 2); lua_setglobal(L, "K_Z");  // K_Z = 2 
  lua_pushnumber(L, 3); lua_setglobal(L, "K_X");  // K_X = 3
  lua_pushnumber(L, 4); lua_setglobal(L, "K_L");  // K_L = 4 (left)
  lua_pushnumber(L, 5); lua_setglobal(L, "K_R");  // K_R = 5 (right)
  lua_pushnumber(L, 6); lua_setglobal(L, "K_U");  // K_U = 6 (up)
  lua_pushnumber(L, 7); lua_setglobal(L, "K_D");  // K_D = 7 (down)

  return L;
}

int l_print(lua_State *L){
  Vector2 position = GetCursorPosition();

  const char* text = lua_tostring(L, 1);
  if(!text) text = "[NIL]";

  int posx = luaL_optinteger(L, 2, position.x); // default is 0
  int posy = luaL_optinteger(L, 3, position.y); // default is 0
  
  int colorIndex = luaL_optinteger(L, 4, 7); // default color is white 
 
  // print/display the text
  ChangeTextCurrentColor(colorIndex);
  PrintTextScreen(text, &(Vector2){posx, posy});

  return 0;
}

int l_pset(lua_State *L){
  int posx = luaL_optinteger(L, 1, 0);
  int posy = luaL_optinteger(L, 2, 0);
  int colorIndex = luaL_optinteger(L, 3, 0);
  pset(posx, posy, colorIndex);

  return 0;
}

int l_cls(lua_State *L){
  int colorIndex = luaL_optinteger(L, 1, 0); // default clear color is black
  if(colorIndex < COLORCOUNT)
    ClearScreen(colorIndex); 
  else ClearScreen(0);
  SetCursorPosition((Vector2){FONTWIDTH, SCREENSCALE});
  return 0;
}

// get the color of a pixel 
// for collision detection, flood fill, sprite masking, effects, debugging
int l_pget(lua_State *L){
  int posx = luaL_optinteger(L, 1, 0);   // x position 
  int posy = luaL_optinteger(L, 2, 0);   // y position 
  lua_pushinteger(L, pget(posx, posy));

  return 1;
}

// Draw line
// used for UI, wireframes, debuging visuals, premetive graphics
int l_line(lua_State *L){
  int posx1 = luaL_optinteger(L, 1, 0); 
  int posy1 = luaL_optinteger(L, 2, 0);
  int posx2 = luaL_optinteger(L, 3, 0);
  int posy2 = luaL_optinteger(L, 4, 0);
  int colorIndex = luaL_optinteger(L, 5, 7);
  
  DrawScreenLine(posx1, posy1, posx2, posy2, colorIndex);

  return 0;
}

int l_rect(lua_State *L){
  int posx = luaL_optinteger(L, 1, 0); 
  int posy = luaL_optinteger(L, 2, 0);
  int width = luaL_optinteger(L, 3, 0);
  int height = luaL_optinteger(L, 4, 0);
  int colorIndex = luaL_optinteger(L, 5, 7);

  DrawScreenLine(posx, posy, posx + width, posy, colorIndex);
  DrawScreenLine(posx, posy, posx, posy + height, colorIndex);
  DrawScreenLine(posx + width, posy, posx + width, posy + height, colorIndex);
  DrawScreenLine(posx, posy + height, posx + width, posy + height, colorIndex);

  return 0;
}

int l_rectfill(lua_State *L){
  int posx = luaL_optinteger(L, 1, 0); 
  int posy = luaL_optinteger(L, 2, 0);
  int width = luaL_optinteger(L, 3, 0);
  int height = luaL_optinteger(L, 4, 0);
  int colorIndex = luaL_optinteger(L, 5, 7);

  DrawRectFill(posx, posy, width, height, colorIndex);
  return 0;
}

int l_circfill(lua_State *L){
  int cx = luaL_optinteger(L, 1, 0); 
  int cy = luaL_optinteger(L, 2, 0);
  int radius = luaL_optinteger(L, 3, 0);
  int colorIndex = luaL_optinteger(L, 4, 7);
  
  DrawCircFill(cx, cy, radius, colorIndex);

  return 0;
}

int l_circ(lua_State *L){
  int cx = luaL_optinteger(L, 1, 0); 
  int cy = luaL_optinteger(L, 2, 0);
  int radius = luaL_optinteger(L, 3, 0);
  int colorIndex = luaL_optinteger(L, 4, 7);
  
  DrawCirc(cx, cy, radius, colorIndex);

  return 0;
}

int l_btn(lua_State *L){
  int btn = luaL_optinteger(L, 1, 0);
  if(btn < 0 || btn > BUTTON_COUNT)
    lua_pushboolean(L, 0);
  else 
    lua_pushboolean(L, buttons[btn].current);
  return 1;
}

int l_btnp(lua_State *L){
  int btn = luaL_optinteger(L, 1, 0);
  if(btn < 0 || btn > BUTTON_COUNT)
    lua_pushboolean(L, 0);
  else 
    lua_pushboolean(L, buttons[btn].current && !buttons[btn].prev);
  return 1;
}

void UpdateButton(Button btn, int isDown){
  buttons[btn].prev = buttons[btn].current;
  buttons[btn].current = isDown;
  if(isDown){
    buttons[btn].holdFrames++;
  } else {
    buttons[btn].holdFrames = 0;
  }
}

void PoolInput(void){
  UpdateButton(BUTTON_LEFT, IsKeyDown(KEY_LEFT));
  UpdateButton(BUTTON_RIGHT, IsKeyDown(KEY_RIGHT));
  UpdateButton(BUTTON_UP, IsKeyDown(KEY_UP));
  UpdateButton(BUTTON_DOWN, IsKeyDown(KEY_DOWN));

  UpdateButton(BUTTON_A, IsKeyDown(KEY_A));
  UpdateButton(BUTTON_S, IsKeyDown(KEY_S));
  UpdateButton(BUTTON_Z, IsKeyDown(KEY_Z));
  UpdateButton(BUTTON_X, IsKeyDown(KEY_X));
}

int l_spr(lua_State *L){
  int sprIndex = luaL_optinteger(L, 1, 0);  // sprite index
  int posx = luaL_optinteger(L, 2, 0);      // position x 
  int posy = luaL_optinteger(L, 3, 0);      // position y
  int width = luaL_optinteger(L, 4, 1);     // width value 
  int height = luaL_optinteger(L, 5, width);    // height value

  DrawSpr(sprIndex, posx, posy, width, height);

  return 0;
}

int l_map(lua_State *L){
  int celX = luaL_checkinteger(L, 1);
  int celY = luaL_checkinteger(L, 2);
  int sx = luaL_checkinteger(L, 3);
  int sy = luaL_checkinteger(L, 4);
  int celW = luaL_checkinteger(L, 5);
  int celH = luaL_checkinteger(L, 6);

  Map(celX, celY, sx, sy, celW, celH);

  return 0;
}

int l_mget(lua_State *L){
  int x = luaL_checkinteger(L, 1);
  int y = luaL_checkinteger(L, 2);
  int mapGet = MGet(x, y);
  lua_pushinteger(L, mapGet);

  return 1;
}

int l_mset(lua_State *L){
  int x = luaL_checkinteger(L, 1);
  int y = luaL_checkinteger(L, 2);
  int tile = luaL_checkinteger(L, 3);
  MSet(x, y, tile);
  
  return 0;
}

int l_fget(lua_State *L){
  int sprite = luaL_checkinteger(L, 1);
  int flag = luaL_checkinteger(L, 2);
  lua_pushboolean(L, FGet(sprite, flag));
  
  return 1;
}

int l_fset(lua_State *L){
  int sprite = luaL_checkinteger(L, 1);
  int flag = luaL_checkinteger(L, 2);
  int value = luaL_checkinteger(L, 3);
  FSet(sprite, flag, value);

  return 0;
}

int l_camera(lua_State *L){
  int x = luaL_optinteger(L, 1, 0);
  int y = luaL_optinteger(L, 2, 0);
  NanoCamera(x, y);

  return 0;
}

// sets where print() starts drawing text
int l_cursor(lua_State *L){
  int x = luaL_optinteger(L, 1, 0);
  int y = luaL_optinteger(L, 2, 0);
  SetCursorPosition((Vector2){x, y});
  return 0;
}

int l_pal(lua_State *L){
  int oldColor = luaL_optinteger(L, 1, 0);
  int newColor = luaL_optinteger(L, 2, 0);
  Pal(oldColor, newColor);

  return 0;
}

int l_palt(lua_State *L){
  int argc = lua_gettop(L);
  if(!argc){
    Palt(0, 0, true);
  } else {
    int color = luaL_checkinteger(L, 1);
    int set = lua_toboolean(L, 2);
    Palt(color, set, false);
  }

  return 0;
}

int l_sspr(lua_State *L){
  int sx = luaL_checkinteger(L, 1);
  int sy = luaL_checkinteger(L, 2);
  int sw = luaL_checkinteger(L, 3);
  int sh = luaL_checkinteger(L, 4);
  int dx = luaL_checkinteger(L, 5);
  int dy = luaL_checkinteger(L, 6);
  int dh = luaL_optinteger(L, 7, sw);
  int dw = luaL_optinteger(L, 8, sw);

  Sspr(sx, sy, sw, sh, dx, dy, dw, dh);
  return 0;
}

int l_flr(lua_State *L){
  float x = luaL_checknumber(L, 1);
  lua_pushinteger(L, (int)x);
  return 1;
}

int l_abs(lua_State *L){
  float x = luaL_checknumber(L, 1);
  lua_pushinteger(L, (x < 0) ? -x : x);
  return 1;
}

int l_ciel(lua_State *L){
  float x = luaL_checknumber(L, 1);
  int i = (int)x;
  lua_pushinteger(L, (x > i) ? i + 1 : i);

  return 1;
}

int l_sqrt(lua_State *L){
  float x = luaL_checknumber(L, 1);
  lua_pushnumber(L, sqrtf(x));
  return 1;
}

int l_sin(lua_State *L){
  float x = luaL_checknumber(L, 1);
  lua_pushnumber(L, sinf(x * 2.0f * M_PI));
  return 1;
}

int l_cos(lua_State *L){
  float x = luaL_checknumber(L, 1);
  lua_pushnumber(L, cosf(x * 2.0f * M_PI));
  return 1;
}

int l_rand(lua_State *L){
  float x = luaL_checknumber(L, 1);
  lua_pushnumber(L, ((float)rand() / RAND_MAX) * x);
  return 1;
}

int l_srand(lua_State *L){
  float seed = luaL_checknumber(L, 1);
  srand(seed);
  return 0;
}

int l_min(lua_State *L){
  float x = luaL_checknumber(L, 1);
  float y = luaL_checknumber(L, 2);
  lua_pushnumber(L, (x < y ? x : y));
  return 1;
}

int l_max(lua_State *L){
  float x = luaL_checknumber(L, 1);
  float y = luaL_checknumber(L, 2);
  lua_pushnumber(L, (x > y ? x : y));
  return 1;
}
