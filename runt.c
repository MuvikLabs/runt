#include <stdlib.h>
#include <string.h>
#include "runt.h"

static runt_int runt_proc_zero(runt_vm *vm, runt_ptr p);
static runt_int runt_proc_link(runt_vm *vm, runt_ptr p);

runt_int runt_init(runt_vm *vm)
{
    vm->status = 0;
    vm->zproc = runt_proc_zero;
    vm->nil = runt_mk_ptr(RUNT_NIL, NULL);

    /* init stack */

    vm->stack.pos = 0;
    vm->stack.size = RUNT_STACK_SIZE;
    return RUNT_OK;
}

runt_int runt_cell_pool_set(runt_vm *vm, runt_cell *cells, runt_uint size)
{
    vm->cell_pool.cells = cells;
    vm->cell_pool.size = size;
    vm->cell_pool.used = 0;
    return RUNT_OK;
}

runt_int runt_cell_pool_init(runt_vm *vm)
{
    runt_uint i;
    runt_cell *cells = vm->cell_pool.cells;

    for(i = 0; i < vm->cell_pool.size; i++) {
       runt_bind(vm, &cells[i], runt_proc_zero);
       cells[i].psize = 1;
    }

    return RUNT_OK;
}

runt_int runt_memory_pool_set(runt_vm *vm, unsigned char *buf, runt_uint size)
{
    vm->memory_pool.data = buf;
    vm->memory_pool.size = size;
    vm->memory_pool.used = 0;
    memset(buf, 0, size);
    return RUNT_OK;
}

runt_uint runt_new_cell(runt_vm *vm, runt_cell **cell)
{
    runt_uint id;
    runt_cell_pool *pool = &vm->cell_pool;
    if(pool->used >= pool->size) {
        return 0;
    }
    pool->used++;
    if(vm->status & RUNT_MODE_PROC) vm->proc->psize++;
    id = pool->used;
    *cell = &pool->cells[id - 1];
    return id;
}

runt_ptr runt_mk_ptr(runt_type type, void *ud)
{
    runt_ptr ptr;
    ptr.type = type;
    ptr.ud = ud;
    return ptr;
}

runt_int runt_bind(runt_vm *vm, runt_cell *cell, runt_proc proc)
{
    cell->fun = proc;
    cell->p = vm->nil;
    return RUNT_OK;
}

runt_int runt_call(runt_vm *vm, runt_cell *cell)
{
    return cell->fun(vm, cell->p);
}

static runt_int runt_proc_zero(runt_vm *vm, runt_ptr p)
{
    return RUNT_OK;
}

runt_int runt_proc_begin(runt_vm *vm, runt_cell *proc)
{
    vm->status |= RUNT_MODE_PROC;
    vm->proc = proc;
    return RUNT_OK;
}

runt_int runt_proc_end(runt_vm *vm)
{
    vm->status &= ~(RUNT_MODE_PROC);
    return RUNT_OK;
}

runt_int runt_exec(runt_vm *vm, runt_cell *cell)
{
    runt_uint i;
    runt_int rc = RUNT_OK;
  
    if(cell->psize == 1) {
        return runt_call(vm, cell);
    }

    for(i = 1; i < cell->psize; i++) {
        if(cell[i].psize == 1) {
            rc = runt_call(vm, &cell[i]);
        } else {
            rc = runt_exec(vm, &cell[i]); 
        }
    }

    return rc;
}

runt_int runt_link_cell(runt_vm *vm, runt_cell *src, runt_cell *dest)
{
    /* TODO: error handling for init */
    dest->fun = runt_proc_link;
    dest->p = runt_mk_ptr(RUNT_CELL, src);
    return RUNT_OK;
}

static runt_int runt_proc_link(runt_vm *vm, runt_ptr p)
{
    runt_cell *cell = runt_to_cell(p);
    return runt_exec(vm, cell);
}

runt_cell * runt_to_cell(runt_ptr p)
{
    runt_cell *cell = NULL;
    /* TODO: error handling */
    if(p.type == RUNT_CELL) {
        cell = (runt_cell *)p.ud;
    }
    return cell;
}

runt_float * runt_to_float(runt_ptr p)
{
    runt_float *f;

    /* TODO: error handling */
    if(p.type == RUNT_FLOAT) {
        f = (runt_float *)p.ud;
    }

    return f;
}

runt_ptr runt_mk_float(runt_vm *vm, runt_float ival)
{
    runt_ptr p;
    float *val;
    runt_malloc(vm, sizeof(runt_float), (void**)&val);
    p = runt_mk_ptr(RUNT_FLOAT, val);
    *val = ival;
    return p;
}

const char * runt_to_string(runt_ptr p)
{
    const char *str;

    /*TODO: error handling */
    if(p.type == RUNT_STRING) {
        str = p.ud;
    }

    return str;
}

runt_ptr runt_mk_string(runt_vm *vm, const char *str, runt_uint size)
{
    runt_ptr p;
    char *buf;
    runt_uint i;
    runt_malloc(vm, size + 1, (void *)&buf);

    for(i = 0; i < size; i++) {
        buf[i] = str[i];
    }

    buf[size] = 0;

    p = runt_mk_ptr(RUNT_STRING, (void *)buf);

    return p;
}

runt_stacklet * runt_push(runt_vm *vm)
{
    /*TODO: error handling for stack overflows */
    vm->stack.pos++;
    return &vm->stack.stack[vm->stack.pos - 1];
}

runt_stacklet * runt_pop(runt_vm *vm)
{
    /*TODO: error handling for stack underflows */
    if(vm->stack.pos > 0) {
        vm->stack.pos--;
        return &vm->stack.stack[vm->stack.pos];
    }
    
    return &vm->stack.stack[0];
}

runt_uint runt_malloc(runt_vm *vm, size_t size, void **ud)
{

    runt_memory_pool *pool = &vm->memory_pool;
    runt_uint id = 0;

    /* TODO: overload error handling */
    if(pool->used + size >= pool->size) {
        return 0;
    }
  
    *ud = (void *)&pool->data[pool->used];

    id = pool->used + 1;
    pool->used += size;

    return id;
}
