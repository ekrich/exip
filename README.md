# EXIP - Embeddable EXI Processor in C

![CI](https://github.com/ekrich/exip/workflows/CI/badge.svg)

## Current status

Known to pass CI with the following configurations:

| OS         | Works | Version(s)              | Arch(s)             | Compiler(s)
| ---------- | ----- | ------------------------| --------------------|-----------------|
| macOS      |   ✅  | 13.6.x                  | arm                 | System          |
| Linux      |   ✅  | Ubuntu 22.0.4           | x86_64              | gcc             |

* The build system calls `gcc`.
* macOS system compiler `gcc` is aliased to `clang`.

This is new information. Refer to the original
[README.txt](README.txt) for more information. Other links will be added in the future.

# Developer Information

The following sections are for new contributors to EXIP or people wanting to try out the latest bits.

## Miscellaneous

The macOS `arm` processors have a non-standard include directory so there needs to be an entry in the `compile_flags.txt` file for `clangd` to support the Check library. This is at the root of the project.

Adding architecture specific [code](https://stackoverflow.com/questions/714100/os-detecting-makefile) to the `Makefile` may not be so good but for now it works.

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

## Building

The following commands will allow you to build everything. There is also a `dynlib` Make target that can be used to create a shared library.

```sh
$ cd build/gcc
$ make TARGET=pc clean all check examples utils doc
```
