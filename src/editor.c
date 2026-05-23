#include "editor.h"
#include "game.h"
#include "font.h"
#include "luaapi.h"

#include <stdio.h>
#include <string.h>

CodeEditor codeEditor;
int32_t tabCount = 1;
int32_t activeTab = 1;
bool cartRunning = false;

int32_t cursorLine = 0;
int32_t cursorColumn = 0;
int32_t scrollY = 0;
int32_t scrollX = 0;
int32_t visibleLines = 16;
int32_t visibleColumns = 30;

// back spacing
float backSpaceHoldTime = 0.0f;
float backSpaceRepeatTimer = 0.0f;
bool backSpaceHeld = false;

// define static functions
static void DrawTextEditor(const char* text, Vector2 *position);
static void DrawFont(const int32_t fontIndex, Vector2 position);
static void InsertCharacter(char* line, int32_t pos, char c);
static void BackSpace(char* line, int32_t pos);

// NEW STATE LUA 
lua_State *L_editor = NULL;

void InitEditor(void){
  codeEditor.cursor = 0;
  codeEditor.cursorPosition = (Vector2){0,0};
}

void ResetLuaForEditor(void){
  L_editor = InitLuaState();
}

void CloseEditor(){
  lua_close(L_editor);
}

lua_State *GetEditorLua(void){
  return L_editor;
}

char *RunCode(void){
  return codeEditor.code;
}

void UpdateEditor(){
}

void InputEditor(){
  int key = GetCharPressed();
  if(key >= 32){
    InsertCharacter(codeEditor.code, codeEditor.cursor, key);
    codeEditor.cursor++;
  }

  if(IsKeyDown(KEY_BACKSPACE) && codeEditor.cursor >= 1){
    if(!backSpaceHeld){
      BackSpace(codeEditor.code, codeEditor.cursor);
      codeEditor.cursor--;
      
      backSpaceHoldTime = 0.0f;
      backSpaceRepeatTimer = 0.5f;
      backSpaceHeld = true;
    } else {
      backSpaceHoldTime += GetFrameTime();
      backSpaceRepeatTimer -= GetFrameTime();

      if(backSpaceRepeatTimer <= 0.0f){
        BackSpace(codeEditor.code, codeEditor.cursor);
        codeEditor.cursor--;
        
        backSpaceRepeatTimer = 0.05f;
      }
    }
  } else backSpaceHeld = false;

  if(IsKeyPressed(KEY_ENTER)){
    InsertCharacter(codeEditor.code, codeEditor.cursor, '\n');
    codeEditor.cursor++;
  }

  if(IsKeyDown(KEY_LEFT_CONTROL) && IsKeyDown(KEY_LEFT_SHIFT) && IsKeyPressed(KEY_T)){
    if(tabCount > 1) tabCount--;
    activeTab = tabCount;
  } else if(IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_T)){
    tabCount++;
    activeTab = tabCount;
  }

  if(IsKeyPressed(KEY_TAB)){
    for(int32_t i=0;i<TABSIZE;i++){ 
      InsertCharacter(codeEditor.code, codeEditor.cursor, ' ');
      codeEditor.cursor++;
    }
  }
  
  if(IsKeyPressed(KEY_LEFT) && codeEditor.cursor > 0) codeEditor.cursor--;
  if(IsKeyPressed(KEY_RIGHT) && (size_t)codeEditor.cursor < strlen(codeEditor.code)) codeEditor.cursor++;

  if(IsKeyPressed(KEY_UP)){
    int32_t col = (int32_t)codeEditor.cursorPosition.x;

    // go to start of the current line 
    while(codeEditor.cursor > 0 && codeEditor.code[codeEditor.cursor -1] != '\n') codeEditor.cursor--;

    // jump over '\n' into prev line 
    if(codeEditor.cursor > 0) codeEditor.cursor--;

    // go to start of prev line 
    while(codeEditor.cursor > 0 && codeEditor.code[codeEditor.cursor - 1] != '\n') codeEditor.cursor--;

    for(int32_t i=0;i<col && codeEditor.code[codeEditor.cursor] != '\n' && codeEditor.code[codeEditor.cursor] != '\0';i++)
      codeEditor.cursor++;

    //printf("cursor %d\n", codeEditor.cursor);
  }

  if(IsKeyPressed(KEY_DOWN)){
    int32_t col = codeEditor.cursorPosition.x;

    // find the next '\n' (end of current line)
    while(codeEditor.code[codeEditor.cursor] != '\n' && codeEditor.code[codeEditor.cursor] != '\0') codeEditor.cursor++;

    // jump over '\n' into next line 
    if(codeEditor.code[codeEditor.cursor] == '\n') codeEditor.cursor++;

    // walk forward nu col (stop at end of line)
    for(int32_t i=0;i<col && codeEditor.code[codeEditor.cursor] != '\n' && codeEditor.code[codeEditor.cursor] != '\0';i++)
      codeEditor.cursor++;
  }
  
  codeEditor.cursorPosition.x = 0;
  codeEditor.cursorPosition.y = 0;
  for(int32_t i=0;i<codeEditor.cursor;i++){
    if(codeEditor.code[i] == '\n'){
      codeEditor.cursorPosition.y++;
      codeEditor.cursorPosition.x = 0;
    } else {
      codeEditor.cursorPosition.x++;
    }
  }

  // scroll for X and Y
  cursorLine = 0;
  for(int32_t i=0;i<codeEditor.cursor;i++){
    if(codeEditor.code[i] == '\n') cursorLine++;
  }

  cursorColumn = 0;
  for(int32_t i=codeEditor.cursor-1;i>=0;i--){
    if(codeEditor.code[i] == '\n') break;
    cursorColumn++;
  }

  if(cursorLine >= scrollY + visibleLines) scrollY = cursorLine - visibleLines + 1;
  if(cursorLine < scrollY) scrollY = cursorLine;

  if(cursorColumn >= scrollX + visibleColumns) scrollX = cursorColumn - visibleColumns + 1;
  if(cursorColumn < scrollX) scrollX = cursorColumn;
}

