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
SFX sfxs[MAX_SFX];
static int32_t currentSFX = 0;

// SFX PLAYER 
static AudioStream stream;

static SFXPlayer channels[MAXCHANNELS];
static uint8_t currentChannel = 0;

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
static float GetWave(float phase, int32_t waveType, int32_t channelIndex);

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

  
  // INITIALIZE SFX PLAYER (initialize the channels[4 channels]) 
  for(int32_t i=0;i<MAXCHANNELS;i++){
    channels[i].playing = false;
    channels[i].currentStep = 0;
    channels[i].sfxIndex = 0;
    channels[i].timer = 0;
    
    //synth 
    channels[i].synth.phase = 0;
    channels[i].synth.freq = 0;
    channels[i].synth.volume = 0;
    channels[i].synth.targetVolume = 0;
    channels[i].synth.waveform = 0;
    channels[i].synth.effect = 0; 
  }

  // AUDIO STREAM
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

  if(prevSFX.clicked){
    channels[currentChannel].playing = false;
    currentSFX--;
  }
  if(nextSFX.clicked){
    channels[currentChannel].playing = false;
    currentSFX++;
  }
  if(currentSFX < 0) currentSFX = MAX_SFX-1;
  if(currentSFX > MAX_SFX-1) currentSFX = 0;
  
  // speed button 
  if(speedButton.clicked) sfxs[currentSFX].speed++;
  if(speedButton.rclicked) sfxs[currentSFX].speed--;
  if(sfxs[currentSFX].speed < 1) sfxs[currentSFX].speed = 1;
  snprintf(speedButton.textLabel, sizeof(speedButton.textLabel), "%02d", sfxs[currentSFX].speed);

  // start loop 
  if(startLoopButton.clicked) sfxs[currentSFX].loopStart++;
  if(startLoopButton.rclicked) sfxs[currentSFX].loopStart--;
  if(sfxs[currentSFX].loopStart < 0) sfxs[currentSFX].loopStart = 0;
  snprintf(startLoopButton.textLabel, sizeof(startLoopButton.textLabel), "%02d", sfxs[currentSFX].loopStart);
  
  // end loop 
  if(endLoopButton.clicked) sfxs[currentSFX].loopEnd++;
  if(endLoopButton.rclicked) sfxs[currentSFX].loopEnd--;
  if(sfxs[currentSFX].loopEnd < 0) sfxs[currentSFX].loopEnd = 0;
  snprintf(endLoopButton.textLabel, sizeof(endLoopButton.textLabel), "%02d", sfxs[currentSFX].loopEnd);

  // KEY INPUT 
  if(IsKeyPressed(KEY_TAB)){
    advancePage = !advancePage;
  }

  if(IsKeyPressed(KEY_SPACE)){ 
    // play the channel index 0 for test play
    currentChannel = 0;
    channels[currentChannel].sfxIndex = currentSFX;
    channels[currentChannel].playing = true;
    channels[currentChannel].currentStep = 0;
    channels[currentChannel].timer = 0.0f;

    SFXNote note = sfxs[channels[currentChannel].sfxIndex].notes[channels[currentChannel].currentStep];
    channels[currentChannel].synth.freq = NoteToFreq(note.pitch);
    channels[currentChannel].synth.targetVolume = note.volume / 7.0f;
    channels[currentChannel].synth.phase = 0;
    channels[currentChannel].synth.waveform = note.waveform;
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
    snprintf(hoveredNoteText, sizeof(hoveredNoteText), "%02d : X#X", hoveredNote);
    DrawTextUI(hoveredNoteText, (Vector2){96, SCREENHEIGHT-FONTHEIGHT-1});
  }
  if(hoveredVolume){
    DrawTextUI("Volume", (Vector2){1, SCREENHEIGHT-FONTHEIGHT-1}); 
    snprintf(hoveredNoteText, sizeof(hoveredNoteText), "%02d : %d", hoveredNote, hoveredValue);
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
  snprintf(text, sizeof(text), "%02d", currentSFX);
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
    if(channels[currentChannel].playing && channels[currentChannel].currentStep == y) color = 0;

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
  static float lowpass = 0.0f;

  for(uint32_t i = 0; i < frames; i++){
    float mixed = 0.0f;
    for(int32_t chl = 0; chl < MAXCHANNELS; chl++){
      if(!channels[chl].playing) continue;
      channels[chl].timer++;
      float noteLength = sfxs[channels[chl].sfxIndex].speed * (44100.0f / 128.0f);
      
      if(channels[chl].timer >= noteLength){
        channels[chl].timer = 0;
        channels[chl].currentStep++;
        if(channels[chl].currentStep >= MAX_NOTES){
          channels[chl].playing = false;
          continue;
        }
        SFXNote note = sfxs[channels[chl].sfxIndex].notes[channels[chl].currentStep];
        channels[chl].synth.freq = NoteToFreq(note.pitch);
        channels[chl].synth.waveform = note.waveform;
        channels[chl].synth.targetVolume = note.volume / 7.0f;
        // IMPORTANT:
        // Do NOT reset phase.
      }
      float sample = GetWave(channels[chl].synth.phase, channels[chl].synth.waveform, chl);
      sample *= channels[chl].synth.targetVolume;
      mixed += sample;
      channels[chl].synth.phase += 2.0f * PI * channels[chl].synth.freq / 44100.0f;
      if(channels[chl].synth.phase >= 2.0f * PI) channels[chl].synth.phase -= 2.0f * PI;
    }
    // Gentle lowpass
    lowpass += (mixed - lowpass) * 0.25f;
    // Soft clip
    samples[i] = tanhf(lowpass) * 0.8f;
  }
}

