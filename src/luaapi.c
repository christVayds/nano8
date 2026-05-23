#include "luaapi.h"

#include <stdio.h>
#include <stdio.h>
#include <raylib.h>
#include "draw.h"
#include "game.h"
#include "font.h"

#define MAX_LOG 1024

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

  return L;
}

int l_print(lua_State *L){
  Vector2 position = GetCursorPosition();

  const char* text = lua_tostring(L, 1);
  if(!text) text = "[NIL]";

  int posx = luaL_optinteger(L, 2, position.x); // default is 0
  int posy = luaL_optinteger(L, 3, position.y); // default is 0
  
  int colorIndex = luaL_optinteger(L, 4, 7); // default color is white
  _DrawText(text, (Vector2){posx, posy}, colorIndex);
 
  // print/display the text
  ChangeTextCurrentColor(colorIndex);
  PrintTextScreen(text, &(Vector2){posx, posy});
  posy += FONTHEIGHT;
  posx = FONTWIDTH;
  SetCursorPosition((Vector2){posx, posy});

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
  lua_pushinteger(L, GetPixelScreenColor(posx, posy));

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

void CallLuaFunction(lua_State *L, const char* funcname, bool *funcExist, bool *catchErr){
  lua_getglobal(L, "debug");
  lua_getfield(L, -1, "traceback");
  lua_remove(L, -2);
  int errFunc = lua_gettop(L);

  lua_getglobal(L, funcname); // push function into stack 

  if(lua_isfunction(L, -1)){
    // call with 0 atgs and 0 returns
    *catchErr = false;
    if(lua_pcall(L, 0, LUA_MULTRET, errFunc) != LUA_OK){
      printf("Error in %s: %s\n", funcname, lua_tostring(L, -1));
      lua_pop(L, 1); 
      lua_remove(L, errFunc);
      *catchErr = true;
    }
    *funcExist = true;
  } else {
    lua_pop(L, 2); // not a function, pop it
    *funcExist = false;
  }
}
