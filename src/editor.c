#include "editor.h"
#include "game.h"
#include "font.h"
#include "luaapi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// CODE EDITOR 
static bool cartRunning = false;
static int32_t tokenCount = 0;

// SECTIONS 
static Sections sections;

// indentation
static int32_t indentCount = 0;

// SELECTIONS / TEXT SELECTING 
int32_t select_start = 0;
int32_t select_end = 0;
bool selecting = false;
char clipboard[1028];

// back spacing
static float backSpaceHoldTime = 0.0f;
static float backSpaceRepeatTimer = 0.0f;
static bool backSpaceHeld = false;

// arrow keys
static float arrowKeyHoldTime = 0.0f;
static float arrowKeyRepeatTimer = 0.0f;
static bool arrowKeyHeldLeft = false;
static bool arrowKeyHeldRight = false;
static bool arrowKeyHeldUp = false;
static bool arrowKeyHeldDown = false;

// define static functions
static void DrawSomeText(char *texts, Vector2 position);
static void DrawTextEditor(Vector2 *position);
static void DrawFont(const int32_t fontIndex, Vector2 position);
static void InsertCharacter(int32_t pos, char c);
static void BackSpace(int32_t pos);
static void CountToken(void);
static void DrawSelection(void);

static int isKeyword(const char* str);
static int isFunc(const char* str);
static int isSymbol(const char c);
static const char* kw[] = {
  "local", "function", "end", "if", "then", "else", "elseif", "for", "while", "do", 
  "return", "break", "false", "true", "yanji", "nano8", "or", "and", "not",
  "K_A", "K_S", "K_Z", "K_X", "K_L", "K_R", "K_U", "K_D"
};

static const char* func[] = {
  "print", "_init", "_update", "_draw", "main", "rectfill", "rect", "circ", "circfill", "spr",
  "pset", "pget", "line", "btn", "btnp", "cls", "map", "mset", "mget", "fget", "fset", "camera", "cursor", "pal", "palt", 
  "flr", "abs", "ciel", "sqrt", "sin", "cos", "rand", "srand", "min", "max"
};

static const char symbols[] = {
  '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '-', '+', '=', '{', '}', '[', ']',
  ';', ':', '\'', ',', '.', '?', '<', '>', '/', '\\', '`', '~'
};

// NEW STATE LUA 
static lua_State *L_editor = NULL;

void InitEditor(void){  
  // INITIALIZE SECTIONS 
  InitSection();
}

void ResetLuaForEditor(void){
  L_editor = InitLuaState();
}

void CloseEditor(void){
  if(L_editor == NULL) return;
  lua_close(L_editor);
  L_editor = NULL;
}

lua_State *GetEditorLua(void){
  return L_editor;
}

// NOTE: can memory leak
char *GetLuaCode(void){  
  char *code = malloc(sizeof(char));
  int32_t codeSize = (int32_t)sizeof(char);
  if(!code) return NULL;

  code[0] = '\0';
  char *ptr = code;
  for(int32_t i=0;i<=sections.sectionsCount;i++){
    codeSize += sections.codeEditor[i].codeCount +2;
    code = realloc(code, codeSize * sizeof(char));
    if(!code) return NULL;
    ptr = code + strlen(code);

    // write 
    int write = sprintf(ptr, "%s\n", sections.codeEditor[i].code);
    ptr += write;
  }

  return code;
}

char *GetLuaCodeInSection(int32_t si){
  // si = section index
  char *code = malloc(sizeof(char) * sections.codeEditor[si].codeCount +2);
  if(!code) return NULL;

  code[0] = '\0';
  char *ptr = code;

  // write 
  int write = sprintf(ptr, "%s\n", sections.codeEditor[si].code);
  ptr += write;

  return code;
}

