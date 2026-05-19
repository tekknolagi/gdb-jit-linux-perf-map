Run GDB, load the reader (make sure to use the full path or it won't load),
run, and see the name in the backtrace.

```
cedar% make
cc -shared -o libreader.so reader.c
cc -ggdb -o main main.c
cedar% gdb -q ./main                                             
Reading symbols from ./main...
(gdb) jit-reader-load /home/max/Documents/code/jitgdb-custom/libreader.so
(gdb) r
Starting program: /home/max/Documents/code/jitgdb-custom/main 
[Thread debugging using libthread_db enabled]
Using host libthread_db library "/lib/x86_64-linux-gnu/libthread_db.so.1".
my pid is 3050084
JITed symbol file is not an object file, ignoring it.

Program received signal SIGTRAP, Trace/breakpoint trap.
0x00007ffff7ffa001 in my_jit_function ()
(gdb) # :)
```

Unfortunately if you run `disassemble my_jit_function` it doesn't recognize the
function for some reason.
