#include "font.h"
#include <stdio.h>

// FOR SCREEN BUFFER
void PrintTextScreen(const char* text, Vector2 *position){
  const int32_t posx = (int32_t)position->x;

  int32_t index = 0;
  while(true){
    if(text[index] != '\0'){
      char c = text[index];
      int32_t fontIndex = c-32;

      if(c == '\n'){
        position->y += FONTHEIGHT;
        position->x = posx;
        fontIndex = -1;
      }

      if(fontIndex >= 0){
        GetFont(fontIndex, *position, false, true);
        position->x += FONTWIDTH;
      }
    } else {
      return;
    }
    index++;
  }
}

// PRINTING TEXT FOR CONSOLE
void PrintText(const char* text, Vector2 *position){
  const int32_t posx = (int32_t)position->x;
  
  int32_t index = 0;
  while(true){ 
    if(text[index] != '\0'){
      char c = text[index];
      int32_t fontIndex = c - 32;
      
      if(c == '\n'){
        position->y += FONTHEIGHT;
        position->x = posx;
        fontIndex = -1;
      }

      if(position->x > SCREENWIDTH - FONTHEIGHT){
        position->y += FONTHEIGHT;
        position->x = posx;
      }

      if(fontIndex >= 0){
        GetFont(fontIndex, *position, true, true);
        position->x += FONTWIDTH;
      }
    } else {
      GetFont(0, *position, true, true);
      //position->x += FONTWIDTH;
      return;
    }
    index++;
  } 
} 

void GetFont(const int32_t fontIndex, Vector2 position, bool visibleBlack, bool drawpset){
  const int32_t posx = position.x;
  for(int32_t y=0;y<FONTHEIGHT;y++){
    for(int32_t x=0;x<FONTWIDTH;x++){
      int32_t index = y * FONTWIDTH + x;
      uint32_t text = GetFontText(fontIndex)[index];
      
      if(text > 0)
        if(drawpset)
          pset(position.x, position.y, GetTextCurrentColor());
        else
          DrawRectangle(position.x * SCREENSCALE, position.y * SCREENSCALE, SCREENSCALE, SCREENSCALE, GetNanoColor(GetTextCurrentColor()));
      else
        if(visibleBlack)
          pset(position.x, position.y, 0);
      position.x += 1;
    } 
    position.x = posx;
    position.y += 1;
  }
}
