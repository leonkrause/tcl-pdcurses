PDCurses SDL2 Tcl extension
===========================

TclOO C extension for PDCurses' SDL2 platform.

Currently only supports `add` (`addch`, `addchstr` & `addstr`), `getch` and some
options using `opt`.
The `PDC_FONT` environment variable can be used to specify the path of a
`.ttf` file, which is useful if the default font is not available.

Building
--------

Link with tclstub, pdcurses, SDL2 and SDL2_ttf:

	gcc -shared -fpic -o out/tclpdc.so tclpdc.c -ltclstub8.6 -lpdcurses -lSDL2 -lSDL2_ttf

PDCurses library must be built for SDL2 platform with `WIDE=Y`.
