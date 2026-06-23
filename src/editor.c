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
static void InsertCharacter(int32_t pos, char c);
static void DeleteCharacterAt(int32_t pos);
static int32_t SmartBackspace(CodeEditor *codeEditor);
static void CountToken(void);
static void DrawSelection(void);
static int32_t GetLineStart(CodeEditor *codeEditor, int32_t pos);
static int32_t GetLineEnd(CodeEditor *codeEditor, int32_t pos);
static int32_t GetLineLeadingSpaces(CodeEditor *codeEditor, int32_t lineStart);
static bool LineOpensBlock(CodeEditor *codeEditor, int32_t lineStart, int32_t lineEnd);
static void InsertSpacesAtCursor(CodeEditor *codeEditor, int32_t count);
static void DeleteSelection(CodeEditor *codeEditor);
static bool GetNormalizedSelection(int32_t *start, int32_t *end);
static void IndentSelectedLines(CodeEditor *codeEditor);
static void UnindentSelectedLines(CodeEditor *codeEditor);
static void SmartIndentCurrentLine(CodeEditor *codeEditor);
static void SmartUnindentCurrentLine(CodeEditor *codeEditor);

static void RemoveSectionAt(int32_t idx);

static int isKeyword(const char* str);
static int isFunc(const char* str);
static int isSymbol(const char c);
static const char* kw[] = {
  "local", "function", "end", "if", "then", "else", "elseif", "for", "while", "do", 
  "return", "break", "false", "true", "yanji", "nano8", "or", "and", "not",
  "K_A", "K_S", "K_Z", "K_X", "K_L", "K_R", "K_U", "K_D", "table"
};

static const char* func[] = {
  "print", "_init", "_update", "_main", "_draw", "main", "rectfill", "rect", "circ", "circfill", "spr",
  "pset", "pget", "line", "btn", "btnp", "cls", "map", "mset", "mget", "fget", "fset", "camera", "cursor", "pal", "palt", 
  "flr", "abs", "ciel", "sqrt", "sin", "cos", "rand", "srand", "min", "max", "insert",
  "sfx", "music"
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
  char *code = malloc(1);
  if(!code) return NULL;
  code[0] = '\0';

  for(int32_t i=0;i<=sections.sectionsCount;i++){
    size_t len = strlen(code);
    size_t add = (size_t)sections.codeEditor[i].codeCount;
    size_t newSize = len + add + 1; // +1 for terminating NUL

    char *tmp = realloc(code, newSize);
    if(!tmp){ free(code); return NULL; }
    code = tmp;

    // append the section code safely
    if(add > 0){
      memcpy(code + len, sections.codeEditor[i].code, add);
    }
    code[len + add] = '\0';
  }

  return code;
}

char *GetLuaCodeInSection(int32_t si){
  // si = section index
  size_t add = (size_t)sections.codeEditor[si].codeCount;
  char *code = malloc(add + 1);
  if(!code) return NULL;
  if(add > 0){
    memcpy(code, sections.codeEditor[si].code, add);
  }
  code[add] = '\0';
  return code;
}

