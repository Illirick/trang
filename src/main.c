#include "parser.h"
#include "audio.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Error: expected 1 command line arguments but got none\n");
        exit(1);
    }
    char *filepath = argv[1];
    parse(filepath);
    saveaudio("out.wav");
    return 0;
}
