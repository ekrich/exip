# EXIP - Embeddable EXI Processor in C

![CI](https://github.com/ekrich/exip/workflows/CI/badge.svg)

## Current status

Known to pass CI with the following configurations:

| OS         | Works | Version(s)              | Arch(s)             | Compiler(s)
| ---------- | ----- | ------------------------| --------------------|-----------------|
| macOS      |   ✅  | 13.6.x                  | arm                 | System          |
| Linux      |   ✅  | Ubuntu 22.0.4           | x86_64              | gcc             |
| Windows    |   ✅  | Visual Studio 2019/2022 | x86/x86_64          | cl              |

* The build system calls `gcc`.
* macOS system compiler `gcc` is aliased to `clang`.
* Windows default creates a 32bit x86 executable.

This is new information. Refer to the original
[README.txt](README.txt) for more information. Other links will be added in the future.

# Developer Information

The following sections are for new contributors to EXIP or people wanting to try out the latest bits.

## Miscellaneous

The macOS `arm` processors have a non-standard include directory so there needs to be an entry in the `compile_flags.txt` file for `clangd` to support the Check library. This is at the root of the project.

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

The easiest setup is to install Visual Studio 2019 to get the latest Windows 10 SDK and the MSVC v142 tools. The following is the installer setup used for the current setup that succeeded.

![Visual Studio Installed Image](doc/images/VS2019.png)

To get the `check` dependency we need to install `vcpkg` as described here: https://learn.microsoft.com/en-us/vcpkg/get_started/get-started-msbuild?pivots=shell-powershell

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

The normal way to build is to open Visual Studio and navigate to `build\vs2019` and then open the `exip.sln` solution file. Once the project is loaded you can right click to build or use the menus. Individual projects can be cleaned and compiled as well.

Note that the option `/FS` is added to the build as the different projects share the same output directory. This was the setup in `vs2010` that was upgraded so this was not changed. The extra `/FS` option can be seen in the `Configuration Properties / C/C++ / Command Line` menu. If you prefer to build on the command line, refer to the Github action and this will also provide clues on how to run the tests.
