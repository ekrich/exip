# EXIP - Embeddable EXI Processor in C

This is just a placeholder for new information. Refer to the original
[README.txt](README.txt)

# Developer Information

The following section is for new contributors to EXIP.

## VSCode Support

[VScode](https://code.visualstudio.com/) is supported by adding the [clangd](https://clangd.llvm.org/) plugin to get C Language Server Protocol (LSP) support. Find the official plugin [here](https://marketplace.visualstudio.com/items?itemName=llvm-vs-code-extensions.vscode-clangd).


## Testing Dependency

The Check Unit Test Framework for C for testing. Link to the [Check](https://libcheck.github.io/check/) library. Please see the install notes [here](https://libcheck.github.io/check/web/install.html).

### macOS

Using the following command work to install `Check`:

```sh
$ brew install check
```

The macOS `arm` processors have a non-standard include directory so there needs to be an entry into the `compile_flags.txt` file for `clangd` to support the Check library. This is at the root of the project.

Adding architecture specific [code](https://stackoverflow.com/questions/714100/os-detecting-makefile) to the `Makefile` may not be so good but for now it works.

Compiles on macOS M1 using Ventura 13.6.7 and Xcode command line tools.

## Documentation Dependency

In order the make the documentation you need to install [doxygen](https://www.doxygen.nl/).

## macOS

Using the following command work to install `doxygen`:

```sh
$ brew install doxygen
```
