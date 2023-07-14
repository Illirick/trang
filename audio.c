#include "audio.h"

Sample samples[SAMPLE_CAP];
SampleInstance sample_instances[SAMPLE_INSTANCE_CAP];

size_t sample_count = 0;
size_t sample_instance_count = 0;

void addsample(Sample sample) {
    if (sample_count >= SAMPLE_CAP) {
        fprintf(stderr, "Error: 2 many samples\n");
        exit(1);
    }
    samples[sample_count] = sample;
    sample_count++;
}

void addsampleinstance(char *sample_name, size_t row) {
    if (sample_instance_count >= SAMPLE_INSTANCE_CAP) {
        fprintf(stderr, "Error: 2 many samples\n");
        exit(1);
    }
    Sample *s = NULL;

    for (size_t i = 0; i < sample_count; ++i) {
        if (!strcmp(samples[i].name, sample_name)) {
            s = &samples[i];
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
    sample_instances[sample_instance_count] = si;
    sample_instance_count++;
}

float sin_sound(float i, float freq, float volume, float samplerate) {
    return sinf(i * M_PI * 2 * freq / samplerate) * volume * (0xFFFF / 2 - 1);
}

float raise_pitch(float base, float semitones) {
    return base * pow(2, 1/semitones);
}

float add_sounds(float s1, float s2) {
    return ((double)s1 - (0xFFFF / 2 - 1) + (double)s2 - (0xFFFF / 2 - 1)) / 2 + (0xFFFF / 2 - 1);
}

size_t save_audio(char *filepath) {
    if (sample_instance_count == 0) {
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

    SampleInstance *latest_sample = &sample_instances[0];
    for (size_t i = 1; i < sample_instance_count; ++i) {
        SampleInstance *si = &sample_instances[i];
        if (si->pos > latest_sample->pos) {
            latest_sample = si;
        }
    }
    size_t items = latest_sample->pos + latest_sample->sample->count;
    if (items % 2 != 0) items ++;
    Frame *buf = (Frame*)malloc(sizeof(Frame) * items);

    for (size_t i = 0; i < sample_instance_count; ++i) {
        SampleInstance *si = &sample_instances[i];
        for (size_t i = 0; i < si->sample->count; ++i) {
            buf[si->pos + i] = add_sounds(buf[si->pos + i], si->sample->frames[i]);
        }
    }

    if (buf == NULL) {
        fprintf(stderr, "Error while allocation memory for the final audio: %s\n", strerror(errno));
        exit(1);
    }

    if ((int) items != sf_write_float(file, buf, items)) {
        fprintf(stderr, "Error while writing to the file %s: %s\n", filepath, sf_strerror(file));
        exit(1);
    }

    sf_close(file);

    return items;
}

Sample* str_to_sample(char *str) {
    for (size_t i = 0; i < sample_count; ++i) {
        if (!strcmp(samples[i].name, str)) {
            return &samples[i];
        }
    }
    return NULL;
}

void load_sample(char *path, char *name) {
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


    Sample *s = str_to_sample(name);
    if (s) {
        s->frames = frame_buf;
    } else {
        //printf("Adding sample %s\n", name);
        Sample sample = {.frames = frame_buf, .count = items};
        strcpy(sample.name, name);
        addsample(sample);
    }
}