// load a lua code
void LoadCode(const char* luaCode, size_t size){
  CodeEditor *codeEditor = &sections.codeEditor[sections.sectionCurrent];
  
  memcpy(codeEditor->code + codeEditor->codeCount, luaCode, sizeof(char) * size);
  codeEditor->codeCount += size;
  codeEditor->code[codeEditor->codeCount] = '\0';
}

void UpdateEditor(void){
  
  // always count token
  CountToken();
}

// EDITOR UPDATE INPUT 
void InputEditor(void){
  CodeEditor *codeEditor = &sections.codeEditor[sections.sectionCurrent];
  
  // GET USER KEY INPUTS 
  int key = GetCharPressed();
  if(key >= 32 && tokenCount < TOKENLIMIT){
    InsertCharacter(codeEditor->cursor, key); 
    codeEditor->cursor++;
    selecting = false;
  } 

  // ---------------- 
  //  SECTIONS CONTROL
  // ---------------- 

  // TODO:
  // CONTROL SHIFT T to remove TAB/SECTION
  //if(IsKeyDown(KEY_LEFT_CONTROL) && IsKeyDown(KEY_LEFT_SHIFT) && IsKeyPressed(KEY_T)){
  //  if(sections.sectionsCount > 1) sections.sectionsCount--;
  //  sections.sectionCurrent = sections.sectionsCount;
  //} 

  // CONTROL + T to create new TAB/SECTION 
  else if(IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_T) && sections.sectionsCount+1 < MAXSECTIONS){
    NewSection();
    return;
  }

  // CONTROL RIGHT KEY - MOVE SECTIONS RIGHT SIDE
  if(IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_RIGHT)){
    if(sections.sectionCurrent < sections.sectionsCount)
      sections.sectionCurrent++;
    else sections.sectionCurrent=0;

    return;
  }
  // CONTROL LEFT KEY - MOVE SECTION LEFT SIDE 
  if(IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_LEFT)){
    if(sections.sectionCurrent > 0)
      sections.sectionCurrent--;
    else sections.sectionCurrent = sections.sectionsCount;

    return;
  }

  // ---------------- 
  //  EDITOR CONTROL
  // ---------------- 

  // SELECT TEXT 
  if(IsKeyDown(KEY_LEFT_SHIFT) && (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_UP))){
    if(selecting){
      select_end = codeEditor->cursor;
      //printf("start %d end %d\n", select_start, select_end);
    } else {
      select_start = codeEditor->cursor;
      select_end = codeEditor->cursor;
      selecting = true;
    }
  }

  // COPY TEXT 
  if(IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_C)){
    if(selecting){
      
      // normalize
      if(select_start > select_end){
        int temp = select_start;
        select_start = select_end;
        select_end = temp;
      }
      
      // copy
      int32_t i=0;
      for(int32_t k=select_start;k<select_end;k++){
        clipboard[i++] = codeEditor->code[k];
      }
      clipboard[i] = '\0';
      SetClipboardText(clipboard);          // push text to clipboard
      printf("clipboard %s\n", clipboard);
      selecting = false;
    }
  }

  // TODO: FIX THIS
  // CUT SELECTION 
  if(IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_X)){
    if(selecting){
      // normalize
      if(select_start > select_end){
        int temp = select_start;
        select_start = select_end;
        select_end = temp;
      }

      // copy 
      int32_t index=0;
      for(int32_t k=select_start;k<select_end;k++){
        clipboard[index++] = codeEditor->code[k];
      }
      clipboard[index] = '\0';
      SetClipboardText(clipboard);          // push text to clipboard

      // cut
      int32_t len = strlen(codeEditor->code);
      for(int32_t i=select_start;i<=len - (select_end - select_start);i++){
        codeEditor->code[i] = codeEditor->code[i + (select_end - select_start)];
      }
      codeEditor->code[len - (select_end - select_start)] = '\0';
      selecting = false;
    }
  }

  // PASTE TEXT 
  if(IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_V)){
    const char *clip = GetClipboardText();
    if(!clip) return;

    int32_t index = 0;
    while(clip[index] != '\0'){
      InsertCharacter(codeEditor->cursor, clip[index]); 
      codeEditor->cursor++;
      index++;
    }
  }

  // TEXT TAB (2 SPACES)
  if(IsKeyPressed(KEY_TAB) && tokenCount < TOKENLIMIT){
    for(int32_t i=0;i<TABSIZE;i++){ 
      InsertCharacter(codeEditor->cursor, ' ');
      codeEditor->cursor++;
    }
    indentCount += 2;
  }

  // ENTER 
  if(IsKeyPressed(KEY_ENTER) && tokenCount < TOKENLIMIT){
    InsertCharacter(codeEditor->cursor, '\n');
    codeEditor->cursor++;
    
    if(indentCount > codeEditor->cursorColumn) indentCount = codeEditor->cursorColumn-1;
    if(indentCount > 0){
      for(int32_t i=0;i<indentCount;i++){ 
        InsertCharacter(codeEditor->cursor, ' ');
        codeEditor->cursor++;
      }
    }
  }

  // BACKSPACE 
  if(IsKeyDown(KEY_BACKSPACE) && codeEditor->cursor >= 1){
    if(!backSpaceHeld){
      BackSpace(codeEditor->cursor);
      codeEditor->cursor--;
      
      backSpaceHoldTime = 0.0f;
      backSpaceRepeatTimer = 0.5f;
      backSpaceHeld = true;

      if(codeEditor->cursorColumn <= indentCount) indentCount--;
    } else {
      backSpaceHoldTime += GetFrameTime();
      backSpaceRepeatTimer -= GetFrameTime();

      if(backSpaceRepeatTimer <= 0.0f){
        BackSpace(codeEditor->cursor);
        codeEditor->cursor--;
        
        backSpaceRepeatTimer = 0.05f;

        if(codeEditor->cursorColumn <= indentCount) indentCount--;
      }
    }
    if(indentCount < 0) indentCount = 0;
  } else backSpaceHeld = false; 
  
  // ARROW KEYS
  if(IsKeyDown(KEY_LEFT) && codeEditor->cursor > 0){ 
    if(!arrowKeyHeldLeft){
      codeEditor->cursor--;
      arrowKeyHoldTime = 0.0f;
      arrowKeyRepeatTimer = 0.5f;
      arrowKeyHeldLeft = true;
    } else {
      arrowKeyHoldTime += GetFrameTime();
      arrowKeyRepeatTimer -= GetFrameTime();

      if(arrowKeyRepeatTimer <= 0.0f){
        codeEditor->cursor--;
        arrowKeyRepeatTimer = 0.05f;
      }
    }
  } else arrowKeyHeldLeft = false;
  
  if(IsKeyDown(KEY_RIGHT) && codeEditor->cursor < codeEditor->codeCount){
    if(!arrowKeyHeldRight){
      codeEditor->cursor++;
      arrowKeyHoldTime = 0.0f;
      arrowKeyRepeatTimer = 0.5f;
      arrowKeyHeldRight = true;
    } else {
      arrowKeyHoldTime += GetFrameTime();
      arrowKeyRepeatTimer -= GetFrameTime();

      if(arrowKeyRepeatTimer <= 0.0f){
        codeEditor->cursor++;
        arrowKeyRepeatTimer = 0.05f;
      }
    }
  } else arrowKeyHeldRight = false;

  if(IsKeyDown(KEY_UP)){
    int32_t col = (int32_t)codeEditor->cursorPosition.x;

    if(!arrowKeyHeldUp){
      // go to start of the current line 
      while(codeEditor->cursor > 0 && codeEditor->code[codeEditor->cursor -1] != '\n') codeEditor->cursor--;

      // jump over '\n' into prev line 
      if(codeEditor->cursor > 0) codeEditor->cursor--;

      // go to start of prev line 
      while(codeEditor->cursor > 0 && codeEditor->code[codeEditor->cursor - 1] != '\n') codeEditor->cursor--;

      for(int32_t i=0;i<col && codeEditor->code[codeEditor->cursor] != '\n' && codeEditor->code[codeEditor->cursor] != '\0';i++)
        codeEditor->cursor++;

      arrowKeyHoldTime = 0.0f;
      arrowKeyRepeatTimer = 0.5f;
      arrowKeyHeldUp = true;
    } else {
      arrowKeyHoldTime += GetFrameTime();
      arrowKeyRepeatTimer -= GetFrameTime();

      if(arrowKeyRepeatTimer <= 0.0f){
        // go to start of the current line 
        while(codeEditor->cursor > 0 && codeEditor->code[codeEditor->cursor -1] != '\n') codeEditor->cursor--;

        // jump over '\n' into prev line 
        if(codeEditor->cursor > 0) codeEditor->cursor--;

        // go to start of prev line 
        while(codeEditor->cursor > 0 && codeEditor->code[codeEditor->cursor - 1] != '\n') codeEditor->cursor--;

        for(int32_t i=0;i<col && codeEditor->code[codeEditor->cursor] != '\n' && codeEditor->code[codeEditor->cursor] != '\0';i++)
          codeEditor->cursor++;
        arrowKeyRepeatTimer = 0.05f;
      }
    } 
  } else arrowKeyHeldUp = false;

  if(IsKeyDown(KEY_DOWN)){
    int32_t col = codeEditor->cursorPosition.x;

    if(!arrowKeyHeldDown){
      // find the next '\n' (end of current line)
      while(codeEditor->code[codeEditor->cursor] != '\n' && codeEditor->code[codeEditor->cursor] != '\0') codeEditor->cursor++;

      // jump over '\n' into next line 
      if(codeEditor->code[codeEditor->cursor] == '\n') codeEditor->cursor++;

      // walk forward nu col (stop at end of line)
      for(int32_t i=0;i<col && codeEditor->code[codeEditor->cursor] != '\n' && codeEditor->code[codeEditor->cursor] != '\0';i++)
        codeEditor->cursor++;

      arrowKeyHoldTime = 0.0f;
      arrowKeyRepeatTimer = 0.5f;
      arrowKeyHeldDown = true;
    } else {
      arrowKeyHoldTime += GetFrameTime();
      arrowKeyRepeatTimer -= GetFrameTime();

      if(arrowKeyRepeatTimer <= 0.0f){
        // find the next '\n' (end of current line)
        while(codeEditor->code[codeEditor->cursor] != '\n' && codeEditor->code[codeEditor->cursor] != '\0') codeEditor->cursor++;

        // jump over '\n' into next line 
        if(codeEditor->code[codeEditor->cursor] == '\n') codeEditor->cursor++;

        // walk forward nu col (stop at end of line)
        for(int32_t i=0;i<col && codeEditor->code[codeEditor->cursor] != '\n' && codeEditor->code[codeEditor->cursor] != '\0';i++)
          codeEditor->cursor++;
        arrowKeyRepeatTimer = 0.05f;
      }
    } 
  } else arrowKeyHeldDown = false;
  
  // UPDATE CUROSOR POSITION 
  codeEditor->cursorPosition.x = 0;
  codeEditor->cursorPosition.y = 0;
  for(int32_t i=0;i<codeEditor->cursor;i++){
    if(codeEditor->code[i] == '\n'){
      codeEditor->cursorPosition.y++;
      codeEditor->cursorPosition.x = 0;
    } else {
      codeEditor->cursorPosition.x++;
    }
  }

  // UPDATE EDITOR SCROLL
  // scroll for X and Y
  codeEditor->cursorLine = 0;
  for(int32_t i=0;i<codeEditor->cursor;i++){
    if(codeEditor->code[i] == '\n') codeEditor->cursorLine++;
  }

  codeEditor->cursorColumn = 0;
  for(int32_t i=codeEditor->cursor-1;i>=0;i--){
    if(codeEditor->code[i] == '\n') break;
    codeEditor->cursorColumn++;
  }

  if(codeEditor->cursorLine >= codeEditor->scrollY + VISIBLELINES) codeEditor->scrollY = codeEditor->cursorLine - VISIBLELINES + 1;
  if(codeEditor->cursorLine < codeEditor->scrollY) codeEditor->scrollY = codeEditor->cursorLine;

  if(codeEditor->cursorColumn >= codeEditor->scrollX + VISIBLECOLUMNS) codeEditor->scrollX = codeEditor->cursorColumn - VISIBLECOLUMNS + 1;
  if(codeEditor->cursorColumn < codeEditor->scrollX) codeEditor->scrollX = codeEditor->cursorColumn; 
 
}

