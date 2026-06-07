#include "game.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sprite.h"
#include "maps.h"

static bool running = true;
static int32_t textCurrentColor = 7;
static Vector2 camera = {0,0};

// MAP DATA 
extern int8_t mapData[MAPWIDTH*MAPHEIGHT];
extern SpriteFlags spriteFlags[16*16];

char *workingDirectory = "test/";

// ------------------
//  FONTS 
// ------------------
static uint8_t font_texts[FONTCOUNT][FONTWIDTH*FONTHEIGHT] = {
  { // SPACE 0
    0,0,0,0,
    0,0,0,0,
    0,0,0,0,
    0,0,0,0,
    0,0,0,0,
    0,0,0,0
  },
  { // ! 1
    0,1,0,0,
    0,1,0,0,
    0,1,0,0,
    0,0,0,0,
    0,1,0,0,
    0,0,0,0
  },
  { // " 2
    1,0,1,0,
    1,0,1,0,
    0,0,0,0,
    0,0,0,0,
    0,0,0,0,
    0,0,0,0
  },
  { // #  13
    1,0,1,0,
    1,1,1,0,
    1,0,1,0,
    1,1,1,0,
    1,0,1,0,
    0,0,0,0
  },
  { // $  14
    1,1,1,0,
    1,1,0,0,
    0,1,1,0,
    1,1,1,0,
    0,1,0,0,
    0,0,0,0
  },
  { // %
    1,0,1,0,
    0,0,1,0,
    0,1,0,0,
    1,0,0,0,
    1,0,1,0,
    0,0,0,0
  },
  { // &
    1,1,0,0,
    1,1,0,0,
    0,1,1,0,
    1,0,1,0,
    1,1,1,0,
    0,0,0,0
  },
  { // ' 58
    0,1,0,0,
    1,0,0,0,
    0,0,0,0,
    0,0,0,0,
    0,0,0,0,
    0,0,0,0
  },
  { // ( 51
    0,1,0,0,
    1,0,0,0,
    1,0,0,0,
    1,0,0,0,
    0,1,0,0,
    0,0,0,0
  },
  { // ) 52
    0,1,0,0,
    0,0,1,0,
    0,0,1,0,
    0,0,1,0,
    0,1,0,0,
    0,0,0,0
  },
  { // * 9
1,0,1,0,
    0,1,0,0,
    1,1,1,0,
    0,1,0,0,
    1,0,1,0,
    0,0,0,0
  },
  { // + 7
0,0,0,0,
    0,1,0,0,
    1,1,1,0,
    0,1,0,0,
    0,0,0,0,
    0,0,0,0
  },
  { // , 6 
0,0,0,0,
    0,0,0,0,
    0,0,0,0,
    0,1,0,0,
    1,0,0,0,
    0,0,0,0
  },
  { // - 8
0,0,0,0,
    0,0,0,0,
    1,1,1,0,
    0,0,0,0,
    0,0,0,0,
    0,0,0,0
  },
  { // . 5
0,0,0,0,
    0,0,0,0,
    0,0,0,0,
    0,0,0,0,
    1,0,0,0,
    0,0,0,0
  }, 
  { // /  10
0,0,1,0,
    0,1,0,0,
    0,1,0,0,
    0,1,0,0,
    1,0,0,0,
    0,0,0,0
  },
  { // 0 50
1,1,1,0,
    1,0,1,0,
    1,0,1,0,
    1,0,1,0,
    1,1,1,0,
    0,0,0,0
  },
  { // 1 41
1,1,0,0,
    0,1,0,0,
    0,1,0,0,
    0,1,0,0,
    1,1,1,0,
    0,0,0,0
  }, 
  { // 2  42
1,1,1,0,
    0,0,1,0,
    1,1,1,0,
    1,0,0,0,
    1,1,1,0,
    0,0,0,0
  },
  { // 3 43
1,1,1,0,
    0,0,1,0,
    0,1,1,0,
    0,0,1,0,
    1,1,1,0,
    0,0,0,0
  },
  { // 4  44
1,0,1,0,
    1,0,1,0,
    1,1,1,0,
    0,0,1,0,
    0,0,1,0,
    0,0,0,0
  },
  { // 5  45
1,1,1,0,
    1,0,0,0,
    1,1,1,0,
    0,0,1,0,
    1,1,1,0,
    0,0,0,0
  },
  { // 6 46
1,1,1,0,
    1,0,0,0,
    1,1,1,0,
    1,0,1,0,
    1,1,1,0,
    0,0,0,0
  },
  { // 7  47
1,1,1,0,
    0,0,1,0,
    0,0,1,0,
    0,0,1,0,
    0,0,1,0,
    0,0,0,0
  },
  { // 8  48
    1,1,1,0,
    1,0,1,0,
    1,1,1,0,
    1,0,1,0,
    1,1,1,0,
    0,0,0,0
  },
  { // 9 49
1,1,1,0,
    1,0,1,0,
    1,1,1,0,
    0,0,1,0,
    0,0,1,0,
    0,0,0,0
  },
  { // : 59
0,0,0,0,
    0,1,0,0,
    0,0,0,0,
    0,1,0,0,
    0,0,0,0,
    0,0,0,0
  },
  { // ; 60
0,0,0,0,
    0,1,0,0,
    0,0,0,0,
    0,1,0,0,
    1,0,0,0,
    0,0,0,0
  },
  { // < 4 
0,0,1,0,
    0,1,0,0,
    1,0,0,0,
    0,1,0,0,
    0,0,1,0,
    0,0,0,0
  },
  { // = 11
0,0,0,0,
    1,1,1,0,
    0,0,0,0,
    1,1,1,0,
    0,0,0,0,
    0,0,0,0
  },
  { // > 3
1,0,0,0,
    0,1,0,0,
    0,0,1,0,
    0,1,0,0,
    1,0,0,0,
    0,0,0,0
  },
  { // ? 2 
1,1,1,0,
    0,0,1,0,
    0,1,1,0,
    0,0,0,0,
    0,1,0,0,
    0,0,0,0
  },
  { // @
    0,1,0,0,
    1,0,1,0,
    1,0,1,0,
    1,0,0,0,
    0,1,1,0,
    0,0,0,0
  },
  
  // CAPITAL LETTERS 
  { // A 15
    1,1,1,0,
    1,0,1,0,
    1,1,1,0,
    1,0,1,0,
    1,0,1,0,
    0,0,0,0
  },
  { // B 16
    1,1,1,0,
    1,0,1,0,
    1,1,0,0,
    1,0,1,0,
    1,1,1,0,
    0,0,0,0
  },
  { // C 17
0,1,1,0,
    1,0,0,0,
    1,0,0,0,
    1,0,0,0,
    0,1,1,0,
    0,0,0,0
  },
  { // D 18
1,1,0,0,
    1,0,1,0,
    1,0,1,0,
    1,0,1,0,
    1,1,1,0,
    0,0,0,0
  },
  { // E 19
1,1,1,0,
    1,0,0,0,
    1,1,0,0,
    1,0,0,0,
    1,1,1,0,
    0,0,0,0
  },
  { // F 20
    1,1,1,0,
    1,0,0,0,
    1,1,0,0,
    1,0,0,0,
    1,0,0,0,
    0,0,0,0
  },
  { // G 21
0,1,1,0,
    1,0,0,0,
    1,0,0,0,
    1,0,1,0,
    1,1,1,0,
    0,0,0,0
  },
  { // H 22
1,0,1,0,
    1,0,1,0,
    1,1,1,0,
    1,0,1,0,
    1,0,1,0,
    0,0,0,0
  },
  { // I 23 
1,1,1,0,
    0,1,0,0,
    0,1,0,0,
    0,1,0,0,
    1,1,1,0,
    0,0,0,0
  }, 
  { // J 24
1,1,1,0,
    0,1,0,0,
    0,1,0,0,
    0,1,0,0,
    1,1,0,0,
    0,0,0,0
  },
  { // K 25
1,0,1,0,
    1,0,1,0,
    1,1,0,0,
    1,0,1,0,
    1,0,1,0,
    0,0,0,0
  },
  { // L 26
1,0,0,0,
    1,0,0,0,
    1,0,0,0,
    1,0,0,0,
    1,1,1,0,
    0,0,0,0
  },
  { // M 27
1,1,1,0,
    1,1,1,0,
    1,0,1,0,
    1,0,1,0,
    1,0,1,0,
    0,0,0,0
  },
  { // N 28
1,1,0,0,
    1,0,1,0,
    1,0,1,0,
    1,0,1,0,
    1,0,1,0,
    0,0,0,0
  },
  { // O 29
0,1,1,0,
    1,0,1,0,
    1,0,1,0,
    1,0,1,0,
    1,1,0,0,
    0,0,0,0
  },
  { // P 30
1,1,1,0,
    1,0,1,0,
    1,1,1,0,
    1,0,0,0,
    1,0,0,0,
    0,0,0,0
  },
  { // Q 31
0,1,0,0,
    1,0,1,0,
    1,0,1,0,
    1,1,0,0,
    0,1,1,0,
    0,0,0,0
  },
  { // R 32
1,1,1,0,
    1,0,1,0,
    1,1,0,0,
    1,0,1,0,
    1,0,1,0,
    0,0,0,0
  },
  { // S 33
0,1,1,0,
    1,0,0,0,
    1,1,1,0,
    0,0,1,0,
    1,1,0,0,
    0,0,0,0
  },
  { // T 34
1,1,1,0,
    0,1,0,0,
    0,1,0,0,
    0,1,0,0,
    0,1,0,0,
    0,0,0,0
  },
  { // U 35
1,0,1,0,
    1,0,1,0,
    1,0,1,0,
    1,0,1,0,
    0,1,1,0,
    0,0,0,0
  },
  { // V 36
1,0,1,0,
    1,0,1,0,
    1,0,1,0,
    1,0,1,0,
    0,1,0,0,
    0,0,0,0
  },
  { // W 37
1,0,1,0,
    1,0,1,0,
    1,1,1,0,
    1,1,1,0,
    1,0,1,0,
    0,0,0,0
  },
  { // X 38
1,0,1,0,
    1,0,1,0,
    0,1,0,0,
    1,0,1,0,
    1,0,1,0,
    0,0,0,0
  },
  { // Y 39
1,0,1,0,
    1,0,1,0,
    1,1,1,0,
    0,0,1,0,
    1,1,1,0,
    0,0,0,0
  },
  { // Z 40 
1,1,1,0,
    0,0,1,0,
    0,1,0,0,
    1,0,0,0,
    1,1,1,0,
    0,0,0,0
  },
  { // [ 53
    1,1,0,0,
    1,0,0,0,
    1,0,0,0,
    1,0,0,0,
    1,1,0,0,
    0,0,0,0
  },
  { /* \ */
    1,0,0,0,
    0,1,0,0,
    0,1,0,0,
    0,1,0,0,
    0,0,1,0,
    0,0,0,0
  },
  { // ] 54
    0,1,1,0,
    0,0,1,0,
    0,0,1,0,
    0,0,1,0,
    0,1,1,0,
    0,0,0,0
  },
  { // ^
    0,1,0,0,
    1,0,1,0,
    0,0,0,0,
    0,0,0,0,
    0,0,0,0,
    0,0,0,0
  },
  { // _
    0,0,0,0,
    0,0,0,0,
    0,0,0,0,
    0,0,0,0,
    1,1,1,0,
    0,0,0,0
  },
  { // `
    1,0,0,0,
    0,1,0,0,
    0,0,0,0,
    0,0,0,0,
    0,0,0,0,
    0,0,0,0
  },

  // SMALL LETTERS 
  { // A 15
0,0,0,0,
    0,1,1,0,
    1,0,1,0,
    1,0,1,0,
    0,1,1,0,
    0,0,0,0
  },
  { // B 16
1,0,0,0,
    1,1,0,0,
    1,0,1,0,
    1,0,1,0,
    1,1,0,0,
    0,0,0,0
  },
  { // C 17
0,0,0,0,
    0,1,1,0,
    1,0,0,0,
    1,0,0,0,
    0,1,1,0,
    0,0,0,0
  },
  { // D 18
0,0,1,0,
    0,1,1,0,
    1,0,1,0,
    1,0,1,0,
    0,1,1,0,
    0,0,0,0
  },
  { // E 19
0,0,0,0,
    0,1,0,0,
    1,0,1,0,
    1,1,0,0,
    0,1,1,0,
    0,0,0,0
  },
  { // F 20
0,0,1,0,
    0,1,0,0,
    0,1,1,0,
    0,1,0,0,
    0,1,0,0,
    0,0,0,0
  },
  { // G 21
    0,0,0,0,
    0,1,0,0,
    1,0,1,0,
    0,1,1,0,
    0,0,1,0,
    0,1,0,0
  },
  { // H 22
1,0,0,0,
    1,0,0,0,
    1,1,0,0,
    1,0,1,0,
    1,0,1,0,
    0,0,0,0
  },
  { // I 23 
0,1,0,0,
    0,0,0,0,
    0,1,0,0,
    0,1,0,0,
    0,1,0,0,
    0,0,0,0
  }, 
  { // J 24
    0,0,0,0,
    0,1,0,0,
    0,0,0,0,
    0,1,0,0,
    0,1,0,0,
    1,1,0,0
  },
  { // K 25
1,0,0,0,
    1,0,1,0,
    1,1,0,0,
    1,0,1,0,
    1,0,1,0,
    0,0,0,0
  },
  { // L 26
0,1,0,0,
    0,1,0,0,
    0,1,0,0,
    0,1,0,0,
    0,0,1,0,
    0,0,0,0
  },
  { // M 27
0,0,0,0,
    1,1,1,0,
    1,1,1,0,
    1,0,1,0,
    1,0,1,0,
    0,0,0,0
  },
  { // N 28
0,0,0,0,
    1,1,0,0,
    1,0,1,0,
    1,0,1,0,
    1,0,1,0,
    0,0,0,0
  },
  { // O 29
0,0,0,0,
    0,1,0,0,
    1,0,1,0,
    1,0,1,0,
    0,1,0,0,
    0,0,0,0
  },
  { // P 30
0,0,0,0,
    1,1,0,0,
    1,0,1,0,
    1,1,0,0,
    1,0,0,0,
    1,0,0,0
  },
  { // Q 31
0,0,0,0,
    0,1,1,0,
    1,0,1,0,
    0,1,1,0,
    0,0,1,0,
    0,0,1,0
  },
  { // R 32
0,0,0,0,
    0,0,1,0,
    0,1,0,0,
    0,1,0,0,
    0,1,0,0,
    0,0,0,0
  },
  { // S 33
0,0,0,0,
    0,1,1,0,
    1,1,0,0,
    0,0,1,0,
    1,1,0,0,
    0,0,0,0
  },
  { // T 34
0,1,0,0,
    1,1,1,0,
    0,1,0,0,
    0,1,0,0,
    0,0,1,0,
    0,0,0,0
  },
  { // U 35
0,0,0,0,
    1,0,1,0,
    1,0,1,0,
    1,0,1,0,
    0,1,1,0,
    0,0,0,0
  },
  { // V 36
0,0,0,0,
    1,0,1,0,
    1,0,1,0,
    1,0,1,0,
    0,1,0,0,
    0,0,0,0
  },
  { // W 37
0,0,0,0,
    1,0,1,0,
    1,0,1,0,
    1,1,1,0,
    1,1,1,0,
    0,0,0,0
  },
  { // X 38
0,0,0,0,
    1,0,1,0,
    0,1,0,0,
    0,1,0,0,
    1,0,1,0,
    0,0,0,0
  },
  { // Y 39
    0,0,0,0,
    1,0,1,0,
    1,0,1,0,
    0,1,1,0,
    0,0,1,0,
    0,1,0,0
  },
  { // Z 40 
0,0,0,0,
    1,1,1,0,
    0,0,1,0,
    1,0,0,0,
    1,1,1,0,
    0,0,0,0
  },

  { // { 55
0,1,1,0,
    0,1,0,0,
    1,1,0,0,
    0,1,0,0,
    0,1,1,0,
    0,0,0,0
  },
  { // |  12
0,1,0,0,
    0,1,0,0,
    0,1,0,0,
    0,1,0,0,
    0,1,0,0,
    0,0,0,0
  },
  { // } 56
1,1,0,0,
    0,1,0,0,
    0,1,1,0,
    0,1,0,0,
    1,1,0,0,
    0,0,0,0
  },
  { // ~  12
0,0,0,0,
    0,0,1,0,
    1,1,1,0,
    1,0,0,0,
    0,0,0,0,
    0,0,0,0
  },
  { // cursor
    1,1,1,0,
    1,1,1,0,
    1,1,1,0,
    1,1,1,0,
    1,1,1,0,
    0,0,0,0
  }
};

