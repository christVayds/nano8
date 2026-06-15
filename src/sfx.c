#include "sfx.h"
#include "game.h"
#include "nanoUI.h"

#include <math.h>
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>

// BUTTON
static NanoButton sfxPage;
static NanoButton sfxAdvance;
static NanoButton prevSFX;
static NanoButton nextSFX;
static NanoButton speedButton;
static NanoButton startLoopButton;
static NanoButton endLoopButton;

// waveform buttons 
static NanoButton waveformButtons[8];

// SFX
static SFX sfxs[MAX_SFX];
static int32_t currentSFX = 0;
static int32_t rampSamples = 0;
static int32_t rampDownSamples = 0;
static float lastValue = 0.0f;

// Fade-in/out length in samples.
// Very short ramps can still produce audible clicks,
// especially for square, saw, pulse, and noise waveforms.
#define RAMP_LEN 512

// SFX PLAYER 
static SFXPlayer sfxPlayer;
static Synth synth = {0};
static AudioStream stream;

static Waveform selectedWaveform = WAVE_TRIANGLE;

// PAGE
static bool advancePage = false;
static bool sfxEditorHovered = false;
static bool hoveredVolume = false;
static int32_t hoveredNote = 0;
static int32_t hoveredValue = 0;
static char hoveredNoteText[32];

// static functions
static void DrawSfxSelector(void);
static void InitWaveformButtons(void);
static void UpdateWaveformButtons(void);
static void InputWaveformButtons(void);
static void DrawSfxWaveform(void);
static void UpdateNotes(void);
static void DrawNotes(void);
static void UpdateVolume(void);
static void DrawVolume(void);
static void AudioCallBack(void *bufferData, uint32_t frames);
static float GetWave(float phase, int32_t waveType);

void SfxInit(void){

  // INIT BUTTON
  InitNanoButtonIcon(&sfxPage, (Rectangle){0, 0, TILESIZE, TILESIZE}, "SFX Editor", 18, false);               // sfx editor page 
  InitNanoButtonIcon(&sfxAdvance, (Rectangle){TILESIZE, 0, TILESIZE, TILESIZE}, "SFX Editor Advance", 19, false);     // advance sfx editor page 
  InitNanoButtonIcon(&prevSFX, (Rectangle){1, 10, TILESIZE, TILESIZE}, "Prev SFX", 14, false);              // prev sfx 
  InitNanoButtonIcon(&nextSFX, (Rectangle){16, 10, TILESIZE, TILESIZE}, "Next SFX", 13, false);             // next sfx 
  InitNanoButtonText(&speedButton, (Rectangle){56, 9, FONTWIDTH*3, FONTHEIGHT+1}, "01", true);
  InitNanoButtonText(&startLoopButton, (Rectangle){100, 9, FONTWIDTH*3, FONTHEIGHT+1}, "00", true);
  InitNanoButtonText(&endLoopButton, (Rectangle){114, 9, FONTWIDTH*3, FONTHEIGHT+1}, "00", true);

  // WAVFORMS BUTTONS 
  InitWaveformButtons();

  sfxPage.active = true;

  // intialize sfx
  ResetSFX();

  
  // INITIALIZE SFX PLAYER 
  sfxPlayer.playing = false;
  sfxPlayer.currentStep = 0;
  sfxPlayer.timer = 0;
  stream = LoadAudioStream(44100, 32, 1);
  PlayAudioStream(stream);
  SetAudioStreamCallback(stream, AudioCallBack);
}

void ResetSFX(void){
  for(int32_t sfx=0;sfx<MAX_SFX;sfx++){
    SFX newSfx = {
      .speed = 1,
      .loopStart=0,
      .loopEnd=0
    };

    // sfx notes 
    for(int32_t i=0;i<MAX_NOTES;i++){
      SFXNote sfxNote = {
        .pitch=0,
        .waveform=WAVE_TRIANGLE,
        .volume=6,
        .effect=EFFECT_NONE
      };

      // push note to sfx
      newSfx.notes[i] = sfxNote;
    }

    // push new sfx
    sfxs[sfx] = newSfx;
  }
}

