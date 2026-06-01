#include "maps.h"

#include <stdio.h>
#include <stdint.h>
#include "game.h"
#include "sprite.h"

#define MAPWIDTH 128
#define MAPHEIGHT 128

static int8_t mapData[MAPWIDTH*MAPHEIGHT];
static MapEditor mapEditor;

extern Sprite sprites[SPRITEWIDTH*SPRITEHEIGHT];
extern SpriteEditor spriteEditor;

// static functions for drawing and updates 
static void DrawMapEditor(Vector2 position);
static void DrawSpriteSelector(Vector2 position);
static void DrawMapTiles(Vector2 position, int32_t tileIndex);

static bool showGrid = false;

void MapInit(void){
  mapEditor.selectedTile = 0;
  mapEditor.camera.x = 0;
  mapEditor.camera.y = 0;
  mapEditor.tileSize = 8;
  mapEditor.showSelector = 1;

  for(int32_t y=0;y<MAPHEIGHT;y++){
    for(int32_t x=0;x<MAPWIDTH;x++){
      int32_t index = y * MAPWIDTH + x;
      mapData[index] = -1;
    }
  }
  mapData[0] = 0;
}

void MapUpdate(void){
  
  // clamp camera position
  //if(mapEditor.camera.x < 0)mapEditor.camera.x = 0;
  //if(mapEditor.camera.y < 0)mapEditor.camera.y = 0;

  UpdateSpriteSheet((Vector2){0,88});
}

void MapInput(void){
  float wheel = GetMouseWheelMove();

  TabInput(&mapEditor.showSelector);

  // pan the map 
  if(IsMouseButtonDown(MOUSE_MIDDLE_BUTTON)){
    mapEditor.camera.x += GetMouseDelta().x / 32;
    mapEditor.camera.y += GetMouseDelta().y / 32;
  }

  showGrid = false;
  if(IsKeyDown(KEY_SPACE)) showGrid = true;

  // mouse wheel to resize tileSize
  if(wheel > 0){
    mapEditor.tileSize *= 2;
  } else if(wheel < 0){
    mapEditor.tileSize /= 2;
  }

  // clamp tileSize
  if(mapEditor.tileSize < 4) mapEditor.tileSize = 4;
  if(mapEditor.tileSize > 8) mapEditor.tileSize = 8;
}

void MapDraw(void){
  // draw the editor 
  DrawMapEditor((Vector2){0,8});

  if(mapEditor.showSelector)
    DrawSpriteSelector((Vector2){0, 80});

  // draw UI top and bottom
  DrawRectangle(0, SCREENHEIGHT*SCREENSCALE - TILESIZE*SCREENSCALE, (SCREENWIDTH+8)*SCREENSCALE, TILESIZE*SCREENSCALE, GetNanoColor(0));
  DrawRectangle(0, 0, SCREENWIDTH*SCREENSCALE, TILESIZE*SCREENSCALE, GetNanoColor(0));
  DrawRectangleLines(0, 0, (SCREENWIDTH+1)*SCREENSCALE, TILESIZE*SCREENSCALE, GetNanoColor(8));
  DrawRectangleLines(0, SCREENHEIGHT*SCREENSCALE - TILESIZE*SCREENSCALE, (SCREENWIDTH+1)*SCREENSCALE, TILESIZE*SCREENSCALE+1, GetNanoColor(8));
  
  // map editor tabs/pages viewers
  DrawIcons(15, (Vector2){0,0}, 6 + (2 * mapEditor.showSelector)); 
  DrawIcons(16, (Vector2){8,0}, 6 + (2 * !mapEditor.showSelector));

  // TEXT UI 
  DrawSpriteIndex((Vector2){36, 0});
}

static int32_t viewPortWidth = 512;
static int32_t viewPortHeight = 256;
static void DrawMapEditor(Vector2 position){ 
  int32_t visibleRows = viewPortHeight / mapEditor.tileSize;
  int32_t visibleCols = viewPortWidth / mapEditor.tileSize;

  // draw grid and background
  DrawRectangle(0, 0, SCREENWIDTH*SCREENSCALE, SCREENHEIGHT*SCREENSCALE, GetNanoColor(2));
  for(int32_t my=0;my<visibleRows;my++){
    for(int32_t mx=0;mx<visibleCols;mx++){
      DrawRectangleLines((position.x + mx * mapEditor.tileSize)*SCREENSCALE, (position.y + my * mapEditor.tileSize)*SCREENSCALE, mapEditor.tileSize*SCREENSCALE, mapEditor.tileSize*SCREENSCALE, GetNanoColor(1));
    }
  }
  
  // draw the map
  for(int32_t my=0;my<visibleRows;my++){
    for(int32_t mx=0;mx<visibleCols;mx++){
      int32_t mapX = mx + mapEditor.camera.x;
      int32_t mapY = my + mapEditor.camera.y;
      int32_t index = mx * MAPWIDTH + my;
      int32_t getMapSprite = mapData[index];
      
      // draw grid
      if(showGrid)
        DrawRectangleLines((position.x + mx * mapEditor.tileSize)*SCREENSCALE, (position.y + my * mapEditor.tileSize)*SCREENSCALE, mapEditor.tileSize*SCREENSCALE, mapEditor.tileSize*SCREENSCALE, GetNanoColor(1));
      
      // draw the sprite 
      DrawRectangle((position.x + mapX * mapEditor.tileSize)*SCREENSCALE, (position.y + mapY * mapEditor.tileSize)*SCREENSCALE, mapEditor.tileSize*SCREENSCALE, mapEditor.tileSize*SCREENSCALE, GetNanoColor(0));
      
      if(getMapSprite >= 0)
        DrawMapTiles((Vector2){position.x + mapX * mapEditor.tileSize, position.y + mapY * mapEditor.tileSize}, getMapSprite);
    }
  } 
}

static void DrawSpriteSelector(Vector2 position){
  DrawRectangle(position.x*SCREENSCALE, position.y*SCREENSCALE, SCREENWIDTH*SCREENSCALE, TILESIZE*5*SCREENSCALE, GetNanoColor(0));
  DrawRectangleLines(position.x*SCREENSCALE, position.y*SCREENSCALE, (SCREENWIDTH+1)*SCREENSCALE, TILESIZE*5*SCREENSCALE, GetNanoColor(6));
  
  // DRAW SPRITESHEET
  DrawSpriteSheetTabs((Vector2){1, 80});
  DrawSpriteSheet((Vector2){0,88});
}

// draw the sprite
static void DrawMapTiles(Vector2 position, int32_t tileIndex){
  int32_t sx = tileIndex % (TILESIZE*2) * TILESIZE;
  int32_t sy = tileIndex / (TILESIZE*2) * TILESIZE;

  for(int32_t y=0;y<TILESIZE;y++){
    for(int32_t x=0;x<TILESIZE;x++){
      int32_t pixelX = sx + x;
      int32_t pixelY = sy + y;
      int32_t index = pixelY * SPRITEWIDTH + pixelX;
      
      // draw every pixel of the sprite
      if(sprites[index].colorIndex)
        DrawRectangle(
          (position.x+x) * SCREENSCALE,
          (position.y+y) * SCREENSCALE,
          SCREENSCALE, 
          SCREENSCALE, 
          GetNanoColor(sprites[index].colorIndex)
        ); 
    }
  }
}
