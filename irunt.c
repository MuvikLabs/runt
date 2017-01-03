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
} irunt_data;


static runt_int parse(runt_vm *vm, char *str, size_t read)
{
    const char *code = str;
    runt_pmark_set(vm);
    runt_compile(vm, code);
    runt_pmark_free(vm);
    return RUNT_OK;
}

static runt_int load_dictionary(runt_vm *vm, char *filename)
{
    FILE *fp; 
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    runt_int rc = RUNT_OK;

    fp = fopen(filename, "r");

    if(fp == NULL) {
        runt_print(vm, "Could not open file %s\n", filename);
        return RUNT_NOT_OK;
    }

    while((read = getline(&line, &len, fp)) != -1) {
        rc = parse(vm, line, read);
        if(rc == RUNT_NOT_OK) break;
    }

    fclose(fp);
    free(line);
    return RUNT_OK;
}

int irunt_get_flag(irunt_data *irunt, 
        char *argv[], 
        runt_int pos, 
        runt_int nargs)
{
    char flag = *(argv[pos] + 1);
    runt_vm *vm = &irunt->vm;
    switch(flag) {
        case 'b':
            irunt->batch_mode = 1;
            return RUNT_OK;
        default:
            runt_print(vm, "Error: Couldn't find flag %s\n", argv[pos]);
            break;
    }
    return RUNT_NOT_OK;
}

int irunt_parse_args(irunt_data *irunt, int argc, char *argv[])
{
    runt_int a;
    runt_int argpos = argc;

    if(argc == 1) return 0;

    for(a = 1; a < argc; a++) {
        if(argv[a][0] == '-') {
            argpos--; 
            if(irunt_get_flag(irunt, argv, a, argc) != RUNT_OK) return -1;
        } else {
            break;
        }
    }

    return argpos;
}

void irunt_init(irunt_data *irunt)
{
    runt_init(&irunt->vm);
    irunt->batch_mode = 0;
}

int main(int argc, char *argv[])
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


    irunt.mem = malloc(MEMPOOL_SIZE);
    irunt.cells = malloc(sizeof(runt_cell) * CELLPOOL_SIZE);

    runt_cell_pool_set(vm, irunt.cells, CELLPOOL_SIZE);
    runt_cell_pool_init(vm);

    runt_memory_pool_set(vm, irunt.mem, MEMPOOL_SIZE);
  
    runt_load_stdlib(vm);


    if(irunt.batch_mode) {
        runt_set_state(vm, RUNT_MODE_INTERACTIVE, RUNT_ON);
    }

    if(argc > 1) load_dictionary(vm, argv[1]);

    if(!irunt.batch_mode) {
        runt_set_state(vm, RUNT_MODE_INTERACTIVE, RUNT_ON);
        while(runt_is_alive(vm) == RUNT_OK) {
            printf("> ");
            read = getline(&line, &len, stdin);
            parse(vm, line, read);
        }
    }

    runt_close_plugins(vm);

    free(line);
    free(irunt.cells);
    free(irunt.mem);

    return 0;
}
