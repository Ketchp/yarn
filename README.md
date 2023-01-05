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

To build with cmake first run:
```bash
mkdir build_dir
cd build_dir
cmake ..
```
To create documentation for library you can start at projects root;
```bash
make docs
```

Documentation main-page will be located at `build_dir/docs/html/index.html`.

To run test you can run:
```bash
make test
```

To build library:
```bash
make yarn
```

Library will be at `build_dir/src/libyarn.a`.