// ------------------
//  NANO-8 COLORS [16]
// ------------------

/*static Color nano8Color[COLORCOUNT] = {
  {0, 0, 0, 255},      // 0 BLACK 
  {29, 43, 83, 255},   // 1 NAVY BLUE 
  {126, 37, 83, 255},  // 2 DARK PURPLE
  {0, 135, 81, 255},   // 3 DARK GREEN 
  {171, 82, 54, 255},  // 4 BROWN 
  {95, 87, 79, 255},   // 5 DARK GRAY
  {194, 195, 199, 255},// 6 LIGHT GRAY 
  {255, 241, 232, 255},// 7 WHITE-ISH
  
  {255, 0, 77, 255},   // 8 RED 
  {255, 163, 0, 255},  // 9 ORANGE 
  {255, 236, 39, 255}, // 10 YELLOW 
  {0, 228, 54, 255},   // 11 GREEN 
  {41, 173, 255, 255}, // 12 BLUE 
  {131, 118, 156, 255},// 13 PURPLE-GRAY
  {255, 119, 168, 255},// 14 PINK
  {255, 204, 170, 255} // 15 PEACH
};*/

static Color nano8Color[COLORCOUNT] = {
  {20, 12, 28, 255},      // 0 BLACK 
  {68, 36, 52, 255},   // 1 NAVY BLUE 
  {48, 52, 109, 255},  // 2 DARK PURPLE
  {78, 74, 78, 255},   // 3 DARK GREEN 
  {133, 76, 48, 255},  // 4 BROWN 
  {52, 101, 36, 255},   // 5 DARK GRAY
  {210, 170, 153, 255},   // 6 LIGHT GRAY  
  {222, 238, 214, 255}, // 7 WHITE-ISH
  
  {208, 70, 72, 255},   // 8 RED 
  {210, 125, 44, 255},  // 9 ORANGE 
  {133, 149, 161, 255}, // 10 YELLOW 
  {109, 170, 44, 255},   // 11 GREEN 
  {89, 125, 206, 255}, // 12 BLUE 
  {109, 194, 202, 255},// 13 PURPLE-GRAY
  {218, 212, 84, 255},// 14 PINK
  {117, 113, 97, 255} // 15 PEACH 
};
static Color nano8ColorDefault[COLORCOUNT]  = {
  {20, 12, 28, 255},      // 0 BLACK 
  {68, 36, 52, 255},   // 1 NAVY BLUE 
  {48, 52, 109, 255},  // 2 DARK PURPLE
  {78, 74, 78, 255},   // 3 DARK GREEN 
  {133, 76, 48, 255},  // 4 BROWN 
  {52, 101, 36, 255},   // 5 DARK GRAY
  {210, 170, 153, 255},   // 6 LIGHT GRAY  
  {222, 238, 214, 255}, // 7 WHITE-ISH
  
  {208, 70, 72, 255},   // 8 RED 
  {210, 125, 44, 255},  // 9 ORANGE 
  {133, 149, 161, 255}, // 10 YELLOW 
  {109, 170, 44, 255},   // 11 GREEN 
  {89, 125, 206, 255}, // 12 BLUE 
  {109, 194, 202, 255},// 13 PURPLE-GRAY
  {218, 212, 84, 255},// 14 PINK
  {117, 113, 97, 255} // 15 PEACH 
};

