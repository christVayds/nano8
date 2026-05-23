#ifndef EDITOR_H 
#define EDITOR_H

#include <raylib.h>
#include <stdbool.h>
#include <stdint.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define TABSIZE 2

typedef struct{
  char code[1024];
  int32_t cursor;   // cursor position
  Vector2 cursorPosition;
} CodeEditor;

void InitEditor(void);
void ResetLuaForEditor(void);
void CloseEditor();
lua_State *GetEditorLua(void);
char* RunCode();
void UpdateEditor();
void InputEditor();
void DrawEditor();

bool GetCartIfRunning(void);
void SetCartRunning(bool set);

#endif
