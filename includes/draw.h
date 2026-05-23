#ifndef DRAW_H
#define DRAW_H

#include <raylib.h>

typedef struct{
  char buffer[256];
  Vector2 position;
  int colorIndex;
} DrawTexts;

void _DrawText(const char* text, Vector2 position, int colorIndex);
int GetTextCount(void);
DrawTexts *GetText(int index);
void ClearText(void);

#endif
