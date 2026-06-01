#include "draw.h"
#include <string.h>
#include <stdio.h>
#include "font.h"

#include "game.h"

// NOTE: WHAT THE HELL IS THIS?

// TODO: check this, the limit is 1024?
DrawTexts drawTexts[1024];
int textCount = 0;

// ------------------
//  TEXTS
// ------------------
void _DrawText(const char* text, Vector2 position, int colorIndex){
  DrawTexts *t = &drawTexts[textCount++];
  strcpy(t->buffer, text);
  t->position.x = position.x;
  t->position.y = position.y;
  t->colorIndex = colorIndex;
}

int GetTextCount(void){
  return textCount;
}

DrawTexts *GetText(int index){
  return &drawTexts[index];
}

void ClearText(void){
  textCount = 0;
}