void SfxUpdate(void){

  // UPDATE BUTTONS 
  UpdateNanoButton(&sfxPage);       // sfx editor 
  UpdateNanoButton(&sfxAdvance);    // advance sfx editor 
  UpdateNanoButton(&prevSFX);       // prev sfx selector 
  UpdateNanoButton(&nextSFX);       // next sfx selector
  UpdateNanoButton(&speedButton);   // currentSFX speed button
  UpdateNanoButton(&startLoopButton);
  UpdateNanoButton(&endLoopButton);

  // UPDATE WAVEFORMS BUTTONS 
  UpdateWaveformButtons();

  if(advancePage){
    sfxPage.active = false;
    sfxAdvance.active = true;
  } else {
    sfxPage.active = true;
    sfxAdvance.active = false;
  }
}

void SfxInput(void){

  // PAGE 
  if(sfxPage.clicked) advancePage = false;
  if(sfxAdvance.clicked) advancePage = true;

  if(prevSFX.clicked) currentSFX--;
  if(nextSFX.clicked) currentSFX++;
  if(currentSFX < 0) currentSFX = MAX_SFX-1;
  if(currentSFX > MAX_SFX-1) currentSFX = 0;
  
  // speed button 
  if(speedButton.clicked) sfxs[currentSFX].speed++;
  if(speedButton.rclicked) sfxs[currentSFX].speed--;
  if(sfxs[currentSFX].speed < 1) sfxs[currentSFX].speed = 1;
  sprintf(speedButton.textLabel, "%02d", sfxs[currentSFX].speed);

  // start loop 
  if(startLoopButton.clicked) sfxs[currentSFX].loopStart++;
  if(startLoopButton.rclicked) sfxs[currentSFX].loopStart--;
  if(sfxs[currentSFX].loopStart < 0) sfxs[currentSFX].loopStart = 0;
  sprintf(startLoopButton.textLabel, "%02d", sfxs[currentSFX].loopStart);
  
  // end loop 
  if(endLoopButton.clicked) sfxs[currentSFX].loopEnd++;
  if(endLoopButton.rclicked) sfxs[currentSFX].loopEnd--;
  if(sfxs[currentSFX].loopEnd < 0) sfxs[currentSFX].loopEnd = 0;
  sprintf(endLoopButton.textLabel, "%02d", sfxs[currentSFX].loopEnd);

  // KEY INPUT 
  if(IsKeyPressed(KEY_TAB)){
    advancePage = !advancePage;
  }

  if(IsKeyPressed(KEY_SPACE)){
    sfxPlayer.playing = true;
    sfxPlayer.currentStep = 0;
    sfxPlayer.timer = 0.0f;
    rampDownSamples = 0;
    rampSamples = 0;

    SFXNote note = sfxs[currentSFX].notes[sfxPlayer.currentStep];
    synth.freq = NoteToFreq(note.pitch);
    synth.targetVolume = note.volume / 7.0f;
    synth.phase = 0;
    synth.waveform = note.waveform;
  } 

  // update input for waveform buttons 
  InputWaveformButtons();

  // update mouse input  
  if(!advancePage){
    UpdateNotes();
    UpdateVolume();
  }
}

