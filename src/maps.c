#include "maps.h"

#include <stdio.h>
#include <stdint.h>
#include "game.h"
#include "sprite.h"

int32_t mapData[MAPWIDTH*MAPHEIGHT];
static MapEditor mapEditor;

extern int8_t sprites[SPRITEWIDTH*SPRITEHEIGHT];
extern SpriteEditor spriteEditor;
extern Tools toolClicked;                 // current tool used 

static int32_t viewPortWidth = 512;
static int32_t viewPortHeight = 256;

// static functions for drawing and updates 
static void UpdateMapEditor(Vector2 position);
static void DrawMapEditor(Vector2 position);
static void DrawSpriteSelector(Vector2 position);
static void DrawMapTiles(Vector2 position, int32_t tileIndex);
static void MapSet(int32_t mapIndex, int32_t spriteIndex);

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
}

void MapUpdate(void){ 
  
  // update sprite selector only if showSelector is true 
  if(mapEditor.showSelector){
    UpdateSpriteSheet((Vector2){0,88});
    UpdateTools((Vector2){62, 80}, 6, false);
  }

  // update the map editor
  UpdateMapEditor((Vector2){0,8});
}

void MapInput(void){
  float wheel = GetMouseWheelMove();

  TabInput(&mapEditor.showSelector);

  // pan the map 
  if(IsMouseButtonDown(MOUSE_MIDDLE_BUTTON)){
    mapEditor.camera.x -= GetMouseDelta().x * (mapEditor.tileSize / 16.0f) * 0.9f;
    mapEditor.camera.y -= GetMouseDelta().y * (mapEditor.tileSize / 16.0f) * 0.9f;
  }

  showGrid = false;
  if(IsKeyDown(KEY_SPACE)) showGrid = true;

  // mouse wheel to resize tileSize
  if(wheel > 0){
    mapEditor.tileSize /= 2;
  } else if(wheel < 0){
    mapEditor.tileSize *= 2;
  } 

  // clamp tileSize
  if(mapEditor.tileSize < 2) mapEditor.tileSize = 2;
  if(mapEditor.tileSize > 8) mapEditor.tileSize = 8;

  // TOOL INPUT 
  int32_t key = GetKeyPressed();
  if(key >= '1' && key <= '5')
    switch(key){
      case '1': toolClicked = TOOL_PEN; break;
      case '2': toolClicked = TOOL_SELECT; break;
      case '3': toolClicked = TOOL_PAN; break;
      case '4': toolClicked = TOOL_FILL; break;
      case '5': toolClicked = TOOL_SHAPE; break;
    }
}

