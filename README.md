Amanuensis
----------

A faithful assistant to your various network applications, scrupulously copying all that they say and preserving it for the ages.

This will, if I find the time to persevere, become a cross-platform HTTP proxy and web-traffic inspector - like Fiddler, but native and perpetually open-source.


### Build requirements

- QT 5.8+

On Windows:

- Visual Studio 2015

On Linux:

- TBD

On Mac:

- Xcode 8.x+ and command-line tools

### Testing

Qt Creator is adequate for running the unit tests.  If you decide that you'd like to run them on the command-line, good luck - qmake documentation _may_ or _may not_ work on your platform.  On Windows 10, at least, the following has worked:

0. Open a command prompt
0. Source vcvarsall.bat
0. Put qmake on your PATH
0. cd path/to/source
0. `qmake -r "M=2"`
0. `nmake check`


----------------------
Copyright (C) 2017 Benjamin Bader
