#include "scene.h"
#include <raylib.h>
#include <stdio.h>

#include "game.h"
#include "editor.h"
#include "sprite.h"
#include "maps.h"
#include "sfx.h"

static void DrawSceneUI(Scene *scene);
static void SwitchScene(Scene *scene, SceneType sceneType, int32_t isDown);

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
    case SCENE_SPRITE_EDITOR:
      SpriteUpdate();
      break;
    case SCENE_MAP_EDITOR:
      MapUpdate();
      break;
    case SCENE_SFX_EDITOR:
      SfxUpdate();
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
      case SCENE_CONSOLE:         // go to editor 
        if(GetCartIfRunning()){
          SetCartRunning(false);
          CloseEditor();
          ResetLuaForEditor();
        } else {
          scene->sceneType = SCENE_EDITOR;
          ShowCursor();
          Pal(0,0);
          Palt(0,0, true);
        }
        break;
      default:                    // go to console 
        scene->sceneType = SCENE_CONSOLE;
        HideCursor();
        break;
    }
  }

  if(scene->sceneType > SCENE_CONSOLE){
    SwitchScene(scene, SCENE_EDITOR, IsKeyPressed(KEY_F1));
    SwitchScene(scene, SCENE_SPRITE_EDITOR, IsKeyPressed(KEY_F2));
    SwitchScene(scene, SCENE_MAP_EDITOR, IsKeyPressed(KEY_F3));
    SwitchScene(scene, SCENE_SFX_EDITOR, IsKeyPressed(KEY_F4));
    SwitchScene(scene, SCENE_MUSIC_EDITOR, IsKeyPressed(KEY_F5));
  }

  // PER SCENE INPUT 
  switch(scene->sceneType){
    case SCENE_CONSOLE:
      InputConsole(scene->console);
      break;
    case SCENE_EDITOR:
      InputEditor();
      break;
    case SCENE_SPRITE_EDITOR:
      SpriteInput();
      break;
    case SCENE_MAP_EDITOR:
      MapInput();
      break;
    case SCENE_SFX_EDITOR:
      SfxInput();
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
      DrawSceneUI(scene);
      break;
    case SCENE_SPRITE_EDITOR:
      SpriteDraw();
      DrawSceneUI(scene);
      break;
    case SCENE_MAP_EDITOR:
      MapDraw();
      DrawSceneUI(scene);
      break;
    case SCENE_SFX_EDITOR:
      SfxDraw();
      DrawSceneUI(scene);
      break;
    default:
      DrawSceneUI(scene);
      break;
  }
}

static void DrawSceneUI(Scene *scene){
  Vector2 position = {88, 0};
  int32_t iconIndex = 0;
  for(int32_t i=1;i<=5;i++){ 
    if((int32_t)scene->sceneType == i)
      DrawRectangle(position.x*SCREENSCALE, position.y, TILESIZE*SCREENSCALE, TILESIZE*SCREENSCALE, GetNanoColor(8));
    //DrawRectangleLines(position.x, position.y, TILESIZE*SCREENSCALE, TILESIZE*SCREENSCALE, WHITE);
    DrawIcons(iconIndex++, (Vector2){position.x, position.y}, 6);
    position.x += TILESIZE;
  }
}

static void SwitchScene(Scene *scene, SceneType sceneType, int32_t isDown){
  if(isDown){
    scene->sceneType = sceneType;
  }
}
