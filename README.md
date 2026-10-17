# plptools

[![build](https://github.com/plptools/plptools/actions/workflows/build.yml/badge.svg)](https://github.com/plptools/plptools/actions/workflows/build.yml)

https://github.com/plptools/plptools/

plptools is a suite of programs for transferring files to and from EPOC
(Psion) devices, as well as backing them up, installing software, and
setting the clock. See below for build instructions and HISTORY for some
history.

See the man pages for documentation: ncpd(8), plpftp(1), sisinstall(1),
plpprintd(8), and, where installed, plpfuse(8).

## Building

Where possible, plptools leans on autotools to handle per-platform variations and ensure builds can be performed consistently on all supported operating systems. Differences that cannot be handled this way (or haven't yet been migrated) are variously documented here, on the [Builds wiki page](https://github.com/plptools/plptools/wiki/Builds), or handled in `build-aux/build.sh`—we are in the process of formalizing our supported platforms and things are likely to evolve over time.

### Dependencies

If you are building from source, you’ll need the following packages installed:

- GNU make
- automake
- autoconf
- pkg-config

You will need additional libraries to enable the following functionality:

- plpfuse – requires readline 4.3 or later (or compatible library) for command-line editing and history support
- plpfuse – requires [FUSE 2](https://github.com/libfuse/libfuse) (MacFUSE on macOS) and [libattr](https://savannah.nongnu.org/projects/attr) (not required on macOS or *BSD)

### Standard Builds

Builds using the default configuration on Linux, macOS, and Haiku can be performed using the `build-aux/build.sh` wrapper script. This matches exactly how builds are performed in CI:

```sh
build-aux/build.sh
```

_Attempting this on other platforms will direct you to the [Builds wiki page](https://github.com/plptools/plptools/wiki/Builds)._

### Custom Builds

1. Bootstrap the project:

   ```sh
   ./bootstrap --skip-po
   ```

   _If additional packages are required; `bootstrap` will tell you what you need to install if anything is missing from your system._

2. Configure the project:

   ```sh
   ./configure
   ```

3. Compile:

   ```sh
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

## Development

The git repository can be cloned with:

```shell
git clone https://github.com/plptools/plptools.git
```

### Tests

Tests are implemented using [doctest.h](https://github.com/doctest/doctest). Configure the project then run the tests as follows:

```sh
make check
```

The tests are compiled into their own target (`tests/tests`) which provides additional command line options when run directly.

For example, you can list all the test cases as follows:

```plaintext
$ tests/tests --list-test-cases
[doctest] listing all test case names
===============================================================================
DeviceConfiguration accessors
DeviceConfiguration::serialize
DeviceConfiguration::deserialize
Drive::getPath
parse_ini
cli_utils::is_number
cli_utils::lookup_default_port
cli_utils::parse_port
pathutils::epoc_basename
pathutils::epoc_dirname
pathutils::is_absolute
pathutils::appending_components
pathutils::split
pathutils::join
pathutils::resolve_path
PlpDirent::getPath
===============================================================================
```

Or run a specific one:

```plaintext
$ tests/tests --test-case=cli_utils::is_number
[doctest] doctest version is "2.4.12"
[doctest] run with "--help" for options
===============================================================================
[doctest] test cases: 1 | 1 passed | 0 failed | 15 skipped
[doctest] assertions: 6 | 6 passed | 0 failed |
[doctest] Status: SUCCESS!
```

## Contributing

We welcome and encourage contributions. We require that all code, pull requests, and issues are human-authored (not AI-generated) and that contributors have undertaken due diligence to ensure they are legally able to assert copyright and grant usage rights. Please endeavor to keep changes small and reviewable. We will close pull requests and issues that violate this policy. This is intended to ensure the quality, sustainability and on-going security and longevity of the plptools project.

We have serious concerns about the copyright, societal, and environmental implications of LLMs and generative AI as implemented today. However, we recognize these tools can provide utility when applied to certain investigative and reverse engineering tasks. We do not wish to blindly reject learnings resulting from the use of these tools, but require disclosure of such usage and request careful consideration of the environmental impact.

## Code of Conduct

This is an inclusive project. We will not accept any form of racism, sexism, or any other discrimination in our community. Please acknowledge that plptools is developed and supported by volunteers and be respectful in all communication.

## License

plptools is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2 of the License, or (at your option)
any later version. For further details, see the LICENSE file.

plptools includes the following separately licensed third-party libraries and components:

- [doctest](https://github.com/doctest/doctest) (/tests/doctest.h), MIT License