void DrawEditor(void){
  CodeEditor *codeEditor = &sections.codeEditor[sections.sectionCurrent];
  
  Vector2 position = {1, TILESIZE+1};

  // draw selected text 
  DrawSelection();

  // Cursor 
  //position.x += FONTWIDTH;
  int32_t cposx = 1 + -(codeEditor->scrollX * FONTWIDTH) + codeEditor->cursorPosition.x * FONTWIDTH;
  int32_t cposy = (TILESIZE+1+ -(codeEditor->scrollY * FONTHEIGHT)) + codeEditor->cursorPosition.y * FONTHEIGHT;
  ChangeTextCurrentColor(10);
  DrawFont(95, (Vector2){cposx, cposy});
  ChangeTextCurrentColor(6);

  // draw code 
  position.x += -(codeEditor->scrollX * FONTWIDTH);
  position.y += -(codeEditor->scrollY * FONTHEIGHT);
  DrawTextEditor(&position); 

  // draw UI top and bottom
  DrawRectangle(0, SCREENHEIGHT*SCREENSCALE - TILESIZE*SCREENSCALE, SCREENWIDTH*SCREENSCALE, TILESIZE*SCREENSCALE, GetNanoColor(0));
  DrawRectangle(0, 0, SCREENWIDTH*SCREENSCALE, TILESIZE*SCREENSCALE, GetNanoColor(0));
  DrawRectangleLines(0, 0, (SCREENWIDTH+1)*SCREENSCALE, TILESIZE*SCREENSCALE, GetNanoColor(8));
  DrawRectangleLines(0, SCREENHEIGHT*SCREENSCALE - TILESIZE*SCREENSCALE, (SCREENWIDTH+1)*SCREENSCALE, TILESIZE*SCREENSCALE+1, GetNanoColor(8));
  
  // draw tabs/sections
  int tabPosition = 0;
  int32_t countPosition = 2;
  char countAscii = '1';
  for(int32_t i=0;i<=sections.sectionsCount;i++){
    if(sections.sectionCurrent == i)
      DrawRectangle(tabPosition, 0, TILESIZE*SCREENSCALE, TILESIZE*SCREENSCALE, GetNanoColor(8)); 
    DrawFont((countAscii++) - 32, (Vector2){countPosition, 1});
    countPosition += TILESIZE + 1;
    tabPosition += TILESIZE * SCREENSCALE + SCREENSCALE;
  }


  // SHOW THE LINE COUNT AND THE TOKEN COUNT
  int32_t posx = 100;
  char printLine[32];
  char printToken[32];
  sprintf(printLine, "Line %d", sections.codeEditor[sections.sectionCurrent].cursorLine+1);
  sprintf(printToken, "%d/%d", tokenCount, TOKENLIMIT);
  DrawSomeText(printLine, (Vector2){1,121});
 
  // count the digit 
  int32_t digitCount = 1;
  int32_t value = tokenCount;
  while(value >= 10){
    value /= 10;
    digitCount++;
  }
  DrawSomeText(printToken, (Vector2){posx - ((digitCount - 1) * FONTWIDTH),121});
}

