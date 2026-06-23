#include "draw.h"
#include <string.h>
#include <stdio.h>
#include "font.h"

#include "game.h"

// NOTE: WHAT THE HELL IS THIS?

// TODO: check this, the limit is 1024?
// drawTexts and textCount are internal to this translation unit; mark static
// to avoid exporting symbols that can collide with other object files.
static DrawTexts drawTexts[1024];
static int textCount = 0;

// ------------------
//  TEXTS
// ------------------
void _DrawText(const char* text, Vector2 position, int colorIndex){
  if(textCount >= (int)(sizeof(drawTexts)/sizeof(drawTexts[0]))) return;
  DrawTexts *t = &drawTexts[textCount++];
  // copy safely
  strncpy(t->buffer, text, sizeof(t->buffer) - 1);
  t->buffer[sizeof(t->buffer) - 1] = '\0';
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
