#include "cartridge.h"

#include "editor.h"
#include "sprite.h"
#include "maps.h"

#include <string.h>
#include <stdlib.h>
#include <raylib.h>

extern int8_t sprites[SPRITEWIDTH*SPRITEHEIGHT];
extern int8_t mapData[MAPWIDTH*MAPHEIGHT];

static int32_t HexToInt(char c);

void SaveCartridge(const char* filename){
  FILE* fp = fopen(filename, "w");
  if(!fp) return;

  fprintf(fp, "NANO-8 cartridge\n");
  fprintf(fp, "version 1\n\n");

  // COLOR PALLET
  fprintf(fp, "__PALLETE__\n\n");
  for(int32_t i=0;i<COLORCOUNT;i++){
    Color palette = GetNanoColor(i);
    fprintf(fp, "%02X%02X%02X\n", palette.r, palette.g, palette.b);
  }

  // SPRITESHEET
  fprintf(fp, "__SPRITESHEET__\n\n");
  for(int32_t y=0;y<SPRITEHEIGHT;y++){
    for(int32_t x=0;x<SPRITEWIDTH;x++){
      int8_t color = sprites[y * SPRITEWIDTH + x];
      fprintf(fp, "%X", color);
    }
    fprintf(fp, "\n");
  }

  // MAP 
  fprintf(fp, "__MAPS__\n\n");
  for(int32_t y=0;y<MAPHEIGHT;y++){
    for(int32_t x=0;x<MAPWIDTH;x++){
      int32_t map = mapData[y * MAPWIDTH + x];
      if(map < 0)
        fprintf(fp, "FFFF");
      else
        fprintf(fp, "%04X", map);
    }
    fprintf(fp, "\n");
  }

  // SFX 
  fprintf(fp, "__SFX__\n\n");

  // MUSIC
  fprintf(fp, "__MUSIC__\n\n");

  // CODE
  fprintf(fp, "__LUA__\n");

  for(int32_t i=0;i<GetEditorSectionCount()+1;i++){
    char* code = GetLuaCodeInSection(i); 
    fprintf(fp, "__SECTION__\n%s", code);
    free(code);
  }

  fclose(fp);
}

// load the cart
bool LoadCartridge(const char* filename){
  FILE* fp = fopen(filename, "r");
  if(!fp){
    printf("Failed to open cartridge\n");
    return false;
  }

  char line[1024];
  int32_t codeCount = 0;
  CartSection cartSection = CART_NONE; 

  while(fgets(line, sizeof(line), fp)){
    if(strncmp(line, "__PALLETE__", 11) == 0){
      cartSection = CART_PALLETE;
      continue;
    }
    if(strncmp(line, "__SPRITESHEET__", 15) == 0){
      cartSection = CART_SPRITES;
      continue;
    }
    if(strncmp(line, "__MAPS__", 8) == 0){
      cartSection = CART_MAP;
      continue;
    }
    if(strncmp(line, "__SFX__", 7) == 0){
      cartSection = CART_SFX;
      continue;
    }
    if(strncmp(line, "__MUSIC__", 9) == 0){
      cartSection = CART_MUSIC;
      continue;
    }
    if(strncmp(line, "__LUA__", 7) == 0){
      cartSection = CART_LUA;
      continue;
    }

    // parse section data here
    switch(cartSection){
      case CART_PALLETE:
        for(int32_t i=0;i<COLORCOUNT;i++){
          fgets(line, sizeof(line), fp);
          unsigned int r, g, b;
          sscanf(line, "%02X%02X%02X", &r, &g, &b);
          SetNanoColor(i, r, g, b);
        }
        break;
      case CART_SPRITES:{
          for(int y = 0; y < SPRITEHEIGHT; y++){
            fgets(line, sizeof(line), fp);
            for(int x = 0; x < SPRITEWIDTH; x++){ 
              sprites[y * SPRITEWIDTH + x] = HexToInt(line[x]);
            }
          }
        }
        break;
      case CART_MAP:{
          for(int y = 0; y < MAPHEIGHT; y++){
            fgets(line, sizeof(line), fp);
            for(int x = 0; x < MAPWIDTH; x++){
              int i = x * 4;

              unsigned int value =
                (HexToInt(line[i]) << 12) |
                (HexToInt(line[i + 1]) << 8) |
                (HexToInt(line[i + 2]) << 4) |
                HexToInt(line[i + 3]);

              if(value == 0xFFFF)
                mapData[y * MAPWIDTH + x] = -1;
              else
                mapData[y * MAPWIDTH + x] = value;
            }
          }
        }
        break;
      case CART_LUA:{
        if(strncmp(line, "__SECTION__", 11) == 0){
          if(codeCount > 0)
            NewSection();
          codeCount++;
          continue;
        }
        //printf("line %s\n", line);
        LoadCode(line, strlen(line));
      }
        break;
      case CART_SFX:
        break;
      case CART_MUSIC:
        break;
      default:
        break;
    }
  }

  return true;
}

static int32_t HexToInt(char c){
  if(c >= '0' && c <= '9') return c - '0';
  if(c >= 'A' && c <= 'F') return c - 'A' + 10;
  if(c >= 'a' && c <= 'f') return c - 'a' + 10;
  return 0;
}
