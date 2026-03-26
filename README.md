# EXIP - Embeddable EXI Processor in C

![CI](https://github.com/ekrich/exip/workflows/CI/badge.svg)

![EXIP Logo](docs/www/logo.png)

## Overview

The main objective of the EXIP project is to provide a C library for parsing and serialization of [Efficient XML Interchange (EXI)](https://www.w3.org/TR/exi/) streams which is a [W3C](https://www.w3.org/) standard. The focus is portability and efficiency for embedded systems development. The library features an efficient, typed, and low-level API. The library should also work for enterprises that want to use C or C++ languages.

* The web site and documenation is available at [https://ekrich.github.io/exip/](https://ekrich.github.io/exip/).

## Current status

We are working to improve this project so it can be used more widely. Quite a bit of work has been done to resurrect the project with documentation and code improvements. Contributions are encouraged but can be slow to adopt based on time available, level of effort, and knowledge of the code base.

## Short History

The code was pulled from the SourceForge SVN repository with all commits and tags. There was [one outstanding issue](https://github.com/ekrich/exip/issues/1) that has been fixed. The project originally supported the Contiki embedded IoT platform and now compiles on the next generation [Contiki-ng](https://github.com/contiki-ng/contiki-ng) project.

Formerly this project was not maintained on [SourceForge](https://sourceforge.net/projects/exip/). All historical information is found there as well as the [original web site](https://exip.sourceforge.net/).

## Supported Platforms

The library is highly portable C code and supports the platforms using CI as shown in the next table.


| OS         | Works | Version(s)              | Arch(s)                | Compiler(s)
| ---------- | ----- | ----------------------- | ---------------------- | --------------- |
| macOS      |   ✅  | 13.6.x                  | arm64/aarch64          | gcc (clang)     |
| Linux      |   ✅  | Ubuntu 22.0.4           | x86_64                 | gcc             |
| Windows    |   ✅  | Visual Studio 2022      | x86/x86_64             | cl              |
| Contiki-ng |   ✅  | latest                  | ARM Cortex-M3 (32-bit) | gcc (c11)       |

* The build system calls `gcc`.
* macOS system compiler `gcc` is aliased to `clang`.
* `clang` should also work on Linux.
* Windows default creates a 32bit x86 executable.
* We now use C99 standard since we refactored to use `bool`.
* Contiki-ng build uses the C11 standard.

## Users/Developers and Contributors

Please clone the repository or download a version by clicking on the *Releases* or *Tags* above.

All the documentation and the User Guide is available via the web site found at [https://ekrich.github.io/exip/](https://ekrich.github.io/exip/).

See the additional tool installation and setup documentation for [Developers](info/DEVELOPERS.md).

## Licenses

 - Current license: [Apache License, Version 2.0](LICENSE.txt)
 - Original license: [EISLAB](LICENSE-orig.txt)
