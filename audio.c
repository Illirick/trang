#include "audio.h"

Samples samples;
Pattern main_pattern;

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

void addsampleinstance(const char *sample_name, size_t row) {
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

    //      row          60 seconds per minute
    // --------------- * --------------------- * samples per second * 2 samples per channel
    // 4 rows per beat     beats per minute
    SampleInstance si = {s, ((float)row / 4 * 60 / BPM) * SAMPLE_RATE * 2 };
    DA_APPEND(main_pattern, si);
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
    if (main_pattern.count == 0) {
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

    size_t total_frames = framecount(&main_pattern);
    Frame *buf = (Frame*)calloc(total_frames, sizeof(Frame));

    for (size_t i = 0; i < main_pattern.count; ++i) {
        SampleInstance *si = &main_pattern.items[i];
        for (size_t i = 0; i < si->sample->count; ++i) {
            buf[si->pos + i] = addsounds(buf[si->pos + i], si->sample->frames[i]);
        }
    }

    if (buf == NULL) {
        fprintf(stderr, "Error while allocation memory for the final audio: %s\n", strerror(errno));
        exit(1);
    }

    if ((int) total_frames != sf_write_float(file, buf, total_frames)) {
        fprintf(stderr, "Error while writing to the file %s: %s\n", filepath, sf_strerror(file));
        exit(1);
    }

    sf_close(file);

    return total_frames;
}

Sample* strtosample(const char *str) {
    for (size_t i = 0; i < samples.count; ++i) {
        if (!strcmp(samples.items[i].name, str)) {
            return &samples.items[i];
        }
    }
    return NULL;
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


    Sample *s = strtosample(name);
    if (s) {
        s->frames = frame_buf;
    } else {
        //printf("Adding sample %s\n", name);
        Sample sample = {.frames = frame_buf, .count = items};
        strcpy(sample.name, name);
        DA_APPEND(samples, sample);
    }
}