// check if cart is running
bool GetCartIfRunning(void){
  return cartRunning;
}

// set cart state 
void SetCartRunning(bool set){
  cartRunning = set;
}

// TODO: CHECK THIS BITCH
// just draw a text 
static void DrawSomeText(char *texts, Vector2 position){
  int32_t index = 0;
  while(true){
    char c = texts[index];
    if(c != '\0'){
      int32_t fontIndex = c-32;

      if(fontIndex >= 0){
        DrawFont(fontIndex, position);
        position.x += FONTWIDTH;
      }
    } else {
      return;
    }
    index++;
  }
}

// draw texts in editor
static void DrawTextEditor(Vector2 *position){
  CodeEditor *codeEditor = &sections.codeEditor[sections.sectionCurrent];

  const int32_t posx = (int32_t)position->x;

  int32_t index = 0;
  while(codeEditor->code[index] != '\0'){
    char c = codeEditor->code[index];
    int32_t fontIndex = c-32;

    if(c == '\n'){
      position->y += FONTHEIGHT;
      position->x = posx;
      fontIndex = -1;
    }

    // COMMENT 
    if(c == '-' && codeEditor->code[index+1] == '-'){
      int32_t start = index;
      while(codeEditor->code[index] != '\n' && codeEditor->code[index] != '\0') index++;

      ChangeTextCurrentColor(COL_COMMENT);
      for(int32_t i=start;i<index;i++){
        int32_t fIndex = codeEditor->code[i]-32;
        DrawFont(fIndex, *position);
        position->x += FONTWIDTH;
      }
      ChangeTextCurrentColor(COL_DEFAULT);
      continue;
    }

    // STRING 
    if(c == '"'){
      int32_t start = index;
      index++;
      while(codeEditor->code[index] && codeEditor->code[index] != '"' && codeEditor->code[index] != '\n') index++;
      
      if(codeEditor->code[index] == '"') index++;

      ChangeTextCurrentColor(COL_STRING);
      for(int32_t i=start;i<index;i++){
        int32_t fIndex = codeEditor->code[i]-32;
        DrawFont(fIndex, *position);
        position->x += FONTWIDTH;
      }
      ChangeTextCurrentColor(COL_DEFAULT);
      continue;
    }

    // NUMBER 
    if(isdigit(c)){
      int32_t start = index;
      while(isdigit(codeEditor->code[index])) index++;

      ChangeTextCurrentColor(COL_NUMBER);
      for(int32_t i=start;i<index;i++){
        int32_t fIndex = codeEditor->code[i]-32;
        DrawFont(fIndex, *position);
        position->x += FONTWIDTH;
      }
      ChangeTextCurrentColor(COL_DEFAULT); 
      continue;
    }

    if(isSymbol(c)){
      ChangeTextCurrentColor(COL_SYMBOL);
      DrawFont(fontIndex, *position);
      position->x += FONTWIDTH;
      ChangeTextCurrentColor(COL_DEFAULT);
      index++;
      continue;
    }

    // WORD / KEYWORD 
    if(isalpha(c) || c == '_'){
      int32_t start = index;
      while(isalnum(codeEditor->code[index]) || codeEditor->code[index] == '_') index++;

      char temp[128];
      int32_t len = index - start;
      strncpy(temp, &codeEditor->code[start], len);
      temp[len] = '\0';

      if(isKeyword(temp))
        ChangeTextCurrentColor(COL_KEYWORD);
      else if(isFunc(temp))
        ChangeTextCurrentColor(COL_FUNC); 
      else
        ChangeTextCurrentColor(COL_DEFAULT);
      
      for(int32_t i=start;i<index;i++){
        int32_t fIndex = codeEditor->code[i]-32;
        DrawFont(fIndex, *position);
        position->x += FONTWIDTH;
      }
      ChangeTextCurrentColor(COL_DEFAULT);
      continue;
    }

    // SINGLE CHAR
    if(fontIndex >= 0){
      DrawFont(fontIndex, *position);
      position->x += FONTWIDTH;
    }

    index++;
  }
}

