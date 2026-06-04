// SPRITE EDITOR 
#ifndef SPRITE_H
#define SPRITE_H

#include <stdint.h>
#include <raylib.h>

typedef struct{
  int32_t spriteIndex;
  int8_t *flags;
} SpriteFlags;

typedef struct{
  int32_t zoom;
  int32_t activeSpriteIndex;
  Vector2 position;
} SpriteEditor;

typedef struct{
  int32_t active;
  Vector2 position;
  Vector2 size;
} PixelSelection;

typedef enum{
  TOOL_PEN,
  TOOL_SELECT,
  TOOL_PAN,
  TOOL_FILL,
  TOOL_SHAPE,
  TOOL_COUNT
} Tools;

typedef enum{
  SPR_UI_NONE,
  SPR_UI_EDITOR, 
  SPR_UI_TOOLS,
  SPR_UI_COLOR,
  SPR_UI_PENSIZE,
  SPR_UI_ZOOMSIZE,
  SPR_UI_FLAGS
} SprUIClicked;

void SpriteInit(void);
void SpriteFree(void);
void SpriteUpdate(void);
void TabInput(int32_t *showTab);
void SpriteInput(void);
void SpriteDraw(void);
int8_t *GetSprite(void);
void UpdateSpriteSheet(Vector2 position);
void DrawSpriteSheetTabs(Vector2 position);
void DrawSpriteSheet(Vector2 position);
void DrawSpriteIndex(Vector2 position);
void DrawTools(Vector2 position, int32_t distance, bool vertical);
void UpdateTools(Vector2 position, int32_t distance, bool vertical);

void FloodFill(int32_t start_x, int32_t start_y, int32_t targetColor, int8_t newColor);

#endif
