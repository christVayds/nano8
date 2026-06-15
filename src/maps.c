#include "maps.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include "game.h"
#include "sprite.h"
#include "nanoUI.h"

int8_t mapData[MAPWIDTH*MAPHEIGHT];
static MapEditor mapEditor;
// shape tool state (map editor)
static Vector2 mapShapeStart = {0,0};
static int32_t mapShaping = 0;
static int32_t mapShapeMode = 0; // 0=rect,1=circle,2=line

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
  // selection init
  mapEditor.selectionActive = 0;
  mapEditor.selX = mapEditor.selY = 0;
  mapEditor.selW = mapEditor.selH = 0;
  mapEditor.selBuffer = NULL;
  mapEditor.selectionMoving = 0;

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

  // if tool button is clicked
  if(IsMouseButtonPressed(0)){
    switch(hoverUI){
      case SPR_UI_TOOLS:
        toolClicked = (Tools)hoveredIndex;
        break;
      default:
        break;
    }
  }

  // update label
  switch(hoverUI){
    case SPR_UI_TOOLS: 
      if(hoveredIndex >= 0 && hoveredIndex < TOOL_COUNT)
        switch(hoveredIndex){
          case TOOL_PEN:
            sprintf(labelname, "Pen");
            break;
          case TOOL_SELECT:
            sprintf(labelname, "Select - bug");
            break;
          case TOOL_PAN:
            sprintf(labelname, "Pan");
            break;
          case TOOL_FILL:
            sprintf(labelname, "Fill");
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

  // Deleting selected tiles
  if(IsKeyPressed(KEY_DELETE) && mapEditor.selectionActive){
    for(int32_t yy=0; yy<mapEditor.selH; yy++){
      for(int32_t xx=0; xx<mapEditor.selW; xx++){
        int32_t sx = mapEditor.selX + xx;
        int32_t sy = mapEditor.selY + yy;
        if(sx < 0 || sx >= MAPWIDTH || sy < 0 || sy >= MAPHEIGHT) continue;
        mapData[sy * MAPWIDTH + sx] = -1;
      }
    }
    // clear selection
    mapEditor.selectionActive = 0;
    mapEditor.selW = mapEditor.selH = 0;
  }

  // Cancel moving selection with Escape: restore buffer back to original location
  if(mapEditor.selectionMoving && IsKeyPressed(KEY_ESCAPE)){
    // restore original into current selX/selY
    int32_t w = mapEditor.selW;
    int32_t h = mapEditor.selH;
    for(int32_t yy=0; yy<h; yy++){
      for(int32_t xx=0; xx<w; xx++){
        int32_t sx = mapEditor.selX + xx;
        int32_t sy = mapEditor.selY + yy;
        if(sx < 0 || sx >= MAPWIDTH || sy < 0 || sy >= MAPHEIGHT) continue;
        mapData[sy * MAPWIDTH + sx] = mapEditor.selBuffer[yy*w + xx];
      }
    }
    if(mapEditor.selBuffer) free(mapEditor.selBuffer);
    mapEditor.selBuffer = NULL;
    mapEditor.selectionMoving = 0;
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
  DrawNanoButton(&mapEditorPage);
  DrawNanoButton(&mapFullPage);

  // TEXT UI 
  DrawSpriteIndex((Vector2){36, 0});
  DrawTextUI(labelname, (Vector2){1, SCREENHEIGHT-FONTHEIGHT-1});
}

// DRAW MAP EDITOR
static void DrawMapEditor(Vector2 position){
  int32_t cameraTileX = (int32_t)floorf(mapEditor.camera.x / (float)mapEditor.tileSize);
  int32_t cameraTileY = (int32_t)floorf(mapEditor.camera.y / (float)mapEditor.tileSize);

  int32_t camXInt = (int32_t)floorf(mapEditor.camera.x);
  int32_t camYInt = (int32_t)floorf(mapEditor.camera.y);

  int32_t offsetX = camXInt - cameraTileX * mapEditor.tileSize;
  int32_t offsetY = camYInt - cameraTileY * mapEditor.tileSize;

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

  // draw selection rectangle if active and not currently moving (moving shows a preview instead)
  if(mapEditor.selectionActive && !mapEditor.selectionMoving){
    int32_t relX = mapEditor.selX - cameraTileX;
    int32_t relY = mapEditor.selY - cameraTileY;
    int32_t selScreenX = position.x + relX * mapEditor.tileSize - offsetX;
    int32_t selScreenY = position.y + relY * mapEditor.tileSize - offsetY;
    DrawRectangleLines(selScreenX*SCREENSCALE, selScreenY*SCREENSCALE, mapEditor.selW * mapEditor.tileSize * SCREENSCALE, mapEditor.selH * mapEditor.tileSize * SCREENSCALE, GetNanoColor(6));
  }

  // draw moving selection preview (if any)
  if(mapEditor.selectionMoving && mapEditor.selBuffer){
    // compute mouse target tile
    float mtx = (mousepos.x / SCREENSCALE - position.x + mapEditor.camera.x) / (float)mapEditor.tileSize;
    float mty = (mousepos.y / SCREENSCALE - position.y + mapEditor.camera.y) / (float)mapEditor.tileSize;
    int32_t mouseTileX = (int32_t)floorf(mtx);
    int32_t mouseTileY = (int32_t)floorf(mty);
    int32_t w = mapEditor.selW;
    int32_t h = mapEditor.selH;
    for(int32_t yy=0; yy<h; yy++){
      for(int32_t xx=0; xx<w; xx++){
        int32_t tile = mapEditor.selBuffer[yy*w + xx];
        if(tile < 0) continue;
        int32_t drawX = mouseTileX + xx - cameraTileX;
        int32_t drawY = mouseTileY + yy - cameraTileY;
        int32_t screenX = position.x + drawX * mapEditor.tileSize - offsetX;
        int32_t screenY = position.y + drawY * mapEditor.tileSize - offsetY;
        // draw the tile preview
        DrawRectangle(screenX*SCREENSCALE, screenY*SCREENSCALE, mapEditor.tileSize*SCREENSCALE, mapEditor.tileSize*SCREENSCALE, GetNanoColor(0));
        DrawMapTiles((Vector2){screenX, screenY}, tile);
      }
    }
    // draw preview outline
    int32_t previewScreenX = position.x + (mouseTileX - cameraTileX) * mapEditor.tileSize - offsetX;
    int32_t previewScreenY = position.y + (mouseTileY - cameraTileY) * mapEditor.tileSize - offsetY;
    DrawRectangleLines(previewScreenX*SCREENSCALE, previewScreenY*SCREENSCALE, w * mapEditor.tileSize * SCREENSCALE, h * mapEditor.tileSize * SCREENSCALE, GetNanoColor(7));
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

  float mtx = (mousepos.x / SCREENSCALE - position.x + mapEditor.camera.x) / (float)mapEditor.tileSize;
  float mty = (mousepos.y / SCREENSCALE - position.y + mapEditor.camera.y) / (float)mapEditor.tileSize;

  int32_t mouseTileX = (int32_t)floorf(mtx);
  int32_t mouseTileY = (int32_t)floorf(mty);

  // Start moving selection if left button pressed inside existing selection (pico-8 like)
  if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && mapEditor.selectionActive && !mapEditor.selectionMoving){
    // check if click is inside the selection rect
    if(mouseTileX >= mapEditor.selX && mouseTileX < mapEditor.selX + mapEditor.selW && mouseTileY >= mapEditor.selY && mouseTileY < mapEditor.selY + mapEditor.selH){
      int32_t w = mapEditor.selW;
      int32_t h = mapEditor.selH;
      // guard against empty selection
      if(w > 0 && h > 0){
        mapEditor.selBuffer = (int8_t*)malloc(sizeof(int8_t) * w * h);
        if(mapEditor.selBuffer){
          for(int32_t yy=0; yy<h; yy++){
            for(int32_t xx=0; xx<w; xx++){
              int32_t sx = mapEditor.selX + xx;
              int32_t sy = mapEditor.selY + yy;
              if(sx < 0 || sx >= MAPWIDTH || sy < 0 || sy >= MAPHEIGHT) mapEditor.selBuffer[yy*w + xx] = -1;
              else mapEditor.selBuffer[yy*w + xx] = mapData[sy * MAPWIDTH + sx];
              // clear source
              if(sx >= 0 && sx < MAPWIDTH && sy >= 0 && sy < MAPHEIGHT) mapData[sy * MAPWIDTH + sx] = -1;
            }
          }
          mapEditor.selectionMoving = 1;
        }
      }
    }
  }

    // cycle shape mode with 'S' key (rect, circle, line)
    if(IsKeyPressed(KEY_S)){
      mapShapeMode = (mapShapeMode + 1) % 3;
      switch(mapShapeMode){
        case 0: sprintf(labelname, "Shape: Rect"); break;
        case 1: sprintf(labelname, "Shape: Circle"); break;
        case 2: sprintf(labelname, "Shape: Line"); break;
      }
    }

  // Single click outside selection clears it (but clicking inside starts move)
  if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && mapEditor.selectionActive && !mapEditor.selectionMoving){
    if(!(mouseTileX >= mapEditor.selX && mouseTileX < mapEditor.selX + mapEditor.selW && mouseTileY >= mapEditor.selY && mouseTileY < mapEditor.selY + mapEditor.selH)){
      mapEditor.selectionActive = 0;
      mapEditor.selW = mapEditor.selH = 0;
      if(mapEditor.selBuffer){ free(mapEditor.selBuffer); mapEditor.selBuffer = NULL; }
      mapEditor.selectionMoving = 0;
    }
  }

  // update tools
  if(IsMouseButtonDown(MOUSE_LEFT_BUTTON)){
    const int32_t editorUIStartPosition = 80; 
    switch(toolClicked){
      default: break;
      case TOOL_PEN:{
        if(mouseTileX >= 0 && mouseTileX < MAPWIDTH && mouseTileY >= 0 && mouseTileY < MAPHEIGHT){
          if(mousepos.y > editorUIStartPosition*SCREENSCALE && mapEditor.showSelector) return; 
          MSet(mouseTileX, mouseTileY, spriteEditor.activeSpriteIndex);
        }
        break;
      }
      case TOOL_SELECT:
        // ignore clicks over sprite selector UI
        if(mousepos.y > editorUIStartPosition*SCREENSCALE && mapEditor.showSelector) return;
        
        // Start a new selection or expand existing one while dragging
        if(mouseTileX >= 0 && mouseTileX < MAPWIDTH && mouseTileY >= 0 && mouseTileY < MAPHEIGHT){ 
            // if we're currently moving a selection, ignore select-drag input
            if(mapEditor.selectionMoving) break;

            if(!mapEditor.selectionActive){
            mapEditor.selectionActive = 1;
            mapEditor.selX = mouseTileX;
            mapEditor.selY = mouseTileY;
            mapEditor.selW = 1;
            mapEditor.selH = 1;
          } else {
            int32_t minx = (mapEditor.selX < mouseTileX) ? mapEditor.selX : mouseTileX;
            int32_t miny = (mapEditor.selY < mouseTileY) ? mapEditor.selY : mouseTileY;
            int32_t maxx = (mapEditor.selX > mouseTileX) ? mapEditor.selX : mouseTileX;
            int32_t maxy = (mapEditor.selY > mouseTileY) ? mapEditor.selY : mouseTileY;
            mapEditor.selX = minx;
            mapEditor.selY = miny;
            mapEditor.selW = maxx - minx + 1;
            mapEditor.selH = maxy - miny + 1;
          }
        }
        break;
      case TOOL_SHAPE:
        // ignore clicks over sprite selector UI
        if(mousepos.y > editorUIStartPosition*SCREENSCALE && mapEditor.showSelector) return;
        if(!mapShaping){
          mapShapeStart.x = mouseTileX;
          mapShapeStart.y = mouseTileY;
          mapShaping = 1;
        }
        break;
      case TOOL_PAN:
        // regular pan camera (selection move is started by clicking inside selection)

        // ignore clicks over sprite selector UI
        if(mousepos.y > editorUIStartPosition*SCREENSCALE && mapEditor.showSelector) return;
        
        mapEditor.camera.x -= GetMouseDelta().x * (mapEditor.tileSize / 16.0f) * 0.9f;
        mapEditor.camera.y -= GetMouseDelta().y * (mapEditor.tileSize / 16.0f) * 0.9f;
        break;
      case TOOL_FILL:
        // If a selection is active, fill the selection rect; otherwise fill from a point

        // ignore clicks over sprite selector UI
        if(mousepos.y > editorUIStartPosition*SCREENSCALE && mapEditor.showSelector) return;

        if(mapEditor.selectionActive){
          for(int32_t yy=0; yy<mapEditor.selH; yy++){
            for(int32_t xx=0; xx<mapEditor.selW; xx++){
              int32_t sx = mapEditor.selX + xx;
              int32_t sy = mapEditor.selY + yy;
              if(sx < 0 || sx >= MAPWIDTH || sy < 0 || sy >= MAPHEIGHT) continue;
              mapData[sy * MAPWIDTH + sx] = spriteEditor.activeSpriteIndex;
            }
          }
        } else {
          // flood fill map from point
          if(mouseTileX >= 0 && mouseTileX < MAPWIDTH && mouseTileY >= 0 && mouseTileY < MAPHEIGHT){
            int32_t target = mapData[mouseTileY * MAPWIDTH + mouseTileX];
            int32_t replacement = spriteEditor.activeSpriteIndex;
            // call flood fill if tile is different
            if(target != replacement) {
              // iterative fill
              int cap = MAPWIDTH * MAPHEIGHT;
              Vector2 *stack = (Vector2*)malloc(sizeof(Vector2) * cap);
              if(stack){
                int top = 0;
                stack[top++] = (Vector2){(float)mouseTileX, (float)mouseTileY};
                while(top > 0){
                  Vector2 p = stack[--top];
                  int32_t px = (int32_t)p.x;
                  int32_t py = (int32_t)p.y;
                  if(px < 0 || px >= MAPWIDTH || py < 0 || py >= MAPHEIGHT) continue;
                  int idx = py * MAPWIDTH + px;
                  if(mapData[idx] != target) continue;
                  mapData[idx] = (int8_t)replacement;
                  // push neighbors (guard to avoid overflowing the stack buffer)
                  if(top + 4 < cap){
                    stack[top++] = (Vector2){(float)(px+1),(float)py};
                    stack[top++] = (Vector2){(float)(px-1),(float)py};
                    stack[top++] = (Vector2){(float)px,(float)(py+1)};
                    stack[top++] = (Vector2){(float)px,(float)(py-1)};
                  }
                }
                free(stack);
              }
            }
          }
        }
        break;
      
  
  // handle selection move drop on mouse release
  if(mapEditor.selectionMoving && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)){
    // compute drop target tile under mouse
    if(mouseTileX >= 0 && mouseTileX < MAPWIDTH && mouseTileY >= 0 && mouseTileY < MAPHEIGHT){
      int32_t w = mapEditor.selW;
      int32_t h = mapEditor.selH;
      for(int32_t yy=0; yy<h; yy++){
        for(int32_t xx=0; xx<w; xx++){
          int32_t dx = mouseTileX + xx;
          int32_t dy = mouseTileY + yy;
          if(dx < 0 || dx >= MAPWIDTH || dy < 0 || dy >= MAPHEIGHT) continue;
          mapData[dy * MAPWIDTH + dx] = mapEditor.selBuffer[yy*w + xx];
        }
      }
      // update selection position
      mapEditor.selX = mouseTileX;
      mapEditor.selY = mouseTileY;
    }
    // free buffer
    if(mapEditor.selBuffer) free(mapEditor.selBuffer);
    mapEditor.selBuffer = NULL;
    mapEditor.selectionMoving = 0;
  }
  // handle shape finalize on mouse release
  if(mapShaping && IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && toolClicked == TOOL_SHAPE){
    int32_t x0 = (int32_t)mapShapeStart.x;
    int32_t y0 = (int32_t)mapShapeStart.y;
    int32_t x1 = mouseTileX;
    int32_t y1 = mouseTileY;
    int32_t minx = (x0 < x1) ? x0 : x1;
    int32_t miny = (y0 < y1) ? y0 : y1;
    int32_t maxx = (x0 > x1) ? x0 : x1;
    int32_t maxy = (y0 > y1) ? y0 : y1;
    int8_t tile = (int8_t)spriteEditor.activeSpriteIndex;

    if(mapShapeMode == 0){
      // rect fill
      for(int32_t yy = miny; yy <= maxy; yy++){
        for(int32_t xx = minx; xx <= maxx; xx++){
          if(xx < 0 || xx >= MAPWIDTH || yy < 0 || yy >= MAPHEIGHT) continue;
          mapData[yy * MAPWIDTH + xx] = tile;
        }
      }
    } else if(mapShapeMode == 1){
      // circle fill within bounding box
      float cx = (minx + maxx) * 0.5f;
      float cy = (miny + maxy) * 0.5f;
      float rx = (maxx - minx) * 0.5f;
      float ry = (maxy - miny) * 0.5f;
      float r = (rx > ry) ? rx : ry;
      float rr = r * r;
      for(int32_t yy = miny; yy <= maxy; yy++){
        for(int32_t xx = minx; xx <= maxx; xx++){
          float dx = xx - cx;
          float dy = yy - cy;
          if(dx*dx + dy*dy <= rr){
            if(xx < 0 || xx >= MAPWIDTH || yy < 0 || yy >= MAPHEIGHT) continue;
            mapData[yy * MAPWIDTH + xx] = tile;
          }
        }
      }
    } else {
      // line (Bresenham)
      int32_t dx = abs(x1 - x0);
      int32_t sxl = x0 < x1 ? 1 : -1;
      int32_t dy = -abs(y1 - y0);
      int32_t syl = y0 < y1 ? 1 : -1;
      int32_t err = dx + dy;
      int32_t cx0 = x0;
      int32_t cy0 = y0;
      while(1){
        if(cx0 >= 0 && cx0 < MAPWIDTH && cy0 >= 0 && cy0 < MAPHEIGHT)
          mapData[cy0 * MAPWIDTH + cx0] = tile;
        if(cx0 == x1 && cy0 == y1) break;
        int32_t e2 = 2*err;
        if(e2 >= dy){ err += dy; cx0 += sxl; }
        if(e2 <= dx){ err += dx; cy0 += syl; }
      }
    }
    mapShaping = 0;
  }
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
