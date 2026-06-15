#include "sprite.h"
#include "game.h"
#include "nanoUI.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

// SPRITE
int8_t sprites[SPRITEWIDTH*SPRITEHEIGHT];
SpriteEditor spriteEditor;
SpriteFlags spriteFlags[16*16];

// pixel selection 
PixelSelection pixelSelection;

static int32_t currentPage = 0; // spritesheet page
// editor state
static Vector2 spritePan = {0,0};
static Vector2 shapeStart = {0,0};
static int32_t shaping = 0;
static int32_t shapeMode = 0; // 0=rect,1=circle,2=line
// clipboard for selection
typedef struct { int32_t w,h; int8_t *data; } Clipboard;
static Clipboard clipboard = {0,0,NULL};

// Undo/Redo stack
static const int32_t UNDO_CAPACITY = 64;
static int8_t **undoStack = NULL;    // array of pointers to snapshots
static int32_t undoCount = 0;        // number of snapshots stored
static int32_t undoIndex = -1;       // current index in stack (0..undoCount-1)
static int32_t mouseDownInEditor = 0; // track press/release to push snapshots

static void UndoInit(void);
static void UndoFree(void);
static void PushSnapshot(void);
static void DoUndo(void);
static void DoRedo(void);

// update and draw function for UIs
static void UpdateSpriteEditor(Vector2 position);
static void DrawSpriteEditor(Vector2 position, int32_t size);
static void UpdateColorPallete(Vector2 position);
static void DrawColorPallete(Vector2 position);
static void UpdateSwitch(Vector2 position, SprUIClicked switchUIType);
static void DrawSwitch(Vector2 position, int32_t index); 
static void UpdateSpriteFlags(Vector2 position);
static void DrawSpriteFlags(Vector2 position);
static void DrawSpriteLabel(Vector2 position);
static void GetMouseClick(Vector2 position, int32_t width, int32_t height, SprUIClicked sprUI, int32_t index);
static void CopyToClipboard(int32_t rect_sx, int32_t rect_sy, int32_t rect_w, int32_t rect_h);
static void PasteFromClipboard(int32_t dst_sx, int32_t dst_sy);

Tools toolClicked;                 // current tool used 
static int32_t colorIndex = 7;
static int32_t zoomValue = 0;
static int32_t pensilSize = 0;
static int32_t showWholeSprite = false;

SprUIClicked hoverUI = SPR_UI_NONE;    // what UI/element user are hovered in mouse 
int32_t hoveredIndex = -1;             // what type of UI are hovered 
char labelname[32];                    // to show the label of the UI

// BUTTONS 
static NanoButton spriteEditorPage;
static NanoButton spriteViewerPage;

int8_t *GetSprite(void){
  return sprites; 
}

void SpriteInit(void){
  // intitialize sprites 
  for(int32_t i=0;i<SPRITEWIDTH*SPRITEHEIGHT;i++){
    // push new sprite to sprites
    sprites[i] = 0;
  }

  // SPRITE FLAGS and INDEX 
  for(int32_t i=0;i<16*16;i++){
    SpriteFlags spFlag = {
      .spriteIndex = i
    };

    // arrays of int len=7, default value = 0s
    spFlag.flags = calloc(FLAGCOUNT, sizeof(int32_t));

    spriteFlags[i] = spFlag;
  }

  spriteEditor.zoom = 1;                    // zoom amount(SPRITE TILE SIZE) [N+N] (1, 2, 4, 8)
  spriteEditor.position = (Vector2){0,0};   // sprite position in spritesheet 
  spriteEditor.activeSpriteIndex = 0;       // sprite index 
  
  // sprite pixels selection 
  pixelSelection.active = false;
  pixelSelection.position = (Vector2){0,0};
  pixelSelection.size = (Vector2){0,0};
  toolClicked = TOOL_PEN;
  spritePan = (Vector2){0,0};
  shaping = 0;
  shapeMode = 0;

  // initialize undo/redo system
  UndoInit();

  // INITIALIZE BUTTONS 
  InitNanoButtonIcon(&spriteEditorPage, (Rectangle){0, 0, TILESIZE, TILESIZE}, "Sprite Editor", 15, false);
  InitNanoButtonIcon(&spriteViewerPage, (Rectangle){TILESIZE, 0, TILESIZE, TILESIZE}, "Sprite Viewer", 16, false);
}

void SpriteFree(void){
  for(int32_t i=0;i<SPRITEWIDTH*SPRITEHEIGHT;i++){
    sprites[i] = 0;
  }

  for(int32_t i=0;i<16*16;i++){
    free(spriteFlags[i].flags);
  }

  if(clipboard.data) free(clipboard.data);

  // free undo/redo stack
  UndoFree();
}