void SfxDraw(void){ 
  // SFX SELECTOR 
  DrawSfxSelector();

  // SFX WAVEFORM 
  DrawSfxWaveform();

  // SPEED AND LOOP 
  DrawTextUI("Speed:", (Vector2){32, 10});
  DrawTextUI("Loop:", (Vector2){80, 10});
  DrawTextUI("Pitch", (Vector2){4, 20});

  // sfx control buttons 
  DrawNanoButton(&speedButton);
  DrawNanoButton(&startLoopButton);
  DrawNanoButton(&endLoopButton);

  // Draw the notes editor 
  if(!advancePage){
    DrawNotes();
    DrawVolume();
  }

  // draw UI top and bottom
  DrawRectangle(0, SCREENHEIGHT*SCREENSCALE - TILESIZE*SCREENSCALE, SCREENWIDTH*SCREENSCALE, TILESIZE*SCREENSCALE, GetNanoColor(0));
  DrawRectangle(0, 0, SCREENWIDTH*SCREENSCALE, TILESIZE*SCREENSCALE, GetNanoColor(0));
  DrawRectangleLines(0, 0, (SCREENWIDTH+1)*SCREENSCALE, TILESIZE*SCREENSCALE, GetNanoColor(8));
  DrawRectangleLines(0, SCREENHEIGHT*SCREENSCALE - TILESIZE*SCREENSCALE, (SCREENWIDTH+1)*SCREENSCALE, TILESIZE*SCREENSCALE+1, GetNanoColor(8));
 
  // DRAW PAGE BUTTONS
  DrawNanoButton(&sfxPage);
  DrawNanoButton(&sfxAdvance);

  // hovered waveform buttons 
  for(int32_t i=0;i<WAVE_PHASER+1;i++){
    if(waveformButtons[i].hovered){
      DrawTextUI(waveformButtons[i].textLabel, (Vector2){1, SCREENHEIGHT-FONTHEIGHT-1}); 
    }
  }

  // hovered editors  
  if(sfxEditorHovered){
    DrawTextUI("Note", (Vector2){1, SCREENHEIGHT-FONTHEIGHT-1}); 
    sprintf(hoveredNoteText, "%02d : X#X", hoveredNote);
    DrawTextUI(hoveredNoteText, (Vector2){96, SCREENHEIGHT-FONTHEIGHT-1});
  }
  if(hoveredVolume){
    DrawTextUI("Volume", (Vector2){1, SCREENHEIGHT-FONTHEIGHT-1}); 
    sprintf(hoveredNoteText, "%02d : %d", hoveredNote, hoveredValue);
    DrawTextUI(hoveredNoteText, (Vector2){104, SCREENHEIGHT-FONTHEIGHT-1});
  }

  if(prevSFX.hovered) DrawTextUI("Previous SFX", (Vector2){1, SCREENHEIGHT-FONTHEIGHT-1});
  if(nextSFX.hovered) DrawTextUI("Next SFX", (Vector2){1, SCREENHEIGHT-FONTHEIGHT-1});
}

// convert pitch / note not frequency
float NoteToFreq(int32_t pitch){
  return 65.41f * powf(2.0f, pitch / 12.0f);
}

static void DrawSfxSelector(void){
  DrawNanoButton(&prevSFX);
  DrawNanoButton(&nextSFX);

  char text[32];
  sprintf(text, "%02d", currentSFX);
  DrawTextUI(text, (Vector2){9, 11});
}

static void InitWaveformButtons(void){
  Vector2 position = {32, 18};
  
  char waveButtonNames[8][32] = {
    "Triangle",
    "Tilted saw",
    "Saw",
    "Square",
    "Pulse",
    "Organ",
    "Noise",
    "Phaser"
  };

  int32_t icon = 20;
  for(int32_t i=0;i<8;i++){
    InitNanoButtonIcon(&waveformButtons[i], (Rectangle){position.x, position.y, TILESIZE, TILESIZE}, waveButtonNames[i], icon+i, true);
    position.x += TILESIZE+4;
  }
  waveformButtons[0].active = true;
}

static void UpdateWaveformButtons(void){
  for(int32_t i=0;i<8;i++){
    UpdateNanoButton(&waveformButtons[i]);
  }
}

static void InputWaveformButtons(void){ 
  for(int32_t i=0;i<WAVE_PHASER+1;i++){
    if(waveformButtons[i].clicked){
      for(int32_t y=0;y<8;y++) waveformButtons[y].active = false; // reset all active buttons 
      selectedWaveform = (Waveform)i;                             // set as active button
      waveformButtons[i].active = true;
    } 
    
    // shift click to change all notes waveform to selected wave form
    if(IsKeyDown(KEY_LEFT_SHIFT) && waveformButtons[i].clicked){
      for(int32_t y=0;y<MAX_NOTES;y++){
        sfxs[currentSFX].notes[y].waveform = selectedWaveform;
      }
    }
  }
}

static void DrawSfxWaveform(void){
  for(int32_t i=0;i<8;i++){
    DrawNanoButton(&waveformButtons[i]);
  }
}

