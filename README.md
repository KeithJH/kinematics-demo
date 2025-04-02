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

## Implementation Notes
### `kinematics`
* [VectorOfStructSim](./notes/kinematics/VectorOfStructSim.md): Conventional Array of Structures (AoS) layout using a `std::vector<Body>`. This means data for various fields is interleaved in memory, which can present a challenge for vectorization.
* [StructOfVectorSim](./notes/kinematics/StructOfVectorSim.md): Structure of Arrays (SoA) style layout using parallel `std::vector<float>` fields. This means data for a particular field is entirely contiguous in memory which typically allows for easier vectorization.
* [OmpSimdSim](./notes/kinematics/OmpSimdSim.md): Same layout as `StructOfVectorSim`, but uses OpenMP for vectorizing code.
* [StructOfArraySim](./notes/kinematics/StructOfArraySim.md): SoA layout that uses `std::array<float, MAX_SIZE>` fields. Data for particular fields are entirely contiguous with a compile-time cap on data size.
* [StructOfPointerSim](./notes/kinematics/StructOfPointerSim.md): SoA layout that uses `float*` fields manually managed with `new[]` and `delete[]`.
* [StructOfAlignedSim](./notes/kinematics/StructOfAlignedSim.md): SoA layout that uses `float*` fields manually managed with `new[]` and `delete[]` while specifying alignment.
* [StructOfOversizedSim](./notes/kinematics/StructOfOversizedSim.md): SoA layout that uses `float*` fields manually managed with `new[]` and `delete[]` while specifying alignment and ensuring adequate capacity that allows for vector commands to "overrun" the actual amount of `Bodies` in the simulation to avoid non-vectorized "tail" calculations.

### `minimal`
* [VectorOfStruct](./notes/minimal/VectorOfStruct.md): Conventional AoS layout using a `std::vector<Point>`. This means data for various fields is interleaved in memory, which can present a challenge for vectorization.
* [VectorOfLargeStruct](./notes/minimal/VectorOfLargeStruct.md): Conventional AoS layout using a `std::vector<Point>`. Incorporates unused fields to mimic data that may be used in a larger application, which reduces the amount of "tricks" that can be used to still vectorize with interleaved data.
* [StructOfVector](./notes/minimal/StructOfVector.md): SoA style layout using parallel `std::vector<float>` fields. This means data for a particular field is entirely contiguous in memory which typically allows for easier vectorization.
* [StructOfPointer](./notes/minimal/StructOfPointer.md): SoA layout that uses `float*` fields manually managed with `new[]` and `delete[]`.
* [Aligned](./notes/minimal/Aligned.md): SoA layout that uses `float*` fields manually managed with `new[]` and `delete[]` while specifying alignment.
* [Oversized](./notes/minimal/Oversized.md): SoA layout that uses `float*` fields manually managed with `new[]` and `delete[]` while specifying alignment and ensuring adequate capacity that allows for vector commands to "overrun" the actual amount of `Points` in the calculation to avoid non-vectorized "tail" calculations.

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
The `Makefile` provides the following targets:
* `all` (default): compiles all the implementations
* `clean`: removes the created binaries
* `run`: runs each of the implementations with default arguments

And uses the following optional variables:
* `CXX`: The C++ compiler to use (or system default)
* `CXX_FLAGS`: The flags to pass to the compiler (or system default)
* `OUT`: Directory (or default of `./out`)

Example:
```
$ make OUT="./custom" CXX="clang++" CXX_FLAGS="-Os" all
```

To aid in comparing various compiling options the bash script `make.sh` invokes `make` with multiple combinations of compilers and flags. The script also supports the same targets as the `Makefile`.

Example:
```
$ ./make.sh all
```

## Running
### `gui`
The application has one optional command line argument to specify the number of bodies to start the simulation with, otherwise the simulation starts with a single body. If the initial count is high enough rendering of the bodies is also disabled.

```
$ ./out/build/release/gui/kinematics-demo-gui [number_of_initial_bodies]
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
If compiling with the given `Makefile` or `make.sh`, one can use the `run` target to run each of the implementations with the default arguments that runs 10,000 update loops of 1,000,000 points.
```
$ make run
# OR
$ ./make.sh run
```

If running a given implementation from the command line one can specify the number of update loops and number of points such as:
```
$ ./out/Oversized [<number_of_points> [<number_of_update_loops>]]
```
