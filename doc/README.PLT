Ok, this file documents how PIC funtion calls work. It only works
if you were compiled with -fPIC.

The basic idea is that there is a double-indirection to access
the function.

1) Calling a function
---------------------
If you want to call the function, you must call into your PLT.
There is one PLT per binary mapped in memory. The PLT is a
read-only text (code) section which reads the address of the
function to call into from the GOT and then calls into
that function. Normally, the data read from the GOT contains
the address of the code of the function. However, this is not
the case the first time you read the data from the GOT. The
first time, the data is the address to a piece of code
which pushes on the stack an index which references the symbol 
of the function to call (the index is actually a relocation offset
in the relocation table of the binary. The relocation entry
which is pointed to by this offset contains a reference in the
binary's symbol table to the symbol name. The relocation entry
also points to the GOT entry which is supposed to contain
the function address). The following instruction jumps into 
another piece of code which does two things. First, it pushes 
on the stack the content of 4@GOT. Then, it jumps into the 
address located in 8@GOT. 4@GOT contains an identifier which 
identifies the binary in which we want to resolve the symbol. 
8@GOT contains the address of the program's loader symbol 
resolution routine which reads from its stack the symbol to resolve.

The loader's symbol resolution routine resolves the symbol
identified by the relocation entry, stores the resulting 
address into the GOT entry pointed to by the relocation entry
and jumps to that address.

Later, since the GOT entry now contains directly the address of
function code, the client code calls directly into the target
function rather than going through the loader's symbol resolution
code.

2) getting the address of a function
------------------------------------
If you want to get the address of a function, the process is 
very different and the address you will get does not correspond
to the address of the code of the function. The address you get is
the data located into a special GOT entry which contains the "address"
of the function. This GOT entry is resolved at load time by the 
loader through a special relocation entry of type R_386_GLOB_DAT
which contains a reference to the symbol name in the symbol table
and a reference to the location in the binary data to update. This 
GOT entry is different from the GOT entry associated to the PLT entry
for the same function. Specifically, it is initialized to the 
address of the PLT entry of this function "in the main binary file" or
to the address of the code of the function if "the main binary file"


