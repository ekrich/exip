# EXIP - Embeddable EXI Processor in C

![CI](https://github.com/ekrich/exip/workflows/CI/badge.svg)

![EXIP Logo](docs/www/logo.png)

## Overview

The main objective of the EXIP project is to provide a C library for parsing and serialization of [Efficient XML Interchange (EXI)](https://www.w3.org/TR/exi/) streams which is a [W3C](https://www.w3.org/) standard. The focus is portability and efficiency for embedded systems development. The library features an efficient, typed, and low-level API. The library should also work for enterprises that want to use C or C++ languages.

* The web site and documenation is available at [https://ekrich.github.io/exip/](https://ekrich.github.io/exip/).

## Current status

Maintenance and developement is ongoing so EXIP can be used more widely. Significant work has been done to resurrect the project with documentation and code improvements. Contributions and feedback are encouraged.

## Short History

The project was created by [Rumen Kyusakov](https://github.com/kjussakov) as part of his PhD program at Luleå University of Technology. His thesis is titled [*Efficient Web Services for End-To-End Interoperability of Embedded Systems*](https://www.diva-portal.org/smash/record.jsf?pid=diva2:991136). Rumen was a [W3C Invited Expert in the Efficient XML Interchange Working Group](https://www.w3.org/groups/wg/exi/former-participants/) and contributed to the [EXI memory profile](https://www.w3.org/TR/exi-profile/) with feedback to the document.

The code was pulled from the [SourceForge](https://sourceforge.net/projects/exip/) SVN repository with all commits and tags in July of 2024. There was [one outstanding issue](https://github.com/ekrich/exip/issues/1) that was fixed. The project originally supported the Contiki embedded IoT platform and now compiles on the next generation [Contiki-ng](https://github.com/contiki-ng/contiki-ng) project. All historical information is found on SourceForge (link above) as well as the [original web site](https://exip.sourceforge.net/).

The first version `0.5.5` was released here on Apr 2, 2025. See the release details on [GitHub](https://github.com/ekrich/exip/releases/tag/v0.5.5).

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

## Licenses

 - Current license: [Apache License, Version 2.0](LICENSE.txt)
 - Original license: [EISLAB](LICENSE-orig.txt)

## Users/Developers and Contributors

Please clone the repository or download a version by clicking on the *Releases* or *Tags* above.

All the documentation and the User Guide is available via the web site found at [https://ekrich.github.io/exip/](https://ekrich.github.io/exip/).

## Developer Information

The following sections are for Users and Contributors to EXIP that need tools installed to use the library. There are no prepared packages that can be installed without compiling the code.

### VSCode Support

[VScode](https://code.visualstudio.com/) is supported by adding the [clangd](https://clangd.llvm.org/) plugin to get C Language Server Protocol (LSP) support. Find the official plugin [here](https://marketplace.visualstudio.com/items?itemName=llvm-vs-code-extensions.vscode-clangd).

The macOS `arm` processors have a non-standard include directory so there needs to be an entry in the `compile_flags.txt` file for `clangd` to support the Check library. This file is at the root of the project.


### Testing Dependency

The Check Unit Test Framework for C for testing. Link to the [Check](https://libcheck.github.io/check/) library. Please see the install notes [here](https://libcheck.github.io/check/web/install.html).

Use one of the following commands to install `Check`:

#### macOS

```sh
$ brew install check
```

#### Linux (Ubuntu/Debian)

```sh
$ apt-get install check
```

#### Windows

The easiest setup is to install Visual Studio Community Edition 2022 to get the latest Windows 10/11 SDK and the MSVC v143 tools. The following is the installer setup used for the current vs2022 setup that succeeded. Searching for vs2022 will get you a direct download link so it is not listed here.

![Visual Studio Installed Image](docs/www/vs2022.png)

To get the `check` dependency you need to install `vcpkg` as described here: https://learn.microsoft.com/en-us/vcpkg/get_started/get-started-msbuild?pivots=shell-powershell

Setting the env var `VCPKG_ROOT` and running the first command sets up the local repo as described in the instructions above. The project uses vcpkg manifest mode (via `build/vs2022/vcpkg.json` and `Directory.Build.props`) to automatically install the `check` unit testing framework. vcpkg will install the appropriate architecture (x64-windows or x86-windows) based on your build configuration. Note that the vcpkg package provides the library as `checkDynamic.lib`. If you are behind a proxy or basic auth proxy and it fails to pull your dependency you can use the *Developer Command Prompt for VS 2022* or *Developer PowerShell for VS 2022* and set the proxy as follows depending on your proxy setup.

```sh
set http_proxy=http://user:pass@host:port
set https_proxy=http://user:pass@host:port
devenv
```
Visual Studio picks up those variables so then it compiles within the app. If you use a normal *Command* or *PowerShell* the use the following:

```sh
start devenv
```

If you don't have access to the Windows store the `vcpkg` bootstrap will fail so you should download the latest version from https://github.com/microsoft/vcpkg-tool. You should check the SHA and then make the executable runnable. Hint: `c:\Users\<user>\.local\bin` is typically on the path by default.

### contiki-ng (IoT support)

Refer to the Github actions for more info on how this version is built. The `contiki-ng` community can be found starting from this link: https://www.contiki-ng.org/

To run the build for `contiki-ng`:

```sh
$ cd build/gcc
$ make TARGET=zoul clean all
```
### Documentation Dependency

In order to `make` the documentation you need to install [doxygen](https://www.doxygen.nl/).

Use one of the following commands to install `doxygen`:

#### macOS

```sh
$ brew install doxygen
```

#### Linux (Ubuntu/Debian)

```sh
$ apt-get install doxygen
```

#### Windows

Refer to https://www.doxygen.nl/download.html to install doxygen for Windows to create the documentation.

### Building on macOS and Linux

The following commands will allow you to build everything. There is also a `dynlib` Make target that can be used to create a shared library.

```sh
$ cd build/gcc
$ make TARGET=pc clean all check examples utils doc
```

### Building on Windows

The normal way to build is to open Visual Studio and navigate to `build\vs2022` and then open the `exip.sln` solution file. Once the project is loaded you can right click to build or use the menus. Individual projects can be cleaned and compiled as well.

Note that the option `/FS` is added to the build as the different projects share the same output directory. This was the setup in `vs2010` that was upgraded so this was not changed. The extra `/FS` option can be seen in the `Configuration Properties / C/C++ / Command Line` menu. If you prefer to build on the command line, refer to the Github action and this will also provide clues on how to run the tests.

#### Running tests Using Visual Studio

The tests (`check_*`) are individual projects so you can right click on the project and select `Debug -> Start Without Debugging` to run the unit test. If the unit test requires test files and doesn't run first Right Click on the project and select `Properties`, Second, at the top select ` Configuration -> All Configurations`. And next in the left tree select `Configuration Properties -> Debugging`. Finally, in `Command Arguments`, add `../../tests/test-set` to give the path to the executable. The test source includes specific test files which are appended to this path.

Once the project is built you can run the following `bat` file script from the root of the project using the option to test in `Release` mode. The default run the `debug` build.

```bat
scripts/run-unit-tests.bat [Release|release]
```

### Build System

EXIP uses a simple Makefile-based build system that aligns with the project's embedded systems focus. The `Makefile` includes architecture-specific [detection code](https://stackoverflow.com/questions/714100/os-detecting-makefile) to support Linux and macOS without requiring additional dependencies. This approach ensures the build system works across desktop platforms (for development) and embedded targets like Contiki-ng (Zoul) where heavyweight build tools may not be available. The simplicity and universality of `make` makes it practical for resource-constrained build environments.
