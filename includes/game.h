#ifndef GAME_H
#define GAME_H

#include <stdint.h>
#include <raylib.h>

// ------------------
//  SYSTEM
// ------------------
#define SCREENWIDTH 128       // screen actual width 
#define SCREENHEIGHT 128      // screen actual height 
#define SCREENSCALE 4         // Window scale 
#define MAXFPS 30             // max fps
#define TILESIZE 8
#define NANO8_VERSION_NAME "v0.0.1"

// MAPS
#define MAPWIDTH 128
#define MAPHEIGHT 128

// SPRITE 
#define SPRITEWIDTH 128
#define SPRITEHEIGHT 128
#define FLAGCOUNT 7

// ------------------
//  COLORS [16]
// ------------------
#define COLORCOUNT 16
#define ICONSCOUNT 28

// ------------------
//  TEXT/FONT CHARACTERS
// ------------------
#define FONTCOUNT 96
#define FONTWIDTH 4 
#define FONTHEIGHT 6

// SFX and MUSIC 
#define MAXCHANNELS 4

void GameRunning(bool run);
bool GameIsRunning(void);
uint8_t *GetFontText(const uint8_t fontIndex);
Color GetNanoColor(int32_t colorIndex);
void SetNanoColor(int32_t colorIndex, int32_t r, int32_t g, int32_t b);
void ChangeTextCurrentColor(int32_t colorIndex);
int8_t GetTextCurrentColor(void);

// for lua functions
void ClearScreen(int32_t color);
void pset(int32_t x, int32_t y, int32_t colorIndex);
int32_t pget(int32_t x, int32_t y);
uint8_t sget(int32_t x, int32_t y);
void DrawScreen(void);
void DrawScreenLine(int32_t posx1, int32_t posy1, int32_t posx2, int32_t posy2, int32_t colorIndex);
void DrawRectFill(int32_t x, int32_t y, int32_t width, int32_t height, int32_t colorIndex);
void DrawCircFill(int32_t cx, int32_t cy, int32_t radius, int32_t colorIndex);
void DrawCirc(int32_t cx, int32_t cy, int32_t radius, int32_t colorIndex);
void DrawSpr(int32_t sprIndex, int32_t posx, int32_t posy, int32_t width, int32_t height);
void Map(int32_t celX, int32_t celY, int32_t sx, int32_t sy, int32_t celW, int32_t celH);
void MSet(int32_t x, int32_t y, int32_t tile);
int32_t MGet(int32_t x, int32_t y);
bool FGet(uint8_t sprite, uint8_t flag);
void FSet(int32_t sprite, int32_t flag, int32_t value);
void NanoCamera(int32_t x, int32_t y);
void NanoCameraReset(void);
void Pal(int32_t oldColor, int32_t newColor);
void Palt(int32_t color, bool set, bool reset);
void Sspr(int32_t sx, int32_t sy, int32_t sw, int32_t sh, int32_t dx, int32_t dy, int32_t dw, int32_t dh);

Vector2 GetCursorPosition(void);
void SetCursorPosition(Vector2 position);
void ScrollUpScreen(int32_t amount);

void DrawIcons(uint32_t iconIndex, Vector2 position, int32_t colorIndex);

// reset console
void ClearConsole(void);

#endif
