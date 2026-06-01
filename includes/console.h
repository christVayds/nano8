#ifndef CONSOLE_H
#define CONSOLE_H

#include <raylib.h>

typedef struct{
  char buffer[256]; // what user has type 
  int cursor;       // cursor position 
  char command[256]; // command 
  bool newCommand;  // check new command
} Console;

Console InitConsole(void);
void CloseConsole();
void UpdateConsole(Console *console);
void InputConsole(Console *console);
void DrawConsole(Console *console);

bool CallLuaFunction(const char* funcname);
#endif