void SpriteUpdate(void){
  hoverUI = SPR_UI_NONE;
  hoveredIndex = -1;

  UpdateSpriteEditor((Vector2){16, 12});
  UpdateTools((Vector2){4, 12}, 6, true);
  UpdateColorPallete((Vector2){85, 37});
  UpdateSwitch((Vector2){100, 20}, SPR_UI_PENSIZE);           // pen size update 
  UpdateSwitch((Vector2){100, 30}, SPR_UI_ZOOMSIZE);          // zoom size update
  UpdateSpriteFlags((Vector2){88, 12});
  UpdateSpriteSheet((Vector2){0,88});

  // UPDATE BUTTONS
  UpdateNanoButton(&spriteEditorPage);
  UpdateNanoButton(&spriteViewerPage);

  // click buttons
  if(IsMouseButtonPressed(0)){
    if(toolClicked == TOOL_SELECT){
      if(hoverUI == SPR_UI_EDITOR){
        pixelSelection.active = false;
        pixelSelection.size = (Vector2){0,0};
      }
    }
    switch(hoverUI){ 
      case SPR_UI_TOOLS: 
        if(hoveredIndex == TOOL_SHAPE && toolClicked == TOOL_SHAPE){
          if(toolClicked == TOOL_SHAPE){ 
            // cycle shape mode
            shapeMode = (shapeMode + 1) % 3;
            switch(shapeMode){
              case 0: sprintf(labelname, "Shape: Rectangle"); break;
              case 1: sprintf(labelname, "Shape: Circle"); break;
              case 2: sprintf(labelname, "Shape: Line"); break;
            } 
          }
        } else {
          toolClicked = (Tools)hoveredIndex;
        }
        break;
      case SPR_UI_COLOR:
        colorIndex = hoveredIndex;
        break;
      case SPR_UI_ZOOMSIZE:
        zoomValue = hoveredIndex;
        break;
      case SPR_UI_PENSIZE:
        pensilSize = hoveredIndex;
        break;
      case SPR_UI_FLAGS:
        // apply the flag to the sprite
        spriteFlags[spriteEditor.activeSpriteIndex].flags[hoveredIndex] = (spriteFlags[spriteEditor.activeSpriteIndex].flags[hoveredIndex]) ? 0 : 1; 
        break;
      default:
        break;
    }
  }

  // Update label names
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
            sprintf(labelname, "Pan - not working yet");
            break;
          case TOOL_FILL:
            sprintf(labelname, "Fill");
            break;
          case TOOL_SHAPE:
            if(shapeMode == 0) sprintf(labelname, "Shape: Rect");
            else if(shapeMode == 1) sprintf(labelname, "Shape: Circle");
            else sprintf(labelname, "Shape: Line");
            break;
        }
      break;
    case SPR_UI_COLOR:
      if(hoveredIndex >= 0 && hoveredIndex < COLORCOUNT)
        sprintf(labelname, "Color %d", hoveredIndex);
      break;
    case SPR_UI_PENSIZE:
      if(hoveredIndex >= 0 && hoveredIndex < COLORCOUNT)
        sprintf(labelname, "Pen size %d", hoveredIndex);
      break;
    case SPR_UI_ZOOMSIZE:
      if(hoveredIndex >= 0 && hoveredIndex < COLORCOUNT)
        sprintf(labelname, "Zoom size %d", hoveredIndex);
      break;
    case SPR_UI_FLAGS:
      if(hoveredIndex >= 0 && hoveredIndex < COLORCOUNT)
        sprintf(labelname, "Flag %d = %s", hoveredIndex, (spriteFlags[spriteEditor.activeSpriteIndex].flags[hoveredIndex]) ? "On" : "Off");
      break;
    case SPR_UI_EDITOR:
      if(hoveredIndex >= 0 && hoveredIndex < COLORCOUNT)
        sprintf(labelname, "X: %d Y: %d", (int32_t)spriteEditor.position.x, (int32_t)spriteEditor.position.y);
      break;
    default:
      sprintf(labelname, " ");
      break;
  }
}

// FOR TAB KEY INPUT (USED IN SPRITE AND MAP EDITOR)
void TabInput(int32_t *showTab){
  if(IsKeyDown(KEY_RIGHT_SHIFT) && IsKeyPressed(KEY_TAB)){
    currentPage++;
    if(currentPage > 3) currentPage = 0;
    return;
  }
  if(IsKeyPressed(KEY_TAB)) *showTab = !(*showTab);
}

// HANDLE INPUT FOR SPRITE EDITOR
void SpriteInput(void){
  TabInput(&showWholeSprite);

  // click inside sprite editor canvas removes active selection
  if(IsMouseButtonPressed(0) && pixelSelection.active && hoverUI == SPR_UI_EDITOR){
    pixelSelection.active = false;
  }

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
  if(spriteEditorPage.clicked) showWholeSprite = false;
  if(spriteViewerPage.clicked) showWholeSprite = true;
  spriteEditorPage.active = !showWholeSprite;
  spriteViewerPage.active = showWholeSprite;

  // Clipboard shortcuts: Ctrl+C, Ctrl+X, Ctrl+V
  if((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL))){
    // Undo/Redo: Ctrl+Z = undo, Ctrl+Shift+Z = redo
    if(IsKeyPressed(KEY_Z)){
      if(IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) DoRedo();
      else DoUndo();
    }
    if(IsKeyPressed(KEY_C)){
      if(pixelSelection.active){
        int32_t sx = (spriteEditor.activeSpriteIndex % (TILESIZE*2)) * TILESIZE;
        int32_t sy = (spriteEditor.activeSpriteIndex / (TILESIZE*2)) * TILESIZE;
        int32_t rect_x = sx + (int32_t)pixelSelection.position.x + (int32_t)spritePan.x;
        int32_t rect_y = sy + (int32_t)pixelSelection.position.y + (int32_t)spritePan.y;
        CopyToClipboard(rect_x, rect_y, (int32_t)pixelSelection.size.x, (int32_t)pixelSelection.size.y);
      }
    }
    if(IsKeyPressed(KEY_X)){
      if(pixelSelection.active){
        // snapshot before cut
        PushSnapshot();
        int32_t sx = (spriteEditor.activeSpriteIndex % (TILESIZE*2)) * TILESIZE;
        int32_t sy = (spriteEditor.activeSpriteIndex / (TILESIZE*2)) * TILESIZE;
        int32_t rect_x = sx + (int32_t)pixelSelection.position.x + (int32_t)spritePan.x;
        int32_t rect_y = sy + (int32_t)pixelSelection.position.y + (int32_t)spritePan.y;
        CopyToClipboard(rect_x, rect_y, (int32_t)pixelSelection.size.x, (int32_t)pixelSelection.size.y);
        // clear selection
        for(int j=0;j<(int)pixelSelection.size.y;j++){
          for(int i=0;i<(int)pixelSelection.size.x;i++){
            int32_t px = rect_x + i;
            int32_t py = rect_y + j;
            if(px < 0 || px >= SPRITEWIDTH || py < 0 || py >= SPRITEHEIGHT) continue;
            sprites[py * SPRITEWIDTH + px] = 0;
          }
        }
        // snapshot after cut
        PushSnapshot();
      }
    }
    if(IsKeyPressed(KEY_V)){
      // paste at current spriteEditor position (mouse hover position)
      if(clipboard.data){
        int32_t dstx = (int32_t)spriteEditor.position.x;
        int32_t dsty = (int32_t)spriteEditor.position.y;
        // snapshot before paste
        PushSnapshot();
        PasteFromClipboard(dstx, dsty);
        // snapshot after paste
        PushSnapshot();
      }
    }
  }
}