// load a lua code
void LoadCode(const char* luaCode, size_t size){
  CodeEditor *codeEditor = &sections.codeEditor[sections.sectionCurrent];
  if(!luaCode || size == 0) return;

  // ensure destination buffer has space for new data + terminating NUL
  if(codeEditor->codeCount + (int32_t)size + 1 > codeEditor->codeSize){
    int32_t needed = (int32_t)size + 1;
    int32_t newSize = codeEditor->codeSize;
    while(codeEditor->codeCount + needed + 0 > newSize) newSize += TEXTSIZE;
    char *tmp = realloc(codeEditor->code, newSize);
    if(!tmp){
      printf("LoadCode: memory allocation failed\n");
      return;
    }
    codeEditor->code = tmp;
    codeEditor->codeSize = newSize;
  }

  memcpy(codeEditor->code + codeEditor->codeCount, luaCode, sizeof(char) * size);
  codeEditor->codeCount += (int32_t)size;
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
    if(selecting) DeleteSelection(codeEditor);
    InsertCharacter(codeEditor->cursor, key); 
    codeEditor->cursor++;
    selecting = false;
  } 

  // ---------------- 
  //  SECTIONS CONTROL
  // ---------------- 
  // CONTROL + T to create new TAB/SECTION 
  else if(IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_T) && sections.sectionsCount+1 < MAXSECTIONS){
    NewSection();
    return;
  }

  // CONTROL RIGHT KEY - MOVE SECTIONS RIGHT SIDE
  if(IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_RIGHT)){
    // auto-remove empty current section when switching (only if more than one section exists)
    if(sections.sectionsCount > 0){
      CodeEditor *cur = &sections.codeEditor[sections.sectionCurrent];
      bool empty = true;
      for(int32_t i=0;i<cur->codeCount;i++){
        char ch = cur->code[i];
        if(ch != '\0' && ch != '\n' && ch != ' ' && ch != '\t' && ch != '\r'){
          empty = false; break;
        }
      }
      if(empty && sections.sectionsCount > 0){
        RemoveSectionAt(sections.sectionCurrent);
      }
    }

    if(sections.sectionCurrent < sections.sectionsCount)
      sections.sectionCurrent++;
    else sections.sectionCurrent = 0;

    return;
  }
  // CONTROL LEFT KEY - MOVE SECTION LEFT SIDE 
  if(IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_LEFT)){
    // auto-remove empty current section when switching (only if more than one section exists)
    if(sections.sectionsCount > 0){
      CodeEditor *cur = &sections.codeEditor[sections.sectionCurrent];
      bool empty = true;
      for(int32_t i=0;i<cur->codeCount;i++){
        char ch = cur->code[i];
        if(ch != '\0' && ch != '\n' && ch != ' ' && ch != '\t' && ch != '\r'){
          empty = false; break;
        }
      }
      if(empty && sections.sectionsCount > 0){
        // if removing the current and it's the first, after removal we want to land on the last valid index
        RemoveSectionAt(sections.sectionCurrent);
      }
    }

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

  // TEXT TAB
  if(IsKeyPressed(KEY_TAB) && tokenCount < TOKENLIMIT){
    if(IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)){
      if(selecting) UnindentSelectedLines(codeEditor);
      else SmartUnindentCurrentLine(codeEditor);
    } else {
      if(selecting) IndentSelectedLines(codeEditor);
      else SmartIndentCurrentLine(codeEditor);
    }
    indentCount = GetLineLeadingSpaces(codeEditor, GetLineStart(codeEditor, codeEditor->cursor));
  }

  // ENTER 
  if(IsKeyPressed(KEY_ENTER) && tokenCount < TOKENLIMIT){
    if(selecting) DeleteSelection(codeEditor);
    int32_t lineStart = GetLineStart(codeEditor, codeEditor->cursor);
    int32_t lineEnd = GetLineEnd(codeEditor, codeEditor->cursor);
    int32_t indent = GetLineLeadingSpaces(codeEditor, lineStart);
    if(LineOpensBlock(codeEditor, lineStart, lineEnd)) indent += TABSIZE;

    InsertCharacter(codeEditor->cursor, '\n');
    codeEditor->cursor++;
    InsertSpacesAtCursor(codeEditor, indent);
    indentCount = indent;
  }

  // BACKSPACE 
  if(IsKeyDown(KEY_BACKSPACE) && codeEditor->cursor >= 1){
    if(!backSpaceHeld){
      int32_t removed = SmartBackspace(codeEditor);
      
      backSpaceHoldTime = 0.0f;
      backSpaceRepeatTimer = 0.5f;
      backSpaceHeld = true;

      if(codeEditor->cursorColumn <= indentCount) indentCount -= removed;
    } else {
      backSpaceHoldTime += GetFrameTime();
      backSpaceRepeatTimer -= GetFrameTime();

      if(backSpaceRepeatTimer <= 0.0f){
        int32_t removed = SmartBackspace(codeEditor);
        
        backSpaceRepeatTimer = 0.05f;

        if(codeEditor->cursorColumn <= indentCount) indentCount -= removed;
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

  // keep a small margin so the cursor isn't flush against the top/left edges
  const int32_t marginY = 5;
  const int32_t marginX = 5;

  if(codeEditor->cursorLine >= codeEditor->scrollY + VISIBLELINES - marginY)
    codeEditor->scrollY = codeEditor->cursorLine - (VISIBLELINES - marginY) + 1;
  if(codeEditor->cursorLine < codeEditor->scrollY + marginY)
    codeEditor->scrollY = codeEditor->cursorLine - marginY;
  if(codeEditor->scrollY < 0) codeEditor->scrollY = 0;

  if(codeEditor->cursorColumn >= codeEditor->scrollX + VISIBLECOLUMNS - marginX)
    codeEditor->scrollX = codeEditor->cursorColumn - (VISIBLECOLUMNS - marginX) + 1;
  if(codeEditor->cursorColumn < codeEditor->scrollX + marginX)
    codeEditor->scrollX = codeEditor->cursorColumn - marginX;
  if(codeEditor->scrollX < 0) codeEditor->scrollX = 0;
 
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
  GetFont(95, (Vector2){cposx, cposy}, false, false);
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
    GetFont((countAscii++) - 32, (Vector2){countPosition, 1}, false, false);
    countPosition += TILESIZE + 1;
    tabPosition += TILESIZE * SCREENSCALE + SCREENSCALE;
  }


  // SHOW THE LINE COUNT AND THE TOKEN COUNT
  int32_t posx = 100;
  char printLine[32];
  char printToken[32];
  snprintf(printLine, sizeof(printLine), "Line %d", sections.codeEditor[sections.sectionCurrent].cursorLine+1);
  snprintf(printToken, sizeof(printToken), "%d/%d", tokenCount, TOKENLIMIT);
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
        GetFont(fontIndex, position, false, false);
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
        GetFont(fIndex, *position, false, false);
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
        GetFont(fIndex, *position, false, false);
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
        GetFont(fIndex, *position, false, false);
        position->x += FONTWIDTH;
      }
      ChangeTextCurrentColor(COL_DEFAULT); 
      continue;
    }

    if(isSymbol(c)){
      ChangeTextCurrentColor(COL_SYMBOL);
      GetFont(fontIndex, *position, false, false);
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
        GetFont(fIndex, *position, false, false); 
        position->x += FONTWIDTH;
      }
      ChangeTextCurrentColor(COL_DEFAULT);
      continue;
    }

    // SINGLE CHAR
    if(fontIndex >= 0){
      GetFont(fontIndex, *position, false, false);
      position->x += FONTWIDTH;
    }

    index++;
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
  if(!codeEditor) return;

  // clamp pos to valid range [0, codeCount]
  if(pos < 0) pos = 0;
  if(pos > codeEditor->codeCount) pos = codeEditor->codeCount;

  // Ensure we have space for new char plus terminating NUL
  if(codeEditor->codeCount + 2 > codeEditor->codeSize){
    size_t newSize = codeEditor->codeSize + TEXTSIZE;
    char *tmp = realloc(codeEditor->code, newSize);
    if(!tmp){
      printf("Memory Reallocation for code failed\n");
      return;
    }
    codeEditor->code = tmp;
    codeEditor->codeSize = (int32_t)newSize;
  }

  // shift right including the terminating NUL so the buffer remains NUL-terminated
  for(int32_t i = codeEditor->codeCount; i >= pos; i--){
    codeEditor->code[i + 1] = codeEditor->code[i];
  }

  codeEditor->code[pos] = c;
  codeEditor->codeCount++;
  // ensure NUL termination after insert
  codeEditor->code[codeEditor->codeCount] = '\0';
}