// ------------------
//  ICONS
// ------------------
static uint8_t icons[ICONSCOUNT][TILESIZE*TILESIZE] = { 
  { // code section icon 0 
    0x00, 0x3C, 0x42, 0x00, 0x00, 0x42, 0x3C, 0x00
  },
  { // sprite section icon 1 
    0x00, 0x7E, 0x76, 0x5E, 0x5E, 0x76, 0x7E, 0x00
  },
  { // maps section icon 2 
    0x00, 0x6E, 0x6E, 0x0E, 0x60, 0x6E, 0x6E, 0x00
  },
  { // sfx section icon 3
    0x00, 0x18, 0x3C, 0x24, 0x42, 0x42, 0x7E, 0x00
  },
  { // mucic section icon 4
    0x00, 0x20, 0x70, 0x70, 0x3E, 0x02, 0x04, 0x00
  },
  { // pensil icon 5
    0x00, 0x70, 0x48, 0x5C, 0x3E, 0x1E, 0x0E, 0x00
  },
  { // select sprite icon 6
    0x00, 0x5A, 0x00, 0x42, 0x42, 0x00, 0x5A, 0x00
  },
  { // move sprite 7
    0x00, 0x3C, 0x70, 0x7E, 0x7E, 0x7E, 0x38, 0x00
  },
  { // fill icon 8
    0x00, 0x70, 0x10, 0x30, 0x74, 0x38, 0x10, 0x00
  },
  { // shape icon (Circle) 9
    0x00, 0x18, 0x24, 0x42, 0x42, 0x24, 0x18, 0x00
  },
  { // shape icon (square) 10
    0x00, 0x7E, 0x42, 0x42, 0x42, 0x42, 0x7E, 0x00
  },
  { // shape icon (line) 11
    0x00, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x00
  },
  { // zoom icon 12
    0x00, 0x66, 0x42, 0x00, 0x00, 0x42, 0x66, 0x00
  },
  { // right arrow 13
    0x00, 0x00, 0x7E, 0x3C, 0x18, 0x00, 0x00, 0x00
  },
  { // left arrow 14
    0x00, 0x00, 0x00, 0x18, 0x3C, 0x7E, 0x00, 0x00
  },
  { // sprite editor page icon 15
    0x00, 0x5E, 0x52, 0x52, 0x5E, 0x00, 0x1A, 0x00
  },
  { // sprite viewer page icon 16
    0x00, 0x7E, 0x2A, 0x7E, 0x2A, 0x7E, 0x2A, 0x00
  },
  { // trash icon 17
    0x00, 0x04, 0x7C, 0x46, 0x46, 0x7C, 0x04, 0x00
  },
  { // sfx icon 18
    0x00, 0x60, 0x70, 0x60, 0x78, 0x70, 0x7C, 0x00
  },
  { // sfx 2dn icon 19
    0x00, 0x6A, 0x00, 0x6A, 0x00, 0x6E, 0x6E, 0x00
  },
  { // waveform triangle 20
    0x00, 0x30, 0x0C, 0x02, 0x02, 0x0C, 0x30, 0x00
  },
  { // waveform tilted saw 21
    0x00, 0x20, 0x10, 0x0C, 0x02, 0x0E, 0x30, 0x00
  },
  { // waveform wave 22
    0x00, 0x40, 0x20, 0x10, 0x08, 0x04, 0x7C, 0x00
  },
  { // waveform square 23
    0x00, 0x10, 0x10, 0x1C, 0x04, 0x04, 0x1C, 0x00
  },
  { // waveform pulse 24
    0x00, 0x10, 0x10, 0x10, 0x10, 0x1C, 0x3C, 0x00
  },
  { // waveform organ 25
    0x00, 0x30, 0x18, 0x0C, 0x10, 0x08, 0x3C, 0x00
  },
  { // waveform noise 26
    0x00, 0x1E, 0x38, 0x7C, 0x30, 0x18, 0x7C, 0x00
  },
  { // waveform phaser 27
    0x00, 0x30, 0x38, 0x1C, 0x0E, 0x1C, 0x38, 0x00
  }
};

