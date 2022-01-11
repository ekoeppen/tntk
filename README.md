tntk
====

Command line NewtonScript compiler and packager

PREREQUISITES
-------------

* [CMake](http://www.cmake.org/cmake/resources/software.html)
* [cDCL](http://github.com/ekoeppen/cDCL)


INSTALLATION
------------

1. Install CMake and cDCL, if you haven't already
2. `git clone --recursive git@github.com:ekoeppen/tntk.git`
3. `cd tntk`
4. `cmake -S . -B build`
5. `cmake --build build`
6. `sudo cmake --install build`

You will also need the [NTK Platform Files](http://www.unna.org/view.php?/apple/development/NTK/platformfiles).

USAGE
-----

    tntk [options] file
      -c    compile-only
      -d    dump contents of a .pkg file
      -l    log file name
      -p    serial port device path
      -P    platform files search path
      -s    serial port speed
      -t    tcp connection

A sample project is included, which can be compiled as follows:

1. Update MiniNewtApp.nprj with the path to your platform file
2. Run `tntk -c MiniNewtApp.nprj`

For further examples of usage, see the following articles:

* [Fun with NEWT/0](http://40hz.org/Pages/Mottek:%202010-11-23)
* [Autoparts with tntk](http://40hz.org/Pages/Mottek:%202011-01-09)
* [Developing Newton apps with Einstein, TextMate and tntk](http://40hz.org/Pages/Mottek:%202011-01-16)

NATIVE CODE SUPPORT
-------------------

tntk can process NTCK files, and add relocation data as needed to the output package. For this
to work, the binary module must have been linked with a base address larger than zero, e.g. 0x10000
is a good value, and cDCL version 1.0 must be used to convert the ELF file to an NTCK module.