static void DeleteCharacterAt(int32_t pos){
  CodeEditor *codeEditor = &sections.codeEditor[sections.sectionCurrent];
  if(!codeEditor) return;
  if(pos < 0 || pos >= codeEditor->codeCount) return;

  for(int32_t i = pos; i < codeEditor->codeCount; i++){
    codeEditor->code[i] = codeEditor->code[i + 1];
  }
  codeEditor->codeCount--;
  if(codeEditor->codeCount < 0) codeEditor->codeCount = 0;
  codeEditor->code[codeEditor->codeCount] = '\0';
}

static int32_t SmartBackspace(CodeEditor *codeEditor){
  if(!codeEditor || codeEditor->cursor <= 0) return 0;
  if(selecting){
    DeleteSelection(codeEditor);
    return 1;
  }

  int32_t lineStart = GetLineStart(codeEditor, codeEditor->cursor);
  int32_t column = codeEditor->cursor - lineStart;
  bool inLeadingSpaces = true;
  for(int32_t i = lineStart; i < codeEditor->cursor; i++){
    if(codeEditor->code[i] != ' '){
      inLeadingSpaces = false;
      break;
    }
  }

  int32_t removeCount = 1;
  if(inLeadingSpaces && column > 0){
    removeCount = column % TABSIZE;
    if(removeCount == 0) removeCount = TABSIZE;
    if(removeCount > column) removeCount = column;
  }

  for(int32_t i = 0; i < removeCount; i++){
    DeleteCharacterAt(codeEditor->cursor - 1);
    codeEditor->cursor--;
  }
  return removeCount;
}

