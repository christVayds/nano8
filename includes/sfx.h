#ifndef SFX_H
#define SFX_H

#include <stdint.h>
#include <raylib.h>

#define MAX_NOTES 32
#define MAX_SFX 64

#define PITCHES 64
#define STEPS 32
#define MAXVOLUME 7
#define NOTEBOXSIZE 2
#define MAX(a, b)((a)>(b)?(a):(b))

typedef enum{
  WAVE_TRIANGLE,
  WAVE_TILTEDSAW,
  WAVE_SAW,
  WAVE_SQUARE,
  WAVE_PULSE,
  WAVE_ORGAN,
  WAVE_NOISE,
  WAVE_PHASER
} Waveform;

typedef enum{
  EFFECT_NONE,
  EFFECT_SLIDE,
  EFFECT_VIBRATO,
  EFFECT_DROP,
  EFFECT_FADEIN,
  EFFECT_FADEOUT,
  EFFECT_ARPEGGIOFAST,
  EFFECT_ARPEGGIOSLOW
} Effects;

typedef struct{
  uint8_t pitch;   // 0-63
  Waveform waveform;
  uint8_t volume;   // 0-7
  Effects effect;   // slide, vibrato etc.
} SFXNote;

typedef struct{
  SFXNote notes[MAX_NOTES];

  int32_t speed;
  int32_t loopStart;
  int32_t loopEnd;
} SFX;

typedef struct{
  float phase;
  float freq;
  float volume;
  float targetVolume;
  int32_t waveform;
  int32_t effect;
} Synth;

typedef struct{
  bool playing;
  int32_t currentStep;
  int32_t sfxIndex;
  int32_t timer;
  
  int32_t rampDownSamples;
  int32_t rampSamples;
  Synth synth;
} SFXPlayer;

void SfxInit(void);
void ResetSFX(void);
void SfxUpdate(void);
void SfxInput(void);
void SfxDraw(void);
void PlaySfx(const int32_t sfxIndex, const int32_t channelIndex);

float NoteToFreq(int32_t picth);
#endif