static void UpdateNotes(void){
  Vector2 mouse = GetMousePosition();
  mouse.x /= SCREENSCALE;
  mouse.y /= SCREENSCALE;

  // CLICK NOTES 
  int32_t gridx = 1;  // x position of note grid 
  int32_t gridy = 28; // y position of note grid
  sfxEditorHovered = false;
  if(mouse.x > gridx && mouse.x < SCREENWIDTH && mouse.y > gridy && mouse.y < gridy+67){ 
    int32_t step = (mouse.x - gridx) / (NOTEBOXSIZE*2);
    int32_t row = ((mouse.y - gridy) * (NOTEBOXSIZE*2)) / (NOTEBOXSIZE*2);
    sfxEditorHovered = true;
    hoveredNote = step;

    if(IsMouseButtonDown(MOUSE_LEFT_BUTTON)){
      if(step >= 0 && step < STEPS && row >= 0 && row < PITCHES){
        int32_t pitch = PITCHES - 1 - row;
        sfxs[currentSFX].notes[step].pitch = pitch;
        sfxs[currentSFX].notes[step].waveform = selectedWaveform;
      }
    }
  }
}

static void DrawNotes(void){
  Vector2 position = {0, 28};
  const int32_t height = 67;
  DrawRectangleLines(position.x*SCREENSCALE, position.y*SCREENSCALE, (SCREENWIDTH+1)*SCREENSCALE, height*SCREENSCALE, GetNanoColor(6)); 

  // draw the grid
  position.x++;
  /*for(int32_t y=0;y<PITCHES;y++){
    for(int32_t x=0;x<STEPS;x++){
      DrawRectangleLines((position.x+x*(NOTEBOXSIZE*2))*SCREENSCALE, (position.y+1+y)*SCREENSCALE, NOTEBOXSIZE*SCREENSCALE, NOTEBOXSIZE*SCREENSCALE, GetNanoColor(3));
    }
  }*/

  // draw existing notes
  SFXNote *note = sfxs[currentSFX].notes;
  int32_t prevX = -1;
  int32_t prevY = -1;
  int32_t color = 0;
  for(int32_t y=0;y<MAX_NOTES;y++){
    int32_t row = MAX_SFX - note[y].pitch;
    
    int32_t px = (position.x+y*(NOTEBOXSIZE*2));
    int32_t py = (position.y+row);
    color = note[y].waveform+8;
    if(sfxPlayer.playing && sfxPlayer.currentStep == y) color = 0;

    if(prevX != -1){
      DrawLine(
        (prevX + 2) * SCREENSCALE,
        prevY * SCREENSCALE,
        px * SCREENSCALE,
        py * SCREENSCALE,
        GetNanoColor(2)
      );
    }

    DrawRectangle(
      px*SCREENSCALE,
      py*SCREENSCALE,
      NOTEBOXSIZE*SCREENSCALE,
      NOTEBOXSIZE*SCREENSCALE,
      GetNanoColor(color)
    );

    prevX = px;
    prevY = py;
  }
}

static void UpdateVolume(void){
  Vector2 mouse = GetMousePosition();
  mouse.x /= SCREENSCALE;
  mouse.y /= SCREENSCALE;
  
  // CLICK VOLUME
  int32_t gridx = 1;
  int32_t gridy = 106;
  hoveredVolume = false;
  if(mouse.x > gridx && mouse.x < SCREENWIDTH && mouse.y > gridy && mouse.y < gridy+13){
    int32_t step = (mouse.x - gridx) / (NOTEBOXSIZE*2);
    int32_t row = (mouse.y - gridy) / NOTEBOXSIZE;
    hoveredVolume = true;
    hoveredNote = step;
    hoveredValue = MAXVOLUME - 1 - row;
    
    if(IsMouseButtonDown(MOUSE_LEFT_BUTTON)){
      if(step >= 0 && step < STEPS && row >= 0 && row < MAXVOLUME){
        int32_t volume = MAXVOLUME - 1 - row;
        sfxs[currentSFX].notes[step].volume = volume;
      }
    }
  }
}

