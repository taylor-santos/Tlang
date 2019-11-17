#include <stdio.h>
#include <stdlib.h>
#include <getopt.h> // getopt()
#include "Tlang_parser.h"
#include "Tlang_scanner.h"
#include "ast/program.h"
#include "codegen.h"

#define NAME    "tcc"
#define VERSION "0.1.0"
#define RED     "\033[0;91m"
#define WHITE   "\033[0m"
#define ERROR   NAME ": " RED "error: " WHITE

void print_usage(char *argv0);
int asprintf(char **strp, const char *fmt, ...);
void *malloc_check(size_t size);
char *strdup_check(const char *s);

static struct option options[] = {
        { "help",    no_argument, 0, 'h' },
        { "version", no_argument, 0, 'v' },
        { 0, 0,                   0, 0 }
};

static char *options_help[] = {
        "--help        Display this information.",
        "--version     Display compiler version information.",
        "-o <file>     Place the output into <file>."
};

int main(int argc, char *argv[]) {
    int             opt, opt_index, file_count, i, status = 0;
    char            *out_filename                         = NULL, *err;
    FILE            **inputs, *output;
    yyscan_t        scanner;
    YY_BUFFER_STATE state;

    opterr = 0;

    while (-1 != (
            opt = getopt_long(argc, argv, "o:h", options, &opt_index))) {
        switch (opt) {
            case 'o':
                out_filename = strdup_check(optarg);
                break;
            case 'h':
                print_usage(argv[0]);
                exit(EXIT_SUCCESS);
            case 'v':
                fprintf(stdout, "%s version %s\n", NAME, VERSION);
                exit(EXIT_SUCCESS);
            default:
                fprintf(stderr, ERROR "unknown argument: '-%c'\n", optopt);
                exit(EXIT_FAILURE);
        }
    }
    file_count  = argc - optind;
    if (file_count == 0) {
        fprintf(stderr, ERROR "no input files\n");
        exit(EXIT_FAILURE);
    }
    inputs = malloc_check(sizeof(FILE *) * file_count);
    for (i = 0; i < file_count; i++) {
        inputs[i] = fopen(argv[optind + i], "r");
        if (inputs[i] == NULL) {
            asprintf(&err, ERROR "unable to open file '%s'", argv[optind + i]);
            perror(err);
            free(err);
            status = 1;
        }
    }
    if (out_filename != NULL) {
        output = fopen(out_filename, "w+");
        free(out_filename);
    } else {
        output = fopen("a.out.c", "w+");
    }
    if (output == NULL) {
        asprintf(&err, ERROR "unable to open file '%s'", argv[optind + i]);
        perror(err);
        free(err);
        status = 1;
    }
    if (status) {
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < file_count; i++) {
        /* Initialize Flex and Bison */
        if (yylex_init(&scanner)) {
            fprintf(stderr, ERROR "could not initialize Flex scanner.\n");
            exit(EXIT_FAILURE);
        }
        state = yy_create_buffer(inputs[i], YY_BUF_SIZE, scanner);
        yy_switch_to_buffer(state, scanner);
        const ASTNode *AST = NULL;

        if (yyparse(&AST, argv[optind + i], scanner)) {
            status = 1;
        } else {
            ASTProgramVTable *vtable = AST->vtable;
            //vtable->json(AST, 0, stdout);
            //printf("\n");
            if (!vtable->type_check(AST)) {
                ASTProgramData *data = AST->data;
                print_Map(data->symbols, print_VarType);
                //GenerateCode(AST, output);
            } else {
                printf("Type checker failed!\n");
            }
            vtable->free(AST);
        }

        yy_delete_buffer(state, scanner);
        yylex_destroy(scanner);
        fclose(inputs[i]);
    }
    free(inputs);
    if (status) {
        exit(EXIT_FAILURE);
    }
    fclose(output);
    return 0;
}

void print_usage(char *argv0) {
    int i, count = sizeof(options_help) / sizeof(*options_help);
    printf("Usage: %s [options] file...\n", argv0);
    printf("Options:\n");
    for (i = 0; i < count; i++) {
        printf("  %s\n", options_help[i]);
    }
}

void *malloc_check(size_t size) {
    void *ptr = malloc(size);
    if (ptr == NULL) {
        perror(ERROR "unable to allocate memory");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

char *strdup_check(const char *s) {
    char *str = strdup(s);
    if (str == NULL) {
        perror(ERROR "unable to allocate memory");
        exit(EXIT_FAILURE);
    }
    return str;
}
