#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // getopt()
#include "Tlang_parser.h"
#include "Tlang_scanner.h"

void print_usage(char *argv0);

int main(int argc, char *argv[]) {
    int opt, verbose = 0;
    FILE *output;

    while ((opt = getopt(argc, argv, "o:v")) != -1) {
        switch (opt) {
            case 'o':
                output = fopen(optarg, "w");
                if (output == NULL) {
                    perror("test");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'v':
                verbose = 1;
                break;
            default:
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    return 0;
}

void print_usage(char *argv0) {
    fprintf(stderr,
        "Usage: %s [-v] [-o outfile] infile1 [infile2 ...]\n",
        argv0
    );
}