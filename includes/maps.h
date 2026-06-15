#ifndef MAP_H
#define MAP_H

#include <stdint.h>
#include <raylib.h>

typedef struct{
  int32_t selectedTile;
  Vector2 camera;
  int32_t tileSize;
  int32_t showSelector;
  
  // selection for map editor
  int32_t selectionActive; // 0/1
  int32_t selX, selY;      // top-left tile coordinates of selection
  int32_t selW, selH;      // selection width/height in tiles
  int8_t *selBuffer;       // temporary buffer for moved selection
  int32_t selectionMoving; // whether currently dragging the selection
} MapEditor;

void MapInit(void);
void MapUpdate(void);
void MapInput(void);
void MapDraw(void);

#endif