static int32_t GetLineStart(CodeEditor *codeEditor, int32_t pos){
  if(!codeEditor) return 0;
  if(pos < 0) pos = 0;
  if(pos > codeEditor->codeCount) pos = codeEditor->codeCount;

  while(pos > 0 && codeEditor->code[pos - 1] != '\n') pos--;
  return pos;
}

static int32_t GetLineEnd(CodeEditor *codeEditor, int32_t pos){
  if(!codeEditor) return 0;
  if(pos < 0) pos = 0;
  if(pos > codeEditor->codeCount) pos = codeEditor->codeCount;

  while(pos < codeEditor->codeCount && codeEditor->code[pos] != '\n') pos++;
  return pos;
}

static int32_t GetLineLeadingSpaces(CodeEditor *codeEditor, int32_t lineStart){
  int32_t count = 0;
  if(!codeEditor) return 0;

  while(lineStart + count < codeEditor->codeCount && codeEditor->code[lineStart + count] == ' ') count++;
  return count;
}

static bool IsWordBoundary(char c){
  return !(isalnum((unsigned char)c) || c == '_');
}

static bool LineEndsWithWord(const char *line, int32_t len, const char *word){
  int32_t wordLen = (int32_t)strlen(word);
  if(len < wordLen) return false;
  if(strncmp(line + len - wordLen, word, wordLen) != 0) return false;
  return len == wordLen || IsWordBoundary(line[len - wordLen - 1]);
}

static bool LineStartsWithWord(const char *line, int32_t len, const char *word){
  int32_t wordLen = (int32_t)strlen(word);
  if(len < wordLen) return false;
  if(strncmp(line, word, wordLen) != 0) return false;
  return len == wordLen || IsWordBoundary(line[wordLen]);
}

static bool LineHasWord(const char *line, int32_t len, const char *word){
  int32_t wordLen = (int32_t)strlen(word);
  for(int32_t i = 0; i <= len - wordLen; i++){
    if(strncmp(line + i, word, wordLen) != 0) continue;
    bool leftOk = i == 0 || IsWordBoundary(line[i - 1]);
    bool rightOk = i + wordLen == len || IsWordBoundary(line[i + wordLen]);
    if(leftOk && rightOk) return true;
  }
  return false;
}

static bool LineOpensBlock(CodeEditor *codeEditor, int32_t lineStart, int32_t lineEnd){
  char line[256];
  int32_t len = 0;
  if(!codeEditor) return false;

  while(lineStart < lineEnd && codeEditor->code[lineStart] == ' ') lineStart++;
  while(lineEnd > lineStart && codeEditor->code[lineEnd - 1] == ' ') lineEnd--;

  for(int32_t i = lineStart; i < lineEnd && len < (int32_t)sizeof(line) - 1; i++){
    if(codeEditor->code[i] == '-' && i + 1 < lineEnd && codeEditor->code[i + 1] == '-') break;
    line[len++] = (char)tolower((unsigned char)codeEditor->code[i]);
  }
  while(len > 0 && line[len - 1] == ' ') len--;
  line[len] = '\0';
  if(len <= 0) return false;

  return LineEndsWithWord(line, len, "then") ||
         LineEndsWithWord(line, len, "do") ||
         LineEndsWithWord(line, len, "else") ||
         LineEndsWithWord(line, len, "repeat") ||
         LineStartsWithWord(line, len, "function") ||
         (LineStartsWithWord(line, len, "local") && LineHasWord(line, len, "function"));
}

static void InsertSpacesAtCursor(CodeEditor *codeEditor, int32_t count){
  if(!codeEditor) return;
  for(int32_t i = 0; i < count; i++){
    InsertCharacter(codeEditor->cursor, ' ');
    codeEditor->cursor++;
  }
}

