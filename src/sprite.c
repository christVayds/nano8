#include "sprite.h"
#include "game.h"
#include "font.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// SPRITE
Sprite sprites[SPRITEWIDTH*SPRITEHEIGHT];
SpriteEditor spriteEditor;
static SpriteFlags spriteFlags[16*16];

// pixel selection 
PixelSelection pixelSelection;

static int32_t currentPage = 0; // spritesheet page

// update and draw function for UIs
static void UpdateSpriteEditor(Vector2 position);
static void DrawSpriteEditor(Vector2 position, int32_t size);
static void UpdateColorPallete(Vector2 position);
static void DrawColorPallete(Vector2 position);
static void UpdateTools(Vector2 position);
static void DrawTools(Vector2 position);
static void UpdateSwitch(Vector2 position, SprUIClicked switchUIType);
static void DrawSwitch(Vector2 position, int32_t index); 
static void UpdateSpriteFlags(Vector2 position);
static void DrawSpriteFlags(Vector2 position);
static void DrawSpriteLabel(Vector2 position);
static void GetMouseClick(Vector2 position, int32_t width, int32_t height, SprUIClicked sprUI, int32_t index);

static Tools toolClicked = TOOL_PEN;          // current tool used 
static int32_t colorIndex = 7;
static int32_t zoomValue = 0;
static int32_t pensilSize = 0;
static int32_t showWholeSprite = false;

static SprUIClicked hoverUI = SPR_UI_NONE;    // what UI/element user are hovered in mouse 
static int32_t hoveredIndex = -1;             // what type of UI are hovered 
static char labelname[32];                    // to show the label of the UI

Sprite *GetSprite(void){
  return sprites; 
}