static void DrawVolume(void){
  Vector2 position = {0, 97};
  //const int32_t height = 25;
  //DrawRectangleLines(position.x*SCREENSCALE, position.y*SCREENSCALE, (SCREENWIDTH+1)*SCREENSCALE, height*SCREENSCALE, GetNanoColor(2));
  
  position.x++;
  DrawTextUI("Volume", (Vector2){position.x, position.y+1});

  position.y+=8;
  /*for(int32_t y=0;y<MAXVOLUME;y++){
    for(int32_t x=0;x<STEPS;x++){
      DrawRectangleLines((position.x+x*(NOTEBOXSIZE*2))*SCREENSCALE, (position.y+y*NOTEBOXSIZE)*SCREENSCALE, NOTEBOXSIZE*SCREENSCALE, NOTEBOXSIZE*SCREENSCALE, GetNanoColor(2));
    }
  }*/

  SFXNote *note = sfxs[currentSFX].notes;
  for(int32_t y=0;y<MAX_NOTES;y++){
    int32_t row = note[y].volume;
    DrawRectangle(
      (position.x+y*(NOTEBOXSIZE*2))*SCREENSCALE,
      (position.y+(MAXVOLUME - 1 - row)*NOTEBOXSIZE)*SCREENSCALE,
      NOTEBOXSIZE*SCREENSCALE,
      ((row + 1) * NOTEBOXSIZE)*SCREENSCALE,
      GetNanoColor(2)
    );
  }
}

static void AudioCallBack(void *bufferData, uint32_t frames){
  float *samples = (float*)bufferData;
  for(uint32_t i=0;i<frames;i++){ 
    if(sfxPlayer.playing){
      sfxPlayer.timer++;

      if(sfxPlayer.timer >= sfxs[currentSFX].speed * (44100.0f / 128.0f)){
        sfxPlayer.timer = 0;

        sfxPlayer.currentStep++;
        if(sfxPlayer.currentStep >= MAX_NOTES){
          sfxPlayer.playing = false;
          rampDownSamples = 0;
        } else {
          SFXNote note = sfxs[currentSFX].notes[sfxPlayer.currentStep];
          synth.freq = NoteToFreq(note.pitch);
          synth.targetVolume = note.volume / 7.0f;
          synth.waveform = note.waveform;
        }
      }

      if(sfxPlayer.playing){
        synth.volume = synth.targetVolume;
        float value = GetWave(synth.phase, synth.waveform) * synth.volume;
        synth.phase += 2.0f * PI * synth.freq / 44100.0f;
        if(synth.phase > 2.0f * PI) synth.phase -= 2.0f * PI;

        if(rampSamples < RAMP_LEN){
          value *= (float)rampSamples / RAMP_LEN;
          rampSamples++;
        }

        lastValue = value;
        samples[i] = value;
        continue;
      }
    }

    if(rampDownSamples < RAMP_LEN){
      float t = (float)rampDownSamples / RAMP_LEN;
      
      // Quadratic fade-out.
      // Smoother than a linear fade and reduces audible clicks
      // when the waveform stops at a non-zero amplitude.
      samples[i] = lastValue * (1.0f - t) * (1.0f - t);
      rampDownSamples++;
    } else {
      samples[i] = 0.0f;
    }
    lastValue = samples[i];
  }
}

static float GetWave(float phase, int32_t waveType){
  // normalize phase 
  phase = phase / (2.0f * PI);

  switch(waveType){
    case WAVE_TRIANGLE:
      return 1.0f - 4.0f * fabsf(phase - 0.5f);
    case WAVE_TILTEDSAW:
      if(phase < 0.875f) return (phase / 0.875f) * 2.0f - 1.0f;
        return 1.0f - ((phase - 0.875f) / 0.125f) * 2.0f;
    case WAVE_SAW:
      return 2.0f * phase - 1.0f;
    case WAVE_SQUARE:
      return phase < 0.5f ? 0.5f : -0.5f;
    case WAVE_PULSE:
      return phase < 0.3125f ? 0.5f : -0.5f;
    case WAVE_ORGAN:
      return 0.5f * sinf(2.0f*PI*phase) + 0.5f * fabsf(sinf(4.0f*PI*phase)) - 0.5f;
    case WAVE_NOISE:
      return ((float)rand() / RAND_MAX)*2.0f-1.0f;
    case WAVE_PHASER:{
      float a = sinf(2.0f * PI * phase);
      float b = sinf(4.0f * PI * phase);
      return (a + b) * 0.5f;
    }
    default:
      break;
  }
  return 0;
}