void MapDraw(void){
  // draw the editor 
  DrawMapEditor((Vector2){0,8});

  // show the sprite selector and other UIs
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

// DRAW MAP EDITOR
static void DrawMapEditor(Vector2 position){
  int32_t cameraTileX = (int)mapEditor.camera.x / mapEditor.tileSize;
  int32_t cameraTileY = (int)mapEditor.camera.y / mapEditor.tileSize;

  int32_t offsetX = (int)mapEditor.camera.x % mapEditor.tileSize;
  int32_t offsetY = (int)mapEditor.camera.y % mapEditor.tileSize;

  int32_t visibleRows = viewPortHeight / mapEditor.tileSize;
  int32_t visibleCols = viewPortWidth / mapEditor.tileSize;

  // draw grid and background
  DrawRectangle(0, 0, SCREENWIDTH*SCREENSCALE, SCREENHEIGHT*SCREENSCALE, GetNanoColor(2)); 
  
  // draw the map
  Vector2 mousepos = GetMousePosition();
  for(int32_t my=0;my<visibleRows;my++){
    for(int32_t mx=0;mx<visibleCols;mx++){
      int32_t mapX = mx + cameraTileX;
      int32_t mapY = my + cameraTileY;
      if(mapX < 0 || mapX >= MAPWIDTH || mapY < 0 || mapY >= MAPHEIGHT) continue;

      int32_t index = mapY * MAPWIDTH + mapX;
      int32_t getMapSprite = mapData[index];

      int32_t screenX = position.x + mx * mapEditor.tileSize - offsetX;
      int32_t screenY = position.y + my * mapEditor.tileSize - offsetY;

      // draw grid
      if(showGrid)
        DrawRectangleLines(screenX*SCREENSCALE, screenY*SCREENSCALE, mapEditor.tileSize*SCREENSCALE, mapEditor.tileSize*SCREENSCALE, GetNanoColor(1));
      
      // draw the sprite 
      DrawRectangle(
        screenX*SCREENSCALE, 
        screenY*SCREENSCALE, 
        mapEditor.tileSize*SCREENSCALE, 
        mapEditor.tileSize*SCREENSCALE, 
        GetNanoColor(0)
      );
      
      if(getMapSprite >= 0)
        DrawMapTiles((Vector2){screenX, screenY}, getMapSprite);
     
      // hovered tile
      if(mousepos.x > screenX*SCREENSCALE && mousepos.x < screenX*SCREENSCALE + mapEditor.tileSize*SCREENSCALE && mousepos.y > screenY*SCREENSCALE && mousepos.y < screenY*SCREENSCALE + mapEditor.tileSize*SCREENSCALE)
      DrawRectangleLines(
        screenX * SCREENSCALE,
        screenY * SCREENSCALE,
        mapEditor.tileSize * SCREENSCALE,
        mapEditor.tileSize * SCREENSCALE,
        GetNanoColor(8)
      );
    }
  }
} 

static void DrawSpriteSelector(Vector2 position){
  DrawRectangle(position.x*SCREENSCALE, position.y*SCREENSCALE, SCREENWIDTH*SCREENSCALE, TILESIZE*5*SCREENSCALE, GetNanoColor(0));
  DrawRectangleLines(position.x*SCREENSCALE, position.y*SCREENSCALE, (SCREENWIDTH+1)*SCREENSCALE, TILESIZE*5*SCREENSCALE, GetNanoColor(6));
  
  // DRAW SPRITESHEET
  DrawSpriteSheetTabs((Vector2){1, 80});
  DrawSpriteSheet((Vector2){0,88});

  DrawTools((Vector2){62, 80}, 6, false);
}

// Update map editor 
static void UpdateMapEditor(Vector2 position){
  Vector2 mousepos = GetMousePosition();

  int32_t mouseTileX = (mousepos.x / SCREENSCALE - position.x + mapEditor.camera.x) / mapEditor.tileSize;
  int32_t mouseTileY = (mousepos.y / SCREENSCALE - position.y + mapEditor.camera.y) / mapEditor.tileSize;
  int32_t index = mouseTileY * MAPWIDTH + mouseTileX;

  //printf("index %d %d %d\n", index, mouseTileX, mouseTileY);
  if(IsMouseButtonDown(MOUSE_LEFT_BUTTON)){ 
    if(mouseTileX >= 0 && mouseTileX < MAPWIDTH && mouseTileY >= 0 && mouseTileX < MAPHEIGHT){
      if(mousepos.y > 80*SCREENSCALE && mapEditor.showSelector) return; 
      MapSet(index, spriteEditor.activeSpriteIndex);
    }
  }

  //printf("x %d y %d index %d\n", mouseTileX, mouseTileY, index);
}

// CHECK THIS LATER
static void MapSet(int32_t mapIndex, int32_t spriteIndex){
  mapData[mapIndex] = spriteIndex;
}

// draw the sprite
static void DrawMapTiles(Vector2 position, int32_t tileIndex){
  int32_t sx = tileIndex % (TILESIZE*2) * TILESIZE;
  int32_t sy = tileIndex / (TILESIZE*2) * TILESIZE;
  float pSize = (float)mapEditor.tileSize / (float)TILESIZE;

  for(int32_t y=0;y<TILESIZE;y++){
    for(int32_t x=0;x<TILESIZE;x++){
      int32_t pixelX = sx + x;
      int32_t pixelY = sy + y;
      int32_t index = pixelY * SPRITEWIDTH + pixelX;
      
      // draw every pixel of the sprite
      if(sprites[index])
        DrawRectangle(
          (position.x+x*pSize) * SCREENSCALE,
          (position.y+y*pSize) * SCREENSCALE,
          pSize*SCREENSCALE, 
          pSize*SCREENSCALE, 
          GetNanoColor(sprites[index])
        ); 
    }
  }
}
