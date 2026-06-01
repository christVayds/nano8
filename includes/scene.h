#ifndef SCENE_H
#define SCENE_H

#include "console.h"
#include <stdbool.h>

typedef enum{
  SCENE_CONSOLE,        // 0
  SCENE_EDITOR,         // 1
  SCENE_SPRITE_EDITOR,  // 2
  SCENE_MAP_EDITOR,     // 3
  SCENE_SFX_EDITOR,     // 4
  SCENE_MUSIC_EDITOR    // 5
} SceneType;

typedef enum{
  ETAB_SCRIPT,
  ETAB_SPRITE,
  ETAB_MAP,
  ETAB_SOUND,
  ETAB_MUSIC
} EditorTab;

typedef struct{
  SceneType sceneType;      // current scene 
  EditorTab editorTab;
  Console *console;
} Scene;

void UpdateScene(Scene *scene);
void DrawScene(Scene *scene);
void InputScene(Scene *scene);

#endif
