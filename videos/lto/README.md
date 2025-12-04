# LTO Experiments
## Goal
Explores potential performance improvements using link time optimization (LTO) by benchmarking various configurations/implementations of a basic kinematic problem.

Notably, how common refactoring can "hide" context from the optimizer resulting in worse performance than expected.

## Structure
Independent folders for each type of implementation, such as `01_aos` and `05_aligned`, that are compiled into combined benchmark applications. Separate binaries are generated without LTO (`default`) and with LTO (`lto`).

## Summary of findings
When iterating over a collection of `Point`s that have an `Update` method defined in another translation unit (roughly cpp file) there is as much as a 10x performance difference between with and without LTO. This is the case with implementations such as `01_aos`.

When calling `Update()` on a `Points` class where the underlying collection, loop, and `Update` method are all in the same translation unit, there is negligible performance difference between with and without LTO. This is the case with implementations such as `05_aligned`.

More details are covered in the videos linked in the base `README.md`.

## Dependencies
[Google benchmark](https://github.com/google/benchmark) for, well, benchmarking. Mostly tested while installed separately on the system, but will otherwise try to fetch with `FetchContent`.

## Building
Built using CMake, with optional presets for `debug` and `release`. Both presets respect `CXX` to optionally change the desired compiler. Only tested on Linux with `g++` and `clang++`.
```
$ cmake --list-presets
Available configure presets:

  "debug"   - debug
  "release" - release
```

Building with a preset configuration:
```
$ cmake --preset release
$ cmake --build out/build/release
```

Building with a manual configuration:
```
$ CXX=clang++ CMAKE_BUILD_TYPE=RelWithDebInfo cmake -S. -Bmybuild
$ cmake --build mybuild
```

## Running
Both the `default` and `lto` binaries can be run like any other Google benchmark program.
```
$ ./out/build/release/default [standard_benchmark_args]
$ ./out/build/release/lto [standard_benchmark_args]
```

