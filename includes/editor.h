#ifndef EDITOR_H 
#define EDITOR_H

#include <raylib.h>
#include <stdbool.h>
#include <stdint.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define TABSIZE 2
#define MAXSECTIONS 9
#define TEXTSIZE 1024
#define VISIBLELINES 16
#define VISIBLECOLUMNS 30
#define TOKENLIMIT 69111

typedef enum{
  COL_DEFAULT = 6,
  COL_KEYWORD = 8,
  COL_NUMBER = 13,
  COL_STRING = 11,
  COL_COMMENT = 15,
  COL_FUNC = 12,
  COL_SYMBOL = 9,
  COL_SELECT = 1
} CodeColor;

typedef struct{
  char *code;
  int32_t codeSize;     // max size 
  int32_t codeCount;    // current size count 
  int32_t cursor;       // cursor position
  Vector2 cursorPosition;

  // CURSOR
  int32_t cursorLine;
  int32_t cursorColumn;
  int32_t scrollY;
  int32_t scrollX;
} CodeEditor;

typedef struct{
  CodeEditor *codeEditor;
  int32_t sectionsCount;
  int32_t sectionCurrent; 
} Sections;

void InitEditor(void);
void ResetLuaForEditor(void);
void CloseEditor(void);
lua_State *GetEditorLua(void);
char* GetLuaCode(void);
void UpdateEditor(void);
void InputEditor(void);
void DrawEditor(void);

bool GetCartIfRunning(void);
void SetCartRunning(bool set);

// SECTTIONS 
void InitSection(void);
void FreeSections(void);
void NewSection(void);
void RemoveSection(void);

#endif
