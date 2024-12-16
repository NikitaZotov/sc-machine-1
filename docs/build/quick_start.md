# Quick start

This guide provides short information for developers to start to work with sc-machine quickly. You can always learn more about the sc-machine's [build system](build_system.md).

## Check CMake

Ensure you are using **CMake version 3.24** or newer. Verify your version with:

```sh
cmake --version
```

To upgrade CMake, follow the installation guide appropriate for your OS or use:
  
```sh
# Use pipx to install cmake if not already installed
# Install pipx first using guide: https://pipx.pypa.io/stable/installation/
pipx install cmake
```

## Start develop sc-machine with Conan

### Install Conan

Install Conan, to build sc-machine dependencies with Conan-provided dependencies:

```sh
# Use pipx to install conan if not already installed
pipx install conan
```

### Use sc-machine in Debug

#### Build sc-machine in Debug

To build sc-machine in debug mode using Conan-provided dependencies, run:

```sh
# debug build type
cmake --preset debug-conan
cmake --build --preset debug
```

Note: By default, configure preset `debug` enables building sc-machine tests.

#### Run sc-machine tests in Debug

After that, you can go to `build/Debug` and run tests via `ctest`:

```sh
cd build/Debug
ctest -V
```

#### Run sc-machine in Debug

```sh
# create empty knowledge base sources folder
mkdir kb
# note: at this stage you can move your KB sources to the ./kb folder

# build knowledge base
./build/Debug/bin/sc-builder -i kb -o kb.bin --clear
# run sc-machine
./build/Debug/bin/sc-machine -e build/Debug/lib/extensions -s kb.bin
```

### Use sc-machine in Release

#### Build sc-machine in Release

To build sc-machine in release mode using Conan-provided dependencies, run:

```sh
# release build type without tests
cmake --preset release-conan
cmake --build --preset release
```

To build sc-machine with tests in release mode using Conan-provided dependencies, run:

```sh
# release build type with tests
cmake --preset release-with-tests-conan
cmake --build --preset release
```

#### Run sc-machine tests in Release

After that, you can run tests:

```sh
cd build/Release
ctest -V
```

#### Run sc-machine in Release

```sh
# create empty knowledge base sources folder
mkdir kb
# note: at this stage you can move your KB sources to the ./kb folder

# build knowledge base
./build/Release/bin/sc-builder -i kb -o kb.bin --clear
# run sc-machine
./build/Release/bin/sc-machine -e build/Release/lib/extensions -s kb.bin
```

You can also check code formatting, build sc-machine with sanitizers and other. To learn more, go to the [CMake flags](cmake_flags.md) page.

## Start develop sc-machine with system-provided dependencies

### Install sc-machine dependencies

Note: sc-machine build system supports installation of dependencies for Ubuntu and macOS only.

#### Install sc-machine dependencies for Ubuntu

```sh
cd scripts
./install_deps_ubuntu.sh
```

#### Install sc-machine dependencies for macOS

```sh
cd scripts
./install_deps_macOS.sh
```

#### Install sc-machine dependencies for other OS

Currently, we require the following packages to be available to CMake at build-time:

- `java`
- `glib2`
- `websocketpp`
  - [`asio`](https://think-async.com/Asio/) as the transitive dependency
- `nlohmann_json`
- `xml2`

You can try install these dependencies on your OS.

### Use sc-machine in Debug

#### Build sc-machine in Debug

To build sc-machine in debug mode using system-provided dependencies, run:

```sh
# debug build type
cmake --preset debug
cmake --build --preset debug
```

Note: By default, configure preset `debug` enables building sc-machine tests.

#### Run sc-machine tests in Debug

After that, you can go to `build/Debug` and run tests via `ctest`:

```sh
cd build/Debug
ctest -V
```

#### Run sc-machine in Debug

```sh
# create empty knowledge base sources folder
mkdir kb
# note: at this stage you can move your KB sources to the ./kb folder

# build knowledge base
./build/Debug/bin/sc-builder -i kb -o kb.bin --clear
# run sc-machine
./build/Debug/bin/sc-machine -e build/Debug/lib/extensions -s kb.bin
```

### Use sc-machine in Release

#### Build sc-machine in Release

To build sc-machine in release mode using system-provided dependencies, run:

```sh
# release build type without tests
cmake --preset release
cmake --build --preset release
```

To build sc-machine with tests in release mode using system-provided dependencies, run:

```sh
# release build type with tests
cmake --preset release-with-tests
cmake --build --preset release
```

#### Run sc-machine tests in Release

After that, you can run tests:

```sh
cd build/Release
ctest -V
```

#### Run sc-machine in Release

```sh
# create empty knowledge base sources folder
mkdir kb
# note: at this stage you can move your KB sources to the ./kb folder

# build knowledge base
./build/Release/bin/sc-builder -i kb -o kb.bin --clear
# run sc-machine
./build/Release/bin/sc-machine -e build/Release/lib/extensions -s kb.bin
```

You can also check code formatting, build sc-machine with sanitizers and other. To learn more, go to the [CMake flags](cmake_flags.md) page.