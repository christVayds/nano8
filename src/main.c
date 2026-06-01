#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <raylib.h>

#include "game.h"
#include "scene.h"
#include "font.h"
#include "console.h"
#include "editor.h"
#include "sprite.h"
#include "luaapi.h"
#include "maps.h"

//#include <emscripten/emscripten.h>
static void UpdateGame(void);

static Scene scene;

int main(){
  InitWindow(SCREENWIDTH*SCREENSCALE, SCREENHEIGHT*SCREENSCALE, "Nano 8 - Untitled.n8");
  SetTargetFPS(MAXFPS);

  SetExitKey(KEY_NULL);
  HideCursor();

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
  //  SPRITE EDITOR 
  // ------------------
  SpriteInit();

  // ------------------
  //  MAP EDITOR 
  // ------------------
  MapInit();

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
  FreeSections();
  SpriteFree();
  CloseAudioDevice();
  CloseWindow();

  return 0;
}

// ------------------
//  UPDATE GAME 
// ------------------
static void UpdateGame(void){
  // ------------------
  //  UPDATE GAME 
  // ------------------
  UpdateScene(&scene);        // update scene 
  InputScene(&scene);         // input scene
  
  bool f_update = false;
  if(GetCartIfRunning())
    f_update = CallLuaFunction("_update"); 

  // ------------------
  //  DRAW GAME 
  // ------------------
  BeginDrawing();
  
  // clear backgound
  ClearBackground(GetNanoColor(0));
  
  DrawScene(&scene);                // draw scene 
  bool f_draw = false;
  if(GetCartIfRunning()) f_draw = CallLuaFunction("_draw");


  if(GetCartIfRunning() && (!f_update && !f_draw)){
    SetCartRunning(false);
    CloseEditor();
    ResetLuaForEditor();
  }
  //DrawFPS(10, 10);

  EndDrawing();
}
