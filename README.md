# Runt
By Paul Batchelor

Runt is a quirky stack-based language and virtual machine. 

Here are some features:

- Stack-based: heavily inspired by Forth
- Memory-pool allocation system: only one call to malloc/free needed!
- built-in parser and interpreter
- written in ANSI C
- simple API for defining new words
- Can be run inside of Polysporth plugin as a scheme-extension

## Compilation

To do a full compile of Runt and the Runt Polysporth extension run:

    make

This assumes that Sporth is installed. 

If you want to just compile the runt interpreter (and implicitly compile 
librunt):

    make irunt 


To install run:

    make install

Note that you do NOTE need sudo, since the files will be installed in ~/.runt/

To clean:

    make clean


## Using Runt

Starting up irunt will get you to a prompt, where you can try things out.

"Hello world" in runt looks like this:

    > "hello world!" say
    hello world!

To make procedures, *record mode* must be turned on, which allows you to
record to the cell pool. To return to interactive mode, you must stop:

    > rec
    Recording.
    > : foo "runt is cool!" say ;
    > stop
    Stopping.
    > foo
    runt is cool!

Runt currently has supported for basic floating-point arithmetic. The "p" command
pops the value from the stack and prints it.

    > 1 3 + p
    4
    > 3 2 - p
    1
    > 10 3 / p
    3.3333
    > 12345 54321 * p
    6.70593e+08

Some forth-like stack operations like swap and dup are also supported currently:

    > rec : pow dup * ; stop
    Recording.
    Stopping.
    > 5 pow p
    25

## Plugins

Plugins are procedures written in C that can be dynamically loaded at 
runtime. They are an ideal way to "glue" other C-libraries together. 

Example code for a plugin exists in *plugin.c*. To compile it, run:

    make plugin.so

Once it is compiled, you can start up irunt in the directory it is in, 
and load it into the runt cell pool:

    > "./plugin.so" dynload
    > test
    this is a plugin!

