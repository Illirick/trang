#ifndef AUDIO_H_
#define AUDIO_H_

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <errno.h>

#include <sndfile.h>

#define SAMPLE_RATE       44100
#define BPM               80

typedef float Frame;

#define SAMPLE_CAP 1024
#define SAMPLE_INSTANCE_CAP 1024

#define WORD_MAX_SZ 64

typedef struct {
    char name[WORD_MAX_SZ];
    Frame *frames;
    size_t count;
} Sample;

typedef struct {
    Sample *sample;
    size_t pos;
} SampleInstance;

void addsample(Sample sample);
void addsampleinstance(const char *sample_name, size_t row);
// float sin_sound(float i, float freq, float volume, float samplerate); not needed yet
// float raise_pitch(float base, float semitones);
float add_sounds(float s1, float s2);
size_t save_audio(const char *filepath);
Sample* str_to_sample(const char *str);
void load_sample(const char *path, const char *name);

#endif