// ------------------
//  SCREEN BUFFER  
// ------------------
static int32_t Screen[SCREENWIDTH*SCREENHEIGHT];

void ClearScreen(int32_t color){
  for(int32_t i=0;i<SCREENWIDTH*SCREENHEIGHT;i++){
    Screen[i] = color; // reset all pixel colors
  }
}

// set the color of a screen pixel 
void pset(int32_t x, int32_t y, int32_t colorIndex){
  x -= camera.x;
  y -= camera.y;
  if(x < 0 || x >= SCREENWIDTH || y < 0 || y >= SCREENHEIGHT) return;
  Screen[y * SCREENWIDTH + x] = colorIndex;
}

// get the color of a screen pixel 
int32_t pget(int32_t x, int32_t y){
  return Screen[y * SCREENWIDTH + x];
}

void DrawScreen(void){
  for(int32_t i=0;i<SCREENWIDTH*SCREENHEIGHT;i++){
    const int32_t x = (i % SCREENWIDTH) * SCREENSCALE;
    const int32_t y = (i / SCREENWIDTH) * SCREENSCALE;
    
    //DrawRectangleLines(x, y, SCREENSCALE, SCREENSCALE, GRAY);
    DrawRectangle(x, y, SCREENSCALE, SCREENSCALE, GetNanoColor(Screen[i]));
  }
}

