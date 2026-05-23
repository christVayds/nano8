#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <raylib.h>

#include "game.h"
#include "scene.h"
#include "font.h"
#include "console.h"
#include "editor.h"
#include "luaapi.h"

//#include <emscripten/emscripten.h>
void UpdateGame(void);

Scene scene;

int main(){
  InitWindow(SCREENWIDTH*SCREENSCALE, SCREENHEIGHT*SCREENSCALE, "Nano 8");
  SetTargetFPS(MAXFPS);

  SetExitKey(KEY_NULL);

  // ------------------
  //  AUDIO 
  // ------------------
  InitAudioDevice();

  // ------------------
  //  CONSOLE 
  // ------------------
  Console console = InitConsole();

  // ------------------
  //  CODE EDITOR 
  // ------------------
  InitEditor();
  ResetLuaForEditor();

  // ------------------
  //  SCENE 
  // ------------------
  scene.sceneType = SCENE_CONSOLE;
  scene.editorTab = ETAB_SCRIPT;
  scene.console = &console;

  // ------------------
  //  LOOP FOR EXE 
  // ------------------ 
  while(!WindowShouldClose()){
    UpdateGame();

    if(!GameIsRunning()) break;
  }

  // ------------------
  //  LOOP FOR WEBASM
  // ------------------
  //emscripten_set_main_loop(UpdateGame, 0, 1);
 
  // ------------------
  //  ON EXIT 
  // ------------------
  CloseConsole();
  CloseEditor();
  CloseAudioDevice();
  CloseWindow();

  return 0;
}

// ------------------
//  UPDATE GAME 
// ------------------
void UpdateGame(void){
  // ------------------
  //  UPDATE GAME 
  // ------------------
  UpdateScene(&scene);        // update scene 
  InputScene(&scene);         // input scene
 
  bool hasUpdate = true;
  bool hasDraw = true;
  bool errUpdate = true;
  bool errDraw = true;
  if(GetCartIfRunning()){
    CallLuaFunction(GetEditorLua(), "_update", &hasUpdate, &errUpdate);
  } 

  // ------------------
  //  DRAW GAME 
  // ------------------
  BeginDrawing();
  
  // clear backgound
  ClearBackground(GetNanoColor(0));
  
  DrawScene(&scene);                // draw scene 
  if(GetCartIfRunning()){
    CallLuaFunction(GetEditorLua(), "_draw", &hasDraw, &errDraw);
  }

  // check if has update and has draw
  if((!hasUpdate && !hasDraw) || (errUpdate || errDraw)){
    SetCartRunning(false);
    CloseEditor();
    ResetLuaForEditor();
  }

  //DrawFPS(10, 10);

  EndDrawing();
}
