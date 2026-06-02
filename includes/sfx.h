#ifndef SFX_H
#define SFX_H

#include <stdint.h>

#define SFX_LEN 32 
#define MAX_SFX 64

typedef struct{
  int32_t note;  // -1 = empty, else MIDI note 
  int32_t instrument; // 0-7 wave types 
  int32_t volume; // 0-7
  int32_t effect; // 0-7 (slide, vibrato, etc.)
} SfxStep;

typedef struct{
  SfxStep step[SFX_LEN];
} Sfx;

typedef struct{
  Sfx sfx[MAX_SFX];
  int32_t current_sfx;
} SfxBank;

typedef struct{
  int32_t step; // 0-31
  int32_t field;  // 0=note, 1=instrument, 2=volume, 3=fx
} SfxCursor;

#endif
