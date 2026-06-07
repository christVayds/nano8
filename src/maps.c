#include "maps.h"

#include <stdio.h>
#include <stdint.h>
#include "game.h"
#include "sprite.h"
#include "nanoUI.h"

int8_t mapData[MAPWIDTH*MAPHEIGHT];
static MapEditor mapEditor;

extern int8_t sprites[SPRITEWIDTH*SPRITEHEIGHT];
extern SpriteEditor spriteEditor;
extern Tools toolClicked;                     // current tool used                         (sprite.c)
extern SprUIClicked hoverUI;                  // what UI/element user are hovered in mouse (sprite.c)
extern int32_t hoveredIndex;                  // what type of UI are hovered               (sprite.c)
extern char labelname[32];                    // to show the label of the UI               (sprite.c)

static int32_t viewPortWidth = 512;
static int32_t viewPortHeight = 256;

// BUTTONS 
static NanoButton mapEditorPage;
static NanoButton mapFullPage;

// static functions for drawing and updates 
static void UpdateMapEditor(Vector2 position);
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

  // INITIALIZE BUTTONS 
  InitNanoButtonIcon(&mapEditorPage, (Rectangle){0, 0, TILESIZE, TILESIZE}, "Map Editor", 15, false);
  InitNanoButtonIcon(&mapFullPage, (Rectangle){TILESIZE, 0, TILESIZE, TILESIZE}, "Map full view", 16, false);
}

void MapUpdate(void){
  hoverUI = SPR_UI_NONE;
  hoveredIndex = -1;
  
  // update sprite selector only if showSelector is true 
  if(mapEditor.showSelector){
    UpdateSpriteSheet((Vector2){0,88});
    UpdateTools((Vector2){62, 80}, 6, false);
  }

  // update the map editor
  UpdateMapEditor((Vector2){0,8});

  // UPDATE BUTTONS 
  UpdateNanoButton(&mapEditorPage);
  UpdateNanoButton(&mapFullPage);

  if(IsMouseButtonPressed(0)){
    switch(hoverUI){
      case SPR_UI_TOOLS:
        toolClicked = (Tools)hoveredIndex;
        break;
      default:
        break;
    }
  }

  switch(hoverUI){
    case SPR_UI_TOOLS: 
      if(hoveredIndex >= 0 && hoveredIndex < TOOL_COUNT)
        switch(hoveredIndex){
          case TOOL_PEN:
            sprintf(labelname, "Pen");
            break;
          case TOOL_SELECT:
            sprintf(labelname, "Select - not working yet");
            break;
          case TOOL_PAN:
            sprintf(labelname, "Pan");
            break;
          case TOOL_FILL:
            sprintf(labelname, "Fill - not working yet");
            break;
          case TOOL_SHAPE:
            sprintf(labelname, "Shapes - nah");
            break;
        }
      break;
    default:
      sprintf(labelname, " ");
      break;
  }
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
  if(key >= '1' && key <= '5'){
    switch(key){
      case '1': toolClicked = TOOL_PEN; break;
      case '2': toolClicked = TOOL_SELECT; break;
      case '3': toolClicked = TOOL_PAN; break;
      case '4': toolClicked = TOOL_FILL; break;
      case '5': toolClicked = TOOL_SHAPE; break;
    }
  }

  // BUTTONS INPUT UPDATE 
  if(mapEditorPage.clicked) mapEditor.showSelector = true;
  if(mapFullPage.clicked) mapEditor.showSelector = false;
  mapEditorPage.active = mapEditor.showSelector;
  mapFullPage.active = !mapEditor.showSelector;
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
  DrawNanoButton(&mapEditorPage);
  DrawNanoButton(&mapFullPage);

  // TEXT UI 
  DrawSpriteIndex((Vector2){36, 0});
  DrawTextUI(labelname, (Vector2){1, SCREENHEIGHT-FONTHEIGHT-1});
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
      if(mousepos.x > screenX*SCREENSCALE && mousepos.x < screenX*SCREENSCALE + mapEditor.tileSize*SCREENSCALE && mousepos.y > screenY*SCREENSCALE && mousepos.y < screenY*SCREENSCALE + mapEditor.tileSize*SCREENSCALE && toolClicked == TOOL_PEN)
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

  if(IsMouseButtonDown(MOUSE_LEFT_BUTTON)){
    switch(toolClicked){
      case TOOL_PEN:{
        if(mouseTileX >= 0 && mouseTileX < MAPWIDTH && mouseTileY >= 0 && mouseTileX < MAPHEIGHT){
          if(mousepos.y > 80*SCREENSCALE && mapEditor.showSelector) return; 
          MSet(mouseTileX, mouseTileY, spriteEditor.activeSpriteIndex);
        }
        break;
      }
      case TOOL_PAN:
        mapEditor.camera.x -= GetMouseDelta().x * (mapEditor.tileSize / 16.0f) * 0.9f;
        mapEditor.camera.y -= GetMouseDelta().y * (mapEditor.tileSize / 16.0f) * 0.9f;
        break;
      case TOOL_FILL:
        // TODO: floodfill for maps
        //FloodFill(mapData, mouseTileX, mouseTileY, mapData[mouseTileY * MAPWIDTH + mouseTileX], spriteEditor.activeSpriteIndex);
        break;
      default:
        break;
    }
  }
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
