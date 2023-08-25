#include "audio.h"

// TODO: combine into a structure?
static Samples samples;
static Patterns patterns;
static float bpm = DEFAULT_BPM;
Sequence sequence;

static size_t framesperrow() {
    //       60 seconds per minute
    // ---------------------------------- * samples per second (or sample rate) * 2 samples per channel
    // 4 rows per beat * beats per minute
    return (60 / 4 / (float)bpm) * SAMPLE_RATE * 2;
}

static size_t rowtoframe(size_t row) {
    return row * framesperrow();
}

static size_t framecount(const Pattern *p) {
    size_t total_frames = 0, frames, offset = 0, ralbc = 0, oldbpm = bpm;
    for (size_t i = 0; i < p->count; ++i) {
        AudioObject ao = p->items[i];
        if (ao.sample != NULL) {
            frames = offset + rowtoframe(ao.row - ralbc) + ao.sample->count;
            total_frames = frames > total_frames ? frames : total_frames;
        }
        if (ao.pc.type == PT_BPM) {
            offset += rowtoframe(ao.row - ralbc);
            bpm = ao.pc.value;
            ralbc = ao.row;
        }
    }
    // size_t min_frames = rowtoframecount(p->rows) - 1;
    // total_frames = total_frames > min_frames ? total_frames : min_frames;
    bpm = oldbpm;
    return total_frames;
}

static float addsounds(float s1, float s2) {
    return ((double)s1 - (0xFFFF / 2 - 1) + (double)s2 - (0xFFFF / 2 - 1)) / 2 + (0xFFFF / 2 - 1);
}

void addsampleinstance(const char *sample_name, Pattern *pat, size_t row) {
    Sample *s = NULL;

    for (size_t i = 0; i < samples.count; ++i) {
        if (!strcmp(samples.items[i].name, sample_name)) {
            s = &samples.items[i];
            break;
        }
    }
    if (s == NULL) {
        fprintf(stderr, "Error: no sample named %s\n", sample_name);
        exit(1);
    }

    AudioObject ao = { .sample=s, .row=row };
    DA_APPEND(pat, ao);
}

void addbpmchange(float value, Pattern *pat, size_t row) {
    ParameterChange pc  = { .type=PT_BPM, .value=value };
    AudioObject ao = { .sample=NULL, .pc=pc, .row=row };
    DA_APPEND(pat, ao);
}

float sinsound(float i, float freq, float volume, float samplerate) {
    return sinf(i * M_PI * 2 * freq / samplerate) * volume * (0xFFFF / 2 - 1);
}

float raisepitch(float base, float semitones) {
    return base * pow(2, 1/semitones);
}

size_t saveaudio(const char *filepath) {
    if (patterns.count == 0) {
        return 0;
    }
    SF_INFO sfinfo;
    sfinfo.samplerate = SAMPLE_RATE;
    sfinfo.channels = 2;
    sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16 | SF_ENDIAN_FILE;
    SNDFILE *file = sf_open(filepath, SFM_WRITE, &sfinfo);
    if (file == NULL) {
        fprintf(stderr, "Error while opening the file %s: %s\n", filepath, sf_strerror(file));
        exit(1);
    }

    size_t total_frames = 0, offset = 0, ralbc = 0;
    Frame *buf = NULL;
    for (size_t pi = 0; pi < sequence.count; ++pi) {
        Pattern *pat = sequence.items[pi];
        size_t sum = offset + framecount(pat);
        total_frames = sum > total_frames ? sum : total_frames;
        if (pat->count == 0) {
            continue;
        }
        buf = (Frame*)realloc(buf, total_frames * sizeof(Frame));

        if (buf == NULL) {
            fprintf(stderr, "Error while allocation memory for the final audio: %s\n", strerror(errno));
            sf_close(file);
            exit(1);
        }

        for (size_t si = 0; si < pat->count; ++si) {
            AudioObject ao = pat->items[si];
            Sample *s = ao.sample;
            if (s != NULL) {
                assert(ao.row >= ralbc);
                size_t pos = offset + rowtoframe(ao.row - ralbc);
                for (size_t fi = 0; fi < s->count; ++fi) {
                    buf[pos + fi] = addsounds(buf[pos + fi], s->frames[fi]);
                }
            }
            if (ao.pc.type == PT_BPM) {
                offset += rowtoframe(ao.row - ralbc);
                bpm = ao.pc.value;
                ralbc = ao.row;
            }
        }
        assert(pat->rows >= ralbc);
        offset += (pat->rows - ralbc) * framesperrow();
        ralbc = 0;
    }
    if (total_frames % 2 != 0) total_frames ++;
    if ((int) total_frames != sf_write_float(file, buf, total_frames)) {
        fprintf(stderr, "Error while writing to the file %s: %s\n", filepath, sf_strerror(file));
        sf_close(file);
        exit(1);
    }

    sf_close(file);

    return total_frames;
}

void loadsample(const char *path, const char *name) {
    SF_INFO sfinfo;
    sfinfo.format = 0;
    SNDFILE *file = sf_open(path, SFM_READ, &sfinfo);
    if (file == NULL) {
        fprintf(stderr, "Error while opening the file %s: %s\n", path, sf_strerror(file));
        exit(1);
    }
    size_t items = sfinfo.frames * sfinfo.channels;
    Frame *frame_buf = (Frame*) calloc(items, sizeof(Frame));
    if ((int)items != sf_read_float(file, frame_buf, items)) {
        fprintf(stderr, "Error while reading from the file %s: %s\n", path, sf_strerror(file));
        exit(1);
    }
    sf_close(file);

    Sample *s = NULL;
    LINEAR_SEARCH(samples, name, s);
    if (s) {
        s->frames = frame_buf;
    } else {
        //printf("Adding sample %s\n", name);
        Sample sample = {.frames = frame_buf, .count = items};
        strcpy(sample.name, name);
        DA_APPEND(&samples, sample);
    }
}

void addpattern(Pattern *pat, const char *name) {
    if (name) {
        memcpy(pat->name, name, WORD_MAX_SZ - 1);
    } else {
        int numstrlength = snprintf(NULL, 0, "%zu", patterns.count);
        assert(numstrlength >= 0);
        assert(numstrlength <  WORD_MAX_SZ);
        sprintf(pat->name, "%zu", patterns.count);
    }
    Pattern *p = NULL;
    LINEAR_SEARCH(patterns, pat->name, p);
    if (p) {
        p->items = pat->items;
    } else {
        DA_APPEND(&patterns, *pat);
    }
}

void addtosequence(const char *pattern_name) {
    Pattern *p = NULL;
    LINEAR_SEARCH(patterns, pattern_name, p);
    if (!p) {
        fprintf(stderr, "Error: pattern not found: %s\n", pattern_name);
        exit(1);
    }
    DA_APPEND(&sequence, p);
}
