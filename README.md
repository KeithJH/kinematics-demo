# kinematics-demo
## Goal
Inspired by "Bunnymark" benchmarks, such as the [textures example from raylib](https://www.raylib.com/examples/textures/loader.html?name=textures_bunnymark), this project aims to dive into the rabbit hole of optimization topics around the kinematics of simple 2D motion. This is not the limiting factor for most such benchmarks, but presents an interesting opportunity to compare implementations, compilers, and compiler flags. Of particular interest when starting the project was compilers' ability to auto-vectorize code depending on how the data is stored in memory.

## Structure
### `kinematics`
Library with full implementations for drawing and updating a simulation of numerous bodies (circles) that bounce around the environment.

### `gui`
Interactive application to explore the performance of a `kinematics` implementation with or without rendering the bodies to screen.

### `bench`
Benchmarking application for `kinematics` implementations without any graphical component.

### `minimal`
Small, independent, (re)implementations scoped down to more easily inspect the generated executables/rough timing.

## Building
Most of the project is set up with `CMake` using `FetchContent` for a few packages.
* [`Catch2`](https://github.com/catchorg/Catch2) is used for benchmarking and a touch of testing
* [`raylib`](https://github.com/raysan5/raylib) is used for graphics

Implementations in `minimal` do not have any dependencies and can be easily compiled with `make` or standalone compilers.

### `kinematics`, `gui`, `bench`
Linux presets are created for `g++` and `clang++`:
```
$ cmake --list-presets
Available configure presets:

  "debug"   - Linux GCC Debug
  "release" - Linux GCC Release
  "clang"   - Linux Clang Release
```

The project can be configured with:
```
$ cmake --preset release
```

The project can then be built with:
```
$ cmake --build out/build/release/
```

### `minimal`
The provided `Makefile` will build a variety of configurations using `g++` and `clang++`, with and without the flag `-march=native`; however, as all the implementations are independent it should be easy enough to build any given one with a compiler and flags of choice.

## Running
### `gui`
The application does not take any command line arguments and can be started simply with:
```
$ ./out/build/release/gui/kinematics-demo-gui
```

Once running there are keyboard actions to interact with the simulation:
* `s`: Toggle a more in-depth stat screen
* `r`: Toggle drawing the circles to screen
* `u`: Toggle calculations for updating the positions of bodies
* `1` - `0`: Set the number of bodies to to 1 through 10
* `Numpad 1` - `Numpad 0`: Set the number of bodies to 1 * 100,000 through 10 * 100,000
* `F1` - `F10`: Set the number of bodies to 1 * 1,000,000 through 10 * 1,000,000

### `bench`
Benchmarking is done via `Catch2` so all the common arguments work as expected. A good starting point is something like:
```
$ ./out/build/release/bench/kinematics-demo-bench --benchmark-no-analysis
```

### `minimal`
If compiling with the given `Makefile` one can run `make run` to run each of the implementations with the default arguments that runs 10,000 update loops of 1,000,000 points. If running a given implementation from the command line you can specify the number of update loops and number of points such as:
```
$ ./out/Oversized.native.gcc [<number_of_points> [<number_of_update_loops>]]
```
