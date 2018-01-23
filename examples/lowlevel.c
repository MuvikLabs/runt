#include <stdio.h>
#include <runt.h>

/* Some examples of the runt virtual machine */

static int magic = 123456;

runt_int rproc_hello(runt_vm *vm, runt_ptr p)
{
    int *val;
    /* get the data from the pointer and cast is a pointer integer. 
     * This is basically what runt_to_cptr does. */
    val = (int *)p.ud;
    printf("hello runt!\n");
    printf("The magic number is %d\n", *val);
    return RUNT_OK;
}

void basic_cell(void)
{
    runt_cell hello;
    runt_ptr p;

    /* The core building block of runt is the cell.
     * All it really is a data type containing a wrapper around a generic
     * void pointer (runt_ptr) and a callback with that pointer as an argument.
     *
     * This is so simple. You don't even need the full runt_vm struct. 
     *
     */

    /* The core of runt_cell_bind */
    hello.fun = rproc_hello;

    /* runt_ptr is a wrapper around a generic void * pointer. 
     * We can use this to store pointers to data, such as the
     * memory address of an integer. */

    p.ud = &magic;

    /* This pointer can then be bound to a cell. This essentially is
     * the function runt_cell_data */
    hello.p = p;

    /* This is exactly what runt_cell_call does */
    hello.fun(NULL, hello.p);

}

int main(int argc, char *argv[])
{
    /* read this function to learn about the basic cell */
    basic_cell();
    /* more to come some day... */
    return 0;
}