// DRAW SPRITE EDITOR PAGE
void SpriteDraw(void){
  // Draw spritesheets
  if(!showWholeSprite){
    DrawSpriteSheetTabs((Vector2){1, 80});
    DrawSpriteSheet((Vector2){0,88}); 
    
    // SPRITE EDITOR UI 
    DrawSpriteEditor((Vector2){16, 12}, 64);
    DrawColorPallete((Vector2){85, 37});
    DrawTools((Vector2){4, 12}, 6, true);
    DrawSwitch((Vector2){100, 20}, pensilSize);         // pen size 
    DrawSwitch((Vector2){100, 30}, zoomValue);         // zoom size  
    DrawSpriteFlags((Vector2){88, 12});
  }

  // draw UI top and bottom
  DrawRectangle(0, SCREENHEIGHT*SCREENSCALE - TILESIZE*SCREENSCALE, (SCREENWIDTH+8)*SCREENSCALE, TILESIZE*SCREENSCALE, GetNanoColor(0));
  DrawRectangle(0, 0, SCREENWIDTH*SCREENSCALE, TILESIZE*SCREENSCALE, GetNanoColor(0));
  DrawRectangleLines(0, 0, (SCREENWIDTH+1)*SCREENSCALE, TILESIZE*SCREENSCALE, GetNanoColor(8));
  DrawRectangleLines(0, SCREENHEIGHT*SCREENSCALE - TILESIZE*SCREENSCALE, (SCREENWIDTH+1)*SCREENSCALE, TILESIZE*SCREENSCALE+1, GetNanoColor(8));

  // sprite editor tabs/pages viewers
  DrawNanoButton(&spriteEditorPage);
  DrawNanoButton(&spriteViewerPage);

  // Text UI
  DrawSpriteIndex((Vector2){36, 0});
  DrawSpriteLabel((Vector2){1, SCREENHEIGHT-FONTHEIGHT-1});
}

