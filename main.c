#include <stdint.h>
#include <limits.h>
#include <math.h>
#include <assert.h>

#include "parser.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Error: expected 1 command line arguments but got none\n");
        exit(1);
    }
    char *filepath = argv[1];
    FILE *prog_file = fopen(filepath, "r");
    if (prog_file == NULL) {
        fprintf(stderr, "Error while opening the file %s: %s\n", filepath, strerror(errno));
        exit(1);
    }
    parse(prog_file);
    save_audio("out.wav");
    return 0;
}