// draw the font
static void DrawFont(const int32_t fontIndex, Vector2 position){
  const int32_t posx = position.x;
  for(int32_t y=0;y<FONTHEIGHT;y++){
    for(int32_t x=0;x<FONTWIDTH;x++){
      int32_t index = y * FONTWIDTH + x;
      uint32_t text = GetFontText(fontIndex)[index];
      
      if(text > 0)
        DrawRectangle(position.x * SCREENSCALE, position.y * SCREENSCALE, SCREENSCALE, SCREENSCALE, GetNanoColor(GetTextCurrentColor()));
      position.x += 1;
    } 
    position.x = posx;
    position.y += 1;
  }
}

static int isKeyword(const char* str){
  for(int32_t i=0;i<KEYWORDCOUNT;i++){
    if(strcmp(str, kw[i]) == 0) return 1;
  }
  return 0;
}

static int isFunc(const char* str){
  for(int32_t i=0;i<FUNCCOUNT;i++){
    if(strcmp(str, func[i]) == 0) return 1;
  }
  return 0;
}

static int isSymbol(const char c){
  for(int32_t i=0;i<SYMBOLSCOUNT;i++){
    if(c == symbols[i]) return 1;
  }
  return 0;
}

// inserting characters
static void InsertCharacter(int32_t pos, char c){
  CodeEditor *codeEditor = &sections.codeEditor[sections.sectionCurrent];

  if(codeEditor->codeCount+1 > codeEditor->codeSize){
    codeEditor->codeSize += TEXTSIZE;
    codeEditor->code = realloc(codeEditor->code, sizeof(codeEditor->codeSize));

    if(!codeEditor->code){
      printf("Memory Reallocation for code failed\n");
      return;
    }
  } 

  for(int32_t i=codeEditor->codeCount;i>=pos;i--){
    codeEditor->code[i + 1] = codeEditor->code[i];
  }

  codeEditor->code[pos] = c;
  codeEditor->codeCount++;
}