void ScrollUpScreen(int32_t amount){
  for(int32_t y=amount;y<SCREENHEIGHT;y++){
    for(int32_t x=0;x<SCREENWIDTH;x++){
      int32_t i = y * SCREENWIDTH + x;
      Screen[(y - amount) * SCREENWIDTH + x] = Screen[i]; 
    }
  }

  // clear bottom area 
  for(int32_t y = SCREENHEIGHT - amount;y < SCREENHEIGHT;y++){
    for(int32_t x=0;x<SCREENWIDTH;x++){
      Screen[y * SCREENWIDTH + x] = 0;
    }
  }
}

void DrawScreenLine(int32_t posx1, int32_t posy1, int32_t posx2, int32_t posy2, int32_t colorIndex){
  int32_t dx = abs(posx2 - posx1);
  int32_t dy = abs(posy2 - posy1);

  int32_t sx = (posx1 < posx2) ? 1: -1; // step direction x
  int32_t sy = (posy1 < posy2) ? 1: -1; // setp direction y

  int err = dx - dy;

  while(1){
    //Screen[posy1 * SCREENWIDTH + posx1] = colorIndex;
    pset(posx1, posy1, colorIndex);
    if(posx1 == posx2 && posy1 == posy2) break;

    int32_t e2 = 2 * err;
    if(e2 > -dy){
      err -= dy;
      posx1 += sx;
    }
    if(e2 < dx) {
      err += dx;
      posy1 += sy;
    }
  } 
}

