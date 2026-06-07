#include "nanoUI.h"
#include "game.h"
#include "font.h"

#include <string.h>

void InitNanoButtonText(NanoButton *button, Rectangle rect, const char* textLabel, bool border){
  button->rect = rect;
  button->labelType = 1;
  strcpy(button->textLabel, textLabel);

  button->hovered = false;
  button->clicked = false;
  button->pressed = false;
  button->active = false;
  button->border = border;
}

void InitNanoButtonIcon(NanoButton *button, Rectangle rect, const char* textLabel, const int32_t icon, bool border){
  button->rect = rect;
  button->labelType = 0;
  button->iconLabel = icon;
  strcpy(button->textLabel, textLabel);

  button->hovered = false;
  button->clicked = false;
  button->pressed = false;
  button->active = false;
  button->border = border;
}

void UpdateNanoButton(NanoButton *button){
  Vector2 mousepos = GetMousePosition();

  button->hovered = CheckCollisionPointRec(mousepos, (Rectangle){button->rect.x*SCREENSCALE, button->rect.y*SCREENSCALE, button->rect.width*SCREENSCALE, button->rect.height*SCREENSCALE});
  button->pressed = button->hovered && IsMouseButtonDown(MOUSE_LEFT_BUTTON);
  button->clicked = button->hovered && IsMouseButtonReleased(MOUSE_LEFT_BUTTON);
  button->rpressed = button->hovered && IsMouseButtonDown(MOUSE_RIGHT_BUTTON);
  button->rclicked = button->hovered && IsMouseButtonReleased(MOUSE_RIGHT_BUTTON);
}

void DrawNanoButton(NanoButton *button){
  int8_t colorIndex = 6;
  if(button->active) colorIndex = 8;
  else if(button->hovered) colorIndex = 7;
  
  if(button->labelType){
    if(button->border)
      DrawRectangleLines(button->rect.x*SCREENSCALE, button->rect.y*SCREENSCALE, button->rect.width*SCREENSCALE, button->rect.height*SCREENSCALE, GetNanoColor(colorIndex));
    
    ChangeTextCurrentColor(colorIndex);
    DrawTextUI(button->textLabel, (Vector2){button->rect.x+1, button->rect.y+1});
    ChangeTextCurrentColor(6);
  }else{
    if(button->border)
      DrawRectangleLines(button->rect.x*SCREENSCALE, button->rect.y*SCREENSCALE, button->rect.width*SCREENSCALE, button->rect.height*SCREENSCALE, GetNanoColor(colorIndex));
    DrawIcons(button->iconLabel, (Vector2){button->rect.x, button->rect.y}, colorIndex);
  }
}

// Draw UI Texts
void DrawTextUI(const char* text, Vector2 position){
  const int32_t posx = (int32_t)position.x;

  int32_t index = 0;
  while(true){
    if(text[index] != '\0'){
      char c = text[index];
      int32_t fontIndex = c-32;

      if(c == '\n'){
        position.y += FONTHEIGHT;
        position.x = posx;
        fontIndex = -1;
      }

      if(fontIndex >= 0){
        GetFont(fontIndex, position, false, false);
        position.x += FONTWIDTH;
      }
    } else {
      return;
    }
    index++;
  }
}
