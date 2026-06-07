#include "console.h"
#include <stdio.h>
#include <raylib.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "font.h"
#include "luaapi.h"
#include "draw.h"
#include "editor.h"
#include "cartridge.h"

static int currentCursorPos = 0;

static void RunLua(void);
static void InsertCharacter(char* line, int32_t pos, char c);
static void BackSpace(char* line, int32_t pos);
static void PushConsoleLog(const char *text);
static void GetCommandArgs(Console *console);

// CONSOLE LOG
#define CONSOLE_LOG_MAX 1024
static char console_log[CONSOLE_LOG_MAX][256];
static int32_t consoleLogCount = 0;
static int32_t consoleBack = 0;

#define COMMAND_ARGS_MAX 5
static char commandArgs[COMMAND_ARGS_MAX][256];
static int32_t argsCount = 0;

// NEW STATE LUA 
static lua_State *L = NULL;

Console InitConsole(void){
  Console newConsole;
  newConsole.cursor = 0;
  newConsole.newCommand = false;

  L = InitLuaState(); 
  return newConsole;
}

void CloseConsole(){
  lua_close(L);
}

void UpdateConsole(Console *console){
  PoolInput();

  Vector2 curPosition = GetCursorPosition();

  // TODO: CHECK THIS SOON, USE CAMERA INSTEAD
  if(curPosition.y > (SCREENHEIGHT - (FONTHEIGHT*SCREENSCALE))){ 
    ScrollUpScreen(FONTHEIGHT * SCREENSCALE);
    SetCursorPosition((Vector2){curPosition.x, curPosition.y - FONTHEIGHT * SCREENSCALE});
  }

  // get new commad here
  if(console->newCommand){
    // push command to the console log
    PushConsoleLog(console->command);

    // get command args 
    GetCommandArgs(console);

    Vector2 position = GetCursorPosition();

    // EXIT NANO 8
    if(strcmp(console->command, "exit") == 0){
      GameRunning(0);
    } else if(strcmp(console->command, "run") == 0){
      // RUN LUA PROGRAM
      RunLua();
    } else if(strcmp(console->command, "cls") == 0 || strcmp(console->command, "clear") == 0){
      ClearScreen(0);
      SetCursorPosition((Vector2){FONTWIDTH, SCREENSCALE});
    } else if(strcmp(commandArgs[0], "save") == 0){                 // SAVE CARTRIDGE
      if(argsCount < 2){
        ChangeTextCurrentColor(8);
        PrintText("Invalid argument", &position);
        position.y += FONTHEIGHT;
        position.x = FONTWIDTH;
        SetCursorPosition((Vector2){position.x, position.y});  
      } else {
        PushConsoleLog(console->command);

        ChangeTextCurrentColor(6);
        PrintText("saved", &position);
        position.y += FONTHEIGHT;
        position.x = FONTWIDTH;
        SetCursorPosition((Vector2){position.x, position.y});

        SaveCartridge(commandArgs[1]);
      }
    } else if(strcmp(commandArgs[0], "load") == 0){                   // LOAD CARTRIDGE
      if(argsCount < 2){
        ChangeTextCurrentColor(8);
        PrintText("Invalid argument", &position);
        position.y += FONTHEIGHT;
        position.x = FONTWIDTH;
        SetCursorPosition((Vector2){position.x, position.y});  
      } else {
        PushConsoleLog(console->command);

        if(!LoadCartridge(commandArgs[1])){
          ChangeTextCurrentColor(8);
          PrintText("Cartridge not found", &position);
          position.y += FONTHEIGHT;
          position.x = FONTWIDTH;
          SetCursorPosition((Vector2){position.x, position.y});
        } else {
          ChangeTextCurrentColor(8);
          PrintText("loaded", &position);
          position.y += FONTHEIGHT;
          position.x = FONTWIDTH;
          SetCursorPosition((Vector2){position.x, position.y}); 
        }
      }
    } else { // NANO 8 CONSOLE
      // LUA ERROR - CONSOLE
      if(luaL_dostring(L, console->command) != LUA_OK){
        ChangeTextCurrentColor(8);
        PrintText(lua_tostring(L, -1), &position);
        position.y += FONTHEIGHT;
        position.x = FONTWIDTH;
        SetCursorPosition((Vector2){position.x, position.y}); 

        lua_pop(L, 1);
      }
    }
 
    // clear command buffer  
    memset(console->command, 0, sizeof(console->command));
    console->newCommand = false; 
  }
}

