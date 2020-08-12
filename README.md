# xprintidle #

xprintidle is a utility that queries the X server for the user's idle
time and prints it to stdout (in milliseconds).

## Building and Installing ##

Basically, type "meson build && ninja -C build install" to compile
and install the program.

### Dependencies ###

You need the development files for the X11, Xext and Xss library.
