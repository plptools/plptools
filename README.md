# plptools

https://github.com/plptools/plptools/

plptools is a suite of programs for transferring files to and from EPOC
(Psion) devices, as well as backing them up, installing software, and
setting the clock. See below for build instructions and HISTORY for some
history.

See the man pages for documentation: ncpd(8), plpftp(1), sisinstall(1),
plpprintd(8), and, where installed, plpfuse(8).


## Building From Source

Providing detailed instructions on how to build plptools for different operating systems is beyond the scope of this README. For specific information on Linux distros and other OSes, please read the following sources:

- The [Builds page in the wiki](https://github.com/plptools/plptools/wiki/Builds)
- The [GitHub CI workflow](.github/workflows/c-cpp.yml) for the necessary install and configuration steps for Ubuntu/Debian and macOS.

What follows are general notes on building plptools from source.

### Getting started

If you are building from source, you’ll need the following packages installed:

- GNU make
- automake
- autoconf
- pkg-config

Then run:

```shell
./bootstrap --skip-po
```

Some extra packages are needed; `bootstrap` will tell you what you need to install if anything is missing from your system.

plptools uses GNU autotools, so the usual sequence of commands works:

```shell
./configure
make
make install
```

In addition to the usual options, `configure` understands the following:

- `--with-serial=/dev/sometty`

  Sets the default serial device for ncpd. Without this option, ncpd tries automagically to find a serial device.

- `--with-speed=baudrate`

  Sets the default serial speed. This overrides the default behaviour of walking through 115200, 57600, 38400, 19200 and 9600 baud.

- `--with-port=portnum`

  Sets the default port on which ncpd listens and to which programs like plpftp and plpfuse connect (default 7501).

- `--with-drive=drivespec`

  Sets the default drive for plpftp. The default `AUTO` triggers a drive-scan on the psion and sets the drive to the first drive found. If you don't want that, specify `C:` for example.

- `--with-basedir=dirspec`

  Overrides the default directory for plpftp. The default is `\`,  which means the root directory.

  **Note:** since backslashes need to be doubled once for C escaping and once for shell escaping, this value is actually supplied as `\\\\`.

### plpftp

For command-line editing and history support in plpftp, Readline 4.3 or later (or a compatible library) is required.

### plpfuse

To build plpfuse, the following packages are required:

- FUSE 2: https://github.com/libfuse/libfuse (MacFUSE on macOS)
- libattr: https://savannah.nongnu.org/projects/attr (not required on macOS or *BSD)

## Development

The git repository can be cloned with:

```shell
git clone https://github.com/plptools/plptools.git
```

## License

plptools is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2 of the License, or (at your option)
any later version. For further details, see the LICENSE file.

plptools includes the following separately licensed third-party libraries and components:

- [doctest](https://github.com/doctest/doctest) (/tests/doctest.h), MIT License
