#ifndef FONT_H
#define FONT_H

#include <stdint.h>
#include <raylib.h>
#include "game.h"

void GetFont(const int32_t fontIndex, Vector2 position, bool visibleBlack, bool drawpset);
void PrintText(const char* text, Vector2 *position);

void PrintTextScreen(const char* text, Vector2 *position);
#endif