void DrawRectFill(int32_t x, int32_t y, int32_t width, int32_t height, int32_t colorIndex){
  for(int32_t i=0;i<height;i++){
    DrawScreenLine(x, y+i,x+width,y+i, colorIndex);
  }
}

void DrawCircFill(int32_t cx, int32_t cy, int32_t radius, int32_t colorIndex){
  for(int32_t y = -radius;y<=radius;y++){
    for(int32_t x = -radius;x<=radius;x++){
      if((x*x)+(y*y) <= radius*radius){
        pset(cx + x, cy + y, colorIndex);
      }
    }
  }
}

void DrawCirc(int32_t cx, int32_t cy, int32_t radius, int32_t colorIndex){
  for(int32_t y=-radius;y<=radius;y++){
    for(int32_t x=-radius;x<=radius;x++){
      int d = (x*x) + (y*y);
      if(d >= radius * radius - radius && d <= radius * radius + radius){
        pset(cx + x, cy + y, colorIndex);
      }
    }
  }
}

uint8_t sget(int32_t x, int32_t y){
  return GetSprite()[y * SPRITEWIDTH + x];
}

// for spr function
void DrawSpr(int32_t sprIndex, int32_t posx, int32_t posy, int32_t width, int32_t height){
  int32_t sx = sprIndex % (TILESIZE*2) * TILESIZE;
  int32_t sy = sprIndex / (TILESIZE*2) * TILESIZE;
  
  for(int32_t y=0;y<height*TILESIZE;y++){
    for(int32_t x=0;x<width*TILESIZE;x++){
      if(sget(sx + x, sy + y))
        pset(posx+x, posy+y, sget(sx + x, sy + y)); 
    }
  }
}

