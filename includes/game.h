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

// ------------------
//  COLORS [16]
// ------------------
#define COLORCOUNT 16

// ------------------
//  TEXT/FONT CHARACTERS
// ------------------
#define FONTCOUNT 96
#define FONTWIDTH 4 
#define FONTHEIGHT 6

void GameRunning(bool run);
bool GameIsRunning(void);
uint32_t *GetFontText(const int32_t fontIndex);
Color GetNanoColor(int32_t colorIndex);
void ChangeTextCurrentColor(int32_t colorIndex);
int32_t GetTextCurrentColor(void);

void ClearScreen(int32_t color);
void pset(int32_t x, int32_t y, int32_t colorIndex);
void DrawScreen(void);
int GetPixelScreenColor(int32_t x, int32_t y);
void DrawScreenLine(int32_t posx1, int32_t posy1, int32_t posx2, int32_t posy2, int32_t colorIndex);
void DrawRectFill(int32_t x, int32_t y, int32_t width, int32_t height, int32_t colorIndex);
void DrawCircFill(int32_t cx, int32_t cy, int32_t radius, int32_t colorIndex);
void DrawCirc(int32_t cx, int32_t cy, int32_t radius, int32_t colorIndex);

Vector2 GetCursorPosition(void);
void SetCursorPosition(Vector2 position);

#endif