// backspacing characters
static void BackSpace(int32_t pos){
  CodeEditor *codeEditor = &sections.codeEditor[sections.sectionCurrent];

  if(pos <= 0) return;

  for(int32_t i=pos-1;i<codeEditor->codeCount;i++){
    codeEditor->code[i] = codeEditor->code[i + 1];
  }
  codeEditor->codeCount--;
}

// NOTE: FIX THIS SOON
void InitSection(void){
  sections.sectionsCount = 0;
  sections.sectionCurrent = 0;
  sections.codeEditor = malloc(sizeof(CodeEditor));  
  if(!sections.codeEditor){ printf("Memory Allocation failed\n"); return; }

  // set section's code cursor
  sections.codeEditor[sections.sectionCurrent].cursor = 0;
  sections.codeEditor[sections.sectionCurrent].cursorPosition = (Vector2){0,0};

  sections.codeEditor[sections.sectionCurrent].cursorLine = 0;
  sections.codeEditor[sections.sectionCurrent].cursorColumn = 0;
  sections.codeEditor[sections.sectionCurrent].scrollY = 0;
  sections.codeEditor[sections.sectionCurrent].scrollX = 0;

  // CODE MEMORY ALLOCATION
  sections.codeEditor[sections.sectionCurrent].codeSize = TEXTSIZE;
  sections.codeEditor[sections.sectionCurrent].codeCount = 0;
  sections.codeEditor[sections.sectionCurrent].code = malloc(sizeof(char) * sections.codeEditor[sections.sectionCurrent].codeSize);
  if(!sections.codeEditor[sections.sectionCurrent].code){
    printf("Code Memory Allocation Failed\n");
    return;
  }
  sections.codeEditor[sections.sectionsCount].code[0] = '\0';
}

