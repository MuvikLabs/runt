#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include "runt.h"

#define CELLPOOL_SIZE 256
#define MEMPOOL_SIZE 4 * RUNT_MEGABYTE

static void parse(runt_vm *vm, char *str, size_t read)
{
    const char *code = str;
    runt_mark_set(vm);
    runt_compile(vm, code);
    runt_mark_free(vm);
}

static runt_int load_dictionary(runt_vm *vm, char *filename)
{
    FILE *fp; 
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen(filename, "r");

    if(fp == NULL) {
        fprintf(stderr, "Could not open file %s\n", filename);
        return RUNT_NOT_OK;
    }

    while((read = getline(&line, &len, fp)) != -1) {
        parse(vm, line, read);
    }

    free(line);
    return RUNT_OK;
}


static int rproc_rec(runt_vm *vm, runt_ptr p)
{
    fprintf(stderr, "Recording.\n");
    return runt_set_state(vm, RUNT_MODE_INTERACTIVE, RUNT_OFF);
}

static int rproc_stop(runt_vm *vm, runt_cell *src, runt_cell *dst)
{
    fprintf(stderr, "Stopping.\n");
    runt_cell_undo(vm);
    return runt_set_state(vm, RUNT_MODE_INTERACTIVE, RUNT_ON);
}

static int rproc_usage(runt_vm *vm, runt_ptr p)
{
    printf("Cell pool: used %d of %d cells.\n", 
            runt_cell_pool_used(vm),
            runt_cell_pool_size(vm));

    printf("Memory pool: used %d of %d bytes.\n", 
            runt_memory_pool_used(vm),
            runt_memory_pool_size(vm));
    return RUNT_OK;
}

int main(int argc, char *argv[])
{
    runt_vm vm;
    unsigned char *mem;
    runt_cell *cells;

    char  *line = NULL;
    size_t len = 0;
    ssize_t read;


    runt_init(&vm);

    mem = malloc(MEMPOOL_SIZE);
    cells = malloc(sizeof(runt_cell) * CELLPOOL_SIZE);

    runt_cell_pool_set(&vm, cells, CELLPOOL_SIZE);
    runt_cell_pool_init(&vm);

    runt_memory_pool_set(&vm, mem, MEMPOOL_SIZE);
  
    runt_load_stdlib(&vm);


    if(argc > 1) load_dictionary(&vm, argv[1]);

    runt_word_define(&vm, "rec", 3, rproc_rec);
    runt_word_define_with_copy(&vm, "stop", 4, vm.zproc, rproc_stop);

    runt_word_define(&vm, "u", 1, rproc_usage);

    runt_set_state(&vm, RUNT_MODE_INTERACTIVE, RUNT_ON);

    while(runt_is_alive(&vm) == RUNT_OK) {
        printf("> ");
        read = getline(&line, &len, stdin);
        parse(&vm, line, read);
    }

    printf("Cell pool: used %d of %d cells.\n", 
            runt_cell_pool_used(&vm),
            runt_cell_pool_size(&vm));

    printf("Memory pool: used %d of %d bytes.\n", 
            runt_memory_pool_used(&vm),
            runt_memory_pool_size(&vm));

    free(line);
    free(cells);
    free(mem);

    return 0;
}