// draw section of the map 
void Map(int32_t celX, int32_t celY, int32_t sx, int32_t sy, int32_t celW, int32_t celH){
  for(int32_t my=0;my<celH;my++){
    for(int32_t mx=0;mx<celW;mx++){
      
      // map position 
      int32_t mapX = celX + mx;
      int32_t mapY = celY + my;

      // bounds check 
      if(mapX < 0 || mapX >= MAPWIDTH || mapY < 0 || mapY >= MAPHEIGHT) continue;

      // tile stored in map 
      int32_t tile = mapData[mapY * MAPWIDTH + mapX];

      // screen position 
      int32_t drawX = sx + (mx * TILESIZE);
      int32_t drawY = sy + (my * TILESIZE);
      if(drawX <= -TILESIZE || drawX >= SCREENWIDTH) continue;
      if(drawY <= -TILESIZE || drawY >= SCREENHEIGHT) continue;
      DrawSpr(tile, drawX, drawY, 1, 1);
    }
  }
}

// get a tile from a map 
int32_t MGet(int32_t x, int32_t y){
  return mapData[y * MAPWIDTH + x];
}

// set a tile in the map 
void MSet(int32_t x, int32_t y, int32_t tile){
  if(tile >= 0 && tile < SPRITEWIDTH*SPRITEWIDTH)
    mapData[y * MAPWIDTH + x] = tile;
}

