#ifndef AUDIO_H_
#define AUDIO_H_

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <sndfile.h>

#define SAMPLE_RATE 44100
#define DEFAULT_BPM 140

typedef float Frame;

#define SAMPLE_CAP 1024
#define SAMPLE_INSTANCE_CAP 1024

#define WORD_MAX_SZ 64

#define DA_INIT_CAP 128

#define DA_APPEND(da, item) do {\
    if ((da)->count >= (da)->capacity) {\
        if ((da)->capacity == 0) {\
            (da)->capacity = DA_INIT_CAP;\
            (da)->items = malloc(DA_INIT_CAP * sizeof(*(da)->items));\
        } else {\
            (da)->capacity *= 2;\
            (da)->items = realloc((da)->items, (da)->capacity * sizeof(*(da)->items));\
        }\
        assert((da)->items != NULL);\
    }\
    (da)->items[(da)->count++] = item;\
} while (0)

#define LINEAR_SEARCH(arr, item_name, var) do {\
    for (size_t iter = 0; iter < (arr).count; ++iter) {\
        if (!strcmp((arr).items[iter].name, (item_name))) {\
            (var) = &(arr).items[iter];\
        }\
    }\
} while (0)

#define DA(type) \
typedef struct {\
    type *items;\
    size_t count;\
    size_t capacity;\
} type##s;

typedef struct {
    char name[WORD_MAX_SZ];
    Frame *frames;
    size_t count;
} Sample;
DA(Sample)

typedef enum {
    PT_NONE,
    PT_BPM,
    PT_COUNT,
} ParameterType;

typedef struct {
    ParameterType type;
    float value;
} ParameterChange;

typedef struct {
    Sample *sample;
    ParameterChange pc;
    size_t row;
} AudioObject;
DA(AudioObject)

typedef struct {
    char name[WORD_MAX_SZ];
    size_t rows;

    AudioObject *items;
    size_t count;
    size_t capacity;
} Pattern;
DA(Pattern)

typedef struct {
    Pattern **items;
    size_t count;
    size_t capacity;
} Sequence;

void addsampleinstance(const char *sample_name, Pattern *pat, size_t row);
void addbpmchange(float value, Pattern *pat, size_t row);
void addpattern(Pattern *p, const char *name);
void addtosequence(const char *pattern_name);

size_t saveaudio(const char *filepath);
void loadsample(const char *path, const char *name);

// Not needed yet
// float sinsound(float i, float freq, float volume, float samplerate);
// float raisepitch(float base, float semitones);

#endif