// CREATE NEW SECTION
void NewSection(void){ 
  // increase section count 
  sections.sectionsCount++;
  sections.sectionCurrent = sections.sectionsCount; 
  sections.codeEditor = realloc(sections.codeEditor, sizeof(CodeEditor)*(sections.sectionsCount+1));
  if(!sections.codeEditor){ printf("Memory Reallocation failed\n"); return; } 

  // SET CODE SECTION/TAB
 
  // set section's code cursor
  sections.codeEditor[sections.sectionCurrent].cursor = 0;
  sections.codeEditor[sections.sectionCurrent].cursorPosition = (Vector2){0,0};

  sections.codeEditor[sections.sectionCurrent].cursorLine = 0;
  sections.codeEditor[sections.sectionCurrent].cursorColumn = 0;
  sections.codeEditor[sections.sectionCurrent].scrollY = 0;
  sections.codeEditor[sections.sectionCurrent].scrollX = 0;

  // CODE MEMORY ALLOCATION
  sections.codeEditor[sections.sectionCurrent].codeSize = TEXTSIZE;
  sections.codeEditor[sections.sectionCurrent].codeCount = 0; 
  sections.codeEditor[sections.sectionCurrent].code = malloc(sizeof(char) * sections.codeEditor[sections.sectionCurrent].codeSize);
  if(!sections.codeEditor[sections.sectionCurrent].code){
    printf("Code Memory Allocation Failed\n");
    return;
  } 
  sections.codeEditor[sections.sectionsCount].code[0] = '\0';  
}

