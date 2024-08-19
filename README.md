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

------

Copyright 2024 by Sasho B

All rights reserved.
Unauthorized use, reproduction, or distribution of this code will be persued to the fullest extent of the law.

...nah im just messing with you, this code is licenced under GNU GPLv3 by the Free Software Foundation.
Due to the code's trivial nature, no attempt will be made to enforce this either way.
