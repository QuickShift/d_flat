# d_flat

University project for the Programming Language Theory module.

A primitive transpiler for a very basic custom programming language D Flat.

Language categorization: procedural, statically + strongly typed.

Has got the standard procedural programming language kit: flow control (`if`), loops (`for`), functions, scopes (done implicitly due being translated into C). Available types: _char_, _int_, _float_, (hacky) _string_. Also got a special 'feature': inline C.

The whole code is located in the `transpiler.cpp` file. `.df` files are the D Flat example source files. Specified `.df` file is transpiled into a relatively readable `result.c` file, which is then compiled using a C compiler (in this case MSVC).

For example: a D Flat file `foo.df` can be built with the `build.bat` file using a command like this:

`build foo.df`

Launching the `run.bat` script with the command `run` will launch the compiled `result` executable.

## Used references:
* [Sean T. Barrett's C lexer](https://github.com/nothings/stb/blob/master/stb_c_lexer.h)
* [LLVM parser tutorial](https://llvm.org/docs/tutorial/index.html)
* [machinamentum's open source JAI transpiler](https://github.com/machinamentum/jai)