void FreeSections(void){
  for(int32_t i=0;i<=sections.sectionsCount;i++){
    free(sections.codeEditor[i].code);
    sections.codeEditor[i].code = NULL;
  }

  free(sections.codeEditor);
  sections.codeEditor = NULL; 
}

int32_t GetEditorSectionCount(void){
  return sections.sectionsCount;
}

// TODO: do this
void RemoveSection(void){

}

static void CountToken(void){
  int32_t count = 0;
  for(int32_t i=0;i<=sections.sectionsCount;i++){
    CodeEditor *codeEditor = &sections.codeEditor[i];
    
    int32_t index = 0;
    while(codeEditor->code[index] != '\0'){
      char c = codeEditor->code[index];

      // ignore spaces 
      if(isspace(c)){
        index++;
        continue;
      }
      
      // identifier
      if(isalpha(c) || c == '_'){
        while(codeEditor->code[index] != '\0' && (isalnum(codeEditor->code[index]) || codeEditor->code[index] == '_')){
          index++;
        }
        count++;
        continue;
      }

      // number 
      if(isdigit(c)){
        while(isdigit(codeEditor->code[index]) && codeEditor->code[index] != '\0'){ 
          index++;
        }

        count++;
        continue;
      }

      // operators/symbols
      count++;
      index++;
    }
  }

  tokenCount = count;
}

static void DrawSelection(void){
  
  // normalize
  int32_t a = select_start;
  int32_t b = select_end;
  if(a > b){
    int32_t temp = a;
    a = b;
    b = temp;
  }

  int32_t x = 1;
  int32_t y = TILESIZE+1;

  CodeEditor *codeEditor = &sections.codeEditor[sections.sectionCurrent];
  for(int32_t i=0;codeEditor->code[i] != '\0';i++){
    char c = codeEditor->code[i];
    
    int32_t isSelected = (i>=a && i<b);
    
    if(selecting && isSelected){
      DrawRectangle((x-(codeEditor->scrollX * FONTWIDTH))*SCREENSCALE, (y-(codeEditor->scrollY * FONTHEIGHT))*SCREENSCALE, FONTWIDTH*SCREENSCALE, FONTHEIGHT*SCREENSCALE, GetNanoColor(COL_SELECT));
    }

    if(c == '\n'){
      x=1;
      y += FONTHEIGHT;
    } else {
      x += FONTWIDTH;
    }
  }
}
