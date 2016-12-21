Intro
=====

This is the `README` file for `aenig4`, the *Enigma M4 Emulator*.

`aenig4` emulates the Enigma M4 cypher machine used by the U-boot division of
the German Navy during World War II. It can be used as well to emulate the
Enigma I machine (M1, M2, M3).

aenig4 is free software. See the file `COPYING` for copying conditions.

Copyright (c) 2016 Jorge Giner Cordero

Home page: http://jorgicor.sdfeu.org/aenig4  
Send bug reports to: jorge.giner@hotmail.com

Windows precompiled binaries
============================

A precompiled distribution for *Microsoft Windows* is provided in a ZIP file.
It can be found at http://jorgicor.sdfeu.org/aenig4 . After decompressing it,
you will have these files:

~~~
aenig4.exe      The program.
aenig4.txt      This file.
copying.txt     License.
changelog.txt   What's new in this version.
~~~

Compiling
=========

Getting the code from revision control
--------------------------------------

If you cloned the project from a revision control system (i.e. GitHub), you
will need first to use the GNU autotools to generate some files, in particular,
the `configure` script. Use:

    $ ./bootstrap

to generate the required files. You will need *GNU autoconf*, *GNU automake*,
*GNU texinfo* and *help2man*.

Compiling from the source distribution
--------------------------------------

If you have the official source package, and you are building on a Unix
environment (this includes *Cygwin* on *Windows*), you can find detailed
instructions in the file `INSTALL`. The complete source distribution can always
be found at http://jorgicor.sdfeu.org/aenig4 .  

After installing, you can type `man aenig4` to see a brief explanation on how
to use the `aenig4` program. More detailed documentation can be found using the
GNU documentation system: type `info aenig4` to read it.

Normally, after installing from source, you can find this on your system:

~~~
/usr/local/bin/aenig4                    The program executable.
/usr/local/share/man/man1/aenig4.1       The manual page.
/usr/local/share/info/aenig4.info        The info manual.
/usr/local/share/doc/uz80as/COPYING      License.
/usr/local/share/doc/uz80as/README.md    This file.
/usr/local/share/doc/uz80as/CHANGELOG.md What's new in this version.
~~~

If you are installing aenig4 using your OS distribution package system, these
folders will probably be different. Try changing `/usr/local` to `/usr`.

