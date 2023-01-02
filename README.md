# threason
An experimental pure C2x JSON parser library designed around single contignious buffer, with fewer allocations and better cache-locality. 
Parsing and querying/walking only, no creation/modification.

## The Idea
My personal goal was to try my hand at designing and implementing a high-quality limited-size project in pure C, thus `-Wall -Wextra -Werror -pedantic -std=c2x` and no libraries used other than `libc`.
This was mostly acheived, the only non-standard extension used is `qsort_r`.
I ended up reimplementing basic stuff from Rust's stdlib like slices, `Result` and growable vectors and bulding upon it.

The original technical goal was to experiment with multi-threaded JSON parsing, thus *threa*son.
This isn't done (yet?), mostly because I keep reiterating on the single-threaded version.

## Benchmarks
Incoming.