void DrawEditor(){
  // draw code
  Vector2 position = {1, TILESIZE+1};
  position.x += -(scrollX * FONTWIDTH);
  position.y += -(scrollY * FONTHEIGHT);
  DrawTextEditor(codeEditor.code, &position);

  // Cursor 
  //position.x += FONTWIDTH;
  position.x = 1 + -(scrollX * FONTWIDTH) + codeEditor.cursorPosition.x * FONTWIDTH;
  position.y = (TILESIZE+1+ -(scrollY * FONTHEIGHT)) + codeEditor.cursorPosition.y * FONTHEIGHT;
  DrawFont(95, position);

  // draw UI top and bottom
  DrawRectangle(0, SCREENHEIGHT*SCREENSCALE - TILESIZE*SCREENSCALE, SCREENWIDTH*SCREENSCALE, TILESIZE*SCREENSCALE, GetNanoColor(0));
  DrawRectangle(0, 0, SCREENWIDTH*SCREENSCALE, TILESIZE*SCREENSCALE, GetNanoColor(0));
  DrawRectangleLines(0, 0, SCREENWIDTH*SCREENSCALE, TILESIZE*SCREENSCALE, GetNanoColor(8));
  DrawRectangleLines(0, SCREENHEIGHT*SCREENSCALE - TILESIZE*SCREENSCALE, SCREENWIDTH*SCREENSCALE, TILESIZE*SCREENSCALE, GetNanoColor(8));
  
  // draw tabs
  int tabPosition = 0;
  int32_t countPosition = 2;
  char countAscii = '1';
  for(int32_t i=1;i<=tabCount;i++){
    if(activeTab == i)
      DrawRectangle(tabPosition, 0, TILESIZE*SCREENSCALE, TILESIZE*SCREENSCALE, GetNanoColor(8)); 
    DrawFont((countAscii++) - 32, (Vector2){countPosition, 1});
    countPosition += TILESIZE + 1;
    tabPosition += TILESIZE * SCREENSCALE + SCREENSCALE;
  }
}

bool GetCartIfRunning(void){
  return cartRunning;
}

void SetCartRunning(bool set){
  cartRunning = set;
}

static void DrawTextEditor(const char* text, Vector2 *position){
  const int32_t posx = (int32_t)position->x;

  int32_t index = 0;
  while(true){
    if(text[index] != '\0'){
      char c = text[index];
      int32_t fontIndex = c-32;

      if(c == '\n'){
        position->y += FONTHEIGHT;
        position->x = posx;
        fontIndex = -1;
      }

      if(fontIndex >= 0){
        DrawFont(fontIndex, *position);
        position->x += FONTWIDTH;
      }
    } else {
      return;
    }
    index++;
  }
}

static void DrawFont(const int32_t fontIndex, Vector2 position){
  const int32_t posx = position.x;
  for(int32_t y=0;y<FONTHEIGHT;y++){
    for(int32_t x=0;x<FONTWIDTH;x++){
      int32_t index = y * FONTWIDTH + x;
      uint32_t text = GetFontText(fontIndex)[index];
      
      if(text > 0)
        DrawRectangle(position.x * SCREENSCALE, position.y * SCREENSCALE, SCREENSCALE, SCREENSCALE, GetNanoColor(GetTextCurrentColor()));
      position.x += 1;
    } 
    position.x = posx;
    position.y += 1;
  }
}

static void InsertCharacter(char* line, int32_t pos, char c){
  int32_t len = strlen(line);

  for(int32_t i=len;i>=pos;i--){
    line[i + 1] = line[i];
  }

  line[pos] = c;
}

static void BackSpace(char* line, int32_t pos){
  if(pos <= 0) return;

  int32_t len = strlen(line);
  for(int32_t i=pos-1;i<len;i++){
    line[i] = line[i + 1];
  }
}
