# plptools

https://github.com/plptools/plptools/

plptools is a suite of programs for transferring files to and from EPOC
(Psion) devices, as well as backing them up, installing software, and
setting the clock. See below for build instructions and HISTORY for some
history.

plptools is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2 of the License, or (at your option)
any later version.

See the man pages for documentation: ncpd(8), plpftp(1), sisinstall(1),
plpprintd(8), and, where installed, plpfuse(8).


## Building from source

To build plpfuse, the following packages are required:

* GNU Make
* FUSE: https://github.com/libfuse/libfuse (MacFUSE on macOS)
* libattr: https://savannah.nongnu.org/projects/attr (not required on macOS or BSD)

For command-line editing and history support in plpftp, Readline 4.3 or later or a compatible library is required.

Providing detailed instructions on how to install these packages for different operating systems is beyond the scope of this README, but see the [GitHub CI workflow](.github/workflows/c-cpp.yml) for the necessary install and configuration steps for Ubuntu/Debian and macOS.

If building from a git checkout, you’ll need the following packages installed: automake, autoconf, pkg-config.

Then run:

```
./bootstrap --skip-po
```

Some extra packages are needed; `bootstrap` will tell you what you need to install if anything is lacking.

plptools uses GNU autotools, so the usual sequence of commands works:

```
./configure
make
make install
```

In addition to the usual options, `configure` understands the following:

<dl>
<dt><tt>--with-serial=/dev/sometty</tt></dt>
<dd>sets the default serial device for ncpd. Without this option, ncpd tries automagically to find a serial device.</dd>
<dt><tt>--with-speed=baudrate</tt></dt>
<dd>sets the default serial speed (normally 115200 baud).</dd>
<dt><tt>--with-port=portnum</tt></dt>
<dd>sets the default port on which ncpd listens and to which plpftp and plpfuse connect (default 7501).</dd>
<dt><tt>--with-drive=drivespec</tt></dt>
<dd>sets the default drive for plpftp. The default <tt>AUTO</tt> triggers a drive-scan on the psion and sets the drive to the first drive found. If you don't want that, specify <tt>C:</tt> for example.</dd>
<dt><tt>--with-basedir=dirspec</tt></dt>
<dd>overrides the default directory for plpftp. The default is <tt>\</tt>,  which means the root directory. Note: since backslashes need to be doubled once for C escaping and once for shell escaping, this value is actually supplied as <tt>\\\\</tt>.</dd>


## Information for developers

The git repository can be cloned with:

git clone https://github.com/plptools/plptools.git

To make a release you need gh: https://cli.github.com
