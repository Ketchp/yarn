# Yarn library

This library is student project, it is intended mostly as practise and
possibly for school assignments. I would highly discourage to use it anywhere
else because C++(STL) and C(POSIX) libraries offers:
- more tested; I wrote tests that heavily tests all classes under various conditions,
but bugs in multithreaded environment depend on context switching and
bug can occur only if switch happens between certain two instructions.
- more portable; tested only in Linux
- more efficient implementations than most primitives offered by yarn.

With that said, yarn also contains/will contain higher level
synchronisation primitives that offer more control that standard libraries.

You can find whole documentation [here](docs/html/index.html).
