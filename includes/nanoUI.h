#ifndef NanoUI_H
#define NanoUI_H

#include <stdint.h>
#include <raylib.h>

typedef struct{
  Rectangle rect;
  int32_t labelType;
  char textLabel[32];
  int32_t iconLabel;

  bool hovered;
  bool pressed;
  bool clicked;
  bool rpressed;
  bool rclicked;
  bool active;
  bool border;

  int32_t id;
} NanoButton;

void InitNanoButtonText(NanoButton *button, Rectangle rect, const char* textLabel, bool border);
void InitNanoButtonIcon(NanoButton *button, Rectangle rect, const char* textLabel, const int32_t icon, bool border);
void UpdateNanoButton(NanoButton *button);
void DrawNanoButton(NanoButton *button);

void DrawTextUI(const char* text, Vector2 position);

#endif