static float GetWave(float phase, int32_t waveType, int32_t channelIndex){
    float p = phase / (2.0f * PI);
    switch(waveType){
      // Triangle
      case WAVE_TRIANGLE:
        return 1.0f - 4.0f * fabsf(p - 0.5f);
      // Tilted Saw
      case WAVE_TILTEDSAW:
        if(p < 0.875f) return (p / 0.875f) * 2.0f - 1.0f;
        return 1.0f - ((p - 0.875f) / 0.125f) * 2.0f;
      // Softer Saw
      case WAVE_SAW:
        return sinf(2.0f * PI * p) + 0.5f * sinf(4.0f * PI * p) + 0.25f * sinf(6.0f * PI * p);
      // Softer Square
      case WAVE_SQUARE:
        return sinf(2.0f * PI * p) + sinf(6.0f * PI * p) / 3.0f + sinf(10.0f * PI * p) / 5.0f;
      // Pulse
      case WAVE_PULSE:
        return p < 0.3125f ? 1.0f : -1.0f;
      // Organ
      case WAVE_ORGAN:
        return 0.6f * sinf(2.0f * PI * p) + 0.3f * sinf(4.0f * PI * p) + 0.1f * sinf(8.0f * PI * p);
      // Pico8-like chunky noise
      case WAVE_NOISE:{
        static float held[MAXCHANNELS];
        static int32_t counter[MAXCHANNELS];
        if(counter[channelIndex] <= 0){
          held[channelIndex] = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
          counter[channelIndex] = 32;
        }
        counter[channelIndex]--;
        return held[channelIndex];
      }
      // Phaser
      case WAVE_PHASER:{
        float a = sinf(2.0f * PI * p);
        float b = sinf(4.0f * PI * p);
        return (a + b) * 0.5f;
      }
    }
    return 0.0f;
}

// play sfx base on the sfxIndex 
void PlaySfx(const int32_t sfxIndex, const int32_t channelIndex){
  if(channelIndex < 0) return;
  currentChannel = channelIndex;

  if(sfxIndex < 0){ 
    channels[currentChannel].playing = false;
    return;
  }

  channels[currentChannel].sfxIndex = sfxIndex; 
  channels[currentChannel].playing = true;
  channels[currentChannel].currentStep = 0;
  channels[currentChannel].timer = 0.0f;

  SFXNote note = sfxs[channels[currentChannel].sfxIndex].notes[channels[currentChannel].currentStep];
  channels[currentChannel].synth.freq = NoteToFreq(note.pitch);
  channels[currentChannel].synth.targetVolume = note.volume / 7.0f;
  channels[currentChannel].synth.phase = 0;
  channels[currentChannel].synth.waveform = note.waveform;
}
