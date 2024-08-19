A C++ implementation of Conway's game of life.

Includes some generic classes of questionable quality + Template Meta Programming.

Use  `cmake -S src -B build` to build

and

`./build/src/main` to run

To change, manually change the call to `run_simulation()`.
May also be used outside of this code, for example, with a GUI.

`./profiler.sh` - script to view performance with gprof + gprof2dot + xdot. Only works when compiled in debug mode. For more info, use google.

This was a technical test for semi-optimized code and usage of a profiler.
A side effect was learning performance pitfalls. Specifically indirection, inefficient IO and helping the compiler by hinting at how variables are used. 

Saddly, the work lacks a propper commit history.

Concrete implementations in vects.cpp and chunks.cpp can be exported and compiled down to actual libraries.
As-is, we are butchering the compiler by just including everything.
Im unsure as to why 20 percent of the program is spent on `Vect2i::Vect2i::operation-` and argument constructor.
Also, over 200% percent of IOs (in this case it isnt really an IO, but the data is in a byte array in a hash table so might as well be) are redundant. This is a stagering reduction from over 900% percent. This was resolved by processing the insides of a chunk without the edges, plus storing sums in collumns of 3 cells. As im writing this, one way to optimize this code is to just side-step the indirection of ChunkLoader while doing this. Also the  `tick_optimized()` function is an eye-sore.

To go on, 2 chunks easily fit into 4KB, and given the 2 switching buffers design, just fusing chunk-loader to and from into one entity, allocating 2x chunks, is not a terrible option. 
------

Copyright 2024 by Sasho B

All rights reserved.
Unauthorized use, reproduction, or distribution of this code will be persued to the fullest extent of the law.

...nah im just messing with you, this code is licenced under GNU GPLv3 by the Free Software Foundation.
Due to the code's trivial nature, no attempt will be made to enforce this either way.
