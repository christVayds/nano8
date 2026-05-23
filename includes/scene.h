#ifndef SCENE_H
#define SCENE_H

#include "console.h"
#include <stdbool.h>

typedef enum{
  SCENE_BOOT,
  SCENE_CONSOLE,
  SCENE_EDITOR
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
