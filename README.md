# threason
An experimental pure C2x multi-threaded JSON parser library. 
Buffer parsing and querying/walking only, no streaming and no creation/modification.

## The Idea
My personal goal was to try my hand at designing and implementing a high-quality limited-size project in pure C, thus `-Wall -Wextra -Werror -pedantic -std=c2x` and no libraries used other than `libc`.
This was mostly acheived, the only non-standard extension used is `qsort_r`.
I ended up reimplementing basic stuff from Rust's stdlib like slices, `Result` and growable vectors and bulding upon it.

The original technical goal was to experiment with multi-threaded JSON parsing, thus **threa**son. 
This has been acheived as well.

## Tools used
As an experiment most of the code was written in Linux framebuffer console using Zellij, Helix, clangd and clang-format, with no graphical environment whatsoever.
The experience is surprisingly pleasant and unsurprisingly disctraction-free.

## Benchmarks
Up to 5x speedup at buffer parsing with 8 threads compared to single thread parsing.
Depends on the document in question tho.