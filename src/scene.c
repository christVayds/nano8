#include "scene.h"
#include <raylib.h>
#include <stdio.h>

#include "game.h"
#include "editor.h"
// ------------------
//  UPDATE SCENE 
// ------------------
void UpdateScene(Scene *scene){
  switch(scene->sceneType){
    case SCENE_CONSOLE:
      UpdateConsole(scene->console);
      break;
    case SCENE_EDITOR:
      UpdateEditor();
      break;
    default:
      break;
  }
}

// ------------------
//  INPUT SCENE 
// ------------------
void InputScene(Scene *scene){
  // GLOBAL SCENE INPUT 
  if(IsKeyPressed(KEY_ESCAPE)){
    switch(scene->sceneType){
      case SCENE_CONSOLE:
        if(GetCartIfRunning()){
          SetCartRunning(false);
          CloseEditor();
          ResetLuaForEditor();
        } else {
          scene->sceneType = SCENE_EDITOR;
        }
        break;
      default:
        scene->sceneType = SCENE_CONSOLE;
        break;
    }
  }

  // PER SCENE INPUT 
  switch(scene->sceneType){
    case SCENE_CONSOLE:
      InputConsole(scene->console);
      break;
    case SCENE_EDITOR:
      InputEditor();
      break;
    default:
      break;
  }
}

// ------------------
//  DRAW SCENE 
// ------------------
void DrawScene(Scene *scene){
  ClearBackground(GetNanoColor(0));

  switch(scene->sceneType){
    case SCENE_CONSOLE:
      DrawScreen();
      DrawConsole(scene->console); 
      break;
    case SCENE_EDITOR:
      DrawEditor();
      break;
    default:
      break;
  }
}
