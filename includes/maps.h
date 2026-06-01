#ifndef MAP_H
#define MAP_H

#include <stdint.h>
#include <raylib.h>

typedef struct{
  int32_t selectedTile;
  Vector2 camera;
  int32_t tileSize;
  int32_t showSelector;
} MapEditor;

void MapInit(void);
void MapUpdate(void);
void MapInput(void);
void MapDraw(void);

#endif