static bool GetNormalizedSelection(int32_t *start, int32_t *end){
  if(!selecting) return false;

  *start = select_start;
  *end = select_end;
  if(*start > *end){
    int32_t temp = *start;
    *start = *end;
    *end = temp;
  }
  return *start != *end;
}

static void DeleteSelection(CodeEditor *codeEditor){
  int32_t start;
  int32_t end;
  if(!codeEditor || !GetNormalizedSelection(&start, &end)) return;

  for(int32_t i = start; i < end; i++) DeleteCharacterAt(start);
  codeEditor->cursor = start;
  selecting = false;
  select_start = start;
  select_end = start;
}

static void IndentSelectedLines(CodeEditor *codeEditor){
  int32_t start;
  int32_t end;
  if(!codeEditor || !GetNormalizedSelection(&start, &end)) return;

  int32_t pos = GetLineStart(codeEditor, start);
  while(pos <= end && pos <= codeEditor->codeCount){
    for(int32_t i = 0; i < TABSIZE; i++) InsertCharacter(pos + i, ' ');
    if(codeEditor->cursor >= pos) codeEditor->cursor += TABSIZE;
    if(select_start >= pos) select_start += TABSIZE;
    if(select_end >= pos) select_end += TABSIZE;
    end += TABSIZE;

    pos = GetLineEnd(codeEditor, pos + TABSIZE);
    if(pos >= codeEditor->codeCount || pos >= end) break;
    pos++;
  }
}

static void UnindentSelectedLines(CodeEditor *codeEditor){
  int32_t start;
  int32_t end;
  if(!codeEditor || !GetNormalizedSelection(&start, &end)) return;

  int32_t pos = GetLineStart(codeEditor, start);
  while(pos <= end && pos < codeEditor->codeCount){
    int32_t removed = 0;
    while(removed < TABSIZE && pos < codeEditor->codeCount && codeEditor->code[pos] == ' '){
      DeleteCharacterAt(pos);
      removed++;
      if(codeEditor->cursor > pos) codeEditor->cursor--;
      if(select_start > pos) select_start--;
      if(select_end > pos) select_end--;
      end--;
    }

    pos = GetLineEnd(codeEditor, pos);
    if(pos >= codeEditor->codeCount || pos >= end) break;
    pos++;
  }
}

static void SmartIndentCurrentLine(CodeEditor *codeEditor){
  if(!codeEditor) return;
  InsertSpacesAtCursor(codeEditor, TABSIZE);
  selecting = false;
}

static void SmartUnindentCurrentLine(CodeEditor *codeEditor){
  if(!codeEditor) return;

  int32_t lineStart = GetLineStart(codeEditor, codeEditor->cursor);
  int32_t removed = 0;
  while(removed < TABSIZE && lineStart < codeEditor->codeCount && codeEditor->code[lineStart] == ' '){
    DeleteCharacterAt(lineStart);
    removed++;
    if(codeEditor->cursor > lineStart) codeEditor->cursor--;
  }
  selecting = false;
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

static void RemoveSectionAt(int32_t idx){
  if(idx < 0 || idx > sections.sectionsCount) return;

  // do not remove the last remaining section
  if(sections.sectionsCount == 0) return;

  // free code buffer for the section being removed
  if(sections.codeEditor[idx].code){
    free(sections.codeEditor[idx].code);
    sections.codeEditor[idx].code = NULL;
  }

  // shift subsequent sections down by one
  for(int32_t i=idx;i<sections.sectionsCount;i++){
    sections.codeEditor[i] = sections.codeEditor[i+1];
  }

  // decrease section count
  sections.sectionsCount--;

  // shrink the allocated array
  if(sections.sectionsCount >= 0){
    CodeEditor *tmp = realloc(sections.codeEditor, sizeof(CodeEditor) * (sections.sectionsCount + 1));
    if(tmp) sections.codeEditor = tmp;
  }

  // ensure current index is within bounds
  if(sections.sectionCurrent > sections.sectionsCount)
    sections.sectionCurrent = sections.sectionsCount;
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

void ResetSection(void){
  // free any existing sections and re-initialize a single empty section
  FreeSections();
  InitSection();
}
