#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include "runt.h"

#define CELLPOOL_SIZE 512
#define MEMPOOL_SIZE 4 * RUNT_MEGABYTE

typedef struct {
    runt_vm vm;
    unsigned char *mem;
    runt_cell *cells;
    int batch_mode;
    unsigned int ncells;
    size_t memsize;
} irunt_data;


static runt_int parse(runt_vm *vm, char *str, size_t read)
{
    const char *code = str;
    runt_int rc;
    runt_pmark_set(vm);
    rc = runt_compile(vm, code);
    runt_pmark_free(vm);
    return rc;
}

static runt_int load_dictionary_handle(runt_vm *vm, FILE *fp)
{
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    runt_int rc = RUNT_OK;

    while((read = runt_getline(&line, &len, fp)) != -1) {
        rc = parse(vm, line, read);
        if(rc == RUNT_NOT_OK) {
            runt_print(vm, "Bad return value. Dipping out.\n");
            break;
        }
    }

    free(line);
    return rc;
}

static runt_int load_dictionary(runt_vm *vm, char *filename)
{
    FILE *fp;

    fp = fopen(filename, "r");

    if(fp == NULL) {
        runt_print(vm, "Could not open file %s\n", filename);
        return RUNT_NOT_OK;
    }

    load_dictionary_handle(vm, fp);

    fclose(fp);
    return RUNT_OK;
}

runt_int runt_parse_file(runt_vm *vm, const char *filename)
{
    FILE *fp;

    fp = fopen(filename, "r");

    if(fp == NULL) {
        runt_print(vm, "Could not open file %s\n", filename);
        return RUNT_NOT_OK;
    }

    return runt_parse_filehandle(vm, fp);
}

runt_int runt_parse_filehandle(runt_vm *vm, FILE *fp)
{
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    runt_int rc = RUNT_OK;

    if(fp == NULL) return RUNT_NOT_OK;

    while((read = runt_getline(&line, &len, fp)) != -1) {
        rc = parse(vm, line, read);
        if(rc == RUNT_NOT_OK) break;
    }

    fclose(fp);
    free(line);
    return rc;
}

static int irunt_get_flag(irunt_data *irunt,
        char *argv[],
        runt_int pos,
        runt_int nargs,
        runt_int *n)
{
    char flag = *(argv[pos] + 1);
    runt_vm *vm = &irunt->vm;
    switch(flag) {
        case 'b':
            irunt->batch_mode = 1;
            *n = 1;
            return RUNT_OK;
        case 'i':
            irunt->batch_mode = 0;
            *n = 1;
            return RUNT_OK;
        case 'c':
            if(pos + 1 <= nargs) {
                irunt->ncells = atoi(argv[pos + 1]);
            } else {
                runt_print(vm, "Not enough arguments for -c\n");
            }
            *n = 2;
            return RUNT_OK;
        case 'm':
            if(pos + 1 <= nargs) {
                irunt->memsize = atol(argv[pos + 1]);
            } else {
                runt_print(vm, "Not enough arguments for -c\n");
            }
            *n = 2;
            return RUNT_OK;
        default:
            runt_print(vm, "Error: Couldn't find flag %s\n", argv[pos]);
            break;
    }
    return RUNT_NOT_OK;
}

/*TODO: simplify arg parsing function. Too many "clever" hacks. */
static int irunt_parse_args(irunt_data *irunt, int argc, char *argv[])
{
    runt_int a;
    runt_int argpos = argc;
    runt_int n;

    if(argc == 1) return 0;

    for(a = 1; a < argc; a++) {
        n = 0;
        if(argv[a][0] == '-') {
            if(irunt_get_flag(irunt, argv, a, argc, &n) != RUNT_OK) return -1;
            argpos -= n;
            a += (n - 1);
        } else {
            break;
        }
    }

    return argpos;
}

static void irunt_init(irunt_data *irunt)
{
    runt_init(&irunt->vm);
    irunt->batch_mode = 1;
    irunt->ncells = 512;
    irunt->memsize = 4 * RUNT_MEGABYTE;
}

runt_int irunt_begin(int argc, char *argv[], runt_int (*loader)(runt_vm *))
{
    irunt_data irunt;
    char  *line = NULL;
    size_t len = 0;
    ssize_t read;
    runt_vm *vm;
    int argpos = 0;

    irunt_init(&irunt);
    argpos = irunt_parse_args(&irunt, argc, argv);

    if(argpos < 0) return 1;

    argv = &argv[argc - argpos];
    argc = argpos;
    vm = &irunt.vm;

    vm->argc = argc;
    vm->argv = argv;
    vm->argpos = argpos;

    irunt.mem = malloc(irunt.memsize);
    irunt.cells = malloc(sizeof(runt_cell) * irunt.ncells);

    runt_cell_pool_set(vm, irunt.cells, irunt.ncells);
    runt_cell_pool_init(vm);

    runt_memory_pool_set(vm, irunt.mem, irunt.memsize);

    vm->loader = loader;
    loader(vm);
    runt_pmark_set(vm);

    if(irunt.batch_mode) {
        runt_set_state(vm, RUNT_MODE_INTERACTIVE, RUNT_ON);
    }

    if(argc > 1) load_dictionary(vm, argv[1]);
    if(irunt.batch_mode && argc == 0) load_dictionary_handle(vm, stdin);
    if(!irunt.batch_mode) {
        runt_set_state(vm, RUNT_MODE_INTERACTIVE, RUNT_ON);
        while(runt_is_alive(vm) == RUNT_OK) {
            printf("> ");
            fflush(stdout);
            read = runt_getline(&line, &len, stdin);
            parse(vm, line, read);
        }
    }

    runt_close_plugins(vm);

    free(line);
    free(irunt.cells);
    free(irunt.mem);

    return 0;
}
