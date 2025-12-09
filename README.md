# EXIP - Embeddable EXI Processor in C

![CI](https://github.com/ekrich/exip/workflows/CI/badge.svg)

![EXIP Logo](doc/www/logo.png)

## Current status

This project was not maintained on [SourceForge](https://sourceforge.net/projects/exip/). The code was pulled from the SourceForge SVN repository with all commits and tags. There was [one outstanding issue](https://github.com/ekrich/exip/issues/1) that has been fixed and of the rest of the work to date is to support the platforms shown in the table below. The project originally supported the embedded IoT [Contiki](https://github.com/contiki-ng/contiki-ng) platform but this support has not been addressed to date. See the following original links for more information:

* [Original README.txt](README.txt)
* [Original Website](https://exip.sourceforge.net/)

Known to pass CI with the following configurations:

| OS         | Works | Version(s)              | Arch(s)             | Compiler(s)
| ---------- | ----- | ------------------------| --------------------|-----------------|
| macOS      |   ✅  | 13.6.x                  | arm                 | gcc             |
| Linux      |   ✅  | Ubuntu 22.0.4           | x86_64              | gcc             |
| Windows    |   ✅  | Visual Studio 2022      | x86/x86_64          | cl              |

* The build system calls `gcc`.
* macOS system compiler `gcc` is aliased to `clang`.
* `clang` should also work on Linux.
* Windows default creates a 32bit x86 executable.
* We now use C99 standard as we have refactored to use `bool`.

# Developer Information

The following sections are for new contributors to EXIP or people wanting to try out the latest bits.

## Miscellaneous

The macOS `arm` processors have a non-standard include directory so there needs to be an entry in the `compile_flags.txt` file for `clangd` to support the Check library. This files is at the root of the project.

Adding architecture specific [code](https://stackoverflow.com/questions/714100/os-detecting-makefile) to the `Makefile` may not be so good but for now it works.

Probably a longer term solution would be to use `CMake` so that the build could be cross platform in a nicer manner. Incidentally, `vcpkg` used for Windows is also cross platform so it could be an option although installing via the platform dependency manager is very convenient.

## VSCode Support

[VScode](https://code.visualstudio.com/) is supported by adding the [clangd](https://clangd.llvm.org/) plugin to get C Language Server Protocol (LSP) support. Find the official plugin [here](https://marketplace.visualstudio.com/items?itemName=llvm-vs-code-extensions.vscode-clangd).


## Testing Dependency

The Check Unit Test Framework for C for testing. Link to the [Check](https://libcheck.github.io/check/) library. Please see the install notes [here](https://libcheck.github.io/check/web/install.html).

Use one of the following commands to install `Check`:

### macOS

```sh
$ brew install check
```

### Linux (Ubuntu/Debian)

```sh
$ apt-get install check
```

### Windows

The easiest setup is to install Visual Studio 2022 to get the latest Windows 10 SDK and the MSVC v143 tools. The following is the installer setup used for the current vs2022 setup that succeeded.

![Visual Studio Installed Image](doc/www/vs2022.png)

To get the `check` dependency we need to install `vcpkg` as described here: https://learn.microsoft.com/en-us/vcpkg/get_started/get-started-msbuild?pivots=shell-powershell

Setting the env var `VCPKG_ROOT` and running the first command sets up the local repo as described in the instructions above. This seems to be a good practice. The project installs `check:x86-windows` via the `vcpkg.json`. If behind a basic auth proxy and it fails to pull your dependency you can use the Developer Command prompt from vs2022 set the proxy as follows depending on your proxy setup.

```sh
set http_proxy=http://user:pass@host:port
set https_proxy=http://user:pass@host:port
devenv
```
Visual Studio seems the pick that up those variables so then it compiles within the app.

Note: Attemped to use static linking and `clang` in the setup but failed pretty badly. Used these below. The seems to be `-clang` artifacts add `x86` is the default build architecture so maybe those libraries might make more sense.

```sh
vcpkg install check:x64-windows-static
vcpkg install check:x64-windows
```

For future potential `cmake` setup.

```
  find_package(check CONFIG REQUIRED)
  target_link_libraries(main PRIVATE $<IF:$<TARGET_EXISTS:Check::check>,Check::check,Check::checkShared>)

```

If you don't have access to the Windows store the bootstrap will fail so you should download the the latest from https://github.com/microsoft/vcpkg-tool The version used here was `2024-11-12` for reference. You should check the SHA and then make the executable runnable.

## Documentation Dependency

In order to `make` the documentation you need to install [doxygen](https://www.doxygen.nl/).

Use one of the following commands to install `doxygen`:

### macOS

```sh
$ brew install doxygen
```

### Linux (Ubuntu/Debian)

```sh
$ apt-get install doxygen
```

### Windows

Refer to https://www.doxygen.nl/download.html to install doxygen for Windows to create the documentation. Recommended to use `vcpkg` for dependencies in general.

## Building on macOS and Linux

The following commands will allow you to build everything. There is also a `dynlib` Make target that can be used to create a shared library.

```sh
$ cd build/gcc
$ make TARGET=pc clean all check examples utils doc
```

## Building on Windows

The normal way to build is to open Visual Studio and navigate to `build\vs2022` and then open the `exip.sln` solution file. Once the project is loaded you can right click to build or use the menus. Individual projects can be cleaned and compiled as well.

Note that the option `/FS` is added to the build as the different projects share the same output directory. This was the setup in `vs2010` that was upgraded so this was not changed. The extra `/FS` option can be seen in the `Configuration Properties / C/C++ / Command Line` menu. If you prefer to build on the command line, refer to the Github action and this will also provide clues on how to run the tests.

### Running tests Using Visual Studio

The tests (`check_*`) are individual projects so you can right click on the project and select `Debug -> Start Without Debugging` to run the unit test. If the unit test requires test files and doesn't run you can Right Click on the project and select `Properties -> Configuration Properties -> Debugging` and then add in `Command Arguments`, `../../tests/test-set` to give the path to the executable. The test source includes specific test files which are appended to this path.

Once the project is built you can run the following `bat` file script from the root of the project.

```bat
scripts/run-unit-tests.bat
```
=======
## Licenses

 - Original license: [EISLAB](LICENSE-orig.txt)
 - Additions: [Apache License, Version 2.0](LICENSE-2.0.txt)