static void UpdateSpriteEditor(Vector2 position){
  // Register clicks for the editor area so hoverUI gets set correctly.
  GetMouseClick(position, 64, 64, SPR_UI_EDITOR, 0);

  // If the mouse isn't over the editor, nothing to do here.
  if(hoverUI != SPR_UI_EDITOR) return;

  // gridSize = how many screen pixels represent one sprite pixel
  // spriteSize = number of sprite pixels along one axis visible in editor
  int32_t gridSize = TILESIZE / spriteEditor.zoom;
  int32_t spriteSize = TILESIZE * spriteEditor.zoom;

  // Absolute top-left of the active sprite within the full spritesheet
  int32_t sx = (spriteEditor.activeSpriteIndex % (TILESIZE*2)) * TILESIZE;
  int32_t sy = (spriteEditor.activeSpriteIndex / (TILESIZE*2)) * TILESIZE;

  // Convert mouse position into local editor grid coordinates (0..spriteSize-1)
  Vector2 mousePos = GetMousePosition();
  int32_t localX = (mousePos.x - position.x * SCREENSCALE) / (gridSize * SCREENSCALE);
  int32_t localY = (mousePos.y - position.y * SCREENSCALE) / (gridSize * SCREENSCALE);

  // If the mouse press begins inside the editor, capture a pre-change snapshot
  if(IsMouseButtonPressed(0)){
    PushSnapshot();
    mouseDownInEditor = 1;
  }

  // If mouse is outside the editor grid, early out.
  if(localX < 0 || localX >= spriteSize || localY < 0 || localY >= spriteSize) return;

  // Compute the pixel coordinates inside the full spritesheet (absolute pixel indices)
  int32_t pixelX = sx + localX + (int32_t)spritePan.x;
  int32_t pixelY = sy + localY + (int32_t)spritePan.y;

  // Validate computed pixel coordinates
  if(pixelX < 0 || pixelX >= SPRITEWIDTH || pixelY < 0 || pixelY >= SPRITEHEIGHT) return;
  int32_t index = pixelY * SPRITEWIDTH + pixelX;

  // Update the editor's cursor/selection absolute position
  spriteEditor.position.x = pixelX;
  spriteEditor.position.y = pixelY;

  // Handle mouse hold actions (continuous) and mouse release actions
  if(IsMouseButtonDown(0)){
    switch(toolClicked){
      case TOOL_PEN: {
        // Draw a square brush of size (pensilSize+1) anchored at current pixel
        for(int32_t peny = 0; peny < pensilSize + 1; peny++){
          for(int32_t penx = 0; penx < pensilSize + 1; penx++){
            int32_t px = pixelX + penx;
            int32_t py = pixelY + peny;
            if(px < 0 || px >= SPRITEWIDTH || py < 0 || py >= SPRITEHEIGHT) continue;
            // If a selection is active, restrict painting to inside that selection
            if(pixelSelection.active){
              int32_t localPx = (px - sx) - (int32_t)spritePan.x; // local coords within sprite editor
              int32_t localPy = (py - sy) - (int32_t)spritePan.y;
              if(localPx < (int32_t)pixelSelection.position.x || localPx >= (int32_t)pixelSelection.position.x + (int32_t)pixelSelection.size.x) continue;
              if(localPy < (int32_t)pixelSelection.position.y || localPy >= (int32_t)pixelSelection.position.y + (int32_t)pixelSelection.size.y) continue;
            }
            sprites[py * SPRITEWIDTH + px] = colorIndex;
          }
        }
        break;
      }
      case TOOL_SELECT:
        // Start/expand a pixel selection in local editor grid coordinates
        if(!pixelSelection.active){
          pixelSelection.position.x = localX;
          pixelSelection.position.y = localY;
          pixelSelection.size.x = 1;
          pixelSelection.size.y = 1;
          pixelSelection.active = true;
        } else {
          int32_t startX = (int32_t)pixelSelection.position.x;
          int32_t startY = (int32_t)pixelSelection.position.y;
          int32_t minx = (startX < localX) ? startX : localX;
          int32_t miny = (startY < localY) ? startY : localY;
          int32_t maxx = (startX > localX) ? startX : localX;
          int32_t maxy = (startY > localY) ? startY : localY;
          pixelSelection.position.x = minx;
          pixelSelection.position.y = miny;
          pixelSelection.size.x = maxx - minx + 1;
          pixelSelection.size.y = maxy - miny + 1;
        }
        break;
      case TOOL_PAN: {
        // Move the viewport by mouse delta (converted to sprite-pixel units)
        Vector2 md = GetMouseDelta();
        float dx = roundf(md.x / (gridSize * SCREENSCALE));
        float dy = roundf(md.y / (gridSize * SCREENSCALE));
        spritePan.x -= (int32_t)dx;
        spritePan.y -= (int32_t)dy;
        // clamp pan inside sprite sheet so we don't pan outside available data
        int32_t minPanX = -sx;
        int32_t maxPanX = SPRITEWIDTH - sx - spriteSize;
        int32_t minPanY = -sy;
        int32_t maxPanY = SPRITEHEIGHT - sy - spriteSize;
        if(spritePan.x < minPanX) spritePan.x = minPanX;
        if(spritePan.x > maxPanX) spritePan.x = maxPanX;
        if(spritePan.y < minPanY) spritePan.y = minPanY;
        if(spritePan.y > maxPanY) spritePan.y = maxPanY;
        break;
      }
      case TOOL_FILL:
        // Fill either inside selection rectangle or flood fill from point
        if(pixelSelection.active){
          int32_t sel_abs_x = sx + (int32_t)pixelSelection.position.x + (int32_t)spritePan.x;
          int32_t sel_abs_y = sy + (int32_t)pixelSelection.position.y + (int32_t)spritePan.y;
          FloodFillRect(sprites, sel_abs_x, sel_abs_y, pixelX, pixelY, sprites[index], colorIndex, (int32_t)pixelSelection.size.x, (int32_t)pixelSelection.size.y);
        } else {
          FloodFill(sprites, sx, sy, pixelX, pixelY, sprites[index], colorIndex, spriteSize);
        }
        break;
      case TOOL_SHAPE:
        // begin shape drawing - store absolute start pixel
        if(!shaping){
          shapeStart.x = pixelX;
          shapeStart.y = pixelY;
          shaping = 1;
        }
        break;
      default:
        break;
    }
  }

  if(IsMouseButtonReleased(0)){
    switch(toolClicked){
      case TOOL_SELECT:
        // keep selection active (nothing special to do on release)
        break;
      case TOOL_SHAPE:
        if(shaping){
          int32_t x0 = (int32_t)shapeStart.x;
          int32_t y0 = (int32_t)shapeStart.y;
          int32_t x1 = pixelX;
          int32_t y1 = pixelY;
          int32_t minx = (x0 < x1) ? x0 : x1;
          int32_t miny = (y0 < y1) ? y0 : y1;
          int32_t maxx = (x0 > x1) ? x0 : x1;
          int32_t maxy = (y0 > y1) ? y0 : y1;
          if(shapeMode == 0){
            // rect fill
            for(int32_t py = miny; py <= maxy; py++){
              for(int32_t px = minx; px <= maxx; px++){
                if(px < 0 || px >= SPRITEWIDTH || py < 0 || py >= SPRITEHEIGHT) continue;
                sprites[py * SPRITEWIDTH + px] = colorIndex;
              }
            }
          } else if(shapeMode == 1){
            // circle fill within bounding box
            float cx = (minx + maxx) * 0.5f;
            float cy = (miny + maxy) * 0.5f;
            float rx = (maxx - minx) * 0.5f;
            float ry = (maxy - miny) * 0.5f;
            float r = (rx > ry) ? rx : ry;
            float rr = r * r;
            for(int32_t py = miny; py <= maxy; py++){
              for(int32_t px = minx; px <= maxx; px++){
                float dx = px - cx;
                float dy = py - cy;
                if(dx*dx + dy*dy <= rr){
                  if(px < 0 || px >= SPRITEWIDTH || py < 0 || py >= SPRITEHEIGHT) continue;
                  sprites[py * SPRITEWIDTH + px] = colorIndex;
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
              if(cx0 >= 0 && cx0 < SPRITEWIDTH && cy0 >= 0 && cy0 < SPRITEHEIGHT)
                sprites[cy0 * SPRITEWIDTH + cx0] = colorIndex;
              if(cx0 == x1 && cy0 == y1) break;
              int32_t e2 = 2*err;
              if(e2 >= dy){ err += dy; cx0 += sxl; }
              if(e2 <= dx){ err += dx; cy0 += syl; }
            }
          }
          shaping = 0;
        }
        break;
      default:
        break;
    }
      // if an operation started inside the editor, save a post-change snapshot on release
      if(mouseDownInEditor){
        PushSnapshot();
        mouseDownInEditor = 0;
      }
  }
}

static void DrawSpriteEditor(Vector2 position, int32_t size){
  int32_t gridSize = TILESIZE/spriteEditor.zoom;        // 8=8x8, 4=16x16, 2=32x32, 1=128x128
  int32_t spriteSize = TILESIZE*spriteEditor.zoom;

  // draw the sprite base on the spritesheet position
  int32_t sx = spriteEditor.activeSpriteIndex % (TILESIZE*2) * TILESIZE;
  int32_t sy = spriteEditor.activeSpriteIndex / (TILESIZE*2) * TILESIZE;

  for(int32_t y=0;y<spriteSize;y++){
    for(int32_t x=0;x<spriteSize;x++){
      int32_t pixelX = sx + x + (int32_t)spritePan.x;
      int32_t pixelY = sy + y + (int32_t)spritePan.y;
      int32_t index = pixelY * SPRITEWIDTH + pixelX;

      // behind the sprites
      //DrawRectangleLines(position.x*SCREENSCALE + x * (gridSize*SCREENSCALE), position.y*SCREENSCALE + y * (gridSize*SCREENSCALE), gridSize*SCREENSCALE, gridSize*SCREENSCALE, GetNanoColor(1));
      if(pixelX < 0 || pixelX >= SPRITEWIDTH || pixelY < 0 || pixelY >= SPRITEHEIGHT) continue; 
      
      // draw every pixel of the sprite
      DrawRectangle(position.x*SCREENSCALE + x * (gridSize*SCREENSCALE), position.y*SCREENSCALE + y * (gridSize*SCREENSCALE), gridSize*SCREENSCALE, gridSize*SCREENSCALE, GetNanoColor(sprites[index])); 
    }
  }

  // show pen size snap to grid 
  if(hoverUI == SPR_UI_EDITOR){
    Vector2 mousePos = GetMousePosition();

    // convert mouse into local grid base 
    int32_t localX = (mousePos.x - position.x*SCREENSCALE) / SCREENSCALE;
    int32_t localY = (mousePos.y - position.y*SCREENSCALE) / SCREENSCALE;

    // snap to TILESIZExTILESIZE
    int32_t snapX = (localX / gridSize) * gridSize;
    int32_t snapY = (localY / gridSize) * gridSize;

    // draw the brush color
    switch(toolClicked){
      case TOOL_PEN:
      case TOOL_SHAPE:
        DrawRectangle((position.x + snapX)*SCREENSCALE, (position.y + snapY)*SCREENSCALE, gridSize*SCREENSCALE*(pensilSize+1), gridSize*SCREENSCALE*(pensilSize+1), GetNanoColor(colorIndex));
        break;
      case TOOL_FILL:
        DrawRectangle((position.x + snapX)*SCREENSCALE, (position.y + snapY)*SCREENSCALE, gridSize*SCREENSCALE, gridSize*SCREENSCALE, GetNanoColor(colorIndex));
        break;
      case TOOL_SELECT:
        break;
      default:
        break;
    }
  }

  // sprite editor rect
  DrawRectangleLines(position.x*SCREENSCALE, position.y*SCREENSCALE, size*SCREENSCALE, size*SCREENSCALE, GetNanoColor(7));  

  // draw selection rectangle if active
  if(pixelSelection.active){
    int32_t sxSel = (int32_t)pixelSelection.position.x;
    int32_t sySel = (int32_t)pixelSelection.position.y;
    int32_t sw = (int32_t)pixelSelection.size.x;
    int32_t sh = (int32_t)pixelSelection.size.y;
    DrawRectangleLines((position.x + sxSel * gridSize)*SCREENSCALE, (position.y + sySel * gridSize)*SCREENSCALE, sw * gridSize * SCREENSCALE, sh * gridSize * SCREENSCALE, GetNanoColor(6));
  }

  // shape preview when shaping
  if(shaping && toolClicked == TOOL_SHAPE){
    Vector2 mousePos = GetMousePosition();
    int32_t localX = (mousePos.x - position.x*SCREENSCALE) / SCREENSCALE;
    int32_t localY = (mousePos.y - position.y*SCREENSCALE) / SCREENSCALE;
    int32_t curX = localX / gridSize;
    int32_t curY = localY / gridSize;

    int32_t relStartX = (int32_t)shapeStart.x - sx - (int32_t)spritePan.x;
    int32_t relStartY = (int32_t)shapeStart.y - sy - (int32_t)spritePan.y;

    int32_t minx = (relStartX < curX) ? relStartX : curX;
    int32_t miny = (relStartY < curY) ? relStartY : curY;
    int32_t maxx = (relStartX > curX) ? relStartX : curX;
    int32_t maxy = (relStartY > curY) ? relStartY : curY;

    switch(shapeMode){
      case 0: // rect preview
        DrawRectangleLines((position.x + minx * gridSize)*SCREENSCALE, (position.y + miny * gridSize)*SCREENSCALE, (maxx-minx+1) * gridSize * SCREENSCALE, (maxy-miny+1) * gridSize * SCREENSCALE, GetNanoColor(8));
        break;
      case 1: // circle preview
        {
          int32_t w = (maxx-minx+1) * gridSize * SCREENSCALE;
          int32_t h = (maxy-miny+1) * gridSize * SCREENSCALE;
          DrawRectangleLines((position.x + minx * gridSize)*SCREENSCALE, (position.y + miny * gridSize)*SCREENSCALE, w, h, GetNanoColor(8));
        }
        break;
      case 2: // line preview
        {
          int32_t x0 = (position.x + relStartX * gridSize) * SCREENSCALE + gridSize*SCREENSCALE/2;
          int32_t y0 = (position.y + relStartY * gridSize) * SCREENSCALE + gridSize*SCREENSCALE/2;
          int32_t x1 = (position.x + curX * gridSize) * SCREENSCALE + gridSize*SCREENSCALE/2;
          int32_t y1 = (position.y + curY * gridSize) * SCREENSCALE + gridSize*SCREENSCALE/2;
          DrawLine(x0, y0, x1, y1, GetNanoColor(8));
        }
        break;
    }
  }
}

// COLOR PALLETE
static void UpdateColorPallete(Vector2 position){
  int32_t prevPosX = position.x;
  int32_t count = 0;
  
  for(int32_t i=0;i<COLORCOUNT;i++){
    int32_t posx = position.x + (TILESIZE/2)/2;
    int32_t posy = position.y + (TILESIZE/2)/2; 
    GetMouseClick((Vector2){posx, posy}, TILESIZE, TILESIZE, SPR_UI_COLOR, i);
    
    position.x += TILESIZE + 1;
    count++;
    if(count == 4){
      position.x = prevPosX;
      position.y += TILESIZE + 1;
      count = 0;
    } 
  }
}

static void DrawColorPallete(Vector2 position){ 
  int32_t width = TILESIZE*4 + 7;
  int32_t height = TILESIZE*4 + 7; 

  DrawRectangleLines(position.x*SCREENSCALE, position.y*SCREENSCALE, width*SCREENSCALE, height*SCREENSCALE, GetNanoColor(7));
  
  int32_t count = 0;
  int32_t prevPosX = position.x;
  for(int32_t i=0;i<COLORCOUNT;i++){
    int32_t posx = position.x*SCREENSCALE + (TILESIZE/2)*SCREENSCALE/2;
    int32_t posy = position.y*SCREENSCALE + (TILESIZE/2)*SCREENSCALE/2;
    DrawRectangle(posx, posy, TILESIZE*SCREENSCALE, TILESIZE*SCREENSCALE, GetNanoColor(i));
    
    // to highlight the selected color
    if(i == colorIndex)
      DrawRectangleLines(posx-2, posy-2, TILESIZE*SCREENSCALE +5, TILESIZE*SCREENSCALE +5, GetNanoColor(7));
    
    position.x += TILESIZE +1;
    count++;
    if(count == 4){
      position.x = prevPosX;
      position.y += TILESIZE +1;
      count = 0;
    } 
  }
}

// TOOLS
void UpdateTools(Vector2 position, int32_t distance, bool vertical){
  for(int32_t i=0;i<5;i++){
    GetMouseClick(position, TILESIZE, TILESIZE, SPR_UI_TOOLS, i);
    if(vertical)
      position.y+=TILESIZE+distance;
    else
      position.x+=TILESIZE+distance;
  }
}

void DrawTools(Vector2 position, int32_t distance, bool vertical){
  int32_t icon = 5;
  int32_t color = 6;
  for(int32_t i=0;i<5;i++){
    color = 6;
    if((Tools)i == toolClicked)
      color = 8;
    DrawRectangleLines(position.x*SCREENSCALE, position.y*SCREENSCALE, TILESIZE*SCREENSCALE, TILESIZE*SCREENSCALE, GetNanoColor(color)); 
    DrawIcons(icon++, (Vector2){position.x, position.y}, color);
    
    if(vertical)
      position.y+=TILESIZE+distance;
    else
      position.x+=TILESIZE+distance;
  }
}

static void UpdateSwitch(Vector2 position, SprUIClicked switchUIType){
  int32_t buttonSize = 4;
  int32_t buttonCount = 4;

  for(int32_t i=0;i<buttonCount;i++){
    GetMouseClick(position, buttonSize, buttonSize, switchUIType, i);
    position.x += 6;
  }

  switch(switchUIType){
    default:
      break;
    case SPR_UI_ZOOMSIZE:
      if(zoomValue == 0) spriteEditor.zoom = 1;    // 8x8
      if(zoomValue == 1) spriteEditor.zoom = 2;    // 16x16
      if(zoomValue == 2) spriteEditor.zoom = 4;    // 32x32
      if(zoomValue == 3) spriteEditor.zoom = 8;    // 64x64
      //printf("zoom size: %d\n", spriteEditor.zoom);
      break; 
  }
}

static void DrawSwitch(Vector2 position, int32_t index){
  position.x *= SCREENSCALE;
  position.y *= SCREENSCALE;
  int32_t buttonSize = 4;
  int32_t buttonCount = 4;
  
  DrawRectangle(position.x, position.y+1*SCREENSCALE, 20*SCREENSCALE, 2*SCREENSCALE, GetNanoColor(6));
  
  int32_t buttonPos = position.x;
  for(int32_t i=0;i<buttonCount;i++){
    DrawRectangle(buttonPos, position.y, buttonSize*SCREENSCALE, buttonSize*SCREENSCALE, GetNanoColor(6));
    if(i == index)
      DrawRectangle(buttonPos+4, position.y+4, 2*SCREENSCALE, 2*SCREENSCALE, GetNanoColor(8));  
    buttonPos += 6*SCREENSCALE;
  }
}

void DrawSpriteIndex(Vector2 position){
  int32_t scalex = FONTWIDTH*8;
  int32_t scaley = FONTHEIGHT + FONTHEIGHT/2;
  char spriteIndexText[32];
  sprintf(spriteIndexText, "SPR=%d\n", spriteEditor.activeSpriteIndex);
  DrawTextUI(spriteIndexText, (Vector2){position.x+(scalex-FONTWIDTH*7)/2, position.y+(scaley-FONTHEIGHT)/2});
}

static void DrawSpriteLabel(Vector2 position){
  char* test = labelname;
  DrawTextUI(test, position);
}

static void UpdateSpriteFlags(Vector2 position){
  const int32_t dotSize = 4;
  for(int32_t i=0;i<FLAGCOUNT;i++){
    GetMouseClick(position, dotSize, dotSize, SPR_UI_FLAGS, i);
    position.x += 1 + dotSize;
  }
}

static void DrawSpriteFlags(Vector2 position){
  const int32_t flagCount = 7;
  const int32_t dotSize = 4;
  int8_t color = 1;
  for(int32_t i=0;i<flagCount;i++){
    color = 1;
    int8_t flagValue = spriteFlags[spriteEditor.activeSpriteIndex].flags[i];
    if(flagValue)
      color += i+1;
    DrawRectangle(position.x*SCREENSCALE, position.y*SCREENSCALE, dotSize*SCREENSCALE, dotSize*SCREENSCALE, GetNanoColor(color));
    position.x += 1 + dotSize;
  }
}

// Get the position of the mouse and what UI is hovered and clicked
static void GetMouseClick(Vector2 position, int32_t width, int32_t height, SprUIClicked sprUI, int32_t index){
  Vector2 mousePos = GetMousePosition();
  position.x *= SCREENSCALE;
  position.y *= SCREENSCALE;
  width *= SCREENSCALE;
  height *= SCREENSCALE;

  if(mousePos.x > position.x && mousePos.x < position.x + width && mousePos.y > position.y && mousePos.y < position.y + height){
    hoverUI = sprUI;
    hoveredIndex = index; 
  }
}

void UpdateSpriteSheet(Vector2 position){
  if(!IsMouseButtonPressed(0)) return;
  Vector2 mousepos = GetMousePosition();

  int32_t mx = (mousepos.x - position.x*SCREENSCALE) / SCREENSCALE;
  int32_t my = (mousepos.y - position.y*SCREENSCALE) / SCREENSCALE;

  if(mx < 0 || mx > 128 || my < 0) return;

  int32_t spriteX = mx / TILESIZE;
  int32_t spriteY = my / TILESIZE;

  int32_t localId = spriteY * 16 + spriteX;

  int32_t selectedSprite = currentPage * 64 + localId;
  //printf("id selected %d\n", selectedSprite);
  spriteEditor.activeSpriteIndex = selectedSprite;
}

// DRAW THE SPRITE SHEET 
void DrawSpriteSheet(Vector2 position){

  // draw the sprite 
  const int32_t pageHeight = 32;
  int32_t pageSX = 0; 
  int32_t pageSY = currentPage * pageHeight;

  for(int32_t y=0;y<pageHeight;y++){
    for(int32_t x=0;x<SPRITEWIDTH;x++){
      int32_t index = (pageSY + y) * SPRITEWIDTH + (pageSX + x);
      int8_t col = sprites[index];
      DrawRectangle((position.x+x)*SCREENSCALE, (position.y+y)*SCREENSCALE, SCREENSCALE, SCREENSCALE, GetNanoColor(col));
    }
  } 
  
  // selection box
  int32_t pageLocal_id = spriteEditor.activeSpriteIndex - currentPage * 64;
  if(pageLocal_id >=0 && pageLocal_id < 64){
    int32_t sel_sx = pageLocal_id % 16;   // 0-5 across
    int32_t sel_sy = pageLocal_id / 16;   // 0-3 down 
    int32_t sel_x = sel_sx * TILESIZE * SCREENSCALE;
    int32_t sel_y = sel_sy * TILESIZE * SCREENSCALE;
    DrawRectangleLinesEx(
      (Rectangle){
        (position.x*SCREENSCALE) + sel_x -4, 
        (position.y*SCREENSCALE) + sel_y -4,  
        TILESIZE*spriteEditor.zoom*SCREENSCALE + 8, 
        TILESIZE*spriteEditor.zoom*SCREENSCALE + 8
      }, 
      4, 
      GetNanoColor(6)
    );
  }

  // draw the spritesheet border
  DrawRectangleLines(position.x*SCREENSCALE, position.y*SCREENSCALE, TILESIZE*SCREENSCALE*16 + 1, TILESIZE*SCREENSCALE*4, GetNanoColor(7));
}

// draw the sprite index tabs
void DrawSpriteSheetTabs(Vector2 position){
  int32_t color = 8;
  char text[32];
  for(int32_t i=0;i<4;i++){
    if(i == currentPage) 
      DrawRectangle((position.x+(i*(TILESIZE+1)))*SCREENSCALE, position.y*SCREENSCALE, TILESIZE*SCREENSCALE, TILESIZE*SCREENSCALE, GetNanoColor(color));
    sprintf(text, "%d", i);
    DrawTextUI(text,(Vector2){position.x+2+(i*(TILESIZE+1)), position.y+1});
  }
}

void FloodFill(int8_t *buffer, int32_t sx, int32_t sy, int32_t start_x, int32_t start_y, int32_t targetColor, int8_t newColor, int32_t spriteSize){
  if(targetColor == newColor) return; 

  Vector2 stack[SPRITEWIDTH*SPRITEHEIGHT];
  int top = 0;

  stack[top++] = (Vector2){start_x - sx, start_y - sy};

  while(top > 0){
    Vector2 p = stack[--top];
    int32_t index = (sy+p.y) * SPRITEWIDTH + (sx+p.x); 

    if(p.x < 0 || p.x >= spriteSize || p.y < 0 || p.y >= spriteSize) continue;
 
    if(buffer[index] != targetColor) continue;
    buffer[index] = newColor;

    stack[top++] = (Vector2){p.x+1, p.y};
    stack[top++] = (Vector2){p.x-1, p.y};
    stack[top++] = (Vector2){p.x, p.y+1};
    stack[top++] = (Vector2){p.x, p.y-1};
  }
}

// Flood fill constrained to a rectangular region (rect_w x rect_h) starting at rect_sx,rect_sy
void FloodFillRect(int8_t *buffer, int32_t rect_sx, int32_t rect_sy, int32_t start_x, int32_t start_y, int32_t targetColor, int8_t newColor, int32_t rect_w, int32_t rect_h){
  if(targetColor == newColor) return;
  if(rect_w <= 0 || rect_h <= 0) return;

  int32_t relStartX = start_x - rect_sx;
  int32_t relStartY = start_y - rect_sy;
  if(relStartX < 0 || relStartX >= rect_w || relStartY < 0 || relStartY >= rect_h) return;

  int32_t capacity = rect_w * rect_h;
  Vector2 *stack = (Vector2*)malloc(sizeof(Vector2) * capacity);
  if(!stack) return;
  int top = 0;

  stack[top++]=(Vector2){(float)relStartX,(float)relStartY};
  while(top > 0){
    Vector2 p = stack[--top];
    int32_t lx = (int32_t)p.x;
    int32_t ly = (int32_t)p.y;
    if(lx < 0 || lx >= rect_w || ly < 0 || ly >= rect_h) continue;
    int32_t index = (rect_sy + ly) * SPRITEWIDTH + (rect_sx + lx);
    if(buffer[index] != targetColor) continue;
    buffer[index] = newColor;

    if(top + 4 < capacity){
      stack[top++] = (Vector2){ (float)(lx+1), (float)ly };
      stack[top++] = (Vector2){ (float)(lx-1), (float)ly };
      stack[top++] = (Vector2){ (float)lx, (float)(ly+1) };
      stack[top++] = (Vector2){ (float)lx, (float)(ly-1) };
    }
  }
  free(stack);
}

// Copy selection rectangle into clipboard. rect_sx,rect_sy are absolute sprite coordinates.
static void CopyToClipboard(int32_t rect_sx, int32_t rect_sy, int32_t rect_w, int32_t rect_h){
  if(clipboard.data) free(clipboard.data);
  clipboard.w = rect_w; clipboard.h = rect_h;
  clipboard.data = malloc(rect_w * rect_h);
  for(int j=0;j<rect_h;j++){
    for(int i=0;i<rect_w;i++){
      int32_t sx = rect_sx + i;
      int32_t sy = rect_sy + j;
      if(sx < 0 || sx >= SPRITEWIDTH || sy < 0 || sy >= SPRITEHEIGHT) clipboard.data[j*rect_w + i] = 0;
      else clipboard.data[j*rect_w + i] = sprites[sy * SPRITEWIDTH + sx];
    }
  }
}

// Paste clipboard to absolute position dst_sx,dst_sy
static void PasteFromClipboard(int32_t dst_sx, int32_t dst_sy){
  if(!clipboard.data) return;
  for(int j=0;j<clipboard.h;j++){
    for(int i=0;i<clipboard.w;i++){
      int32_t sx = dst_sx + i;
      int32_t sy = dst_sy + j;
      if(sx < 0 || sx >= SPRITEWIDTH || sy < 0 || sy >= SPRITEHEIGHT) continue;
      sprites[sy * SPRITEWIDTH + sx] = clipboard.data[j*clipboard.w + i];
    }
  }
}

// Undo/Redo implementation
static void UndoInit(void){
  if(undoStack) return;
  undoStack = (int8_t**)malloc(sizeof(int8_t*) * UNDO_CAPACITY);
  if(!undoStack) return;
  for(int32_t i=0;i<UNDO_CAPACITY;i++) undoStack[i]=NULL;
  undoCount = 0;
  undoIndex = -1;
  // push initial snapshot of current sprite data so undo has a baseline
  PushSnapshot();
}

static void UndoFree(void){
  if(!undoStack) return;
  for(int32_t i=0;i<undoCount;i++){
    if(undoStack[i]) free(undoStack[i]);
  }
  free(undoStack);
  undoStack = NULL;
  undoCount = 0;
  undoIndex = -1;
}

// push a snapshot of current 'sprites' into the undo stack
static void PushSnapshot(void){
  if(!undoStack) return;
  int32_t size = SPRITEWIDTH * SPRITEHEIGHT;
  int8_t *buf = (int8_t*)malloc(size);
  if(!buf) return;
  memcpy(buf, sprites, size);

  // if we have undone some steps and then make a new change, discard redo entries
  if(undoIndex < undoCount - 1){
    for(int32_t i = undoIndex + 1; i < undoCount; i++){
      if(undoStack[i]) free(undoStack[i]);
      undoStack[i] = NULL;
    }
    undoCount = undoIndex + 1;
  }

  // Avoid pushing a snapshot identical to the latest stored snapshot
  if(undoCount > 0 && undoStack[undoCount - 1]){
    if(memcmp(sprites, undoStack[undoCount - 1], size) == 0){
      free(buf);
      return;
    }
  }

  // if capacity full, drop oldest
  if(undoCount == UNDO_CAPACITY){
    free(undoStack[0]);
    memmove(&undoStack[0], &undoStack[1], sizeof(int8_t*)*(UNDO_CAPACITY-1));
    undoCount--;
    undoIndex--;
  }

  undoStack[undoCount++] = buf;
  undoIndex = undoCount - 1;
}

static void DoUndo(void){
  if(!undoStack || undoCount == 0) return;
  if(undoIndex <= 0) return; // nothing to undo
  undoIndex--;
  int32_t size = SPRITEWIDTH * SPRITEHEIGHT;
  if(undoStack[undoIndex]) memcpy(sprites, undoStack[undoIndex], size);
}

static void DoRedo(void){
  if(!undoStack || undoCount == 0) return;
  if(undoIndex >= undoCount - 1) return; // nothing to redo
  undoIndex++;
  int32_t size = SPRITEWIDTH * SPRITEHEIGHT;
  if(undoStack[undoIndex]) memcpy(sprites, undoStack[undoIndex], size);
}