// get a sprite flag
bool FGet(uint8_t sprite, uint8_t flag){
  return spriteFlags[sprite].flags[flag];
}

// set a sprite flag 
void FSet(int32_t sprite, int32_t flag, int32_t value){
  spriteFlags[sprite].flags[flag] = value;
}

// move the camera offset
void NanoCamera(int32_t x, int32_t y){
  camera.x = x;
  camera.y = y;
}

void NanoCameraReset(void){
  camera.x = 0;
  camera.y = 0;
}

// replace colors globally (used for effects like night mode, damage, flash, etch.)
void Pal(int32_t oldColor, int32_t newColor){
  if(!oldColor && !newColor)
    memcpy(nano8Color, nano8ColorDefault, 16);
  else 
    nano8Color[oldColor] = nano8Color[newColor];
}

// marks which colors are transparent
void Palt(int32_t color, bool set, bool reset){
  if(reset)
    memcpy(nano8Color, nano8ColorDefault, 16);
  else 
    if(set)
      nano8Color[color] = nano8ColorDefault[0];
    else
      nano8Color[color] = nano8ColorDefault[color];
}

void Sspr(int32_t sx, int32_t sy, int32_t sw, int32_t sh, int32_t dx, int32_t dy, int32_t dw, int32_t dh){
  for(int32_t y=0;y<dh;y++){
    for(int32_t x=0;x<dw;x++){
      // map destination pixel -> source pixel 
      int32_t srcX = sx + (x * sw) / dw;
      int32_t srcY = sy + (y * sh) / dh;

      // read from spritesheet 
      uint8_t color = GetSprite()[srcY * SPRITEWIDTH + srcX];
      if(color)
        pset(dx + x, dy + y, color);
    }
  }
}

// ------------------
//  GAME SYSTEM
// ------------------
void GameRunning(bool run){
  running = run;
}

bool GameIsRunning(void){
  return running;
}

uint8_t *GetFontText(const uint8_t fontIndex){
  return font_texts[fontIndex];
}

Color GetNanoColor(int32_t colorIndex){
  return nano8Color[colorIndex];
}

void SetNanoColor(int32_t colorIndex, int32_t r, int32_t g, int32_t b){
  nano8Color[colorIndex].r = r;
  nano8Color[colorIndex].g = g;
  nano8Color[colorIndex].b = b;

  nano8ColorDefault[colorIndex].r = r;
  nano8ColorDefault[colorIndex].g = g;
  nano8ColorDefault[colorIndex].b = b;
}

int8_t GetTextCurrentColor(void){
  return textCurrentColor;
}

void ChangeTextCurrentColor(int32_t colorIndex){
  textCurrentColor = colorIndex;
}

// ------------------
//  CONSOLE CURSOR
// ------------------
Vector2 cursorPosition = {FONTWIDTH, SCREENSCALE};
Vector2 GetCursorPosition(void){
  return cursorPosition;
}

void SetCursorPosition(Vector2 position){
  cursorPosition.x = position.x;
  cursorPosition.y = position.y;
}

void DrawIcons(uint32_t iconIndex, Vector2 position, int32_t colorIndex){ 
  
  for(int32_t y=0;y<TILESIZE;y++){
    unsigned char row = icons[iconIndex][y];
    for(int32_t x=0;x<TILESIZE;x++){
      int32_t bit = (row >> (7-x)) & 1;

      if(bit){
        int32_t newX = y;
        int32_t newY = 7-x;
        DrawRectangle(
          (position.x+newX)*SCREENSCALE, 
          (position.y+newY)*SCREENSCALE, 
          SCREENSCALE, 
          SCREENSCALE, 
          GetNanoColor(colorIndex)
        );
      }
    }
  }
}
