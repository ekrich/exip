# Developer Information

The following sections are for Users and Contributors to EXIP that need tools installed to use the library. There are no prepared packages that can be installed without compiling the code.

## Miscellaneous

Architecture specific [code](https://stackoverflow.com/questions/714100/os-detecting-makefile) is added to the `Makefile` to support Linux and macOS which was newly added.

Perhaps a longer term solution would be to use `CMake` so that the build could be cross platform in a nicer manner. Incidentally, `vcpkg` used for Windows is also cross platform so it could be an option although installing via the OS platform dependency manager is also very convenient. The only reservation is that `make` is more universal.

## VSCode Support

[VScode](https://code.visualstudio.com/) is supported by adding the [clangd](https://clangd.llvm.org/) plugin to get C Language Server Protocol (LSP) support. Find the official plugin [here](https://marketplace.visualstudio.com/items?itemName=llvm-vs-code-extensions.vscode-clangd).

The macOS `arm` processors have a non-standard include directory so there needs to be an entry in the `compile_flags.txt` file for `clangd` to support the Check library. This file is at the root of the project.


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

The easiest setup is to install Visual Studio Community Edition 2022 to get the latest Windows 10/11 SDK and the MSVC v143 tools. The following is the installer setup used for the current vs2022 setup that succeeded. Searching for vs2022 will get you a direct download link so it is not listed here.

![Visual Studio Installed Image](docs/www/vs2022.png)

To get the `check` dependency we need to install `vcpkg` as described here: https://learn.microsoft.com/en-us/vcpkg/get_started/get-started-msbuild?pivots=shell-powershell

Setting the env var `VCPKG_ROOT` and running the first command sets up the local repo as described in the instructions above. This seems to be a good practice. The project uses vcpkg manifest mode (via `build/vs2022/vcpkg.json`) to automatically install the `check` unit testing framework. vcpkg will install the appropriate architecture (x64-windows or x86-windows) based on your build configuration. Note that the vcpkg package provides the library as `checkDynamic.lib`. If you are behind a proxy or basic auth proxy and it fails to pull your dependency you can use the *Developer Command Prompt for VS 2022* or *Developer PowerShell for VS 2022* and set the proxy as follows depending on your proxy setup.

```sh
set http_proxy=http://user:pass@host:port
set https_proxy=http://user:pass@host:port
devenv
```
Visual Studio picks up those variables so then it compiles within the app. If you use a normal *Command* or *PowerShell* the use the following:

```sh
start devenv
```

If you don't have access to the Windows store the `vcpkg` bootstrap will fail so you should download the latest version from https://github.com/microsoft/vcpkg-tool The version used here was `2024-11-12` for reference. You should check the SHA and then make the executable runnable. Hint: `c:\Users\<user>\.local\bin` is typically on the path by default.

Note: In vs2022, you can use the menu *Tools -> Options* and type in *vcpkg* for settings related to *vcpkg*.

## contiki-ng (IoT support)

Refer to the Github actions for more info on how this version is built. The `contiki-ng` community can be found starting from this link: https://www.contiki-ng.org/

To run the build for `contiki-ng`:

```sh
$ cd build/gcc
$ make TARGET=zoul clean all
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

### Windows

Refer to https://www.doxygen.nl/download.html to install doxygen for Windows to create the documentation.

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

The tests (`check_*`) are individual projects so you can right click on the project and select `Debug -> Start Without Debugging` to run the unit test. If the unit test requires test files and doesn't run first Right Click on the project and select `Properties`, Second, at the top select ` Configuration -> All Configurations`. And next in the left tree select `Configuration Properties -> Debugging`. Finally, in `Command Arguments`, add `../../tests/test-set` to give the path to the executable. The test source includes specific test files which are appended to this path.

Once the project is built you can run the following `bat` file script from the root of the project using the option to test in `Release` mode. The default run the `debug` build.

```bat
scripts/run-unit-tests.bat [Release|release]
```