void InputConsole(Console *console){
  if(GetCartIfRunning()) return;
  int key = GetCharPressed();
  
  if(key >= 32 && console->cursor < 256){
    InsertCharacter(console->buffer, console->cursor, key);
    console->cursor++;
    currentCursorPos++;
  }

  if(IsKeyPressed(KEY_BACKSPACE) && console->cursor > 0){ 
    BackSpace(console->buffer, console->cursor);
    console->cursor--;
    currentCursorPos--;
  }

  if(IsKeyPressed(KEY_ENTER)){
    console->newCommand = true;
    Vector2 getCursorPosition = GetCursorPosition();
    getCursorPosition.y += FONTHEIGHT;
    SetCursorPosition(getCursorPosition);

    strcpy(console->command, console->buffer);
    memset(console->buffer, 0, sizeof(console->buffer));
    console->cursor = 0;
    currentCursorPos = 0;
    consoleBack = consoleLogCount;
  }

  if(IsKeyPressed(KEY_LEFT) && console->cursor > 0){
    console->cursor--;
  }
  if(IsKeyPressed(KEY_RIGHT) && console->cursor < currentCursorPos){
    console->cursor++;
  }

  // TODO: fix this soon
  if(IsKeyPressed(KEY_UP)){
    console->cursor = 0;
    currentCursorPos = 0;
    if(consoleBack < 0) return;
    char *getLatestBuffer = console_log[consoleBack--];
    int32_t len = strlen(getLatestBuffer);
    strcpy(console->buffer, getLatestBuffer);
    console->cursor = len;
    currentCursorPos = len;
  }
}

void DrawConsole(Console *console){
  if(GetCartIfRunning()) return;

  Vector2 position = GetCursorPosition(); 
  position.x = FONTWIDTH;                   // reset cursor position

  if(!console->newCommand){
    ChangeTextCurrentColor(8);
    PrintText(">", &position);
    position.x += FONTWIDTH;
  }
  ChangeTextCurrentColor(6); 
  PrintText(console->buffer, &position); 
  GetFont(95, position, true, false);       // draw cursor 
}

// RUNNING LUA CODE FROM EDITOR

static void RunLua(void){
  lua_State *L_editor = GetEditorLua();
  char *luaCode = GetLuaCode();
  NanoCameraReset();
  
  // load and execute user code 
  if(luaL_dostring(L_editor, luaCode) != LUA_OK){
    printf("Lua Error: %s\n", lua_tostring(L_editor, -1));

    Vector2 position = GetCursorPosition();
    
    // push error message to the console log
    PushConsoleLog(lua_tostring(L_editor, -1));

    ChangeTextCurrentColor(8);
    PrintText(lua_tostring(L_editor, -1), &position);
    position.y += FONTHEIGHT;
    position.x = FONTWIDTH;
    SetCursorPosition((Vector2){position.x, position.y});

    lua_pop(L_editor, 1);
   
    SetCartRunning(false);
    CloseEditor();
    ResetLuaForEditor();
    
    free(luaCode);
    luaCode = NULL;
    return;
  }
  free(luaCode);
  luaCode = NULL;
  
  SetCartRunning(true);
  
  // call init function 
  CallLuaFunction("_init");
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

static void PushConsoleLog(const char *text){
  if(consoleLogCount < CONSOLE_LOG_MAX)
    strcpy(console_log[consoleLogCount++], text);
  else return;
}

bool CallLuaFunction(const char* funcname){
  lua_State *L_editor = GetEditorLua();
  bool funcExist = true;

  lua_getglobal(L_editor, funcname); // push function into stack 

  if(lua_isfunction(L_editor, -1)){
    Vector2 position = GetCursorPosition();

    // call with 0 atgs and 0 returns
    if(lua_pcall(L_editor, 0, 0, 0) != LUA_OK){
      printf("Error in %s: %s\n", funcname, lua_tostring(L_editor, -1));
      
      // push error message to the console log
      PushConsoleLog(lua_tostring(L_editor, -1));

      ChangeTextCurrentColor(8);
      PrintText(lua_tostring(L_editor, -1), &position);
      position.y += FONTHEIGHT;
      position.x = FONTWIDTH;
      SetCursorPosition((Vector2){position.x, position.y});

      lua_pop(L_editor, 1); 
     
      SetCartRunning(false);
      CloseEditor();
      ResetLuaForEditor();
    }
  } else {
    lua_pop(L_editor, 1); // not a function, pop it
    funcExist = false;
  }

  return funcExist;
}

static void GetCommandArgs(Console *console){
  argsCount = 0; 
  int32_t index = 0;
  char text[32];
  int32_t textCount = 0;
  
  while(true){
    if(console->command[index] == '\0'){
      strcpy(commandArgs[argsCount++], text);
      return;
    }
    if(console->command[index] == ' '){
      strcpy(commandArgs[argsCount++], text); 
      index++;
      textCount = 0;
      continue;
    }

    text[textCount++] = console->command[index];
    index++;
  }
}
