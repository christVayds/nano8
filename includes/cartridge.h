#ifndef CART_H
#define CART_H


#include <stdint.h>
#include "game.h"

typedef enum{
  CART_NONE,
  CART_PALLETE,
  CART_SPRITES,
  CART_MAP,
  CART_LUA
} CartSection;

typedef struct{
  char code[65536];
  int8_t pallete[COLORCOUNT];
  int8_t sprites[SPRITEWIDTH*SPRITEHEIGHT];
  int8_t map[MAPWIDTH*MAPHEIGHT];
} Cartridge;

void SaveCartridge(const char* filename);
bool LoadCartridge(const char* filename);

#endif