void SpriteInit(void){
  // intitialize sprites 
  for(int32_t i=0;i<SPRITEWIDTH*SPRITEHEIGHT;i++){
    Sprite sprite = {
      .colorIndex = 0,
    }; 
    
    // push new sprite to sprites
    sprites[i] = sprite;
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
}

void SpriteFree(void){
  for(int32_t i=0;i<SPRITEWIDTH*SPRITEHEIGHT;i++){
    sprites[i].colorIndex = 0;
  }

  for(int32_t i=0;i<16*16;i++){
    free(spriteFlags[i].flags);
  }
}

void SpriteUpdate(void){
  hoverUI = SPR_UI_NONE;
  hoveredIndex = -1;

  UpdateSpriteEditor((Vector2){16, 12});
  UpdateTools((Vector2){4, 12});
  UpdateColorPallete((Vector2){85, 37});
  UpdateSwitch((Vector2){100, 20}, SPR_UI_PENSIZE);           // pen size update 
  UpdateSwitch((Vector2){100, 30}, SPR_UI_ZOOMSIZE);          // zoom size update
  UpdateSpriteFlags((Vector2){88, 12});
  UpdateSpriteSheet((Vector2){0,88});

  if(IsMouseButtonPressed(0)){
    switch(hoverUI){
      case SPR_UI_TOOLS:
        toolClicked = (Tools)hoveredIndex;
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
            sprintf(labelname, "Shapes - nah");
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

  int32_t key = GetKeyPressed();
  if(key >= '1' && key <= '6')
    switch(key){
      case '1': toolClicked = TOOL_PEN; break;
      case '2': toolClicked = TOOL_SELECT; break;
      case '3': toolClicked = TOOL_PAN; break;
      case '4': toolClicked = TOOL_FILL; break;
      case '5': toolClicked = TOOL_SHAPE; break;
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
    DrawTools((Vector2){4, 12});
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
  DrawIcons(15, (Vector2){0,0}, 6 + (2 * !showWholeSprite));
  DrawIcons(16, (Vector2){8,0}, 6 + (2 * showWholeSprite));

  // Text UI
  DrawSpriteIndex((Vector2){36, 0});
  DrawSpriteLabel((Vector2){1, SCREENHEIGHT-FONTHEIGHT-1});
}

static void UpdateSpriteEditor(Vector2 position){
  GetMouseClick(position, 64, 64, SPR_UI_EDITOR, 0); 

  int32_t gridSize = TILESIZE/spriteEditor.zoom;        // 8=8x8, 4=16x16, 2=32x32, 1=64x64 - how many sprites to draw 
  int32_t spriteSize = TILESIZE*spriteEditor.zoom;      // sprite size base on how many sprite is to draw

  // draw the sprite base on the spritesheet position
  int32_t sx = (spriteEditor.activeSpriteIndex % (TILESIZE*2)) * TILESIZE;
  int32_t sy = (spriteEditor.activeSpriteIndex / (TILESIZE*2)) * TILESIZE;

  Vector2 mousePos = GetMousePosition();
  for(int32_t y=0;y<spriteSize;y++){
    for(int32_t x=0;x<spriteSize;x++){
      int32_t pixelX = sx + x;
      int32_t pixelY = sy + y;
      int32_t index = pixelY * SPRITEWIDTH + pixelX;
      
      int32_t xpos = position.x*SCREENSCALE + x * (gridSize*SCREENSCALE);
      int32_t ypos = position.y*SCREENSCALE + y * (gridSize*SCREENSCALE);
      if(pixelX < 0 || pixelX >= SPRITEWIDTH || pixelY < 0 || pixelY >= SPRITEHEIGHT) continue;
 
      if(mousePos.x > xpos && mousePos.x < xpos+gridSize*SCREENSCALE && mousePos.y > ypos && mousePos.y < ypos+gridSize*SCREENSCALE){
        spriteEditor.position.x = pixelX;
        spriteEditor.position.y = pixelY;

        if(IsMouseButtonDown(0)){
          switch(toolClicked){
            case TOOL_PEN:{
              // TODO: make bigger pensil size avoid other sprites
              for(int32_t peny=0;peny<pensilSize+1;peny++){
                for(int32_t penx=0;penx<pensilSize+1;penx++){
                  int32_t nindex = peny * SPRITEWIDTH + penx;
                  sprites[index+nindex].colorIndex = colorIndex;
                }
              }
              break;
            }
            case TOOL_SELECT:
              if(!pixelSelection.active){
                pixelSelection.position.x = x;
                pixelSelection.position.y = y;
                pixelSelection.active = true;
              } else {
                pixelSelection.size.x = x+1;
                pixelSelection.size.y = y+1;
              }
              //printf("yeahhh %d %d %d %d\n", (int32_t)pixelSelection.position.x, (int32_t)pixelSelection.position.y, (int32_t)pixelSelection.size.x, (int32_t)pixelSelection.size.y);
              break;
            case TOOL_PAN:
              break;
            case TOOL_FILL:{
              FloodFill(pixelX, pixelY, sprites[index].colorIndex, colorIndex);
              break;
            } 
            case TOOL_SHAPE:
              break;
            default:
              //sprites[index].colorIndex = colorIndex;
              break;
          }
        }

        if(IsMouseButtonReleased(0)){
          switch(toolClicked){
            default: break;
            case TOOL_SELECT:
              pixelSelection.active = false;
              break;
          }
        }
      }
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
      int32_t pixelX = sx + x;
      int32_t pixelY = sy + y;
      int32_t index = pixelY * SPRITEWIDTH + pixelX;

      // behind the sprites
      //DrawRectangleLines(position.x*SCREENSCALE + x * (gridSize*SCREENSCALE), position.y*SCREENSCALE + y * (gridSize*SCREENSCALE), gridSize*SCREENSCALE, gridSize*SCREENSCALE, GetNanoColor(1));
      if(pixelX < 0 || pixelX >= SPRITEWIDTH || pixelY < 0 || pixelY >= SPRITEHEIGHT) continue; 
      
      // draw every pixel of the sprite
      DrawRectangle(position.x*SCREENSCALE + x * (gridSize*SCREENSCALE), position.y*SCREENSCALE + y * (gridSize*SCREENSCALE), gridSize*SCREENSCALE, gridSize*SCREENSCALE, GetNanoColor(sprites[index].colorIndex)); 
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
        // NOTE: TEMPORARY SIZE OF PEN
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
static void UpdateTools(Vector2 position){
  int32_t distance = 6;

  for(int32_t i=0;i<5;i++){
    GetMouseClick(position, TILESIZE, TILESIZE, SPR_UI_TOOLS, i);
    position.y+=TILESIZE+distance;
  }
}

static void DrawTools(Vector2 position){ 
  int32_t distance = 6;

  int32_t icon = 5;
  int32_t color = 6;
  for(int32_t i=0;i<5;i++){
    color = 6;
    if((Tools)i == toolClicked)
      color = 8;
    DrawRectangleLines(position.x*SCREENSCALE, position.y*SCREENSCALE, TILESIZE*SCREENSCALE, TILESIZE*SCREENSCALE, GetNanoColor(color)); 
    DrawIcons(icon++, (Vector2){position.x, position.y}, color);
    position.y+=TILESIZE+distance;
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
      int8_t col = sprites[index].colorIndex;
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

void FloodFill(int32_t start_x, int32_t start_y, int32_t targetColor, int8_t newColor){
  if(targetColor == newColor) return;
  int32_t spriteSize = TILESIZE * spriteEditor.zoom;   

  int32_t sx = (spriteEditor.activeSpriteIndex % 16) * TILESIZE;
  int32_t sy = (spriteEditor.activeSpriteIndex / 16) * TILESIZE;

  Vector2 stack[SPRITEWIDTH*SPRITEHEIGHT];
  int top = 0;

  stack[top++] = (Vector2){start_x - sx, start_y - sy};

  while(top > 0){
    Vector2 p = stack[--top];
    int32_t index = (sy+p.y) * SPRITEWIDTH + (sx+p.x); 

    if(p.x < 0 || p.x >= spriteSize || p.y < 0 || p.y >= spriteSize) continue;
 
    if(sprites[index].colorIndex != targetColor) continue;
    sprites[index].colorIndex = newColor;

    stack[top++] = (Vector2){p.x+1, p.y};
    stack[top++] = (Vector2){p.x-1, p.y};
    stack[top++] = (Vector2){p.x, p.y+1};
    stack[top++] = (Vector2){p.x, p.y-1};
  }
}
