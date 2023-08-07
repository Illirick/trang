#include "audio.h"

// TODO: combine into a structure?
static Samples samples;
static Patterns patterns;
Sequence sequence;

size_t framecount(const Pattern *p) {
    size_t total_frames = 0, frames;
    for (size_t i = 0; i < p->count; ++i) {
        frames = p->items[i].pos + p->items[i].sample->count;
        if (frames > total_frames) {
            total_frames = frames;
        }
    }
    if (total_frames % 2 != 0) total_frames ++;
    return total_frames;
}

size_t rowtoframecount(size_t row) {
    //      row          60 seconds per minute
    // --------------- * --------------------- * samples per second (or sample rate) * 2 samples per channel
    // 4 rows per beat     beats per minute
    return ((float)row / 4 * 60 / BPM) * SAMPLE_RATE * 2;
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

    SampleInstance si = { s, rowtoframecount(row) };
    DA_APPEND(pat, si);
}

float sinsound(float i, float freq, float volume, float samplerate) {
    return sinf(i * M_PI * 2 * freq / samplerate) * volume * (0xFFFF / 2 - 1);
}

float raisepitch(float base, float semitones) {
    return base * pow(2, 1/semitones);
}

float addsounds(float s1, float s2) {
    return ((double)s1 - (0xFFFF / 2 - 1) + (double)s2 - (0xFFFF / 2 - 1)) / 2 + (0xFFFF / 2 - 1);
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

    size_t total_frames = 0, offset = 0;
    Frame *buf = NULL;
    for (size_t i = 0; i < sequence.count; ++i) {
        Pattern *pat = sequence.items[i];
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

        for (size_t i = 0; i < pat->count; ++i) {
            SampleInstance *si = &pat->items[i];
            for (size_t i = 0; i < si->sample->count; ++i) {
                size_t pos = si->pos + i + offset;
                buf[pos] = addsounds(buf[pos], si->sample->frames[i]);
            }
        }
        offset += rowtoframecount(pat->rows);
    }
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
        assert(numstrlength <= WORD_MAX_SZ);
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
        fprintf(stderr, "Error: pattern not found: %s", pattern_name);
        exit(1);
    }
    DA_APPEND(&sequence, p);
}
